#include "cwe.h"

// CWE-119: Improper Restriction of Operations within the Bounds of a Memory Buffer
// pattern 1: multiple array index increments in loop, without bounds checking?
// pattern 2: array indexed with unchecked return value obtained from another function
// pattern 3: index is parameter and is not checked before use in array index for < and >

#define myfind(x)	{ if (!mymatch(x)) { continue; } }

typedef struct ThreadLocal119 ThreadLocal119;

struct ThreadLocal119 {
	Prim *sol, *eol;
	TrackVar *ixvar, *tested, *modified, *suspect, *params;
};

static ThreadLocal119 *thr;

extern TokRange **tokrange;	// cwe_util.c

static void cwe119_check_1(int);
static void cwe119_check_2(int);

static void
cwe119_init(void)
{	static int lastN = 0;

	if (lastN < Ncore)
	{	thr = (ThreadLocal119 *) emalloc(Ncore * sizeof(ThreadLocal119));
		lastN = Ncore;
	} else
	{	memset(thr, 0, Ncore * sizeof(ThreadLocal119));
	}
}

static void
cwe119_1_range(Prim *from, Prim *upto, int cid)	// multiple array index increments in loop, without bound checking
{	Prim *mycur;

	mycur = from;
	while (mycur_nxt() && mycur->seq <= upto->seq)
	{	if (mymatch("for")
		||  mymatch("while"))
		{	thr[cid].sol = mycur;
			mycur_nxt();
			myfind("(");
			mycur = mycur->jmp;
			if (!mycur) { break; }
			myfind(")");
			mycur_nxt();
			// at { or else its a do..while(); or no {}
		} else
		{	myfind("do");
			thr[cid].sol = mycur;
			mycur_nxt();
		}
		myfind("{");

		thr[cid].eol = mycur->jmp;
		mycur_nxt();
		if (thr[cid].eol)
		while (mycur->seq < thr[cid].eol->seq)		// body of loop
		{	if (mymatch("["))
			{	Prim *eob = mycur->jmp;
				mycur_nxt();
				if (eob)
				while (mycur->seq < eob->seq)
				{	if (mytype("ident"))
					{	store_var(&(thr[cid].ixvar), mycur, 0, 0);
					}
					mycur_nxt();
				}
			} else
			{	if (mytype("ident"))
				{	Prim *t = mycur;
					mycur = t->prv;
						if (mymatch("++")
						||  mymatch("--"))
						{	store_var(&(thr[cid].modified), t, 0, 0);
						}
					mycur = t->nxt;
						if (mymatch("++")
						||  mymatch("--"))
						{	store_var(&(thr[cid].modified), t, 0, 0);
						}
					mycur = t;
					if (t->round > 0 && t->bracket == 0)
					{	store_var(&(thr[cid].tested), t, 0, 0);
				}	}
			}
			mycur_nxt();
		}
		// at end of each loop, check and mark
		cwe119_check_1(cid);
	}
	thr[cid].ixvar    = 0;
	thr[cid].modified = 0;
	thr[cid].tested   = 0;
}

static void
cwe119_check_1(int cid)
{	TrackVar *i, *m, *t;
	int c1, c2;

	for (i = thr[cid].ixvar; i; i = i->nxt)
	for (m = thr[cid].modified; m; m = m->nxt)
	{	c1 = strcmp(i->t->txt, m->t->txt);
		if (c1 == 0
		&&  strcmp(i->t->fnm, m->t->fnm) == 0
		&&  m->cnt > 1)	// modified more than once
		{	for (t = thr[cid].tested; t; t = t->nxt)
			{	c2 = strcmp(i->t->txt, t->t->txt);
				if (c2 == 0 && strcmp(t->t->fnm, i->t->fnm) == 0)
				{	break;
				}
				if (c2 < 0)
				{	t = NULL;
					break;
			}	}
			if (!t)		// ixvar not tested
			{	i->t->mark  = 1191;
				i->t->bound = thr[cid].sol;
				i->t->jmp   = thr[cid].eol;
			}
			break;
		}
		if (c1 < 0)
		{	break;
	}	}

	thr[cid].ixvar    = 0;	// could recycle, but this is faster
	thr[cid].modified = 0;
	thr[cid].tested   = 0;
}

