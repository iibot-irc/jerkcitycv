#include "context.h"

void drawDebugLine(Context& ctx, int x0, int y0, int x1, int y1) {
    if (ctx.debug) {
      cv::line(ctx.debugImg, { x0, y0 }, { x1, y1 }, { 0, 0, 255 }, 3);
    }
}

void findPanels(Context& ctx) {
  ASSERT(ctx.img.channels() == 1);

  const size_t kSkipAmount = 100; // If we find a panel divider, skip 80 pixels for the next one.
  const uint8_t kBasicallyWhite = 200; // Any value above this is to be considered "white"

  const size_t width = ctx.img.size().width;
  const size_t height = ctx.img.size().height;

  if (width <= kSkipAmount || height <= kSkipAmount) {
    throw std::runtime_error{"Image is too small for these algorithms to work..."};
  }

  auto* data = (uint8_t*)ctx.img.data;

  // Find horizontal lines
  std::vector<size_t> ys;
  for (size_t y = 0; y < height; y++) {
    size_t x;
    for (x = 0; x < width && data[x + y*width] >= kBasicallyWhite; x++) {}

    // Ensure there is a divider above the top panels
    if (ys.size() == 0 && y > kSkipAmount) {
      ys.push_back(0);
      drawDebugLine(ctx, 0, 0, x, 0);
    }

    // If we made it to the right side of the image we've got a panel divider
    if (x == width) {
      ys.push_back(y);
      drawDebugLine(ctx, 0, y, x, y);

      y += kSkipAmount;
    }
  }

  // Find vertical lines
  const size_t kPointOfNoReturn = 150;
  const size_t kUrgTolerance = 7;
  size_t urgCounter = 0;
  std::vector<size_t> xs;
  for (size_t x = 0; x < width; x++) {
    size_t y;
    for (y = 0; y < height; y++) {
      if (data[x + y*width] < kBasicallyWhite) {
        #ifdef RED_KICKSTARTER_FOOTER
        // hack: during the BBoJC Kickstarter a red footer was applied to each comic
        if (y > height - 30) {
          y = height;
          break;
        }
        #endif

        // After some point we do not permit any non-white pixels
        if (y > kPointOfNoReturn) {
          break;
        }

        // If we are near the top of the image, permit some amount of non-white chars
        // This allows the line to "tunnel through" the title text which sometimes
        // overflows panel 1.
        if (urgCounter++ > kUrgTolerance) {
          break;
        }
      } else {
        urgCounter = 0;
      }
    }

    // Ensure there is a divider to the left of the left-most panels
    if (xs.size() == 0 && x > kSkipAmount) {
      xs.push_back(0);
      drawDebugLine(ctx, 0, 0, 0, y);
    }

    // If we made it to the bottom of the image we've got a panel divider
    if (y == height) {
      xs.push_back(x);
      drawDebugLine(ctx, x, 0, x, y);

      x += kSkipAmount;
    }
  }

  // Ensure there is a divider to the right of the right-most panels
  assert(xs.size() > 0);
  if (xs.back() < width - kSkipAmount) {
    xs.push_back(width - 1);
    drawDebugLine(ctx, width - 1, 0, width - 1, height);
  }
  // Ensure there is a divider below the bottom panels
  assert(ys.size() > 0);
  if (ys.back() < height - kSkipAmount) {
    ys.push_back(height - 1);
    drawDebugLine(ctx, 0, height - 1, width, height - 1);
  }

  // Would be possible to construct an image such that this gets thrown.
  // We can't continue without enough dividers, though.
  if (xs.size() < 2 || ys.size() < 2) {
    throw std::runtime_error{"Finding the panels went horribly wrong"};
  }

  // Collect panels
  for (auto yit = ys.begin(); yit != ys.end() - 1; ++yit) {
    for (auto xit = xs.begin(); xit != xs.end() - 1; ++xit) {
      int32_t x0 = *xit;
      int32_t x1 = *(xit + 1);
      int32_t y0 = *yit;
      int32_t y1 = *(yit + 1);
      ASSERT(x0 < x1 && y0 < y1);
      ctx.panels.emplace_back(Panel{cv::Rect{x0, y0, x1 - x0, y1 - y0}});
    }
  }
}
