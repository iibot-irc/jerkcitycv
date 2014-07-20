#include "context.h"

#include <opencv2/nonfree/nonfree.hpp>

void doSIFTStuff(const cv::Mat img, std::vector<cv::KeyPoint>& outKeypoints, cv::Mat&  outDescriptors) {
  cv::SiftFeatureDetector detector{400};
  detector.detect(img, outKeypoints);

  cv::SiftDescriptorExtractor extractor;
  extractor.compute(img, outKeypoints, outDescriptors);
}

struct ActorTemplate {
  ActorTemplate(const Template& genericTemplate) {
    doSIFTStuff(genericTemplate.img, keypoints, descriptors);
    name = genericTemplate.name;
  }

  std::vector<cv::KeyPoint> keypoints;
  cv::Mat descriptors;
  std::string name;
};

void attributeDialog(Context& ctx) {
  auto actorTmpls = std::vector<ActorTemplate>{};
  {
    auto genericTmpls = loadTemplates("actors");
    actorTmpls = std::vector<ActorTemplate>(genericTmpls.begin(), genericTmpls.end());
  }

  for (auto&& panel : ctx.panels) {
    for (auto&& bubble : panel.dialog) {
      bubble.actor = "unknown";
    }
  }
}