static void
cwe119_2_range(Prim *from, Prim *upto, int cid)	// array indexed with unchecked return value from another fct
{	Prim *v = from;
	Prim *mycur, *pt;

	mycur = from;
	while (mycur_nxt() && mycur->seq <= upto->seq)
	{	if (mycur->curly == 0)
		{	cwe119_check_2(cid);
		}
		if (!mytype("ident")
		||  mycur->curly == 00)
		{	continue;
		}
		v = mycur;
		mycur_nxt();
		if (mymatch("="))
		{	mycur_nxt();
			if (mytype("ident")
			&&  strcmp(v->prv->txt, "*") != 0)
			{	mycur_nxt();
				if (mymatch("("))
				{	store_var(&(thr[cid].suspect), v, 0, 0);
					mycur = v;
					continue;
			}	}
			unstore_var(&(thr[cid].suspect), v);
			mycur = v;
			continue;
		} else
		{	mycur = v->prv;
			if (mymatch("&"))
			{	mycur = mycur->prv;
				if (!mytype("ident")
				&&  strcmp(v->nxt->txt, "->") != 0
				&&  strcmp(v->nxt->txt,  ".") != 0)
				{	store_var(&(thr[cid].suspect), v, 0, 0);
				}
				mycur = v->nxt;
				continue;
			}
			mycur = v->nxt;	// undo prv
		}
		if (v->round > 0)
		{	mycur = v->prv;
			if (mytype("oper"))
			{	unstore_var(&(thr[cid].suspect), v);
			}
			mycur = v->nxt;
			if (mytype("oper"))
			{	unstore_var(&(thr[cid].suspect), v);
		}	}

		mycur = v->nxt;
		if (mymatch("["))
		{	Prim *eob = mycur->jmp;
			if (eob)
			while (mycur->seq < eob->seq)
			{	mycur_nxt();
				if (mytype("ident"))
				{	Lnrs *x;

					Prim *nt = mycur->nxt;
					pt = mycur->prv;
					if (strcmp(nt->txt, "->") == 0
					||  strcmp(nt->txt,  ".") == 0
					||  strcmp(nt->txt,  "&") == 0
					||  strcmp(pt->txt, "->") == 0
					||  strcmp(pt->txt,  ".") == 0
					||  strcmp(pt->txt,  "&") == 0)
					{	continue;
					}

					x = is_stored(thr[cid].suspect, mycur, 0);
					while (x != NULL)
					{	if (x->nv->lnr <= mycur->lnr)
						{	store_var(&(thr[cid].ixvar), mycur, 0, 0);
							break;
						}
						x = x->nxt;
		}	}	}	}
	}
	cwe119_check_2(cid);
}

static void
cwe119_check_2(int cid)
{	TrackVar *p, *q;
	Lnrs *z, *y;
	Prim *e;
	int c;

	for (p = thr[cid].suspect; p; p = p->nxt)	// list of var names with values that need a check
	for (q = thr[cid].ixvar; q; q = q->nxt)		// list of var names used in array indices
	{	c = strcmp(p->t->txt, q->t->txt);
		if (c > 0)
		{	continue;
		}
		if (c < 0)
		{	break;
		}
		// (c == 0)
		for (z = p->lst; z; z = z->nxt)	// go through list of dubious names and line nrs
		for (y = q->lst; y; y = y->nxt)	// and the places where they are used in array ixs
		{
			if (z->nv->lnr > y->nv->lnr)
			{	continue;
			}
			// are points of introduction and use
			// of the index in the same fct?
			for (e = z->nv; e->seq < y->nv->seq; e = e->nxt)
			{	if (e->curly == 0)	// not in same fct
				{	break;
			}	}
			if (e->curly == 0)		// no
			{	continue;
			}
			y->nv->mark  = 1192;
			y->nv->bound = z->nv;
	}	}

	thr[cid].ixvar   = 0;
	thr[cid].suspect = 0;
}

