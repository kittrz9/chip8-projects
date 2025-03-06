#!/bin/sh

set -xe

CC=clang
CFLAGS="-fsanitize=undefined -fsanitize=address -g -Wall -Wextra -Wpedantic"
CFILES="$(find src/ -name "*.c")"

rm -rf build
mkdir build

$CC $CFLAGS $CFILES -o build/test

