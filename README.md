# Cobra
## An interactive static source code analyzer

Cobra is a fast code analyzer that can be used
to interactively probe and query up to millions of lines
of code. The basic design of the tool is language-neutral,
though a lot of query and rule libraries have been
developed, and are included in the distribution, that target
C or C-like languages. The original version of the tool
(version 1.0) was developed at NASA/JPL and cleared
for public release in April 2016. The current version (3.0)
is a significantly extended version of the tool,
released under the same license in June 2019.

## Installation

* choose a directory to install the tool,
   below this is referred to as directory $COBRA

   $ git clone https://github.com/nimble-code/Cobra

   which gives you a directory with a set of
   sub-directories like this:

   drwxrwxr-x 2 gh gh 4096 May 16 12:59 bin_linux  # executables for linux
   drwxrwxr-x 2 gh gh 4096 May 16 12:59 bin_cygwin # executables for cygwin
   drwxrwxr-x 2 gh gh 4096 May 16 12:59 bin_mac    # executables for macs
   drwxrwxr-x 2 gh gh 4096 May 16 10:03 doc     # change history, manpage, license
   drwxrwxr-x 2 gh gh 4096 May 16 10:03 gui     # optional small tcl/tk script
   drwxrwxr-x 8 gh gh 4096 May 16 15:55 rules   # cobra checker libraries
   drwxrwxr-x 1 gh gh 4096 May 16 12:43 src     # cobra source files
   drwxrwxr-x 1 gh gh 4096 May 16 12:43 src_app # standalone cobra checkers

* to compile the tool (if you are not using precompiled executables in one
  of the ./bin_... directories)

   $ cd src
   # depending on your platform, do:
   	$ sudo make install_linux
     or
   	$ make install_cygwin
     or
   	$ make install_mac

* add $COBRA/bin_... to your search PATH environment variable, matching
   the platform you are using.
   if you use the bash shell, you can add this line at the end
   of the ~/.bashrc script, where $COBRA is defined as above,
   for instance:

     export PATH=$PATH:$COBRA/bin_linux

* configure the tool so that it knows where to find the rule libraries
  (using the $COBRA directory set at the beginning):

   $ cobra -configure $COBRA/rules

   this creates a ~/.cobra file in your home directory, which
   cobra reads on startup to find the predefined checker libraries

   you can also tell Cobra where the libraries are by setting and
   exporting an environment variable C_BASE, for instance as follows:

     export C_BASE=$COBRA/rules

   if both a ~/.cobra file exists and the $C_BASE variable is set, the
   latter will be used.

* for basic usage, to get started using Cobra, see doc/BasicUsage.txt
   and online for more detailed tutorials: [http://spinroot.com/cobra]
