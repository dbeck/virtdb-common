#!/bin/sh
XP=$PWD
MF=$MAKEFLAGS
unset MAKEFLAGS
echo XP=$XP
cd $XP/lz4
echo $PWD
CFLAGS="-fPIC" LDFLAGS="-fPIC" make -C lib liblz4
RET=$?
cd $XP
ln -sf ./lz4/lib/liblz4.a xlast-lz4.a
export MAKEFLAGS=$MF
exit $RET

