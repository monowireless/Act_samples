#!/bin/bash

LANG=C
MAKE=make

# set ECHO=echo to show commands, so "./build_all.sh |sh" will execute.
ECHO=echo
# set ECHO= to execute commands in this script.
#ECHO=

# check MWSDK_ROOT env
if [ -z "$MWSDK_ROOT" ]; then
  export MWSDK_ROOT=`pwd`/..
  echo "#*** no MWSDK_ROOT, assume $MWSDK_ROOT ***"
fi

# guess Physical CPU count
JOBS=0
UNAME_S=`uname -s`
if [ "$UNAME_S" = "Darwin" ]; then # macOS
  JOBS=$(sysctl -n hw.physicalcpu)
elif [ "$UNAME_S" = "Linux" ]; then # Linux
  JOBS=$(expr $(cat /proc/cpuinfo | grep ^cpu.cores | uniq | cut -s -d: -f2))
elif [ ! -z "$WINDIR" ]; then # windows msys bash
  JOBS=`$WINDIR/System32/Wbem/wmic.exe cpu get NumberOfCores |awk 'NR==2 { print $1 }'`
  [ -z "$JOBS" ] && JOBS=0
  # if failed, use logical cpu count.
  [ $(expr $JOBS) -eq 0 ] && JOBS=$(expr $NUMBER_OF_PROCESSORS / 2)
fi
[ -z "$JOBS" ] && JOBS=0
[ $(expr $JOBS) -eq 0 ] && JOBS=4
echo "#*** jobs=$JOBS ***"

# check WSL
if [ ! -z "$WSLENV" ]; then
  # call msys bash
  MAKE=$MWSDK_ROOT/../Tools/MinGW/msys/1.0/bin/make.exe
fi

ck_exitvalue() {
  if [ -z $ECHO ]; then
    [ $? = 0 ] || exit 1
  else
    $ECHO '[ $? = 0 ] || exit 1'
  fi
}

do_build() {
  if [ -f $1/Makefile ]; then
    $ECHO echo ...building $1...
    $ECHO cd $1
    $ECHO rm -rfv *.bin objs_*
    $ECHO $MAKE -j$JOBS TWELITE=BLUE DISABLE_LTO=1
    ck_exitvalue
    $ECHO $MAKE -j$JOBS TWELITE=RED DISABLE_LTO=1 || exit 1
    ck_exitvalue
    $ECHO rm -rfv objs_*
    $ECHO cd ../..
  fi
}

for f in [a-zA-Z]*/build; do
  ( do_build $f )
done
