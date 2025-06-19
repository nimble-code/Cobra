#!/bin/sh

## example of using Cobra to create a command
## by using a socalled here-document for the input
## can be called as: ./factorial.sh -var N=25

cobra -noecho -terse -solo $* <<COBRA_INLINE

requires 5.0

def Factorial(N)
%{
	function factorial(n)
	{
	   if (n < 0)
	   {  print "error: negative number\n"\;
		Stop\;
	   }
	   if (n <= 1)
	   {  return 1\;
	   }
	   return n * factorial(n-1)\;
	}
	print N "! = " factorial(N) "\n"\;
%}
end
Factorial(10)

COBRA_INLINE
