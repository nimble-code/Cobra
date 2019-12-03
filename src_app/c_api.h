/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

#ifndef COBRA_H
#define COBRA_H

// should really be a copy of cobra_fe.h

#define tool_version	"Version 3.1 - 30 October 2019"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
// #include <malloc.h>
#include <time.h>

#ifdef PC
 #include <io.h>
#else
 #include <unistd.h>
 #include <pthread.h>
 #include <sys/times.h>
#endif

typedef struct Prim	Prim;
typedef struct TokRange	TokRange;

struct Prim {
	char	*fnm;
	int	 lnr;

	short	curly;		// {} level of nesting
	short	round;		// ()
	short	bracket;	// []
	short	len;

	char	*typ;
	char	*txt;

	int	 seq;		// sequence nr
	int	 mark;		// user-definable

	short	 mset[4];	// 4 sets of marks;  0=undo
	Prim	*mbnd[4];	// 4 sets of bounds; 0=undo

	Prim	*bound;		// bound symbol
	Prim	*jmp;		// from { or } to } or { etc

	Prim	*prv;
	Prim	*nxt;
};

struct TokRange {
	int	 seq;
	int	 param;
	Prim	*from;
	Prim	*upto;
};

#define NEXT		cobra_nxt()
#define LAST		cobra_prv()
#define MATCH(x)	(strcmp(cobra_txt(), x) == 0)
#define TYPE(x)		(strcmp(cobra_typ(), x) == 0)
#define FIND(x)		{ if (!MATCH(x)) { continue; } }

typedef unsigned char	uchar;

#define EXPECT(x) { \
	NEXT; \
	if (!MATCH(x)) \
	{	printf("%s:%d: expected '%s', saw '%s'\n", \
			cobra_bfnm(), cobra_lnr()-1, x, \
			cobra_txt()); \
		rval = 0; \
	} \
}

extern Prim *cur;
extern Prim *plst;
extern Prim *prim;

extern char *cobra_commands;
extern char *cobra_target;
extern char *cobra_texpr;

extern char *cobra_bfnm(void);
extern char *cobra_fnm(void);
extern char *cobra_txt(void);
extern char *cobra_typ(void);

extern int cobra_lnr(void);
extern int cobra_nxt(void);
extern int cobra_prv(void);
extern int cobra_rewind(void);

extern void cobra_main(void);
extern void cobra_tag(Prim *);
extern void cobra_range(Prim *, Prim *);
extern void *emalloc(size_t);

extern void lock_print(int);
extern void unlock_print(int);
extern void do_lock(int);
extern void do_unlock(int);
extern void clear_seen(void);
extern void set_links(void);

extern void ini_timers(void);
extern void start_timer(int);
extern void stop_timer(int, int, const char *);
extern size_t hmalloc(size_t, const int);

extern int ada;
extern int count;
extern int cplusplus;
extern int java;
extern int Ncore;
extern int Nfiles;
extern int no_cpp;
extern int no_headers;
extern int no_match;
extern int no_display;
extern int p_debug;
extern int verbose;
extern int with_comments;
extern int runtimes;

extern char *cwe_args;
extern char *progname;
#endif
