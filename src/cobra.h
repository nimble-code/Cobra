/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

#ifndef COBRA_H
#define COBRA_H

#include "cobra_fe.h"	// shared with front-end

typedef struct Bound	Bound;
typedef struct Cmd	Cmd;
typedef struct Commands	Commands;
typedef struct FList	FList;
typedef struct Function	Function;
typedef struct History	History;
typedef struct Lextok	Lextok;
typedef struct Match	Match;
typedef struct Qual	Qual;
typedef struct Script	Script;

typedef Lextok *Lexptr;

struct Lextok {
	int	typ;
	int	val;
	int	lnr;		// cobra_prog.y
	unsigned short	tag;
	unsigned short	visit;
	char	*s;

	Lextok *a, *b, *c;	// for fsm in cobra_prog.y
	Lextok *core;		// core qualifier
	Function *f;		// set for fct calls

	Lextok *lft;
	Lextok *rgt;
};

struct FList {
	int   visited;
	int   marked;
	char *nm;
	char *ns;	// scope name, for cplusplus
	char *detail;
	Prim *p;	// fct name
	const Prim *q;	// location of curly (start of body)
	FList *matched;
	FList *calls;
	FList *nxt;
};

struct History {
	char *s;
	History *nxt;
	History *prv;
};

struct Cmd {
	char	*cmd;
	char	*work;
	Cmd	*nxt;
};

struct Script {
	char	*nm;
	ArgList	*arg;
	Cmd	*txt;
	Script	*nxt;
};

struct Qual {
	const char *s;
	int *param;
};

struct Commands {
	char *cmd;
	void (*f)(char *, char *);
	char *explanation;
	int   n; // nr chars needed to disambiguate
};

struct Bound {
	Prim	*ref;
	Bound	*nxt;
};

struct Match {
	Prim	*from;
	Prim	*upto;
	Bound	*bind;
	Match	*nxt;
};

struct Function {
	Lextok	 *nm;
	Lextok   *formal;
	Lextok	 *body;
	int	  has_fsm;
	Function *nxt;
};

extern Files *seenbefore(const char *, int);
extern FILE  *track_fd;

extern char *check_args(char *, const char *);
extern char *cobra_bfnm(void);
extern char *cobra_fnm(void);
extern char *cobra_txt(void);
extern char *cobra_typ(void);
extern char *fct_which(const Prim *);

extern int add_stream(Prim *);
extern int cobra_lnr(void);
extern int cobra_nxt(void);
extern int cobra_prv(void);
extern int cobra_rewind(void);
extern int convert_matches(int);
extern int do_eval(const Prim *);
extern int exec_prog(Prim **, int);
extern int nr_marks(const int);
extern int prep_eval(void);
extern int prep_prog(FILE *);
extern int regex_match(const int, const char *);
extern int restore(const char *, const char *);
extern int save(const char *, const char *);

extern void cfg(char *from_f, char *to_f);
extern void clear_matches(void);
extern void cobra_range(Prim *, Prim *);
extern void cobra_tag(Prim *);
extern void cobra_te(char *, int, int);
extern void context(char *, char *);
extern void display(char *, char *);
extern void dogrep(const char *);
extern void fcg(char *from_f, char *to_f);
extern void fct_defs(void);
extern void fcts(char *, char *);
extern void findfunction(char *, char *);
extern void fix_imbalance(void);
extern void list_checkers(void);
extern void noreturn(void);
extern void rescan(void);
extern void run_bup_threads(void *(*f)(void*));
extern void run_threads(void *(*f)(void*), int);
extern void set_ranges(Prim *, Prim *);
extern void set_regex(char *);
extern void set_links(void);
extern void set_tmpname(char *, const char *, const int);
extern void show_line(FILE *, const char *, int, int, int, int);
extern void stop_threads(void);
extern void undo_matches(void);

extern void *clear_range(void *);
extern void *nr_marks_range(void *);
extern void *save_range(void *);
extern void *restore_range(void *);
extern void *fct_defs_range(void *);

extern FList *find_match_str(char *);
extern ArgList *cl_var;

extern char  CobraDot[64];
extern char  FsmDot[64];
extern char  ShowDot[128];
extern char  ShowFsm[128];
extern char *cobra_target;
extern char *cobra_texpr;
extern char *cobra_commands;
extern char *scrub_caption;
extern char *yytext;
extern char *global_t;

extern int inside_range;
extern int inverse;
extern int and_mode;
extern int view_mode;
extern int stream;
extern int top_only;
extern int top_up;
extern int global_n;

extern Prim *cur;
extern Prim *plst;
extern Prim *prim;

extern FList	**flist;
extern TokRange	**tokrange;

typedef unsigned int		uint;
typedef long unsigned int	ulong;

#ifndef DOT
 #ifdef PC
  #define DOT "dotty"
 #else
  #define DOT "dot -Tx11"
 #endif
#endif

#endif
