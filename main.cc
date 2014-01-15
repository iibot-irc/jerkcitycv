#include <cv.h>
#include <stdio.h>
#include <highgui.h>
#include <vector>
#include <string>
#include <sstream>

#define CHAR_MATCH_THRESH 15000
#define MAX_CHARS 1000
#define INTRA_WORD_X_SPACING 4
#define INTRA_WORD_Y_SPACING 3
#define INTER_WORD_X_SPACING 14
#define INTER_ROW_SPACING 5
#define BASICALLY_WHITE 200
#define PANEL_SKIP_AMOUNT 80
#define ALMOST_SAME_HEIGHT 5

int charcount = 0;

const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ-,?!";

IplImage**             tmpl_alphabet;
std::vector<int> alphabet_counts;

struct char_match {
  int x, y;
  int w, h;
  char c;
  bool whitespace;
  char_match* prev;
  char_match* next;
};

// newline delimited string
struct line {
  int xs, ys;
  int w, h;
  std::string s;
  line* prev;
  line* next;
};

struct bubble {
  int x, y;
  int w, h;
  std::string s;
};

struct segment {
  int offset;
};

std::vector<char_match> char_matches;
std::vector<line> lines;
std::vector<bubble> bubbles;
std::vector<segment> v_segments, h_segments;
std::vector<std::vector<bubble> > panel_bubbles;

IplImage* debug_img;

bool bubble_comp(bubble i, bubble j) {
  if(abs(i.y - j.y) < ALMOST_SAME_HEIGHT) {
    return i.x < j.x;
  } else {
    return i.y < j.y;
  }
}

void sort_panels() {
  for(int i = 0; i < panel_bubbles.size(); ++i) {
    std::sort(panel_bubbles[i].begin(), panel_bubbles[i].end(), bubble_comp);
  }
}

void coalesce_panels() {
  for (int i = 0; i < v_segments.size() * h_segments.size(); ++i) {
    std::vector<bubble> v;
    panel_bubbles.push_back(v);
  }
  //fprintf(stderr, "%d x %d panels detected\n", h_segments.size(), v_segments.size());

  int x, y, panel;
  for (int i = 0; i < bubbles.size(); ++i) {
    for (x = 0; x < h_segments.size(); ++x) {
      if (bubbles[i].x < h_segments[x].offset) break;
    }
    for (y = 0; y < v_segments.size(); ++y) {
      if (bubbles[i].y < v_segments[y].offset) break;
    }
    assert(y > 0 && x > 0);
    panel = (y-1) * h_segments.size() + (x-1);
    //fprintf(stderr, "panel #: %d = %d x %d\n", panel, x, y);
    panel_bubbles[panel].push_back(bubbles[i]);
  }
}

void find_panels(IplImage* img) {
  uchar* data = (uchar*)img->imageData;
  segment s;
  s.offset = 0;
  v_segments.push_back(s);
  h_segments.push_back(s);
  for(int i = 0; i < img->height; i++) {
    int j = 0;
    while(j < img->width && data[i*img->widthStep + j] >= BASICALLY_WHITE) j++;
    if(j == img->width) {
      s.offset = i;
      v_segments.push_back(s);
      for(int j = 0; j < img->width; j++) {
        ((uchar*)debug_img->imageData)[i*img->widthStep + j] = 127;
      }
      i += PANEL_SKIP_AMOUNT;
    }
  }
  for(int j = 0; j < img->width; j++) {
    int i = 0;
    int ass = 0;
    while(i < img->height) {
      if(data[i*img->widthStep + j] < BASICALLY_WHITE) {
	if(i > img->height - 30) { i++; continue; } // red fucking banner
        if(i > 150) break;
        if(ass++ > 9) break;
      } else ass = 0;
      i++;
    }
    if(i == img->height) {
      s.offset = j;
      h_segments.push_back(s);
      for(int i = 0; i < img->height; i++) {
        ((uchar*)debug_img->imageData)[i*img->widthStep + j] = 127;
      }
      j += PANEL_SKIP_AMOUNT;
    }
  }
}

IplImage* find_template(IplImage* img, IplImage* tmpl) {
  int W = img->width - tmpl->width + 1;
  int H = img->height - tmpl->height + 1;
  IplImage* match = cvCreateImage(cvSize(W, H), 32, 1);
  cvMatchTemplate(img, tmpl, match, CV_TM_SQDIFF);
  return match;
}

