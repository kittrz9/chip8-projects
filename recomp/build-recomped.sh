#!/bin/sh

set -xe

FILE=$1

if [ -z "$FILE" ]; then
	echo "file not found"
	exit 1
fi

RAYLIB_LINK="https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_linux_amd64.tar.gz"
RAYLIB_ARCHIVE="raylib-5.5_linux_amd64.tar.gz"
RAYLIB_DIR="raylib-5.5_linux_amd64"

CC=clang
CFLAGS="-O2 -Wall -Wextra -Wpedantic"
INCLUDE="-I./$RAYLIB_DIR/include/"
LIBS="./$RAYLIB_DIR/lib/libraylib.a -lm"

if [ ! -f "$RAYLIB_ARCHIVE" ]; then
	wget "$RAYLIB_LINK"
fi

if [ ! -d "$RAYLIB_DIR" ]; then
	tar xavf "$RAYLIB_ARCHIVE"
fi

$CC $INCLUDE $CFLAGS $FILE $LIBS -o $FILE.out

