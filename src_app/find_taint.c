#include "c_api.h"

typedef struct Tainted Tainted;
#define TPSZ	128	// must be power of 2

static struct Tainted {
	char	*fnm;		// fct name
	Prim	*src;		// where alert originated
	Prim	*param;		// name of param
	int	 pos;		// position of param
	int	 handled;	// in search_params
	Tainted *nxt;
} *tainted_param[TPSZ], *tainted_return[TPSZ];

enum Seen {
	SeenVar		=   1,
	SeenParam	=   2,
	SeenDecl	=   4
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

static char *marks[] = {
	"None",			// 0	0
	"FctName",		// 1	1
	"Target",		// 2	2
	"Alloca",		// 4	3
	"Taintable",		// 8	4
	"DerivedFromTaint",	// 16	5
	"PropViaFct",		// 32	6
	"VulnerableTarget",	// 64	7
	"CorruptableSource",	// 128	8
	"ExternalSource",	// 256	9
	"DerivedFromExternal",	// 512	10
	"Stop"			// 1024	11
};

extern void mark_fcts(void);	// in cwe_util.c

#if 0
	mset[0] stores a mark to indicate that scope was checked before
	mset[1] stores marks indicating potentially dangerous targets
	mset[2] stores reusable tags indicating fct definitions
	mset[3] stores marks for vars set from external sources
#endif

// utility functions

static void
what_mark(enum Tags t)
{	int i, j;

	for (i = j = 1; j < Stop; i++, j *= 2)
	{	if (t & j)
		{	printf("%s ", marks[i]);
	}	}
#if 0
	if (t&FctName)	printf("FctName ");
	if (t&Target)	printf("Target ");
	if (t&Alloca)	printf("Alloca ");
	if (t&UseOfTaint)	printf("Taintable ");
	if (t&DerivedFromTaint)	printf("DerivedFromTaint ");
	if (t&PropViaFct)	printf("PropViaFct ");
	if (t==VulnerableTarget)	printf("VulnerableTarget ");
	if (t==CorruptableSource)	printf("CorruptableSource ");
	if (t&ExternalSource)	printf("ExternalSource ");
	if (t&DerivedFromExternal) printf("DerivedFromExternal ");
#endif
}

static void
show_bindings(const char *s, const Prim *a, const Prim *b)
{
	if (!verbose)
	{	return;
	}

	if (a)
	{	printf("Bind <%s>\t%s:%d: %s (%d)\tto ", s,
			a->fnm, a->lnr, a->txt, a->mark);
	}

	if (b)
	{	printf("%s:%d: %s :: ",
			b->fnm, b->lnr, b->txt);
		what_mark(b->mark);
		printf("\n");
	} else
	{	printf("--\n");
	}

	if (a
	&&  a->bound
	&&  a->bound != b)
	{	printf("\t\t[already bound to %s:%d: %s]\n",
			a->bound->fnm, a->bound->lnr, a->bound->txt);
	}
}

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
TaintedParam(Prim *p, const int pos, Prim *nm)
{	Tainted *t, *prv = 0;	// remember which params are potentially tainted
	char *s;
	int h, k;

	if (!p)
	{	return;
	}

	s = p->txt;
	if (strstr(s, "scanf") != NULL
	||  strstr(s, "printf")  != NULL	// warn about these calls?
	||  strcmp(s, "fopen") == 0
	||  strcmp(s, "strcmp") == 0
	||  strcmp(s, "gets") == 0
	||  strcmp(s, "main") == 0)	// not called directly
	{	return;			// no source to search for these
	}
	p->bound = nm->bound?nm->bound:nm;

	h = t_hash(s)&(TPSZ-1);

	for (t = tainted_param[h]; t; prv = t, t = t->nxt)	// sorted
	{	k = strcmp(t->fnm, s);
		if (k > 0)
		{	break;
		}
		if (k == 0
		&&  t->src == p
		&&  t->pos == pos)
		{	return; // already there
	}	}

	t = (Tainted *) emalloc(sizeof(Tainted));
	t->fnm = (char *) emalloc(strlen(s)+1);
	strcpy(t->fnm, s);
	t->src = p;	// where did the alert originate
	t->param = nm;
	t->pos = pos;

	if (prv == NULL)	// new first entry
	{	t->nxt = tainted_param[h];
		tainted_param[h] = t;
	} else	// insert after prv
	{	t->nxt = prv->nxt;
		prv->nxt = t;
	}

	if (verbose)
	{	printf("%s:%d:\tTaintedParam %s( param %d = %s )\n",
			p->fnm, p->lnr, s, pos, nm->txt);
	}
}

static void
TaintedReturn(const char *s)
{	Tainted *t, *prv = 0;	// remember which fcts can return a tainted result
	int h, k;

	h = t_hash(s)&(TPSZ-1);
	for (t = tainted_return[h]; t; prv = t, t = t->nxt)
	{	k = strcmp(t->fnm, s);
		if (k > 0)
		{	break;
		}
		if (k == 0)
		{	return;
	}	}

	t = (Tainted *) emalloc(sizeof(Tainted));
	t->fnm = (char *) emalloc(strlen(s)+1);
	strcpy(t->fnm, s);
	if (prv == NULL)	// new first entry
	{	t->nxt = tainted_return[h];
		tainted_return[h] = t;
	} else	// insert after prv
	{	t->nxt = prv->nxt;
		prv->nxt = t;
	}

	if (verbose)
	{	printf("\tTaintedReturn %s\n", s);
	}
}

static int
HasTaintedReturn(const char *s)
{	Tainted *t;
	int h, k;

	h = t_hash(s)&(TPSZ-1);
	for (t = tainted_return[h]; t; t = t->nxt)
	{	k = strcmp(t->fnm, s);
		if (k > 0)
		{	break;
		}
		if (k == 0)
		{	return 1;
	}	}
	return 0;
}

static void
search_calls(Prim *inp)	// see if marked var inp.txt is used as a parameter in a fct call
{	Prim *r = inp;
	int cnt = 1;	// parameter nr

	if (r->mset[0] & SeenParam)	// handled before
	{	return;
	}
	r->mset[0] |= SeenParam;

	while (r && r->round >= inp->round)
	{	if (r->round == inp->round
		&&  strcmp(r->txt, ",") == 0)
		{	cnt++;
		}
		r = r->prv;
	}
	r = r?r->prv:0;	// should be the fct name

	if (!r
	|| strcmp(r->typ, "ident") != 0
	|| strcmp(r->txt, "main") == 0)
	{	return;
	}

	show_bindings("search_calls", r, inp->bound?inp->bound:inp);

	// Remember
	//	fct name r->txt
	//	param name inp and nr (cnt)
	// there may be multiple entries for the same call

	TaintedParam(r, cnt, inp);	// store and check use of the param in this fct later
}

static int
search_params(void)	// cur->txt is  fct name, at start of fct def, PropViaFct
{	Prim *r, *ocur = cur;
	Tainted *t;
	int pos, cnt = 0, level, k, h;

	// the marked formal parameters are vulnerable targets,
	// so check if the params are assigned to somewhere in the fct

	if (strcmp(cur->txt, "main") == 0)	// cannot be called directly
	{	return 0;			// params like argv handled separately
	}

	if (verbose > 1)
	{	printf("%s:%d: check fct body of %s() for possibly bad params\n",
		cur->fnm, cur->lnr, cur->txt);
	}

	h = t_hash(cur->txt)&(TPSZ-1);

	for (t = tainted_param[h]; t; t = t->nxt)		// list of fcts that use marked vars as params
	{	k = strcmp(t->fnm, cur->txt);
		if (k > 0)
		{	break;	// cur->txt is not in the (sorted) list
		}
		if (k != 0	// if fct is on the bad list
		||  t->handled)	// it could propagate taints
		{	continue;
		}
		t->handled = 1;

		// if cur->txt is a file static fct, then
		// the caller at t->src must be in the same file
		if (strcmp(t->src->fnm, cur->fnm) != 0) // different file
		{	// check that cur->txt is not preceded by "static"
			Prim *z = cur->prv;	// skip over typenames and * decorations
			while (z
			&&	z->curly == 0	// preceding fct name
			&&	(strcmp(z->typ, "type") == 0
			||       strcmp(z->txt, "*") == 0
			||	 strcmp(z->typ, "modifier") == 0))
			{	z = z->prv;
			}
			if (z
			&&  strcmp(z->txt, "static") == 0)	// typ: @storage
			{	continue;			// fct not visible to caller
		}	}

		if (verbose)
		{	printf("%s:%d:	checking fct %s()==%s() for use of tainted param nr %d -- %s\n",
				cur->fnm, cur->lnr, cur->txt, t->fnm, t->pos, ocur->txt);
		}
		pos = 1;
		level = 1 + cur->round;	// to see if we've reached the end of the formal params
more:
		while (cur->round > level
		||    (!MATCH(")")
		&&     !MATCH(",")))
		{	cur = cur->nxt;		// move to next param
		}
		if (pos == t->pos)		// the position of a vulnerable param
		{	cur = cur->prv;
			if (TYPE("ident"))	// the (formal) param name, before the comma
			{			// check the fct body for uses of this param
				if (verbose > 1)
				{	printf("cnt %d :: param check: %s (param pos %d = %s)\n",
						pos, t->fnm, t->pos, cur->txt);
				}
				r = cur->nxt;
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
					if (strcmp(r->txt, cur->txt) == 0	// same ident
					&&  r->mark == 0)			// not yet marked
					{
#if 0
	// mark it, no matter what the context is
						Prim *s = r->nxt;		// check if assigned to
						if (strcmp(s->txt, "[") == 0
						&&  s->jmp)
						{	s = s->jmp->nxt;
						}
						if (!strchr(s->txt, (int) '='))
						{
#endif
							r->mark |= PropViaFct;		// new taint mark
							show_bindings("search_params", r, t->param);
							if (!r->bound)		// for traceback
							{	r->bound = t->param;
							}
							cnt++;
							if (verbose > 1)
							{	printf("%s:%d: use of marked param %s -> +%d\n",
									r->fnm, r->lnr, cur->txt, PropViaFct);
							}
#if 0
						}
#endif
					}
					r = r->nxt;
			}	}
			cur = ocur;		// restore; done with this param
		} else if (strcmp(cur->txt, ")") != 0)
		{	cur = cur->nxt;
			pos++;
			if (pos <= t->pos)
			{	goto more;	// move to next position
	}	}	}