float get_match_min(IplImage* img, int& x, int& y) {
  float min_so_far = FLT_MAX;
  float* data = (float*)img->imageData;
  for(int i = 0; i < img->width * img->height; ++i) {
    if(min_so_far > data[i]) {
      min_so_far = data[i];
      x = i % img->width;
      y = i / img->width;
    }
  }
  return min_so_far;
}

void draw_rect(IplImage* img, float x, float y, float w, float h, float color, int thickness) {
  cvRectangle(img,
              cvPoint(x, y),
              cvPoint(x + w, y + h),
              cvScalar(color),
              thickness);
}

void find_chars(IplImage* img) {
  for(int i = 0; i < alphabet.length(); ++i) {
    IplImage* match = find_template(img, tmpl_alphabet[i]);
    int x, y;
    char_match c;
    c.next = c.prev = NULL;
    c.whitespace = false;
    c.c = alphabet[i];
    c.w = tmpl_alphabet[i]->width;
    c.h = tmpl_alphabet[i]->height;
    alphabet_counts.push_back(0);
    while(get_match_min(match, c.x, c.y) < CHAR_MATCH_THRESH && charcount < MAX_CHARS) {
      alphabet_counts[i]++;
      char_matches.push_back(c);
      charcount++;
      draw_rect(match, c.x, c.y, (float)tmpl_alphabet[i]->width - 1, (float)tmpl_alphabet[i]->height, FLT_MAX, CV_FILLED);
      draw_rect(debug_img, c.x, c.y, (float)tmpl_alphabet[i]->width - 1, (float)tmpl_alphabet[i]->height, 0, CV_FILLED);
    }
    cvReleaseImage(&match);
  }
  if(charcount == MAX_CHARS) {
    printf("whoa something bad happened\n");
    exit(1);
  }
}

IplImage* load_template(std::string& path) {
  IplImage* img;
  if(!(img = cvLoadImage(path.c_str(), 0))) {
    printf("Couldn't load template %s\n", path.c_str());
    exit(1);
  }
  //fprintf(stderr, "\t%s (%d, %d)\n", path.c_str(), img->width, img->height);
  return img;
}

void load_templates() {
  //fprintf(stderr, "Loading templates\n");
  tmpl_alphabet = new IplImage*[alphabet.length()];
  std::string path = "tmpl/X.png";
  for(int i = 0; i < alphabet.length(); ++i) {
    path[5] = alphabet[i];
    tmpl_alphabet[i] = load_template(path);
  }
}

bool lines_overlap(line *l1, line *l2) {
  line* max =  (l1->w > l2->w) ? l1 : l2;
  line* less = (max == l2) ? l1 : l2;
  return (less->xs >= max->xs && less->xs < (max->xs + max->w)) ||
    (less->xs < max->xs && ((less->xs+less->w) > max->xs));
}

bool probably_not_line(std::string s) {
  if(s.length() > 3) return false;
  int crap = 0;
  for(int i = 0; i < s.length(); i++) {
    switch(s[i]) {
      case '-': case '.': case '=': case '/': case '\\': case '\'': case '"':
      case '|': case ':': case ';': case ',': crap++; break;
    }
  }
  return crap == s.length() - 1;
}

void gather_lines() {
  for (int i = 0 ; i < lines.size(); ++i) {
    for (int j = 0; j < lines.size(); ++j) {
      if (i == j) continue;
      bool b1 = lines_overlap(&lines[i], &lines[j]);
      //printf("lines overlap: %d\n", b1);
      bool b2 = abs(lines[i].ys + lines[i].h - lines[j].ys) < INTER_ROW_SPACING;
      //printf("lines close by: %d\n", b2);
      if (b1 && b2) {
        lines[i].next = &lines[j];
        lines[j].prev = &lines[i];
      }
    }
  }
  // coalesce speech bubbles
  bubble b;
  line *n, *p;
  int x, y, w, h;
  for (int i = 0; i < lines.size(); ++i) {
    if (lines[i].prev == NULL) {
      std::string s;
      n = &lines[i];
      x = lines[i].xs;
      y = lines[i].ys;
      w = lines[i].w;
      h = lines[i].h;
      while(n) {
        s.append(n->s);
        s.append(1, ' ');
        x = std::min(x, n->xs);
        w = std::max(w, n->w);
        p = n;
        n = n->next;
      }
      h = abs(lines[i].ys - p->ys) + p->h;
      //fprintf(stderr, "bubble x y = %d %d\n", x, y);
      b.s = s;
      b.x = x; b.y = y;
      b.w = w; b.h = h;
      draw_rect(debug_img, b.x, b.y, b.w, b.h, FLT_MAX/2.0, 2);
      if(probably_not_line(s)) continue;
      bubbles.push_back(b);
    }
  }
}

