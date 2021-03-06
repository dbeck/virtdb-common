#!/bin/sh
XP=$PWD
LZI="-I$XP/lz4/lib"
LZL="-L$XP/lz4/lib"
FKI="-I$XP/fake-includes"
ALI="$LZI $FKI"
ALL="$LZL"

MF=$MAKEFLAGS
unset MAKEFLAGS
rm -f rocksdb/make_config.mk
echo XP=$XP
cd $XP/rocksdb
echo $PWD
echo "CFLAGS=\"$ALI\" CXXFLAGS=\"$ALI\" LDFLAGS=\"$ALL\" PORTABLE=1 make static_lib PORTABLE=1"
CFLAGS="$ALI" CXXFLAGS="$ALI" LDFLAGS="$ALL" PORTABLE=1 make static_lib PORTABLE=1
RET=$?
cd $XP
export MAKEFLAGS=$MF

exit $RET

