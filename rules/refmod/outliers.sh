#!/bin/bash

# generate the source text for matches in the control part
# that appear only once in .RM/Ref_Mod_control_sorted

if [ -d .RM ]
then
	if [ -s .RM/Ref_Mod_control_sorted ]
	then	true
	else
		echo "error: file ./.RM/Ref_Mod_control_sorted does not exist"
		echo "       run the full refmod/build first"
	fi
else
	echo "error: directory ./.RM does not exist"
	echo "       run refmod/build first"
	exit 1
fi

# make matching simpler by mapping white-space sequences to a single space
sed 's;[ 	][ 	]*; ;g' < .RM/Ref_Src > .RM/Ref_Src_Space

# should sort the selected .RM/Ref_Mod_control_sorted entries by length
# and only look at the first N shortest entries

echo "set -v" > .RM/_tmp4_
cat .RM/Ref_Mod_control_sorted |
	grep -e ".*1 ." |
	sed 's;.*[0-9][0-9]*  ;;' |
	awk 'NF>0 { print length($0), $0 }' |
	sort -n | awk ' { $1=""; print $0; }' | 	## tail -25 |
	sed 's;^;egrep -e "[0-9] ;' |
	sed 's;$; ## _ _ $" .RM/Ref_Src_Space;' |
	sed "s;\";';g" |
	sed 's;[ 	][ 	]*; ;g' |
	sed 's;||;\\|\\|;g'  |
	sed 's;\&;\\\&;g'  |
	sed 's;\+;\\+;g'  |
	sed 's;\*;\\*;g' |
	sed 's;\$;*$;' >> .RM/_tmp4_

## these are unique control patterns,
## so each can match many full patterns
## we can limit the output by looking only at:
##	the longest patterns (tail -25 above) and
##	those with _ _ in the loop-body (added ## _ _ $ above)
##
## [[can still be hard to see the use
##   of which loop var was matched]]

echo "set -v" > .RM/_tmp5_
sh .RM/_tmp4_  2>&1 |
	awk '$1!="egrep" { print $1 } $1=="egrep" { print "echo " $0 }' |
	sed 's;:; ;' |
	sed 's;^\([^e]\);for_line \1;' >> .RM/_tmp5_

# limit to one match per pattern per file
cat .RM/_tmp5_ |
	awk '
	BEGIN	{ Last="" }
		{ if ($1 != Last || $1 != "for_line") { print $0; Last = $1; } }' > .RM/_tmp6_

sh .RM/_tmp6_ 2>&1 |
	grep -v -e "echo" |
	sed 's; .RM/Ref_Src_Space$;;' |
	sed 's;for_line ;file: ;' |
	sed 's;egrep -e;\npattern:;'

if true
then	# cleanup
	rm -f .RM/_tmp[456]_
fi

exit 0
