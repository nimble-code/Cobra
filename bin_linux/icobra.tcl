#!/bin/sh
# The next line is executed by /bin/sh, but not tcl \
exec wish "$0" -- $*

## Cobra GUI -- gjh @ LaRS
## (c) 2015 All Rights Reserved

wm title . "icobra"
wm geometry . 1500x800+20+20	;# widthxheight+offsetx+offsety

set xversion "Cobra GUI -- 2022.03.31";	# was 2015.10.16
set version  "Cobra Version unknown"; # updated below
set Unix 1;                           # updated below
set COBRA cobra;                      # background tool
set LOGO "/cygdrive/f/Dropbox_orig/Tools/Cobra/GUI/cobra_small.gif"
set d_mode 1;			      # src display mode
set nrpat 0

### Tools
	## check if we have the right version
	if {[auto_execok $COBRA] == "" \
	||  [auto_execok $COBRA] == 0} {
		puts "No executable $COBRA found..."
		exit 0
	} else {
		catch { set fd [open "|$COBRA -V" r] } errmsg
		if {$fd == -1} {
			puts "$errmsg"
			exit 0
		} else {
			if {[gets $fd line] > -1} {
				set version "$line"
			}
			catch "close $fd"
		}
		if {[string first "Version "  $version] < 0 } {
			puts "this Cobra GUI requires Cobra Version 0.0 or later"
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
set nrm 0	;# nr matches
set nrr 1	;# line in r_log

proc dothis {n} {
	global h_log c_log nrc

	set x [$h_log get $n.0 $n.end]

	$c_log insert end "$x"
	handle_command $x "dothis"
	$c_log insert end "\n"
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

proc writeoutfile {to} {
	global h_log

	if ![file_ok $to] { return 0 }

	if [catch {set fd [open $to w]} errmsg] {
		add_log $errmsg 0
		return 0
	}
	fconfigure $fd -translation lf	;# no cr at end of line, just lf

	scan [$h_log index end] %d numLines
	for {set i 2} {$i < $numLines} {incr i} {
		set line [$h_log get $i.0 $i.end]
		if {[string length $line] > 0} {
			puts $fd $line
	}	}
	close $fd

	put_result "<saved $to>"

	return 1
}

proc save_hist {} {

	set f [tk_getSaveFile]
	if {$f != ""} { writeoutfile $f }
}

proc reverse {n} {
	global h_log
	$h_log tag configure hist$n -foreground black -background white
}
proc normal {n} {
	global h_log
	$h_log tag configure hist$n -foreground white -background black
}

set ftags(0) {}
set fnames {}

proc tagfile {fn n} {
	global ftags fnames

	lappend ftags($fn) $n

	set idx [lsearch $fnames $fn]
	if { $idx == -1 } {
		lappend fnames $fn
	}
}

proc showfile {fn n} {
	global ftags c_log nrc

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
		wm title .f$f "$f:$n"
		wm iconname .f$f "$f"
	
		text .f$f.t -relief raised -bd 2 \
			-width 120 -height 24 \
			-setgrid 1
		pack append .f$f .f$f.t { top expand fillx }

		set fd -1
		catch { set fd [open $fn r] } errmsg
		if {$fd < 0} {
			$c_log insert end "$errmsg\n"
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
	foreach x $ftags($fn) {
		.f$f.t tag configure sel$x -foreground red -background white
	}
	.f$f.t yview -pickplace [expr $n - 12]
}

proc put_result {s} {
	global nrr r_log

	incr nrr
	$r_log insert end "$s\n"
	$r_log yview -pickplace end

	set x [scan $s "%d\: %\[a-zA-Z_\./\]%\[:\]%d%\[:\] %s" a fnm c lnr e f]
	if {$x >= 5} {
		$r_log tag add  hist$nrr $nrr.0 $nrr.end
		$r_log tag bind hist$nrr <Any-Enter> "reverse $nrr"
		$r_log tag bind hist$nrr <Any-Leave> "normal  $nrr"
		$r_log tag bind hist$nrr <ButtonPress-1> "showfile $fnm $lnr"
		tagfile $fnm $lnr	;# prep
	}
}

proc put_hist {s} {
	global h_log nrh

	incr nrh
	$h_log insert end "$s\n"
	$h_log yview -pickplace end
	
	$h_log tag add  hist$nrh $nrh.0 $nrh.end
	$h_log tag bind hist$nrh <Any-Enter> "reverse $nrh"
	$h_log tag bind hist$nrh <Any-Leave> "normal  $nrh"
	$h_log tag bind hist$nrh <ButtonPress-1> "dothis $nrh"
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
	global c_log
	global qfd nrm nrc

puts "process_line $line"

	if {[string first "wrote: " $line] == 0} {
		put_result "$line"
	} elseif {[string first "view with: " $line] == 0} {
		put_hist "!dot -Tx11 cobra.dot &"
	} elseif {[string first " matches" $line] > 0} {
		set pn ""
		if {[string first " stored in " $line] > 0} {
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
	} elseif {[string first "title " $line] >= 0} {
		scan $line "%d title %s = %s" n a b
		set x [string first " = " $line]
		if {$x > 0} {
			incr x 3
			set b [string range $line $x end]
			patterns_title $n $a "$b"
		}
	} else {
		put_result "$line"
	}
}

proc handle_command {cmd who} {
	global c_log dflt
	global qfd nrm d_mode

puts "handle command $cmd -- $who"

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
		update
		catch {
			puts $qfd "$cmd"
			flush $qfd
			while {[gets $qfd line] >= 0} {
				if {[string first "Ready: " $line] >= 0} {
					break
				}
				process_line $line
				update
			}
			update
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
	update
}

proc launchBrowser {url} {
	global tcl_platform

	if {$tcl_platform(platform) eq "windows"} {
		set command [list {*}[auto_execok start] {}]
		set url [string map {& ^&} $url]
	} elseif {$tcl_platform(os) eq "Darwin"} {
 		set command [list open]
	} else {
		set command [list xdg-open]
	}
	exec {*}$command $url &
}

proc Manual {} {
	launchBrowser "file:///home/gh/Dropbox/Tools/Cobra/cobra/doc/manual.html"
}

proc Tutorial {} {
	launchBrowser "file:///home/gh/Dropbox/Tools/Cobra/cobra/doc/tutorial.html"
}

proc dobutton {x} {
	global c_log nrc

	$c_log insert end "$x"
	incr nrc
	handle_command "$x" "dobutton"
	$c_log insert end "\n"
	$c_log yview -pickplace end
}

proc shortcut {a b c d} {
	global NBG NFG HV1

	button $a.$b -text $d -command "dobutton $c" \
		-bg $NBG -fg $NFG \
		-font $HV1 \
		-activebackground $NFG \
		-activeforeground $NBG
	pack $a $a.$b -side right -fill both -expand yes
}

proc changeformat {} {
	global dflt d_mode
	r_clear

	if {$d_mode == 0} { set dflt "l"; handle_command "mode tok" "changeformat" }
	if {$d_mode == 1} { set dflt "d"; handle_command "mode src" "changeformat" }
	if {$d_mode == 2} { set dflt "p"; handle_command "mode pre" "changeformat" }
}

proc displaymode {a b t} {
	global d_mode dflt
	global NBG NFG HV2

	radiobutton $a.d$b -text $t -variable d_mode -value $b -width 8 \
		-bg darkblue -fg gold \
		-selectcolor black \
		-font $HV2 \
		-indicatoron true \
		-activebackground $NFG \
		-activeforeground $NBG \
		-command "changeformat"
	pack $a $a.d$b -side left -fill both -expand yes
}

proc main_panels {t} {
	global c_log r_log h_log
	global HV0 HV1 HV2 CW1
	global CBG CFG
	global TBG TFG
	global NFG NBG
	global nr d_mode

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
	$r_log insert end "matches:\n"
	$r_log edit modified false

  # cobra command window, bottom
	set p1     [$pw add -minsize 20]	;# command window, bottom
        set sw11   [ScrolledWindow $p1.sw]
        set c_log   [text $sw11.lb -height 10 \
		-highlightthickness 3 -bg $CBG -fg $CFG -font $HV2]
	$sw11 setwidget $c_log
	pack $sw11 -fill both -expand yes

	bind $c_log <Return> {
		incr nrc
		handle_command [$c_log get $nrc.0 $nrc.end] "main"
	}

  # history window, right
	set cv [ScrolledWindow $q1.wide]
	set h_log [text $cv.right -width 250 \
		-takefocus 0 \
		-bg $CBG -fg $CFG -font $HV1]
	$cv setwidget $h_log

	frame $q1.ctl -bg $NBG

	button $q1.ctl.save -text "save history..." -command "save_hist" \
		-bg $NBG -fg $NFG -font $HV0 \
		-activebackground $NFG -activeforeground $NBG

	pack $q1.ctl $q1.ctl.save -side right -fill x -expand no
	pack $q1 $q1.ctl -side top

	pack $cv -side right -fill both -expand yes
	pack $pw -fill both -expand yes

	$h_log insert end "history:\n"
}

proc helper {} {
	global qfd h_l

	puts $qfd "?"
	flush $qfd
	while {[gets $qfd line] >= 0} {
		if {[string first "Ready: " $line] >= 0} {
			break
		}
		$h_l insert end "	$line\n"
	}
	update
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
	button $ql.ctl.manual -text " Show Manual in Browser " \
		-command Manual \
		-bg $NBG -fg $NFG \
		-activebackground $NFG \
		-activeforeground $NBG

	button $ql.ctl.tutorial -text " Show Tutorial in Browser " \
		-command Tutorial \
		-bg $NBG -fg $NFG \
		-activebackground $NFG \
		-activeforeground $NBG

	pack $ql.ctl $ql.ctl.manual $ql.ctl.tutorial -side right -fill x
	pack $ql $ql.ctl -side top -fill x

	set h_l [text $ql.lb -undo 1 -width 300 -highlightthickness 0 -font $HV1]

	$wl setwidget $h_l

	pack $wl -side left -fill both ;#-expand yes
	pack $xx -side top  -fill both -expand yes
	pack $pw -fill both -expand yes
}

proc create_panels {} {
	global MFG MBG
	global NBG NFG
	global version
	global HV0 HV1
	global LOGO

	frame .menubar -bg $MFG
	pack append . .menubar {top frame w fillx}

	label  .menubar.title -text "  Cobra $version" -bg $MFG -fg $MBG -font $HV1
	button .menubar.q -text "quit" -bd 0 -bg $NBG -fg $NFG -font $HV1 -command "exit"

	frame .menubar.a
		displaymode .menubar.a 0 "token"
		displaymode .menubar.a 1 "source"
		displaymode .menubar.a 2 "marked"

	# logo - if available
	catch {
		set image [image create photo -file $LOGO ]
		label .menubar.pic -image $image
		pack append .menubar .menubar.pic {left}
		update
	}
	pack append .menubar .menubar.title {left}
	pack append .menubar .menubar.q {right}
	pack append .menubar .menubar.a {right padx 100}


	set pane .f
	set nb [NoteBook $pane \
		-bg $NBG -fg $NFG \
		-font $HV0 \
		-activebackground $NFG \
		-activeforeground $NBG \
		-side top]
	pack $pane -fill both -expand yes

	# create tabs on the notebook pane
	set mp [$nb insert end Mp -text " matches " -raisecmd "$pane raise Mp"]
	set hp [$nb insert end Hp -text " help " -raisecmd "$pane raise Hp; helper"]

	main_panels $mp
	help_panels $hp

	$pane raise Mp	;# default view
}

proc get_ready {} {
	global qfd

	while (1) {
		gets $qfd line
		if {[string first "Ready: " $line] >= 0} {
			break
		}
		flush $qfd
	}
}

proc patterns_title {nr pn tl} {
	global pmap ptitle

	puts "set title of $nr named $pn to $tl"
	if {$nr < 0} { return; }

	array set ptitle "$pn \"$tl\""
	puts "set ptitle $pn to $tl"
	update
	catch {
	  if {[winfo exists .patterns.main.b$nr]} {
		.patterns.main.b$nr configure -anchor w -text "$pn ($pmap($pn) patterns) :: $tl"
	  } else {
		puts "error: no entry for pattern $nr named $pn with $pmap($pn) patterns"
	  }
	} emsg
	if {"$emsg" != ""} {
		puts "patterns_title error: $emsg"
	}
}

set Yoffset 220

proc patterns_panel {nrm pn} {
	global NBG NFG HV1 nrpat
	global pmap Yoffset
	puts "new pattern set $pn has $nrm matches"

	catch {
		if {![winfo exists .patterns]} {
			toplevel .patterns
			wm title .patterns "patterns"
			wm iconname .patterns "pat"
			wm geometry .patterns 300x150+1830+$Yoffset
			frame .patterns.main
		}
		incr nrpat
		set nh [expr {40*$nrpat}]
		button .patterns.main.b$nrpat -text "$pn ($nrm patterns)" \
			-bd 0 -bg $NBG -fg $NFG -font $HV1 \
			-command "pat_show $pn $nrm $nrpat"
		wm geometry .patterns 300x$nh+1830+$Yoffset
		pack .patterns.main .patterns.main.b$nrpat \
			-side top -fill both -expand yes
		array set pmap "$pn $nrm"
	} emsg

	if {"$emsg" != ""} {
		puts "error: patterns panel $emsg"
	}
}

proc do_detail {pn nr} {

	dobutton "dp $pn $nr"
	.p_detail.f.b$nr configure -bg lightgreen
}

proc pattern_detail {pn nr} {
	global pmap ptitle
	global Yoffset

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
	wm geometry .p_detail 300x$nh+2135+$Yoffset

	catch {
		# if the pattern set was deleted, this fails
		label .p_detail.t -text "$ptitle($pn)"
		pack .p_detail.t -side top -anchor w
		# in case we didnt succeed in filling in the full title before
		.patterns.main.b$nr configure -anchor w \
			-text "$pn ($pmap($pn) patterns) :: $ptitle($pn)"
	} emsg
	if {"$emsg" != ""} {
		puts "set $pn was deleted or merged with another set $pn : $emsg"
		destroy .p_detail
		destroy .patterns.main.b$nr
		return
	}

	frame .p_detail.f
	pack .p_detail.f -side top -anchor w

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
}

proc pat_show {pn nrm nrpat} {

	puts "$pn $nrm"
	handle_command "dp $pn" "pat_show"
	pattern_detail $pn $nrpat
}

proc predefined_list {} {
	global CBG CFG HV2 COBRA qfd
	global Yoffset

	toplevel .predef
	wm title .predef "Predefined Checks"
	wm iconname .predef "predef"
	wm geometry .predef 300x600+1530+$Yoffset

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
	set sn [ .smart.c.mark get ]
	set pe [ .smart.b.mark get ]

	if {"$pn" == ""} {
		puts "error: the new pattern set must have a name"
		return
	}
	if {"$sn"} {
		puts "error: define a search pattern for set $p"
		return
	}
	dobutton "pe ${sn}: $pe"
	handle_command "ps caption \[ .smart.c.mark get \] \[ .smart.c.mark get \]" "smart"
	handle_command "ps list\" \"smart"
}

proc smart_panel {} {
	global TFG TBG
	global d_mode Yoffset

	toplevel .smart
	wm title .smart "PatternSearch"
	wm iconname .smart "patsearch"
	wm geometry .smart 600x80+1530+20

	frame .smart.b
	frame .smart.c

	label .smart.b.y -width 8 -text "pattern:" -bg $TBG -fg $TFG
	entry .smart.b.mark -relief sunken -width 16 -bg "lightblue" -fg $TFG
	label .smart.c.y -width 8 -text "setname:" -bg $TBG -fg $TFG
	entry .smart.c.mark -relief sunken -width 16 -bg "lightgrey" -fg $TFG

	pack .smart.b .smart.b.mark -side right -fill both -expand yes
	pack .smart.b .smart.b.y    -side left  -fill both -expand no
	pack .smart.c .smart.c.mark -side right -fill both -expand yes
	pack .smart.c .smart.c.y    -side left  -fill both -expand no

	pack append .smart .smart.b {top fillx filly expand}
	pack append .smart .smart.c {top fillx filly expand}

	bind .smart.b.mark <Return> " smart_action "
	bind .smart.c.mark <Return> " smart_action "
}

proc do_query {which} {

	puts "here: $which"
	dobutton ". $which; r"
}

#### Startup
	create_panels
	set cargs "-g"

	if {$argc >= 1} {
		set ix [lsearch $argv "-n"]
		if {$ix >= 0} {
			set cargs "-g -n"
			set argv [lreplace $argv $ix $ix]
			incr argc -1
	}	}

	if [catch {set qfd [open "|$COBRA $cargs cobra_te.c" r+]} errmsg] {
		puts $errmsg
		exit 1
	}
	smart_panel
	predefined_list
	update
	get_ready
	update

	if {$argc >= 1} {
		foreach x $argv {
			put_hist "a $x"
			puts $qfd "a $x"
			flush $qfd
			get_ready
			update
	}	}

	$c_log insert end "queries:\n"
	$h_log configure -width 250

	focus $c_log

	update
