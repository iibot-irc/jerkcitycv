#include "context.h"

std::string findActor(cv::Mat);

void customThreshImg(cv::Mat img) {
  for (int y = 5; y < img.rows - 5; y++) {
    for (int x = 5; x < img.cols - 5; x++) {
      auto val = img.at<uint8_t>(y, x);
      if(val > 5 && val < 250) {
        for (int yy = std::max(0, y - 2); yy < std::min(img.rows, y + 2); yy++) {
          for (int xx = std::max(0, x - 2); xx < std::min(img.cols, x + 2); xx++) {
            auto& val = img.at<uint8_t>(yy, xx);
            if ((yy < y || val > 5) && val < 250) {
              val = 127;
            }
          }
        }
      }
    }
  }
  for (int y = 0; y < img.rows; y++) {
    for (int x = 0; x < img.cols; x++) {
      auto& val = img.at<uint8_t>(y, x);
      if (val == 127) {
        val = 255;
      }
    }
  }
  cv::imwrite("/home/j3parker/www/scratch/out4.png", img);
}

void attributeDialog(Context& ctx) {
  for (size_t i = 0; i < ctx.panels.size(); i++) {
    const auto& panel = ctx.panels[i];

    // TODO: get better window to search in. for now, we use entire panel below the top 1/3rd
    auto panelBounds = panel.bounds;
    panelBounds.y += panel.bounds.height / 3;
    panelBounds.height *= 2.0f/3.0f;
    auto panelImg = cv::Mat{ctx.img, panelBounds};
    //customThreshImg(panelImg);

    for (auto&& bubble : ctx.panels[i].dialog) {
      bubble.actor = findActor(panelImg);
    }

  }
}
