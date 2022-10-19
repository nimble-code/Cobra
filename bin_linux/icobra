#!/bin/sh
# The next line is executed by /bin/sh, but not tcl \
exec wish "$0" -- $*

## Cobra GUI -- gh @ Nimble Research
## (c) 2022 All Rights Reserved
## requires Cobra version 4.1 or later

## location of main panels

## tuned for a display of 2560x1440 resolution

	set WMain 1200		;# the main window
	set HMain  800
	set XMain   20
	set YMain   50

	set Yoffset $YMain	;# side pannels, like predefined checks

	set Wpredefined 300
	set Hpredefined 600
	set Xpredefined [expr $XMain + $WMain + 10]
	set Ypredefined $Yoffset

	set Wpatternsearch 480
	set Hpatternsearch 100
	set Xpatternsearch [expr $XMain + $WMain + $Wpredefined + 20]
	set Ypatternsearch $YMain

	set Wpatternmatch 400
	set Hpatternmatch 150
	set Xpatternmatch $Xpatternsearch
	set Ypatternmatch [expr $YMain + $Hpatternsearch + 20 + 20 + 10]

	set Wpatterndetail 300
	set Hpatterndetail 0	;# adjusted dynamically
	set Xpatterndetail [expr $Xpatternmatch + $Wpatternmatch + 10]
	set Ypatterndetail $Ypatternmatch

	set Wfind 820
	set Hfind  80
	set Xfind $XMain
	set Yfind [expr $HMain + $YMain + 20 + 20]

	set Wpatternsets 480
	set Hpatternsets 120
	set Xpatternsets $Xpatternsearch
	set Ypatternsets [expr $Ypatternsearch + $Hpatternsearch + 20 + 20 + 10]

## end configurable locations

wm title . "icobra"
wm geometry . ${WMain}x$HMain+$XMain+$YMain	;# widthxheight+offsetx+offsety
update

set iversion "iCobra Version 1.2";	# was 2015.10.16
set version  "Cobra Version unknown";	# updated below
set Unix 1;				# updated below
set COBRA cobra;			# background tool
set LOGO "/cygdrive/f/Dropbox_orig/Tools/Cobra/GUI/cobra_small.gif"
set d_mode 1;				# src display mode
set nrpat 0
set verbose 0
set linenumbers 0
set bw_mode 0

### Tools
	## check if we have the right version
	if {[auto_execok $COBRA] == "" \
	||  [auto_execok $COBRA] == 0} {
		puts "No executable $COBRA found..."
		exit 0
	} else {
		if [catch { set fd [open "|$COBRA -V" r] } errmsg] {
			puts "$errmsg"
			exit 0
		} else {
			if {[gets $fd line] > -1} {
				set version "$line"
			}
			catch "close $fd"
		}
		set nf 0
		catch { set nf [scan $version "Version %d.%d" a b] }
		if {$nf != 2 || $a < 4 || ($a == 4 && $b < 1)} {
			puts "this GUI requires Cobra Version 4.1 or later"
			puts "You have: $version"
			exit 0
	}	}

	if [info exists tcl_platform] {
		set sys $tcl_platform(platform)
		if {[string match windows $sys]} {
			set Unix 0	;# Windows
	}	}

### Some other configurable items
	set ScrollBarSize	10

### Colors
	set MBG azure     ;# menu
	set MFG black

	set TBG	azure	  ;#WhiteSmoke	;# text window
	set LTG lightgrey ;# background option
	set TFG	black
	set SFG	red       ;# text selections - standout from TBG

	set CBG black     ;# command window
	set CFG azure     ;# gold

	set NBG	darkblue  ;# main tabs
	set NFG gold

### Fonts
	set HV0 "helvetica 11"
	set HV1 "helvetica 12"
	set HV2 "helvetica 14"
	set CW1 "Courier 14"

### end of configurable items ##########################################
##                                                                    ##
## The first part of this code is based on the BWidget-1.9.2 package  ##
## To skip ahead to where the iSpin specific code starts,             ##
## search for "Cobra GUI" which starts about half-way down            ##
##                                                                    ##
########################################################################

#######
## The BWidget Toolkit comes with the following
## license text that is reproduced here.
#######
## BWidget ToolKit
## Copyright (c) 1998-1999 UNIFIX.
## Copyright (c) 2001-2002 ActiveState Corp.
## 
## The following terms apply to all files associated with the software
## unless explicitly disclaimed in individual files.
## 
## The authors hereby grant permission to use, copy, modify, distribute,
## and license this software and its documentation for any purpose, provided
## that existing copyright notices are retained in all copies and that this
## notice is included verbatim in any distributions. No written agreement,
## license, or royalty fee is required for any of the authorized uses.
## Modifications to this software may be copyrighted by their authors
## and need not follow the licensing terms described here, provided that
## the new terms are clearly indicated on the first page of each file where
## they apply.
## 
## IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
## FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
## ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
## DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
## 
## THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
## INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
## IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
## NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
## MODIFICATIONS.
## 
## GOVERNMENT USE: If you are acquiring this software on behalf of the
## U.S. government, the Government shall have only "Restricted Rights"
## in the software and related documentation as defined in the Federal
## Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
## are acquiring the software on behalf of the Department of Defense, the
## software shall be classified as "Commercial Computer Software" and the
## Government shall have only "Restricted Rights" as defined in Clause
## 252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing, the
## authors grant the U.S. Government and others acting in its behalf
## permission to use and distribute the software in accordance with the
## terms specified in this license.
#######

namespace eval Widget {}

proc Widget::_opt_defaults {{prio widgetDefault}} {
    if {$::tcl_version >= 8.4} {
	set plat [tk windowingsystem]
    } else {
	set plat $::tcl_platform(platform)
    }
    switch -exact $plat {
	"aqua" {
	}
	"win32" -
	"windows" {
	    option add *ListBox.background	SystemWindow $prio
	    option add *Dialog.padY		0 $prio
	    option add *Dialog.anchor		e $prio
	}
	"x11" -
	default {
	    option add *Scrollbar.width		12 $prio
	    option add *Scrollbar.borderWidth	1  $prio
	    option add *Dialog.separator	1  $prio
	    option add *MainFrame.relief	raised $prio
	    option add *MainFrame.separator	none   $prio
	}
    }
}

Widget::_opt_defaults

bind Entry <<TraverseIn>> { %W selection range 0 end; %W icursor end }
bind all   <Key-Tab>      { Widget::traverseTo [Widget::focusNext %W] }
bind all   <<PrevWindow>> { Widget::traverseTo [Widget::focusPrev %W] }

# ----------------------------------------------------------------------------
#  widget.tcl -- part of Unifix BWidget Toolkit
# ----------------------------------------------------------------------------

# Uses newer string operations
package require Tcl 8.1.1

namespace eval Widget {
    variable _optiontype
    variable _class
    variable _tk_widget

    # This controls whether we try to use themed widgets from Tile
    variable _theme 0

    variable _aqua [expr {($::tcl_version >= 8.4) &&
			  [string equal [tk windowingsystem] "aqua"]}]

    array set _optiontype {
        TkResource Widget::_test_tkresource
        BwResource Widget::_test_bwresource
        Enum       Widget::_test_enum
        Int        Widget::_test_int
        Boolean    Widget::_test_boolean
        String     Widget::_test_string
        Flag       Widget::_test_flag
        Synonym    Widget::_test_synonym
        Color      Widget::_test_color
        Padding    Widget::_test_padding
    }

    proc use {} {}
}

proc Widget::tkinclude { class tkwidget subpath args } {
    foreach {cmd lopt} $args {
        switch -- $cmd {
            remove {
                foreach option $lopt {
                    set remove($option) 1
                }
            }
            include {
                foreach option $lopt {
                    set include($option) 1
                }
            }
            prefix {
                set prefix [lindex $lopt 0]
                foreach option [lrange $lopt 1 end] {
                    set rename($option) "-$prefix[string range $option 1 end]"
                }
            }
            rename     -
            readonly   -
            initialize {
                array set $cmd $lopt
            }
            default {
                return -code error "invalid argument \"$cmd\""
            }
        }
    }

    namespace eval $class {}
    upvar 0 ${class}::opt classopt
    upvar 0 ${class}::map classmap
    upvar 0 ${class}::map$subpath submap
    upvar 0 ${class}::optionExports exports

    set foo [$tkwidget ".ericFoo###"]
    # create resources informations from tk widget resources
    foreach optdesc [_get_tkwidget_options $tkwidget] {
        set option [lindex $optdesc 0]
        if { (![info exists include] || [info exists include($option)]) &&
             ![info exists remove($option)] } {
            if { [llength $optdesc] == 3 } {
                # option is a synonym
                set syn [lindex $optdesc 1]
                if { ![info exists remove($syn)] } {
                    # original option is not removed
                    if { [info exists rename($syn)] } {
                        set classopt($option) [list Synonym $rename($syn)]
                    } else {
                        set classopt($option) [list Synonym $syn]
                    }
                }
            } else {
                if { [info exists rename($option)] } {
                    set realopt $option
                    set option  $rename($option)
                } else {
                    set realopt $option
                }
                if { [info exists initialize($option)] } {
                    set value $initialize($option)
                } else {
                    set value [lindex $optdesc 1]
                }
                if { [info exists readonly($option)] } {
                    set ro $readonly($option)
                } else {
                    set ro 0
                }
                set classopt($option) \
			[list TkResource $value $ro [list $tkwidget $realopt]]

		# Add an option database entry for this option
		set optionDbName ".[lindex [_configure_option $realopt ""] 0]"
		if { ![string equal $subpath ":cmd"] } {
		    set optionDbName "$subpath$optionDbName"
		}
		option add *${class}$optionDbName $value widgetDefault
		lappend exports($option) "$optionDbName"

		# Store the forward and backward mappings for this
		# option <-> realoption pair
                lappend classmap($option) $subpath "" $realopt
		set submap($realopt) $option
            }
        }
    }
    ::destroy $foo
}

proc Widget::bwinclude { class subclass subpath args } {
    foreach {cmd lopt} $args {
        switch -- $cmd {
            remove {
                foreach option $lopt {
                    set remove($option) 1
                }
            }
            include {
                foreach option $lopt {
                    set include($option) 1
                }
            }
            prefix {
                set prefix [lindex $lopt 0]
                foreach option [lrange $lopt 1 end] {
                    set rename($option) "-$prefix[string range $option 1 end]"
                }
            }
            rename     -
            readonly   -
            initialize {
                array set $cmd $lopt
            }
            default {
                return -code error "invalid argument \"$cmd\""
            }
        }
    }

    namespace eval $class {}
    upvar 0 ${class}::opt classopt
    upvar 0 ${class}::map classmap
    upvar 0 ${class}::map$subpath submap
    upvar 0 ${class}::optionExports exports
    upvar 0 ${subclass}::opt subclassopt
    upvar 0 ${subclass}::optionExports subexports

    # create resources informations from BWidget resources
    foreach {option optdesc} [array get subclassopt] {
	set subOption $option
        if { (![info exists include] || [info exists include($option)]) &&
             ![info exists remove($option)] } {
            set type [lindex $optdesc 0]
            if { [string equal $type "Synonym"] } {
                # option is a synonym
                set syn [lindex $optdesc 1]
                if { ![info exists remove($syn)] } {
                    if { [info exists rename($syn)] } {
                        set classopt($option) [list Synonym $rename($syn)]
                    } else {
                        set classopt($option) [list Synonym $syn]
                    }
                }
            } else {
                if { [info exists rename($option)] } {
                    set realopt $option
                    set option  $rename($option)
                } else {
                    set realopt $option
                }
                if { [info exists initialize($option)] } {
                    set value $initialize($option)
                } else {
                    set value [lindex $optdesc 1]
                }
                if { [info exists readonly($option)] } {
                    set ro $readonly($option)
                } else {
                    set ro [lindex $optdesc 2]
                }
                set classopt($option) \
			[list $type $value $ro [lindex $optdesc 3]]

		# Add an option database entry for this option
		foreach optionDbName $subexports($subOption) {
		    if { ![string equal $subpath ":cmd"] } {
			set optionDbName "$subpath$optionDbName"
		    }
		    # Only add the option db entry if we are overriding the
		    # normal widget default
		    if { [info exists initialize($option)] } {
			option add *${class}$optionDbName $value \
				widgetDefault
		    }
		    lappend exports($option) "$optionDbName"
		}

		# Store the forward and backward mappings for this
		# option <-> realoption pair
                lappend classmap($option) $subpath $subclass $realopt
		set submap($realopt) $option
            }
        }
    }
}

proc Widget::declare { class optlist } {
    variable _optiontype

    namespace eval $class {}
    upvar 0 ${class}::opt classopt
    upvar 0 ${class}::optionExports exports
    upvar 0 ${class}::optionClass optionClass

    foreach optdesc $optlist {
        set option  [lindex $optdesc 0]
        set optdesc [lrange $optdesc 1 end]
        set type    [lindex $optdesc 0]

        if { ![info exists _optiontype($type)] } {
            # invalid resource type
            return -code error "invalid option type \"$type\""
        }

        if { [string equal $type "Synonym"] } {
            # test existence of synonym option
            set syn [lindex $optdesc 1]
            if { ![info exists classopt($syn)] } {
                return -code error "unknow option \"$syn\" for Synonym \"$option\""
            }
            set classopt($option) [list Synonym $syn]
            continue
        }

        # all other resource may have default value, readonly flag and
        # optional arg depending on type
        set value [lindex $optdesc 1]
        set ro    [lindex $optdesc 2]
        set arg   [lindex $optdesc 3]

        if { [string equal $type "BwResource"] } {
            # We don't keep BwResource. We simplify to type of sub BWidget
            set subclass    [lindex $arg 0]
            set realopt     [lindex $arg 1]
            if { ![string length $realopt] } {
                set realopt $option
            }

            upvar 0 ${subclass}::opt subclassopt
            if { ![info exists subclassopt($realopt)] } {
                return -code error "unknow option \"$realopt\""
            }
            set suboptdesc $subclassopt($realopt)
            if { $value == "" } {
                # We initialize default value
                set value [lindex $suboptdesc 1]
            }
            set type [lindex $suboptdesc 0]
            set ro   [lindex $suboptdesc 2]
            set arg  [lindex $suboptdesc 3]
	    set optionDbName ".[lindex [_configure_option $option ""] 0]"
	    option add *${class}${optionDbName} $value widgetDefault
	    set exports($option) $optionDbName
            set classopt($option) [list $type $value $ro $arg]
            continue
        }

        # retreive default value for TkResource
        if { [string equal $type "TkResource"] } {
            set tkwidget [lindex $arg 0]
	    set foo [$tkwidget ".ericFoo##"]
            set realopt  [lindex $arg 1]
            if { ![string length $realopt] } {
                set realopt $option
            }
            set tkoptions [_get_tkwidget_options $tkwidget]
            if { ![string length $value] } {
                # We initialize default value
		set ind [lsearch $tkoptions [list $realopt *]]
                set value [lindex [lindex $tkoptions $ind] end]
            }
	    set optionDbName ".[lindex [_configure_option $option ""] 0]"
	    option add *${class}${optionDbName} $value widgetDefault
	    set exports($option) $optionDbName
            set classopt($option) [list TkResource $value $ro \
		    [list $tkwidget $realopt]]
	    set optionClass($option) [lindex [$foo configure $realopt] 1]
	    ::destroy $foo
            continue
        }

	set optionDbName ".[lindex [_configure_option $option ""] 0]"
	option add *${class}${optionDbName} $value widgetDefault
	set exports($option) $optionDbName
        # for any other resource type, we keep original optdesc
        set classopt($option) [list $type $value $ro $arg]
    }
}

proc Widget::define { class filename args } {
    variable ::BWidget::use
    set use($class)      $args
    set use($class,file) $filename
    lappend use(classes) $class

    if {[set x [lsearch -exact $args "-classonly"]] > -1} {
	set args [lreplace $args $x $x]
    } else {
	interp alias {} ::${class} {} ${class}::create
	proc ::${class}::use {} {}

	bind $class <Destroy> [list Widget::destroy %W]
    }

    foreach class $args { ${class}::use }
}

proc Widget::create { class path {rename 1} } {
    if {$rename} { rename $path ::$path:cmd }
    proc ::$path { cmd args } \
    	[subst {return \[eval \[linsert \$args 0 ${class}::\$cmd [list $path]\]\]}]
    return $path
}

proc Widget::addmap { class subclass subpath options } {
    upvar 0 ${class}::opt classopt
    upvar 0 ${class}::optionExports exports
    upvar 0 ${class}::optionClass optionClass
    upvar 0 ${class}::map classmap
    upvar 0 ${class}::map$subpath submap

    foreach {option realopt} $options {
        if { ![string length $realopt] } {
            set realopt $option
        }
	set val [lindex $classopt($option) 1]
	set optDb ".[lindex [_configure_option $realopt ""] 0]"
	if { ![string equal $subpath ":cmd"] } {
	    set optDb "$subpath$optDb"
	}
	option add *${class}${optDb} $val widgetDefault
	lappend exports($option) $optDb
	# Store the forward and backward mappings for this
	# option <-> realoption pair
        lappend classmap($option) $subpath $subclass $realopt
	set submap($realopt) $option
    }
}

proc Widget::syncoptions { class subclass subpath options } {
    upvar 0 ${class}::sync classync

    foreach {option realopt} $options {
        if { ![string length $realopt] } {
            set realopt $option
        }
        set classync($option) [list $subpath $subclass $realopt]
    }
}

proc Widget::init { class path options } {
    variable _inuse
    variable _class
    variable _optiontype

    upvar 0 ${class}::opt classopt
    upvar 0 ${class}::$path:opt  pathopt
    upvar 0 ${class}::$path:mod  pathmod
    upvar 0 ${class}::map classmap
    upvar 0 ${class}::$path:init pathinit

    if { [info exists pathopt] } {
	unset pathopt
    }
    if { [info exists pathmod] } {
	unset pathmod
    }

    set fpath $path
    set rdbclass [string map [list :: ""] $class]
    if { ![winfo exists $path] } {
	set fpath ".#BWidget.#Class#$class"
	# encapsulation frame to not pollute '.' childspace
	if {![winfo exists ".#BWidget"]} { frame ".#BWidget" }
	if { ![winfo exists $fpath] } {
	    frame $fpath -class $rdbclass
	}
    }
    foreach {option optdesc} [array get classopt] {
        set pathmod($option) 0
	if { [info exists classmap($option)] } {
	    continue
	}
        set type [lindex $optdesc 0]
        if { [string equal $type "Synonym"] } {
	    continue
        }
        if { [string equal $type "TkResource"] } {
            set alt [lindex [lindex $optdesc 3] 1]
        } else {
            set alt ""
        }
        set optdb [lindex [_configure_option $option $alt] 0]
        set def   [option get $fpath $optdb $rdbclass]
        if { [string length $def] } {
            set pathopt($option) $def
        } else {
            set pathopt($option) [lindex $optdesc 1]
        }
    }

    if {![info exists _inuse($class)]} { set _inuse($class) 0 }
    incr _inuse($class)

    set _class($path) $class
    foreach {option value} $options {
        if { ![info exists classopt($option)] } {
            unset pathopt
            unset pathmod
            return -code error "unknown option \"$option\""
        }
        set optdesc $classopt($option)
        set type    [lindex $optdesc 0]
        if { [string equal $type "Synonym"] } {
            set option  [lindex $optdesc 1]
            set optdesc $classopt($option)
            set type    [lindex $optdesc 0]
        }
        # this may fail if a wrong enum element was used
        if {[catch {
             $_optiontype($type) $option $value [lindex $optdesc 3]
        } msg]} {
            if {[info exists pathopt]} {
                unset pathopt
            }
            unset pathmod
            return -code error $msg
        }
        set pathopt($option) $msg
	set pathinit($option) $pathopt($option)
    }
}

proc Widget::parseArgs {class options} {
    variable _optiontype
    upvar 0 ${class}::opt classopt
    upvar 0 ${class}::map classmap
    
    foreach {option val} $options {
	if { ![info exists classopt($option)] } {
	    error "unknown option \"$option\""
	}
        set optdesc $classopt($option)
        set type    [lindex $optdesc 0]
        if { [string equal $type "Synonym"] } {
            set option  [lindex $optdesc 1]
            set optdesc $classopt($option)
            set type    [lindex $optdesc 0]
        }
	if { [string equal $type "TkResource"] } {
	    # Make sure that the widget used for this TkResource exists
	    Widget::_get_tkwidget_options [lindex [lindex $optdesc 3] 0]
	}
	set val [$_optiontype($type) $option $val [lindex $optdesc 3]]
		
	if { [info exists classmap($option)] } {
	    foreach {subpath subclass realopt} $classmap($option) {
		lappend maps($subpath) $realopt $val
	    }
	} else {
	    lappend maps($class) $option $val
	}
    }
    return [array get maps]
}

proc Widget::initFromODB {class path options} {
    variable _inuse
    variable _class

    upvar 0 ${class}::$path:opt  pathopt
    upvar 0 ${class}::$path:mod  pathmod
    upvar 0 ${class}::map classmap

    if { [info exists pathopt] } {
	unset pathopt
    }
    if { [info exists pathmod] } {
	unset pathmod
    }

    set fpath [_get_window $class $path]
    set rdbclass [string map [list :: ""] $class]
    if { ![winfo exists $path] } {
	set fpath ".#BWidget.#Class#$class"
	# encapsulation frame to not pollute '.' childspace
	if {![winfo exists ".#BWidget"]} { frame ".#BWidget" }
	if { ![winfo exists $fpath] } {
	    frame $fpath -class $rdbclass
	}
    }

    foreach {option optdesc} [array get ${class}::opt] {
        set pathmod($option) 0
	if { [info exists classmap($option)] } {
	    continue
	}
        set type [lindex $optdesc 0]
        if { [string equal $type "Synonym"] } {
	    continue
        }
	if { [string equal $type "TkResource"] } {
            set alt [lindex [lindex $optdesc 3] 1]
        } else {
            set alt ""
        }
        set optdb [lindex [_configure_option $option $alt] 0]
        set def   [option get $fpath $optdb $rdbclass]
        if { [string length $def] } {
            set pathopt($option) $def
        } else {
            set pathopt($option) [lindex $optdesc 1]
        }
    }

    if {![info exists _inuse($class)]} { set _inuse($class) 0 }
    incr _inuse($class)

    set _class($path) $class
    array set pathopt $options
}

proc Widget::destroy { path } {
    variable _class
    variable _inuse

    if {![info exists _class($path)]} { return }

    set class $_class($path)
    upvar 0 ${class}::$path:opt pathopt
    upvar 0 ${class}::$path:mod pathmod
    upvar 0 ${class}::$path:init pathinit

    if {[info exists _inuse($class)]} { incr _inuse($class) -1 }

    if {[info exists pathopt]} {
        unset pathopt
    }
    if {[info exists pathmod]} {
        unset pathmod
    }
    if {[info exists pathinit]} {
        unset pathinit
    }

    if {![string equal [info commands $path] ""]} { rename $path "" }

    ## Unset any variables used in this widget.
    foreach var [info vars ::${class}::$path:*] { unset $var }

    unset _class($path)
}

