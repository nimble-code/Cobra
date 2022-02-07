// CWE-197: numeric truncation error
// occurs when a primitive is cast to a primitive of a smaller size and data is lost in the conversion.

// example checker for trapping the assignment or casting of int or long to short or char
// can be extended with similar checks for other combinations of basic types

#include "cwe.h"

typedef struct TrackPtr TrackPtr;
typedef struct ThreadLocal197 ThreadLocal197;

struct TrackPtr {
        Lnrs     *lst;
        Prim     *t;
        Prim     *ptr;
        TrackPtr *nxt;
};

struct ThreadLocal197 {
	TrackPtr *Target;
	char *ofnm;
};

static ThreadLocal197 *thr;

extern TokRange **tokrange;	// cwe_util.c

static void
cwe197_init(void)
{	static int lastN = 0;

	if (lastN < Ncore)
	{	thr = (ThreadLocal197 *) emalloc(Ncore * sizeof(ThreadLocal197));
		lastN = Ncore;
	} else
	{	memset(thr, 0, Ncore * sizeof(ThreadLocal197));
	}
}

static void
store_ptr(TrackPtr **lst, Prim *v, Prim *ptr, int cid)
{	TrackPtr *p, *n, *prv = NULL;

	assert(lst && v);

	for (p = *lst; p; prv = p, p = p->nxt)
	{	if (strcmp(v->txt, p->t->txt) < 0)
		{	break;
	}	}

	n = (TrackPtr *) hmalloc(sizeof(TrackPtr), cid);
	n->t = v;
	n->ptr = ptr;

	if (prv)
	{	n->nxt = prv->nxt;
		prv->nxt = n;
	} else
	{	n->nxt = *lst;
		*lst = n;
	}
}

static Prim *
is_ptr(TrackPtr *lst, Prim *v)
{	TrackPtr *p;
	int c;

	for (p = lst; p; p = p->nxt)
	{	c = strcmp(v->txt, p->t->txt);
		if (c == 0)
		{	return p->ptr;
		}
		if (c < 0)
		{	break;
	}	}
	return NULL;
}

static void
cwe197_range(Prim *from, Prim *upto, int cid)
{	Prim *f, *start, *limit, *btype, *mycur;
	Prim *x, *y, *dst, *a2;

	assert(upto != NULL);

	mycur = from;
	thr[cid].ofnm = mycur->fnm;

	while (mycur && mycur->seq < upto->seq && mycur_nxt())
	{	if (mycur->mark == 0)		// not start of fct def
		{	continue;
		}
		if (mycur->curly == 0
		||  strcmp(mycur->fnm, thr[cid].ofnm) != 0)
		{	thr[cid].Target = 0;
			thr[cid].ofnm = mycur->fnm;
		}

		f = mycur;			// fctname (, at fctname
		while (!mymatch("{"))
		{	mycur_nxt();
		}
		start = mycur;
		limit = mycur->jmp;		// end of function body
		if (!limit)
		{	continue;
		}

		mycur = f->nxt;			// scan also parameter declarations

		// 1. find things in each fct declared as scalar int
		while (mycur->seq < limit->seq)
		{	if (mymatch("int")
			||  mymatch("long")
			||  mymatch("size_t")
			||  mymatch("char")
			||  mymatch("short"))
			{
				if (mycur->curly > 0	// not a param decl
				&&  mycur->round > 0)	// likely a cast
				{	mycur_nxt();
					continue;
				}

				btype = mycur;
				mycur_nxt();
				while (!mymatch(";") && !mytype("oper")
				&&    (!mymatch(")") || mycur->curly > 0))
				{	if (mytype("ident"))
					{	store_ptr(&(thr[cid].Target), mycur, btype, cid);
						mycur_nxt();
						while (!mymatch(",")
						&&     !mymatch(";")
						&&    (!mymatch(")") || mycur->curly > 0))
						{	mycur_nxt();
						}
					} else
					{	if (mymatch("*") || mymatch("="))
						{	while (!mymatch(",")
							&&     !mymatch(";")
							&&    (!mymatch(")") || mycur->curly > 0))
							{	if (mymatch("("))
								{	mycur = mycur->jmp;
								}
								mycur_nxt();
							}
							mycur_nxt();
							break;
						} else
						{	if (mymatch("[") || mymatch("("))
							{	mycur = mycur->jmp;
							} else
							{	mycur_nxt();
			}	}	}	}	}
			mycur_nxt();
		}

		// 2. now recheck function for casts and assignments
		mycur = start;
		while (mycur->seq < limit->seq)			// first check for explicit casts
		{	if (mymatch("("))
			{	mycur_nxt();
				if (mymatch("short")
				||  mymatch("char"))		// cast to short or char
				{	mycur_nxt();		// )
					if (!mymatch(","))	// not a cast
					{	mycur_nxt();
						if (mytype("ident"))
						{	x = is_ptr(thr[cid].Target, mycur);
							if (x != NULL
							&& (strcmp(x->txt, "int") == 0
							 || strcmp(x->txt, "long") == 0
							 || strcmp(x->txt, "size_t") == 0))	// from int or long
							{	mycur->mark = 1970;
			}	}	}	}	}
			mycur_nxt();
		}

		// 3. check for assignments without cast
		mycur = start;
		while (mycur->seq < limit->seq)
		{	if (mymatch("="))
			{	mycur = mycur->prv;		// dst var
				dst = mycur;
				x = is_ptr(thr[cid].Target, mycur);	// do we know the type?
				mycur_nxt();			// undo prv - to point at '=' again
	
				if (x != NULL			// we know the type
				&& (strcmp(x->txt, "short") == 0
				||  strcmp(x->txt, "char") == 0))	 // assignment to short or char
				{	mycur_nxt();		// point at the rhs
					if (mytype("ident"))			// no cast
					{	a2 = mycur->nxt;		// look ahead
						if (strcmp(a2->txt, "?") == 0)	// ternary expr
						{	mycur = a2->nxt;	// look only at the true part
							if (!mytype("ident"))
							{	continue;
						}	}
	
						y = is_ptr(thr[cid].Target, mycur);
						if (y != NULL
						&&  x->lnr <= dst->lnr
						&& (strcmp(y->txt, "int") == 0
						 || strcmp(y->txt, "long") == 0
						 || strcmp(y->txt, "size_t") == 0))	// from int or long
						{	mycur->mark = 197;
							mycur->bound = dst;
							mycur->jmp = x;
			}	}	}	}
			mycur_nxt();
		}
		mycur = limit;
	}
}

