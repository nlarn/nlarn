#!/bin/sh
#
# make NLarn distribution package
#
# For Windows this depends on MSYS and 7-Zip
# 

ARCH=$(uname -m)

if [ "$OS" = "Windows_NT" ]
then
	OS=win32
	SUFFIX="exe"
	EXE="nlarn.exe pdcurses.dll libglib-2.0-0.dll"
else
	OS=$(uname -s)
	SUFFIX="tar.gz"
	EXE="nlarn"
fi
FILES="lib/fortune lib/maze lib/nlarn.hlp lib/nlarn.msg"

VERSION_MAJOR=$(grep VERSION_MAJOR inc/nlarn.h | cut -f 3 -d" ")
VERSION_MINOR=$(grep VERSION_MINOR inc/nlarn.h | cut -f 3 -d" ")
VERSION_PATCH=$(grep VERSION_PATCH inc/nlarn.h | cut -f 3 -d" ")

DIRNAME=nlarn-"$VERSION_MAJOR"."$VERSION_MINOR"."$VERSION_PATCH"
PACKAGE="$DIRNAME"_"$OS"."$ARCH"."$SUFFIX"

# Regenerate Makefile if necessary
if [ ! -f Makefile -a ! -f nlarn.make ]
then
  premake4 gmake
fi

make verbose=yes 

# Quit on errors
if [ $? -gt 0 ]
then
	exit
fi

mkdir -p "$DIRNAME"/lib
cp $EXE "$DIRNAME"
cp $FILES "$DIRNAME"/lib

rm -f "$PACKAGE"

if [ "$OS" != "win32" ]
then
	tar cfvz "$PACKAGE" "$DIRNAME"
else
	7z a -r -sfx7z.sfx "$PACKAGE" "$DIRNAME"
fi
rm -rf "$DIRNAME"
