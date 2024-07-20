#!/bin/sh

set -xe

CC=clang
NAME=chip8
LIBS=-lSDL2

C_FILES="$(find src/ -name "*.c")"

rm -rf obj/ build/
mkdir obj/ build/

for f in $C_FILES; do
	OBJNAME=$(echo $f | sed -e "s/src/obj/;s/\.c/\.o/")
	$CC -c $f -o $OBJNAME
	OBJS="$OBJNAME $OBJS"
done

$CC $OBJS -o build/$NAME $LIBS
