#!/bin/sh
XP=$PWD
SNI="-I$XP/snappy"
LZI="-I$XP/lz4/lib"
SNL="-L$XP/snappy/.libs"
LZL="-L$XP/out/Release -L$XP/out/Debug"
ALI="$SNI $LZI"
ALL="$SNL $LZL"

rm -f rocksdb/make_config.mk
echo XP=$XP
cd $XP/rocksdb
echo $PWD
echo "CFLAGS=\"$ALI\" CXXFLAGS=\"$ALI\" LDFLAGS=\"$ALL\" make static_lib"
CFLAGS="$ALI" CXXFLAGS="$ALI" LDFLAGS="$ALL" make static_lib
RET=$?
cd $XP

exit $RET

