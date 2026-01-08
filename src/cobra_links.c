/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at https://codescrub.com/cobra
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
static int seen_symbols;

static  Lnrs *store_var(TrackVar **, Prim *, int, int);
static  ThreadLocal_links *thr;

FILE	*track_fd;

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

	do_lock(cid, 10);	// store_var : set gotolinks
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
			{	q = (Lnrs *) hmalloc(sizeof(Lnrs), cid, 119);
				q->nxt = p->lst;
				q->nv  = v;
				p->lst = q;
			}
			do_unlock(cid, 10);

			return q;
		}
		if (c < 0)
		{	break;
	}	}

	n = (TrackVar *) hmalloc(sizeof(TrackVar), cid, 120);
	n->t = v;
	n->lst = (Lnrs *) hmalloc(sizeof(Lnrs), cid, 120);
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
	do_unlock(cid, 10);

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
else_links_range(Prim *from, Prim *upto)
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
switch_links_range(Prim *from, Prim *upto)
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

	else_links_range(from, upto);

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

	switch_links_range(from, upto);

	return NULL;
}

static void
goto_links(void)
{
	if (seen_goto_links)
	{	return;
	}
	seen_goto_links = 1;

	run_threads(goto_links_run);
}

static void
else_links(void)
{
	if (seen_else_links > 0)
	{	return;
	}
	seen_else_links = 1;
	run_threads(else_links_run);
}

static void
switch_links(void)
{
	if (seen_switch_links > 0)
	{	return;
	}
	seen_switch_links = 1;
	run_threads(switch_links_run);
}

static void
break_links(void)
{	static int lastN = 0;

	if (seen_break_links)
	{	return;
	}
	seen_break_links = 1;

	if (lastN < Ncore || !thr)
	{	thr = (ThreadLocal_links *) emalloc(Ncore * sizeof(ThreadLocal_links), 68);
		lastN = Ncore;
	} else
	{	memset(thr, 0, Ncore * sizeof(ThreadLocal_links));
	}

	run_threads(break_links_run);
}

// functions for linking variables to most likely place
// of declaration, using a minimal scan of type information

typedef struct V_ref V_ref;
struct V_ref {
	Prim  *nm;	// char  *txt;
	Prim  *loc;	// type
	V_ref *nxt;
};

#define HT_SZ		512
#define MAXSCOPE	128
#define LOCALS		0
#define GLOBALS		1
#define PARAMS		2
#define MISSED		3

static int counts[MISSED+1];
static V_ref *ref_free;
static V_ref ***scope;	// V_ref *scope[MAXSCOPE][HT_SZ]
static V_ref *params;

static int
likely_decl(Prim *x)	// x points to the first token after a typename
{
	if (x
	&&  (x->txt[0] == ','
	||   x->txt[0] == '='))
	{	return 0;
	}

	// scan ahead to first ';'
	while (x && x->txt[0] != ';')
	{	if (strcmp(x->typ, "type") == 0	// saw another typename
		||  x->txt[0] == '.'
		||  x->txt[0] == '{')		// likely a struct/union
		{	return 0;
		}
		if (strcmp(x->txt, "=") == 0)	// initializer
		{	break;			// so its not a struct or typedef
		}		
		x = x->nxt;
	}
	return 1;
}

static int
is_it_static(Prim *p)	// assumed to point at type in a global declaration
{
	if (p->round == 0)	// ie not a parameter
	{	while (p && p->txt[0] != ';' && p->curly == 0)
		{	if (strcmp(p->txt, "static") == 0)
			{	return 1;
			}
			p = p->prv;
	}	}
	return 0;
}

static int
find_in_list(Prim *v, V_ref *lst, char *s, int n)
{	V_ref *d;
	FILE *fd = (track_fd) ? track_fd : stdout;

	for (d = lst; d; d = d->nxt)
	{	if (strcmp(v->txt, d->nm->txt) == 0)
		{	if (strcmp(v->fnm, d->loc->fnm) != 0
			&&  is_it_static(d->loc))
			{	if (0)
				{  fprintf(fd, "%s:%d: '%s'\t%sdeclared at %s:%d (level %d) as %s\n",
					v->fnm, v->lnr, v->txt, s,
					d->loc->fnm, d->loc->lnr, n, d->loc->txt);
				}
				continue;	// not a match
			}
			if (v != d->nm)
			{	if (verbose)
				{  fprintf(fd, "%s:%d: '%s'\t%sdeclared at %s:%d (level %d) as %s",
					v->fnm, v->lnr, v->txt, s,
					d->loc->fnm, d->loc->lnr, n, d->loc->txt);
				   if (strcmp(v->fnm, d->loc->fnm) != 0)
				   {	fprintf(fd, "  (extern)");
				   }
				   fprintf(fd, "\n");
			}	 }
			v->bound = d->loc;	// bind var to location of declaration
			return 1;
	}	}
	return 0;
}

