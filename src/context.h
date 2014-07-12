#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include <map>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

inline void threshold(cv::Mat img) {
  cv::adaptiveThreshold(img, img, 255, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY, 5, 5);
}

struct Panel {
  Panel(cv::Rect r) : bounds{std::move(r)} {}
  cv::Rect bounds;
  std::vector<std::pair<std::string, std::string>> dialog;
};

struct Context {
  Context(const std::string& file, bool debug);

  void saveDebug(const std::string& file);

  bool debug;
  cv::Mat img;
  cv::Mat debugImg;
  std::vector<Panel> panels;
};

struct Template {
  Template(char ch_, cv::Mat img_) : ch{ch_}, img{img_} {}

  char ch;
  cv::Mat img;
};

std::vector<Template> loadTemplates(const std::string& pathStr);

#endif
