# Cobra - interactive static structural code analyzer
# Version 0 and 1:
# (c) 2015-2016 California Institute of Technology NASA/JPL
# see License file in this directory
#
# Version 2 and later:
# (c) 2017-2026 Gerard Holzmann, Nimble Research
# same licensing

MAN?=/usr/share/man/man1
APP=../src_app

OS1=cobra_prim.o cobra_prep.o cobra_heap.o \
   cobra_lib.o cobra_fcg.o cobra_lex.o \
   cobra_sym.o cobra_cfg.o cobra_links.o \
   cobra_array.o cobra_list.o

OS2=cobra_eval.o cobra_prog.o

OS3=cobra_json.o cobra_te.o cobra_match.o

AR=cobra_lex.o cobra_prep.o cobra_prim.o \
   cobra_heap.o ../src_app/c_util.o cobra_links.o cobra_json.o \
   cobra_stub.o cobra_match.o

			# gprof: make F="-g -pg"
			# coverage: make F="-fprofile-arcs -ftest-coverage"
F?=-O2
LIB?=-pthread		# linux, cygwin
LIBM?=			# mac
BINDIR?=/usr/local/bin

CC?=gcc
CFLAGS=-I. -Wall ${F} -pedantic -Werror -Wshadow -std=c99 -DYY_NO_INPUT -Wformat-truncation=1
CMAC=-I. -Wall ${F} -pedantic -Werror -Wshadow -std=c99 -DYY_NO_INPUT

# adding -DPC uses "dotty" instead of "dot -Tx11" for dot displays
# adjusts the memory allocator to be more PC friendly
# and sidesteps a cygwin bug that prevents gcc from being called in multicore mode

.PHONY:	linux cygwin mac install clean man

all:	$(OS1) $(OS2) $(OS3)
	$(CC) $(CFLAGS) -o cobra $(OS1) $(OS2) $(OS3) $(LIB) -lm

mac:	$(OS1) $(OS2) $(OS3)
	$(CC) $(CMAC) -o cobra $(OS1) $(OS2) $(OS3) $(LIBM)

app:	c.ar $(APP)/c_api.h
	cd $(APP); make

man:
	cp ../doc/*.1 $(MAN)

c.ar:	$(AR)
	ar -r c.ar $(AR)
	cp c.ar  $(APP)

install: c.ar man
	cp -f cobra $(BINDIR)
	cp -f ../bin_tools/tgrep $(BINDIR)
	cp -f ../gui/icobra.tcl $(BINDIR)/icobra
	cp -f ../gui/window.tcl $(BINDIR)
	cd $(APP); make clean install

${OS1}:	cobra.h cobra_fe.h cobra_prim.h
${OS2}:	cobra.h cobra_fe.h cobra_prim.h cobra_eval.c cobra_prog.c
${OS3}: cobra.h cobra_fe.h cobra_prim.h cobra_pre.h cobra_te.h
${AR}:	cobra_pre.h cobra_fe.h

cobra_array.o: cobra_array.c cobra_array.h cobra_prog.c
cobra_list.o:  cobra_list.c cobra_list.h

cobra_eval.c:	cobra_eval.y cobra.h
	yacc -o $@ cobra_eval.y

cobra_prog.c:	cobra_prog.y cobra.h cobra_array.h cobra_list.h
	yacc -d -p xx -o $@ cobra_prog.y

clean:
	rm -f *.o *.exe cobra
	rm -f cobra_eval.c cobra_prog.[ch] y.output
	rm -f c.ar $(APP)/c.ar
