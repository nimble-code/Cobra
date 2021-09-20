#include "c_api.h"
#include <pthread.h>

#define mycur_nxt()	(mycur = mycur?mycur->nxt:NULL)
#define mymatch(s)	(strcmp(mycur->txt, s) == 0)
#define mytype(s)	(strcmp(mycur->typ, s) == 0)

typedef struct TrackVar	TrackVar;
typedef struct Lnrs	Lnrs;

struct Lnrs {
	Prim *nv;
	int   fnr;
	Lnrs *nxt;
};

struct TrackVar {
	Lnrs	 *lst;
	Prim	 *t;
	int	  cnt;
	TrackVar *nxt;
};

int   run_threads(void *(*f)(void*));
void  set_multi(void);

Lnrs *store_var(TrackVar **, Prim *, int, int);
void  store_simple(TrackVar **, Prim *, int);
void  unstore_var(TrackVar **, Prim *);
Lnrs *is_stored(TrackVar *, Prim *, int);
void  clear_marks(Prim *, Prim *);
void  mark_fcts(void);
void  reset_fcts(void);

void cwe119_1(void);
void cwe119_2(void);
void cwe119_3(void);

void cwe120_1(void);
void cwe120_2(void);
void cwe120_3(void);

void cwe131_0(void);
void cwe134_0(void);
void cwe170_0(void);
void cwe197_0(void);
void cwe468_0(void);
void cwe805_0(void);
void cwe416_0(void);
void cwe457_0(void);
void cwe457_init(void);
