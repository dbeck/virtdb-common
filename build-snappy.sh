#!/bin/sh
COMMON_DIR=$PWD
pushd snappy
./virtdb-build.sh
popd
if [ -e snappy/.libs/libsnappy.a ]
then
  echo 'snappy built'
  exit 0
else
  echo 'snappy build failed'
  exit 100
fi

