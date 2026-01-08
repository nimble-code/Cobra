#include "c_api.h"
#include "find_taint.h"

typedef struct ThreadLocal_taint {
	Tainted *tainted_param[TPSZ];
	Tainted *tainted_return[TPSZ];
} ThreadLocal_taint;

static ThreadLocal_taint *thr;	// thr[cid].tainted_param[h]

static unsigned int
t_hash(const char *s)
{	unsigned int h = 0x88888EEFL;
	const char t = *s++;

	while (*s != '\0')
	{	h ^= ((h << 8) ^ (h >> (24))) + *s++;
	}
	return ((h << 7) ^ (h >> (25))) ^ t;
}

static void
TaintedReturn(const char *s, const int cid)
{	Tainted *t, *prv = 0;	// remember which fcts can return a tainted result
	int h, k;

	h = t_hash(s)&(TPSZ-1);
	for (t = thr[cid].tainted_return[h]; t; prv = t, t = t->nxt)
	{	k = strcmp(t->fnm, s);
		if (k > 0)
		{	break;
		}
		if (k == 0)
		{	return;
	}	}

	t = (Tainted *) hmalloc(sizeof(Tainted), cid, 13);
	t->fnm = (char *) hmalloc(strlen(s)+1, cid, 14);
	strcpy(t->fnm, s);
	if (prv == NULL)	// new first entry
	{	t->nxt = thr[cid].tainted_return[h];
		thr[cid].tainted_return[h] = t;
	} else	// insert after prv
	{	t->nxt = prv->nxt;
		prv->nxt = t;
	}

	if (verbose)
	{	do_lock(cid, 19);
		fprintf(stderr, "\tTaintedReturn %s\n", s);
		do_unlock(cid, 19);
	}
}

void
taint_init(void)
{	static int lastN = 0;

	if (lastN < Ncore)
	{	thr = (ThreadLocal_taint *) emalloc(Ncore * sizeof(ThreadLocal_taint), 19);
		lastN = Ncore;
	} else
	{	memset(thr, 0, Ncore * sizeof(ThreadLocal_taint));
	}
}

void
param_is_tainted(Prim *p, const int pos, Prim *nm, const int cid)
{	Tainted *t, *prv = 0;
	char *s;
	int h, k;

	if (!p)
	{	return;
	}

	s = p->txt;
	p->bound = nm->bound?nm->bound:nm;

	h = t_hash(s)&(TPSZ-1);

	// remember which params are potentially tainted
	for (t = thr[cid].tainted_param[h]; t; prv = t, t = t->nxt)	// sorted
	{	k = strcmp(t->fnm, s);
		if (k > 0)
		{	break;
		}
		if (k == 0
		&&  t->src == p
		&&  t->pos == pos)
		{	return; // already there
	}	}

	t = (Tainted *) hmalloc(sizeof(Tainted), cid, 15);
	t->fnm = (char *) hmalloc(strlen(s)+1, cid, 16);
	strcpy(t->fnm, s);
	t->src = p;	// where the alert originated
	t->param = nm;
	t->pos = pos;

	if (prv == NULL)	// new first entry
	{	t->nxt = thr[cid].tainted_param[h];
		thr[cid].tainted_param[h] = t;
	} else	// insert after prv
	{	t->nxt = prv->nxt;
		prv->nxt = t;
	}

	if (verbose)
	{	do_lock(cid, 20);
		fprintf(stderr, "%s:%d:\tparam_is_tainted %s( param %d = %s )\n",
			p->fnm, p->lnr, s, pos, nm->txt);
		do_unlock(cid, 20);
	}
}

