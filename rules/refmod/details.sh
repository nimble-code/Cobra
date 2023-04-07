#!/bin/bash

# process output from: cobra -eol -f refmod/suspicious.cobra .RM/Ref_Mod_sorted
# and locate the patterns matched in file .RM/Ref_Src_Space
# selecting just the file:linenr lines in the output from 'd'

grep -e "Ref_Mod_sorted:" |
	sed 's;:; ;g' |
	sed 's;^;line .RM/;' > .RM/_tmp1_
sh .RM/_tmp1_ |
	sed 's;.*[0-9][0-9]* ;;' |
	sed 's;^;egrep -e ";' |
	sed 's;$;$" .RM/Ref_Src_Space;' |
	sed "s;\";';g" |
	sed 's;||;\\|\\|;g'  |
	sed 's;\&;\\\&;g'  |
	sed 's;\+;\\+;g'  |
	sed 's;\*;\\*;g' > .RM/_tmp2_

# .RM/_tmp2_ greps for the patterns in file .RM/Ref_Src
# the output has file:linenr pattern
# we then select just the file:linenr part
# and turn that into line commands in file .RM/_tmp3_
# to get the source text for each match

echo "set -v" > .RM/_tmp3_
sh .RM/_tmp2_ |
	awk ' { print $1 }' |
	sed 's;:; ;' |
	sed 's;^;for_line ;' >> .RM/_tmp3_
sh .RM/_tmp3_  2>&1 |
	sed 's;for_line ;;'

# could detect if 'for' isn't part of the output
# and then iteratively include more lines before the
# line selected until the 'for' appears

if true
then	# cleanup
	rm -f .RM/_tmp[123]_
fi

exit 0