	if (verbose && cnt > 0)
	{	printf("found %d uses of marked params (adding mark %d)\n", cnt, PropViaFct);
	}

	return cnt;
}

static void
search_returns(const char *nm)
{	Prim *r, *s;
	int cnt = 0;

	r = cur->nxt;
	while (r && r->curly == 0)
	{	r = r->nxt;
	}

	while (r && r->curly > 0)	// search the fct body for return stmnts
	{	if (strcmp(r->txt, "return") == 0)
		{	s = r->nxt;
			while (s && strcmp(s->txt, ";") != 0)
			{	if (s->mark != 0)
				{	TaintedReturn(nm); // fct nm can return tainted result
					cnt++;
					break;
				}
				s = s->nxt;
		}	}
		r = r->nxt;
	}

	if (verbose && cnt > 0)
	{	printf("search returns of %s\n", nm);
	}
}

static void
save_set(const int into)
{
	// assert(into < 4);
	for (cur = prim; cur; cur = cur->nxt)	// save_set, called 3x
	{	cur->mset[into] = cur->mark;
		cur->mbnd[into] = cur->bound;
		cur->mark = 0;
		cur->bound = 0;
	}
}

static Prim *
move_to(Prim *p, const char *s)
{
	while (p && strcmp(p->txt, s) != 0)
	{	p = p->nxt;
	}
	return p;
}

