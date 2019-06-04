/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

#include "cobra.h"

// functions for setting links for if/else/switch/case/break statements
// similar to those as used in play/goto_links.cobra etc

#define mycur_nxt()	(mycur = mycur?mycur->nxt:NULL)
#define mymatch(s)      (strcmp(mycur->txt, (s)) == 0)
#define mytype(s)       (strcmp(mycur->typ, (s)) == 0)

typedef struct TrackVar	TrackVar;
typedef struct Lnrs	Lnrs;
typedef struct ThreadLocal_links ThreadLocal_links;

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

struct ThreadLocal_links {	// break_links
	int n;			// nesting level of break destinations
	int bad_cnt, good_cnt;	// self-test
	Prim *breakdest;
	Prim *remem[128];
};

static int seen_goto_links;
static int seen_else_links;
static int seen_switch_links;
static int seen_break_links;

static Lnrs *store_var(TrackVar **, Prim *, int, int);
static ThreadLocal_links *thr;

static Prim *
skip_cond(Prim *mycur)
{
	while (!mymatch("("))
	{	mycur_nxt();
	}
	if (!mycur->jmp)
	{	return NULL;
	}
	mycur = mycur->jmp;
	mycur_nxt();
	while (mytype("cmnt"))
	{	mycur_nxt();
	}
	return mycur;
}

static Prim *
skip_stmnt(Prim *mycur)
{	Prim *t, *q;
	int lev;

L1:	if (mymatch("{"))
	{	if (!mycur->jmp)
		{	return NULL;
		}
		mycur = mycur->jmp;
		mycur_nxt();
		if (mymatch("else"))
		{	mycur_nxt();
			goto L1;
		}
		return mycur;
	}

	if (mymatch("if")
	||  mymatch("for")
	||  mymatch("switch")
	||  mymatch("while"))
	{	mycur = skip_cond(mycur);
		if (!mycur)
		{	return NULL;
		}
		goto L1;
	}
	if (mymatch("else"))
	{	mycur_nxt();
		goto L1;
	}

	if (mymatch("do"))	// find matching while (...)
	{	q = mycur;
		lev = 1;
		while (1)
		{	t = mycur->nxt;
			if (!t
			||  mycur->seq == t->seq)
			{	return NULL;
			}
			mycur_nxt();
			if (mycur->curly == q->curly
			&&  mycur->round == q->round
			&&  mycur->bracket == q->bracket)
			{	if (mymatch("do"))
				{	lev++;
				}
				if (mymatch("while"))
				{	lev--;
					if (lev == 0)
					{	break;
		}	}	}	}
		mycur = skip_cond(mycur);
		if (!mycur)
		{	return NULL;
	}	}

	while (!mymatch(";"))
	{	 mycur_nxt();
	}
	mycur_nxt();
	while (mytype("cmnt"))
	{	mycur_nxt();
	}
	return mycur;
}

static int
empty_case_stmnt(Prim *f)
{
	f = f->nxt;	// ident or :
	while (strcmp(f->txt, ":") != 0)
	{	f = f->nxt;
	}
	f = f->nxt;
	while (strcmp(f->typ, "cmnt") == 0)
	{	f = f->nxt;
	}
	if (strcmp(f->txt, "case") == 0
	||  strcmp(f->txt, "default") == 0
	||  strcmp(f->txt, "}") == 0)
	{	return 1;
	}
	return 0;
}

static void
connect_gotos(TrackVar *Gt, TrackVar *Lb)	// should be multi-core safe
{	TrackVar *p, *q;
	Lnrs *lnr, *gnr;
	Prim *s;
	int cmp;

	for (p = Gt; p; p = p->nxt)
	{	for (q = Lb; q; q = q->nxt)
		{	cmp = strcmp(p->t->txt, q->t->txt);	// goto vs label
			if (cmp > 0)
			{	continue;
			}
			if (cmp < 0)
			{	break;
			}
			// (cmp == 0)
			for (gnr = p->lst; gnr; gnr = gnr->nxt)
			for (lnr = q->lst; lnr; lnr = lnr->nxt)
			{	if (lnr->fnr == gnr->fnr)	// same fct-block
				{	s = gnr->nv->prv;	// location of goto
					s->bound = lnr->nv;
					break;
	}	}	}	}
}

