/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

#ifndef COBRA_FE
#define COBRA_FE

#define tool_version	"Version 4.0 - 8 June 2022"

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

#define NEW_M	  	0
#define OLD_M	  	1

#define NAHASH		512
#define MAXYYTEXT	2048

typedef struct ArgList	ArgList;
typedef struct Files	Files;
typedef struct Stack	Stack;
typedef struct Typedef Typedef;
typedef unsigned char	uchar;
typedef struct Match	Match;
typedef struct Named	Named;	// named sets of matches
typedef struct Bound	Bound;
typedef struct Store	Store;

struct ArgList {
	char	*s;	// actual param
	char	*nm;	// formal param
	ArgList	*nxt;
};

struct Bound {
	Prim	*bdef;		// where binding was set
	Prim	*ref;		// where match was found
	Bound	*nxt;
};

struct Files {
	int	 imbalance;
	char	*s;
	char	*pp;	// optional file-specific preprocessing directives
	Prim	*first_token;
	Prim	*last_token;
	Files	*nxt;
};

struct Match {
	char	*msg;
	Prim	*from;
	Prim	*upto;
	Bound	*bind;
	Match	*nxt;
};

struct Named {
	char	*nm;
	char	*msg;
	Prim	*cloned;
	Match	*m;
	Named	*nxt;
};

struct Stack {
	const char *nm;
	int	 na;		// NrArgs
	char	*sa[MaxArg];	// ScriptArg
	Stack	*nxt;
};

struct Store {
	char	*name;
	char	*text;
	Prim	*bdef;		// where binding was set
	Prim	*ref;		// last place where bound var was seen
	Store	*nxt;
};

struct Typedef {
	int	level;
	Stack	*lst;
	Typedef	*up;
};

enum TokenStream {
	CMNT = 1,
	SRC
};

extern char	*C_BASE;
extern char	*glob_te;
extern char	*scrub_caption;
extern char	*cobra_texpr;
extern pthread_t *t_id;
extern Named	*namedset;
extern Match	*free_match;
extern Bound	*free_bound;

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
extern int	p_matched;
extern int	p_debug;
extern int	preserve;
extern int	python;
extern int	scrub;
extern int	stream;
extern int	stream_lim;
extern int	stream_margin;
extern int	verbose;

extern int	json_format;
extern int	json_plus;
extern int	nr_json;
extern char	json_msg[1024];
extern char	bvars[1024];
extern void	json_match(const char *, const char *, const Prim *, const Prim *);
extern void	new_named_set(const char *);
extern Named	*findset(const char *, int, int);
extern Files	*findfile(const char *);

extern int	add_file(char *, int, int);
extern int	check_config(void);
extern int	c_lex(int);
extern int	do_markups(const char *);
extern int	json_convert(const char *);
extern int	listfiles(int, const char *);
extern int	matches2marks(int);
extern int	mkstemp(char *);
extern int	sanitycheck(int);

extern size_t	*hmalloc(size_t, const int, const int);

extern void	add_match(Prim *f, Prim *t, Store *bd);
extern void	add_pattern(const char *, const char *, Prim *, Prim *, int);
extern void	basic_prim(const char *s, int cid);
extern void	clear_matches(void);
extern void	clr_matches(int);
extern void	cobra_main(void);
extern void	del_pattern(const char *, Prim *, Prim *, int);
extern void	do_lock(int);
extern void	do_unlock(int);
extern void	efree(void *);
extern void	*emalloc(size_t, const int);
extern void	ini_heap(void);
extern void	ini_lock(void);
extern void	ini_par(void);
extern void	ini_pre(int);
extern void	ini_timers(void);
extern void	json_import(const char *, int);
extern void	lock_print(int);
extern void	memusage(void);
extern void	post_process(int);
extern void	prep_pre(void);
extern void	remember(const char *, int, int);
extern void	show_line(FILE *, const char *, int, int, int, int);
extern void	start_timer(int);
extern void	stop_timer(int, int, const char *);
extern void	strip_comments_and_renumber(int);
extern void	t_lex(int);	// text only mode
extern void	unlock_print(int);

#define free		efree
#define is_blank(x)	((x) == ' ' || (x) == '\t')	// avoiding isblank()

#endif
