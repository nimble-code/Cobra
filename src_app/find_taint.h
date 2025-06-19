#include "c_api.h"

#define TPSZ	128	// must be a power of 2

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

enum ConfigTypes {
	TAINTSOURCES = 0,
	IMPORTERS, 
	PROPAGATORS,
	IGNORE,	// non-propagators
	TARGETS,
	NONE
};

typedef struct Tainted {
	char	*fnm;		// fct name
	Prim	*src;		// where alert originated
	Prim	*param;		// name of param
	int	 pos;		// position of param
	int	 handled;	// in is_param_tainted
	struct Tainted *nxt;
} Tainted;

typedef struct ConfigDefs {
	char	*nm;
	char	*fct;		// optional for TAINTSOURCES
	int	 from;
	int	 into;
	struct ConfigDefs  *nxt;
} ConfigDefs;

typedef struct ConfigMap {
	char	*heading;
	int	 type;
} ConfigMap;

extern ConfigDefs *configged[NONE];
extern TokRange **tokrange;			// c_util.c
extern char *C_BASE;	// cobra_prep.c, rules directory
extern char *Cfg;	// optional user-defined configuration file
extern int ngets, warnings;

extern void mark_fcts(void);			// c_util.c
extern void param_is_tainted(Prim *p, const int pos, Prim *nm, const int cid);
extern void reset_tables(void);
extern void search_returns(Prim *mycur, const int cid);
extern void set_multi(void);			// c_util.c
extern void show_bindings(const char *s, const Prim *a, const Prim *b, int cid);
extern void taint_init(void);

extern int  cfg_ignore(const char *);
extern int  handle_importers(Prim *);
extern int  handle_taintsources(Prim *);
extern int  is_param_tainted(Prim *mycur, const int setlimit, const int cid);
extern int  is_propagator(const char *);
extern int  is_return_tainted(const char *s, int cid);
extern int  run_threads(void *(*f)(void*));	// c_util.c
extern int  taint_configs(void);

extern Prim *next_arg(Prim *, const Prim *);
extern Prim *pick_arg(Prim *, Prim *, int);
extern Prim *start_args(Prim *);
