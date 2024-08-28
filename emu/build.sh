#!/bin/sh

set -xe

CC=clang
NAME=chip8emu
LIBS=-lSDL2
MODE=interp

case $MODE in
	jit)
		DEFINES=-DCPU_JIT
		;;
	interp)
		DEFINES=-DCPU_INTERP
		;;
	*)
		echo "unknown mode $MODE in build.sh"
		exit 1
esac

C_FILES="$(find src/ -name "*.c")"

rm -rf obj/ build/
mkdir obj/ build/

for f in $C_FILES; do
	OBJNAME=$(echo $f | sed -e "s/src/obj/;s/\.c/\.o/")
	$CC $DEFINES -c $f -o $OBJNAME
	OBJS="$OBJNAME $OBJS"
done

$CC $OBJS -o build/$NAME $LIBS