static V_ref *
v_recycle(V_ref *p)
{	V_ref *q;

	if (!p)
	{	return NULL;
	}
	for (q = p; q; q = q->nxt)
	{	q->nm  = NULL;
		q->loc = NULL;
		if (!q->nxt)
		{	q->nxt = ref_free;
			break;
	}	}
	ref_free = p;
	return NULL;
}

static V_ref *
v_new(Prim *v, Prim *n)
{	V_ref *p;

	if (ref_free)
	{	p = ref_free;
		ref_free = p->nxt;
		p->nxt = 0;
	} else
	{	p = (V_ref *) emalloc(sizeof(V_ref), 159);
	}

	p->nm  = v;
	p->loc = n;
	return p;
}

static int
all_uppercase(Prim *p)
{	char *s = p->txt;
	int n;

	// not a likely macro, and not in a headerfile
	n = strlen(s);
	if (n > 2
	&&  s[n-1] == 'h'
	&&  s[n-2] == '.')
	{	return 1;	// headerfile
	}

	while (*s != '\0')
	{	if (islower((int) *s++))
		{	return 0;
	}	}

	return 1;	// all-upper
}

static void
add_scope(Prim *q, Prim *p, int level, int who)	// q is associated with type p
{	V_ref *y;
	uint h;
	FILE *fd = (track_fd) ? track_fd : stdout;

	if (all_uppercase(q))
	{	return;	// likely macro
	}

	if (level == 0
	&&  p->round == 1)
	{	y = v_new(q, p);
		y->nxt = params;
		params = y;
		if (verbose)
		{	fprintf(fd, "+Param '%s' '%s' (%d)\n", q->txt, p->txt, who);
		}
	} else if (p->round == 0)
	{	y = v_new(q, p);
		h = hasher(q->txt) & (HT_SZ - 1);
		y->nxt = scope[level][h];
		scope[level][h] = y;
		if (verbose)
		{	fprintf(fd, "%s:%d: +Scope[%d] '%s' '%s' (%d::%d %d::%d::%d)\n",
				q->fnm, q->lnr, level,
				q->txt, p->txt,
				p->curly, q->curly,
				p->round, q->round,
				who);
	}	}

	if (q->bound
	&&  strcmp(q->bound->txt, "unsigned") != 0
	&&  strcmp(q->bound->txt, "signed") != 0
	&&  strcmp(q->bound->txt, p->txt) != 0)
	{	if (strcmp(p->typ, "type") != 0
		&&  strcmp(q->bound->typ, "void") != 0)
		{	return;
		}
		if (verbose)
		{	fprintf(fd, "%s:%d: bound on '%s' reassigned from '%s' (%s) to '%s' (%s)\n",
				q->fnm, q->lnr, q->txt, q->bound->txt,
				q->bound->typ, p->txt, p->typ);
	}	}

	q->bound = p;
}

static void
possible_typedef(Prim *p, int level)
{	Prim *q = p;

	// saw an ident, and can't find a declaration
	// this could be Typedefedname [*]* varname [,;]

	if (p->prv
	&&  (p->prv->txt[0] == '*'
	||   p->prv->txt[0] == ','))
	{	return; // not a typename
	}
	if (p->nxt
	&&  (p->nxt->txt[0] == ','
	||   p->nxt->txt[0] == '('
	||   p->nxt->txt[0] == ')'
	||   p->nxt->txt[1] == '='))
	{	return;
	}

	for (q = q->nxt; q; q = q->nxt)
	{	// the only acceptable tokens
		if (q->txt[0] == '*'
		||  q->txt[0] == ',')
		{	continue;
		}
		if (q->txt[0] == ';'
		||  strcmp(q->typ, "ident") != 0)
		{	break;
		}
		if (0)
		{	printf("%s:%d: ident '%s' is likely of type '%s'\n",
				p->fnm, p->lnr, q->txt, p->txt);
		}
		add_scope(q, p, level, 1);
		break;
	}
	
}

