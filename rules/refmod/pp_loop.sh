#!/bin/bash

## postprocess output from loop_patterns
## time cobra -f refmod/loop_patterns `cat c_files` > .RM/Ref_Mod (about 5 min, 2 min after first run)
## or when including filenames and linenrs (tagged=1):
## time cobra -f refmod/loop_patterns `cat c_files` > .RM/Ref_Src

if [ -d .RM ]
then	true
else
	echo "error: directory ./.RM does not exist"
	echo " run overview create first"
	exit 1
fi

if [ -s .RM/Ref_Src ]
then
	if [ -s .RM/Ref_Mod ]
	then	echo ".RM/Ref_Mod exists"
		exit 1
	fi
	awk ' { $1=""; print $0; }' < .RM/Ref_Src > .RM/Ref_Mod
fi

cat .RM/Ref_Mod | sort | uniq -c | sort -n          > .RM/Ref_Mod_sorted
cat .RM/Ref_Mod | sed 's;##.*;;'                    > .RM/Ref_Mod_control		## (control part)
cat .RM/Ref_Mod_control | sort | uniq -c | sort -n  > .RM/Ref_Mod_control_sorted
cat .RM/Ref_Mod | sed 's;^.*##;;'                   > .RM/Ref_Mod_loopbody		## (loop part)
cat .RM/Ref_Mod_loopbody | sort | uniq -c | sort -n > .RM/Ref_Mod_loopbody_sorted


echo "All Patterns:"

N0=`cat .RM/Ref_Mod | wc -l`

A3=`cat .RM/Ref_Mod_sorted | wc -l`
A4=`echo "(100 * $A3) / $N0" > .RM/calc; hoc < .RM/calc`
A5=`echo $A4 | awk ' { printf("%2.1f", $1) }' `

	echo "of $N0 patterns $A3 are distinct ($A5%)"

N4=`awk '$1=="##" { print $0 }' < .RM/Ref_Mod | wc -l`
N5=`echo "(100 * $N4) / $N0" > .RM/calc; hoc < .RM/calc`
N6=`echo $N5 | awk ' { printf("%2.1f", $1) }' `

	echo "of $N0 patterns  $N4 start with ## (ie are false matches) ($N6%)"

N7=`tail -1 .RM/Ref_Mod_sorted | awk ' { print $1 }'`
N8=`echo "(100 * $N7) / $N0" > .RM/calc; hoc < .RM/calc`
N9=`echo $N8 | awk ' { printf("%2.1f", $1) }' `

	echo "of $N0 patterns 1 appears $N7 times, the most frequent ($N9%)"
	tail -1 .RM/Ref_Mod_sorted | awk ' { $1="	"; print $0; }'
	##   _       ++      _       <       _       =       ##      _       _
	## for (ident =, ident <, ident++) { ident isn't modified in loop body }

N1=`awk '$1=="1" { print $0 }' < .RM/Ref_Mod_sorted | wc -l`
N2=`echo "(100 * $N1) / $A3" > .RM/calc; hoc < .RM/calc`
N3=`echo $N2 | awk ' { printf("%2.1f", $1) }' `

	echo "of  $A3 distinct patterns $N1 appear only once ($N3%)"

echo ""
echo "Control Parts:"

A0=`cat .RM/Ref_Mod_control_sorted | wc -l`
A1=`echo "(100 * $A0) / $N0" > .RM/calc; hoc < .RM/calc`
A2=`echo $A1 | awk ' { printf("%2.1f", $1) }' `

	echo "of $N0 patterns $A0 control parts are distinct ($A2%)"

X7=`tail -1 .RM/Ref_Mod_control_sorted | awk ' { print $1 }'`
X8=`echo "(100 * $X7) / $N0" > .RM/calc; hoc < .RM/calc`
X9=`echo $X8 | awk ' { printf("%2.1f", $1) }' `

	echo "of $N0 patterns 1 appears $X7 times, the most frequent ($X9%)"
	tail -1 .RM/Ref_Mod_control_sorted | awk ' { $1="	"; print $0; }'

B0=`cat .RM/Ref_Mod_control_sorted | awk '$1=="1" { print $0 }' | wc -l`
B1=`echo "(100 * $B0) / $A0" > .RM/calc; hoc < .RM/calc`
B2=`echo $B1 | awk ' { printf("%2.1f", $1) }' `

	echo "of   $A0 distinct patterns $B0 appear only once ($B2%)"

echo ""
echo "Use in Loop Body:"

B6=`cat .RM/Ref_Mod_loopbody_sorted| wc -l`
B7=`echo "(100 * $B6) / $N0" > .RM/calc; hoc < .RM/calc`
B8=`echo $B7 | awk ' { printf("%2.1f", $1) }' `

	echo "of $N0 patterns $B6 loop uses are distinct ($B8%)"

C4=`tail -1 .RM/Ref_Mod_loopbody_sorted | awk ' { print $1 }'`
C5=`echo "(100 * $C4) / $N0" > .RM/calc; hoc < .RM/calc`
C6=`echo $C5 | awk ' { printf("%2.1f", $1) }' `

	echo "of $N0 patterns 1 appears $C4 times, the most frequent ($C6%)"
	tail -1 .RM/Ref_Mod_loopbody_sorted | awk ' { $1="	"; print $0; }'
	##  _ _  next: + _, = _, _ ->, & _, _ =

C1=`awk '$1=="1" { print $0 }' < .RM/Ref_Mod_loopbody_sorted | wc -l`
C2=`echo "(100 * $C1) / $B6" > .RM/calc; hoc < .RM/calc`
C3=`echo $C2 | awk ' { printf("%2.1f", $1) }' `

	echo "of  $B6 distinct patterns  $C1 appear only once ($C3%)"

M3=`awk 'NF>2 && $(NF-2)=="##" && $(NF-1)=="_" && $NF == "_"' < .RM/Ref_Mod_sorted | wc -l`
M4=`echo "(100 * $M3) / $B6" > .RM/calc; hoc < .RM/calc`
M5=`echo $M4 | awk ' { printf("%2.1f", $1) }' `

	echo "of  $B6 distinct patterns  $M3 leave the loop var unmodified in the loop body ($M5%)"

D1=`cat .RM/Ref_Mod_loopbody_sorted | grep -e '[^=!]=[^=]' | wc -l`
D2=`echo "(100 * $D1) / $B6" > .RM/calc; hoc < .RM/calc`
D3=`echo $D2 | awk ' { printf("%2.1f", $1) }' `

	echo "of  $B6 distinct patterns $D1 contain =, a direct assignment of or to the loop var ($D3%)"
	# if appearing as a post *after* the loop var itself...., check context...

D4=`grep -e '++' -e '--' -e '*=' -e '%=' -e '+=' -e '-=' .RM/Ref_Mod_loopbody_sorted | wc -l`
D5=`echo "(100 * $D4) / $B6" > .RM/calc; hoc < .RM/calc`
D6=`echo $D5 | awk ' { printf("%2.1f", $1) }' `

	echo "of  $B6 distinct patterns $D4 contain ++, --, +=, -=, *=, %=, ($D6%)"
	# if appearing as a post *after* the loop var itself....

rm -f .RM/calc
