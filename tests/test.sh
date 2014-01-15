#!/bin/bash
NUM=$1
EXPECTED=`awk "/^JERKCITY \#$NUM:/{a=1;next}/--cut here--/{a=0}a" jerkcity.txt | tail -n +3`

if [ ! -f $NUM.png ]; then
  wget --quiet http://jerkcity.com/jerkcity$NUM.gif
  convert jerkcity$NUM.gif $NUM.png
  rm jerkcity$NUM.gif
fi

cd ../
ACTUAL=`./jerkcity tests/$NUM.png 2>/dev/null | sed -e 's/ *$//' -e 's/^ *//'`
EX=$?

if [[ $EX -ne 0 ]]; then
  echo "<p>Non-zero exit code: $EX</p>"
  exit 40
fi

EXESC=`echo "$EXPECTED" | sed -e 's/</&gt;/g' -e 's/>/&lt;/g'`
ACESC=`echo "$ACTUAL" | sed -e 's/</&gt;/g' -e 's/>/&lt;/g'`

if [[ $EXPECTED == "" ]]; then
  echo "<p>No expected data.<p>"
  echo "<h2>Actual</h2><pre>$ACESC</pre>"
  exit 10
fi

EXLINES=`echo "$EXPECTED" | wc -l`
ACLINES=`echo "$ACTUAL" | wc -l`

if [[ $EXLINES -ne $ACLINES ]]; then
  echo "<p>Line count mismatch.</p>"
  echo "<h2>Expected</h2><pre>$EXESC</pre><h2>Actual</h2><pre>$ACESC</pre>"
  exit 30
fi

EXWORDS=`echo "$EXPECTED" | sed -e 's/^[^:]*: //'`
ACWORDS=`echo "$ACTUAL" | sed -e 's/^[^:]*: //'`

if [[ "$EXWORDS" != "$ACWORDS" ]]; then
  echo "<p>Incorrect dialog.</p>"
  echo "<h2>Expected</h2><pre>$EXESC</pre><h2>Actual</h2><pre>$ACESC</pre>"
  exit 31
fi

if [[ "$EXPECTED" != "$ACTUAL" ]]; then
  echo "<p>Incorrect cast.</p>"
  echo "<h2>Expected</h2><pre>$EXESC</pre><h2>Actual</h2><pre>$ACESC</pre>"
  exit 20
fi

echo "<p>Yay!</p>"
echo "<h2>Expected</h2><pre>$EXESC</pre><h2>Actual</h2><pre>$ACESC</pre>"
