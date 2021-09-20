#include "cwe.h"

// CWE-120
typedef struct ThreadLocal120 ThreadLocal120;
typedef struct Results120 Results120;

struct ThreadLocal120 {
	 TrackVar *Strcpy, *Strcat, *Gets;
	 TrackVar *Sprintf, *Snprintf, *Scanf;
	 TrackVar *Var_scope;
	 int w_cnt, cnt;
};

struct Results120 {
	Prim *mycur;
	Prim *fc;
	Prim *src;
	Prim *dst;
	int   ssc;
	int   dsc;
	Results120 *nxt;
};

static ThreadLocal120 *thr;
static Results120 **results;

extern TokRange **tokrange;	// cwe_util.c

void
cwe120_init(void)
{	static int lastN = 0;

	if (lastN < Ncore)
	{	thr = (ThreadLocal120 *) emalloc(Ncore * sizeof(ThreadLocal120));
		results = (Results120 **) emalloc(Ncore * sizeof(Results120 *));
		lastN = Ncore;
	} else
	{	memset(thr,     0, Ncore * sizeof(ThreadLocal120));
		memset(results, 0, Ncore * sizeof(Results120 *));
	}
}

static void
store_ifnot_checked(Prim *mycur, int cid)	// check if the src variable was checked earlier
{	Prim *pm = mycur;
	Prim *src;

	while (mycur && strcmp(mycur->txt, ",") != 0)
	{	mycur_nxt();
	}
	if (!mycur_nxt())
	{	return;
	}
	if (strcmp(mycur->txt, "&") == 0
	||  strcmp(mycur->txt, "*") == 0)
	{	mycur_nxt();
	}
	src = mycur;

	if (!src
	||  strcmp(src->typ, "str") == 0)
	{	return;				// src is a fixed size string
	}

	mycur = pm;
	while (mycur->curly > 0)
	{	mycur = mycur->prv;
		if (mycur->round > 0		// did we see this var before in a condition
		&&  mymatch(src->txt))
		{	while (strcmp(mycur->txt, "(") != 0)
			{	mycur = mycur->prv;
			}
			mycur = mycur->prv;	// name (
			if (mymatch("strlen")
			||  mymatch("sizeof"))
			{	return;		// likely ok
	}	}	}

	mycur = pm;
	assert(cid >= 0 && cid < Ncore);
	if (mymatch("strcpy"))
	{	store_simple(&(thr[cid].Strcpy), mycur, 0);
	} else if (mymatch("strcat"))
	{	store_simple(&(thr[cid].Strcat), mycur, 0);
}	}

int
size_fct(TrackVar *p)
{	int sz = 0;

	while (p)
	{	sz += p->cnt;
		p = p->nxt;
	}
	return sz;
}

void
cwe120_1_range(Prim *from, Prim *upto, int cid)
{	Prim *mycur;

	mycur = from;
	assert(cid >= 0 && cid < Ncore);
	while (mycur_nxt() && mycur->seq <= upto->seq)
	{	if (mymatch("strcpy")
		||  mymatch("strcat"))
		{	store_ifnot_checked(mycur, cid);
			continue;
		}
		if (mymatch("gets")
		&&  strcmp(mycur->nxt->txt, "(") == 0)
		{	store_simple(&(thr[cid].Gets), mycur, 0);
			continue;
		}
		if (mymatch("sprintf"))
		{	store_simple(&(thr[cid].Sprintf), mycur, 0);
			continue;
	}	}
}

static void
cwe120_check_list(TrackVar *p, const char *s)
{	Lnrs *q;

	while (p)
	{	for (q = p->lst; q; q = q->nxt)
		{	printf("%s:%d: %s\n", q->nv->fnm, q->nv->lnr, s);
		}
		p = p->nxt;
	}
}