static Lnrs *
store_var(TrackVar **lst, Prim *v, int tag, int cid)
{	TrackVar *p, *n, *prv = NULL;
	Lnrs *q = NULL;
	int c;

	// if tag == 0: count occurrences
	// if tag != 0; or in that value

	assert(lst && v);

	do_lock(cid);
	for (p = *lst; p; prv = p, p = p->nxt)
	{	if (!p->t || !p->t->txt)
		{	continue;	// likely multicore bug
		}
		c = strcmp(v->txt, p->t->txt);
		if (c == 0
		&&  strcmp(p->t->fnm, v->fnm) == 0)
		{	if (tag)
			{	p->cnt |= tag;
			} else
			{	p->cnt++;
			}
			if (v->lnr != p->t->lnr)
			{	q = (Lnrs *) hmalloc(sizeof(Lnrs), cid);
				q->nxt = p->lst;
				q->nv  = v;
				p->lst = q;
			}
			do_unlock(cid);

			return q;
		}
		if (c < 0)
		{	break;
	}	}

	n = (TrackVar *) hmalloc(sizeof(TrackVar), cid);
	n->t = v;
	n->lst = (Lnrs *) hmalloc(sizeof(Lnrs), cid);
	n->lst->nv = v;

	if (tag)
	{	n->cnt = tag;
	} else
	{	n->cnt = 1;
	}
	if (prv)
	{	n->nxt = prv->nxt;
		prv->nxt = n;
	} else
	{	n->nxt = *lst;
		*lst = n;
	}
	do_unlock(cid);

	return n->lst;
}

static void
break_links_range(Prim *from, Prim *upto, int cid)
{	Prim *mycur, *b;
	ThreadLocal_links *ptr;

	ptr = &thr[cid];
	ptr->breakdest = from;
	ptr->remem[0]  = from;
	ptr->bad_cnt   = 0;
	ptr->good_cnt  = 0;

	mycur = from;
	while (mycur_nxt() && mycur->seq <= upto->seq)
	{	if (mymatch("}"))
		{	if (mycur->nxt == ptr->breakdest)
			{	if (ptr->n > 0)
				{	ptr->n--;
					ptr->breakdest = ptr->remem[ptr->n];
				} else
				{	ptr->breakdest = from;
			}	}
			if (mycur->curly == 0)
			{	ptr->n = 0;
				ptr->breakdest = from;
				ptr->remem[0] = ptr->breakdest;
			}
			continue;
		}
	
		if (!mytype("key"))
		{	continue;
		}
	
		if (mymatch("while"))
		{	b = mycur->nxt;
			while (strcmp(b->txt, "(") != 0)
			{	b = b->nxt;
			}
			b = b->jmp;
			b = b->nxt;
			if (strcmp(b->txt, ";") == 0)	// it's a do-while
			{	continue;		// was handled before at do
		}	}
	
		if (mymatch("for")
		||  mymatch("while")
		||  mymatch("switch"))
		{
L:			mycur = mycur->nxt;
			while (!mymatch("("))
			{	mycur = mycur->nxt;
			}
			mycur = mycur->jmp;
			mycur = mycur->nxt;
			while (mytype("cmnt"))
			{	mycur = mycur->nxt;
			}
			if (mymatch("{") && mycur->jmp)
			{	ptr->remem[ptr->n] = ptr->breakdest;
				ptr->breakdest = mycur->jmp->nxt;
				ptr->n++;
			} else 				// could be do, while, for, switch
			{	if (mymatch("for")
				||  mymatch("while")
				||  mymatch("switch")
				||  mymatch("if"))
				{	goto L;	// not precise, but somewhat close
				}
				if (mymatch("do"))
				{	goto M;	// ditto
				}
				// if anything else, it cannot contain a break stmnt
			}
			continue;
		}
		if (mymatch("do"))
		{
M:			mycur = mycur->nxt;
			while (mytype("cmnt"))
			{	mycur = mycur->nxt;
			}
			if (mymatch("{") && mycur->jmp)
			{	ptr->remem[ptr->n]  = ptr->breakdest;
				ptr->breakdest = mycur->jmp->nxt;
				ptr->n++;
			} else
			{	if (mymatch("for")
				||  mymatch("while")
				||  mymatch("switch")
				||  mymatch("if"))
				{	goto L;	// not precise, but somewhat close
				}
				if (mymatch("do"))
				{	goto M;	// ditto
				}
				// if anything else, it cannot contain a break stmnt
			}
			continue;
		}
		if (mymatch("break"))
		{	if (ptr->breakdest->seq != from->seq)
			{	if (mycur->bound != NULL)
				{	if (mycur->bound != ptr->breakdest)
					{	ptr->bad_cnt++;
#if 0
					// for debugging in case things dont line up
					printf("%s:%d: %s was linked to %s:%d: %s, iso %s:%d: %s\n",
						mycur->fnm, mycur->lnr, mycur->txt,
						mycur->bound->fnm, mycur->bound->lnr,
						mycur->bound->txt,
						ptr->breakdest->fnm, ptr->breakdest->lnr,
						ptr->breakdest->txt);
					int k; for (k = 0; k <= ptr->n; k++)
						{	if (ptr->remem[k])
							{	Prim *q = ptr->remem[k];
								printf(">> %d  %s:%d %s\n",
									k, q->fnm, q->lnr, q->txt);
						}	}
#endif
					} else
					{	ptr->good_cnt++;
				}	}
				mycur->bound = ptr->breakdest;
			} else if (verbose)
			{	printf("%s:%d: invalid break statement?\n",
					mycur->fnm, mycur->lnr);;
			}
			continue;
	}	}

	if (ptr->n != 0)
	{	printf("cobra: internal error, nesting: n=%d\n", ptr->n);
	}
	if (verbose && ptr->bad_cnt)
	{	printf("bad cnt: %d, good cnt %d\n", ptr->bad_cnt, ptr->good_cnt);
	}
}