int
is_param_tainted(Prim *mycur, const int setlimit, const int cid)
{	Prim *r, *ocur = mycur;
	Tainted *t, *best_t=NULL;
	int pos, cnt = 0, level, k, h, i, n;
	int b, bestmatch = -1;

	// mycur->txt is  fct name, at start of fct def, PropViaFct
	// the marked formal parameters are vulnerable targets,
	// so check if the params are assigned to somewhere in the fct

	if (strcmp(mycur->txt, "main") == 0)	// cannot be called directly
	{	return 0;			// params like argv are handled separately
	}

	if (verbose > 1)
	{	do_lock(cid, 21);
		fprintf(stderr, "%d: %s:%d: check fct body of %s() for possibly bad params\n",
			cid, mycur->fnm, mycur->lnr, mycur->txt);
		do_unlock(cid, 21);
	}

	h = t_hash(mycur->txt)&(TPSZ-1);

	for (i = cid, n = 0; n < Ncore && !best_t; n++, i = (i+1)%Ncore)	// must check all, start with own
	for (t = thr[i].tainted_param[h]; t; t = t->nxt)		// fcts using marked vars as params
	{	k = strcmp(t->fnm, mycur->txt);
		if (k > 0)
		{	break;	// mycur->txt is not in the (sorted) list
		}
		if (k != 0	// if fct is on the bad list
		||  t->handled)	// it could propagate taints
		{	continue;
		}

		// if mycur->txt is a file static fct, then
		// the caller at t->src must be in the same file
		if (strcmp(t->src->fnm, mycur->fnm) != 0) // different file
		{	// check that mycur->txt is not preceded by "static"
			Prim *z = mycur->prv;	// skip over typenames and * decorations
			while (z
			&&	z->curly == 0	// preceding fct name
			&&	(strcmp(z->typ, "type") == 0
			||       strcmp(z->txt, "*") == 0
			||	 strncmp(z->txt, "__", 2) == 0		// eg __init
			||	 strcmp(z->typ, "modifier") == 0))
			{	z = z->prv;
			}
			if (z
			&&  strcmp(z->txt, "static") == 0)	// typ: @storage
			{	continue;			// fct not visible to caller
		}	}

		b = abs(t->src->seq - mycur->seq);
		if (bestmatch < 0
		||  b < bestmatch)
		{	bestmatch = b;
			best_t = t;
	}	}

	if (0
	&&  bestmatch > 0)
	{	do_lock(cid, 22);
		fprintf(stderr, "%d: %d	BM %s -> <%s> -> %s\n",
			cid, bestmatch, mycur->fnm, mycur->txt, best_t->src->fnm);
		do_unlock(cid, 22);
	}

	if (bestmatch > 0
	&&  (!setlimit || bestmatch <= setlimit))
	{	t = best_t;
		t->handled = 1;
		if (verbose)
		{	do_lock(cid, 23);
			fprintf(stderr, "%d: %s:%d:\tchecking fct %s()==%s() for use of tainted param nr %d -- %s\n",
				cid, mycur->fnm, mycur->lnr, mycur->txt, t->fnm, t->pos, ocur->txt);
			do_unlock(cid, 23);
		}
		pos = 1;
		level = 1 + mycur->round;	// to see if we've reached the end of the formal params
more:
		while (mycur->round > level
		||    (strcmp(mycur->txt, ")") != 0
		&&     strcmp(mycur->txt, ",") != 0))
		{	mycur = mycur->nxt;		// move to next param
		}
		if (pos == t->pos)		// the position of a vulnerable param
		{	mycur = mycur->prv;
			if (mycur
			&&  strcmp(mycur->typ, "ident") == 0)	// the (formal) param name, before the comma
			{			// check the fct body for uses of this param
				if (verbose > 1)
				{	do_lock(cid, 24);
					fprintf(stderr, "%d: cnt %d :: param check: %s (param pos %d = %s)\n",
						cid, pos, t->fnm, t->pos, mycur->txt);
					do_unlock(cid, 24);
				}
				r = mycur->nxt;
				while (strcmp(r->txt, "{") != 0)
				{	r = r->nxt;
				}
				r = r->nxt;
				while (r->curly >= 1)				// search fct body
				{	if (strcmp(r->txt, "sizeof") == 0)	// args need no mark
					{	r = r->nxt;
						if (r->jmp)
						{	r = r->jmp;
					}	}
					if (strcmp(r->txt, mycur->txt) == 0	// same ident
					&&  r->mark == 0)			// not yet marked
					{	r->mark |= PropViaFct;		// new taint mark
						if (verbose)
						{	show_bindings("is_param_tainted", r, t->param, cid);
						}
						if (!r->bound)		// for traceback
						{	r->bound = t->param;
						}
						cnt++;
						if (verbose > 1)
						{	do_lock(cid, 25);
							fprintf(stderr, "%d: %s:%d: use of marked param %s -> +%d\n",
								cid, r->fnm, r->lnr, mycur->txt, PropViaFct);
							do_unlock(cid, 25);
					}	}
					r = r->nxt;
			}	}
			mycur = ocur;		// restore; done with this param
		} else if (strcmp(mycur->txt, ")") != 0)
		{	mycur = mycur->nxt;
			pos++;
			if (pos <= t->pos)
			{	goto more;	// move to next position
	}	}	}

	if (verbose && cnt > 0)
	{	do_lock(cid, 26);
		fprintf(stderr, "%d: found %d uses of marked params (adding mark %d)\n",
			cid, cnt, PropViaFct);
		do_unlock(cid, 26);
	}

	return cnt;
}

void
search_returns(Prim *mycur, const int cid)
{	Prim *r, *s;
	int cnt = 0;
	char *nm = mycur->txt;

	r = mycur->nxt;
	while (r && r->curly == 0)
	{	r = r->nxt;
	}

	while (r && r->curly > 0)	// search the fct body for return stmnts
	{	if (strcmp(r->txt, "return") == 0)
		{	s = r->nxt;
			while (s && strcmp(s->txt, ";") != 0)
			{	if (s->mark != 0)
				{	TaintedReturn(nm, cid); // fct nm can return tainted result
					cnt++;
					break;
				}
				s = s->nxt;
		}	}
		r = r->nxt;
	}

	if (verbose && cnt > 0)
	{	do_lock(cid, 27);
		fprintf(stderr, "search returns of %s\n", nm);
		do_unlock(cid, 27);
	}
}

int
is_return_tainted(const char *s, int cid)	// runs in parallel, but needs to see all entries
{	Tainted *t;
	int h, k, i, n;

	h = t_hash(s)&(TPSZ-1);
	for (i = cid, n = 0; n < Ncore; n++, i = (i+1)%Ncore) // start with own table
	for (t = thr[i].tainted_return[h]; t; t = t->nxt)
	{	k = strcmp(t->fnm, s);
		if (k > 0)
		{	break;
		}
		if (k == 0)
		{	return 1;
	}	}
	return 0;
}

void
reset_tables(void)
{
	memset(thr, 0, Ncore * sizeof(ThreadLocal_taint));
}
