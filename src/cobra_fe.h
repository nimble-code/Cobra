/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

#ifndef COBRA_FE
#define COBRA_FE

#define tool_version	"Version 3.3 - 19 January 2021"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
// shared with ../src_app/c_api.h :
#include "cobra_prim.h"

#ifndef PC
 #if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  #define PC
  #define WINDOWS
 #endif
#endif

#ifdef PC
 #include <io.h>
#endif

#ifndef MaxArg
 #define MaxArg	8
#endif

#define NAHASH		512
#define MAXYYTEXT	2048

typedef struct ArgList	ArgList;
typedef struct Files	Files;
typedef struct Stack	Stack;
typedef struct Typedef Typedef;
typedef unsigned char	uchar;

struct ArgList {
	char	*s;	// actual param
	char	*nm;	// formal param
	ArgList	*nxt;
};

struct Files {
	int	 imbalance;
	char	*s;
	Prim	*first_token;
	Prim	*last_token;
	Files	*nxt;
};

struct Stack {
	const char *nm;
	int	 na;		// NrArgs
	char	*sa[MaxArg];	// ScriptArg
	Stack	*nxt;
};

struct Typedef {
	int	level;
	Stack	*lst;
	Typedef	*up;
};

extern char	*C_BASE;
extern pthread_t *t_id;

extern int	ada;
extern int	all_headers;
extern int	count;
extern int	cplusplus;
extern int	Ctok;
extern int	gui;
extern int	java;
extern int	Ncore;
extern int	Nfiles;
extern int	no_cpp;
extern int	no_cpp_sticks;
extern int	no_display;
extern int	no_headers;
extern int	no_match;
extern int	parse_macros;
extern int	p_debug;
extern int	preserve;
extern int	python;
extern int	scrub;
extern int	stream_lim;
extern int	stream_margin;
extern int	verbose;
extern int	with_comments;

extern int	add_file(char *, int, int);
extern int	c_lex(int);
extern int	listfiles(int, const char *);
extern int	mkstemp(char *);
extern int	sanitycheck(int);

extern size_t	*hmalloc(size_t, const int, const int);

extern void	basic_prim(const char *s, int cid);
extern void	cobra_main(void);
extern void	do_lock(int);
extern void	do_unlock(int);
extern void	efree(void *);
extern void	*emalloc(size_t, const int);
extern void	ini_heap(void);
extern void	ini_lock(void);
extern void	ini_par(void);
extern void	ini_pre(int);
extern void	ini_timers(void);
extern void	lock_print(int);
extern void	memusage(void);
extern void	post_process(int);
extern void	prep_pre(void);
extern void	remember(const char *, int, int);
extern void	start_timer(int);
extern void	stop_timer(int, int, const char *);
extern void	t_lex(int);	// text only mode
extern void	unlock_print(int);

#define free		efree
#define is_blank(x)	((x) == ' ' || (x) == '\t')	// avoiding isblank()

#endif
