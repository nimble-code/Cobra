#ifndef COBRA_PRIM_H
#define COBRA_PRIM_H

// definitions shared with ../src_app/c_api.h

#define tool_version	"Version 5.3 - 6 January 2026"

typedef struct Prim	Prim;
typedef struct TokRange	TokRange;

typedef unsigned char	uchar;

struct Prim {
	char	*fnm;
	char	*typ;
	char	*txt;

	int	 lnr;
	int	 seq;		// sequence nr
	int	 mark;		// user-definable

	short	curly;		// {} level of nesting
	short	round;		// ()
	short	bracket;	// []
	short	len;

	Prim	*nxt;
	Prim	*prv;

	short	 mset[4];	// 4 sets of marks;  0=undo
	Prim	*mbnd[4];	// 4 sets of bounds; 0=undo

	Prim	*bound;		// bound symbol
	Prim	*jmp;		// from { or } to } or { etc
};

struct TokRange {
	int	 seq;
	int	 param;
	Prim	*from;
	Prim	*upto;
};


extern Prim *cur;
extern Prim *plst;
extern Prim *prim;

extern char *cobra_commands;
extern char *cobra_target;
extern char *cobra_texpr;
extern char  json_msg[1024];

extern int   json_convert(const char *);
extern void  json(const char *);
extern void  json_match(const char *, const char *, const char *, const Prim *, const Prim *, int);
extern void  add_pattern(const char *, const char *, Prim *, Prim *, int);
extern void  del_pattern(const char *, Prim *, Prim *, int);

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
extern void *emalloc(size_t, const int);

extern void lock_print(int);
extern void unlock_print(int);
extern void do_lock(const int, const int);
extern void do_unlock(const int, const int);
extern void clear_seen(void);
extern void set_links(void);

extern void ini_timers(void);
extern void start_timer(int);
extern void stop_timer(int, int, const char *);
extern size_t *hmalloc(size_t, const int, const int);

extern int ada;
extern int count;
extern int cplusplus;
extern int java;
extern int json_format;
extern int json_plus;
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

extern char *progname;
extern char *backend;

#endif