static void
check_var(Prim *r, Prim *x)	// mark later uses of r->txt within scope level, DerivedFromExternal
{	Prim *w = r;		// starting at token x
	int level = x?x->curly:0;

	if (x
	&&  r->seq < x->seq)
	{	// r is a formal parameter
		// and the block to search starts at x
		r = x;
	} else
	{	r = r->nxt;
	}

	while (r && r->curly >= level)
	{	if (strcmp(r->txt, w->txt) == 0)
		{	r->mark |= DerivedFromExternal;
			show_bindings("check_var", r, w);
			r->bound = w; // point to place where an external source was tagged
			if (verbose > 1)
			{	printf("%s:%d: marked new occurrence of %s (+%d -> %d)\n",
					r->fnm, r->lnr, r->txt, DerivedFromExternal, r->mark);
		}	}
		r = r->nxt;
	}
}

static void
trace_back(const Prim *p, const int a)
{	Prim *e, *f = (Prim *) 0;
	int cnt=10;

	if (!p)
	{	return;
	}

	for (e = p->mbnd[a]; cnt > 0 && e; cnt--, e = e->mbnd[a])
	{	if (e != f
		&& (e->mset[a] != Target || verbose))
		{	printf("\t\t>%s:%d: %s (", e->fnm, e->lnr, e->txt);
			what_mark(e->mset[a]);
			printf(")\n");
			if (!f) { f = e; } // remember the first
	}	}
}

// the main steps

static int
step1_find_sources(void)	// external sources of potentially malicious data
{	Prim *q;
	int pos, cnt = 0, isfd;
	unsigned int bad;
	char *z, *r;

	//  argv
	//  scanf(     "...", arg, ...)
	// fscanf(fd,  "...", arg, ...)
	// sscanf(str, "...", arg, ...)
	// fgets(str, size, fd)
	//  gets(str)
	// getline(buf, size, fd)
	// getdelim(buf, size, delim, fd)

	for (cur = prim; cur; cur = cur->nxt)	// step1: mark potentially tainted sources
	{
		if (strcmp(cur->txt, "argv") == 0)
		{	cur->mark |= ExternalSource;
			cnt++;
			if (verbose > 1)
			{	printf("%s:%d: marked '%s' (+%d)\n",
					cur->fnm, cur->lnr, cur->txt, ExternalSource);
			}
			continue;
		}
		if (strcmp(cur->txt, "scanf") == 0)		// mark args below
		{	isfd = 0;
		} else if (strcmp(cur->txt, "fscanf") == 0
		       ||  strcmp(cur->txt, "sscanf") == 0)	// mark args below
		{	isfd = 1;
		} else if (strcmp(cur->txt, "gets") == 0)	// warn right away
		{	isfd = 0;
			printf("%s:%d: error: unbounded call to gets\n",
				cur->fnm, cur->lnr);
			q = move_to(cur->nxt, "(");
			if (!q)
			{	continue;
			}
			q = q->nxt;
			if (strcmp(q->typ, "ident") == 0)
			{	q->mark |= ExternalSource;
				cnt++;
				if (verbose > 1)
				{	printf("%s:%d: marked gets arg '%s' (+%d)\n",
						q->fnm, q->lnr, q->txt, ExternalSource);
			}	}
			continue;
		} else if (strcmp(cur->txt, "fgets") == 0
		       ||  strcmp(cur->txt, "getline") == 0
		       ||  strcmp(cur->txt, "getdelim") == 0)	// mark first arg
		{	q = move_to(cur->nxt, "(");
			if (!q)
			{	continue;
			}
			q = q->nxt;
			if (q && strcmp(q->typ, "ident") == 0)
			{	q->mark |= ExternalSource;
			}
			continue;
		} else
		{	continue;
		}

		// *scanf calls, mark corruptable args
		q = cur->nxt;
		if (!q || strcmp(q->txt, "(") != 0)
		{	continue;
		}
		if (isfd)	// move passed the fd or str field
		{	while (q && strcmp(q->txt, ",") != 0)
			{	q = q->nxt;
			}
			if (!q)
			{	continue;
			}
			q = q->nxt;
		}

		bad = pos = 0;
		while (q && strcmp(q->txt, ",") != 0)	// move to 2nd arg, skipping format string
		{	char *u = q->txt;

			while ((z = strstr(u, "%s")) != NULL)	// checking for unbounded %s
			{	r = u;
				while (r <= z)
				{	if (*r == '%'
					&&  *(r+1) != '%')
					{	pos++;
					}
					r++;
				}
				if (pos < 8*sizeof(unsigned int))
				{	bad |= 1<<pos;	// allows up to 64 params
				}
				u = z+2;
			}
			q = q->nxt;
		}
		if (!bad || !q)				// at least one unbounded %s seen
		{	continue;
		}

		q = q->nxt;
		pos = 1;
		while (strcmp(q->txt, ")") != 0
		&&     q->round > cur->round)	// all args
		{	if (strcmp(q->txt, ",") == 0)
			{	pos++;
			} else if (strcmp(q->typ, "ident") == 0
			       &&  (bad & (1<<pos)))
			{	q->mark |= ExternalSource;	// mark the corresponding args
				cnt++;
				if (verbose > 1)
				{	printf("%s:%d: marked scanf arg '%s' (+%d)\n",
						q->fnm, q->lnr, q->txt, ExternalSource);
			}	}
			q = q->nxt;
		}
	}
	if (verbose && cnt > 0)
	{	printf("marked %d vars as potential external taint source%s (+%d)\n",
			cnt, cnt>1?"s":"", ExternalSource);
	}
	return cnt;
}

