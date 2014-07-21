#include "context.h"

#include <opencv2/nonfree/nonfree.hpp>

void findFeatures(cv::Mat img, std::vector<cv::KeyPoint>& outKeypoints, cv::Mat& outDescriptors) {
  auto detector = cv::SiftFeatureDetector{};
  detector.detect(img, outKeypoints);

  auto extractor = cv::SiftDescriptorExtractor{};
  extractor.compute(img, outKeypoints, outDescriptors);
}

struct ActorTemplate {
  ActorTemplate(const Template& genericTemplate) {
    findFeatures(genericTemplate.img, keypoints, descriptors);
    ASSERT(!descriptors.empty(), "no features detected in this actor template!");
    name = genericTemplate.name;
    img = genericTemplate.img;
  }

  cv::Mat img;
  std::vector<cv::KeyPoint> keypoints;
  cv::Mat descriptors;
  std::string name;
};

std::string findActor(cv::Mat img) {
  auto keypoints = std::vector<cv::KeyPoint>{};
  auto descriptors = cv::Mat{};

  static auto actorTmpls = std::vector<ActorTemplate>{};
  if (actorTmpls.empty()) {
    auto genericTmpls = loadTemplates("actors");
    actorTmpls.reserve(2*genericTmpls.size() + 1);
    for (auto&& tmpl : genericTmpls) {
      actorTmpls.emplace_back(tmpl);
      auto tmplFlipped = tmpl;
      tmplFlipped.img = cv::Mat{};
      cv::flip(tmpl.img, tmplFlipped.img, 1);
      actorTmpls.emplace_back(tmplFlipped);
    }
  }

  findFeatures(img, keypoints, descriptors);

  if (descriptors.empty()) {
    return "unknown";
  }

  std::vector<std::pair<double, const ActorTemplate&>> actorMatches;

  for (const auto& actor : actorTmpls) {
    auto matcher = cv::FlannBasedMatcher{};
    auto matches = std::vector<cv::DMatch>{};
    matcher.match(actor.descriptors, descriptors, matches);

    auto bounds = std::minmax_element(matches.begin(), matches.end(), [](const auto& a, const auto& b) {
      return a.distance < b.distance;
    });
    float minDist = std::min(150.0f, std::max(3.0f*bounds.first->distance, 0.5f));

    auto goodMatches = std::vector<cv::DMatch>{};
    std::copy_if(matches.begin(), matches.end(), std::back_inserter(goodMatches), [minDist](const auto& m) {
      return m.distance <= minDist;
    });

    if (goodMatches.size() < 2) {
      continue;
    }

    double score = (double)goodMatches.size()/(double)matches.size();
    if (score > 0.02f) {
      actorMatches.emplace_back(score, actor);
      //std::cerr << actor.name << ": " << goodMatches.size() << " out of " << matches.size() << ": " << score << "\n";
    }
  }

  if (actorMatches.empty()) {
    return "unknown";
  }

  return std::max_element(actorMatches.begin(), actorMatches.end(), [](const auto& a, const auto& b) { return a.first < b.first; })->second.name;
}

