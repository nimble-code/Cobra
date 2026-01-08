#!/bin/sh

## shell wrapper for cobra to create a single
## command to compute a factorial, using the
## predefined script in rules/version5/factorial.cobra
## for instance:
## 	$ ./factorial.sh 25
## 	25! = 15511211079246240657965056.00

if [ "$#" -ne 1 ]
then
	echo "usage: factorial.sh N"
	echo " with N a number >= 0"
	exit 1
fi

cobra -solo -var N=$1 -f version5/factorial.cobra
