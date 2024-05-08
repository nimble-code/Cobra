# Cobra
## An interactive static source code analyzer

Cobra is a fast code analyzer that can be used
to interactively probe and query up to millions of lines
of code. The basic design of the tool is language-neutral,
though a lot of query and rule libraries have been
developed, and are included in the distribution, that target
C or C-like languages. The original version of the tool
(version 1.0) was developed at NASA/JPL and cleared
for public release in April 2016. The current version (4.6)
is a significantly extended version of the tool,
released under the same license.

## Installation

* choose a directory to install the tool,
   below this is referred to as directory $COBRA

   $ git clone https://github.com/nimble-code/Cobra

   which gives you a directory with a set of
   sub-directories like this:

   drwxrwxr-x 2 gh gh 4096 May 16 10:03 doc     # change history, manpage, license  
   drwxrwxr-x 2 gh gh 4096 May 16 10:03 gui     # optional small tcl/tk script  
   drwxrwxr-x 8 gh gh 4096 May 16 15:55 rules   # cobra checker libraries  
   drwxrwxr-x 1 gh gh 4096 May 16 12:43 src     # cobra source files  
   drwxrwxr-x 1 gh gh 4096 May 16 12:43 src_app # standalone cobra checkers  

* you can also download binaries from the last stable release of Cobra
  from the Releases tab -- they are in three .zip files with precompiled
  binaries of cobra and related tools for cygwin, linux, and mac.

* to compile the tool (if you are not using precompiled executables in one
  of the ./bin_... directories)

   $ cd src

   and depending on your platform, do:

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

   $ cobra -configure $COBRA

   this creates a ~/.cobra file in your home directory, which
   cobra reads on startup to find the predefined checker libraries

   you can also tell Cobra where the libraries are by setting and
   exporting an environment variable C_BASE, for instance as follows:

     export C_BASE=$COBRA/rules

   if both a ~/.cobra file exists and the $C_BASE variable is set, the
   latter will be used.

* on older cygwin 32bit platforms you may also have to help cobra
  find where the /tmp directory for temporary files is located, e.g.
  to make the cobra -view -pat '...' option work with dot or dotty.
  to do so, add the following environment variable as well, for instance:

     export C_TMP=C:/cygwin

* for basic usage, to get started using Cobra, see doc/BasicUsage.txt
   and online for more detailed tutorials: [http://spinroot.com/cobra]


## SysML v2 Usage

Basic usage with pattern on commandline:
```shell
cobra -Sysml2 -pe PATTERN *.sysml
```

Also the pattern search mode and inline programs are supported:
```shell
cobra -Sysml *.sysml
```


`-Sysml2` switches cobra to the SysML v2 mode, such that its keywords element names and syntax is recognized. The following new matchers are defined:
1. @element matches sysml elements, e.g., part, package, port, ...
1. @usage matches usages

The following existing ones are supported and extended
1. @type matches all defined types, including names of definitions in the model
1. @key matches all defined SysML v2 keywords

Examples 0:

Match recursive definitions (simplified example):
```shell
cobra -Sysml2 -pe '@element def x:@ident { .* : :x .*  } *.sysml
```
Here we match first an element as defined by the standard (e.g., part) followed by the keyword `def` followed by an indentifier. In the body we match for a `:` that indicates an usage followed by the name of the definition. Thought, we ignore here short names and some other cases.

Using the interactive prompt:
```
m @element def 
```
