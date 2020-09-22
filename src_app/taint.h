#include "c_api.h"

extern TokRange **tokrange;			// cwe_util.c

#define TPSZ	128	// must be power of 2, for hash to work

enum Seen {
	SeenVar	  =   1,
	SeenParam =   2,
	SeenDecl  =   4
};

enum Tags {
	None			=    0,
	FctName			=    1,
	Target			=    2,
	Alloca			=    4,
	UseOfTaint		=    8,
	DerivedFromTaint	=   16,
	PropViaFct		=   32,
	VulnerableTarget   	=   64,	// vulnerable targets
	CorruptableSource   	=  128,	// corruptable sources
	ExternalSource		=  256,
	DerivedFromExternal	=  512,
	Stop			= 1024
};

typedef struct Tainted {
	char	*fnm;		// fct name
	Prim	*src;		// where alert originated
	Prim	*param;		// name of param
	int	 pos;		// position of param
	int	 handled;	// in is_param_tainted
	struct Tainted *nxt;
} Tainted;

extern int is_return_tainted(const char *s, int cid);
extern int is_param_tainted(Prim *mycur, const int setlimit, const int cid);

extern void mark_fcts(void);			// cwe_util.c
extern void set_multi(void);			// cwe_util.c
extern int  run_threads(void *(*f)(void*));	// cwe_util.c
extern void param_is_tainted(Prim *p, const int pos, Prim *nm, const int cid);
extern void reset_tables(void);
extern void search_returns(Prim *mycur, const int cid);
extern void show_bindings(const char *s, const Prim *a, const Prim *b, int cid);
extern void taint_init(void);
