#!/bin/sh
XP=$PWD
SNI="-I$PWD/snappy"
LZI="-I$PWD/lz4/lib"
SNL="-L$PWD/snappy/.libs"
LZL="-L$PWD/out/Release -L$PWD/out/Debug"
ALI="$SNI $LZI"
ALL="$SNL $LZL"

pushd rocksdb
rm -f make_config.mk
CFLAGS="$ALI" CXXFLAGS="$ALI" LDFLAGS="$ALL" make static_lib
RET=$?
popd

exit $RET

