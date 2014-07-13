#!/bin/bash
NUM=$1
echo $NUM
CHUNK=`awk "/^<issue num=\"$NUM\">$/{a=1;next}/<\/dialog>/{a=0}a" dialog.xml`
EXPECTED=`echo "$CHUNK" | grep -m1 -A5000 '<dialog>' | tail -n +2`

echo "<img src=\"http://jerkcity.com/jerkcity$NUM.gif\"><img src=\"$NUM.debug.png\">" > out/$NUM.html

if [ ! -f img/$NUM.png ]; then
  wget --quiet http://jerkcity.com/jerkcity$NUM.gif
  convert jerkcity$NUM.gif img/$NUM.png
  rm jerkcity$NUM.gif
fi

cd ../src
ACTUAL=`nice -n 5 timeout -s 9 10 ./jerkcity --debug-file=../tests/out/$NUM.debug.png --input-file=../tests/img/$NUM.png | sed -e 's/ *$//' -e 's/^ *//'`
EX=$?
cd ../tests

if [[ $EX -ne 0 ]]; then
  echo "<a href=\"$NUM.html\" style=\"font-size: 20; display: block-inline; width: 5em; background: purple\">$NUM</a>" > out/$NUM.dat
  echo "<p>Non-zero exit code: $EX</p>" >> out/$NUM.html
  exit
fi

EXESC=`echo "$EXPECTED" | sed -e 's/</\&lt;/g' -e 's/>/\&gt;/g'`
ACESC=`echo "$ACTUAL" | sed -e 's/</\&lt;/g' -e 's/>/\&gt;/g'`

if [[ $EXPECTED == "" ]]; then
  echo "<a href=\"$NUM.html\" style=\"font-size: 20; display: block-inline; width: 5em; background: grey\">$NUM</a>" > out/$NUM.dat
  echo "<p>No expected data.<p><h2>Possible Transcription</h2><pre>$ACESC</pre>" >> out/$NUM.html
  exit
fi

EXLINES=`echo "$EXPECTED" | wc -l`
ACLINES=`echo "$ACTUAL" | wc -l`

if [[ $EXLINES -ne $ACLINES ]]; then
  echo "<a href=\"$NUM.html\" style=\"font-size: 20; display: block-inline; width: 5em; background: red\">$NUM</a>" > out/$NUM.dat
  echo "<p>Line count mismatch.</p><h2>Expected</h2><pre>$EXESC</pre><h2>Actual</h2><pre>$ACESC</pre>" >> out/$NUM.html
  exit
fi

EXWORDS=`echo "$EXPECTED" | sed -e 's/^[^:]*: //'`
ACWORDS=`echo "$ACTUAL" | sed -e 's/^[^:]*: //'`

if [[ "$EXWORDS" != "$ACWORDS" ]]; then
  echo "<a href=\"$NUM.html\" style=\"font-size: 20; display: block-inline; width: 5em; background: red\">$NUM</a>" > out/$NUM.dat
  echo "<p>Incorrect dialog.</p><h2>Expected</h2><pre>$EXESC</pre><h2>Actual</h2><pre>$ACESC</pre>" >> out/$NUM.html
  exit
fi

if [[ "$EXPECTED" != "$ACTUAL" ]]; then
  echo "<a href=\"$NUM.html\" style=\"font-size: 20; display: block-inline; width: 5em; background: yellow\">$NUM</a>" > out/$NUM.dat
  echo "<p>Incorrect cast.</p><h2>Expected</h2><pre>$EXESC</pre><h2>Actual</h2><pre>$ACESC</pre>" >> out/$NUM.html
  exit
fi

echo "<a href=\"$NUM.html\" style=\"font-size: 20; display: block-inline; width: 5em; background: green\">$NUM</a>" > out/$NUM.dat
echo "<p>Yay!</p><h2>Expected</h2><pre>$EXESC</pre><h2>Actual</h2><pre>$ACESC</pre>" >> out/$NUM.html
