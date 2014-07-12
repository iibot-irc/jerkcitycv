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

  void checkRep() const {
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

template <class T>
void checkRep(const std::vector<T>& ts) {
  for(const auto& t : ts) {
    t.checkRep();
  }
}

void merge(StrBox& a, StrBox& b, bool asWords = false) {
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

void drawDebugRects(Context& ctx, std::vector<StrBox>& boxes, cv::Scalar c, int thick) {
  if (ctx.debug) {
    for (const auto& box : boxes) {
      cv::rectangle(ctx.debugImg, box.bounds, c, thick);
    }
  }
}

void drawDebugArrow(Context& ctx, CharBox* a, CharBox* b, cv::Scalar c) {
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

      chunks[(size_t)which == i ? j : i].checkRep();

      chunks.erase(chunks.begin() + which);

      // restart the process
      i = 0;
      j = -1;
    }
  }
}

auto horizCollector(Context& ctx, std::vector<StrBox>& elems, const int kXSpacing, bool asWords, cv::Scalar debugColor) {
  return [=, &ctx, &elems](int i, int j) {
    const auto kYSpacing = 3;

    CharBox* endOfA = elems[i].last;
    CharBox* startOfB = elems[j].first;

    // Force 'a' to be to the left of 'b'
    if (endOfA->bounds.x > startOfB->bounds.x) {
      std::swap(i, j);
      endOfA = elems[i].last;
      startOfB = elems[j].first;
    }

    auto& a = elems[i];
    auto& b = elems[j];

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
    drawDebugArrow(ctx, a.last, b.first, debugColor);
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

  drawDebugRects(ctx, words, {255, 127, 255}, 2);
}

bool intervalIntersects(int a0, int a1, int b0, int b1) {
  if(a0 < b0) {
    return b0 < a1;
  } else {
    return a0 < b1;
  }
}

void collectBubbles(Context& ctx, std::vector<StrBox>& lines) {
  const auto kInterLineSpacing = 5;
  collect(lines, [&](int i, int j) {

    // Make lines[i] be above lines[j]
    if (lines[i].bounds.y > lines[j].bounds.y) {
      std::swap(i, j);
    }

    auto& a = lines[i];
    auto& b = lines[j];

    auto yDist = std::abs(a.bounds.y + a.bounds.height - b.bounds.y);
    if (!intervalIntersects(a.bounds.x, a.bounds.x + a.bounds.width,
                           b.bounds.x, b.bounds.x + b.bounds.width)
     || yDist > kInterLineSpacing) {
      return -1;
    }

    merge(a, b);

    return j;
  });

  drawDebugRects(ctx, lines, {127, 255, 255}, 2);
}

std::vector<StrBox> initStrBoxes(std::vector<CharBox>& charBoxes) {
  std::vector<StrBox> result;
  for (auto& box : charBoxes) {
    result.emplace_back(&box, &box, box.bounds);
  }
  return result;
}


// Sometimes we may pick up "garbage" lines - i.e. glyphs will be recognized in the background
// (usually punctuation.) This function removes them from the line list.
void filterGarbageLines(Context& ctx, std::vector<StrBox>& lines) {
  const size_t kMaxSuspiciousLength = 3;

  for (int i = 0; i < (int)lines.size(); i++) {
    size_t length = 0;
    auto& L = lines[i];

    size_t questionableChars = 0;
    CharBox* ptr = L.first;
    do {
      switch (ptr->ch) {
        case '-': case '.': case '=': case '/': case '\\': case '\'': case '"':
        case '|': case ':': case ';': case ',': questionableChars++; break;
        default: break;
      }
    } while ((ptr = ptr->next) != nullptr && length++ <= kMaxSuspiciousLength);

    if (length > kMaxSuspiciousLength || questionableChars != length) {
      continue;
    }

    if (ctx.debug) {
      cv::rectangle(ctx.debugImg, L.bounds, { 255, 255, 255 }, CV_FILLED);
      cv::line(ctx.debugImg, { L.bounds.x, L.bounds.y }, { L.bounds.x + L.bounds.width, L.bounds.y + L.bounds.height }, { 0, 0, 255 }, 2, CV_AA);
      cv::line(ctx.debugImg, { L.bounds.x, L.bounds.y + L.bounds.height }, { L.bounds.x + L.bounds.width, L.bounds.y }, { 0, 0, 255 }, 2, CV_AA);
    }

    lines.erase(lines.begin() + i);
    i = -1;
  }
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
  checkRep(chunks);

  collectLines(ctx, chunks);
  checkRep(chunks);

  filterGarbageLines(ctx, chunks);
  checkRep(chunks);

  collectBubbles(ctx, chunks);
  checkRep(chunks);

  for (auto bubble : chunks) {
    CharBox* charBox = bubble.first;
    std::string dialog;
    do {
      dialog += charBox->ch;
      if (charBox->wordBoundary) {
        dialog += " ";
      }
    } while ((charBox = charBox->next) != nullptr);
    // add dialog to something
  }
}
