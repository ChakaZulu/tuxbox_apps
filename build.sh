#!/bin/bash

error() {
	echo
	echo "+++ PRESS <ENTER> TO CONTINUE OR <CTRL-C> TO ABORT +++"
	read
}

make_dir() {
	dir=$1
	target=$2
	for subdir in `ls --ignore CVS $dir`; do
		make -C "$dir/$subdir" $target
		if [ $? -ne 0 ]; then error; fi
	done
}

make_dir "dvb" $1
make_dir "misc" $1
make_dir "tuxbox" $1

