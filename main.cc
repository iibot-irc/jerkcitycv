#include <cv.h>
#include <stdio.h>
#include <highgui.h>
#include <vector>
#include <string>

#define MATCH_THRESH 15000
#define COLOR_THRESH 200
#define MAX_CHARS 1000

const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ'-,?!";

IplImage**                       tmpl_alphabet;
std::map<std::string, IplImage*> tmpl_actors;

struct char_match {
  int x, y;
  char c;
};
std::vector<std::string> cast;
std::vector<char_match> char_matches;

IplImage* debug_img;

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

int charcount = 0;

void find_chars(IplImage* img) {
  for(int i = 0; i < alphabet.length(); ++i) {
    IplImage* match = find_template(img, tmpl_alphabet[i]);
    int x, y;
    char_match c;
    while(get_match_min(match, c.x, c.y) < MATCH_THRESH && charcount < MAX_CHARS) {
      c.c = alphabet[i];
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
}

int main(int argc, char** argv) {
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
  printf("Done (%d chars)\n", charcount);

  cvSaveImage("foo.png", debug_img);
//  cvSaveImage("foo.png", find_template(img, tmpl_alphabet[4]));

  return 0;
}
