#include "context.h"

#include <opencv2/nonfree/nonfree.hpp>

void findFeatures(cv::Mat img, std::vector<cv::KeyPoint>& outKeypoints, cv::Mat& outDescriptors) {
  const int kMinHessian = 400;
  auto detector = cv::SurfFeatureDetector{kMinHessian};
  detector.detect(img, outKeypoints);

  auto extractor = cv::SurfDescriptorExtractor{};
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

std::string findActor(Context& ctx, cv::Mat img, const std::vector<ActorTemplate>& actorTmpls) {
  auto keypoints = std::vector<cv::KeyPoint>{};
  auto descriptors = cv::Mat{};

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
    float minDist = std::min(0.25f, std::max(2.0f*bounds.first->distance, 0.02f));

    auto goodMatches = std::vector<cv::DMatch>{};
    std::copy_if(matches.begin(), matches.end(), std::back_inserter(goodMatches), [minDist](const auto& m) {
      return m.distance <= minDist;
    });

    double score = (double)goodMatches.size()/(double)matches.size();
    if (score > 0.25f) {
      actorMatches.emplace_back(score, actor);
      //std::cerr << actor.name << ": " << goodMatches.size() << " out of " << matches.size() << ": " << score << "\n";
    }
  }

  if (actorMatches.empty()) {
    return "unknown";
  }

  return std::max_element(actorMatches.begin(), actorMatches.end(), [](const auto& a, const auto& b) { return a.first < b.first; })->second.name;
}

void attributeDialog(Context& ctx) {
  auto actorTmpls = std::vector<ActorTemplate>{};
  {
    auto genericTmpls = loadTemplates("actors");
    actorTmpls = std::vector<ActorTemplate>(genericTmpls.begin(), genericTmpls.end());
  }

  for (size_t i = 0; i < ctx.panels.size(); i++) {
    const auto& panel = ctx.panels[i];

    // TODO: get better window to search in. for now, we use entire panel below the top 1/3rd
    auto panelBounds = panel.bounds;
    panelBounds.y += panel.bounds.height / 3;
    panelBounds.height *= 2.0f/3.0f;
    auto panelImg = cv::Mat{ctx.img, panelBounds};

    for (auto&& bubble : ctx.panels[i].dialog) {
      bubble.actor = findActor(ctx, panelImg, actorTmpls);
    }

  }
}