static void
goto_links_range(Prim *from, Prim *upto, int cid)
{	TrackVar *Goto = 0;
	TrackVar *Label = 0;
	Lnrs *ln;
	char *ofnm;
	Prim *mycur, *q, *qq;
	int fnr = 0;

	mycur = from;
	ofnm = mycur->fnm;
	while (mycur_nxt() && mycur->seq <= upto->seq)
	{
		if (strcmp(mycur->fnm, ofnm) != 0)	// no need to track across files
		{	connect_gotos(Goto, Label);
			Label = Goto = NULL;		// to keep tables small
			ofnm  = mycur->fnm;
			fnr = 0;
		}

		if (mycur->curly == 0
		&&  mymatch("{"))
		{	fnr++;
		}
	
		if (mymatch("goto"))			// location of goto
		{	mycur_nxt();			// labelname
			ln = store_var(&Goto, mycur, 0, cid);
			if (ln)
			{	ln->fnr = fnr;
		}	}

		if (mymatch(":"))			// possible label
		{	q = mycur->prv;
			if (strcmp(q->typ, "ident") != 0)
			{	continue;		// not a label
			}
			// could be a ternary expression
			qq = q;
			do { q = q->prv;
			} while (strcmp(q->typ, "cmnt") == 0);

			if (q->txt[1] == '\0')
			switch (q->txt[0]) {
			case ';':
			case ':':
			case '}':
			case '{':
			case ')':
				ln = store_var(&Label, qq, 0, cid);
				if (ln)
				{	ln->fnr = fnr;
				}
				break;
			default:
				break;
			}
			continue;
	}	}

	connect_gotos(Goto, Label);
}

