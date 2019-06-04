# Cobra - interactive static structural code analyzer
# Sample background checkers, linked to the Cobra
# front-end in c.ar, using the api defined in c_api.h
# See the license file in ../src

CWE=cwe.o cwe_util.o \
	cwe_119.o cwe_120.o cwe_131.o \
	cwe_134.o cwe_170.o cwe_197.o \
	cwe_468.o cwe_805.o cwe_416.o \
	cwe_457.o

OTHER=abstract \
	scope_check \
	binop \
	cfg \
	deref \
	fct_param_counts \
	flatten \
	float \
	ident_check \
	ident_length \
	ifelseif \
	igrep \
	lf \
	misra2004 \
	nomacros \
	nr_cases \
	rule23_rule31 \
	stmnt_per_line \
	switch_default

.PHONY: all install clean

CFLAGS=-O2 -Wall -DYY_NO_INPUT -I. -pedantic -Werror -Wshadow -std=c99
LDFLAGS?=c.ar -pthread		# on mac: make LDFLAGS=c.ar
CC?=gcc

all:	c.ar cwe scope_check

cwe:	cwe.h c.ar c_api.h $(CWE)
	$(CC) $(CFLAGS) -o cwe $(CWE) $(LDFLAGS)

c.ar:
	cd ../src; make c.ar

$(OTHER): c_api.h c.ar

install:
	@echo "use one of: install_linux, install_cygwin, or install_mac"

install_linux: all
	cp cwe scope_check ../bin_linux

install_cygwin: all
	cp *.exe ../bin_cygwin

install_mac: all
	cp cwe scope_check ../bin_mac

clean:
	rm -f *.exe *.o cwe $(OTHER)
