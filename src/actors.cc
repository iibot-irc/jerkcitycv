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

bool tryFindBubbleSource_Destructive(cv::Mat img, cv::Rect bubbleBounds, cv::Rect panelBounds, cv::Point& outPt) {
  // This algorithm finds the bottom tip of a speech bubble with a flood-fill-esque technique
  // First, we drop some grey pixels in the middle row of the speech bubble, 2/3 wide, everywhere there is a white pixel.
  // The algorithm proceeds in a loop with two passes each run:
  //    1. based on the minX/maxX of the previous iteration, look at the row below the previous iteration and if the pixel
  //       is white and the above is grey, make the pixel grey. This "bleeds" grey downwards
  //    2. "smear" the grey horizontally, stopping at black pixels.
  auto basicallyBlack = [](auto pix) {
    const auto kBasicallyBlack = 10;
    return pix < kBasicallyBlack;
  };

  auto minX = bubbleBounds.x + bubbleBounds.width/6 - panelBounds.x;
  auto maxX = minX + 2*bubbleBounds.width/3;
  ASSERT(0 < minX && minX < panelBounds.width);
  ASSERT(0 < maxX && maxX < panelBounds.width);

  auto lastX = -1;
  auto y = bubbleBounds.y + bubbleBounds.height/3 - panelBounds.y;
  ASSERT(0 < y && y < panelBounds.height);

  // "Seed" grey pixels
  for (auto x = minX; x <= maxX; x++) {
    auto& val = img.at<uint8_t>(y, x);
    if (!basicallyBlack(val)) {
      std::tie(minX, maxX) = std::minmax({minX, x, maxX});
      val = 127;
      lastX = x;
    }
  }

  // We weren't able to find any white pixels to seed... weird.
  if (lastX == -1) {
    return false;
  }

  y++;

  auto lastModifiedCount = 0;
  while (true) {
    auto modifiedCount = 0;

    // Pass 1: "bleed"
    for (auto x = minX; x <= maxX; x++) {
      auto valAbove = img.at<uint8_t>(y - 1, x);
      auto& val = img.at<uint8_t>(y, x);
      if (valAbove == 127 && !basicallyBlack(val)) {
        val = 127;
        lastX = x;
        modifiedCount++;
      }
    }

    // If we couldn't "bleed" any down it is time to stop
    if (modifiedCount == 0) {
      break;
    }
    lastModifiedCount = modifiedCount;

    // Pass 2: "smear"
    auto minBlackX = 0;
    for (auto x = 1; x < panelBounds.width; x++) {
      auto val = img.at<uint8_t>(y, x);
      if (val == 127) {
        for (x = minBlackX + 1; x < panelBounds.width; x++) {
          auto& val = img.at<uint8_t>(y, x);
          if (basicallyBlack(val)) {
            minBlackX = x;
            break;
          }
          val = 127;
          lastX = x;
          std::tie(minX, maxX) = std::minmax({minX, x, maxX});
        }
      } else if (basicallyBlack(val)) {
        minBlackX = x;
      }
    }

    y++;

    // If this is tripped either there is a bug in the algo or the comic is weird (does happen)
    if (y == panelBounds.height) {
      return false;
    }
  }

  ASSERT(0 <= lastX && lastX < panelBounds.width);
  ASSERT(0 <= y && y < panelBounds.height);
  outPt.x = lastX;
  outPt.y = y;

  const auto kProbablyNotAnArrowedBubble = 5;
  return lastModifiedCount < kProbablyNotAnArrowedBubble;
}

void attributeDialog(Context& ctx) {
  for (size_t i = 0; i < ctx.panels.size(); i++) {
    const auto& panel = ctx.panels[i];

    // Create an RGB copy of the panel so we can manipulate the pixels as scratch space
    auto panelImgRef = cv::Mat{ctx.img, panel.bounds};
    auto panelImg = cv::Mat{};
    panelImgRef.assignTo(panelImg, CV_8UC3);

    for (auto&& bubble : ctx.panels[i].dialog) {
      auto pt = cv::Point{};
      if (tryFindBubbleSource_Destructive(panelImg, bubble.bounds, panel.bounds, pt)) {
        const auto kWindowWidth = 128;
        const auto kWindowYOffset = 16;

        auto bounds = cv::Rect{std::max(0, pt.x - kWindowWidth/2),
                               std::min(panel.bounds.height - 1, pt.y + kWindowYOffset),
                               0, 0};
        bounds.width = std::min(panel.bounds.width - bounds.x, kWindowWidth);
        bounds.height = panel.bounds.height - bounds.y;
        ASSERT(bounds.x < panel.bounds.width);
        ASSERT(bounds.y < panel.bounds.height);
        ASSERT(bounds.y + bounds.height <= panel.bounds.height);
        ASSERT(bounds.x + bounds.width <= panel.bounds.width);
        auto windowRef = cv::Mat{panelImg, bounds};
        // probably run bg-remover on this ROI
        auto window = cv::Mat{};
        windowRef.assignTo(window, CV_8UC1); // create another copy, make this greyscale
        bubble.actor = findActor(window);
      }
    }

  }
}
