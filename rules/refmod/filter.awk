#!/bin/bash

# filter out pattern lines not followed by a file match
# and file matches not preceded by a pattern (multiple matches of same pattern)
# usage: refmod/outliers.sh | refmod/filter.awk

awk '
BEGIN		{ Pat=""; File=""; cnt=1; }
$1=="pattern:"	{ Pat=$0; File=""; }
$1=="file:"	{
		  if (Pat != "")
		  { printf("\n%d\t%s\n\t%s\n", cnt++, Pat, $0);
		    Pat = ""; SawFor = 0; lastline = "";
		    File=$2;
		  }
		}
$1!="pattern:" && $1!="file:" {
		  if (substr($1,0,3)=="for") { SawFor = 1; }
		  if (File != "" && NF>0 && SawFor == 1 && lastline != $0)
		  {	print "\t" $0;
			lastline = $0;
		  }
		}'