static int
step2a_pre_scope(void)	// propagate external source marks across '='
{	Prim *q;
	int cnt = 0;

	for (cur = prim; cur; cur = cur->nxt)	// step2a: prop of tainted thru assignments
	{	if (cur->mark == 0)
		{	continue;
		}
		q = cur->prv;

		if (q && strchr(q->txt, '='))	// tainted var is assigned to something
		{	q = q->prv;		// propagate mark to lhs

			if (strcmp(q->txt, "]") == 0) // skip over possible array index
			{	q = q->jmp;
				q = q?q->prv:q;
			}

			if (q
			&&  strcmp(q->typ, "ident") == 0
			&&  !(q->mark & DerivedFromExternal))
			{	q->mark |= DerivedFromExternal;
				if (verbose > 1)
				{	printf("\t%s:%d: prescope propagated %s (+%d)\n",
						q->fnm, q->lnr, q->txt, DerivedFromExternal);
				}
				cnt++;
	}	}	}

	return cnt;
}

static int
likely_decl(const Prim *p)
{	Prim *q;

	if (!p)
	{	return 0;
	}
	q = p->prv;

	while (q && strcmp(q->txt, "*") == 0)
	{	q = q->prv;
	}

	if (q
	&&  (strcmp(q->typ, "type") == 0
	||   strcmp(q->txt, ",")    == 0))
	{	return 1;
	}

	return 0;
}

static void
step2b_check_scope(void)	// ExternalSource|DerivedFromExternal, DerivedFromExternal
{	Prim *q;

	// find declaration for marked external sources, so that
	// we can check all occurrences within the same scope

	for (cur = prim; cur; cur = cur->nxt)	// step2b: use of tainted within same scope
	{	if (!(cur->mark & (ExternalSource|DerivedFromExternal | DerivedFromExternal))
		||   (cur->mset[0] & SeenVar))
		{	continue;
		}
		cur->mset[0] |= SeenVar;	// only once per marked variable

		// search backwards to likely point of declaration
		for (q = cur; q && !likely_decl(q); )
		{	q = q->prv;
			while (q
			&&     (q->curly > 0 || q->round > 0)	// stick to locals & params
			&&     strcmp(q->txt, cur->txt) != 0)	// find the same ident name
			{	q = q->prv;
			}
			if (!q
			||  (q->curly <= 0 && q->round <= 0))	// probably not local
			{	q = 0;
		}	}
		if (!q)
		{	continue;
		}

		// found the likely point of decl, which could be a formal param
		// q->txt is the name of the variable, matching cur->txt
		if (verbose > 1)
		{	printf("%s:%d: %s likely declared here (level %d::%d)\n",
				q->fnm, q->lnr, cur->txt, q->curly, q->round);
		}
		if (q->mset[0] & SeenDecl) // checked before for this var
		{	continue;
		}
		q->mset[0] |= SeenDecl;

		if (q->curly == 0)		// must be a formal parameter
		{	if (q->round != 1)	// nope, means we dont know
			{	if (verbose>1)
				{	printf("%s:%d: '%s' untracked global (declared at %s:%d)\n",
						cur->fnm, cur->lnr, cur->txt, q->fnm, q->lnr);

				}
				continue; 	// likely global, give up
			}
			q = move_to(q, "{");	// scope of param is the fct body
		} else
		{	Prim *r = q;
			while (q->curly == r->curly)
			{	q = q->prv;	// local decl: find start of fct body
		}	}

		if (!q)
		{	continue;		// give up
		}

		// located the block level where cur->txt is declared:
		// now we must search this scope to mark all new
		// occurences that appear *after* cur itself upto the
		// end of this level of scope

		if (0)
		{	printf("check_var %s:%d '%s' level %d <<%d %d>>\n",
				cur->fnm, cur->lnr, cur->txt, q->nxt->curly,
				cur->seq, q->nxt->seq);
		}
		check_var(cur, q->nxt);	// mark DerivedFromExternal
	}
}

