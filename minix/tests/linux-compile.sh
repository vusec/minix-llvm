#!/bin/bash

set -e

TESTNUMS=" 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20
          21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38    40
          41    43 44 45 46 47    49 50    52    54    56 57 58    60
          61    63       66 67 68 69 70 71       74 75 76 77 78 79 80
          81 82   "
TESTS="`echo "$TESTNUMS" | sed 's/[0-9]\+/test\0/g'`"
PROGS="t10a t11a t11b t40a t40b t40c t40d t40e t40f t40g t60a t60b t67a t67b t68a t68b mod"
SCRIPTS="testsh1 testsh2"
FILES="test1.c"

rm -rf linux-bin
mkdir -p linux-bin
for p in $TESTS $PROGS; do
	echo Compiling $p
	EXTRA=""
	[ "$p" != test56 ] || EXTRA="common-socket.c"
	[ "$p" != test57 ] || EXTRA="test57loop.S"
	[ "$p" != test63 ] || EXTRA="-ldl"
	[ "$p" != test71 ] || EXTRA="testcache.c"
	[ "$p" != test74 ] || EXTRA="testcache.c"
	[ "$p" != test76 ] || EXTRA="-lutil"
	[ "$p" != test77 ] || EXTRA="-lutil"
	[ "$p" != test80 ] || EXTRA="common-socket.c"
	[ "$p" != test81 ] || EXTRA="common-socket.c"
	[ "$p" != mod    ] || EXTRA="-fPIC -shared"
	cc -m32 -lm -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -o linux-bin/$p $p.c common.c $EXTRA &
done
for p in $SCRIPTS; do
	echo Installing $p
	install -m 755 $p.sh linux-bin/$p
done
for p in $FILES; do
	echo Installing $p
	install -m 644 $p linux-bin/$p
done
install -m 755 run linux-bin/run
wait

