#!/bin/bash
rm -rf out
mkdir out
for NUM in `seq 1 5464`; do
  echo $NUM
  OUT=`./test.sh $NUM`
  CODE=$?
  echo '<!doctype html><meta charset="utf-8"><h1>Test results for comic #'$NUM'</h1><img src="http://jerkcity.com/jerkcity'$NUM'.gif">'"$OUT" > out/$NUM.html
  case $CODE in
    1*)
      COLOR="grey"
      ;;
    2*)
      COLOR="yellow"
      ;;
    3*)
      COLOR="red"
      ;;
    4*)
      COLOR="purple"
      ;;
    0)
      COLOR="green"
      ;;
  esac
  echo '<a href="'$NUM'" style="font-size: 20; background: '$COLOR'">'$NUM'</a>' >> out/index.html
done