static void
else_links_range(Prim *from, Prim *upto, int cid)
{	Prim *q, *mycur;

	mycur = from;
	while (mycur_nxt() && mycur->seq <= upto->seq)
	{	// set mycur->bound for if statements
		// to point to else block, if any, or else
		// the first stmnt after the then-block
		if (mymatch("if"))
		{	q = mycur;
			mycur = skip_cond(q);
			if (!mycur)
			{	break;
			}
			while (mytype("cmnt"))
			{	mycur_nxt();
			}
			if (mymatch("{"))
			{	mycur = mycur->jmp;
				mycur_nxt();
			} else
			{
				mycur = skip_stmnt(mycur);
			}
			if (!mycur)
			{	break;
			}
			if (mymatch("else"))
			{	q->bound = mycur->nxt;
			} else
			{	q->bound = mycur;
			}
			mycur = q;
		} else if (mymatch("else"))	// connect else to first stmnt after else-block
		{	q = mycur;
			mycur_nxt();
			mycur = skip_stmnt(mycur);
			if (!mycur)
			{	break;
			}
			q->bound = mycur;
			mycur = q;
	}	}
}

static void
switch_links_range(Prim *from, Prim *upto, int cid)
{	Prim *mycur, *q, *e, *lastcase = NULL;
	int sawdefault;

	mycur = from;
	while (mycur_nxt() && mycur->seq <= upto->seq)
	{	// set .bound for switch statements at each case
		// to point to the next case, or the stmnt after the switch.
		// the switch keyword itself is bound to the stmnt after the
		// switch body only of there is no default clause (meaning
		// that its execution may be skipped if no case matches)
	
		if (!mymatch("switch"))
		{	continue;
		}
		q = mycur;
		mycur = skip_cond(q);
		if (!mycur
		||  !mymatch("{"))
		{	continue;
		}

		e = mycur->jmp;
		if (!e)
		{	continue;
		}
		lastcase = NULL;
		sawdefault = 0;

		while (mycur->seq < e->seq)	// body of the switch stmnt
		{	if (mycur->curly == e->curly + 1) // top level
			{	if (mymatch("case")
				||  mymatch("default"))
				{	if (mymatch("default"))
					{	sawdefault = 1;
					}
					if (empty_case_stmnt(mycur))
					{	mycur_nxt();
						continue;
					}
					if (lastcase)
					{	lastcase->bound = mycur;
					}
					lastcase = mycur;
				}
				if (mymatch("break"))
				{	mycur->bound = e->nxt;	// added 5/2/19
			}	}
			mycur_nxt();
		}
		// at closing curly brace of switch stmnt
		mycur_nxt();	// first stmnt after switch
		while (mytype("cmnt"))
		{	mycur_nxt();
		}
		if (lastcase)
		{	lastcase->bound = mycur;
		}
		if (!sawdefault)
		{	q->bound = mycur;
		}
		mycur = q;
	}	
}

static void *
goto_links_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	goto_links_range(from, upto, *i);

	return NULL;
}

static void *
else_links_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	else_links_range(from, upto, *i);

	return NULL;
}

static void *
break_links_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	break_links_range(from, upto, *i);

	return NULL;
}

static void *
switch_links_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	switch_links_range(from, upto, *i);

	return NULL;
}

static void
goto_links(void)
{
	if (seen_goto_links)
	{	return;
	}
	seen_goto_links = 1;

	run_threads(goto_links_run, 10);
}

static void
else_links(void)
{
	if (seen_else_links > 0)
	{	return;
	}
	seen_else_links = 1;
	run_threads(else_links_run, 11);
}

static void
switch_links(void)
{
	if (seen_switch_links > 0)
	{	return;
	}
	seen_switch_links = 1;
	run_threads(switch_links_run, 12);
}

static void
break_links(void)
{	static int lastN = 0;

	if (seen_break_links)
	{	return;
	}
	seen_break_links = 1;

	if (lastN < Ncore || !thr)
	{	thr = (ThreadLocal_links *) emalloc(Ncore * sizeof(ThreadLocal_links));
		lastN = Ncore;
	} else
	{	memset(thr, 0, Ncore * sizeof(ThreadLocal_links));
	}

	run_threads(break_links_run, 13);
}

// the only two externally visible functions

void
clear_seen(void)
{
	seen_goto_links = 0;
	seen_else_links = 0;
	seen_switch_links = 0;
	seen_break_links = 0;
}

void
set_links(void)
{
	goto_links();
	else_links();
	switch_links();
	break_links();
}
