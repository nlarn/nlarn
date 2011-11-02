#!/bin/sh
#
# make NLarn distribution package
#
# For Windows this depends on MSYS and 7-Zip
#

ARCH=$(uname -m)
PREVIEW=0
NUMJOBS=1

while getopts "j:p" OPTION; do
	case "$OPTION" in
		p) PREVIEW=1;;
		j) NUMJOBS="$OPTARG";;
	esac
done

if [[ "$OS" == "Windows_NT" ]]; then
	OS=win32
	SUFFIX="zip"
	EXE="nlarn.exe libglib-2.0-0.dll libz-1.dll"
	export CC=gcc
else
	OS=$(uname -s)
	SUFFIX="tar.gz"
	EXE="nlarn"
fi

MAINFILES="$EXE nlarn.ini-sample README.txt LICENSE"
LIBFILES="lib/fortune lib/maze lib/maze_doc.txt lib/nlarn.hlp lib/nlarn.msg lib/monsters.lua"

VERSION_MAJOR=$(awk '/VERSION_MAJOR/ { print $3 }' inc/nlarn.h)
VERSION_MINOR=$(awk '/VERSION_MINOR/ { print $3 }' inc/nlarn.h)
VERSION_PATCH=$(awk '/VERSION_PATCH/ { print $3 }' inc/nlarn.h)

VERSION="${VERSION_MAJOR}.${VERSION_MINOR}"
if [[ "$VERSION_PATCH" -gt 0 ]]; then
	VERSION="${VERSION}.${VERSION_PATCH}"
fi

if [[ "$PREVIEW" -eq 1 ]]; then
	GITREV="-$(git show --pretty=oneline | sed -ne '1 s/\(.\{6\}\).*/\1/p')"
	export CFLAGS=-DGITREV=\'\"$GITREV\"\'
	VERSION="${VERSION}${GITREV}"
fi

DIRNAME="nlarn-${VERSION}"
PACKAGE="${DIRNAME}_${OS}.${ARCH}.${SUFFIX}"

# Regenerate Makefile if necessary
if [[ ! -f Makefile || ! -f nlarn.make ]]; then
	premake4 gmake || exit 1
fi

make clean
make -j $NUMJOBS verbose=yes

# Quit on errors
if [[ $? -ne 0 ]]; then
	echo Build failed.
	exit
fi

mkdir -p "$DIRNAME"/lib
cp $MAINFILES "$DIRNAME"
cp $LIBFILES "$DIRNAME"/lib

rm -f "$PACKAGE"

# create archives for distribution
if [[ "$OS" != "win32" ]]; then
	tar cfvz "$PACKAGE" "$DIRNAME"
else
	zip -r "$PACKAGE" "$DIRNAME"
	# create windows installer (double slash to protect it from shell)
	makensis //DVERSION="$VERSION" nlarn.nsi
fi

rm -rf "$DIRNAME"