void
cwe119_3_range(Prim *from, Prim *upto, int cid)
{	Prim *eop, *eob, *eoi, *mycur;
	Lnrs *zx;

	assert(upto != NULL);

	mycur = from;
	while (mycur_nxt() && mycur->seq <= upto->seq)
	{	if (mycur->mark == 0)
		{	continue;
		}
		thr[cid].params = 0;
		thr[cid].tested = 0;
		mycur_nxt();
		myfind("(");
		eop = mycur->jmp;
		if (!eop
		||  strcmp(eop->txt, ")") != 0
		||   eop->seq == 0)
		{	continue;
		}
		while (mycur->seq <= eop->seq)
		{	if (mytype("ident"))
			{	mycur = mycur->prv;
				if (!mymatch("*")
				&&  !mymatch("struct"))
				{	mycur_nxt();
					if (strncmp(mycur->txt, "yy", 2) != 0)	// yacc output
					{	store_var(&(thr[cid].params), mycur, 0, 0);
					}
				} else
				{	mycur_nxt();
			}	}
			mycur_nxt();
		}
		// {
		while (!mymatch("{"))
		{	mycur_nxt();
		}
		if (!mymatch("{"))
		{	continue;
		}
		eob = mycur->jmp;
		if (eob)
		while (mycur->seq < eob->seq)	// check fct body
		{	if (mycur->round > 0
			&&  mytype("ident")
			&&  is_stored(thr[cid].params, mycur, 0))
			{	Prim *vv = mycur;

				mycur = vv->prv;
				if (mymatch("<")
				||  mymatch("<="))
				{	store_var(&(thr[cid].tested), vv, 2, 0);
				}
				if (mymatch(">")
				||  mymatch(">="))
				{	store_var(&(thr[cid].tested), vv, 1, 0);
				}
				if (mymatch("!"))
				{	store_var(&(thr[cid].tested), vv, 3, 0);
				}

				mycur = vv->nxt;
				if (mymatch("<")
				||  mymatch("<="))
				{	store_var(&(thr[cid].tested), vv, 1, 0);
				}
				if (mymatch(">")
				||  mymatch(">="))
				{	store_var(&(thr[cid].tested), vv, 2, 0);
				}
				if (mymatch("=="))
				{	store_var(&(thr[cid].tested), vv, 3, 0);
				}
			}
			if (mymatch("["))
			{	eoi = mycur->jmp;
				mycur_nxt();
				if (eoi)
				while (mycur->seq <= eoi->seq)
				{	if (mytype("ident")
					&& (zx = is_stored(thr[cid].params, mycur, 0))
					&& !is_stored(thr[cid].tested, mycur, 3))
					{	mycur->mark = 1193;
						do_lock(cid);
						while (zx && zx->nv)
						{	Prim *ntok = (Prim *) hmalloc(sizeof(Prim), cid);
							ntok->lnr = zx->nv->lnr;
							ntok->jmp = zx->nv;
							ntok->nxt = mycur->bound;
							mycur->bound = ntok;
							zx = zx->nxt;
						}
						do_unlock(cid);
					}
					mycur_nxt();
				}
				mycur = mycur->prv;
			}
			mycur_nxt();
	}	}

	thr[cid].params = 0;
	thr[cid].tested = 0;
}

static void *
cwe119_1_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	cwe119_1_range(from, upto, *i);

	return NULL;
}

static void *
cwe119_2_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	cwe119_2_range(from, upto, *i);

	return NULL;
}

static void *
cwe119_3_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	cwe119_3_range(from, upto, *i);

	return NULL;
}

static void
report_119_1(void)
{	Prim *mycur = prim;
	int w_cnt = 0;
	int at_least_one = 0;

	if (json_format && !no_display)
	{	for (; mycur; mycur = mycur->nxt)
		{	if (mycur->mark == 1191
			&&  mycur->bound
			&&  mycur->jmp)
			{	at_least_one = 1;
				printf("[\n");
				break;
	}	}	}

	for (; mycur; mycur = mycur->nxt)
	{	if (mycur->mark == 1191)
		{	if (no_display)
			{	w_cnt++;
			} else if (mycur->bound && mycur->jmp)
			{	const char *caption = " modified multiple times in loop ";
				sprintf(json_msg, "array-index variable '%s'%sln %d-%d",
					mycur->txt, caption,
					mycur->bound->lnr, mycur->jmp->lnr);
				if (json_format)
				{	json_match("cwe_119_1", json_msg, mycur, 0);
				} else
				{	printf("%s:%d: cwe_119_1: %s\n",
						mycur->fnm, mycur->lnr, json_msg);
			}	}
			mycur->bound = 0;
			mycur->jmp = 0;
			mycur->mark = 0;
	}	}

	if (no_display
	&&  w_cnt > 0)
	{	fprintf(stderr, "cwe_119_1: %d warnings: array-index variable ", w_cnt);
		fprintf(stderr, "modified multiple times in loop\n");
	}
	if (at_least_one)	// implies json_format
	{	printf("\n]\n");
	}
}