static void
cwe120_check_gen(const int which, const char *v, const char *w)
{	TrackVar *p;
	int i, w_cnt = 0;

	for (i = 0; i < Ncore; i++)
	{	switch (which) {
		case 0:	p = thr[i].Gets;    break;
		case 1:	p = thr[i].Strcpy;  break;
		case 2:	p = thr[i].Strcat;  break;
		case 3:	p = thr[i].Sprintf; break;
		default:
			fprintf(stderr, "cwe120_check_gen: cannot happen\n");
			exit(1);
		}
		w_cnt += size_fct(p);
	}
	if (w_cnt > 0)
	{	if (no_display)
		{	printf("cwe_120_1: %d warnings: %s performs no bounds checking, (use %s)\n", w_cnt, v, w);
		} else
		{	char *buf;
			int len = strlen(v)+strlen(w)+strlen("cwe_120_1,  performs no bounds checking, (use )");

			buf = (char *) emalloc( (len+1)*sizeof(char) );
			sprintf(buf, "cwe_120_1, %s performs no bounds checking, (use %s)", v, w);

			for (i = 0; i < Ncore; i++)
			{	switch (which) {
				case 0:	p = thr[i].Gets;    break;
				case 1:	p = thr[i].Strcpy;  break;
				case 2:	p = thr[i].Strcat;  break;
				case 3:	p = thr[i].Sprintf; break;
				default:
					fprintf(stderr, "cwe120_check_gen: cannot happen\n");
					exit(1);
				}
				cwe120_check_list(p, buf);
	}	}	}
}

void
cwe120_2_range(Prim *from, Prim *upto, int cid)
{	Prim *mycur;

	mycur = from;
	assert(cid >= 0 && cid < Ncore);
	while (mycur_nxt() && mycur->seq <= upto->seq)
	{	if (mymatch("snprintf"))
		{	mycur_nxt();	// (
			if (!mymatch("(")) { continue; }
			mycur_nxt();	// "..."
			if (strstr(mycur->txt, "%s"))	// no width specifier or %m
			{	store_simple(&(thr[cid].Snprintf), mycur, 0);
			}
			continue;
		}
		if (strstr(mycur->txt, "scanf"))
		{	mycur_nxt();	// (
			if (!mymatch("(")) { continue; }
			mycur_nxt();	// "..."
			if (strstr(mycur->txt, "%s"))	// no width specifier or %m
			{	store_simple(&(thr[cid].Scanf), mycur, 0);
	}	}	}
}

// tag every string var with its likely declaration
// if we can determine a size, eg char s[32] as global, local, or parameter
// this can then be used in a scan for suspicious strcpy or strncpy calls
// tokens are marked with .fct scope or "global" but not subscopes
// look for declarations of type char []

void
cwe120_3_add(Prim *mycur, Prim *fc, Prim *src, Prim *dst, int ssc, int dsc, int cid)
{	Results120 *r;

	assert(cid >= 0 && cid < Ncore);

	r = (Results120 *) hmalloc(sizeof(Results120), cid);
	r->mycur = mycur;
	r->fc = fc;
	r->src = src;
	r->dst = dst;
	r->ssc = ssc;
	r->dsc = dsc;

	r->nxt = results[cid];
	results[cid] = r;
}

void
cwe120_3_range(Prim *from, Prim *upto, int cid)
{	Prim *mycur, *nm;
	Prim *fc, *val, *val2;
	Prim *src, *dst;
	Lnrs *x, *y;
	int dsc, ssc, found = 0;

	mycur = from;
	assert(cid >= 0 && cid < Ncore);
	while (mycur_nxt() && mycur->seq <= upto->seq)
	{	// look for char name[constant]
		if (!mymatch("char"))
		{	continue;
		}
		mycur_nxt();

		if (!mytype("ident"))
		{	continue;
		}
		nm = mycur;
		mycur_nxt();

		if (!mymatch("["))
		{	continue;
		}
		mycur_nxt();

		if (strcmp(mycur->typ, "const_int") != 0)
		{	continue;
		}
		store_var(&(thr[cid].Var_scope), nm, 0, 0);	// nm.fct needed for match
	}

	// use Var_scope info to check strcpy and strncpy calls
	// within the given range

	mycur = from;
	while (mycur_nxt() && mycur->seq <= upto->seq)
	{
		if (!mymatch("strcpy"))
		{	continue;
		}
	
		fc = mycur;	// fc

		mycur_nxt();	// (
		mycur_nxt();
		if (!mytype("ident"))
		{	continue;
		}
		dst = mycur;	// dst

		mycur_nxt();	// ,
		if (!mymatch(","))
		{	continue;
		}
		mycur_nxt();
		if (!mytype("ident"))
		{	continue;
		}
		src = mycur;		// src
	
		// src and dst are both string refs - look them up in Var_scope
		// to see if we know something about their declared sizes

		x = is_stored(thr[cid].Var_scope, dst, 0);
		for ( ; x && !found; x = x->nxt)
		{	if (!x->nv)
			{	continue;
			}
			val = x->nv->nxt->nxt;	// [ N ]
			if (!isdigit((int) (val->txt[0])))
			{	continue;
			}
			dsc = atoi( val->txt );			// array size field of dst	
			y = is_stored(thr[cid].Var_scope, src, 0);
			for ( ; y; y = y->nxt)
			{	if (!y->nv)
				{	continue;
				}
				val2 = y->nv->nxt->nxt;
				if (!isdigit((int) (val2->txt[0])))
				{	continue;
				}
				ssc = atoi( val2->txt );	// array size field of src
				if (dsc > 0
				&&  dsc < ssc)
				{	cwe120_3_add(mycur, fc, src, dst, ssc, dsc, cid);
					found = 1;
					break;
	}	}	}	}
}

