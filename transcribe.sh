#!/bin/bash

mkdir -p /scratch/ssalbiz/jerkcity

for p in /scratch/j3parker/jerkcity/*png
do
  t=`basename $p`
  echo "./jerkcity $p > /scratch/ssalbiz/jerkcity/$t.txt"
  ./jerkcity $p > "/scratch/ssalbiz/jerkcity/$t.txt"
done
