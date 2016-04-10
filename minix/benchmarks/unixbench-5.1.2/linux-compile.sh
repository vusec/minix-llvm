#!/bin/bash

set -e

rm -rf linux-bin

mkdir -p linux-bin/results linux-bin/testdir linux-bin/tmp
install -m 755 run_bc.sh linux-bin

compile()
{
	name="$1"
	echo "Compiling $name"
	shift
	cc -m32 -D_FILE_OFFSET_BITS=64 $FLAGS -o "linux-bin/pgms/$name" "$@"
}

mkdir -p "linux-bin/pgms"
compile context1 src/context1.c
FLAGS="-DHZ=60 -DREG=register" compile dhry2reg src/dhry_1.c src/dhry_2.c
compile execl src/execl.c
compile fstime src/fstime.c
compile looper src/looper.c
compile pipe src/pipe.c
compile spawn src/spawn.c
compile syscall src/syscall.c
FLAGS="-DDP -DUNIX -DUNIXBENCH -lm" compile whetstone-double src/whets.c
install -m 755 pgms/multi.sh/multi.sh linux-bin/pgms/multi.sh