static int
find_decl(Prim *p, int level)
{	Prim *x;
	uint h;
	int n;
	FILE *fd = (track_fd) ? track_fd : stdout;

	// if the immediately preceding token is a typename, it's easy
	if (0 && p->prv
	&&  strcmp(p->prv->typ, "type"))
	{	p->bound = p->prv;
		return 1;
	}
	x = p->nxt;
	if (x->txt[0] == '('		// fct def or call
	||  x->txt[0] == ':')		// labelname
	{	return 1;
	}
	x = p->prv;
	if (x)
	if (x->txt[0] == '.'
	|| strcmp(x->txt, "goto") == 0		// labelname
	|| strcmp(x->txt, "->") == 0)		// struct field
	{	return 1;
	}
	if (p->round > 0
	&&  p->prv
	&&  strcmp(p->prv->typ, "type") == 0)
	{	return 1; // in parameter list
	}

	if (find_in_list(p, params, "param ", level))
	{	counts[PARAMS]++;
		return 1;
	}

	// not a param, could be local or global

	for (n = level; n >= 0; n--)
	{	// printf("\tcheck if '%s' is in Scope[%d]:\n", p->txt, n);
		h = hasher(p->txt) & (HT_SZ - 1);
		if (find_in_list(p, scope[n][h], "", level))
		{	if (n > 0)
			{	counts[LOCALS]++;
			} else
			{	counts[GLOBALS]++;
			}
			return 1;
	}	}

	if (!p->bound				// set for identifiers in declarations
	&&  strcmp(p->txt, "stdin") != 0
	&&  strcmp(p->txt, "stdout") != 0
	&&  strcmp(p->txt, "stderr") != 0
	&&  strstr(p->txt, "_t") == 0		// should be: names that end in _t
	&&  !all_uppercase(p))
	{	n = strlen(p->txt);
		if (n > 2
		&&  strcmp(&(p->txt[n-2]), "_t") == 0)
		{	return 0;
		}
		if (verbose)
		{  fprintf(fd, "%s:%d: '%s'\t, declaration not found (level %d::%d)\n",
			p->fnm, p->lnr, p->txt, level, p->curly);
		}
		counts[MISSED]++;
	}

	return 0;
}

static int
not_prototype(Prim *p)	// not in a formal param list of prototype decl
{	// look at first char after param list
	// if '{' it's a fct,
	// if ';' it's a prototype

	if (p->round == 0)
	{	return 1;
	}

	while (p && p->round > 0)
	{	p = p->nxt;
	}
	if (p && p->nxt && p->nxt->txt[0] == ';')
	{	return 0;
	}
	return 1;
}

// externally visible functions

#if 1
 #if defined(__GNUC__) && defined(__i386__)
	#define get16bits(d) (*((const uint16_t *) (d)))
 #else
	#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8) \
                              +(uint32_t)(((const uint8_t *)(d))[0]) )
 #endif

uint
hasher(const char *s)
{	int len = strlen(s);
	uint32_t h = len, tmp;
	int rem;

	rem = len & 3;
	len >>= 2;

	for ( ; len > 0; len--)
	{	h  += get16bits(s);
        	tmp = (get16bits(s+2) << 11) ^ h;
        	h   = (h << 16) ^ tmp;
        	s  += 2*sizeof(uint16_t);
		h  += h >> 11;
	}
	switch (rem) {
	case 3: h += get16bits(s);
		h ^= h << 16;
		h ^= s[sizeof(uint16_t)] << 18;
		h += h >> 11;
		break;
	case 2: h += get16bits(s);
		h ^= h << 11;
		h += h >> 17;
		break;
	case 1: h += *s;
		h ^= h << 10;
		h += h >> 1;
		break;
	}
	h ^= h << 3;
	h += h >> 5;
	h ^= h << 4;
	h += h >> 17;
	h ^= h << 25;
	h += h >> 6;

	return h;	// caller adds &H_MASK
}
#else
 uint
 hasher(const char *s)
 {	unsigned int h = 0x88888EEFL;
	const char t = *s;
 
	while (*s != '\0')
	{	h ^= ((h << 4) ^ (h >> 28)) + *s++;
	}
	return (uint) (t ^ (h ^ (h>>(H_BITS))));
 }
#endif

