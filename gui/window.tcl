#!/bin/sh
# The next line is executed by /bin/sh, but not tcl \
exec wish "$0" -- $*

## Cobra file display -- gjh
## (c) 2017 All Rights Reserved

set W 100
set H  24

wm title . "icobra"
wm geometry . ${W}x$H+20+20	;# widthxheight+offsetx+offsety

proc showfile {fn n} {
	global W H

	set gn $fn
	set x [string last "/" $fn]
	if {$x >= 0} {
		incr x
		set gn [string range $fn $x end]
	}
	scan $gn "%\[a-zA-z_\]\." f

	frame .p

	text .p.t -relief raised -bd 2 \
		-width $W -height $H \
		-yscrollcommand ".p.ys set" \
		-xscrollcommand ".p.xs set" \
		-setgrid 1

	scrollbar .p.xs -command ".p.t xview" -orient horiz
	scrollbar .p.ys -command ".p.t yview"

	pack .p.xs -side bottom -fill x
	pack .p.ys -side right  -fill y
	pack .p.t -side left
	pack .p

	set fd -1
	catch { set fd [open $fn r] } errmsg
	if {$fd < 0} {
		puts "$errmsg\n"
		exit 1
	}
	wm title . "$fn : $n"
	set cnt 1
	while {[gets $fd line] >= 0} {
		.p.t insert end "$cnt  $line\n"
		.p.t tag add sel$cnt $cnt.0 $cnt.end
		incr cnt
	}
	catch "close $fd"

	.p.t tag configure sel$n -foreground red ;# -background green

	.p.t yview -pickplace [expr $n - 12]
}

if {$argc != 2} {
	puts "usage: foo filename linenr"
	exit 1
}

showfile [lindex $argv 0] [lindex $argv 1]