proc Widget::configure { path options } {
    set len [llength $options]
    if { $len <= 1 } {
        return [_get_configure $path $options]
    } elseif { $len % 2 == 1 } {
        return -code error "incorrect number of arguments"
    }

    variable _class
    variable _optiontype

    set class $_class($path)
    upvar 0 ${class}::opt  classopt
    upvar 0 ${class}::map  classmap
    upvar 0 ${class}::$path:opt pathopt
    upvar 0 ${class}::$path:mod pathmod

    set window [_get_window $class $path]
    foreach {option value} $options {
        if { ![info exists classopt($option)] } {
            return -code error "unknown option \"$option\""
        }
        set optdesc $classopt($option)
        set type    [lindex $optdesc 0]
        if { [string equal $type "Synonym"] } {
            set option  [lindex $optdesc 1]
            set optdesc $classopt($option)
            set type    [lindex $optdesc 0]
        }
        if { ![lindex $optdesc 2] } {
            set newval [$_optiontype($type) $option $value [lindex $optdesc 3]]
            if { [info exists classmap($option)] } {
		set window [_get_window $class $window]
                foreach {subpath subclass realopt} $classmap($option) {
                    if { [string length $subclass] && ! [string equal $subclass ":cmd"] } {
                        if { [string equal $subpath ":cmd"] } {
                            set subpath ""
                        }
                        set curval [${subclass}::cget $window$subpath $realopt]
                        ${subclass}::configure $window$subpath $realopt $newval
                    } else {
                        set curval [$window$subpath cget $realopt]
                        $window$subpath configure $realopt $newval
                    }
                }
            } else {
		set curval $pathopt($option)
		set pathopt($option) $newval
	    }
	    set pathmod($option) [expr {![string equal $newval $curval]}]
        }
    }

    return {}
}

proc Widget::cget { path option } {
    variable _class
    if { ![info exists _class($path)] } {
        return -code error "unknown widget $path"
    }

    set class $_class($path)
    if { ![info exists ${class}::opt($option)] } {
        return -code error "unknown option \"$option\""
    }

    set optdesc [set ${class}::opt($option)]
    set type    [lindex $optdesc 0]
    if {[string equal $type "Synonym"]} {
        set option [lindex $optdesc 1]
    }

    if { [info exists ${class}::map($option)] } {
	foreach {subpath subclass realopt} [set ${class}::map($option)] {break}
	set path "[_get_window $class $path]$subpath"
	return [$path cget $realopt]
    }
    upvar 0 ${class}::$path:opt pathopt
    set pathopt($option)
}

proc Widget::subcget { path subwidget } {
    variable _class
    set class $_class($path)
    upvar 0 ${class}::$path:opt pathopt
    upvar 0 ${class}::map$subwidget submap
    upvar 0 ${class}::$path:init pathinit

    set result {}
    foreach realopt [array names submap] {
	if { [info exists pathinit($submap($realopt))] } {
	    lappend result $realopt $pathopt($submap($realopt))
	}
    }
    return $result
}

proc Widget::hasChanged { path option pvalue } {
    variable _class
    upvar $pvalue value
    set class $_class($path)
    upvar 0 ${class}::$path:mod pathmod

    set value   [Widget::cget $path $option]
    set result  $pathmod($option)
    set pathmod($option) 0

    return $result
}

proc Widget::hasChangedX { path option args } {
    variable _class
    set class $_class($path)
    upvar 0 ${class}::$path:mod pathmod

    set result  $pathmod($option)
    set pathmod($option) 0
    foreach option $args {
	lappend result $pathmod($option)
	set pathmod($option) 0
    }

    set result
}

proc Widget::setoption { path option value } {
    Widget::configure $path [list $option $value]
}

proc Widget::getoption { path option } {
    return [Widget::cget $path $option]
}

proc Widget::getMegawidgetOption {path option} {
    variable _class
    set class $_class($path)
    upvar 0 ${class}::${path}:opt pathopt
    set pathopt($option)
}

proc Widget::setMegawidgetOption {path option value} {
    variable _class
    set class $_class($path)
    upvar 0 ${class}::${path}:opt pathopt
    set pathopt($option) $value
}

proc Widget::_get_window { class path } {
    set idx [string last "#" $path]
    if { $idx != -1 && [string equal [string range $path [expr {$idx+1}] end] $class] } {
        return [string range $path 0 [expr {$idx-1}]]
    } else {
        return $path
    }
}

proc Widget::_get_configure { path options } {
    variable _class

    set class $_class($path)
    upvar 0 ${class}::opt classopt
    upvar 0 ${class}::map classmap
    upvar 0 ${class}::$path:opt pathopt
    upvar 0 ${class}::$path:mod pathmod

    set len [llength $options]
    if { !$len } {
        set result {}
        foreach option [lsort [array names classopt]] {
            set optdesc $classopt($option)
            set type    [lindex $optdesc 0]
            if { [string equal $type "Synonym"] } {
                set syn     $option
                set option  [lindex $optdesc 1]
                set optdesc $classopt($option)
                set type    [lindex $optdesc 0]
            } else {
                set syn ""
            }
            if { [string equal $type "TkResource"] } {
                set alt [lindex [lindex $optdesc 3] 1]
            } else {
                set alt ""
            }
            set res [_configure_option $option $alt]
            if { $syn == "" } {
                lappend result [concat $option $res [list [lindex $optdesc 1]] [list [cget $path $option]]]
            } else {
                lappend result [list $syn [lindex $res 0]]
            }
        }
        return $result
    } elseif { $len == 1 } {
        set option  [lindex $options 0]
        if { ![info exists classopt($option)] } {
            return -code error "unknown option \"$option\""
        }
        set optdesc $classopt($option)
        set type    [lindex $optdesc 0]
        if { [string equal $type "Synonym"] } {
            set option  [lindex $optdesc 1]
            set optdesc $classopt($option)
            set type    [lindex $optdesc 0]
        }
        if { [string equal $type "TkResource"] } {
            set alt [lindex [lindex $optdesc 3] 1]
        } else {
            set alt ""
        }
        set res [_configure_option $option $alt]
        return [concat $option $res [list [lindex $optdesc 1]] [list [cget $path $option]]]
    }
}

proc Widget::_configure_option { option altopt } {
    variable _optiondb
    variable _optionclass

    if { [info exists _optiondb($option)] } {
        set optdb $_optiondb($option)
    } else {
        set optdb [string range $option 1 end]
    }
    if { [info exists _optionclass($option)] } {
        set optclass $_optionclass($option)
    } elseif { [string length $altopt] } {
        if { [info exists _optionclass($altopt)] } {
            set optclass $_optionclass($altopt)
        } else {
            set optclass [string range $altopt 1 end]
        }
    } else {
        set optclass [string range $option 1 end]
    }
    return [list $optdb $optclass]
}

proc Widget::_get_tkwidget_options { tkwidget } {
    variable _tk_widget
    variable _optiondb
    variable _optionclass

    set widget ".#BWidget.#$tkwidget"
    # encapsulation frame to not pollute '.' childspace
    if {![winfo exists ".#BWidget"]} { frame ".#BWidget" }
    if { ![winfo exists $widget] || ![info exists _tk_widget($tkwidget)] } {
	set widget [$tkwidget $widget]
	# JDC: Withdraw toplevels, otherwise visible
	if {[string equal $tkwidget "toplevel"]} {
	    wm withdraw $widget
	}
	set config [$widget configure]
	foreach optlist $config {
	    set opt [lindex $optlist 0]
	    if { [llength $optlist] == 2 } {
		set refsyn [lindex $optlist 1]
		# search for class
		set idx [lsearch $config [list * $refsyn *]]
		if { $idx == -1 } {
		    if { [string index $refsyn 0] == "-" } {
			# search for option (tk8.1b1 bug)
			set idx [lsearch $config [list $refsyn * *]]
		    } else {
			# last resort
			set idx [lsearch $config [list -[string tolower $refsyn] * *]]
		    }
		    if { $idx == -1 } {
			# fed up with "can't read classopt()"
			return -code error "can't find option of synonym $opt"
		    }
		}
		set syn [lindex [lindex $config $idx] 0]
		# JDC: used 4 (was 3) to get def from optiondb
		set def [lindex [lindex $config $idx] 4]
		lappend _tk_widget($tkwidget) [list $opt $syn $def]
	    } else {
		# JDC: used 4 (was 3) to get def from optiondb
		set def [lindex $optlist 4]
		lappend _tk_widget($tkwidget) [list $opt $def]
		set _optiondb($opt)    [lindex $optlist 1]
		set _optionclass($opt) [lindex $optlist 2]
	    }
	}
    }
    return $_tk_widget($tkwidget)
}

proc Widget::_test_tkresource { option value arg } {
    foreach {tkwidget realopt} $arg break
    set path     ".#BWidget.#$tkwidget"
    set old      [$path cget $realopt]
    $path configure $realopt $value
    set res      [$path cget $realopt]
    $path configure $realopt $old

    return $res
}

proc Widget::_test_bwresource { option value arg } {
    return -code error "bad option type BwResource in widget"
}

proc Widget::_test_synonym { option value arg } {
    return -code error "bad option type Synonym in widget"
}

proc Widget::_test_color { option value arg } {
    if {[catch {winfo rgb . $value} color]} {
        return -code error "bad $option value \"$value\": must be a colorname \
		or #RRGGBB triplet"
    }

    return $value
}

proc Widget::_test_string { option value arg } {
    set value
}

proc Widget::_test_flag { option value arg } {
    set len [string length $value]
    set res ""
    for {set i 0} {$i < $len} {incr i} {
        set c [string index $value $i]
        if { [string first $c $arg] == -1 } {
            return -code error "bad [string range $option 1 end] value \"$value\": characters must be in \"$arg\""
        }
        if { [string first $c $res] == -1 } {
            append res $c
        }
    }
    return $res
}

proc Widget::_test_enum { option value arg } {
    if { [lsearch $arg $value] == -1 } {
        set last [lindex   $arg end]
        set sub  [lreplace $arg end end]
        if { [llength $sub] } {
            set str "[join $sub ", "] or $last"
        } else {
            set str $last
        }
        return -code error "bad [string range $option 1 end] value \"$value\": must be $str"
    }
    return $value
}

proc Widget::_test_int { option value arg } {
    if { ![string is int -strict $value] || \
	    ([string length $arg] && \
	    ![expr [string map [list %d $value] $arg]]) } {
		    return -code error "bad $option value\
			    \"$value\": must be integer ($arg)"
    }
    return $value
}

proc Widget::_test_boolean { option value arg } {
    if { ![string is boolean -strict $value] } {
        return -code error "bad $option value \"$value\": must be boolean"
    }

    # Get the canonical form of the boolean value (1 for true, 0 for false)
    return [string is true $value]
}

proc Widget::_test_padding { option values arg } {
    set len [llength $values]
    if {$len < 1 || $len > 2} {
        return -code error "bad pad value \"$values\":\
                        must be positive screen distance"
    }

    foreach value $values {
        if { ![string is int -strict $value] || \
            ([string length $arg] && \
            ![expr [string map [list %d $value] $arg]]) } {
                return -code error "bad pad value \"$value\":\
                                must be positive screen distance ($arg)"
        }
    }
    return $values
}

proc Widget::_get_padding { path option {index 0} } {
    set pad [Widget::cget $path $option]
    set val [lindex $pad $index]
    if {$val == ""} { set val [lindex $pad 0] }
    return $val
}

proc Widget::focusNext { w } {
    set cur $w
    while 1 {
	# Descend to just before the first child of the current widget.
	set parent $cur
	set children [winfo children $cur]
	set i -1

	# Look for the next sibling that isn't a top-level.
	while 1 {
	    incr i
	    if {$i < [llength $children]} {
		set cur [lindex $children $i]
		if {[string equal [winfo toplevel $cur] $cur]} {
		    continue
		} else {
		    break
		}
	    }

	    set cur $parent
	    if {[string equal [winfo toplevel $cur] $cur]} {
		break
	    }
	    set parent [winfo parent $parent]
	    set children [winfo children $parent]
	    set i [lsearch -exact $children $cur]
	}
	if {[string equal $cur $w] || [focusOK $cur]} {
	    return $cur
	}
    }
}

proc Widget::focusPrev { w } {
    set cur $w
    set origParent [winfo parent $w]
    while 1 {

	if {[string equal [winfo toplevel $cur] $cur]}  {
	    set parent $cur
	    set children [winfo children $cur]
	    set i [llength $children]
	} else {
	    set parent [winfo parent $cur]
	    set children [winfo children $parent]
	    set i [lsearch -exact $children $cur]
	}

	while {$i > 0} {
	    incr i -1
	    set cur [lindex $children $i]
	    if {[string equal [winfo toplevel $cur] $cur]} {
		continue
	    }
	    set parent $cur
	    set children [winfo children $parent]
	    set i [llength $children]
	}
	set cur $parent
	if {[string equal $cur $w]} {
	    return $cur
	}

	if {[string equal $cur $origParent]
	    && [info procs ::$origParent] != ""} {
	    continue
	}
	if {[focusOK $cur]} {
	    return $cur
	}
    }
}

