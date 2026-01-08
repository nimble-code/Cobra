#!/bin/sh

## to combine large number of files into a single
## file, with markers added that Cobra can read to
## record the correct original filename, and line
## numbers within each file
## this can speed up the startup phase of a cobra
## session, although a similar effect can be
## achieved by starting cobra in multicore mode
## eg cobra -N8

if [ $# -lt 2 ]
then
	echo "usage: flatten.sh *.c > somename"
	exit 1
fi

for i in $*
do
	echo "@>$i"
	cat $i
done