static void
step2c_part1(void)
{
	for (cur = prim; cur; cur = cur->nxt)	// step2c_6: prop of tainted thru fct calls
	{	if (cur->round > 0
		&&  cur->curly > 0
		&&  cur->mark != 0)
		{	search_calls(cur);
			// check if marked var is used as a param in a fct call
			// and remember these params in TaintedParam
			// which is used below to mark vars derived from these
	}	}
}

static int
step2c_part2(void)
{	Prim *q;
	int cnt = 0;

	for (cur = prim; cur; cur = cur->nxt)	// step2c_6: prop of tainted thru fct params and returns
	{	if (cur->mset[2])		// for each function
		{	q = cur;		  // save
			cnt += search_params();	  // mark uses of tainted params PropViaFct
			cur = q;		  // restore
			search_returns(cur->txt); // used in next scan below
	}	}

	return cnt;
}

static int
step2c_part3(void)
{	Prim *r;
	int cnt = 0;

	for (cur = prim; cur; cur = cur->nxt)	// step2c_6: use of tainted returns from fct calls
	{	if (strcmp(cur->txt, "(") == 0
		&&  cur->curly > 0
		&&  cur->prv
		&&  strcmp(cur->prv->typ, "ident") == 0
		&&  HasTaintedReturn(cur->prv->txt))
		{	// possible fct call returning tainted result
			r = cur->prv->prv;	// token before the call
			if (strchr(r->txt, '=') != NULL) // used in assignment?
			{	r = r->prv;
				if (strcmp(r->txt, "]") == 0
				&&  r->jmp)
				{	r = r->jmp->prv;
				}
				if (r
				&&  strcmp(r->typ, "ident") == 0
				&&  r->mark == 0)
				{	r->mark |= PropViaFct;	// mark it
					show_bindings("iterate", r, cur->prv);
					if (!r->bound)
					{	r->bound = cur->prv; // origin, fct nm
					}
					cnt++;
	}	}	}	}

	return cnt;
}

static int
step2c_step6_iterate(void)	// PropViaFct
{	int cnt;

	// check propagation of the marked variables (vulnerable targets)
	// through fct calls or via return statements

	       step2c_part1();
	cnt  = step2c_part2();
	cnt += step2c_part3();

	if (verbose)
	{	printf("iterate (%d) returns %d\n", PropViaFct, cnt);
	}

	return cnt;
}

