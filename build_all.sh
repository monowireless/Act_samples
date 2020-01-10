#!/bin/bash

function doit() {
  cd $1
  if [ -f Makefile ]; then
    rm -rfv *.bin objs_*
    make -j6 TWELITE=BLUE || exit 1
    make -j6 TWELITE=RED || exit 1
    rm -rfv objs_*
  fi
  cd ../..
}

date >> build.log

for f in */build; do
  echo $f
  doit $f >>build.log 2>&1
done
