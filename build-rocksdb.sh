#!/bin/sh
XP=$PWD
SNI="-I$XP/snappy"
LZI="-I$XP/lz4/lib"
SNL="-L$XP/snappy/.libs"
LZL="-L$XP/lz4/lib"
FKI="-I$XP/fake-includes"
ALI="$SNI $LZI $FKI"
ALL="$SNL $LZL"

MF=$MAKEFLAGS
unset MAKEFLAGS
rm -f rocksdb/make_config.mk
echo XP=$XP
cd $XP/rocksdb
echo $PWD
echo "CFLAGS=\"$ALI\" CXXFLAGS=\"$ALI\" LDFLAGS=\"$ALL\" make static_lib"
CFLAGS="$ALI" CXXFLAGS="$ALI" LDFLAGS="$ALL" make static_lib 
RET=$?
cd $XP
export MAKEFLAGS=$MF

exit $RET

