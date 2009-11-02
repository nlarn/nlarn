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
	SUFFIX="zip"
	EXE="nlarn.exe pdcurses.dll libglib-2.0-0.dll libz-1.dll"
	export CC=gcc
else
	OS=$(uname -s)
	SUFFIX="tar.gz"
	EXE="nlarn"
fi

MAINFILES="$EXE nlarn.ini-sample LICENSE"
LIBFILES="lib/fortune lib/maze lib/nlarn.hlp lib/nlarn.msg"

VERSION_MAJOR=$(grep VERSION_MAJOR inc/nlarn.h | cut -f 3 -d" ")
VERSION_MINOR=$(grep VERSION_MINOR inc/nlarn.h | cut -f 3 -d" ")
VERSION_PATCH=$(grep VERSION_PATCH inc/nlarn.h | cut -f 3 -d" ")

VERSION="$VERSION_MAJOR"."$VERSION_MINOR"
if [ "$VERSION_PATCH" -gt 0 ]
then
	VERSION="$VERSION"."$VERSION_PATCH"
fi

DIRNAME=nlarn-"$VERSION"
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
	echo Build failed.
	exit
fi

mkdir -p "$DIRNAME"/lib
cp $MAINFILES "$DIRNAME"
cp $LIBFILES "$DIRNAME"/lib

rm -f "$PACKAGE"

# create archives for distribution
if [ "$OS" != "win32" ]
then
	tar cfvz "$PACKAGE" "$DIRNAME"
else
	zip -r "$PACKAGE" "$DIRNAME"
	# create windows installer (double slash to protect it from shell)
	makensis //DVERSION="$VERSION" nlarn.nsi
fi

rm -rf "$DIRNAME"
