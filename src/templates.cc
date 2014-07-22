#include <boost/filesystem.hpp>

#include "context.h"

namespace fs = boost::filesystem;

std::vector<Template> loadTemplates(const std::string& pathStr) {
  auto path = fs::path{pathStr};

  if (!fs::exists(path) || !fs::is_directory(path)) {
    throw std::runtime_error{"bad template src path " + pathStr};
  }

  std::vector<Template> templates;
  for (auto fIt = fs::directory_iterator{path}; fIt != fs::directory_iterator{};
       ++fIt) {
    auto file = fs::path{*fIt};
    if (!fs::is_regular_file(file) || file.extension() != ".png") {
      continue;
    }

    auto img = cv::imread(file.string(), CV_LOAD_IMAGE_GRAYSCALE);
    if (img.empty()) {
      throw std::runtime_error{"Could not open template " + file.string()};
    }

    auto label = file.stem().string();
    label = label.substr(0, label.find("."));
    label = label == "dot" ? "." : label;  // This is a hack for the "." char

    templates.emplace_back(label, img);
  }

  return templates;
}
