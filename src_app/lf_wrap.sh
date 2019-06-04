#!/bin/bash

// wrapper around the ./lf prim checker

./lf -v -t $1 *.c |
	awk '
		{ split($1, a, ":")
		  names[a[1]]++
		  if (names[a[1]] == 1) {
			print "// file " $1
			print ""
		  }
		}
		{ for (i = 2; i <= NF; i++) {
			printf("%s ", $i)
		  }
		  printf("\n")
		}' |
	indent
