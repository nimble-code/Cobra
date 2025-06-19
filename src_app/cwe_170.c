#include "cwe.h"

// CWE-170: improper null termination

typedef struct TrackValues TrackValues;
typedef struct ThreadLocal170 ThreadLocal170;

struct TrackValues {
	Prim		*t;
	int		 n1;
	int		 n2;
	int		 val;
	TrackValues	*nxt;
};

struct ThreadLocal170 {
	int w_cnt;
	int Curly[128];
};

static ThreadLocal170 *thr;
static int first_e = 1;

extern TokRange **tokrange;	// c_util.c

static void
cwe170_init(void)
{	static int lastN = 0;

	if (lastN < Ncore)
	{	thr = (ThreadLocal170 *) emalloc(Ncore * sizeof(ThreadLocal170));
		lastN = Ncore;
	} else
	{	memset(thr, 0, Ncore * sizeof(ThreadLocal170));
	}
}

static void
store_var_vals(TrackValues **lst, Prim *v, int n1, int n2, int val, int cid)
{	TrackValues *p, *n, *prv = NULL;

	assert(lst != NULL);
	assert(v != NULL);

	for (p = *lst; p; prv = p, p = p->nxt)
	{	if (strcmp(v->txt, p->t->txt) < 0)
		{	break;	// insert at this point
	}	}

	n = (TrackValues *) hmalloc(sizeof(TrackValues), cid);
	n->t   = v;	// array basnename v->txt
	n->n1  = n1;	// nesting level
	n->n2  = n2;	// block number
	n->val = val;	// declared size
	if (prv)
	{	n->nxt = prv->nxt;
		prv->nxt = n;
	} else
	{	n->nxt = *lst;
		*lst = n;
	}
}

static int
is_stored_value(TrackValues *lst, Prim *v, int n1, int n2)
{	TrackValues *p;
	int c;

	for (p = lst; p; p = p->nxt)
	{	c = strcmp(v->txt, p->t->txt);
		if (c == 0
		&&  (p->n1 < n1			// n1: nesting level of use, p->n1: nl of decl
		||   (p->n1 == n1		// nesting level matches
		&&    p->n2 == n2))		// block number matches
		&&  strcmp(v->fnm, p->t->fnm) == 0)
		{	return p->val;
		}
		if (c < 0)
		{	break;
	}	}

	return 0;
}

static void
cwe170_range(Prim *from, Prim *upto, int cid)
{	Prim *vnm=0, *dst=0, *mycur;
	TrackValues *strings = 0;
	int bnr = 0, dsz;

	mycur = from;
	while (mycur && mycur->seq < upto->seq && mycur_nxt())
	{	// look for simple character array declarations with known size
		if (mymatch("{"))
		{	thr[cid].Curly[mycur->curly+1]++;	// block number
		}
		if (mymatch("char"))
		{	mycur_nxt();
			if (mytype("ident"))
			{	vnm = mycur;			// the array name
				mycur_nxt();
				if (mymatch("["))
				{	mycur_nxt();
					if (mytype("const_int"))	// store name, size, and blocknr
					{	bnr = thr[cid].Curly[mycur->curly];
						store_var_vals(&strings, vnm, mycur->curly, bnr, atoi(mycur->txt), cid);
		}	}	}	}
	
		if (!mymatch("strncpy"))	// look for calls of strncpy
		{	continue;
		}
	
		mycur_nxt();			// (
		mycur_nxt();			// mytype("ident")
		if (!mytype("ident"))
		{	continue;
		}
		dst = mycur;			// dest of copy
		while (!mymatch(","))
		{	mycur_nxt();
		}
		// first ,
		mycur_nxt();			// skip over the src arg, to match the 3rd
		while (!mymatch(","))
		{	mycur_nxt();
		}
		// second ,
		mycur_nxt();			// size argument of the strncpy
	
		if (mymatch("sizeof"))		// find sizeof ( what )
		{	mycur_nxt();		// (
			mycur_nxt();		// mytype("ident")
			if (mytype("ident")
			&&  mymatch(dst->txt))
			{	mycur_nxt();	// )

				while (!mymatch(")"))
				{	mycur_nxt();
				}

				mycur_nxt();	// should be - 1
				if (!mymatch("-"))
				{	mycur->mark = 170;
					mycur->bound = dst;
			}	}
		} else
		{	if (mytype("const_int"))
			{	dsz = is_stored_value(strings, dst, mycur->curly, bnr);	// find size of dest
				if (dsz > 0
				&&  dsz <= atoi(mycur->txt))
				{	mycur->mark = 170;
					mycur->bound = dst;
	}	}	}	}
}

static void *
cwe170_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	cwe170_range(from, upto, *i);

	return NULL;
}

static void
cwe170_report(void)
{	Prim *mycur = prim;
	int w_cnt = 0;
	int at_least_one = 0;

	if (json_format && !no_display)
	{	for (; mycur; mycur = mycur->nxt)
		{	if (mycur->mark == 170
			&&  mycur->bound)
			{	at_least_one = 1;
				printf("[\n");
				break;
	}	}	}

	for (; mycur; mycur = mycur->nxt)
	{	if (mycur->mark == 170
		&&  mycur->bound)
		{	if (no_display)
			{	w_cnt++;
			} else
			{	sprintf(json_msg, "'%s' in strncpy may not be null terminated",
					mycur->bound->txt);
				if (json_format)
				{	json_match("", "cwe_170", json_msg, mycur, 0, first_e);
					first_e = 0;
				} else
				{	printf("%s:%d: cwe_170: %s\n",
						mycur->fnm, mycur->lnr, json_msg);
			}	}
			mycur->mark = 0;
			mycur->bound = NULL;
	}	}

	if (no_display && w_cnt > 0)
	{	fprintf(stderr, "cwe_170: %d warnings: destination in strncpy may not be null terminated\n",
			w_cnt);
	}
	if (at_least_one)	// implies json_format
	{	printf("\n]\n");
	}
}

void
cwe170_0(void)
{
	cwe170_init();
	run_threads(cwe170_run);
	cwe170_report();
}
