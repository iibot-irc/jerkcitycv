#include "context.h"

// A CharBox is a node in an intrusive doubly-linked list
struct CharBox {
  char ch;
  cv::Rect bounds;
  bool wordBoundary = false; // Is this node at the end of a word? (i.e. does it need a space after it when derasterizing?)
  CharBox* next = nullptr;
  CharBox* prev = nullptr;
};

// A StrBox points to the start and end of a CharBox list. It also caches the bounding rect for the entire list.
struct StrBox {
  StrBox(CharBox* first_, CharBox *last_, cv::Rect bounds_) : first{first_}, last{last_}, bounds{bounds_} {
    checkRep();
  }

  CharBox* first;
  CharBox* last;
  cv::Rect bounds;

  void checkRep() {
    ASSERT(first != nullptr);
    ASSERT(last != nullptr);
    ASSERT(first->prev == nullptr);
    ASSERT(last->next == nullptr);

    // Make sure last is reachable from first and vice-versa
    if (first == last) {
      return;
    }
    auto soFar = std::string{first->ch};
    auto tortoise = first;
    auto hare = first->next;
    ASSERT(hare != nullptr);
    while (1) {
      // We are maintaining the invariant that everything up to the tortoise has its prev pointers set correctly.

      if (tortoise == last) {
        return;
      }

      // If the tortoise isnt at the end it will only reach the hare if there is a cycle
      ASSERT(tortoise != hare, "string so far: " + soFar);

      // Since we aren't at the end, verify that we are linked to the next node correctly
      ASSERT(tortoise->next->prev = tortoise, "string so far: " + soFar);

      // Tortoise moves 1 step
      tortoise = tortoise->next;
      soFar += tortoise->ch;

      // Hare attempts to move 2 steps forward
      hare = hare->next ? (hare->next->next ? hare->next->next : hare->next) : hare;
    }
  }
};

void merge(StrBox& a, StrBox& b, bool asWords) {
  a.last->next = b.first;
  b.first->prev = a.last;
  if (asWords) {
    ASSERT(!a.last->wordBoundary);
    ASSERT(!b.last->wordBoundary);
    a.last->wordBoundary = true;
  }
  a.last = b.last;

  a.bounds = a.bounds | b.bounds; // union rects
}

void debugArrow(Context& ctx, CharBox* a, CharBox* b, cv::Scalar c) {
  int ax = a->bounds.x + a->bounds.width/2;
  int ay = a->bounds.y + a->bounds.height/2;
  int bx = b->bounds.x + b->bounds.width/2;
  int by = b->bounds.y + b->bounds.height/2;

  if (ctx.debug) {
    cv::line(ctx.debugImg, { ax, ay }, { bx, by }, c, 2, CV_AA);
    cv::line(ctx.debugImg, { bx, by }, { bx - 3, by + 3 }, c, 1, CV_AA);
    cv::line(ctx.debugImg, { bx, by }, { bx - 3, by - 3 }, c, 1, CV_AA);
    cv::line(ctx.debugImg, { bx - 3, by + 3 }, { bx - 3, by - 3 }, c, 1, CV_AA);
  }
}

template <class F>
void collect(std::vector<StrBox>& chunks, F attemptToJoin) {
  for (size_t i = 0; i < chunks.size(); i++) {
    for (size_t j = 0; j < chunks.size(); j++) {
      if (i == j) { continue; }
      chunks[i].checkRep();
      chunks[j].checkRep();

      auto which = attemptToJoin(i, j);
      if (which == -1) { continue; }

      chunks[which == i ? j : i].checkRep();

      chunks.erase(chunks.begin() + which);

      // restart the process
      i = 0;
      j = -1;
    }
  }
}

auto horizCollector(Context& ctx, std::vector<StrBox>& chars, const int kXSpacing, bool asWords, cv::Scalar debugColor) {
  return [=, &ctx, &chars](int i, int j) {
    const auto kYSpacing = 3;

    CharBox* endOfA = chars[i].last;
    CharBox* startOfB = chars[j].first;

    // Force 'a' to be to the left of 'b'
    if (endOfA->bounds.x > startOfB->bounds.x) {
      std::swap(i, j);
      endOfA = chars[i].last;
      startOfB = chars[j].first;
    }

    StrBox& a = chars[i];
    StrBox& b = chars[j];

    // Point on 'a' is on the middle of the right edge
    auto ax = endOfA->bounds.x + endOfA->bounds.width;
    auto ay = endOfA->bounds.y + endOfA->bounds.height/2;

    // Point on 'b' is on the middle of the left edge
    auto bx = startOfB->bounds.x;
    auto by = startOfB->bounds.y + startOfB->bounds.height/2;

    auto xDist = std::abs(bx - ax); // abs because chars can slightly penetrate
    auto yDist = std::abs(by - ay);

    //std::cout << xDist << " " << yDist << "\n";
    if (xDist > kXSpacing || yDist > kYSpacing) {
      return -1;
    }

    // TODO: old stuff had some extra logic here... is it important?
    debugArrow(ctx, a.last, b.first, debugColor);
    merge(a, b, asWords);

    return j;
  };
}

void collectWords(Context& ctx, std::vector<StrBox>& chars) {
  const auto kIntraWordXSpacing = 4;
  collect(chars, horizCollector(ctx, chars, kIntraWordXSpacing, false, { 255, 127, 127 }));
}

void collectLines(Context& ctx, std::vector<StrBox>& words) {
  const auto kInterWordXSpacing = 14;
  const auto debugColor = cv::Scalar{127, 255, 127};
  collect(words, horizCollector(ctx, words, kInterWordXSpacing, true, debugColor));
  for (const auto& line : words) {
    if (ctx.debug) {
      cv::rectangle(ctx.debugImg, line.bounds, {255, 127, 255}, 2);
    }
  }
}

void collectBubbles(Context& ctx, std::vector<StrBox>& lines) {
  collect(lines, [](int i, int j) {
    return -1;
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
      return startIndex + match.width - 1;
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
    ch.bounds = cv::Rect{{0, 0}, tmpl.img.size()};

    // Find all instances of this glyph
    int index = 0;
    while((index = getCredibleMatch(matchAtlas, index, ch.bounds)) != -1 && results.size() < kMaxChars) {
      results.push_back(ch);

      // Clear out a ROI around the match we ju
      cv::rectangle(matchAtlas, ch.bounds, FLT_MAX, CV_FILLED);
      if (ctx.debug) { cv::rectangle(ctx.debugImg, ch.bounds, {0, 0, 0}, CV_FILLED); }
    }
    if (results.size() >= kMaxChars) {
      throw std::runtime_error{"Whoa - too many characters. Something went wrong."};
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
