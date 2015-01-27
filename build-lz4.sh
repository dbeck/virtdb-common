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
export MAKEFLAGS=$MF
exit $RET

