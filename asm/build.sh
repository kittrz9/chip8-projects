#!/bin/sh

set -xe

CC=clang
NAME=chip8asm
CFLAGS=-O2

CFILES="$(find src/ -name "*.c")"

rm -rf obj/ build/
mkdir obj/ build/

for f in $CFILES; do
	OBJNAME=$(echo $f | sed -e "s/src/obj/;s/\.c/\.o/")
	$CC $CFLAGS -c $f -o $OBJNAME
	OBJS="$OBJNAME $OBJS"
done

$CC $OBJS -o build/$NAME
