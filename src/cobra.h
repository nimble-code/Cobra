/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at https://codescrub.com/cobra
 */

#ifndef COBRA_H
#define COBRA_H

#include "cobra_fe.h"	// shared with front-end

#ifndef NOFLOAT
 #define C_FLOAT	// Version 5.1
#endif
#ifdef C_FLOAT
 #define C_TYP	float
#else
 #define C_TYP	int
#endif

typedef unsigned int		uint;
typedef long unsigned int	ulong;

typedef struct Cmd	Cmd;
typedef struct Commands	Commands;
typedef struct FList	FList;
typedef struct FHtab	FHtab;
typedef struct Function	Function;
typedef struct History	History;
typedef struct Lextok	Lextok;
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

struct FHtab {
	FList **fht;	// size: [NAHASH];
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

struct Function {
	Lextok	 *nm;
	Lextok   *formal;
	Lextok	 *body;
	int	  has_fsm;
	Function *nxt;
};

enum Renum { UNDEF = 0, VAL = 1, STR, PTR, STP, PRCD };

typedef enum	Renum	Renum;
typedef struct	Rtype	Rtype;
typedef struct  Separate Separate;

struct Rtype {
	Renum	rtyp;
	C_TYP	val;	// VAL
	char	*s;	// STR
	Prim	*ptr;	// PTR
};

struct Separate {	// thread-local copies, to avoid cache misses
	int Verbose;
	int P_debug;
	int Nest;
	int T_stop;
	char spacer[4096];
};

extern TokRange	**tokrange;
extern char	*C_TMP;
extern char	*b_cmd;
extern char	*cobra_commands;
extern char	*cobra_target;
extern char	*global_t;
extern char	*yytext;
extern char	CobraDot[64];
extern char	FsmDot[64];
extern char	ShowDot[128];
extern char	ShowFsm[128];

extern int	and_mode;
extern int	case_insensitive;
extern int	display_mode;
extern int	echo;
extern int	eof;
extern int	eol;
extern int	global_n;
extern int	inside_range;
extern int	inverse;
extern int	json_format;
extern int	json_plus;
extern int	no_caller_info;
extern int	no_echo;
extern int	read_stdin;
extern int	runtimes;
extern int	showprog;
extern int	solo;
extern int	stream;
extern int	stream_override;
extern int	lcount;
extern int	tcount;
extern int	tgrep;
extern int	top_only;
extern int	top_up;
extern int	view_mode;

extern Prim	*cur;
extern Prim	*plst;
extern Prim	*prim;

extern FHtab	**flist;
extern ArgList	*cl_var;
extern Bound	*get_bound(void);
extern FILE	*track_fd;
extern FList	*find_match_str(char *);
extern Files	*seenbefore(const char *, int);
extern Lextok	*prep_eval(void);
extern Prim	*bound_prim(const char *, int);
extern Prim	*cp_pset(char *, Lextok *, int);

extern char	*bound_text(const char *, int);
extern char	*check_args(char *, const char *);
extern char	*cobra_bfnm(void);
extern char	*cobra_fnm(void);
extern char	*cobra_txt(void);
extern char	*cobra_typ(void);
extern char	*fct_which(const Prim *);
extern char	*pattern(char *);
extern char	*unquoted(char *);

extern int	add_stream(Prim *);
extern int	cobra_lnr(void);
extern int	cobra_nxt(void);
extern int	cobra_prv(void);
extern int	cobra_rewind(void);
extern int	do_eval(const Prim *, const int);
extern int	do_split(char *, const char *, const char *, Rtype *, const int); // cobra_array.c
extern int	evaluate(const Prim *, const Lextok *, const int, const int);
extern int	exec_prog(Prim **, int);
extern int	has_stop(void);
extern int	have_lock(const int);
extern int	is_comments_or_source(void);
extern int	matches_suppress(const Prim *, const char *);
extern int	nr_marks(const int);
extern int	prep_prog(FILE *);
extern int	regex_match(const int, const char *);
extern int	setexists(const char *);
extern int	setexists(const char *s);
extern int	xxparse(void);

extern uint	hasher(const char *);

extern void	*clear_range(void *);
extern void	*fct_defs_range(void *);
extern void	*nr_marks_range(void *);
extern void	*restore_range(void *);
extern void	*save_range(void *);
extern void	Nthreads_set(int);
extern void	cfg(char *from_f, char *to_f);
extern void	clear_file_list(void);
extern void	clear_seen(void);
extern void	cobra_range(Prim *, Prim *);
extern void	cobra_tag(Prim *);
extern void	cobra_te(char *, int, int, int);
extern void	comments_or_source(int);
extern void	context(char *, char *);
extern void	declarations(void);
extern void	do_typedefs(int);
extern void	dogrep(const char *);
extern void	dp_help(void);
extern void	dump_tree(Lextok *, int);
extern void	eval_prog(Prim **, Lextok *, Rtype *, const int);
extern void	fcg(char *from_f, char *to_f);
extern void	fct_defs(void);
extern void	findfunction(char *, char *);
extern void	fix_imbalance(void);
extern void	handle_html(void);
extern void	json(const char *);
extern void	list(char *, char *);
extern void	list_checkers(void);
extern void	list_stats(void);
extern void	new_array(char *, int);	// cobra_array.c
extern void	noreturn(void);
extern void	patterns_create(void);
extern void	patterns_delete(void);
extern void	patterns_display(const char *);
extern void	patterns_json(char *);
extern void	patterns_list(char *);
extern void	patterns_rename(char *);
extern void	pe_help(void);
extern void	put_bound(Bound *);
extern void	put_match(Match *);
extern void	report_memory_use(void);
extern void	reproduce(const int, const char *);
extern void	rescan(void);
extern void	reset_int(const int); // cobra_prog.y
extern void	run_bup_threads(void *(*f)(void*));
extern void	run_threads(void *(*f)(void*));
extern void	set_cnt(const int);
extern void	set_links(void);
extern void	set_operation(char *);
extern void	set_ranges(Prim *, Prim *, int);
extern void	set_regex(char *);
extern void	set_string_type(Lextok *, char *);
extern void	set_textmode(void);
extern void	set_tmpname(char *, const char *, const int);
extern void	setname(char *);
extern void	show_error(FILE *, int);
extern void	slice_set(Named *, Lextok *, int);
extern void	stop_threads(void);
extern void	undo_matches(void);
extern void	var_links(char *, char *);
extern void	what_type(FILE *, Renum);
extern void	wrap_stats(void);

#ifndef DOT
 #ifdef PC
  #define DOT "dotty"
 #else
  #define DOT "dot -Tx11"
 #endif
#endif

#define Emalloc(a, b)	(void *) ((Ncore==1)?emalloc(a,b):hmalloc(a,ix,b)) // also in: cobra_pre.h

#endif
