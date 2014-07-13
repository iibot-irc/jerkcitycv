#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include <map>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

inline void threshold(cv::Mat img) {
  cv::adaptiveThreshold(img, img, 255, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY, 5, 5);
}

struct Bubble {
  Bubble(std::string contents_, cv::Rect bounds_) : contents{contents_}, bounds{bounds_} {}

  std::string contents;
  std::string actor;
  cv::Rect bounds;
};

struct Panel {
  Panel(cv::Rect r) : bounds{std::move(r)} {}
  cv::Rect bounds;
  std::vector<Bubble> dialog;
};

struct Context {
  Context(const std::string& file, bool debug);

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

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define ASSERT(x, ...) if (!(x)) { throw std::runtime_error{"Assertion " #x " failed at " __FILE__ ":" TOSTRING(__LINE__) __VA_ARGS__}; }

#endif
