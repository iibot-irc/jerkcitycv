#!/bin/bash

MAX=500
PARALLEL=3

rm -rf out
mkdir out
seq 1 $MAX | xargs -n 1 -P $PARALLEL ./test.sh
seq -f "out/%g.dat" 1 $MAX | xargs cat > out/index.html
rm out/*.dat