static void
step3_step7_check_uses(const int v)	// used as args of: memcpy, strcpy, sprintf, or fopen
{	Prim *q;			// VulnerableTarget or CorruptableSource
	int cnt = 0;
	int nr_commas;

	for (cur = prim; cur; cur = cur->nxt)	// step3_7: check suspect uses of tainted data
	{	if (cur->mark == 0
		|| (cur->mark & v)
		||  cur->round == 0)
		{	continue;
		}
		if (verbose > 1)
		{	printf("%s:%d: Check Uses, mark %d, txt %s round: %d\n",
				cur->fnm, cur->lnr, v, cur->txt, cur->round);
		}
		q = cur;
		nr_commas = 0;
		while (q->round >= cur->round)
		{	if (q->round == cur->round
			&&  strcmp(q->txt, ",") == 0)
			{	nr_commas++;
			}
			q = q->prv;
		}
		q = q->prv;	// fct name
		if (verbose > 1)
		{	printf("\t\tfctname? '%s' round %d nrcommas %d (v=%d cm %d)\n",
				q->txt, q->round, nr_commas, v, cur->mark);
		}

		if (strcmp(q->txt, "sizeof") == 0)
		{	continue;
		}

		if ((strcmp(q->txt, "fopen") == 0	// make sure the fd doesn't have external source
		||   strcmp(q->txt, "freopen") == 0)
		&&  nr_commas == 0			// first arg: pathname
		&&  (cur->mark & (ExternalSource | DerivedFromExternal)))
		{	cnt++;
			printf("%s:%d: contents of %s in call to fopen() is possibly tainted: ",
				cur->fnm, cur->lnr, cur->txt);
				what_mark(cur->mark);
				printf("\n");
			continue;
		}

		// in the printf family, if a %n format specifier occurs
		// then the corresponding argument ptr is vulnerable and
		// should not be a corruptable: warn if a marked arg links to %n

		if (strstr(q->txt, "printf") != NULL)
		{	Prim *w;
			char *qtr;
			int nr;		// counts nr of format specifiers before %n
			int correction=0;

			w = q->nxt;				// (
			w = w->nxt;				// the format string for printf
			if (strcmp(w->typ, "ident") == 0)	// sprintf/fprintf/snprintf
			{	w = w->nxt;			// skip ,
				w = w->nxt;			// the format string
				correction = 1;			// remember the extra arg
			}

			if (strcmp(q->txt, "snprintf") == 0)
			{	correction = 2;			// account for size arg
				while (strcmp(w->txt, ",") != 0)
				{	w = w->nxt;		// skip over size arg
				}
				w = w->nxt;
			}

			// scan the format string for conversions specifiers

			for (qtr = w->txt, nr = 1; ; qtr++, nr++)
			{	while (*qtr != '\0' && *qtr != '%')
				{	qtr++;
				}
				if (*qtr == '\0'
				|| *(qtr+1) == '\0')
				{	break;	// checked all
				}
				qtr++;	// char following %

				switch (*qtr) {
				case 'n':
					if (0)
					{	printf("%d\tsaw %%n, nr %d nr_commas %d correction %d (%s)\n",
							cur->lnr, nr, nr_commas, correction, cur->txt);
					}
					// the nr-th format conversion is %n
					// check if its that the one that was marked
					if (nr == nr_commas - correction)
					{	printf("%s:%d: warning: %s() ", cur->fnm, cur->lnr, q->txt);
						printf("uses '%%n' with corruptable arg '%s'\n", cur->txt);
					}
					break;
				case '%':	// a literal %%
					nr--;
					break;
				default:
					break;
			}	}
			continue;
		}

		if (strcmp(q->txt, "memcpy") == 0
		||  strcmp(q->txt, "strcpy") == 0
		||  strcmp(q->txt, "sprintf") == 0
		||  (v == CorruptableSource && strcmp(q->txt, "gets") == 0))
		{
			if (((v == VulnerableTarget  && nr_commas == 0)
			||   (v == CorruptableSource && nr_commas  > 0))
			&&  !(cur->mark & v))	// not already marked
			{	cur->mark |= v;
				show_bindings("check_uses", cur, q);
				if (!cur->bound)
				{	cur->bound = q;	// origin
				} // else printf("6 does this happen?\n");
				cnt++;
				if (verbose > 1)
				{	printf("%s:%d: %s (mark |= %d) used as target in call of %s()\n",
					cur->fnm, cur->lnr, cur->txt, v, q->txt);
	}	}	}	}

	if (verbose)
	{	printf("%d of the potential targets used in suspicious calls (+%d)\n",
			cnt, v);
	}
}

static int
step4_find_targets(void)
{	Prim *q, *r;
	int cnt = 0;

	for (cur = prim; cur; cur = cur->nxt)	// step4: mark potentially corruptable targets
	{	// dont look inside struct declarations
		if (MATCH("struct")
		||  MATCH("union"))
		{	while (!MATCH("{") && cobra_nxt())
			{	;
			}
			if (!cur)
			{	break;
			}
			cur = cur->jmp;
			continue;
		}
		if (MATCH("static"))	// not a stack var
		{	if (!cobra_nxt())	// skips the following type
			{	break;
			}
			continue;
		}
		if (MATCH("alloca"))	// just 2 matches in linux3.0 sources
		{	q = cur;
			cur = cur->prv;
			while (!MATCH("=")
			&&     !MATCH(";")
			&&     !TYPE("ident")
			&&     cur->curly > 0)
			{	cur = cur->prv;
			}
			if (!MATCH("="))
			{	cur = q;
				continue;
			}
			cur = cur->prv;
			if (!TYPE("ident"))
			{	cur = q;
				continue;
			}
			cur->mark |= Alloca;
			cnt++;
			if (verbose > 1)
			{	printf("possibly taintable (alloca): %s:%d: %s\n",
					cur->fnm, cur->lnr, cur->txt);
			}
			cur = q;	// restore
			continue;
		}
		if (TYPE("type")
		&&  cur->curly > 0	// local to fct
		&&  cur->round == 0)	// not a type cast
		{	q = cur;
findmore:		while (!TYPE("ident")
			&&     !MATCH(";")
			&&     cur->curly == q->curly)
			{	if (MATCH("="))	// skip initializers
				{	while (!MATCH(",") && !MATCH(";"))
					{	cur = cur->nxt;
					}
					if (MATCH(";"))
					{	break;
				}	}
				cur = cur->nxt;
			}
			if (TYPE("ident")
			&&  cur->round == 0		// not a fct arg
			&&  cur->curly == q->curly)	// still in same block
			{	r = cur;
				cur = cur->nxt;
				if (MATCH("["))		// arrays are vulnerable
				{	r->mark |= Target;
					if (verbose > 1)
					{	printf("possibly taintable: %s:%d: %s\n",
							r->fnm, r->lnr, r->txt);
					}
					cnt++;
				}
				goto findmore;
			}
			cur = q;	// restore
	}	}

	if (verbose && cnt > 0)
	{	printf("found %d potentially taintable sources (marked with %d)\n", cnt, Target);
	}

	return cnt;
}

