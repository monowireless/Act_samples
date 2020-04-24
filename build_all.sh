#!/bin/bash

LANG=C

# check MWSDK_ROOT env
if [ -z "$MWSDK_ROOT" ]; then
  export MWSDK_ROOT=`pwd`/..
  echo "*** no MWSDK_ROOT, assume $MWSDK_ROOT ***"
fi

# guess CPU count
UNAME_S=`uname -s`
if [ "$UNAME_S" = "Darwin" ]; then
  JOBS=$(sysctl -n hw.physicalcpu)
elif [ "$UNAME_S" = "Darwin" ]; then
  JOBS=$(expr $(cat /proc/cpuinfo | grep ^cpu.cores | uniq | cut -s -d: -f2))
else
  JOBS=$(expr $NUMBER_OF_PROCESSORS / 2)
fi
echo "*** jobs=$JOBS ***"

do_build() {
  echo $1
  cd $1
  if [ -f Makefile ]; then
    rm -rfv *.bin objs_*
    make -j$JOBS TWELITE=BLUE || exit 1
    make -j$JOBS TWELITE=RED || exit 1
    rm -rfv objs_*
  fi
  cd ../..
}

for f in [a-zA-Z]*/build; do
  ( do_build $f )
done