void *
cwe120_1_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	cwe120_1_range(from, upto, *i);

	return NULL;
}

void *
cwe120_2_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	cwe120_2_range(from, upto, *i);

	return NULL;
}

void *
cwe120_3_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	cwe120_3_range(from, upto, *i);

	return NULL;
}

static void
report_120_1(void)
{
	cwe120_check_gen(0, "gets",    "fgets");
	cwe120_check_gen(1, "strcpy",  "strncpy");
	cwe120_check_gen(2, "strcat",  "strncat");
	cwe120_check_gen(3, "sprintf", "snprintf");
}

static void
report_120_2(void)
{	int i, w_cnt = 0;
	char *buf;

	for (i = 0; i < Ncore; i++)
	{	w_cnt += size_fct(thr[i].Snprintf);
	}
	if (w_cnt > 0)
	{	buf = "cwe_120_2, snprintf  :: missing width limit on %%s";

		if (no_display)
		{	printf("cwe_120_2: %d warnings: %s\n", w_cnt, buf);
		} else
		{	for (i = 0; i < Ncore; i++)
			{	cwe120_check_list(thr[i].Snprintf, buf);
	}	}	}

	for (i = w_cnt = 0; i < Ncore; i++)
	{	w_cnt += size_fct(thr[i].Scanf);
	}
	if (w_cnt > 0)
	{	buf = "cwe_120_2, scanf  :: missing width limit on %%s";

		if (no_display)
		{	printf("cwe_120_2: %d warnings: %s\n", w_cnt, buf);
		} else
		{	for (i = 0; i < Ncore; i++)
			{	cwe120_check_list(thr[i].Scanf, buf);
	}	}	}
}

void
report_120_3(void)
{	int i, w_cnt = 0;
	Results120 *r;

	for (i = 0; i < Ncore; i++)
	{	for (r = results[i]; r; r = r->nxt)
		{	if (no_display)
			{	w_cnt++;
				continue;
			}
			printf("%s:%d: cwe_120_3 %s :: buffer overrun, ",
				r->mycur->fnm, r->mycur->lnr, r->fc->txt);
			printf("%s is size %d but %s is size %d\n",
				r->dst->txt, r->dsc, r->src->txt, r->ssc);
		}
		results[i] = 0;
	}

	if (no_display
	&&  w_cnt > 0)
	{	printf("cwe_120_3: %d warnings: potential buffer overrun\n", w_cnt);
	}
}

void
cwe120_1(void)
{
	cwe120_init();			// single_core
	run_threads(cwe120_1_run);	// multi-core
	report_120_1();			// single-core
}

void
cwe120_2(void)
{
	cwe120_init();			// single_core
	run_threads(cwe120_2_run);	// multi-core
	report_120_2();			// single-core
}

void
cwe120_3(void)
{
	cwe120_init();			// single_core
	run_threads(cwe120_3_run);	// multi-core
	report_120_3();			// single core:
}
