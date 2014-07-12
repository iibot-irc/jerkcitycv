#include "context.h"

// A CharBox is a node in an intrusive linked list
struct CharBox {
  char ch;
  cv::Rect bounds;
  bool wordBoundary = false; // Is this node at the end of a word? (i.e. does it need a space after it when derasterizing?)
  CharBox* next = nullptr;
};

// A StrBox points to the start and end of a CharBox list. It also caches the bounding rect for the entire list.
struct StrBox {
  StrBox(CharBox* first_, CharBox *last_, cv::Rect bounds_) : first{first_}, last{last_}, bounds{bounds_} {
    assert(first != nullptr);
    assert(last != nullptr);
  }

  CharBox* first;
  CharBox* last;
  cv::Rect bounds;
};

//StrBox merge(const StrBox& a, const StrBox& b, bool AsWords = false) {
//  return PPPP
//}

template <class F>
void collect(Context& ctx, std::vector<StrBox>& chunks, F joinable) {
}

void collectWords(Context& ctx, std::vector<StrBox>& chars) {
  collect(ctx, chars, [](CharBox& a, CharBox& b) {
    return false;
  });
}

void collectLines(Context& ctx, std::vector<StrBox>& words) {
  collect(ctx, words, [](CharBox& a, CharBox& b) {
    return false;
  });
}

void collectBubbles(Context& ctx, std::vector<StrBox>& lines) {
  collect(ctx, lines, [](CharBox& a, CharBox& b) {
    return false;
  });
}

std::vector<StrBox> initStrBoxes(std::vector<CharBox>& charBoxes) {
  std::vector<StrBox> result;
  for (auto& box : charBoxes) {
    result.emplace_back(&box, &box, box.bounds);
  }
  return result;
}

// Sometimes we may pick up "garbage" words - i.e. glyphs will be recognized in the background
// (usually punctuation.) This function removes them from the word list.
void filterGarbageWords(std::vector<StrBox>& words) {
  // TODO
}

int getCredibleMatch(cv::Mat matchAtlas, int startIndex, cv::Rect& match) {
  const size_t kCharMatchThresh = 15000;

  const int width = matchAtlas.size().width;
  const int height = matchAtlas.size().height;

  auto data = reinterpret_cast<float*>(matchAtlas.data);

  for(int i = startIndex; i < width * height; i++) {
    if (data[i] < kCharMatchThresh) {
      match.x = i % width;
      match.y = i / width;
      return startIndex + match.width;
    }
  }
  return -1;
}

std::vector<CharBox> findGlyphs(Context& ctx, const std::vector<Template>& templates) {
  const size_t kMaxChars = 5000; // Probably unnecessary now, but this once was a safety check

  std::vector<CharBox> results;

  for(auto&& tmpl : templates) {
    auto matchAtlas = cv::Mat(cv::Size(ctx.img.size().width, ctx.img.size().height), CV_32F, 1);
    cv::matchTemplate(ctx.img, tmpl.img, matchAtlas, CV_TM_SQDIFF);

    CharBox ch;
    ch.ch = tmpl.ch;
    ch.bounds = cv::Rect{0, 0, tmpl.img.size().width, tmpl.img.size().height};

    // Find all instances of this glyph
    int index = 0;
    while((index = getCredibleMatch(matchAtlas, index, ch.bounds)) != -1 && results.size() < kMaxChars) {
      results.push_back(ch);

      // Clear out a ROI around the match we ju
      cv::rectangle(matchAtlas, ch.bounds, FLT_MAX, CV_FILLED);
      if (ctx.debug) { cv::rectangle(ctx.debugImg, ch.bounds, cv::Scalar(255, 127, 127), CV_FILLED); }
    }
    if (results.size() >= kMaxChars) {
      break;
    }
  }
  return results;
}

void untypeset(Context& ctx) {
  std::vector<CharBox> charBoxes;
  {
    auto glyphs = loadTemplates("glyphs");
    charBoxes = findGlyphs(ctx, glyphs);
  }

  auto chunks = initStrBoxes(charBoxes);
  collectWords(ctx, chunks);
  filterGarbageWords(chunks);
  collectLines(ctx, chunks);
  collectBubbles(ctx, chunks);

  for (auto bubble : chunks) {
    CharBox* charBox = bubble.first;
    std::string dialog;
    size_t loopCheck = 0; // make sure we don't get caught in a cycle
    do {
      dialog += charBox->ch;
      if (charBox->wordBoundary) {
        dialog += " ";
      }
      if (loopCheck++ == charBoxes.size()) { // This is a "we have surely screwed up" check
        throw std::runtime_error{"Cycle detected in strBoxes during bubble derasterization"};
      }
    } while ((charBox = charBox->next) != nullptr);
    // add dialog to something
  }
}
