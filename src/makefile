# Cobra - interactive static structural code analyzer
# Version 0 and 1:
# (c) 2015-2016 California Institute of Technology NASA/JPL
# see License file in this directory
#
# Version 2, 3 and 4:
# (c) 2017-2022 Gerard Holzmann, Nimble Research
# same licensing

MAN?=/usr/share/man/man1
APP=../src_app

OS=cobra_prim.o cobra_prep.o cobra_heap.o \
   cobra_lib.o cobra_eval.o cobra_fcg.o cobra_lex.o \
   cobra_prog.o cobra_sym.o cobra_cfg.o cobra_te.o \
   cobra_links.o cobra_array.o cobra_list.o cobra_json.o

AR=cobra_lex.o cobra_prep.o cobra_prim.o \
   cobra_heap.o cobra_links.o cobra_json.o \
   cobra_stub.o

			# gprof: make F="-g -pg"
			# coverage: make F="-fprofile-arcs -ftest-coverage"
F?=-O2
LIB?=-pthread		# linux (and more recent versions of cygwin)
LIBW?=-lpthread		# cygwin
LIBM?=			# mac

CC?=gcc
CFLAGS=-I. -Wall ${F} -pedantic -Werror -Wshadow -std=c99 -DYY_NO_INPUT

# adding -DPC uses "dotty" instead of "dot -Tx11" for dot displays
# adjusts the memory allocator to be more PC friendly
# and sidesteps a cygwin bug that prevents gcc from being called in multicore mode

.PHONY:	linux cygwin mac install clean man

linux:	$(OS)
	$(CC) $(CFLAGS) -o cobra $(OS) $(LIB)

cygwin:	$(OS)
	$(CC) $(CFLAGS) -o cobra $(OS) $(LIBW)

mac:	$(OS)
	$(CC) $(CFLAGS) -o cobra $(OS) $(LIBM)

app:	c.ar $(APP)/c_api.h
	cd $(APP); make

man:
	cp ../doc/*.1 $(MAN)

c.ar:	$(AR)
	ar -r c.ar $(AR)
	cp c.ar  $(APP)

install:
	@echo "use one of: install_linux, install_cygwin, or install_mac"

install_linux: linux c.ar man
	cp -f cobra ../bin_linux
	cp -f ../gui/window.tcl ../bin_linux
	cp -f ../gui/icobra.tcl ../bin_linux/icobra
	cd $(APP); make clean install_linux

install_cygwin: cygwin c.ar man
	cp -f cobra.exe ../bin_cygwin
	cp -f ../gui/window.tcl ../bin_cygwin
	cp -f ../gui/icobra.tcl ../bin_cygwin/icobra
	cd $(APP); make clean install_cygwin

install_mac: mac c.ar man
	cp -f cobra ../bin_mac
	cp -f ../gui/window.tcl ../bin_mac
	cp -f ../gui/icobra.tcl ../bin_mac/icobra
	cd $(APP); make clean install_mac

${OS}:	cobra.h cobra_fe.h

${AR}:	cobra_pre.h cobra_fe.h

cobra_array.o: cobra_array.c cobra_array.h

cobra_list.o:  cobra_list.c cobra_list.h

cobra_eval.c:	cobra_eval.y cobra.h
	yacc -o $@ cobra_eval.y

cobra_prog.c:	cobra_prog.y cobra.h cobra_array.h cobra_list.h
	yacc -d -p xx -o $@ cobra_prog.y

clean:
	rm -f *.o *.exe cobra
	rm -f cobra_eval.c cobra_prog.[ch] y.output
	rm -f c.ar $(APP)/c.ar