static void
cwe197_report(void)
{	Prim *mycur = prim;
	int w_cnt = 0, b_cnt = 0;
	int at_least_one = 0;

	if (json_format && !no_display)
	{	for (; mycur; mycur = mycur->nxt)
		{	if (mycur->mark == 1970
			|| (mycur->mark == 197
			&&  mycur->bound
			&&  mycur->jmp))
			{	at_least_one = 1;
				printf("[\n");
				break;
	}	}	}

	for (; mycur; mycur = mycur->nxt)
	{	if (mycur->mark == 1970)
		{	if (no_display)
			{	b_cnt++;
			} else
			{	sprintf(json_msg, "potential loss of information in cast of %s",
					mycur->txt);
				if (json_format)
				{	json_match("cwe_197b", json_msg, mycur, 0);
				} else
				{	printf("%s:%d: cwe_197b, %s",
						mycur->fnm, mycur->lnr, json_msg);
			}	}
			mycur->mark = 0;
		} else
		if (mycur->mark == 197
		&&  mycur->bound
		&&  mycur->jmp)
		{	if (no_display)
			{	w_cnt++;
			} else
			{	Prim *dst, *x;
				const char *caption = "potential loss of information in assignment from";
				dst = mycur->bound;
				x   = mycur->jmp;

				sprintf(json_msg, "%s '%s' to '%s' type: %s set at %s:%d",
					caption,
					mycur->txt, dst->txt, x->txt, x->fnm, x->lnr);

				if (json_format)
				{	json_match("cwe_197a", json_msg, mycur, 0);
				} else
				{	printf("%s:%d: cwe_197a, %s\n",
						mycur->fnm, mycur->lnr, json_msg);
			}	}
			mycur->mark  = 0;
			mycur->bound = NULL;
			mycur->jmp   = NULL;
	}	}

	if (no_display)
	{	if (w_cnt > 0)
		{	fprintf(stderr, "cwe_197a: %d warnings: potential loss of information in assignment\n",
				w_cnt);
		}
		if (b_cnt > 0)
		{	fprintf(stderr, "cwe_197b: %d warnings: potential loss of information in cast\n",
				b_cnt);
	}	}
	if (at_least_one)	// implies json_format
	{	printf("\n]\n");
	}
}

static void *
cwe197_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	cwe197_range(from, upto, *i);

	return NULL;
}

void
cwe197_0(void)
{
	mark_fcts();
	cwe197_init();
	run_threads(cwe197_run);
	cwe197_report();
}
