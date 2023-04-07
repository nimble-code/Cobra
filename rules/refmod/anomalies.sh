#!/bin/bash

# called from refmod/check
# detect statistically anomalous code by comparing code patterns against
# the patterns in a reference model
# for instance:
#	cd Spin/src
#	refmod build; refmod assess; refmod check

if [ $# -eq 1 ]
then
	RD=$1
	RM=$1/Ref_Mod_control_sorted
	if [ -d $RD ]
	then	true
	else	echo "error: directory $RD not found"
		exit 1
	fi
	if [ -s $RM ]
	then	true
	else	echo "error: file $RM not found"
		exit 1
	fi
else
	echo "usage: anomalies refdir -- see refmod/check"
	exit 1	
fi

cat .RM/Ref_Mod_control_sorted |
	awk 'NF>1 { $1=""; print $0 }' |	# strip count, keep pattern
	sed 's;^ *;;' > .RM/_to_find_		# strip leading spaces

echo "seqnr=1" > .RM/_do_find_
echo -n "" > .RM/_foo_
cat .RM/_to_find_ |
	sed "s;^;egrep -e \'[0-9]  ;" |
	sed 's;||;\\|\\|;g'  |
	sed 's;\&;\\\&;g'  |
	sed 's;\+;\\+;g'  |
	sed 's;\*;\\*;g' |
	sed "s; *$; $\' $RM > /dev/null \; if [ \$\? -ne 0 ] \; then echo sed -n \${seqnr}p .RM/_to_find_ >> .RM/_foo_ \; fi\; seqnr=\`expr \$seqnr + 1\`;"  >> .RM/_do_find_

# echo "echo \$seqnr" >> .RM/_do_find_

sh .RM/_do_find_

## .RM/_foo_ has the patterns not seen in the reference

sh .RM/_foo_ |
	sed "s;^;egrep -e \'[0-9] ;" |
	sed 's;||;\\|\\|;g'  |
	sed 's;\&;\\\&;g'  |
	sed 's;\+;\\+;g'  |
	sed 's;\*;\\*;g' |
	sed "s; *$; ##\' .RM/Ref_Src_Space;" > .RM/_suspicious_

echo "set -v" > .RM/_showthis_

sh .RM/_suspicious_|
	awk '
		{ printf("for_line %s ", $1)
		  $1="";
		  print "## " $0
		}
	' |
	sed 's;:; ;'  >> .RM/_showthis_

sh .RM/_showthis_ 2>&1 |
	sed 's;for_line ;;' > .RM/Ref_Src_anomalies

if [ -s .RM/Ref_Src_anomalies ]
then
	echo -n "control patterns in this source not in reference model: "
	grep -c -e "^\." .RM/Ref_Src_anomalies
	echo "File: .RM/Ref_Src_anomalies:"
	cat .RM/Ref_Src_anomalies
else
	echo "no anomalies detected"
fi

if true
then
	# cleanup
	rm -f .RM/_do_find_ .RM/_to_find_ 
	rm -f .RM/_foo_ .RM/_suspicious_ .RM/_showthis_
fi

exit 0