static void
step5_find_taints(void)		// completes Step 1 from taint_track.cobra script
{	Prim *q, *r, *s;	// marks UseOfTaint or DerivedFromTaint
	int cnt = 0;

	for (cur = prim; cur; cur = cur->nxt)	// step5: find additonal uses of marked items, forward search
	{	if (cur->mark)		// a target, search to the end of the fct
		{	q = cur;	// check where else the name occurs,
					// and if it is assigned to another variable
			if (strcmp(q->prv->txt, ".")  == 0
			||  strcmp(q->prv->txt, "->") == 0)
			{	continue;	// not top-level name
			}
		
			cur = cur->nxt;
			while (cur->curly > 0)		// up to end of fct
			{	if (!MATCH(q->txt)	// not same ident
				||  strcmp(cur->prv->txt, ".") == 0
				||  strcmp(cur->prv->txt, "->") == 0)	// not toplevel name
				{	cur = cur->nxt;
					continue;
				}
				if (!(cur->mark & UseOfTaint))
				{	cur->mark |= UseOfTaint;	// use of the taintable var
					show_bindings("find_taints1", cur, q);
					if (!cur->bound)
					{	cur->bound = q;	// remember origin
					} // else printf("1 does this happen?\n");
				} else
				{	break;	// already searched from here for q->txt
				}

				if (verbose > 1)
				{	printf("%s:%d: marked place where %s is used (+%d -> %d [%d::%d])\n",
						cur->fnm, cur->lnr, q->txt, UseOfTaint,
						cur->mark, cur->mset[1], cur->mset[3]);
				}

				r = cur->prv;
				s = cur->nxt;
				if (strchr(s->txt, (int) '=') != 0)	// q followed by =
				{	cur = cur->nxt;			// q is overwritten
					continue;
				}
				if (strcmp(r->txt, "&") == 0)		// q's address is taken
				{	r = r->prv;
				}
				if (strcmp(r->txt, "=") == 0)		// q used in assignment
				{	r = r->prv;
					if (strcmp(r->txt, "]") == 0)
					{	r = r->jmp;
						r = r->prv;
					}
					if (strcmp(r->typ, "ident") == 0
					&&  !(r->mark & DerivedFromTaint))
					{	r->mark |= DerivedFromTaint;	// newly tainted
						show_bindings("find_taints2", r, q);
						if (!r->bound)
						{	r->bound = q;	// remember origin
						} // else printf("2 does this happen?\n");

						cnt++;
						if (verbose > 1)
						{	printf("%s:%d '%s' tainted by asignment\n",
								r->fnm, r->lnr, r->txt);
				}	}	}
			// doesnt catch less likely assignments, like ptr = offset + buf
			// not yet: n.q = buf n->q = buf
				cur = cur->nxt;
			}
			cur = q;	// restore cur
	}	}

	if (verbose && cnt > 0)
	{	printf("found %d propagated marks (%d)\n", cnt, UseOfTaint);
	}
}

static void
do_report(const Prim *q, const int cnt, const int a, const int b)
{	Prim *p;

	switch (cnt) {
	default:
	case 0: break;
	case 1:
		if (verbose)
		{	printf("%s:%d: '%s' -- target vulnerable, but no external source\n",
				cur->fnm, cur->lnr, cur->txt);
		}
		break;
	case 2:
		if (verbose)
		{	printf("%s:%d: '%s' -- external source, but target not vulnerable\n",
				cur->fnm, cur->lnr, cur->txt);
		}
		break;
	case 3:
		printf("%s:%d: '%s'", cur->fnm, cur->lnr, cur->txt);
		if (q)
		{	int none = 1;
			printf(" marked arguments:");
			for (p = q->nxt; p && p->round > cur->round; p = p->nxt)
			{	if (strcmp(p->txt, ",") == 0
				|| p->mset[a] + p->mset[b] == 0)
				{	continue; // for-loop
				}
				if (none)
				{	printf("\n");
				}
				printf("\t%s\t", p->txt);
				if (p->mset[a])
				{	what_mark(p->mset[a]);
					printf("\t");
				}
				what_mark(p->mset[b]);
				printf("\n");
				trace_back(p, a);
				trace_back(p, b);
				none = 0;
			}
			if (none)
			{	printf("\n");
			}
			cur = p; // avoid duplicate reporting
		} else
		{	p = cur;
			printf("\t'%s'\t", p->txt);
			if (p->mset[a])
			{	what_mark(p->mset[a]);
				printf("\t");
			}
			what_mark(p->mset[b]);
			printf("\n");
			if (p->bound)
			{	printf("\t\tbound: %s:%d\n",
					p->bound?p->bound->fnm:"",
					p->bound?p->bound->lnr:0);
			}
			if (p->mbnd[a])
			{	trace_back(p, a);
			}
			if (p->mbnd[b]
			&&  p->mbnd[b] != p->mbnd[a])
			{	trace_back(p, b);
		}	}
		break;
	}
}