/**
 * Dumps gathered words into a vector of lines with the starting
 */
void dump_lines() {
  char_match *cm, *cp;
  line l;
  int max_height;
  for (int i = 0; i < char_matches.size(); ++i) {
    if (char_matches[i].prev == NULL) {
      cm = &char_matches[i];
      l.prev = NULL; l.next = NULL;
      l.xs = cm->x; l.ys = cm->y;
      max_height = cm->h;
      while(cm) {
        l.s.append(1, cm->c);
        if (cm->whitespace) {
          l.s.append(1, ' ');
        }
        // get the max bounding height
        max_height = cm->h > max_height ? cm->h : max_height;
        cp = cm;
        cm = cm->next;
      }
      l.w = abs(cp->x - l.xs) + cp->w;
      l.h = max_height;
      lines.push_back(l);
      l.s.clear();
    }
  }
}

void gather_rows() {
  for(int i = 0; i < char_matches.size(); ++i) {
    if(char_matches[i].next != NULL) continue; // we want the end of a word
    for(int j = 0; j < char_matches.size(); ++j) {
      if(i == j) continue;
      if(char_matches[j].prev != NULL) continue; // we want the start of another word
      if(char_matches[i].x > char_matches[j].x) continue;
      if(abs(char_matches[i].x + char_matches[i].w - char_matches[j].x) < INTER_WORD_X_SPACING &&
         abs(char_matches[i].y + char_matches[i].h/2 - char_matches[j].y - char_matches[j].h/2) < INTRA_WORD_Y_SPACING) {
        char_matches[i].next = &char_matches[j];
        char_matches[j].prev = &char_matches[i];
        char_matches[i].whitespace = true;
      }
    }
  }
}

void gather_words() {
  // link up characters from left to right into words
  for(int i = 0; i < char_matches.size(); ++i) {
    if(char_matches[i].next != NULL) continue;
    for(int j = 0; j < char_matches.size(); ++j) {
      if(char_matches[j].prev != NULL) continue;
      if(char_matches[j].x < char_matches[i].x) continue;
      if(abs(char_matches[i].x + char_matches[i].w - char_matches[j].x) <= INTRA_WORD_X_SPACING &&
         abs(char_matches[i].y + char_matches[i].h/2 - char_matches[j].y - char_matches[j].h/2) <= INTRA_WORD_Y_SPACING) {
        if (char_matches[i].next != NULL) {
          // either insert it on the end of the chain or into the middle based on which is cleaner
          if (abs(char_matches[i].x - char_matches[j].x) < abs(char_matches[i].next->x - char_matches[j].x)) {
            char_matches[i].next->next = &char_matches[j];
            char_matches[j].prev = char_matches[i].next;
          } else {
            char_matches[j].next = char_matches[i].next;
            char_matches[i].next->prev = &char_matches[j];
            char_matches[i].next = &char_matches[j];
            char_matches[j].prev = char_matches[j].next;
          }
        } else {
          char_matches[i].next = &char_matches[j];
          char_matches[j].prev = &char_matches[i];
        }
      }
    }
  }
}

int main(int argc, char** argv) {
  char_matches.reserve(256);

  if(argc != 2) {
    printf("No filename provided.\n");
    exit(1);
  }

  IplImage *img1, *img;
  img1 = cvLoadImage(argv[1], 0); // force greyscale
  img = cvCloneImage(img1);
  cvAdaptiveThreshold(img1, img, 255, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY, 5);
  if(!img) {
    printf("Could not load file: %s\n", argv[1]);
    exit(1);
  }
  debug_img = cvCloneImage(img);

  load_templates();
  find_chars(img);
  gather_words();
  gather_rows();
  dump_lines();
  gather_lines();
  find_panels(img);
  coalesce_panels();
  sort_panels();

  cvSaveImage("foo.png", debug_img);
  /*
  cvSaveImage("foo.png", find_template(img, tmpl_alphabet[4]));

  for(int i = 0; i < alphabet_counts.size(); ++i)
    printf("%c -> %d\n", alphabet[i], alphabet_counts[i]);
  */
  for(int i = 0; i < panel_bubbles.size(); ++i) {
    for(int j = 0; j < panel_bubbles[i].size(); ++j) {
      printf("unknown: %s\n", panel_bubbles[i][j].s.c_str());
    }
  }
  /*
  for(int i = 0; i < lines.size(); ++i)
    printf("%s\n", lines[i].s.c_str());
  */
  return 0;
}
