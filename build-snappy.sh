#!/bin/sh
COMMON_DIR=$PWD
MF=$MAKEFLAGS
unset MAKEFLAGS
pushd snappy
./virtdb-build.sh
popd
export MAKEFLAGS=$MF
if [ -e snappy/.libs/libsnappy.a ]
then
  echo 'snappy built'
  ln -sf ./snappy/.libs/libsnappy.a xlast-snappy.a
  exit 0
else
  echo 'snappy build failed'
  exit 100
fi