static void
step8_check_overlap(const int a, const int b)	// a: CorruptableSource, b: VulnerableTarget
{	Prim *p, *q;
	int nrcommas, nrdots, cnt, level;

	for (cur = prim; cur; cur = cur->nxt)	// step8: check overlap of taints and targets
	{
		if (verbose > 1
		&& (cur->mset[a] || cur->mset[b]))
		{	printf("%s:%d:  '%s' %d %d\t",
				cur->fnm, cur->lnr, cur->txt,
				cur->mset[1], cur->mset[3]);
			what_mark(cur->mset[a]);
			printf("\t");
			what_mark(cur->mset[b]);
			printf("\n");
		}

		cnt = 0;
		if (cur->mset[a])	// & (UseOfTaint     | DerivedFromTaint)
		{	cnt |= 1;
		}
		if (cur->mset[b])	// & (ExternalSource | DerivedFromExternal)
		{	cnt |= 2;
		}
		if (cnt)
		{	do_report(0, cnt, a, b);
			continue;
		}

		// additional check for the use of tainted or corruptable data
		// in the following four specific function calls:

		if (!TYPE("ident")
		|| (strcmp(cur->txt, "sprintf") != 0
		&&  strcmp(cur->txt, "snprintf") != 0
		&&  strcmp(cur->txt, "strcpy") != 0
		&&  strcmp(cur->txt, "memcpy") != 0))
		{	continue;
		}

		// first arg should have mset[a] != 0 -- ie a corruptable target
		// some the remaining args should have mset[b] != 0 -- ie an external source

		p = q = move_to(cur, "(");
		cnt = nrcommas = nrdots = 0;
		level = (p && p->nxt)?p->nxt->round:0;
		do {
			p = p->nxt;
			if (strcmp(p->txt, ",") == 0)
			{	nrdots = 0;
				nrcommas++;
			} else if (strcmp(p->txt, ".") == 0
			       ||  strcmp(p->txt, "->") == 0)
			{	nrdots++;
			} else if (nrdots == 0 && p->bracket == 0 && p->round == level)
			{	if (p->mset[a] != 0 && nrcommas == 0)
				{	cnt |= 1;
				}
				if (p->mset[b] != 0 && nrcommas != 0)
				{	cnt |= 2;
			}	}
			if (verbose>1)
			{	printf("%s:%d: '%s' (%d :: %d)\n",
					p->fnm, p->lnr, p->txt, p->mset[a], p->mset[b]);
			}
		} while (p && p->round > cur->round); // arg list

		do_report(q, cnt, a, b);
	}
}

void
cobra_main(void)
{	int i, cnt = 0;

	mark_fcts();
	save_set(2);	// marking function definitions

	// part 1 of 3
	// 	mark data that could originate from external inputs
	// 	after locating the initial external sources, iterate
	//	to find
	// 	 1. additional uses within the same scope
	// 	 2. assignments to other variables (leading back to 1)
	// 	 3. parameters to function calls (leading back to 1 and 2)
	//
	// 	 the Phase2 search for vulnerable (stack-based) targets is
	// 	 simpler since they can only reasonably propagate through
	//	 pointers to those targets (in assignments and fct calls)

	if (verbose > 1)
	{	printf("\n=====Phase1 -- mark external sources\n");
	}

	cnt = step1_find_sources();	// check the most common types; mark ExternalSource
	if (!cnt)
	{	printf("taint: no sources of external input found\n");
		if (!p_debug)
		{	return;
		} else
		{	printf("taint: debug mode, continuing scan anyway\n");
	}	}

	// first iteration, to mark all derived external sources

	while (cnt)
	{	cnt = step2a_pre_scope();	// mark DerivedFromExternal
		step2b_check_scope();		// mark DerivedFromExternal
		cnt += step2c_step6_iterate();	// mark PropViaFct
	}

	// once: check how the marked vars may be used
	// in dangerous calls, and mark dubious cases
	step3_step7_check_uses(VulnerableTarget);	// does any appear in dangerous calls
	save_set(3);	// uses of VulnerableTarget     // eg in memcpy, strcpy, sprintf

	// ==== done with potentially dangerous external sources  ====

	// part 2 of 3
	// look for fixed size buffers declared on the stack
	// these can be targets for code injection
	// pattern:  @type ^;* @ident [   ^;* ;
	if (verbose > 1)
	{	printf("\n=====Phase2 -- mark taintable targets\n");
	}

	cnt  = step4_find_targets();	// fixed size buffers declared on stack, marked Target
					// stack variables assigned with alloca; marked Alloca

	if (cnt == 0)
	{	printf("taint: no taintable targets found\n");
		return;
	}

	// find anything that can point to Target or Alloca

	step5_find_taints();	// mark: UseOfTaint, DerivedFromTaint

	// check propagation of marked vars thru fct params or returns
	// and mark vars derived from these

	for (i = 0; i < TPSZ; i++)
	{	tainted_param[i] = NULL;
		tainted_return[i] = NULL;			// new lists
	}
	while (step2c_step6_iterate() != 0)	// mark PropViaFct
	{	;
	}

	// if any marked variables are there
	// now check if they can get assigned/filled with data
	// coming from untrusted sources
	// e.g., in sprintf, strcpy, or memcpy calls

	step3_step7_check_uses(CorruptableSource);
	// all potentially dangerous targets that appear in copy routines
	// are now marked CorruptableSource

	save_set(1);	// uses of CorruptableSource

	// part 3 of 3
	// check if the msets 1 and 3 overlap
	// meaning that an external source can be
	// used in a dangerous operation for a fct-local buffer
	if (verbose > 1)
	{	printf("\n=====Phase3 -- check overlap\n");
	}

	step8_check_overlap(1, 3); // CorruptableSource , VulnerableTarget
}