void
var_links(char *unused1, char *unused2)	// symbols command
{	int h, level = 0;
	Prim *x;

	start_timer(0);

	if (seen_symbols)
	{	goto done;
	}

	if (!scope)
	{	scope = (V_ref ***) emalloc(sizeof(V_ref **) * MAXSCOPE, 115);
		for (h = 0; h < MAXSCOPE; h++)
		{	scope[h] = (V_ref **) emalloc(sizeof(V_ref *) * HT_SZ, 116);
		}
	} else
	{	for (h = 0; h < MAXSCOPE; h++)
		{	memset(scope[h], 0, sizeof(V_ref *) * HT_SZ);
	}	}
	memset(counts, 0, sizeof(counts));

	for (cur = prim; cur; cur = cur?cur->nxt:0)
	{
		switch (cur->txt[0]) {
		case '{':	// scope level
			level = cur->curly + 1;
			if (level >= MAXSCOPE)
			{	fprintf(stderr, "%s:%d: error: scope to deeply nested (max %d)\n",
					cur->fnm, cur->lnr, MAXSCOPE);
				stop_timer(0, 0, "V1");
				return;
			}
			for (h = 0; h < HT_SZ; h++)
			{	scope[level][h] = v_recycle(scope[level][h]);
			}
			continue;
		case '}':
			level = cur->curly;
			if (level < 0)
			{	fprintf(stdout, "%s:%d: error: bad nesting\n",
					cur->fnm, cur->lnr);
				level = 0;
			}
			if (level == 0)	// end of fct body
			{	params = v_recycle(params);
			}
			continue;
		}

		// printf("%s:%d: at '%s' (%s)\n", cur->fnm, cur->lnr, cur->txt, cur->typ);
		if ((strcmp(cur->typ, "type") == 0	// mine declarations
		||   strcmp(cur->typ, "modifier") == 0)
		&&  cur->bracket == 0)
		{	if (strcmp(cur->typ, "modifier") == 0
			&&  strcmp(cur->nxt->typ, "type") == 0)	// eg unsigned int
			{	cur = cur->nxt;
			}
			if (cur->round == 1
			&&  cur->curly == 0)		// possible param decl
			{	if (cur->nxt
				&&  (cur->nxt->txt[0] == ','
				||   cur->nxt->txt[0] == '='))
				{	continue;
				}
				x = cur;
				while (x && x->round >= 1 && x->txt[0] != ',')
				{	if (strcmp(x->typ, "ident") == 0)
					{	add_scope(x, cur, level, 2);
					}
					x = x->nxt;
				}
				continue;
			}
			if (cur->round == 0
			&&  likely_decl(cur->nxt))		// local or global decl
			{	// tag all identifiers between here and the first semi-colon
				// with typename (x) and add to current scope
X:				x = cur;
				// printf("%s:%d: in '%s'\n", x->fnm, x->lnr, x->txt);
				while (cur && cur->txt[0] != ';')
				{	if (strcmp(cur->txt, "=") == 0)	// initializer, skip ahead
					{	cur = cur->nxt;
						while (cur
						&& cur->txt[0] != ','
						&& cur->txt[0] != ';')
						{	if (strcmp(cur->typ, "ident") == 0)
							{	(void) find_decl(cur, level);
							}
							switch (cur->txt[0]) {
							case '{':
								level = cur->curly + 1;
								break;
							case '}':
								level = cur->curly;
								break;
							}
							cur = cur->nxt;
						}
						if (cur->txt[0] == ';')
						{	continue;
					}	}
					if (strcmp(cur->typ, "type") == 0)
					{	if (cur->seq != x->seq)
						{	// walked into new decl
							goto X;
					}	}

					if (cur
					&&  strcmp(cur->typ, "ident") == 0
					&&  cur->nxt
					&&  cur->nxt->txt[0] != '('
					&&  not_prototype(cur))
					{	add_scope(cur, x, level, 3);
					}
					cur = cur?cur->nxt:0;
				}
				continue;
		}	}

		if (strcmp(cur->typ, "ident") == 0)	// try to determine type and point of decl
		{	// look in params and in scope levels
			if (!find_decl(cur, level))
			{	// common issue: typedef's that aren't in the current .c file
				// look for @ident * @ident [,;] as likely typedef'ed declarations
				possible_typedef(cur, level);
			}
		}
	}
done:	if (!no_match)
	{	printf("symbols: linked %d params, %d locals, %d globals, %d unknown\n",
			counts[PARAMS], counts[LOCALS], counts[GLOBALS], counts[MISSED]);
	}
	seen_symbols = 1;
	stop_timer(0, 1, "V2");
}

void
clear_seen(void)
{
	seen_goto_links = 0;
	seen_else_links = 0;
	seen_switch_links = 0;
	seen_break_links = 0;
	seen_symbols = 0;
	counts[PARAMS] = 0;
	counts[LOCALS] = 0;
	counts[GLOBALS] = 0;
	counts[MISSED] = 0;
}

void
set_links(void)
{
	goto_links();
	else_links();
	switch_links();
	break_links();
}
