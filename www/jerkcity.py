#!/usr/bin/env python

import re, random, urllib2, os, cgi
import shutil
import simplejson as json

IMAGE_PATHS = "images/"
TRANSCRIPT_PATHS = "transcripts/"

def get_image_and_transcript(comic=None):
    image = None
    if not comic:
        image = random.choice([f for f in os.listdir(IMAGE_PATHS) if f.endswith(".png")])
    else:
        image = comic + ".png"
    transcript = image + ".txt"
    return image, transcript

def display_status():
    template = open("index.templ", "r", True).read()
    form = cgi.FieldStorage()
    if form.getvalue('action') == 'submit' and form.getvalue('transcript') \
            and form.getvalue('id'):
        open("/tmp/dumpfile", "a", True).write(
            json.dumps({'id': form.getvalue('id'),
                'transcript': form.getvalue('transcript')}))
        print "SUBMITTED UPDATED TRANSCRIPT: HAVE A COCK FILLED DAY<br>"
    image, transcript = get_image_and_transcript(form.getvalue('comic'))
    template = re.sub("###ID###", image.replace(".png", ""), template)
    template = re.sub("###IMAGE###",
            "http://www.jerkcity.com/jerkcity" + image.replace("png", "gif"), template)
    template = re.sub("###TRANSCRIPT###",
            open(os.path.join(TRANSCRIPT_PATHS, transcript), "r", True).read(), template)
    print template

if __name__ == '__main__':
  print "Content-type: text/html\n\n";
  display_status()