proc Widget::focusOK { w } {
    set code [catch {$w cget -takefocus} value]
    if { $code == 1 } {
        return 0
    }
    if {($code == 0) && ($value != "")} {
	if {$value == 0} {
	    return 0
	} elseif {$value == 1} {
	    return [winfo viewable $w]
	} else {
	    set value [uplevel \#0 $value $w]
            if {$value != ""} {
		return $value
	    }
        }
    }
    if {![winfo viewable $w]} {
	return 0
    }
    set code [catch {$w cget -state} value]
    if {($code == 0) && ($value == "disabled")} {
	return 0
    }
    set code [catch {$w cget -editable} value]
    if {($code == 0) && ($value == 0)} {
        return 0
    }

    set top [winfo toplevel $w]
    foreach tags [bindtags $w] {
        if { ![string equal $tags $top]  &&
             ![string equal $tags "all"] &&
             [regexp Key [bind $tags]] } {
            return 1
        }
    }
    return 0
}

proc Widget::traverseTo { w } {
    set focus [focus]
    if {![string equal $focus ""]} {
	event generate $focus <<TraverseOut>>
    }
    focus $w

    event generate $w <<TraverseIn>>
}

proc Widget::getVariable { path varName {newVarName ""} } {
    variable _class
    set class $_class($path)
    if {![string length $newVarName]} { set newVarName $varName }
    uplevel 1 [list upvar \#0 ${class}::$path:$varName $newVarName]
}

proc Widget::options { path args } {
    if {[llength $args]} {
        foreach option $args {
            lappend options [_get_configure $path $option]
        }
    } else {
        set options [_get_configure $path {}]
    }

    set result [list]
    foreach list $options {
        if {[llength $list] < 5} { continue }
        lappend result [lindex $list 0] [lindex $list end]
    }
    return $result
}

proc Widget::exists { path } {
    variable _class
    return [info exists _class($path)]
}
# ----------------------------------------------------------------------------
#  utils.tcl -- part of Unifix BWidget Toolkit
# ----------------------------------------------------------------------------

namespace eval BWidget {
    variable _top
    variable _gstack {}
    variable _fstack {}
    proc use {} {}
}

proc BWidget::get3dcolor { path bgcolor } {
    foreach val [winfo rgb $path $bgcolor] {
        lappend dark [expr {60*$val/100}]
        set tmp1 [expr {14*$val/10}]
        if { $tmp1 > 65535 } {
            set tmp1 65535
        }
        set tmp2 [expr {(65535+$val)/2}]
        lappend light [expr {($tmp1 > $tmp2) ? $tmp1:$tmp2}]
    }
    return [list [eval format "#%04x%04x%04x" $dark] [eval format "#%04x%04x%04x" $light]]
}
# ----------------------------------------------------------------------------
#  panedw.tcl -- part of Unifix BWidget Toolkit
# ----------------------------------------------------------------------------

namespace eval PanedWindow {
    Widget::define PanedWindow panedw

    namespace eval Pane {
        Widget::declare PanedWindow::Pane {
            {-minsize Int 0 0 "%d >= 0"}
            {-weight  Int 1 0 "%d >= 0"}
        }
    }

    Widget::declare PanedWindow {
        {-side       Enum       top   1 {top left bottom right}}
        {-width      Int        10    1 "%d >=3"}
        {-pad        Int        4     1 "%d >= 0"}
        {-background TkResource ""    0 frame}
        {-bg         Synonym    -background}
        {-activator  Enum       ""    1 {line button}}
	{-weights    Enum       extra 1 {extra available}}
    }

    variable _panedw
}

proc PanedWindow::create { path args } {
    variable _panedw

    Widget::init PanedWindow $path $args

    frame $path -background [Widget::cget $path -background] -class PanedWindow
    set _panedw($path,nbpanes) 0
    set _panedw($path,weights) ""
    set _panedw($path,configuredone) 0

    set activator [Widget::getoption $path -activator]
    if {[string equal $activator ""]} {
        if { $::tcl_platform(platform) != "windows" } {
            Widget::setMegawidgetOption $path -activator button
        } else {
            Widget::setMegawidgetOption $path -activator line
        }
    }
    if {[string equal [Widget::getoption $path -activator] "line"]} {
        Widget::setMegawidgetOption $path -width 3
    }
    
    bind $path <Configure> [list PanedWindow::_realize $path %w %h]
    bind $path <Destroy>   [list PanedWindow::_destroy $path]

    return [Widget::create PanedWindow $path]
}

proc PanedWindow::configure { path args } {
    variable _panedw

    set res [Widget::configure $path $args]

    if { [Widget::hasChanged $path -background bg] && $_panedw($path,nbpanes) > 0 } {
        $path:cmd configure -background $bg
        $path.f0 configure -background $bg
        for {set i 1} {$i < $_panedw($path,nbpanes)} {incr i} {
            set frame $path.sash$i
            $frame configure -background $bg
            $frame.sep configure -background $bg
            $frame.but configure -background $bg
            $path.f$i configure -background $bg
            $path.f$i.frame configure -background $bg
        }
    }
    return $res
}

proc PanedWindow::cget { path option } {
    return [Widget::cget $path $option]
}

proc PanedWindow::add { path args } {
    variable _panedw

    set num $_panedw($path,nbpanes)
    Widget::init PanedWindow::Pane $path.f$num $args
    set bg [Widget::getoption $path -background]

    set wbut   [Widget::getoption $path -width]
    set pad    [Widget::getoption $path -pad]
    set width  [expr {$wbut+2*$pad}]
    set side   [Widget::getoption $path -side]
    set weight [Widget::getoption $path.f$num -weight]
    lappend _panedw($path,weights) $weight

    if { $num > 0 } {
        set frame [frame $path.sash$num -relief flat -bd 0 \
                       -highlightthickness 0 -width $width -height $width -bg $bg]
        set sep [frame $frame.sep -bd 5 -relief raised \
                     -highlightthickness 0 -bg $bg]
        set but [frame $frame.but -bd 1 -relief raised \
                     -highlightthickness 0 -bg $bg -width $wbut -height $wbut]
	set sepsize 2

        set activator [Widget::getoption $path -activator]
	if {$activator == "button"} {
	    set activator $but
	    set placeButton 1
	} else {
	    set activator $sep
	    $sep configure -bd 1
	    set placeButton 0
	}
        if {[string equal $side "top"] || [string equal $side "bottom"]} {
            place $sep -relx 0.5 -y 0 -width $sepsize -relheight 1.0 -anchor n
	    if { $placeButton } {
		if {[string equal $side "top"]} {
		    place $but -relx 0.5 -y [expr {6+$wbut/2}] -anchor c
		} else {
		    place $but -relx 0.5 -rely 1.0 -y [expr {-6-$wbut/2}] \
			    -anchor c
		}
	    }
            $activator configure -cursor sb_h_double_arrow 
            grid $frame -column [expr {2*$num-1}] -row 0 -sticky ns
            grid columnconfigure $path [expr {2*$num-1}] -weight 0
        } else {
            place $sep -x 0 -rely 0.5 -height $sepsize -relwidth 1.0 -anchor w
	    if { $placeButton } {
		if {[string equal $side "left"]} {
		    place $but -rely 0.5 -x [expr {6+$wbut/2}] -anchor c
		} else {
		    place $but -rely 0.5 -relx 1.0 -x [expr {-6-$wbut/2}] \
			    -anchor c
		}
	    }
            $activator configure -cursor sb_v_double_arrow 
            grid $frame -row [expr {2*$num-1}] -column 0 -sticky ew
            grid rowconfigure $path [expr {2*$num-1}] -weight 0
        }
        bind $activator <ButtonPress-1> \
	    [list PanedWindow::_beg_move_sash $path $num %X %Y]
    } else {
        if { [string equal $side "top"] || \
		[string equal $side "bottom"] } {
            grid rowconfigure $path 0 -weight 1
        } else {
            grid columnconfigure $path 0 -weight 1
        }
    }

    set pane [frame $path.f$num -bd 0 -relief flat \
	    -highlightthickness 0 -bg $bg]
    set user [frame $path.f$num.frame  -bd 0 -relief flat \
	    -highlightthickness 0 -bg $bg]
    if { [string equal $side "top"] || [string equal $side "bottom"] } {
        grid $pane -column [expr {2*$num}] -row 0 -sticky nsew
        grid columnconfigure $path [expr {2*$num}] -weight $weight
    } else {
        grid $pane -row [expr {2*$num}] -column 0 -sticky nsew
        grid rowconfigure $path [expr {2*$num}] -weight $weight
    }
    pack $user -fill both -expand yes
    incr _panedw($path,nbpanes)
    if {$_panedw($path,configuredone)} {
	_realize $path [winfo width $path] [winfo height $path]
    }

    return $user
}

proc PanedWindow::getframe { path index } {
    if { [winfo exists $path.f$index.frame] } {
        return $path.f$index.frame
    }
}
    
proc PanedWindow::_beg_move_sash { path num x y } {
    variable _panedw

    set fprev $path.f[expr {$num-1}]
    set fnext $path.f$num
    set wsash [expr {[Widget::getoption $path -width] + 2*[Widget::getoption $path -pad]}]

    $path.sash$num.but configure -relief sunken
    set top  [toplevel $path.sash -borderwidth 1 -relief raised]

    set minszg [Widget::getoption $fprev -minsize]
    set minszd [Widget::getoption $fnext -minsize]
    set side   [Widget::getoption $path -side]

    if { [string equal $side "top"] || [string equal $side "bottom"] } {
        $top configure -cursor sb_h_double_arrow
        set h    [winfo height $path]
        set yr   [winfo rooty $path.sash$num]
        set xmin [expr {$wsash/2+[winfo rootx $fprev]+$minszg}]
        set xmax [expr {-$wsash/2-1+[winfo rootx $fnext]+[winfo width $fnext]-$minszd}]
        wm overrideredirect $top 1
        wm geom $top "2x${h}+$x+$yr"

        update idletasks
        grab set $top
        bind $top <ButtonRelease-1> [list PanedWindow::_end_move_sash $path $top $num $xmin $xmax %X rootx width]
        bind $top <Motion>          [list PanedWindow::_move_sash $top $xmin $xmax %X +%%d+$yr]
        _move_sash $top $xmin $xmax $x "+%d+$yr"
    } else {
        $top configure -cursor sb_v_double_arrow
        set w    [winfo width $path]
        set xr   [winfo rootx $path.sash$num]
        set ymin [expr {$wsash/2+[winfo rooty $fprev]+$minszg}]
        set ymax [expr {-$wsash/2-1+[winfo rooty $fnext]+[winfo height $fnext]-$minszd}]
        wm overrideredirect $top 1
        wm geom $top "${w}x2+$xr+$y"

        update idletasks
        grab set $top
        bind $top <ButtonRelease-1> [list PanedWindow::_end_move_sash \
		$path $top $num $ymin $ymax %Y rooty height]
        bind $top <Motion>          [list PanedWindow::_move_sash \
		$top $ymin $ymax %Y +$xr+%%d]
        _move_sash $top $ymin $ymax $y "+$xr+%d"
    }
}

proc PanedWindow::_move_sash { top min max v form } {

    if { $v < $min } {
	set v $min
    } elseif { $v > $max } {
	set v $max
    }
    wm geom $top [format $form $v]
}

proc PanedWindow::_end_move_sash { path top num min max v rootv size } {
    variable _panedw

    destroy $top
    if { $v < $min } {
	set v $min
    } elseif { $v > $max } {
	set v $max
    }
    set fprev $path.f[expr {$num-1}]
    set fnext $path.f$num

    $path.sash$num.but configure -relief raised

    set wsash [expr {[Widget::getoption $path -width] + 2*[Widget::getoption $path -pad]}]
    set dv    [expr {$v-[winfo $rootv $path.sash$num]-$wsash/2}]
    set w1    [winfo $size $fprev]
    set w2    [winfo $size $fnext]

    for {set i 0} {$i < $_panedw($path,nbpanes)} {incr i} {
        if { $i == $num-1} {
            $fprev configure -$size [expr {[winfo $size $fprev]+$dv}]
        } elseif { $i == $num } {
            $fnext configure -$size [expr {[winfo $size $fnext]-$dv}]
        } else {
            $path.f$i configure -$size [winfo $size $path.f$i]
        }
    }
}

proc PanedWindow::_realize { path width height } {
    variable _panedw

    set x    0
    set y    0
    set hc   [winfo reqheight $path]
    set hmax 0
    for {set i 0} {$i < $_panedw($path,nbpanes)} {incr i} {
        $path.f$i configure \
            -width  [winfo reqwidth  $path.f$i.frame] \
            -height [winfo reqheight $path.f$i.frame]
        place $path.f$i.frame -x 0 -y 0 -relwidth 1 -relheight 1
    }

    bind $path <Configure> {}

    _apply_weights $path
    set _panedw($path,configuredone) 1
    return
}

proc PanedWindow::_apply_weights { path } {
    variable _panedw

    set weights [Widget::getoption $path -weights]
    if {[string equal $weights "extra"]} {
	return
    }

    set side   [Widget::getoption $path -side]
    if {[string equal $side "top"] || [string equal $side "bottom"] } {
	set size width
    } else {
	set size height
    }
    set wsash [expr {[Widget::getoption $path -width] + 2*[Widget::getoption $path -pad]}]
    set rs [winfo $size $path]
    set s [expr {$rs - ($_panedw($path,nbpanes) - 1) * $wsash}]
    
    set tw 0.0
    foreach w $_panedw($path,weights) { 
	set tw [expr {$tw + $w}]
    }

    for {set i 0} {$i < $_panedw($path,nbpanes)} {incr i} {
	set rw [lindex $_panedw($path,weights) $i]
	set ps [expr {int($rw / $tw * $s)}]
	$path.f$i configure -$size $ps
    }    
    return
}

proc PanedWindow::_destroy { path } {
    variable _panedw

    for {set i 0} {$i < $_panedw($path,nbpanes)} {incr i} {
        Widget::destroy $path.f$i
    }
    unset _panedw($path,nbpanes)
    Widget::destroy $path
}
# ------------------------------------------------------------------------------
#  arrow.tcl -- part of Unifix BWidget Toolkit
# ------------------------------------------------------------------------------

namespace eval ArrowButton {
    Widget::define ArrowButton arrow

    Widget::tkinclude ArrowButton button .c \
	    include [list \
		-borderwidth -bd \
		-relief -highlightbackground \
		-highlightcolor -highlightthickness -takefocus]

    Widget::declare ArrowButton [list \
	    [list -type		Enum button 0 [list arrow button]] \
	    [list -dir		Enum top    0 [list top bottom left right]] \
	    [list -width	Int	15	0	"%d >= 0"] \
	    [list -height	Int	15	0	"%d >= 0"] \
	    [list -ipadx	Int	0	0	"%d >= 0"] \
	    [list -ipady	Int	0	0	"%d >= 0"] \
	    [list -clean	Int	2	0	"%d >= 0 && %d <= 2"] \
	    [list -activeforeground	TkResource	""	0 button] \
	    [list -activebackground	TkResource	""	0 button] \
	    [list -disabledforeground 	TkResource	""	0 button] \
	    [list -foreground		TkResource	""	0 button] \
	    [list -background		TkResource	""	0 button] \
	    [list -state		TkResource	""	0 button] \
	    [list -troughcolor		TkResource	""	0 scrollbar] \
	    [list -arrowbd	Int	1	0	"%d >= 0 && %d <= 2"] \
	    [list -arrowrelief	Enum	raised	0	[list raised sunken]] \
	    [list -command		String	""	0] \
	    [list -armcommand		String	""	0] \
	    [list -disarmcommand	String	""	0] \
	    [list -repeatdelay		Int	0	0	"%d >= 0"] \
	    [list -repeatinterval	Int	0	0	"%d >= 0"] \
	    [list -fg	Synonym	-foreground] \
	    [list -bg	Synonym	-background] \
	    ]

    bind BwArrowButtonC <Enter>           {ArrowButton::_enter %W}
    bind BwArrowButtonC <Leave>           {ArrowButton::_leave %W}
    bind BwArrowButtonC <ButtonPress-1>   {ArrowButton::_press %W}
    bind BwArrowButtonC <ButtonRelease-1> {ArrowButton::_release %W}
    bind BwArrowButtonC <Key-space>       {ArrowButton::invoke %W; break}
    bind BwArrowButtonC <Return>          {ArrowButton::invoke %W; break}
    bind BwArrowButton <Configure>       {ArrowButton::_redraw_whole %W %w %h}
    bind BwArrowButton <Destroy>         {ArrowButton::_destroy %W}

    variable _grab
    variable _moved

    array set _grab {current "" pressed "" oldstate "normal" oldrelief ""}
}

proc ArrowButton::create { path args } {
    # Initialize configuration mappings and parse arguments
    array set submaps [list ArrowButton [list ] .c [list ]]
    array set submaps [Widget::parseArgs ArrowButton $args]

    # Create the class frame (so we can do the option db queries)
    frame $path -class ArrowButton -borderwidth 0 -highlightthickness 0 
    Widget::initFromODB ArrowButton $path $submaps(ArrowButton)

    # Create the canvas with the initial options
    eval [list canvas $path.c] $submaps(.c)

    # Compute the width and height of the canvas from the width/height
    # of the ArrowButton and the borderwidth/hightlightthickness.
    set w   [Widget::getMegawidgetOption $path -width]
    set h   [Widget::getMegawidgetOption $path -height]
    set bd  [Widget::cget $path -borderwidth]
    set ht  [Widget::cget $path -highlightthickness]
    set pad [expr {2*($bd+$ht)}]

    $path.c configure -width [expr {$w-$pad}] -height [expr {$h-$pad}]
    bindtags $path [list $path BwArrowButton [winfo toplevel $path] all]
    bindtags $path.c [list $path.c BwArrowButtonC [winfo toplevel $path.c] all]
    pack $path.c -expand yes -fill both

    set ::ArrowButton::_moved($path) 0

    return [Widget::create ArrowButton $path]
}

proc ArrowButton::configure { path args } {
    set res [Widget::configure $path $args]

    set ch1 [expr {[Widget::hasChanged $path -width  w] |
                   [Widget::hasChanged $path -height h] |
                   [Widget::hasChanged $path -borderwidth bd] |
                   [Widget::hasChanged $path -highlightthickness ht]}]
    set ch2 [expr {[Widget::hasChanged $path -type    val] |
                   [Widget::hasChanged $path -ipadx   val] |
                   [Widget::hasChanged $path -ipady   val] |
                   [Widget::hasChanged $path -arrowbd val] |
                   [Widget::hasChanged $path -clean   val] |
                   [Widget::hasChanged $path -dir     val]}]

    if { $ch1 } {
        set pad [expr {2*($bd+$ht)}]
        $path.c configure \
            -width [expr {$w-$pad}] -height [expr {$h-$pad}] \
            -borderwidth $bd -highlightthickness $ht
	set ch2 1
    }
    if { $ch2 } {
        _redraw_whole $path [winfo width $path] [winfo height $path]
    } else {
        _redraw_relief $path
        _redraw_state $path
    }

    return $res
}

proc ArrowButton::cget { path option } {
    return [Widget::cget $path $option]
}

proc ArrowButton::invoke { path } {
    if { ![string equal [winfo class $path] "ArrowButton"] } {
	set path [winfo parent $path]
    }
    if { ![string equal [Widget::getoption $path -state] "disabled"] } {
        set oldstate [Widget::getoption $path -state]
        if { [string equal [Widget::getoption $path -type] "button"] } {
            set oldrelief [Widget::getoption $path -relief]
            configure $path -state active -relief sunken
        } else {
            set oldrelief [Widget::getoption $path -arrowrelief]
            configure $path -state active -arrowrelief sunken
        }
	update idletasks
        if {[llength [set cmd [Widget::getoption $path -armcommand]]]} {
            uplevel \#0 $cmd
        }
	after 10
        if { [string equal [Widget::getoption $path -type] "button"] } {
            configure $path -state $oldstate -relief $oldrelief
        } else {
            configure $path -state $oldstate -arrowrelief $oldrelief
        }
        if {[llength [set cmd [Widget::getoption $path -disarmcommand]]]} {
            uplevel \#0 $cmd
        }
        if {[llength [set cmd [Widget::getoption $path -command]]]} {
            uplevel \#0 $cmd
        }
    }
}

proc ArrowButton::_redraw { path width height } {
    variable _moved

    set _moved($path) 0
    set type  [Widget::getoption $path -type]
    set dir   [Widget::getoption $path -dir]
    set bd    [expr {[$path.c cget -borderwidth] + [$path.c cget -highlightthickness] + 1}]
    set clean [Widget::getoption $path -clean]
    if { [string equal $type "arrow"] } {
        if { [set id [$path.c find withtag rect]] == "" } {
            $path.c create rectangle $bd $bd [expr {$width-$bd-1}] [expr {$height-$bd-1}] -tags rect
        } else {
            $path.c coords $id $bd $bd [expr {$width-$bd-1}] [expr {$height-$bd-1}]
        }
        $path.c lower rect
        set arrbd [Widget::getoption $path -arrowbd]
        set bd    [expr {$bd+$arrbd-1}]
    } else {
        $path.c delete rect
    }
    # w and h are max width and max height of arrow
    set w [expr {$width  - 2*([Widget::getoption $path -ipadx]+$bd)}]
    set h [expr {$height - 2*([Widget::getoption $path -ipady]+$bd)}]

    if { $w < 2 } {set w 2}
    if { $h < 2 } {set h 2}

    if { $clean > 0 } {
        # arrange for base to be odd
        if { [string equal $dir "top"] || [string equal $dir "bottom"] } {
            if { !($w % 2) } {
                incr w -1
            }
            if { $clean == 2 } {
                # arrange for h = (w+1)/2
                set h2 [expr {($w+1)/2}]
                if { $h2 > $h } {
                    set w [expr {2*$h-1}]
                } else {
                    set h $h2
                }
            }
        } else {
            if { !($h % 2) } {
                incr h -1
            }
            if { $clean == 2 } {
                # arrange for w = (h+1)/2
                set w2 [expr {($h+1)/2}]
                if { $w2 > $w } {
                    set h [expr {2*$w-1}]
                } else {
                    set w $w2
                }
            }
        }
    }

    set x0 [expr {($width-$w)/2}]
    set y0 [expr {($height-$h)/2}]
    set x1 [expr {$x0+$w-1}]
    set y1 [expr {$y0+$h-1}]

    switch $dir {
        top {
            set xd [expr {($x0+$x1)/2}]
            if { [set id [$path.c find withtag poly]] == "" } {
                $path.c create polygon $x0 $y1 $x1 $y1 $xd $y0 -tags poly
            } else {
                $path.c coords $id $x0 $y1 $x1 $y1 $xd $y0
            }
            if { [string equal $type "arrow"] } {
                if { [set id [$path.c find withtag bot]] == "" } {
                    $path.c create line $x0 $y1 $x1 $y1 $xd $y0 -tags bot
                } else {
                    $path.c coords $id $x0 $y1 $x1 $y1 $xd $y0
                }
                if { [set id [$path.c find withtag top]] == "" } {
                    $path.c create line $x0 $y1 $xd $y0 -tags top
                } else {
                    $path.c coords $id $x0 $y1 $xd $y0
                }
                $path.c itemconfigure top -width $arrbd
                $path.c itemconfigure bot -width $arrbd
            } else {
                $path.c delete top
                $path.c delete bot
            }
        }
        bottom {
            set xd [expr {($x0+$x1)/2}]
            if { [set id [$path.c find withtag poly]] == "" } {
                $path.c create polygon $x1 $y0 $x0 $y0 $xd $y1 -tags poly
            } else {
                $path.c coords $id $x1 $y0 $x0 $y0 $xd $y1
            }
            if { [string equal $type "arrow"] } {
                if { [set id [$path.c find withtag top]] == "" } {
                    $path.c create line $x1 $y0 $x0 $y0 $xd $y1 -tags top
                } else {
                    $path.c coords $id $x1 $y0 $x0 $y0 $xd $y1
                }
                if { [set id [$path.c find withtag bot]] == "" } {
                    $path.c create line $x1 $y0 $xd $y1 -tags bot
                } else {
                    $path.c coords $id $x1 $y0 $xd $y1
                }
                $path.c itemconfigure top -width $arrbd
                $path.c itemconfigure bot -width $arrbd
            } else {
                $path.c delete top
                $path.c delete bot
            }
        }
        left {
            set yd [expr {($y0+$y1)/2}]
            if { [set id [$path.c find withtag poly]] == "" } {
                $path.c create polygon $x1 $y0 $x1 $y1 $x0 $yd -tags poly
            } else {
                $path.c coords $id $x1 $y0 $x1 $y1 $x0 $yd
            }
            if { [string equal $type "arrow"] } {
                if { [set id [$path.c find withtag bot]] == "" } {
                    $path.c create line $x1 $y0 $x1 $y1 $x0 $yd -tags bot
                } else {
                    $path.c coords $id $x1 $y0 $x1 $y1 $x0 $yd
                }
                if { [set id [$path.c find withtag top]] == "" } {
                    $path.c create line $x1 $y0 $x0 $yd -tags top
                } else {
                    $path.c coords $id $x1 $y0 $x0 $yd
                }
                $path.c itemconfigure top -width $arrbd
                $path.c itemconfigure bot -width $arrbd
            } else {
                $path.c delete top
                $path.c delete bot
            }
        }
        right {
            set yd [expr {($y0+$y1)/2}]
            if { [set id [$path.c find withtag poly]] == "" } {
                $path.c create polygon $x0 $y1 $x0 $y0 $x1 $yd -tags poly
            } else {
                $path.c coords $id $x0 $y1 $x0 $y0 $x1 $yd
            }
            if { [string equal $type "arrow"] } {
                if { [set id [$path.c find withtag top]] == "" } {
                    $path.c create line $x0 $y1 $x0 $y0 $x1 $yd -tags top
                } else {
                    $path.c coords $id $x0 $y1 $x0 $y0 $x1 $yd
                }
                if { [set id [$path.c find withtag bot]] == "" } {
                    $path.c create line $x0 $y1 $x1 $yd -tags bot
                } else {
                    $path.c coords $id $x0 $y1 $x1 $yd
                }
                $path.c itemconfigure top -width $arrbd
                $path.c itemconfigure bot -width $arrbd
            } else {
                $path.c delete top
                $path.c delete bot
            }
        }
    }
}

proc ArrowButton::_redraw_state { path } {
    set state [Widget::getoption $path -state]
    if { [string equal [Widget::getoption $path -type] "button"] } {
        switch $state {
            normal   {set bg -background;       set fg -foreground}
            active   {set bg -activebackground; set fg -activeforeground}
            disabled {set bg -background;       set fg -disabledforeground}
        }
        set fg [Widget::getoption $path $fg]
        $path.c configure -background [Widget::getoption $path $bg]
        $path.c itemconfigure poly -fill $fg -outline $fg
    } else {
        switch $state {
            normal   {set stipple "";     set bg [Widget::getoption $path -background] }
            active   {set stipple "";     set bg [Widget::getoption $path -activebackground] }
            disabled {set stipple gray50; set bg black }
        }
        set thrc [Widget::getoption $path -troughcolor]
        $path.c configure -background [Widget::getoption $path -background]
        $path.c itemconfigure rect -fill $thrc -outline $thrc
        $path.c itemconfigure poly -fill $bg   -outline $bg -stipple $stipple
    }
}

proc ArrowButton::_redraw_relief { path } {
    variable _moved

    if { [string equal [Widget::getoption $path -type] "button"] } {
        if { [string equal [Widget::getoption $path -relief] "sunken"] } {
            if { !$_moved($path) } {
                $path.c move poly 1 1
                set _moved($path) 1
            }
        } else {
            if { $_moved($path) } {
                $path.c move poly -1 -1
                set _moved($path) 0
            }
        }
    } else {
        set col3d [BWidget::get3dcolor $path [Widget::getoption $path -background]]
        switch [Widget::getoption $path -arrowrelief] {
            raised {set top [lindex $col3d 1]; set bot [lindex $col3d 0]}
            sunken {set top [lindex $col3d 0]; set bot [lindex $col3d 1]}
        }
        $path.c itemconfigure top -fill $top
        $path.c itemconfigure bot -fill $bot
    }
}

proc ArrowButton::_redraw_whole { path width height } {
    _redraw $path $width $height
    _redraw_relief $path
    _redraw_state $path
}

proc ArrowButton::_enter { path } {
    variable _grab
    set path [winfo parent $path]
    set _grab(current) $path
    if { ![string equal [Widget::getoption $path -state] "disabled"] } {
        set _grab(oldstate) [Widget::getoption $path -state]
        configure $path -state active
        if { $_grab(pressed) == $path } {
            if { [string equal [Widget::getoption $path -type] "button"] } {
                set _grab(oldrelief) [Widget::getoption $path -relief]
                configure $path -relief sunken
            } else {
                set _grab(oldrelief) [Widget::getoption $path -arrowrelief]
                configure $path -arrowrelief sunken
            }
        }
    }
}

proc ArrowButton::_leave { path } {
    variable _grab
    set path [winfo parent $path]
    set _grab(current) ""
    if { ![string equal [Widget::getoption $path -state] "disabled"] } {
        configure $path -state $_grab(oldstate)
        if { $_grab(pressed) == $path } {
            if { [string equal [Widget::getoption $path -type] "button"] } {
                configure $path -relief $_grab(oldrelief)
            } else {
                configure $path -arrowrelief $_grab(oldrelief)
            }
        }
    }
}

proc ArrowButton::_press { path } {
    variable _grab
    set path [winfo parent $path]
    if { ![string equal [Widget::getoption $path -state] "disabled"] } {
        set _grab(pressed) $path
            if { [string equal [Widget::getoption $path -type] "button"] } {
            set _grab(oldrelief) [Widget::getoption $path -relief]
            configure $path -relief sunken
        } else {
            set _grab(oldrelief) [Widget::getoption $path -arrowrelief]
            configure $path -arrowrelief sunken
        }
        if {[llength [set cmd [Widget::getoption $path -armcommand]]]} {
            uplevel \#0 $cmd
            if { [set delay [Widget::getoption $path -repeatdelay]]    > 0 ||
                 [set delay [Widget::getoption $path -repeatinterval]] > 0 } {
                after $delay [list ArrowButton::_repeat $path]
            }
        }
    }
}

proc ArrowButton::_release { path } {
    variable _grab
    set path [winfo parent $path]
    if { $_grab(pressed) == $path } {
        set _grab(pressed) ""
            if { [string equal [Widget::getoption $path -type] "button"] } {
            configure $path -relief $_grab(oldrelief)
        } else {
            configure $path -arrowrelief $_grab(oldrelief)
        }
        if {[llength [set cmd [Widget::getoption $path -disarmcommand]]]} {
            uplevel \#0 $cmd
        }
        if { $_grab(current) == $path &&
             ![string equal [Widget::getoption $path -state] "disabled"] &&
             [llength [set cmd [Widget::getoption $path -command]]]} {
            uplevel \#0 $cmd
        }
    }
}

proc ArrowButton::_repeat { path } {
    variable _grab
    if { $_grab(current) == $path && $_grab(pressed) == $path &&
         ![string equal [Widget::getoption $path -state] "disabled"] &&
         [llength [set cmd [Widget::getoption $path -armcommand]]]} {
        uplevel \#0 $cmd
    }
    if { $_grab(pressed) == $path &&
         ([set delay [Widget::getoption $path -repeatinterval]] > 0 ||
          [set delay [Widget::getoption $path -repeatdelay]]    > 0) } {
        after $delay [list ArrowButton::_repeat $path]
    }
}

proc ArrowButton::_destroy { path } {
    variable _moved
    Widget::destroy $path
    unset _moved($path)
}
# ---------------------------------------------------------------------------
#  notebook.tcl -- part of Unifix BWidget Toolkit
# ---------------------------------------------------------------------------

namespace eval NoteBook {
    Widget::define NoteBook notebook ArrowButton

    namespace eval Page {
        Widget::declare NoteBook::Page {
            {-state      Enum       normal 0 {normal disabled}}
            {-createcmd  String     ""     0}
            {-raisecmd   String     ""     0}
            {-leavecmd   String     ""     0}
            {-image      TkResource ""     0 label}
            {-text       String     ""     0}
            {-foreground         String     ""     0}
            {-background         String     ""     0}
            {-activeforeground   String     ""     0}
            {-activebackground   String     ""     0}
            {-disabledforeground String     ""     0}
        }
    }

    Widget::bwinclude NoteBook ArrowButton .c.fg \
	    include {-foreground -background -activeforeground \
		-activebackground -disabledforeground -repeatinterval \
		-repeatdelay -borderwidth} \
	    initialize {-borderwidth 1}
    Widget::bwinclude NoteBook ArrowButton .c.fd \
	    include {-foreground -background -activeforeground \
		-activebackground -disabledforeground -repeatinterval \
		-repeatdelay -borderwidth} \
	    initialize {-borderwidth 1}

    Widget::declare NoteBook {
	{-foreground		TkResource "" 0 button}
        {-background		TkResource "" 0 button}
        {-activebackground	TkResource "" 0 button}
        {-activeforeground	TkResource "" 0 button}
        {-disabledforeground	TkResource "" 0 button}
        {-font			TkResource "" 0 button}
        {-side			Enum       top 0 {top bottom}}
        {-homogeneous		Boolean 0   0}
        {-borderwidth		Int 1   0 "%d >= 1 && %d <= 2"}
 	{-internalborderwidth	Int 10  0 "%d >= 0"}
        {-width			Int 0   0 "%d >= 0"}
        {-height		Int 0   0 "%d >= 0"}

        {-repeatdelay        BwResource ""  0 ArrowButton}
        {-repeatinterval     BwResource ""  0 ArrowButton}

        {-fg                 Synonym -foreground}
        {-bg                 Synonym -background}
        {-bd                 Synonym -borderwidth}
        {-ibd                Synonym -internalborderwidth}

	{-arcradius          Int     2     0 "%d >= 0 && %d <= 8"}
	{-tabbevelsize       Int     0     0 "%d >= 0 && %d <= 8"}
        {-tabpady            Padding {0 6} 0 "%d >= 0"}
    }

    Widget::addmap NoteBook "" .c {-background {}}

    variable _warrow 12

    bind NoteBook <Configure> [list NoteBook::_resize  %W]
    bind NoteBook <Destroy>   [list NoteBook::_destroy %W]
}

proc NoteBook::create { path args } {
    variable $path
    upvar 0  $path data

    Widget::init NoteBook $path $args

    set data(base)     0
    set data(select)   ""
    set data(pages)    {}
    set data(pages)    {}
    set data(cpt)      0
    set data(realized) 0
    set data(wpage)    0

    _compute_height $path

    # Create the canvas
    set w [expr {[Widget::cget $path -width]+4}]
    set h [expr {[Widget::cget $path -height]+$data(hpage)+4}]

    frame $path -class NoteBook -borderwidth 0 -highlightthickness 0 \
	    -relief flat
    eval [list canvas $path.c] [Widget::subcget $path .c] \
	    [list -relief flat -borderwidth 0 -highlightthickness 0 \
	    -width $w -height $h]
    pack $path.c -expand yes -fill both

    # Removing the Canvas global bindings from our canvas as
    # application specific bindings on that tag may interfere with its
    # operation here. [SF item #459033]

    set bindings [bindtags $path.c]
    set pos [lsearch -exact $bindings Canvas]
    if {$pos >= 0} {
	set bindings [lreplace $bindings $pos $pos]
    }
    bindtags $path.c $bindings

    # Create the arrow button
    eval [list ArrowButton::create $path.c.fg] [Widget::subcget $path .c.fg] \
	    [list -highlightthickness 0 -type button -dir left \
	    -armcommand [list NoteBook::_xview $path -1]]

    eval [list ArrowButton::create $path.c.fd] [Widget::subcget $path .c.fd] \
	    [list -highlightthickness 0 -type button -dir right \
	    -armcommand [list NoteBook::_xview $path 1]]

    Widget::create NoteBook $path

    set bg [Widget::cget $path -background]
    foreach {data(dbg) data(lbg)} [BWidget::get3dcolor $path $bg] {break}

    return $path
}

proc NoteBook::configure { path args } {
    variable $path
    upvar 0  $path data

    set res [Widget::configure $path $args]
    set redraw 0
    set opts [list -font -homogeneous -tabpady]
    foreach {cf ch cp} [eval Widget::hasChangedX $path $opts] {break}
    if {$cf || $ch || $cp} {
        if { $cf || $cp } {
            _compute_height $path
        }
        _compute_width $path
        set redraw 1
    }
    set chibd [Widget::hasChanged $path -internalborderwidth ibd]
    set chbg  [Widget::hasChanged $path -background bg]
    if {$chibd || $chbg} {
        foreach page $data(pages) {
            $path.f$page configure \
                -borderwidth $ibd -background $bg
        }
    }

    if {$chbg} {
        set col [BWidget::get3dcolor $path $bg]
        set data(dbg)  [lindex $col 0]
        set data(lbg)  [lindex $col 1]
        set redraw 1
    }
    if { [Widget::hasChanged $path -foreground  fg] ||
         [Widget::hasChanged $path -borderwidth bd] ||
	 [Widget::hasChanged $path -arcradius radius] ||
         [Widget::hasChanged $path -tabbevelsize bevel] ||
         [Widget::hasChanged $path -side side] } {
        set redraw 1
    }
    set wc [Widget::hasChanged $path -width  w]
    set hc [Widget::hasChanged $path -height h]
    if { $wc || $hc } {
        $path.c configure \
		-width  [expr {$w + 4}] \
		-height [expr {$h + $data(hpage) + 4}]
    }
    if { $redraw } {
        _redraw $path
    }

    return $res
}

proc NoteBook::cget { path option } {
    return [Widget::cget $path $option]
}

proc NoteBook::compute_size { path } {
    variable $path
    upvar 0  $path data

    set wmax 0
    set hmax 0
    update idletasks
    foreach page $data(pages) {
        set w    [winfo reqwidth  $path.f$page]
        set h    [winfo reqheight $path.f$page]
        set wmax [expr {$w>$wmax ? $w : $wmax}]
        set hmax [expr {$h>$hmax ? $h : $hmax}]
    }
    configure $path -width $wmax -height $hmax
    # Sven... well ok so this is called twice in some cases...
    NoteBook::_redraw $path
    # Sven end
}

proc NoteBook::insert { path index page args } {
    variable $path
    upvar 0  $path data

    if { [lsearch -exact $data(pages) $page] != -1 } {
        return -code error "page \"$page\" already exists"
    }

    set f $path.f$page
    Widget::init NoteBook::Page $f $args

    set data(pages) [linsert $data(pages) $index $page]
    # If the page doesn't exist, create it; if it does reset its bg and ibd
    if { ![winfo exists $f] } {
        frame $f \
	    -relief      flat \
	    -background  [Widget::cget $path -background] \
	    -borderwidth [Widget::cget $path -internalborderwidth]
        set data($page,realized) 0
    } else {
	$f configure \
	    -background  [Widget::cget $path -background] \
	    -borderwidth [Widget::cget $path -internalborderwidth]
    }
    _compute_height $path
    _compute_width  $path
    _draw_page $path $page 1
    _redraw $path

    return $f
}

proc NoteBook::delete { path page {destroyframe 1} } {
    variable $path
    upvar 0  $path data

    set pos [_test_page $path $page]
    set data(pages) [lreplace $data(pages) $pos $pos]
    _compute_width $path
    $path.c delete p:$page
    if { $data(select) == $page } {
        set data(select) ""
    }
    if { $pos < $data(base) } {
        incr data(base) -1
    }
    if { $destroyframe } {
        destroy $path.f$page
        unset data($page,width) data($page,realized)
    }
    _redraw $path
}

proc NoteBook::itemconfigure { path page args } {
    _test_page $path $page
    set res [_itemconfigure $path $page $args]
    _redraw $path

    return $res
}

proc NoteBook::itemcget { path page option } {
    _test_page $path $page
    return [Widget::cget $path.f$page $option]
}

proc NoteBook::bindtabs { path event script } {
    if { $script != "" } {
	append script " \[NoteBook::_get_page_name [list $path] current 1\]"
        $path.c bind "page" $event $script
    } else {
        $path.c bind "page" $event {}
    }
}

proc NoteBook::move { path page index } {
    variable $path
    upvar 0  $path data

    set pos [_test_page $path $page]
    set data(pages) [linsert [lreplace $data(pages) $pos $pos] $index $page]
    _redraw $path
}

proc NoteBook::raise { path {page ""} } {
    variable $path
    upvar 0  $path data

    if { $page != "" } {
        _test_page $path $page
        _select $path $page
    }
    return $data(select)
}

proc NoteBook::see { path page } {
    variable $path
    upvar 0  $path data

    set pos [_test_page $path $page]
    if { $pos < $data(base) } {
        set data(base) $pos
        _redraw $path
    } else {
        set w     [expr {[winfo width $path]-1}]
        set fpage [expr {[_get_x_page $path $pos] + $data($page,width) + 6}]
        set idx   $data(base)
        while { $idx < $pos && $fpage > $w } {
            set fpage [expr {$fpage - $data([lindex $data(pages) $idx],width)}]
            incr idx
        }
        if { $idx != $data(base) } {
            set data(base) $idx
            _redraw $path
        }
    }
}

proc NoteBook::page { path first {last ""} } {
    variable $path
    upvar 0  $path data

    if { $last == "" } {
        return [lindex $data(pages) $first]
    } else {
        return [lrange $data(pages) $first $last]
    }
}

proc NoteBook::pages { path {first ""} {last ""}} {
    variable $path
    upvar 0  $path data

    if { ![string length $first] } {
	return $data(pages)
    }

    if { ![string length $last] } {
        return [lindex $data(pages) $first]
    } else {
        return [lrange $data(pages) $first $last]
    }
}

proc NoteBook::index { path page } {
    variable $path
    upvar 0  $path data

    return [lsearch -exact $data(pages) $page]
}

proc NoteBook::_destroy { path } {
    variable $path
    upvar 0  $path data

    foreach page $data(pages) {
        Widget::destroy $path.f$page
    }
    Widget::destroy $path
    unset data
}

proc NoteBook::getframe { path page } {
    return $path.f$page
}

proc NoteBook::_test_page { path page } {
    variable $path
    upvar 0  $path data

    if { [set pos [lsearch -exact $data(pages) $page]] == -1 } {
        return -code error "page \"$page\" does not exists"
    }
    return $pos
}

proc NoteBook::_getoption { path page option } {
    set value [Widget::cget $path.f$page $option]
    if {![string length $value]} {
        set value [Widget::cget $path $option]
    }
    return $value
}

proc NoteBook::_itemconfigure { path page lres } {
    variable $path
    upvar 0  $path data

    set res [Widget::configure $path.f$page $lres]
    if { [Widget::hasChanged $path.f$page -text foo] } {
        _compute_width $path
    } elseif  { [Widget::hasChanged $path.f$page -image foo] } {
        _compute_height $path
        _compute_width  $path
    }
    if { [Widget::hasChanged $path.f$page -state state] &&
         $state == "disabled" && $data(select) == $page } {
        set data(select) ""
    }
    return $res
}

proc NoteBook::_compute_width { path } {
    variable $path
    upvar 0  $path data

    set wmax 0
    set wtot 0
    set hmax $data(hpage)
    set font [Widget::cget $path -font]
    if { ![info exists data(textid)] } {
        set data(textid) [$path.c create text 0 -100 -font $font -anchor nw]
    }
    set id $data(textid)
    $path.c itemconfigure $id -font $font
    foreach page $data(pages) {
        $path.c itemconfigure $id -text [Widget::cget $path.f$page -text]
	# Get the bbox for this text to determine its width, then substract
	# 6 from the width to account for canvas bbox oddness w.r.t. widths of
	# simple text.
	foreach {x1 y1 x2 y2} [$path.c bbox $id] break
	set x2 [expr {$x2 - 6}]
        set wtext [expr {$x2 - $x1 + 20}]
        if { [set img [Widget::cget $path.f$page -image]] != "" } {
            set wtext [expr {$wtext + [image width $img] + 4}]
            set himg  [expr {[image height $img] + 6}]
            if { $himg > $hmax } {
                set hmax $himg
            }
        }
        set  wmax  [expr {$wtext > $wmax ? $wtext : $wmax}]
        incr wtot  $wtext
        set  data($page,width) $wtext
    }
    if { [Widget::cget $path -homogeneous] } {
        foreach page $data(pages) {
            set data($page,width) $wmax
        }
        set wtot [expr {$wmax * [llength $data(pages)]}]
    }
    set data(hpage) $hmax
    set data(wpage) $wtot
}

proc NoteBook::_compute_height { path } {
    variable $path
    upvar 0  $path data

    set font    [Widget::cget $path -font]
    set pady0   [Widget::_get_padding $path -tabpady 0]
    set pady1   [Widget::_get_padding $path -tabpady 1]
    set metrics [font metrics $font -linespace]
    set imgh    0
    set lines   1
    foreach page $data(pages) {
        set img  [Widget::cget $path.f$page -image]
        set text [Widget::cget $path.f$page -text]
        set len [llength [split $text \n]]
        if {$len > $lines} { set lines $len}
        if {$img != ""} {
            set h [image height $img]
            if {$h > $imgh} { set imgh $h }
        }
    }
    set height [expr {$metrics * $lines}]
    if {$imgh > $height} { set height $imgh }
    set data(hpage) [expr {$height + $pady0 + $pady1}]
}

proc NoteBook::_get_x_page { path pos } {
    variable _warrow
    variable $path
    upvar 0  $path data

    set base $data(base)
    # notebook tabs start flush with the left side of the notebook
    set x 0
    if { $pos < $base } {
        foreach page [lrange $data(pages) $pos [expr {$base-1}]] {
            incr x [expr {-$data($page,width)}]
        }
    } elseif { $pos > $base } {
        foreach page [lrange $data(pages) $base [expr {$pos-1}]] {
            incr x $data($page,width)
        }
    }
    return $x
}

proc NoteBook::_xview { path inc } {
    variable $path
    upvar 0  $path data

    if { $inc == -1 } {
        set base [expr {$data(base)-1}]
        set dx $data([lindex $data(pages) $base],width)
    } else {
        set dx [expr {-$data([lindex $data(pages) $data(base)],width)}]
        set base [expr {$data(base)+1}]
    }

    if { $base >= 0 && $base < [llength $data(pages)] } {
        set data(base) $base
        $path.c move page $dx 0
        _draw_area   $path
        _draw_arrows $path
    }
}

proc NoteBook::_highlight { type path page } {
    variable $path
    upvar 0  $path data

    if { [string equal [Widget::cget $path.f$page -state] "disabled"] } {
        return
    }

    switch -- $type {
        on {
            $path.c itemconfigure "$page:poly" \
		    -fill [_getoption $path $page -activebackground]
            $path.c itemconfigure "$page:text" \
		    -fill [_getoption $path $page -activeforeground]
        }
        off {
            $path.c itemconfigure "$page:poly" \
		    -fill [_getoption $path $page -background]
            $path.c itemconfigure "$page:text" \
		    -fill [_getoption $path $page -foreground]
        }
    }
}

proc NoteBook::_select { path page } {
    variable $path
    upvar 0  $path data

    if {![string equal [Widget::cget $path.f$page -state] "normal"]} { return }

    set oldsel $data(select)

    if {[string equal $page $oldsel]} { return }

    if { ![string equal $oldsel ""] } {
	set cmd [Widget::cget $path.f$oldsel -leavecmd]
	if { ![string equal $cmd ""] } {
	    set code [catch {uplevel \#0 $cmd} res]
	    if { $code == 1 || $res == 0 } {
		return -code $code $res
	    }
	}
	set data(select) ""
	_draw_page $path $oldsel 0
    }

    set data(select) $page
    if { ![string equal $page ""] } {
	if { !$data($page,realized) } {
	    set data($page,realized) 1
	    set cmd [Widget::cget $path.f$page -createcmd]
	    if { ![string equal $cmd ""] } {
		uplevel \#0 $cmd
	    }
	}
	set cmd [Widget::cget $path.f$page -raisecmd]
	if { ![string equal $cmd ""] } {
	    uplevel \#0 $cmd
	}
	_draw_page $path $page 0
    }

    _draw_area $path
}

proc NoteBook::_redraw { path } {
    variable $path
    upvar 0  $path data

    if { !$data(realized) } { return }

    _compute_height $path

    foreach page $data(pages) {
        _draw_page $path $page 0
    }
    _draw_area   $path
    _draw_arrows $path
}

proc NoteBook::_draw_page { path page create } {
    variable $path
    upvar 0  $path data

    # --- calcul des coordonnees et des couleurs de l'onglet ------------------
    set pos [lsearch -exact $data(pages) $page]
    set bg  [_getoption $path $page -background]

    # lookup the tab colors
    set fgt   $data(lbg)
    set fgb   $data(dbg)

    set h   $data(hpage)
    set xd  [_get_x_page $path $pos]
    set xf  [expr {$xd + $data($page,width)}]

    # Set the initial text offsets -- a few pixels down, centered left-to-right
    set textOffsetY [expr [Widget::_get_padding $path -tabpady 0] + 3]
    set textOffsetX 9

    set top		2
    set arcRadius	[Widget::cget $path -arcradius]
    set xBevel		[Widget::cget $path -tabbevelsize]

    if { $data(select) != $page } {
	if { $pos == 0 } {
	    # The leftmost page is a special case -- it is drawn with its
	    # tab a little indented.  To achieve this, we incr xd.  We also
	    # decr textOffsetX, so that the text doesn't move left/right.
	    incr xd 2
	    incr textOffsetX -2
	}
    } else {
	# The selected page's text is raised higher than the others
	incr top -2
    }

    # Precompute some coord values that we use a lot
    set topPlusRadius	[expr {$top + $arcRadius}]
    set rightPlusRadius	[expr {$xf + $arcRadius}]
    set leftPlusRadius	[expr {$xd + $arcRadius}]

    # Sven
    set side [Widget::cget $path -side]
    set tabsOnBottom [string equal $side "bottom"]

    set h1 [expr {[winfo height $path]}]
    set bd [Widget::cget $path -borderwidth]
    if {$bd < 1} { set bd 1 }

    if { $tabsOnBottom } {
	# adjust to keep bottom edge in view
	incr h1 -1
	set top [expr {$top * -1}]
	set topPlusRadius [expr {$topPlusRadius * -1}]
	# Hrm... the canvas has an issue with drawing diagonal segments
	# of lines from the bottom to the top, so we have to draw this line
	# backwards (ie, lt is actually the bottom, drawn from right to left)
        set lt  [list \
		$rightPlusRadius			[expr {$h1-$h-1}] \
		[expr {$rightPlusRadius - $xBevel}]	[expr {$h1 + $topPlusRadius}] \
		[expr {$xf - $xBevel}]			[expr {$h1 + $top}] \
		[expr {$leftPlusRadius + $xBevel}]	[expr {$h1 + $top}] \
		]
        set lb  [list \
		[expr {$leftPlusRadius + $xBevel}]	[expr {$h1 + $top}] \
		[expr {$xd + $xBevel}]			[expr {$h1 + $topPlusRadius}] \
		$xd					[expr {$h1-$h-1}] \
		]
	# Because we have to do this funky reverse order thing, we have to
	# swap the top/bottom colors too.
	set tmp $fgt
	set fgt $fgb
	set fgb $tmp
    } else {
	set lt [list \
		$xd					$h \
		[expr {$xd + $xBevel}]			$topPlusRadius \
		[expr {$leftPlusRadius + $xBevel}]	$top \
		[expr {$xf + 1 - $xBevel}]		$top \
		]
	set lb [list \
		[expr {$xf + 1 - $xBevel}] 		[expr {$top + 1}] \
		[expr {$rightPlusRadius - $xBevel}]	$topPlusRadius \
		$rightPlusRadius			$h \
		]
    }

    set img [Widget::cget $path.f$page -image]

    set ytext $top
    if { $tabsOnBottom } {
	# The "+ 2" below moves the text closer to the bottom of the tab,
	# so it doesn't look so cramped.  I should be able to achieve the
	# same goal by changing the anchor of the text and using this formula:
	# ytext = $top + $h1 - $textOffsetY
	# but that doesn't quite work (I think the linespace from the text
	# gets in the way)
	incr ytext [expr {$h1 - $h + 2}]
    }
    incr ytext $textOffsetY

    set xtext [expr {$xd + $textOffsetX}]
    if { $img != "" } {
	# if there's an image, put it on the left and move the text right
	set ximg $xtext
	incr xtext [expr {[image width $img] + 2}]
    }
	
    if { $data(select) == $page } {
        set bd    [Widget::cget $path -borderwidth]
	if {$bd < 1} { set bd 1 }
        set fg    [_getoption $path $page -foreground]
    } else {
        set bd    1
        if { [Widget::cget $path.f$page -state] == "normal" } {
            set fg [_getoption $path $page -foreground]
        } else {
            set fg [_getoption $path $page -disabledforeground]
        }
    }

    # --- creation ou modification de l'onglet --------------------------------
    # Sven
    if { $create } {
	# Create the tab region
        eval [list $path.c create polygon] [concat $lt $lb] [list \
		-tags		[list page p:$page $page:poly] \
		-outline	$bg \
		-fill		$bg \
		]
        eval [list $path.c create line] $lt [list \
            -tags [list page p:$page $page:top top] -fill $fgt -width $bd]
        eval [list $path.c create line] $lb [list \
            -tags [list page p:$page $page:bot bot] -fill $fgb -width $bd]
        $path.c create text $xtext $ytext 			\
		-text	[Widget::cget $path.f$page -text]	\
		-font	[Widget::cget $path -font]		\
		-fill	$fg					\
		-anchor	nw					\
		-tags	[list page p:$page $page:text]

        $path.c bind p:$page <ButtonPress-1> \
		[list NoteBook::_select $path $page]
        $path.c bind p:$page <Enter> \
		[list NoteBook::_highlight on  $path $page]
        $path.c bind p:$page <Leave> \
		[list NoteBook::_highlight off $path $page]
    } else {
        $path.c coords "$page:text" $xtext $ytext

        $path.c itemconfigure "$page:text" \
            -text [Widget::cget $path.f$page -text] \
            -font [Widget::cget $path -font] \
            -fill $fg
    }
    eval [list $path.c coords "$page:poly"] [concat $lt $lb]
    eval [list $path.c coords "$page:top"]  $lt
    eval [list $path.c coords "$page:bot"]  $lb
    $path.c itemconfigure "$page:poly" -fill $bg  -outline $bg
    $path.c itemconfigure "$page:top"  -fill $fgt -width $bd
    $path.c itemconfigure "$page:bot"  -fill $fgb -width $bd
    
    # Sven end

    if { $img != "" } {
        # Sven
	set id [$path.c find withtag $page:img]
	if { [string equal $id ""] } {
	    set id [$path.c create image $ximg $ytext \
		    -anchor nw    \
		    -tags   [list page p:$page $page:img]]
        }
        $path.c coords $id $ximg $ytext
        $path.c itemconfigure $id -image $img
        # Sven end
    } else {
        $path.c delete $page:img
    }

    if { $data(select) == $page } {
        $path.c raise p:$page
    } elseif { $pos == 0 } {
        if { $data(select) == "" } {
            $path.c raise p:$page
        } else {
            $path.c lower p:$page p:$data(select)
        }
    } else {
        set pred [lindex $data(pages) [expr {$pos-1}]]
        if { $data(select) != $pred || $pos == 1 } {
            $path.c lower p:$page p:$pred
        } else {
            $path.c lower p:$page p:[lindex $data(pages) [expr {$pos-2}]]
        }
    }
}

proc NoteBook::_draw_arrows { path } {
    variable _warrow
    variable $path
    upvar 0  $path data

    set w       [expr {[winfo width $path]-1}]
    set h       [expr {$data(hpage)-1}]
    set nbpages [llength $data(pages)]
    set xl      0
    set xr      [expr {$w-$_warrow+1}]

    set side [Widget::cget $path -side]
    if { [string equal $side "bottom"] } {
        set h1 [expr {[winfo height $path]-1}]
        set bd [Widget::cget $path -borderwidth]
	if {$bd < 1} { set bd 1 }
        set y0 [expr {$h1 - $data(hpage) + $bd}]
    } else {
        set y0 1
    }

    if { $data(base) > 0 } {
        # Sven 
        if { ![llength [$path.c find withtag "leftarrow"]] } {
            $path.c create window $xl $y0 \
                -width  $_warrow            \
                -height $h                  \
                -anchor nw                  \
                -window $path.c.fg            \
                -tags   "leftarrow"
        } else {
            $path.c coords "leftarrow" $xl $y0
            $path.c itemconfigure "leftarrow" -width $_warrow -height $h
        }
        # Sven end
    } else {
        $path.c delete "leftarrow"
    }

    if { $data(base) < $nbpages-1 &&
         $data(wpage) + [_get_x_page $path 0] + 6 > $w } {
        # Sven
        if { ![llength [$path.c find withtag "rightarrow"]] } {
            $path.c create window $xr $y0 \
                -width  $_warrow            \
                -height $h                  \
                -window $path.c.fd            \
                -anchor nw                  \
                -tags   "rightarrow"
        } else {
            $path.c coords "rightarrow" $xr $y0
            $path.c itemconfigure "rightarrow" -width $_warrow -height $h
        }
        # Sven end
    } else {
        $path.c delete "rightarrow"
    }
}

proc NoteBook::_draw_area { path } {
    variable $path
    upvar 0  $path data

    set w   [expr {[winfo width  $path] - 1}]
    set h   [expr {[winfo height $path] - 1}]
    set bd  [Widget::cget $path -borderwidth]
    if {$bd < 1} { set bd 1 }
    set x0  [expr {$bd - 1}]

    set arcRadius [Widget::cget $path -arcradius]

    # Sven
    set side [Widget::cget $path -side]
    if {"$side" == "bottom"} {
        set y0 0
        set y1 [expr {$h - $data(hpage)}]
        set yo $y1
    } else {
        set y0 $data(hpage)
        set y1 $h
        set yo [expr {$h-$y0}]
    }
    # Sven end
    set dbg $data(dbg)
    set sel $data(select)
    if {  $sel == "" } {
        set xd  [expr {$w/2}]
        set xf  $xd
        set lbg $data(dbg)
    } else {
        set xd [_get_x_page $path [lsearch -exact $data(pages) $data(select)]]
        set xf [expr {$xd + $data($sel,width) + $arcRadius + 1}]
        set lbg $data(lbg)
    }

    # Sven
    if { [llength [$path.c find withtag rect]] == 0} {
        $path.c create line $xd $y0 $x0 $y0 $x0 $y1 \
            -tags "rect toprect1" 
        $path.c create line $w $y0 $xf $y0 \
            -tags "rect toprect2"
        $path.c create line 1 $h $w $h $w $y0 \
            -tags "rect botrect"
    }
    if {"$side" == "bottom"} {
        $path.c coords "toprect1" $w $y0 $x0 $y0 $x0 $y1
        $path.c coords "toprect2" $x0 $y1 $xd $y1
        $path.c coords "botrect"  $xf $y1 $w $y1 $w $y0
        $path.c itemconfigure "toprect1" -fill $lbg -width $bd
        $path.c itemconfigure "toprect2" -fill $dbg -width $bd
        $path.c itemconfigure "botrect" -fill $dbg -width $bd
    } else {
        $path.c coords "toprect1" $xd $y0 $x0 $y0 $x0 $y1
        $path.c coords "toprect2" $w $y0 $xf $y0
        $path.c coords "botrect"  $x0 $h $w $h $w $y0
        $path.c itemconfigure "toprect1" -fill $lbg -width $bd
        $path.c itemconfigure "toprect2" -fill $lbg -width $bd
        $path.c itemconfigure "botrect" -fill $dbg -width $bd
    }
    $path.c raise "rect"
    # Sven end

    if { $sel != "" } {
        # Sven
        if { [llength [$path.c find withtag "window"]] == 0 } {
            $path.c create window 2 [expr {$y0+1}] \
                -width  [expr {$w-3}]           \
                -height [expr {$yo-3}]          \
                -anchor nw                      \
                -tags   "window"                \
                -window $path.f$sel
        }
        $path.c coords "window" 2 [expr {$y0+1}]
        $path.c itemconfigure "window"    \
            -width  [expr {$w-3}]           \
            -height [expr {$yo-3}]          \
            -window $path.f$sel
        # Sven end
    } else {
        $path.c delete "window"
    }
}

proc NoteBook::_resize { path } {
    variable $path
    upvar 0  $path data

    if {!$data(realized)} {
	if { [set width  [Widget::cget $path -width]]  == 0 ||
	     [set height [Widget::cget $path -height]] == 0 } {
	    compute_size $path
	}
	set data(realized) 1
    }

    NoteBook::_redraw $path
}

proc NoteBook::_get_page_name { path {item current} {tagindex end-1} } {
    return [string range [lindex [$path.c gettags $item] $tagindex] 2 end]
}
# -----------------------------------------------------------------------------
#  scrollw.tcl -- part of Unifix BWidget Toolkit
# -----------------------------------------------------------------------------

namespace eval ScrolledWindow {
    Widget::define ScrolledWindow scrollw

    Widget::declare ScrolledWindow {
	{-background  TkResource ""   0 button}
	{-scrollbar   Enum	 both 0 {none both vertical horizontal}}
	{-auto	      Enum	 both 0 {none both vertical horizontal}}
	{-sides	      Enum	 se   0 {ne en nw wn se es sw ws}}
	{-size	      Int	 0    1 "%d >= 0"}
	{-ipad	      Int	 1    1 "%d >= 0"}
	{-managed     Boolean	 1    1}
	{-relief      TkResource flat 0 frame}
	{-borderwidth TkResource 0    0 frame}
	{-bg	      Synonym	 -background}
	{-bd	      Synonym	 -borderwidth}
    }

    Widget::addmap ScrolledWindow "" :cmd {-relief {} -borderwidth {}}
}

proc ScrolledWindow::create { path args } {
    Widget::init ScrolledWindow $path $args

    Widget::getVariable $path data

    set bg     [Widget::cget $path -background]
    set sbsize [Widget::cget $path -size]
    set sw     [eval [list frame $path \
			  -relief flat -borderwidth 0 -background $bg \
			  -highlightthickness 0 -takefocus 0] \
		    [Widget::subcget $path :cmd]]

    scrollbar $path.hscroll \
	    -highlightthickness 0 -takefocus 0 \
	    -orient	 horiz	\
	    -relief	 sunken	\
	    -bg	 $bg
    scrollbar $path.vscroll \
	    -highlightthickness 0 -takefocus 0 \
	    -orient	 vert	\
	    -relief	 sunken	\
	    -bg	 $bg

    set data(realized) 0

    _setData $path \
	    [Widget::cget $path -scrollbar] \
	    [Widget::cget $path -auto] \
	    [Widget::cget $path -sides]

    if {[Widget::cget $path -managed]} {
	set data(hsb,packed) $data(hsb,present)
	set data(vsb,packed) $data(vsb,present)
    } else {
	set data(hsb,packed) 0
	set data(vsb,packed) 0
    }
    if {$sbsize} {
	$path.vscroll configure -width $sbsize
	$path.hscroll configure -width $sbsize
    } else {
	set sbsize [$path.vscroll cget -width]
    }
    set data(ipad) [Widget::cget $path -ipad]

    if {$data(hsb,packed)} {
	grid $path.hscroll -column 1 -row $data(hsb,row) \
		-sticky ew -ipady $data(ipad)
    }
    if {$data(vsb,packed)} {
	grid $path.vscroll -column $data(vsb,column) -row 1 \
		-sticky ns -ipadx $data(ipad)
    }

    grid columnconfigure $path 1 -weight 1
    grid rowconfigure	 $path 1 -weight 1

    bind $path <Configure> [list ScrolledWindow::_realize $path]
    bind $path <Destroy>   [list ScrolledWindow::_destroy $path]

    return [Widget::create ScrolledWindow $path]
}

proc ScrolledWindow::getframe { path } {
    return $path
}

proc ScrolledWindow::setwidget { path widget } {
    Widget::getVariable $path data

    if {[info exists data(widget)] && [winfo exists $data(widget)]
	&& ![string equal $data(widget) $widget]} {
	grid remove $data(widget)
	$data(widget) configure -xscrollcommand "" -yscrollcommand ""
    }
    set data(widget) $widget
    grid $widget -in $path -row 1 -column 1 -sticky news

    $path.hscroll configure -command [list $widget xview]
    $path.vscroll configure -command [list $widget yview]
    $widget configure \
	    -xscrollcommand [list ScrolledWindow::_set_hscroll $path] \
	    -yscrollcommand [list ScrolledWindow::_set_vscroll $path]
}

proc ScrolledWindow::configure { path args } {
    Widget::getVariable $path data

    set res [Widget::configure $path $args]
    if { [Widget::hasChanged $path -background bg] } {
	$path configure -background $bg
	catch {$path.hscroll configure -background $bg}
	catch {$path.vscroll configure -background $bg}
    }

    if {[Widget::hasChanged $path -scrollbar scrollbar] | \
	    [Widget::hasChanged $path -auto	 auto]	| \
	    [Widget::hasChanged $path -sides	 sides]} {
	_setData $path $scrollbar $auto $sides
	foreach {vmin vmax} [$path.hscroll get] { break }
	set data(hsb,packed) [expr {$data(hsb,present) && \
		(!$data(hsb,auto) || ($vmin != 0 || $vmax != 1))}]
	foreach {vmin vmax} [$path.vscroll get] { break }
	set data(vsb,packed) [expr {$data(vsb,present) && \
		(!$data(vsb,auto) || ($vmin != 0 || $vmax != 1))}]

	set data(ipad) [Widget::cget $path -ipad]

	if {$data(hsb,packed)} {
	    grid $path.hscroll -column 1 -row $data(hsb,row) \
		-sticky ew -ipady $data(ipad)
	} else {
	    if {![info exists data(hlock)]} {
		set data(hsb,packed) 0
		grid remove $path.hscroll
	    }
	}
	if {$data(vsb,packed)} {
	    grid $path.vscroll -column $data(vsb,column) -row 1 \
		-sticky ns -ipadx $data(ipad)
	} else {
	    if {![info exists data(hlock)]} {
		set data(vsb,packed) 0
		grid remove $path.vscroll
	    }
	}
    }
    return $res
}

proc ScrolledWindow::cget { path option } {
    return [Widget::cget $path $option]
}

proc ScrolledWindow::_set_hscroll { path vmin vmax } {
    Widget::getVariable $path data

    if {$data(realized) && $data(hsb,present)} {
	if {$data(hsb,auto) && ![info exists data(hlock)]} {
	    if {$data(hsb,packed) && $vmin == 0 && $vmax == 1} {
		set data(hsb,packed) 0
		grid remove $path.hscroll
		set data(hlock) 1
		update idletasks
		unset data(hlock)
	    } elseif {!$data(hsb,packed) && ($vmin != 0 || $vmax != 1)} {
		set data(hsb,packed) 1
		grid $path.hscroll -column 1 -row $data(hsb,row) \
			-sticky ew -ipady $data(ipad)
		set data(hlock) 1
		update idletasks
		unset data(hlock)
	    }
	}
	$path.hscroll set $vmin $vmax
    }
}

proc ScrolledWindow::_set_vscroll { path vmin vmax } {
    Widget::getVariable $path data

    if {$data(realized) && $data(vsb,present)} {
	if {$data(vsb,auto) && ![info exists data(vlock)]} {
	    if {$data(vsb,packed) && $vmin == 0 && $vmax == 1} {
		set data(vsb,packed) 0
		grid remove $path.vscroll
		set data(vlock) 1
		update idletasks
		unset data(vlock)
	    } elseif {!$data(vsb,packed) && ($vmin != 0 || $vmax != 1) } {
		set data(vsb,packed) 1
		grid $path.vscroll -column $data(vsb,column) -row 1 \
			-sticky ns -ipadx $data(ipad)
		set data(vlock) 1
		update idletasks
		unset data(vlock)
	    }
	}
	$path.vscroll set $vmin $vmax
    }
}

proc ScrolledWindow::_setData {path scrollbar auto sides} {
    Widget::getVariable $path data

    set sb    [lsearch {none horizontal vertical both} $scrollbar]
    set auto  [lsearch {none horizontal vertical both} $auto]

    set data(hsb,present)  [expr {($sb & 1) != 0}]
    set data(hsb,auto)	   [expr {($auto & 1) != 0}]
    set data(hsb,row)	   [expr {[string match *n* $sides] ? 0 : 2}]

    set data(vsb,present)  [expr {($sb & 2) != 0}]
    set data(vsb,auto)	   [expr {($auto & 2) != 0}]
    set data(vsb,column)   [expr {[string match *w* $sides] ? 0 : 2}]
}

proc ScrolledWindow::_realize { path } {
    Widget::getVariable $path data

    bind $path <Configure> {}
    set data(realized) 1
}

proc ScrolledWindow::_destroy { path } {
    Widget::destroy $path
}

############ end of BWidget code ##############
############   Cobra GUI code    ##############

set qfd	0
set nrc 1	;# line in c_log
set nrh 1	;# line in h_log
set frh 1
set nrm 0	;# nr matches
set nrr 1	;# line in r_log

set noupdate 0

proc show_source {s} {
	global r_log c_log noupdate linenumbers nrc

	$r_log delete 0.0 end
	$r_log insert end "file: $s\n"

	if [catch { set fd [open $s r] } errmsg] {
		i_error "$errmsg"
		incr nrc	;# ?
		return
	}
	set cnt 1
	while {[gets $fd line] >= 0} {
		if {$linenumbers} {
			if {$cnt < 10} {
				$r_log insert end "    "
			} elseif {$cnt < 100} {
				$r_log insert end "   "
			} elseif {$cnt < 1000} {
				$r_log insert end "  "
			} elseif {$cnt < 10000} {
				$r_log insert end " "
			}
			$r_log insert end "$cnt  "
		}
		$r_log insert end "$line\n"
		incr cnt
	}
	catch "close $fd"
	update
	$r_log yview 0
	update
	return
}

proc do_this {n} {
	global h_log c_log nrc

	set x [$h_log get $n.0 $n.end]

	$c_log insert end "$x"
	handle_command $x "do_this"
	$c_log insert end "\n"
	$c_log yview -pickplace end
	incr nrc
	update
}

proc file_ok {f} {

	if {[file exists $f]} {
		if {![file isfile $f] || ![file writable $f]} {
			put_result "error: $f is not writable"
			return 0
	}	}
	return 1
}

proc writeoutfile {to which} {
	global h_log r_log

	if ![file_ok $to] { return 0 }

	if [catch {set fd [open $to w]} errmsg] {
		add_log $errmsg 0
		return 0
	}
	fconfigure $fd -translation lf	;# no cr at end of line, just lf

	if {$which == 0} {
		set from $h_log		;# history
	} else {
		set from $r_log		;# results
	}

	scan [$from index end] %d numLines
	for {set i 2} {$i < $numLines} {incr i} {
		set line [$from get $i.0 $i.end]
		if {[string length $line] > 0} {
			puts $fd $line
	}	}
	close $fd

	put_result "<saved $to>"

	return 1
}

proc save_hist {} {

	set f [tk_getSaveFile]
	if {$f != ""} { writeoutfile $f 0 }
}

proc save_results {} {

	set f [tk_getSaveFile]
	if {$f != ""} { writeoutfile $f 1 }
}

proc load_pats {} {

	set f [tk_getOpenFile]
	if {$f == ""} { return }

	if [catch {set fd [open $f r]} err] {
		i_error "$err"
		return
	}
	catch {
		while {[gets $fd line] > -1} {
			if {[string first "ps " $line] == 0 \
			||  [string first "pe " $line] == 0} {
				handle_command "$line" "load_pats"
			} else {
				i_error "load_pats unrecognized input: $line"
		}	}
		close $fd
	} emsg
	if {"$emsg" != ""} {
		i_error "$emsg"
	} else {
		handle_command "ps list" "load_pats"
	}
}

proc save_pats {} {
	global nrpat ptitle pmap pnm2nr pn2pe

	# save user-defined patterns, if any
	# puts "there are (up to) $nrpat user-defined patterns"

	if {$nrpat > 0} {
		set f [tk_getSaveFile]
		if {$f == "" || ![file_ok $f]} {
			return
		}
		if [catch {set fd [open $f w]} err] {
			i_error "$err"
			return
		}
	} else {
		i_error "there are no user-defined patterns yet"
		return
	}

	catch {
		fconfigure $fd -translation lf		;# no cr at end of line
		foreach x [array names pnm2nr] {	;# "pnm2nr $index : $pnm2nr($index)"
			set a $pnm2nr($x)
			set b $pmap($x)
			set c $pn2pe($x)
			set d $ptitle($x)

		#	puts "title:: $d = description of pattern $x"
		#	puts "nr::  $x => menu item .patterns.main.b$a"
		#	puts "map:: $b = the nr of matches in pattern $x"
		#	puts "pe::  $c = the search pattern of pattern $x"

			puts $fd "pe $x: $c"
			puts $fd "ps caption $x $d"
		}
	} emsg
	if {"$emsg" != ""} {
		i_error "$emsg"
	}
	catch {
		close $fd
	}
}

proc reverse {lg n} {
	global HV1
#	$lg tag configure $n -font $HV1	;# -foreground black -background white
}

proc normal {lg n} {
	global CW1
#	$lg tag configure $n -font $CW1 ;# -foreground white -background black
}

set ftags(0) {}
set htags(0) {}
set fnames {}

proc tagfile {fn n} {
	global ftags fnames

	lappend ftags($fn) $n

	set idx [lsearch $fnames $fn]
	if { $idx == -1 } {
		lappend fnames $fn
	}
}

set tnow(0) 0
set tcnt(0) 0

proc move_doit {f fn} {
	global htags stags
	global tcnt		;# tcnt is total nr of matches
	global tnow		;# the current pick+1 (to start at 1..)

	set colors(0) red
	set colors(1) blue
	set colors(2) orange
	set colors(3) green
	set colors(4) purple
	set colors(5) gold
	set colors(6) firebrick

	set seqn 0
	set setn 0
	foreach nm [array names htags] {
		set nnrs [llength $htags($nm)]
		while {$nnrs > 0} {
			incr nnrs -1
			incr seqn
			if {$seqn == $tnow($fn)} {	;# matched
				set lnr [lindex $htags($nm) $nnrs]
				.f$f.t yview -pickplace [expr $lnr - 12]
				.f$f.top.mid configure -text "#$tnow($fn) of $tcnt($fn)"
				.f$f.top.setname configure -text " $stags($fn,$lnr) line $lnr" -fg $colors($setn) -bg white

				.f$f.t tag configure sel$lnr -foreground $colors($setn) -background white

				return
		}	}
		set setn [expr ($setn + 1) % 7]	;# circle thru colors to mark matches in each set differently
	}
}

proc move_forward {f fn} {
	global tcnt		;# total nr of matches
	global tnow		;# current pick+1

	incr tnow($fn) -1	;# base 0
	set tnow($fn) [expr 1 + (($tnow($fn) + 1) % $tcnt($fn))]
	move_doit $f $fn
}

proc move_first {f fn} {
	global tcnt		;# total nr of matches
	global tnow		;# current pick+1

	set tnow($fn) 1
	move_doit $f $fn
}

proc move_last {f fn} {
	global tcnt		;# total nr of matches
	global tnow		;# current pick+1

	set tnow($fn) $tcnt($fn)
	move_doit $f $fn
}

proc move_back {f fn} {
	global htags stags
	global tcnt tnow

	incr tnow($fn) -1
	if {$tnow($fn) <= 0} {
		set tnow($fn) $tcnt($fn)
	}
	move_doit $f $fn
}

proc showfile {fn n how} {
	global ftags htags stags
	global tcnt tnow c_log nrc bw_mode

	set f ""
	set gn $fn
	set x [string last "/" $fn]
	if {$x >= 0} {
		incr x
		set gn [string range $fn $x end]
	}
	scan $gn "%\[a-zA-z_\]\." f

	if {[winfo exists .f$f] == 0} {
		toplevel .f$f
		wm title .f$f "matches in file: $f"
		wm iconname .f$f "$f"

		if {$how} {	;# using htags
			frame .f$f.top -width 120 -height 2

			set tcnt($fn) 0
			set tnow($fn) 1
			foreach nm [array names htags] {
				incr tcnt($fn) [llength $htags($nm)]
			}
			# puts "tcnt $fn is now: $tcnt($fn)"

			label .f$f.top.t -text "[array size htags] patterns matched, $tcnt($fn) total matches"
			pack append .f$f.top .f$f.top.t { left expand fillx filly}

			label .f$f.top.setname -text " $stags($fn,$n) " -relief raised -bg white -fg red

			button .f$f.top.first -text "<<" -command "move_first $f $fn"
			button .f$f.top.left -text "<" -command "move_back $f $fn"
			label .f$f.top.mid -text "#$tnow($fn) of $tcnt($fn)" -relief raised
			button .f$f.top.right -text ">" -command "move_forward $f $fn"
			button .f$f.top.last -text ">>" -command "move_last $f $fn"

			pack append .f$f.top .f$f.top.first { left filly }
			pack append .f$f.top .f$f.top.left { left filly }
			pack append .f$f.top .f$f.top.mid  { left filly }
			pack append .f$f.top .f$f.top.setname {left filly }
			pack append .f$f.top .f$f.top.right { left filly }
			pack append .f$f.top .f$f.top.last { left filly }

			pack append .f$f .f$f.top { top fillx }
		}
	
		text .f$f.t -relief raised -bd 2 \
			-width 120 -height 24 \
			-setgrid 1
		pack append .f$f .f$f.t { top expand fillx filly }

		set fd -1
		if [catch { set fd [open $fn r] } errmsg] {
			i_error "$errmsg"
			incr nrc
			return
		}
		set cnt 1
		while {[gets $fd line] >= 0} {
			.f$f.t insert end "$cnt  $line\n"
			.f$f.t tag add sel$cnt $cnt.0 $cnt.end
			incr cnt
		}
		catch "close $fd"
	}
	.f$f.t yview -pickplace [expr $n - 12]

	if {$bw_mode} {
		catch {
			.f$f.t configure -bg black -fg white
			foreach x $ftags($fn) {
				.f$f.t tag configure sel$x -foreground red -background black
		}	}
	} else {
		catch {
			.f$f.t configure -bg white -fg black
			foreach x $ftags($fn) {
				.f$f.t tag configure sel$x -foreground red -background white
		}	}
	}

#	foreach nm [array names htags] {
#		set nnrs [llength $htags($nm)]
#		puts $nm
#		while {$nnrs > 0} {
#			incr nnrs -1
#			set lnr [lindex $htags($nm) $nnrs]
#			if {$lnr >= 0} {
#				if {$lnr < $smallest} {
#					set smallest $lnr
#				}
#				puts "$nm matched on $fnm line $lnr"
#				lappend ftags($fn) $lnr
#		}	}
#	}

}

proc put_result {s} {
	global nrr r_log noupdate

	incr nrr
	$r_log insert end "$s\n"
	if {$noupdate == 0} {
		$r_log yview -pickplace end
	}
	set x [scan $s "%d\: %\[a-zA-Z_\./\]%\[:\]%d%\[:\] %s" a fnm c lnr e f]
	if {$x >= 5} {
		$r_log tag add  hist$nrr $nrr.0 $nrr.end
#		$r_log tag bind hist$nrr <Any-Enter> "reverse $r_log hist$nrr"
#		$r_log tag bind hist$nrr <Any-Leave> "normal $r_log hist$nrr"
		$r_log tag bind hist$nrr <ButtonPress-1> "showfile $fnm $lnr 0"
		tagfile $fnm $lnr	;# prep
	}
}

proc put_files {s} {
	global r_log f_log frh

	incr frh
	$f_log insert end "$s\n"
	$f_log yview -pickplace end	
	
	$f_log tag add  fls$frh $frh.0 $frh.end
	$f_log tag bind fls$frh <Any-Enter> "reverse $f_log fls$frh"
	$f_log tag bind fls$frh <Any-Leave> "normal $f_log fls$frh"
	$f_log tag bind fls$frh <ButtonPress-1> "show_source $s"
}

proc put_hist {s} {
	global h_log nrh

	incr nrh
	$h_log insert end "$s\n"
	$h_log yview -pickplace end
	
	$h_log tag add  hist$nrh $nrh.0 $nrh.end
	$h_log tag bind hist$nrh <Any-Enter> "reverse $h_log hist$nrh"
	$h_log tag bind hist$nrh <Any-Leave> "normal $h_log hist$nrh"
	$h_log tag bind hist$nrh <ButtonPress-1> "do_this $nrh"
}

proc r_clear {} {
	global r_log nrr
	global fnames ftags

	$r_log delete 0.0 end
	set nrr 0

	foreach fn $fnames {
		scan $fn "%\[a-zA-z_\]\." f
		catch { destroy .f$f }
		set ftags($fn) {}
	}
	set fnames {}
}

set dflt "l"

proc process_line {line} {
	global c_log noupdate verbose
	global qfd nrm nrc
	global htags

	if {$noupdate == 0 && $verbose} { puts "process_line $line" }

	if {[string first "wrote: " $line] == 0} {		;# "wrote"
		put_result "$line"
	} elseif {[string first "view with: " $line] == 0} {	;# "view with"
		put_hist "!dot -Tx11 cobra.dot &"
	} elseif {[string first " matches" $line] > 0} {	;# "matches"
		set pn ""
		if {[string first " stored in " $line] > 0} {	;# "stored in"
			scan $line "%d matches stored in %s" nrm pn
			if {$nrm > 0} {
				patterns_panel $nrm $pn
				set line "$nrm patterns"
			}
		}
	        if {$nrm > 0 && [string length $line] < 16} {
			$c_log insert end "\t=> $line"
			if {"$pn" != ""} {
				$c_log insert end " ($pn)\n"
				incr nrc
			}
			$c_log yview -pickplace end
			scan $line "%d matches" nrm
		}
	} elseif {[string first "title " $line] >= 0} {		;# "title"
		scan $line "%d title %s = %s" n a b
		set x [string first " = " $line]
		if {$x > 0} {
			incr x 3
			set b [string range $line $x end]
			patterns_title $n $a "$b"
		}
	} elseif {[string first "opened file " $line] >= 0} {	;# "opened file"
		scan $line "opened file %s" a
		put_files "$a"
	} elseif {[string first "tagged" $line] == 0} {
		set st [scan $line "tagged %d %d %d %s" nfrom nto ncnt nset]
		lappend htags($nset) $nfrom	;# worry about $nto later
	} else {
		put_result "$line"
	}
}

proc handle_command {cmd who} {
	global c_log dflt
	global qfd nrm d_mode
	global noupdate verbose

	if {$verbose} { puts "handle command $cmd -- $who" }

	if {$d_mode == 0} { set dflt "l" }
	if {$d_mode == 1} { set dflt "d" }
	if {$d_mode == 2} { set dflt "p" }

	set nrm 0
	if {[string length $cmd] > 0} {
		if {[string match "q"    $cmd] \
		||  [string match "quit" $cmd]} {
			puts $qfd "q"
			close $qfd
			exit 0
		}
		if [string match "r" $cmd] {
			r_clear
		}
		if {![string match "l" $cmd]} {
			put_hist $cmd
		}
		if {$noupdate == 0} {
			update
		}
		catch {
			puts $qfd "$cmd"
			flush $qfd
			while {[gets $qfd line] >= 0} {
				if {[string first "Ready: " $line] >= 0} {
					break
				}
				process_line $line
				if {$noupdate == 0} {
					update
			}	}
			if {$noupdate == 0} {
				update
			}
		} err
		if {$err != ""} {
			put_result "error: $err"
	}	}

	if {$nrm > 0 && ![string match $dflt $cmd]} {	;# if there were matches, prevent recursion
		r_clear
		handle_command $dflt "handle"		;# use the default display mode
	}

	$c_log yview -pickplace end
	$c_log configure -cursor bottom_tee

	if {$noupdate == 0} {
		update
	}
}

proc launchBrowser {url} {
	global tcl_platform
	set sys $tcl_platform(platform)
	# gh: gives inconsisten responses 4/2022
	# gh: see what works

	# windows
	set command [list {*}[auto_execok start] {}]
	catch {exec {*}$command $url &} err
	if {[string first "execute" $err] == -1} { return }

	# cygwin
	catch {exec rundll32 url.dll,FileProtocolHandler $url &} err
	if {[string first "execute" $err] == -1} { return }

	# Darwin
 	set command [list open]
	catch {exec {*}$command $url &} err
	if {[string first "execute" $err] == -1} { return }

	# unix/ubuntu
	set command "sensible-browser"
	catch {exec {*}$command $url &} err
	if {[string first "execute" $err] == -1} { return }

	set command [list xdg-open]
	catch {exec {*}$command $url &} err
	if {[string first "execute" $err] == -1} { return }

	tk_messageBox -icon error \
		-message "error: failed to invoke browser on $sys for $url"
}

proc iManual {} {
	launchBrowser "https://spinroot.com/cobra/icobra.html"
}

proc Manual {} {
	launchBrowser "https://spinroot.com/cobra/manual.html"
}

proc Tutorial {} {
	launchBrowser "https://github.com/nimble-code/Cobra/blob/master/doc/Tutorial_July_2019.pdf"
}

proc do_button {x} {
	global c_log nrc

	$c_log insert end "$x"
	incr nrc
	handle_command "$x" "do_button"
	$c_log insert end "\n"
	$c_log yview -pickplace end
}

proc shortcut {a b c d} {
	global NBG NFG HV1

	button $a.$b -text $d -command "do_button $c" \
		-bg $NBG -fg $NFG \
		-font $HV1 \
		-activebackground $NFG \
		-activeforeground $NBG
	pack $a $a.$b -side right -fill both -expand yes
}

proc changeformat {p} {
	global dflt d_mode

	r_clear

	if {$p == 0} {
		set dflt "l"
		handle_command "mode tok" "changeformat"
		.menubar.mode configure -text "ViewMode: Tokens"
	}
	if {$p == 1} {
		handle_command "json_format off" "changeformat"
		set dflt "d"
		handle_command "mode src" "changeformat"
		.menubar.mode configure -text "ViewMode: Context"
	}
	if {$p == 2} {
		set dflt "p"
		handle_command "mode pre" "changeformat"
		.menubar.mode configure -text "ViewMode: Marked"
	}
	if {$p == 3} {
		set dflt "j"
		handle_command "json_format" "changeformat"
		.menubar.mode configure -text "ViewMode: JSON"
	}
	set d_mode $p
}

proc main_panels {t} {
	global c_log r_log h_log f_log
	global HV0 HV1 HV2 CW1 WMain
	global CBG CFG
	global TBG TFG
	global NFG NBG
	global nr d_mode nrc

	set pw  [PanedWindow $t.pw -side left -activator button ]

  # cobra output window, left
	set p2  [$pw add -minsize 200]
	set xx [PanedWindow $p2.wide -side top -activator button ]
	set q0 [$xx add -minsize 200]
	set q1 [$xx add -minsize  80]
	set sw22   [ScrolledWindow $q0.wide]
	set r_log  [text $sw22.lb -undo 1 -width 300 \
		-takefocus 0 \
		-highlightthickness 0 -font $CW1]	;# was $HV1
	$sw22 setwidget $r_log

  # shortcuts
	frame $q0.ctl -bg $NBG
	pack $q0 $q0.ctl -side top
	pack $sw22 -side left -fill both -expand yes
	pack $xx              -fill both -expand yes
	$r_log insert end "results:\n"
	$r_log edit modified false

  # cobra command window, bottom
	set p1     [$pw add -minsize 20]	;# command window, bottom
        set sw11   [ScrolledWindow $p1.sw]
        set c_log   [text $sw11.lb -height 8 \
		-takefocus 1 \
		-highlightthickness 3 -bg $CBG -fg $CFG -font $HV2]
	$sw11 setwidget $c_log
	pack $sw11 -fill both -expand yes

	bind $c_log <Return> {
		incr nrc
		handle_command [$c_log get $nrc.0 $nrc.end] "main"
	}

  # files list
	set cv [ScrolledWindow $q1.wide]
	set f_log [text $cv.right -width 280 \
		-bg $CBG -fg $CFG -font $HV1]
	$cv setwidget $f_log

	frame $q1.ctl -bg $NBG

	pack $q1 $q1.ctl -side top

	$f_log insert end "files:\n"

	# command history log
	set h_log [text $cv.right2 -width 250 \
		-takefocus 0 \
		-bg $CBG -fg $CFG -font $HV1]

	pack $cv -side right -fill y -expand no
	pack $pw -fill both -expand yes

	$h_log insert end "history:\n"
}

proc helper {} {
	global qfd h_l

	if {0} {
		puts $qfd "?"
		flush $qfd
		while {[gets $qfd line] >= 0} {
			if {[string first "Ready: " $line] >= 0} {
				break
			}
			$h_l insert end "	$line\n"
		}
	} else {
		$h_l insert end "A brief synopsis of what you can do:\n\
   \n\
Start icobra with a list of files to analyze, for instance: \n\
   \n\
           $ icobra *.h *.c\n \
   \n\
Using the Predefined Checkers:\n \
1. Click on one of the predefined Cobra checkers in the panel on the right\n \
2. A summary of the analysis results (if any) will appear in a separate menu\n \
3. Clicking on a results entry lists all results in the main panel, and pops\n \
   up an additional panel with numbered entries, one for each match found\n \
4. Clicking on a results square will display that match in the main results panel\n \
   \n\
Defining New Search Patterns:\n \
1. Give a short name to the set that will collect the results of the search (if any)\n \
2. Add a description of the set, so that you can remember what you tried to find\n \
3. Define the search pattern, remember to separate tokens with spaces, as in: while ( 0 )\n \
4. Results, if any, will again pop up in a menu, the you can use to browse the results as before\n \
   \n\
The Metrics tab\n \
- Highlight some metric for the current application.\n \
   \n\
The Heatmap tab \n \
- Shows where most warnings that are stored in Pattern Sets\n \
are located. You can click on each file to get the details and navigate\n \
through the set of warnings. Each warning is color coded by the Pattern Set\n \
that contains it \n \
   \n\
The Warnings tab \n \
- Is a reverse heatmap, showing which files contain most warnings from pattern\n \
sets. You can click on each file to get the details and navigate\n \
through the set of warnings. Each warning is again color coded by the Pattern Set\n \
that contains it \n \
   \n\
Other Commands:\n \
Type any valid Cobra query command in the command panel at the bottom of the display\n \
For instance, typing \n \
	\n \
  	m while (\n \
	d \n \
	\n \
will report all matches of this sequence of tokens in the top panel.\n \
   \n\
Type ? in the command window to get an overview of the command language itself.\n\
or check the online manual and tutorial pages with the top right buttons on this page."
	}
	update
}

set gw 280.0
set gh 250.0

proc place_box {x y w h mx my} {
	global m_l gh gw

	$m_l create line \
		$x             $y \
		$x             [expr $y + $h] \
		[expr $x + $w] [expr $y + $h] \
		-fill black

	$m_l create text \
		[expr $x + $w + 5] $y -text "($my)" -anchor w		;# max y value
	$m_l create text \
		[expr $x + $w + 5] [expr $y + $gh / 2] -text "([expr $my / 2.0])" -anchor w
	$m_l create text \
		[expr $x + $w + 5] [expr $y + $gh / 2 - $gh / 4] -text "([expr 3.0 * $my / 4.0])" -anchor w
	$m_l create text \
		[expr $x + $w + 5] [expr $y + $gh / 2 + $gh / 4] -text "([expr $my / 4.0])" -anchor w

	$m_l create line \
		$x                  $y \
		[expr $x + $w - 10] $y \
		-fill lightgrey
	$m_l create line \
		$x                  [expr $y + $gh / 2] \
		[expr $x + $w - 10] [expr $y + $gh / 2] \
		-fill lightgrey
	$m_l create line \
		$x                  [expr $y + $gh / 2 - $gh / 4] \
		[expr $x + $w - 10] [expr $y + $gh / 2 - $gh / 4] \
		-fill lightgrey
	$m_l create line \
		$x                  [expr $y + $gh / 2 + $gh / 4] \
		[expr $x + $w - 10] [expr $y + $gh / 2 + $gh / 4] \
		-fill lightgrey
}

proc prepdata {f tf c_h} {
	global noupdate HV1

	set noupdate 1
	handle_command "!rm -f $tf" "prepdata"
	handle_command "quiet on; track start $tf" "prepdata"
	handle_command "$f" "prepdata"
	handle_command "track stop; quiet off" "prepdata"
	set noupdate 0

	if [catch {set dfd [open $tf r]} err] {
		$c_h create text 100 100 \
			-text "cannot open $tf : $err" \
			-font $HV1 -fill red
		return -1
	}
	return $dfd
}

set maxsz 0
set maxnr 0

proc getdata {x y dfd fstr} {
	global maxsz maxnr gh gw
	global numbers m_l HV1

	set maxsz 0
	set maxnr 0

	while {[gets $dfd line] >= 0} {
		set x [scan $line "$fstr" sz nr]
		if {$x != 2} {
			puts "$x bad read: $line -- $fstr"
		} else {
			set numbers($sz) $nr
			if {$sz > $maxsz} { set maxsz $sz }
			if {$nr > $maxnr} { set maxnr $nr }
	}	}
	catch "close $dfd" emsg

	if {$maxsz <= 0 || $maxnr <= 0} {
		$m_l create text \
			[expr $x + $gw / 2] [expr $y + $gh / 2] \
			-text "no data" \
			-font $HV1 -fill red
		return -1
	}
	return 0
}

proc plot_data {cnr x y xfactor yfactor spacer minwidth limit} {
	global numbers m_l gh gw
	global maxsz maxnr

	set first 1
	set by [expr $y + $gh]	;# base y
	set sx [expr $x + 1]	;# lhs

	catch { $m_l delete chart$cnr }

	for {set index 1} {1} {incr index} {
		set a $index
		set b 0
		catch {	;# not all size are present
			set b $numbers($a)
			set col "light blue"
			set numbers($a) 0
		} emsg

		set nx [expr $x + $a / $xfactor]
		set ny [expr $y + $gh - $b / $yfactor]

		if {$minwidth > 10} {
			set nx [expr $nx -15.0]
		}

		set ox $sx
		set tx $nx
		if {[expr $nx - $sx] < $minwidth} {
			set ox [expr $sx - $minwidth / 2]
			set tx [expr $sx + $minwidth / 2]
		}

		if {$a > $limit} {
			set col "light coral"
		}

		if {$b > 0 && [string first "no such element" $emsg] < 0} {
		  if {$first} {
			$m_l create rectangle $ox $by $tx $ny -fill "$col" -tags chart$cnr
			set first 0	;# set sx and sy below
		  } else {
			$m_l create rectangle $ox $by $tx $ny -fill "$col" -tags chart$cnr
		} }
	 	set sx $nx
		set sy $ny
	
		# horizontal tick marks & values
		if {$a == 1 || $a % $spacer == 0 || $a == $maxsz} {
			if {$spacer == 1 || $a == $maxsz || $a < [expr $maxsz - 2]} {
			$m_l create text \
				$sx [expr $y + $gh + 5] -text "$a" -anchor nw -tags chart$cnr
		}	}
		if {$index >= $maxsz} { break }
	}
}

proc chart_ident_length {x y tf limit} {
	global m_l HV1
	global gh gw maxsz maxnr numbers

	# 1. generate the data
	set dfd [prepdata ". stats/ident_length0.cobra" $tf $m_l]
	if {$dfd == -1} { return }

	# 2. read the data y:sz and x:nr
	if {[getdata $x $y $dfd "%d char : %d"] == -1} { return }

	# 3. compute scale factors
	set xfactor [expr $maxsz / $gw]
	set yfactor [expr $maxnr / $gh]

	# 4. plot x and y coordinates, and add ticks for y-axis
	place_box $x $y $gw [expr $gh + 0] $maxsz $maxnr

	# 5. add title
	$m_l create text \
		[expr $x + $gw / 2] $y \
		-text "Identifier Lengths (1..${maxsz} chars)" \
		-font $HV1 -fill black -anchor s

	# 6. plot the data points and ticks for x-axis
	plot_data 1 $x $y $xfactor $yfactor 4 1 $limit

	# 7. add vertical warning line at nominal max value
	$m_l create line \
		[expr $x + $limit / $xfactor] $y \
		[expr $x + $limit / $xfactor] [expr $y + $gh + 5] \
		-fill "light salmon"

	# 8. cleanup
	handle_command "!rm -f $tf" "chart_ident_length"

	update
}

proc chart_fct_length {x y tf limit} {
	global m_l HV1
	global gh gw maxsz maxnr numbers

	# 1. generate the data
	set dfd [prepdata ". stats/fct_len_list0.cobra" $tf $m_l]
	if {$dfd == -1} { return }

	# 2. read the data y:sz and x:nr
	if {[getdata $x $y $dfd "%d %d"] == -1} { return }

	# 3. compute scale factors
	set xfactor [expr $maxsz / $gw]
	set yfactor [expr $maxnr / $gh]

	# puts "$maxsz $maxnr -- factors $xfactor $yfactor";

	# 4. plot x and y coordinates, and add ticks for y-axis
	place_box $x $y $gw [expr $gh + 0] $maxsz $maxnr

	# 5. add title
	$m_l create text \
		[expr $x + $gw / 2] $y \
		-text "Nr of Functions with less than N lines" \
		-font $HV1 -fill black -anchor s

	# 6. plot the data points and ticks for x-axis
	plot_data 2 $x $y $xfactor $yfactor 20 28 $limit

	# 7. add vertical warning line at nominal max value
	$m_l create line \
		[expr $x + $limit / $xfactor] $y \
		[expr $x + $limit / $xfactor] [expr $y + $gh + 5] \
		-fill "light salmon"

	# 8. cleanup
	handle_command "!rm -f $tf" "chart_fct_length"
	update
}

proc chart_fct_params {x y tf limit} {
	global m_l HV1
	global gh gw maxsz maxnr numbers

	# 1. generate the data
	set dfd [prepdata ". stats/fct_param_count0.cobra" $tf $m_l]
	if {$dfd == -1} { return }

	# 2. read the data y:sz and x:nr
	if {[getdata $x $y $dfd "%d %d"] == -1} { return }

	# 3. compute scale factors
	set xfactor [expr $maxsz / $gw]
	set yfactor [expr $maxnr / $gh]

	# puts "$maxsz $maxnr -- factors $xfactor $yfactor";

	# 4. plot x and y coordinates, and add ticks for y-axis
	place_box $x $y $gw [expr $gh + 0] $maxsz $maxnr

	# 5. add title
	$m_l create text \
		[expr $x + $gw / 2] $y \
		-text "Nr of Functions with N Params" \
		-font $HV1 -fill black -anchor s

	# 6. plot the data points and ticks for x-axis
	plot_data 3 $x $y $xfactor $yfactor 1 1 $limit

	# 7. add vertical warning line at nominal max value
	$m_l create line \
		[expr $x + $limit / $xfactor] $y \
		[expr $x + $limit / $xfactor] [expr $y + $gh + 5] \
		-fill "light salmon"

	# 8. cleanup
	handle_command "!rm -f $tf" "chart_fct_params"
	update
}

proc chart_fct_per_file {x y tf limit} {
	global m_l HV1
	global gh gw maxsz maxnr numbers

	# 1. generate the data
	set dfd [prepdata ". stats/fct_per_file0.cobra" $tf $m_l]
	if {$dfd == -1} { return }

	# 2. read the data y:sz and x:nr
	if {[getdata $x $y $dfd "%d %d"] == -1} { return }

	# 3. compute scale factors
	set xfactor [expr $maxsz / $gw]
	set yfactor [expr $maxnr / $gh]

	# puts "$maxsz $maxnr -- factors $xfactor $yfactor";

	# 4. plot x and y coordinates, and add ticks for y-axis
	place_box $x $y $gw [expr $gh + 0] $maxsz $maxnr

	# 5. add title
	$m_l create text \
		[expr $x + $gw / 2] $y \
		-text "Nr of Files with upto N Functions" \
		-font $HV1 -fill black -anchor s

	# 6. plot the data points and ticks for x-axis
	plot_data 4 $x $y $xfactor $yfactor 10 20 $limit

	# 7. add vertical warning line at nominal max value
	$m_l create line \
		[expr $x + $limit / $xfactor] $y \
		[expr $x + $limit / $xfactor] [expr $y + $gh + 5] \
		-fill "light salmon"

	# 8. cleanup
	handle_command "!rm -f $tf" "chart_fct_per_file"
	update

}

proc chart_ncsl {x y tf limit} {
	global m_l HV1
	global gh gw maxsz maxnr numbers

	# 1. generate the data
	set dfd [prepdata ". stats/ncsl0.cobra" $tf $m_l]
	if {$dfd == -1} { return }

	# 2. read the data y:sz and x:nr
	if {[getdata $x $y $dfd "%d %d"] == -1} { return }

	# 3. compute scale factors
	set xfactor [expr $maxsz / $gw]
	set yfactor [expr $maxnr / $gh]

	if {$yfactor == 0.0} { set yfactor 1.0 }	;# precaution

	# puts "$maxsz $maxnr -- factors $xfactor $yfactor";

	# 4. plot x and y coordinates, and add ticks for y-axis
	place_box $x $y $gw [expr $gh + 0] $maxsz $maxnr

	# 5. add title
	$m_l create text \
		[expr $x + $gw / 2] $y \
		-text "Nr of Files with Comment/Code Ratio" \
		-font $HV1 -fill black -anchor s

	# 6. plot the data points and ticks for x-axis
	catch {
		plot_data 5 $x $y $xfactor $yfactor 10 20 $limit
	} emsg
	if {$emsg != ""} { puts "error: $xfactor $yfactor $emsg"; }

	# 7. add vertical warning line at nominal max value
	$m_l create line \
		[expr $x + $limit / $xfactor] $y \
		[expr $x + $limit / $xfactor] [expr $y + $gh + 5] \
		-fill "light salmon"
	# there is no perfect comment ratio of course
	# here we pick 25% as a sample target

	# 8. cleanup
	handle_command "!rm -f $tf" "chart_ncsl"
	update
}

proc chart_cyclo {x y tf limit} {
	global m_l HV1
	global gh gw maxsz maxnr numbers

	# 1. generate the data
	set dfd [prepdata ". play/cyclo0.cobra" $tf $m_l]
	if {$dfd == -1} { return }

	# 2. read the data y:sz and x:nr
	if {[getdata $x $y $dfd "%d %d"] == -1} { return }

	# 3. compute scale factors
	set xfactor [expr $maxsz / $gw]
	set yfactor [expr $maxnr / $gh]

	if {$yfactor == 0.0} { set yfactor 1.0 }	;# precaution

	# puts "$maxsz $maxnr -- factors $xfactor $yfactor";

	# 4. plot x and y coordinates, and add ticks for y-axis
	place_box $x $y $gw [expr $gh + 0] $maxsz $maxnr

	# 5. add title
	$m_l create text \
		[expr $x + $gw / 2] $y \
		-text "Nr of Files with Cyclomatic Complexity" \
		-font $HV1 -fill black -anchor s

	# 6. plot the data points and ticks for x-axis
	catch {
		plot_data 6 $x $y $xfactor $yfactor 10 20 $limit
	} emsg
	if {$emsg != ""} { puts "error: $xfactor $yfactor $emsg"; }

	# 7. add vertical warning line at nominal max value
	$m_l create line \
		[expr $x + $limit / $xfactor] $y \
		[expr $x + $limit / $xfactor] [expr $y + $gh + 5] \
		-fill "light salmon"
	# there is no perfect comment ratio of course
	# here we pick 25% as a sample target

	# 8. cleanup
	handle_command "!rm -f $tf" "chart_cyclo"
	update
}

proc dashboard {} {
	global m_l HV1
	global tcl_precision gh gw

	set xoffset 20
	set yoffset 20
	set tcl_precision 3

	catch {
		regexp {(.*)x(.*)[+-](.*)[+-](.*)} [wm geometry .] -> WMain HMain nx ny
		# puts "w: $WMain h: $HMain x: $nx y: $ny"
		$m_l configure -height $HMain -width $WMain
		# sets bg color correctly
		# but we don't resize the charts below to adjust
	}

	# canvas is nominally 800x1200 ($HMain x $WMain)
	# (x=0,y=0) is topleft
	# plan up to 200x400 chart tiles (4x3=12)

	# top row
	chart_ident_length $xoffset                   [expr $yoffset + 20]       "_TMP1_" 32
	chart_fct_length   [expr $xoffset + $gw + 80] [expr $yoffset + 20]       "_TMP2_" 66
	chart_fct_params   [expr $xoffset + 2 * ($gw + 80)] [expr $yoffset + 20] "_TMP3_" 5

	# bottom row
	chart_fct_per_file $xoffset [expr $yoffset + $gh + 80] "_TMP4_" 50
	chart_ncsl [expr $xoffset + $gw + 80]  [expr $yoffset + $gh + 80]        "_TMP5_" 25
	chart_cyclo [expr $xoffset + 2 * ($gw + 80)]  [expr $yoffset + $gh + 80] "_TMP6_" 15
}

proc annotated {fn} {
	global htags ftags stags

	# 1. read warnings filtered by $fn

	handle_command "dp filter $fn"	"annotated"
	handle_command "terse on" 	"annotated"
	if {[array size htags] > 0} {
		array unset htags
	}
	catch {
		set ftags($fn) {}
	}
	handle_command "dp *" 		"annotated"
	handle_command "dp filter off"	"annotated"

	# 2. add tags: tagfile {fn ln}

#	parray htags
#	puts "==start=="
	set smallest 100000
	foreach nm [array names htags] {
		set nnrs [llength $htags($nm)]
#		puts $nm
		while {$nnrs > 0} {
			incr nnrs -1
			set lnr [lindex $htags($nm) $nnrs]
			if {$lnr >= 0} {
				if {$lnr < $smallest} {
					set smallest $lnr
				}
#				puts "$nm matched on $fn line $lnr"
				lappend ftags($fn) $lnr
				set stags($fn,$lnr) $nm
		}	}
	}
#	puts "==done=="
	# 3. showfile $fn $ln
	showfile $fn $smallest 1
}

proc warnings {} {
	global wr_l HV0 HV1
	global HMain WMain
	global pmap pnm2nr

	set x 0.0
	set y 0.0
	set tf "_TMP7_"
	set total  0.0
	set need   0.0
	set width  0.0
	set toggle 0	;# 0 = horizontal, 1 = vertical

	set dfd [prepdata ". stats/warning_counts.cobra" $tf $wr_l]
	if {$dfd == -1} { return }

	catch { $wr_l tag delete heat }

	catch {
		regexp {(.*)x(.*)[+-](.*)[+-](.*)} [wm geometry .] -> WMain HMain nx ny
		# puts "w: $WMain h: $HMain x: $nx y: $ny"
		$wr_l configure -height $HMain -width $WMain
	}
	set w [expr $WMain - 25]
	set h [expr $HMain - 80]

	while {[gets $dfd line] >= 0} {
		if {[string first "PerSet" $line] != 0} {
			continue;
		}

		set n [scan $line "PerSet %d %d %s %d" nr cnt snm perc];

		if {$n < 4} {
			$wr_l create rect $x $y $w $h -fill white -tags heat
			$wr_l create text \
				[expr $x + ($w - $x) / 2] [expr $y - 20 + ($h - $y) / 2] \
				-text "Please update rules/stats/warning_counts.cobra for this version of iCobra" \
				-font $HV0 -fill black -tags heat
			catch "close $dfd" emsg
			handle_command "!rm -f $tf" "warnings"
			return
		}

		if {$cnt > 0} {
			set perc [expr (100.0 * double($nr)) / double($cnt) ]
		} else {
			continue
		}

		set total [expr $total + $perc]

		set need [expr ($w * $h * $perc) / 100]

		if {$n != 4 || $need <= 0} {
			continue
		}

		if {$perc >= 50} {
			set col #f00		;# red
		} elseif {$perc > 25} {
			set col #f60		;# orange
		} elseif {$perc > 12} {
			set col #fc0		;# yellow
		} elseif {$perc > 6} {
			set col #ff0		;# light yellow
		} else {
			set col #ffb		;# soft yellow
		}

		if {$toggle == 0} {	;# vertical
			set width [expr $need / ($h - $y)]
			set nperc [expr int($perc)]
			set bb [$wr_l create rect $x $y [expr $x + $width] $h -fill $col -tags heat]
			if {$perc > 2} {
				set bc [$wr_l create text \
					[expr $x + $width / 2] [expr $y - 20 + ($h - $y) / 2] \
					-text "$snm" -font $HV1 -fill black -tags heat]
				$wr_l create text \
					[expr $x + $width / 2] [expr $y + ($h - $y) / 2] \
					-text "($nperc% = $nr)" -font $HV1 -fill black -tags heat
			} else {
				set bc [$wr_l create text \
					[expr $x + $width / 2] [expr $y - 20 + ($h - $y) / 2] \
					-text "$nperc%" -font $HV1 -fill black -tags heat]
			}
			set x [expr $x + $width]
		} else {		;# horizontal
			set height [expr $need / ($w - $x)]
			set nperc [expr int($perc)]
			set bb [$wr_l create rect $x $y $w [expr $y + $height] -fill $col -tags heat]
			if {1 || $perc > 2} {
				set bc [ $wr_l create text \
					[expr $x + ($w - $x) / 2] [expr $y - 20 + $height / 2] \
					-text "$snm" -font $HV1 -fill black -tags heat]
				$wr_l create text \
					[expr $x + ($w - $x) / 2] [expr $y  + $height / 2] \
					-text "($nperc% = $nr)" -font $HV1 -fill black -tags heat
			} else {
				set bc [$wr_l create text \
					[expr $x + ($w - $x) / 2] [expr $y + $height / 2] \
					-text "$snm ($nperc% = $nr)" -font $HV1 -fill black -tags heat]
			}
			set y [expr $y + $height]
		}
		catch {
			$wr_l bind $bb <ButtonPress-1> "pat_show $snm $pmap($snm) $pnm2nr($snm)"
			$wr_l bind $bc <ButtonPress-1> "pat_show $snm $pmap($snm) $pnm2nr($snm)"
		} emsg
		if {$emsg != ""} {
			i_error "$emsg"
		}
		set toggle [expr 1 - $toggle]
	}

	if {$total == 0} {
		$wr_l create rect $x $y $w $h -fill white -tags heat
		$wr_l create text \
			[expr $x + ($w - $x) / 2] [expr $y - 20 + ($h - $y) / 2] \
			-text "There are no pattern sets with warnings to display yet.\n(This panel shows which warnings have most entries.)" \
			-font $HV0 -fill black -tags heat
	} elseif {$total < 100} {
		$wr_l create rect $x $y $w $h -fill #ccc -tags heat
		$wr_l create text \
			[expr $x + ($w - $x) / 2] [expr $y - 20 + ($h - $y) / 2] \
			-text "([expr 100 - int($total)]%)" -font $HV0 -fill white -tags heat
	}

	catch "close $dfd" emsg
	handle_command "!rm -f $tf" "warnings"
}

proc heatmap {} {
	global he_l HV0 HV1
	global HMain WMain
	# canvas size is 800x1200 ($HMain x $WMain)

	set x 0.0
	set y 0.0
	set tf "_TMP7_"
	set total  0.0
	set need   0.0
	set width  0.0
	set toggle 0	;# 0 = horizontal, 1 = vertical

	set dfd [prepdata ". stats/warning_counts.cobra" $tf $he_l]
	if {$dfd == -1} { return }

	catch { $he_l tag delete heat }

	catch {
		regexp {(.*)x(.*)[+-](.*)[+-](.*)} [wm geometry .] -> WMain HMain nx ny
		# puts "w: $WMain h: $HMain x: $nx y: $ny"
		$he_l configure -height $HMain -width $WMain
	}
	set w $WMain
	set h $HMain

	while {[gets $dfd line] >= 0} {
		if {[string first "PerFile" $line] != 0} {
			continue;
		}
		set n [scan $line "PerFile %d %d %s %d" nr cnt fnm perc];

		if {$cnt > 0} {
			set perc [expr (100.0 * double($nr)) / double($cnt) ]
		} else {
			continue
		}

		set total [expr $total + $perc]

		set need [expr ($w * $h * $perc) / 100]

		if {$n != 4 || $need <= 0} {
			continue
		}
		if {$perc >= 50} {
			set col #f00		;# red
		} elseif {$perc > 25} {
			set col #f60		;# orange
		} elseif {$perc > 12} {
			set col #fc0		;# yellow
		} elseif {$perc > 6} {
			set col #ff0		;# light yellow
		} else {
			set col #ffb		;# soft yellow
		}

		if {$toggle == 0} {	;# vertical
			set width [expr $need / ($h - $y)]
			set nperc [expr int($perc)]
			set bb [$he_l create rect $x $y [expr $x + $width] $h -fill $col -tags heat]
			if {$perc > 2} {
				set bc [$he_l create text \
					[expr $x + $width / 2] [expr $y - 20 + ($h - $y) / 2] \
					-text "$fnm" -font $HV1 -fill black -tags heat]
				$he_l create text \
					[expr $x + $width / 2] [expr $y + ($h - $y) / 2] \
					-text "($nperc% = $nr)" -font $HV1 -fill black -tags heat
			} else {
				set bc [$he_l create text \
					[expr $x + $width / 2] [expr $y - 20 + ($h - $y) / 2] \
					-text "$nperc%" -font $HV1 -fill black -tags heat]
			}
			set x [expr $x + $width]
		} else {		;# horizontal
			set height [expr $need / ($w - $x)]
			set bb [$he_l create rect $x $y $w [expr $y + $height] -fill $col -tags heat]
			if {1 || $perc > 2} {
				set bc [ $he_l create text \
					[expr $x + ($w - $x) / 2] [expr $y - 20 + $height / 2] \
					-text "$fnm" -font $HV1 -fill black -tags heat]
				$he_l create text \
					[expr $x + ($w - $x) / 2] [expr $y  + $height / 2] \
					-text "($nperc% = $nr)" -font $HV1 -fill black -tags heat
			} else {
				set bc [$he_l create text \
					[expr $x + ($w - $x) / 2] [expr $y + $height / 2] \
					-text "$fnm ($nperc% = $nr)" -font $HV1 -fill black -tags heat]
			}
			set y [expr $y + $height]
		}
		catch {
			$he_l bind $bb <ButtonPress-1> "annotated $fnm "
			$he_l bind $bc <ButtonPress-1> "annotated $fnm "
		}
		set toggle [expr 1 - $toggle]
	}
	if {$total == 0} {
		$he_l create rect $x $y $w $h -fill white -tags heat
		$he_l create text \
			[expr $x + ($w - $x) / 2] [expr $y - 20 + ($h - $y) / 2] \
			-text "There are no pattern sets with warnings to display yet.\n(The heatmap shows which files have most warnings.)" \
			-font $HV0 -fill black -tags heat
	} elseif {$total < 100} {
		$he_l create rect $x $y $w $h -fill #ccc -tags heat
		$he_l create text \
			[expr $x + ($w - $x) / 2] [expr $y - 20 + ($h - $y) / 2] \
			-text "([expr 100 - int($total)]%)" -font $HV0 -fill white -tags heat
	}
	# puts "Total: $total"
	catch "close $dfd" emsg
	handle_command "!rm -f $tf" "heatmap"
}

proc metrics_canvas {t} {
	global HV1 NBG m_l NFG NBG
	global HMain WMain

	catch {
		regexp {(.*)x(.*)[+-](.*)[+-](.*)} [wm geometry .] -> WMain HMain nx ny
	}

	set pw [PanedWindow $t.pw -side left -activator button ]
		set hp [$pw add -minsize 200]
	set xx [PanedWindow $hp.wide -side top -activator button ]
		set ql [$xx add -minsize 100]
		set wl [ScrolledWindow $ql.wide]

	set m_l [canvas $ql.db -relief raised -background white \
		-height $HMain -width $WMain -scrollregion "0 0 100 100" ]

	$wl setwidget $m_l

	pack $wl -side left -fill both
	pack $xx -side top  -fill both -expand yes
	pack $pw -fill both -expand yes
}

proc warnings_canvas {t} {
	global HV1 NBG NFG
	global HMain WMain
	global wr_l		;# warnings panel

	catch {
		regexp {(.*)x(.*)[+-](.*)[+-](.*)} [wm geometry .] -> WMain HMain nx ny
	}

	set pw [PanedWindow $t.pw -side left -activator button ]
		set hp [$pw add -minsize 200]
	set xx [PanedWindow $hp.wide -side top -activator button ]
		set ql [$xx add -minsize 100]
		set wl [ScrolledWindow $ql.wide]

	set wr_l [canvas $ql.db -relief raised -background white \
		-height $HMain -width $WMain -scrollregion "0 0 100 100" ]

	$wl setwidget $wr_l

	pack $wl -side left -fill both
	pack $xx -side top  -fill both -expand yes
	pack $pw -fill both -expand yes
}

proc heatmap_canvas {t} {
	global HV1 NBG NFG
	global HMain WMain
	global he_l		;# heatmap panel

	catch {
		regexp {(.*)x(.*)[+-](.*)[+-](.*)} [wm geometry .] -> WMain HMain nx ny
	}

	set pw [PanedWindow $t.pw -side left -activator button ]
		set hp [$pw add -minsize 200]
	set xx [PanedWindow $hp.wide -side top -activator button ]
		set ql [$xx add -minsize 100]
		set wl [ScrolledWindow $ql.wide]

	set he_l [canvas $ql.db -relief raised -background white \
		-height $HMain -width $WMain -scrollregion "0 0 100 100" ]

	$wl setwidget $he_l

	pack $wl -side left -fill both
	pack $xx -side top  -fill both -expand yes
	pack $pw -fill both -expand yes
}

proc help_panels {t} {
	global HV1 NBG h_l NFG NBG

	# contents in left tab
	# shorthands in right tab
	# main reference in middle tab

	set pw  [PanedWindow $t.pw -side left -activator button ]

	set hp [$pw add -minsize 200]
	set xx [PanedWindow $hp.wide -side top -activator button ]

	set ql [$xx add -minsize 100]
	set wl [ScrolledWindow $ql.wide]

	frame $ql.ctl -bg black
	button $ql.ctl.imanual -text " iCobra Manual " \
		-command iManual \
		-bg $NBG -fg $NFG \
		-activebackground $NFG \
		-activeforeground $NBG

	button $ql.ctl.manual -text " Cobra Manual " \
		-command Manual \
		-bg $NBG -fg $NFG \
		-activebackground $NFG \
		-activeforeground $NBG

	button $ql.ctl.tutorial -text " Cobra Tutorial " \
		-command Tutorial \
		-bg $NBG -fg $NFG \
		-activebackground $NFG \
		-activeforeground $NBG

	pack $ql.ctl $ql.ctl.manual $ql.ctl.tutorial $ql.ctl.imanual -side right -fill x
	pack $ql $ql.ctl -side top -fill x

	set h_l [text $ql.lb -undo 1 -width 300 -highlightthickness 0 -font $HV1]

	$wl setwidget $h_l

	pack $wl -side left -fill both ;#-expand yes
	pack $xx -side top  -fill both -expand yes
	pack $pw -fill both -expand yes
}

proc add_file {} {
	global qfd

	set x [tk_getOpenFile]
	if {$x == ""} { return }

	if [catch {set fd [open $x r]} err] {
		i_error $err"
		return
	}
	catch { close $fd }

	put_files "$x"
	put_hist " a $x"
	update
}

proc flip_bw {} {
	global bw_mode r_log
	global CW1 HV1 HV2

	if {$bw_mode == 1} {
		set bw_mode 0
		$r_log configure -fg black -bg white -font $CW1
	} else {
		set bw_mode 1
		$r_log configure -fg white -bg black -font $HV2
	}
}

set fwhat 1

proc find_it {} {
	global fwhat r_log noupdate dflt

	r_clear
		 .ff.a.f configure -fg gold
		 .ff.a.c configure -fg gold
		 .ff.a.t configure -fg gold
		 .ff.a.m configure -fg gold
		 .ff.a.s configure -fg gold
		 .ff.a.v configure -fg gold
	update
	set noupdate 1
	switch $fwhat {
	1	{	;# fct defs
		 handle_command "r; ff [.ff.b.e get]" "find_it"
		 .ff.a.f configure -fg white
		}
	2	{	;# fct calls
		 handle_command "r; pe [.ff.b.e get] ( .* ) \\;" "find_it"
		 handle_command "$dflt" "find_it"
		 .ff.a.c configure -fg white
		}		
	3	{	;# typedefs
		 handle_command "r; pe typedef ^\\;* [.ff.b.e get]" "find_it"
		 handle_command "$dflt" "find_it"
		 .ff.a.t configure -fg white
		}
	4	{	;# macro defs
		 handle_command "r; m /define [.ff.b.e get]" "find_it"
		 handle_command "$dflt" "find_it"
		 .ff.a.m configure -fg white
		}
	5	{	;# struct/union defs
		 handle_command "r; pe \[struct union\] ^\[\\; \{\]* [.ff.b.e get] { .* }" "find_it"
		 handle_command "$dflt" "find_it"
		 .ff.a.s configure -fg white
		}
	6	{	;# variable declarations
		 handle_command "r; pe @type ^\[\\; ( ) { } @type\]* [.ff.b.e get]" "find_it"
		 handle_command "$dflt" "find_it"
		 .ff.a.v configure -fg white
		}
	default	{
		 i_error "bad switch command, find_it";
		}
	}
	set noupdate 0
	update
	$r_log yview 0
	update
}

proc find_stuff {} {
	global TFG TBG LTG NBG NFG MFG
	global HV0 HV1 HV2 fwhat bw_mode
	global Wfind Hfind Xfind Yfind

	if [winfo exists .ff] { return; }

	toplevel .ff
	wm title .ff "Find.."
	wm iconname .ff "Find"
	wm geometry .ff ${Wfind}x$Hfind+${Xfind}+$Yfind

	frame .ff.a
	frame .ff.b

	label .ff.a.nm -text "(C) Type:" -width 12 -bg $NBG -fg $NFG -font $HV1
	radiobutton .ff.a.f -text "Function Def" \
		-variable fwhat -value 1 -fg $NFG -bg $MFG -font $HV1 -selectcolor green
	radiobutton .ff.a.c -text "Function Calls" \
		-variable fwhat -value 2 -fg $NFG -bg $MFG -font $HV1 -selectcolor green
	radiobutton .ff.a.t -text "Typedef" \
		-variable fwhat -value 3 -fg $NFG -bg $MFG -font $HV1 -selectcolor green
	radiobutton .ff.a.m -text "Macro Def" \
		-variable fwhat -value 4 -fg $NFG -bg $MFG -font $HV1 -selectcolor green
	radiobutton .ff.a.s -text "Struct/Union Def" \
		-variable fwhat -value 5 -fg $NFG -bg $MFG -font $HV1 -selectcolor green
	radiobutton .ff.a.v -text "Var Decl" \
		-variable fwhat -value 6 -fg $NFG -bg $MFG -font $HV1 -selectcolor green

	pack .ff.a.nm -side left -fill y -expand no -anchor w
	pack .ff.a.f -side left -fill both -expand yes
	pack .ff.a.c -side left -fill both -expand yes
	pack .ff.a.t -side left -fill both -expand yes
	pack .ff.a.m -side left -fill both -expand yes
	pack .ff.a.s -side left -fill both -expand yes
	pack .ff.a.v -side left -fill both -expand yes

	label .ff.b.n -text "Name:" -width 12 -bg $NBG -fg $NFG -font $HV1
	if {$bw_mode == 0} {
	 entry .ff.b.e -relief sunken -width 16 -bg white -fg black -font $HV1 -takefocus 1
	} else {
	 entry .ff.b.e -relief sunken -width 16 -bg black -fg white -font $HV1 -takefocus 1
	}
	pack .ff.b .ff.b.n -side left -fill y -expand no
	pack .ff.b .ff.b.e -side left -fill both -expand yes

	pack append .ff .ff.a {top fillx filly expand}
	pack append .ff .ff.b {top fillx filly expand}

	bind .ff.b.e <Return> "find_it"
}

proc create_panels {} {
	global MFG MBG
	global NBG NFG
	global TFG TBG
	global iversion
	global version
	global HV0 HV1
	global LOGO
	global d_mode
	global pane

	frame .menubar -bg $MFG
	pack append . .menubar {top frame w fillx}

	wm title . "Cobra $version -- $iversion"

	menubutton .menubar.file -text "File.." \
		-bg $MFG -fg $NFG -font $HV1 \
		-relief raised -menu .menubar.file.m

	menu .menubar.file.m
	 .menubar.file.m add command -label "Add new source file.." -command "add_file"
	 .menubar.file.m add command -label "Save results.." -command "save_results"
	 .menubar.file.m add command -label "Save new patterns.." -command "save_pats"
	 .menubar.file.m add command -label "Save history.." -command "save_hist"
	 .menubar.file.m add command -label "Exit" -command "exit"

	pack append .menubar .menubar.file {left frame w}

	button .menubar.find -text "Find.." \
		-bg $MFG -fg $NFG -font $HV1 \
		-relief raised -command "find_stuff"
	pack append .menubar .menubar.find {left}

	menubutton .menubar.view -text "View.." \
		-bg $MFG -fg $NFG -font $HV1 \
		-relief raised -menu .menubar.view.m
	menu .menubar.view.m
	 .menubar.view.m add command -label "Tokens"  -command "changeformat 0"
	 .menubar.view.m add command -label "Context" -command "changeformat 1"
	 .menubar.view.m add command -label "Marked"  -command "changeformat 2"
	 .menubar.view.m add command -label "JSON"    -command "changeformat 3"
	pack append .menubar .menubar.view {left frame w}

	menubutton .menubar.options -text "Options.." \
		-bg $MFG -fg $NFG -font $HV1 \
		-relief raised -menu .menubar.options.m
	menu .menubar.options.m
	 .menubar.options.m add checkbutton -label "Display Line Numbers" -variable linenumbers -onvalue 1 -offvalue 0
	 .menubar.options.m add command -label "Invert B/W" -command "flip_bw"
	pack append .menubar .menubar.options {left frame w}

	menubutton .menubar.sets -text "Pattern Sets.." \
		-bg $MFG -fg $NFG -font $HV1 \
		-relief raised -menu .menubar.sets.m
	menu .menubar.sets.m
	 .menubar.sets.m add command -label "Show Predefined Checks" -command "predefined_list"
	 .menubar.sets.m add command -label "Read patterns from file.." -command "load_pats"
	 .menubar.sets.m add command -label "Define New Pattern Sets" -command "smart_panel"
	 .menubar.sets.m add command -label "Apply Operations on Sets" -command "set_ops"
	pack append .menubar .menubar.sets {left frame w}

	label .menubar.title -text "Cobra $version" -fg $NFG -bg $MFG -font $HV1
	pack append .menubar .menubar.title {left frame c expand}

	label .menubar.mode -text "ViewMode: Context" -bg $MFG -fg white -font $HV1
	pack append .menubar .menubar.mode {right frame e}

#	# optional logo
#	catch {
#		set image [image create photo -file $LOGO ]
#		label .menubar.pic -image $image
#		pack append .menubar .menubar.pic {left}
#		update
#	}

	set pane .f
	set nb [NoteBook $pane \
		-bg $NBG -fg $NFG \
		-font $HV0 \
		-activebackground $NFG \
		-activeforeground $NBG \
		-side top]
	pack $pane -fill both -expand yes

	# create tabs on the notebook pane
	set mp [$nb insert end Mp -text " matches "  -raisecmd "$pane raise Mp"]
	set db [$nb insert end Db -text " metrics "  -raisecmd "$pane raise Db; dashboard"]
	set hm [$nb insert end Hm -text " heatmap "  -raisecmd "$pane raise Hm; heatmap"]
	set rh [$nb insert end Rh -text " warnings " -raisecmd "$pane raise Rh; warnings"]
	set hp [$nb insert end Hp -text " help "     -raisecmd "$pane raise Hp; helper"]

	main_panels $mp
	help_panels $hp
	metrics_canvas $db
	heatmap_canvas $hm
	warnings_canvas $rh	;# reverse heatmap, categories sorted by nr warnings

	$pane raise Mp	;# default view
	# cannot start with Hp, because no files are loaded yet
}

proc get_ready {} {
	global qfd verbose

	while (1) {
		gets $qfd line
		if {[string first "Ready: " $line] >= 0} {
			break
		}
		flush $qfd
	}
}

proc i_error {s} {
	tk_messageBox -icon error \
		-message "$s"
}

proc patterns_title {nr pn tl} {
	global pmap ptitle verbose Wpatternmatch

	if {$verbose} { puts "set title of $nr named $pn to $tl" }

	if {$nr < 0} { return; }

	array set ptitle "$pn \"$tl\""

	if {$verbose} { puts "set ptitle $pn to $tl" }

	catch {
	  if {[winfo exists .patterns.main.c.frWidgets.b$nr]} {
		.patterns.main.c.frWidgets.b$nr configure \
			-text "$pn ($pmap($pn) patterns) :: $tl" \
			-width [expr $Wpatternmatch / 8]
	  } elseif {$verbose} {	;# should this happen?
		puts "error: no entry for pattern $nr named $pn with $pmap($pn) patterns"
	  }
	} emsg
#	if {"$emsg" != ""} {
#		i_error "patterns_title: $emsg"
#	}
	update
}

proc patterns_panel {nrm pn} {
	global NBG NFG HV1 nrpat
	global pmap Yoffset pnm2nr
	global verbose ptitle
	global Wpatternmatch Hpatternmatch Xpatternmatch Ypatternmatch

	# https://stackoverflow.com/questions/39956549/how-to-add-a-scrollbar-to-tcl-frame
	# puts "XXX Patterns Panel $nrm $pn"

	catch {
		if {![winfo exists .patterns]} {
			set nrpat 0
			toplevel .patterns
			wm title .patterns "patterns"
			wm iconname .patterns "pat"
			wm geometry .patterns ${Wpatternmatch}x${Hpatternmatch}+${Xpatternmatch}+$Ypatternmatch

			ttk::frame .patterns.main
			canvas .patterns.main.c \
				-xscrollcommand ".patterns.main.xscroll set" \
				-yscrollcommand ".patterns.main.yscroll set"
			ttk::scrollbar .patterns.main.xscroll \
				-orient horizontal -command ".patterns.main.c xview"
			ttk::scrollbar .patterns.main.yscroll -command ".patterns.main.c yview"
			pack .patterns.main.xscroll -side bottom -fill x
			pack .patterns.main.yscroll -side right -fill y
			pack .patterns.main.c -expand yes -fill both -side top
			ttk::frame .patterns.main.c.frWidgets -borderwidth 1 -relief solid -width 340 -height 700
	}	}

	catch {
		if {$nrm == $pmap($pn)} {
			return 0	;# returns value from catch
		}
	} emsg

	if {"$emsg" == "0" && [winfo exists .patterns.main.c.frWidgets] } {
		for {set tval 0} {$tval <= $nrpat} {incr tval} {
			if {$tval == $pnm2nr($pn)} {
				return		;# entry exists
	}	}	}

	catch {
		incr nrpat
		set nh [expr {40*$nrpat}]

		# puts "create .patterns.main.c.frWidgets.b$nrpat -- $pn"

		ttk::style configure My.TButton \
			-font $HV1 \
			-foreground gold -background darkblue \
			-justify left
		ttk::button .patterns.main.c.frWidgets.b$nrpat \
			-style My.TButton -text "$pn ($nrm patterns)" \
			-command "pat_show $pn $nrm $nrpat"

		pack .patterns.main.c.frWidgets .patterns.main.c.frWidgets.b$nrpat \
			-anchor w \
			-side top -fill x -expand yes

		.patterns.main.c create window 0 0 -anchor nw -window .patterns.main.c.frWidgets
		.patterns.main.c configure -scrollregion [.patterns.main.c bbox all]
		pack .patterns.main -expand yes -fill both -side top

		if {$nrpat < 14} {
		 wm geometry .patterns ${Wpatternmatch}x$nh+${Xpatternmatch}+$Ypatternmatch
		}

		array set pmap "$pn $nrm"
		array set pnm2nr "$pn $nrpat"
		if {[info exists ptitle($pn)]} {
		  .patterns.main.c.frWidgets.b$nrpat configure \
			-text "$pn ($nrm patterns) :: $ptitle($pn)"
		}
	} emsg

	if {"$emsg" != ""} {
		i_error "error: patterns panel $emsg"
	}
}

proc do_detail {pn nr} {

	do_button "dp $pn $nr"
	catch { .p_detail.f.b$nr configure -bg lightgreen }
	.f raise Mp
}

proc do_get_detail {pn} {
	r_clear
	set nr [.p_detail.f.e get]
	do_detail $pn $nr
}

proc pattern_detail {pn nr} {
	global pmap ptitle LTG TFG NBG NFG
	global Yoffset verbose HV1
	global Wpatterndetail Hpatterndetail
	global Ypatterndetail Xpatterndetail

	if {[winfo exists .p_detail]} {
		destroy .p_detail
		update
	}
	toplevel .p_detail
	wm title .p_detail "Detail $pn"
	wm iconname .p_detail "detail"

	set upto $pmap($pn)
	set nrrows [expr {1 + $upto / 8}]

	set cnt $nrrows
	set nh [expr {45*$nrrows}]
	wm geometry .p_detail ${Wpatterndetail}x$nh+$Xpatterndetail+$Ypatterndetail

	catch {
		# if the pattern set was deleted, this fails
		label .p_detail.t -text "$ptitle($pn)" -bg $NBG -fg $NFG -font $HV1
		pack .p_detail.t -side top -anchor w -fill both -expand yes
		# in case we didnt succeed in filling in the full title before
		if {$verbose} { puts "label .patterns.main.c.frWidgets.b$nr -- $pn" }
		.patterns.main.c.frWidgets.b$nr configure \
			-text "$pn ($pmap($pn) patterns) :: $ptitle($pn)"
	} emsg
	if {"$emsg" != ""} {
	#	puts "set $pn was deleted or merged with another set $pn : $emsg"
		destroy .p_detail
		if {$verbose} { puts "destroy .patterns.main.c.frWidgets.b$nr -- $pn" }
		destroy .patterns.main.c.frWidgets.b$nr
		return
	}

	frame .p_detail.f
	pack .p_detail.f -side top -anchor w

	if {$upto < 100} {
		set nr 1
		set row 1	;# populate the rows
		while {$nr <= $upto} {
			set mnr [expr {$nr - 1}]
			if {[expr {$mnr % 8}] == 0} {
				incr row
			}
			button .p_detail.f.b$nr -text "$nr" -bg white -width 1 \
				-command "do_detail $pn $nr"
			grid .p_detail.f.b$nr -row $row -column [expr {$mnr % 8}]
			incr nr
		}
		grid rowconfigure .p_detail.f "all" -uniform allTheSame
		grid columnconfigure .p_detail.f "all" -uniform allTheSame
	} else {
		wm geometry .p_detail ${Wpatterndetail}x40+$Xpatterndetail+$Ypatterndetail
		label .p_detail.f.lb -text "Show Match Nr 1 .. $upto: " -bg black -fg white -font $HV1
		entry .p_detail.f.e -relief sunken -width 16 -bg white -fg black
		pack .p_detail.f .p_detail.f.lb -side left -fill both -expand no
		pack .p_detail.f .p_detail.f.e  -side left -fill both -expand yes
		bind .p_detail.f.e <Return> "do_get_detail $pn"
	}
}

proc pat_show {pn nrm nrpat} {
	global verbose

	if {$verbose} { puts "$pn $nrm $nrpat" }

	# handle_command "dp $pn" "pat_show"
	pattern_detail $pn $nrpat
}

proc predefined_list {} {
	global CBG CFG HV2 COBRA qfd
	global Yoffset WMain Wpredefined Hpredefined
	global Xpredefined Ypredefined

	toplevel .predef
	wm title .predef "Predefined Checks"
	wm iconname .predef "predef"
	wm geometry .predef ${Wpredefined}x$Hpredefined+$Xpredefined+$Ypredefined

	if [winfo exists .predef.main] { return; }

	frame .predef.main

	if [catch { set lfd [open "|$COBRA -list" r+]} errmsg] {
		puts $errmsg
		exit 1
	}

	set nr 0
	while {[gets $lfd line] > -1} {
		if {[string first "predefined" $line] < 0} {
			button .predef.main.lst$nr -height 1 -text "$line" \
				-highlightthickness 3 \
				-bg $CBG -fg $CFG -font $HV2 \
				-command "do_query \"$line\""
			pack .predef.main .predef.main.lst$nr \
				-side top -fill both -expand yes
			incr nr
	}	}

	catch "close $lfd"
}

proc smart_action {} {
	global pnm2nr nrpat pn2pe

	set sn [ .smart.c.mark get ]	;# setname
	set pn [ .smart.b.mark get ]	;# pattern
	set tl [ .smart.d.mark get ]	;# description/title

	if {"$sn" == ""} {
		i_error "error: the new pattern set must have a name"
		return
	}
	if {"$pn" == ""} {
		i_error "error: define a search pattern for set $sn"
		return
	}
	if {$tl == ""} {
		set tl $sn
	}

	# delete older patterns entries with the same name $sn
	catch {
		set n $pnm2nr($sn)
		if {[winfo exists .patterns.main.b$n]} {
			destroy .patterns.main.b$n
	}	}

	array set pn2pe "$sn \"$pn\""
	
	handle_command "pe ${sn}: $pn" "smart"
	handle_command "ps caption $sn $tl" "smart"
	handle_command "ps list" "smart"
}

set setop 2	;# set Union by default

proc do_setop {} {
	global setop

	set D [.sets.a.en get]
	set L [.sets.b.en get]
	set R [.sets.c.en get]

	if {"$D" == ""} { i_error "specify a target set name"; return; }
	if {"$L" == ""} { i_error "specify set name for lhs"; return; }
	if {"$R" == ""} { i_error "specify set name for rhs"; return; }

	switch $setop {
	1	{ set op "&" }
	2	{ set op "+" }
	3	{ set op "-" }
	default	{ i_error "unknow set operation $setop"
		  return
		}
	}
	handle_command "ps $D = $L $op $R" "do_setop"
	handle_command "ps caption $D $L$R$setop" "do_setop"
	handle_command "dp $D" "do_setup"
	handle_command "ps list" "do_setop"
}

proc set_ops {} {
	global Wpatternsets Hpatternsets Xpatternsets Ypatternsets
	global setop HV0 HV1 NFG MFG NBG Xpatternmatch Ypatternmatch

	if [winfo exists .sets] { return; }

	toplevel .sets
	wm title .sets "Set Operations"
	wm iconname .sets "set ops"
	wm geometry .sets ${Wpatternsets}x$Hpatternsets+${Xpatternsets}+$Ypatternsets

	frame .sets.a
	frame .sets.b
	frame .sets.c
	frame .sets.d

	label .sets.a.nm -width 16 -text "target setname:" -bg $NBG -fg $NFG -font $HV1
	entry .sets.a.en -relief sunken -width 16 -bg white -fg black
	pack append .sets.a .sets.a.nm {left frame w fillx filly}
	pack append .sets.a .sets.a.en {left fillx filly expand}

	label .sets.b.nm -width 16 -text "source setname (lhs):" -bg $NBG -fg $NFG -font $HV1
	entry .sets.b.en -relief sunken -width 16 -bg white -fg black
	pack append .sets.b .sets.b.nm {left frame w fillx filly}
	pack append .sets.b .sets.b.en {left frame w fillx filly expand}

	label .sets.d.nm -width 16 -text "operation:" -bg $NBG -fg $NFG -font $HV1

	radiobutton .sets.d.o1 -text "Intersect (&)" \
		-variable setop -value 1 -fg $NFG -bg $MFG -font $HV1 -selectcolor green
	radiobutton .sets.d.o2 -text "Union (+)" \
		-variable setop -value 2 -fg $NFG -bg $MFG -font $HV1 -selectcolor green
	radiobutton .sets.d.o3 -text "Subtract (-)" \
		-variable setop -value 3 -fg $NFG -bg $MFG -font $HV1 -selectcolor green
	pack .sets.d.nm -side left -fill both -expand no -anchor w
	pack .sets.d.o1 -side left -fill both -expand yes
	pack .sets.d.o2 -side left -fill both -expand yes
	pack .sets.d.o3 -side left -fill both -expand yes

	label .sets.c.nm -width 16 -text "source setname (rhs):" -bg $NBG -fg $NFG -font $HV1
	entry .sets.c.en -relief sunken -width 16 -bg white -fg black
	pack append .sets.c .sets.c.nm {left frame w fillx filly}
	pack append .sets.c .sets.c.en {left frame w fillx filly expand}

	pack append .sets .sets.d {top frame w fillx filly expand}
	pack append .sets .sets.b {top frame w fillx filly expand}
	pack append .sets .sets.c {top frame w fillx filly expand}
	pack append .sets .sets.a {top frame w fillx filly expand}

	bind .sets.a.en <Return> "do_setop"
	bind .sets.b.en <Return> "do_setop"
	bind .sets.c.en <Return> "do_setop"

	# move patternmatch down by $Hpatternsets
	if [winfo exists .patterns] {
		set Ypatternmatch [expr ${Ypatternmatch} + $Hpatternsets + 20 + 20]
		wm geometry .patterns +$Xpatternmatch+$Ypatternmatch
	}
}

proc smart_panel {} {
	global TFG TBG NBG NFG LTG HV0
	global d_mode Yoffset WMain YMain
	global Wpredefined Hpredefined
	global Wpatternsearch Hpatternsearch
	global Xpatternsearch Ypatternsearch

	if [winfo exists .smart] { return; }

	toplevel .smart
	wm title .smart "Pattern Search"
	wm iconname .smart "patsearch"
	wm geometry .smart ${Wpatternsearch}x${Hpatternsearch}+${Xpatternsearch}+$Ypatternsearch

	frame .smart.b
	frame .smart.c
	frame .smart.d

	label .smart.c.y -width 12 -text "new setname:" -bg $NBG -fg $NFG -font $HV0
	entry .smart.c.mark -relief sunken -width 16 -bg white -fg black
	pack .smart.c .smart.c.mark -side right -fill both -expand yes
	pack .smart.c .smart.c.y    -side left  -fill both -expand no

	label .smart.d.y -width 12 -text "description:" -bg $NBG -fg $NFG -font $HV0
	entry .smart.d.mark -relief sunken -width 16 -bg white -fg black
	pack .smart.d .smart.d.mark -side right -fill both -expand yes
	pack .smart.d .smart.d.y    -side left  -fill both -expand no

	label .smart.b.y -width 12 -text "search pattern:" -bg $NBG -fg $NFG -font $HV0
	entry .smart.b.mark -relief sunken -width 16 -bg white -fg black
	pack .smart.b .smart.b.y    -side left  -fill both -expand no
	pack .smart.b .smart.b.mark -side right -fill both -expand yes

	pack append .smart .smart.c {top fillx filly expand}
	pack append .smart .smart.d {top fillx filly expand}
	pack append .smart .smart.b {top fillx filly expand}

	bind .smart.b.mark <Return> "smart_action"
	bind .smart.c.mark <Return> "smart_action"
	bind .smart.d.mark <Return> "smart_action"
}

proc do_query {which} {

#	puts "here: $which"
	do_button ". $which; r"
}

#### Startup
	create_panels

	set cargs "-gui"
	set fargs {}

	if {$argc >= 1} {
		set ix [lsearch $argv "--verbose"]	;# icobra flag
		if {$ix >= 0} {
			set verbose 1
			set argv [lreplace $argv $ix $ix]
			incr argc -1
		}
		set skipnext 0
		foreach x $argv {			;# check valid main cobra flags
			if {$skipnext} {
				puts "icobra: unhandled option: $y $x"
				set skipnext 0
				incr argc -1
			} else {
				set ix [string match {\-[a-zA-Z]*} $x]
				if {$ix > 0} {
					switch $x {
					"-e"		-
					"-f"		-
					"-F"		-
					"-pe"		-
					"-pattern"	-
					"-recursive"	-
					"-regex"	-
					"-seed"		-
					"-stream"	-
					"-stream_margin"	-
					"-view"		-
					"-e"		-
					"-var"		-
					"-c"	{
						  set skipnext 1
						  set y $x
						}
					default {
						set cargs "$cargs $x"
						}
					}
					incr argc -1
				} else {
					lappend fargs $x
	}	}	}	}
	set argv $fargs

	# puts "verbose: $verbose"
	# puts "argc: $argc"
	# puts "args: $cargs"
	# puts "file: $argv"

	# needs at least one file name, to run in non-streaming mode
	if {$argc == 0} {
		puts "usage: icobra filenames..."
		exit 1
	}


	if [catch {set qfd [open "|$COBRA $cargs" r+]} errmsg] {
		puts $errmsg
		exit 1
	}

	predefined_list
	update
	get_ready
	update

	if {$argc >= 1} {
		foreach x $argv {
			put_files "$x"
			put_hist "a $x"
			puts $qfd "a $x"
			flush $qfd
			get_ready
			update
	}	}

	$c_log insert end "commands:\n"
	$c_log yview -pickplace end
#	$f_log configure -width 250

	focus $c_log
	update
