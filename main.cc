#include <cv.h>
#include <stdio.h>
#include <highgui.h>
#include <vector>
#include <string>
#include <sstream>

#define CHAR_MATCH_THRESH 15000
#define ACTOR_MATCH_THRESH 1000000
#define MAX_CHARS 1000
#define MAX_ACTORS 64
#define INTRA_WORD_X_SPACING 4
#define INTRA_WORD_Y_SPACING 3
#define INTER_WORD_X_SPACING 10

int charcount = 0;
int actorcount = 0;

enum actor_ID {
  SPIGOT = 0,
  DEUCE,
  RANDS,

  NUM_ACTORS
};

const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ'-,?!";

IplImage**             tmpl_alphabet;
std::vector<IplImage*> tmpl_actors[NUM_ACTORS];

struct char_match {
  int x, y;
  int w, h;
  char c;
  bool whitespace;
  char_match* prev;
  char_match* next;
};

struct actor_match {
  int x, y;
  int w, h;
  actor_ID id;
};

// newline delimited string
struct line {
  int xs, ys;
  int w, h;
  std::string s;
};

std::vector<actor_match> actor_matches;
std::vector<char_match> char_matches;
std::vector<line> lines;

IplImage* debug_img;

const char* actor_name(actor_ID id) {
  switch(id) {
    case SPIGOT: return "SPIGOT";
    case DEUCE:  return "DEUCE";
    case RANDS:  return "RANDS";
    default:     return "?!?!";
  }
}

int num_actor_templates(actor_ID id) {
  switch(id) {
    case SPIGOT: return 8;
    default:     return 0;
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
    while(get_match_min(match, c.x, c.y) < CHAR_MATCH_THRESH && charcount < MAX_CHARS) {
      char_matches.push_back(c);
      charcount++;
      cvRectangle(match,
                  cvPoint(c.x, c.y),
                  cvPoint(c.x + (float)tmpl_alphabet[i]->width,
                          c.y + (float)tmpl_alphabet[i]->height),
                  cvScalar(FLT_MAX),
                  CV_FILLED);
      cvRectangle(debug_img,
                  cvPoint(c.x, c.y),
                  cvPoint(c.x + (float)tmpl_alphabet[i]->width,
                          c.y + (float)tmpl_alphabet[i]->height),
                  cvScalar(0.0),
                  CV_FILLED);
    }
    cvReleaseImage(&match);
  }
  if(charcount == MAX_CHARS) {
    printf("whoa something bad happened\n");
    exit(1);
  }
  fprintf(stderr, "Found %d chars\n", charcount);
}

void find_actors(IplImage* img) {
  for(int id = 0; id < NUM_ACTORS; ++id) {
    for(int i = 0; i < tmpl_actors[id].size(); ++i) {
      IplImage* match = find_template(img, tmpl_actors[id][i]);
      int x, y;
      actor_match a;
      a.id = (actor_ID)id;
      a.w = tmpl_actors[id][i]->width;
      a.h = tmpl_actors[id][i]->height;
      while(get_match_min(match, a.x, a.y) < ACTOR_MATCH_THRESH && actorcount < MAX_ACTORS) {
        actor_matches.push_back(a);
        actorcount++;
        cvRectangle(match, cvPoint(a.x, a.y), cvPoint(a.x + a.w, a.y + a.h), cvScalar(FLT_MAX), CV_FILLED);
        cvRectangle(debug_img, cvPoint(a.x, a.y), cvPoint(a.x + a.w, a.y + a.h), cvScalar(127.0), CV_FILLED);
      }
      cvReleaseImage(&match);
    }
    if(actorcount == MAX_ACTORS) {
      printf("whoa something bad happened\n");
      exit(1);
    }
  }
  fprintf(stderr, "Found %d actors\n", actorcount);
}

IplImage* load_template(std::string& path) {
  IplImage* img;
  if(!(img = cvLoadImage(path.c_str(), 0))) {
    printf("Couldn't load template %s\n", path.c_str());
    exit(1);
  }
  fprintf(stderr, "\t%s (%d, %d)\n", path.c_str(), img->width, img->height);
  return img;
}

void load_templates() {
  fprintf(stderr, "Loading templates\n");
  tmpl_alphabet = new IplImage*[alphabet.length()];
  std::string path = "tmpl/X.png";
  for(int i = 0; i < alphabet.length(); ++i) {
    path[5] = alphabet[i];
    tmpl_alphabet[i] = load_template(path);
  }
  for(int id = 0; id < NUM_ACTORS; ++id) {
    for(int i = 0; i < num_actor_templates((actor_ID)id); i++) {
      std::stringstream ppath;
      std::string path;
      ppath << "tmpl/" << actor_name((actor_ID)id) << i << ".png";
      path = ppath.str();
      IplImage* img = load_template(path);
      IplImage* flipped = cvCloneImage(img); // shouldn't need to do a full clone, TODO fix
      cvFlip(img, flipped, 1); // flip on Y axis because actor can talk in either direction
      tmpl_actors[id].push_back(img);
      tmpl_actors[id].push_back(flipped);
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
      l.w = (cp->x - l.xs) + cp->w;
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
        char_matches[i].next = &char_matches[j];
        char_matches[j].prev = &char_matches[i];
      }
    }
  }
}

int main(int argc, char** argv) {
  char_matches.reserve(256);
  actor_matches.reserve(16);

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
  find_actors(img);
  gather_words();
  gather_rows();
  dump_lines();


//  cvSaveImage("foo.png", debug_img);
  cvSaveImage("foo.png", find_template(img, tmpl_alphabet[4]));
  printf("%d\n", actorcount);
  for(int i = 0; i < lines.size(); i++) {
    printf("%d %d %d %d: %s\n", lines[i].xs, lines[i].ys, lines[i].w, lines[i].h, lines[i].s.c_str());
  }
  return 0;
}
