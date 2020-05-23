#!/bin/sh

mkdir -p output

rm -f output/out.jpg || true
rm -f tmp.jpg~ || true

while true
do
  STARTTIME=$(date +%s)
  raspistill -t 200 -w 2592 -h 1944 -o tmp.jpg
  mv -f tmp.jpg output/out.jpg
  ENDTIME=$(date +%s)
  echo "Took photo in $(($ENDTIME - $STARTTIME)) seconds"
done
