#!/bin/sh
#
# make NLarn distribution package
#

FILES="nlarn lib/fortune lib/maze lib/nlarn.hlp lib/nlarn.msg"

ARCH=$(uname -p)
OS=$(uname -s)
VERSION_MAJOR=$(grep VERSION_MAJOR inc/nlarn.h | cut -f 3 -d" ")
VERSION_MINOR=$(grep VERSION_MINOR inc/nlarn.h | cut -f 3 -d" ")
VERSION_PATCH=$(grep VERSION_PATCH inc/nlarn.h | cut -f 3 -d" ")

DIRNAME=nlarn-"$VERSION_MAJOR"."$VERSION_MINOR"."$VERSION_PATCH"
PACKAGE="$DIRNAME"_"$OS"."$ARCH".tar.gz

if [ ! -f Makefile -a ! -f nlarn.make ]
then
  premake4 gmake
fi

make clean
make verbose=yes 

mkdir "$DIRNAME"
cp $FILES "$DIRNAME"

rm -f "$PACKAGE"
tar cfvz "$PACKAGE" "$DIRNAME"

rm -rf "$DIRNAME"
