#include "context.h"

#include <iostream>

#include <boost/program_options.hpp>

#include <opencv2/highgui/highgui.hpp>

void findPanels(Context& ctx);
void untypeset(Context& ctx);
void attributeDialog(Context& ctx);

Context::Context(const std::string& file, bool debug_) : debug{debug_} {
  img = cv::imread(file, CV_LOAD_IMAGE_GRAYSCALE);
  if (img.dims == 0) {
    throw std::runtime_error{"Couldn't load: " + file};
  }
  //threshold(img);

  if (debug) {
    debugImg = cv::imread(file, CV_LOAD_IMAGE_COLOR);
    if (debugImg.dims == 0) {
      throw std::runtime_error{"Couldn't load debug image: " + file};
    }
  }
}

void saveDebug(const Context& ctx, const std::string& file) {
  if (ctx.debug) {
    cv::imwrite(file.c_str(), ctx.debugImg);
  }
}

void process(Context& ctx) {
  findPanels(ctx);
  untypeset(ctx);
  attributeDialog(ctx);

  for (const auto& panel : ctx.panels) {
    for (const auto& bubble : panel.dialog) {
      std::cout << bubble.actor << ": " << bubble.contents << "\n";
    }
  }
}

int main(int argc, char** argv) {
  namespace po = boost::program_options;
  auto desc = po::options_description{"Allowed options"};
  desc.add_options()
    ("help", "this message")
    ("debug-file", po::value<std::string>(),  "file to store a .png with debug output")
    ("input-file", po::value<std::string>(), "input comic in png format");

  auto po_desc = po::positional_options_description{};

  auto vm = po::variables_map{};
  po::store(po::command_line_parser(argc, argv).options(desc).positional(po_desc).run(), vm);
  po::notify(vm);

  if (vm.count("help") || !vm.count("input-file")) {
    std::cout << desc << "\n";
    return -1;
  }

  const auto& inFile = vm["input-file"].as<std::string>();

  auto ctx = Context{inFile, vm.count("debug-file") != 0};

  const std::string debugFile = vm.count("debug-file") ? vm["debug-file"].as<std::string>() : "";

  try {
    process(ctx);
  } catch (...) {
    saveDebug(ctx, debugFile);
    throw;
  }

  saveDebug(ctx, debugFile);
}