static void
report_119_2(void)
{	Prim *mycur = prim;
	int w_cnt = 0;
	int lastone = 0;	// limit nr of reports for single issue
	int at_least_one = 0;

	if (json_format && !no_display)
	{	for (; mycur; mycur = mycur->nxt)
		{	if (mycur->mark == 1192)
			{	at_least_one = 1;
				printf("[\n");
				break;
	}	}	}

	for (; mycur; mycur = mycur->nxt)
	{	if (mycur->mark == 1192)
		{	if (no_display)	// terse
			{	if (!mycur->bound || mycur->bound->lnr != lastone)
				{	w_cnt++;
				}
				if (mycur->bound)
				{	lastone = mycur->bound->lnr;
				}
			} else if (mycur->bound && mycur->bound->lnr != lastone)
			{	const char *caption = "has unchecked min/max value (cf lnr";
				sprintf(json_msg, "array-index variable '%s' %s %d)",
					mycur->txt, caption, mycur->bound->lnr);
				if (json_format)
				{	json_match("cwe_119_2", json_msg, mycur, 0);
				} else
				{	printf("%s:%d: cwe_119_2: %s\n",
						mycur->fnm, mycur->lnr, json_msg);
				}
				lastone = mycur->bound->lnr;
				mycur->bound = 0;
			}
			mycur->mark = 0;
	}	}

	if (no_display
	&&  w_cnt > 0)
	{	fprintf(stderr, "cwe_119_2: %d warnings: ", w_cnt);
		fprintf(stderr, "array-index variable has unchecked min/max value\n");
	}
	if (at_least_one)	// implies json_format
	{	printf("\n]\n");
	}
}

static void
report_119_3(void)
{	Prim *mycur = prim, *w;
	int w_cnt = 0;
	int at_least_one = 0;

	if (json_format && !no_display)
	{	for (; mycur; mycur = mycur->nxt)
		{	if (mycur->mark == 1193)
			{	at_least_one = 1;
				printf("[\n");
				break;
	}	}	}

	for (; mycur; mycur = mycur->nxt)
	{	if (mycur->mark == 1193)
		{	w_cnt++;
			if (!mycur->bound)
			{	if (!no_display)
				{	const char *caption = "min/max unchecked parameter value";
					sprintf(json_msg, "%s '%s' used in array index",
						caption, mycur->txt);
					if (json_format)
					{	json_match("cwe_119_3", json_msg, mycur, 0);
					} else
					{	printf("%s:%d: cwe_119_3: %s\n",
							mycur->fnm, mycur->lnr, json_msg);
				}	}
			} else
			for (w = mycur->bound; w; w = w->nxt)
			{	if (w->jmp)	// ptr to param declaration
				{	if (w->jmp->mark == 11930)
					{	w_cnt--;
						continue;
					}
					w->jmp->mark = 11930; // one warning per param
				}
				if (!no_display)
				{	const char *caption = "min/max unchecked parameter value";
					sprintf(json_msg, "%s '%s' used in array index (cf lnr %d)",
						caption, mycur->txt, w->lnr);
					if (json_format)
					{	json_match("cwe_119_3", json_msg, mycur, 0);
					} else
					{	printf("%s:%d: cwe_119_3: %s\n",
							mycur->fnm, mycur->lnr, json_msg);
			}	}	}
			mycur->mark  = 0;
			mycur->bound = 0;
	}	}

	for (mycur = prim; mycur; mycur = mycur->nxt)
	{	if (mycur->mark == 11930)
		{	mycur->mark = 0;
	}	}

	if (no_display
	&&  w_cnt > 0)
	{	fprintf(stderr, "cwe_119_3: %d warnings: unchecked parameter value used in array-index\n",
			w_cnt);
	}
	if (at_least_one)	// implies json_format
	{	printf("\n]\n");
	}
}

void
cwe119_1(void)
{
	cwe119_init();			// single_core
	run_threads(cwe119_1_run);	// multi-core
	report_119_1();			// single-core
}

void
cwe119_2(void)
{
	cwe119_init();			// single_core
	run_threads(cwe119_2_run);	// multi-core
	report_119_2();			// single-core
}

void
cwe119_3(void)
{
	mark_fcts();
	cwe119_init();			// single_core
	run_threads(cwe119_3_run);	// multi-core
	report_119_3();			// single-core
}
