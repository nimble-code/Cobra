#include "find_taint.h"

// read-only constants

static const char *marks[] = {
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

static const char *t_links[] = {
	 "[", "]", "(", ")", "{", "}"
};

static int setlimit;	// not modified after init

// globals

static int Into;
static int banner;
static int ngets_bad;
static int nfopen;

FILE *track_fd;

int ngets;
int warnings;
char *Cfg;	// user-defined configuration file, can be set with --cfg=newfilenae

//	mset[0] stores a mark to indicate that scope was checked before
//	mset[1] stores marks indicating potentially dangerous targets
//	mset[2] stores reusable tags indicating fct definitions
//	mset[3] stores marks for vars set from external sources

// utility functions

static void
what_mark(FILE *fd, const char *prefix, enum Tags t)
{	int i, j;
	int cnt = 0;

	for (i = j = 1; j < Stop; i++, j *= 2)
	{	if (t & j)
		{	fprintf(fd, "%s%s", cnt ? ", " : prefix, marks[i]);
			cnt++;
	}	}
}

static void
search_calls(Prim *inp, int cid)	// see if marked var inp.txt is used as a parameter in a fct call
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

	// if (verbose)
	// {	show_bindings("search_calls", r, inp->bound?inp->bound:inp, cid);
	// }
	// Remember
	//	fct name r->txt
	//	param name inp and nr (cnt)
	// there may be multiple entries for the same call

	if (!cfg_ignore(r->txt))
	{	param_is_tainted(r, cnt, inp, cid); // store and check use of param in this fct later
	}
}

static void
save_set_range(Prim *from, Prim *upto, int cid)
{	Prim *mycur = from;
	int into = Into;	// get a local copy

	while (mycur && mycur->seq <= upto->seq)
	{	mycur->mset[into] = mycur->mark;
		mycur->mbnd[into] = mycur->bound;
		mycur->mark  = 0;
		mycur->bound = 0;
		mycur = mycur->nxt;
	}
}

static void *
save_set_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	save_set_range(from, upto, *i);

	return NULL;
}

static void
save_set(const int into)
{
	Into = into;
	run_threads(save_set_run);
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
check_var(Prim *r, Prim *x, int cid)	// mark later uses of r->txt within scope, DerivedFromExternal
{	Prim *w = r;			// starting at token x
	int level = x?x->curly:0;

	if (0)
	{	fprintf(stderr, "%d: check_var %s - forward scan <%d> ((%d::%s))\n",
			r->lnr, r->txt, r->mark, x->lnr, x->txt);
	}

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
			if (verbose)
			{	show_bindings("check_var", r, w, cid);
			}
			r->bound = w; // point to place where an external source was tagged
			if (verbose > 1)
			{	do_lock(cid);
				fprintf(stderr, "%d: %s:%d: marked new occurrence of %s (+%d -> %d)\n",
					cid, r->fnm, r->lnr, r->txt, DerivedFromExternal, r->mark);
				do_unlock(cid);
		}	}
		r = r->nxt;
	}
}

static void
trace_back(FILE *fd, const char *prefix, const Prim *p, const int a)
{	Prim *e, *f = (Prim *) 0;
	int cnt=10, first=1;

	if (!p || json_format)
	{	return;
	}
	for (e = p->mbnd[a]; cnt > 0 && e; cnt--, e = e->mbnd[a])
	{	if (e != f
		&& (e->mset[a] != Target || verbose))
		{	if (first)
			{	first = 0;
				fprintf(fd, "%s\n", prefix);				// n
			}
			fprintf(fd, "\t\t>%s:%d: %s (", e->fnm, e->lnr, e->txt);	// n
			what_mark(fd, "", e->mset[a]);
			what_mark(fd, "\t", e->mset[(a==3)?1:3]);	// 1->3, 3->1
			fprintf(fd, ")\n");						// n
			if (!f) { f = e; } // remember the first
	}	}
}

static void
add_details(FILE *fd, Prim *p, const int a, Prim *q, const int b)
{	//   prv		 nxt

	if (json_format)
	{	return;
	}
	if (p->mset[a])
	{	what_mark(fd, "\tT:", p->mset[a]);
	}
	if (q->mset[b])
	{	what_mark(fd, "\tE:", q->mset[b]);
	}
	fprintf(fd, "\n");								// n
	trace_back(fd, "\t  (taints)", p, a);
	trace_back(fd, "\t  (external)", q, b);
}

// the main steps

static int
find_sources(void)	// external sources of potentially malicious data
{	int cnt = 0;

	for (cur = prim; cur; cur = cur->nxt)	// mark potentially tainted sources
	{	cnt += handle_taintsources(cur);
		cnt += handle_importers(cur);
	}
	if (verbose && cnt > 0)
	{	fprintf(stderr, "marked %d vars as potential external taint source%s (+%d)\n",
			cnt, cnt>1?"s":"", ExternalSource);
	}
	return cnt;
}

static void
pre_scope_range(Prim *from, Prim *upto, int cid)	// propagate external source marks
{	Prim *mycur, *q;
	int cnt = 0; // propagation of tainted vars thru assignments thru '=' or sscanf

	for (mycur = from; mycur && mycur->seq <= upto->seq; mycur = mycur->nxt)
	{
		if (mycur->mark == 0)
		{	continue;
		}
		q = mycur->prv;

		if (q && strchr(q->txt, '=')
		&& (q->txt[1] == '\0'
		|| (q->txt[0] != '='
		&&  q->txt[0] != '>'
		&&  q->txt[0] != '<'
		&&  q->txt[0] != '!')))	// tainted var assigned to something
		{	q = q->prv;	// propagate mark to lhs

			if (strcmp(q->txt, "]") == 0) // skip over possible array index
			{	q = q->jmp;
				q = q?q->prv:q;
			}

			if (q
			&&  strcmp(q->typ, "ident") == 0
			&&  !(q->mark & DerivedFromExternal))
			{	q->mark |= DerivedFromExternal;
				if (verbose > 1)
				{	do_lock(cid);
					fprintf(stderr, "\t%s:%d: prescope propagated %s (+%d)\n",
						q->fnm, q->lnr, q->txt, DerivedFromExternal);
					do_unlock(cid);
				}
				cnt++;
		}	}
	}
	tokrange[cid]->param = cnt;
	if (verbose > 1)
	{	do_lock(cid);
		fprintf(stderr, "%d: pre_scope returns %d\n", cid, cnt);
		do_unlock(cid);
	}
}

static void *
pre_scope_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	pre_scope_range(from, upto, *i);

	return NULL;
}

static int
pre_scope(void)
{	int cnt;

	cnt = run_threads(pre_scope_run);
	if (verbose > 1)
	{	fprintf(stderr, "pre_scope returns %d\n", cnt);
	}
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
check_scope_range(Prim *from, Prim *upto, int cid)	// ExternalSource|DerivedFromExternal, DerivedFromExternal
{	Prim *q, *mycur;

	// find declaration for marked external sources, and
	// then mark all occurrences within the same scope

	mycur = from;
	while (mycur && mycur->seq <= upto->seq)	// mark use of tainted within same scope
	{	if (!(mycur->mark & (ExternalSource|DerivedFromExternal | DerivedFromExternal))
		||   (mycur->mset[0] & SeenVar))
		{	mycur = mycur->nxt;
			continue;
		}
		mycur->mset[0] |= SeenVar;	// only once per marked variable

		// search backwards to likely point of declaration
		for (q = mycur; q && !likely_decl(q); )
		{	q = q->prv;
			while (q
			&&     (q->curly > 0 || q->round > 0)	// stick to locals & params
			&&     strcmp(q->txt, mycur->txt) != 0)	// find the same ident name
			{	q = q->prv;
			}
			if (!q
			||  (q->curly <= 0 && q->round <= 0))	// probably not local
			{	q = 0;
		}	}
		if (!q)
		{	mycur = mycur->nxt;
			continue;
		}

		// found the likely point of decl, which could be a formal param
		// q->txt is the name of the variable, matching mycur->txt
		if (verbose > 1)
		{	do_lock(cid);
			fprintf(stderr, "%d: %s:%d: %s likely declared here (level %d::%d)\n",
				cid, q->fnm, q->lnr, mycur->txt, q->curly, q->round);
			do_unlock(cid);
		}
		if (q->mset[0] & SeenDecl) // checked before for this var
		{	mycur = mycur->nxt;
			continue;
		}
		q->mset[0] |= SeenDecl;

		if (q->curly == 0)		// must be a formal parameter
		{	if (q->round != 1)	// nope, means we dont know
			{	if (verbose>1)
				{	do_lock(cid);
					fprintf(stderr, "%d: %s:%d: '%s' untracked global (declared at %s:%d)\n",
						cid, mycur->fnm, mycur->lnr, mycur->txt, q->fnm, q->lnr);
					do_unlock(cid);
				}
				mycur = mycur->nxt;
				continue; 	// likely global, give up
			}
			q = move_to(q, "{");	// scope of param is the fct body
		} else
		{	Prim *r = q;
			while (q->curly == r->curly)
			{	q = q->prv;	// local decl: find start of fct body
		}	}

		if (!q)
		{	mycur = mycur->nxt;
			continue;		// give up
		}

		// located the block level where mycur->txt is declared:
		// now we must search this scope to mark all new
		// occurences that appear *after* mycur itself upto the
		// end of this level of scope

		if (0)
		{	do_lock(cid);
			fprintf(stderr, "%d: check_var %s:%d '%s' level %d <<%d %d>>\n",
				cid, mycur->fnm, mycur->lnr, mycur->txt, q->nxt->curly,
				mycur->seq, q->nxt->seq);
			do_unlock(cid);
		}
		check_var(mycur, q->nxt, cid);	// mark DerivedFromExternal
		mycur = mycur->nxt;
	}
}

static void *
check_scope_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	check_scope_range(from, upto, *i);

	return NULL;
}

static void
check_scope(void)
{
	run_threads(check_scope_run);
}

static void
prop_calls_range(Prim *from, Prim *upto, int cid)
{	Prim *mycur;

	mycur = from;
	while (mycur && mycur->seq <= upto->seq)	// propagation of tainted thru fct calls
	{	if (mycur->mark != 0
		&&  mycur->round > 0
		&&  mycur->curly > 0)
		{	search_calls(mycur, cid);
			// check if marked var is used as a param in a fct call
			// and remember these params in param_is_tainted
			// which is used below to mark vars derived from these
		}
		mycur = mycur->nxt;
	}
}

static void *
prop_calls_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	prop_calls_range(from, upto, *i);

	return NULL;
}

static void
prop_calls(void)
{
	run_threads(prop_calls_run);
}

static void
prop_params_range(Prim *from, Prim *upto, int cid)
{	Prim *mycur = from;
	Prim *q;
	int cnt = 0;

	while (mycur && mycur->seq <= upto->seq)	// propagation of tainted thru fct params and returns
	{	if (mycur->mset[2])			// for each function
		{	q = mycur;			  // save
			cnt += is_param_tainted(mycur, setlimit, cid);  // mark uses of tainted params PropViaFct
			mycur = q;		 	// restore
			search_returns(mycur, cid);	// used in next scan below
		}
		mycur = mycur->nxt;
	}
	tokrange[cid]->param = cnt;
}

static void *
prop_params_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	prop_params_range(from, upto, *i);

	return NULL;
}

static int
prop_params(void)
{	int cnt;

	cnt = run_threads(prop_params_run);
	return cnt;
}

static void
prop_returns_range(Prim *from, Prim *upto, int cid)
{	Prim *mycur, *r;
	int cnt = 0;

	// track use of tainted returns from fct calls
	for (mycur = from; mycur && mycur->seq <= upto->seq; mycur = mycur->nxt)
	{	if (strcmp(mycur->txt, "(") == 0
		&&  mycur->curly > 0
		&&  mycur->prv
		&&  strcmp(mycur->prv->typ, "ident") == 0
		&&  is_return_tainted(mycur->prv->txt, cid))
		{	// possible fct call returning tainted result
			r = mycur->prv->prv;	// token before the call
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
					if (verbose)
					{	show_bindings("iterate", r, mycur->prv, cid);
					}
					if (!r->bound)
					{	r->bound = mycur->prv; // origin, fct nm
					}
					cnt++;
	}	}	}	}
	tokrange[cid]->param = cnt;
}

static void *
prop_returns_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	prop_returns_range(from, upto, *i);

	return NULL;
}

static int
prop_returns(void)
{	int cnt;

	cnt = run_threads(prop_returns_run);
	return cnt;
}

static int
prop_iterate(void)	// PropViaFct
{	int cnt;

	// check propagation of the marked variables (vulnerable targets)
	// through fct calls or via return statements

	       prop_calls();	// parallel -- prop of tainted thru fct calls
	cnt  = prop_params();	// parallel -- prop of tainted thru fct params and returns
	cnt += prop_returns();	// parallel -- use of tainted returns from fct calls

	if (verbose)
	{	fprintf(stderr, "prop_iterate (%d) returns %d\n", PropViaFct, cnt);
	}

	return cnt;
}

static int
to_ignore(const char *s)
{	ConfigDefs *x;

	if (!s)
	{	return 1;
	}
	for (x = configged[IGNORE]; x; x = x->nxt)
	{	if (strcmp(x->nm, s) == 0)
		{	return 1;
	}	}
	return 0;
}

static int
handle_targets(Prim *q)
{	ConfigDefs *x;
	Prim *p, *r;
	int cnt = 0;

	if (q)
	for (x = configged[TARGETS]; x; x = x->nxt)
	{	if (strcmp(x->nm, q->txt) != 0)
		{	continue;
		}
		p = start_args(q);
		r = p->jmp;
		p = pick_arg(p, r, x->from);

		if (p->mark & (ExternalSource | DerivedFromExternal))
		{	if (verbose
			|| !no_display
			|| !no_match)	// no -terse argument was given
			{	cnt++;
				warnings++;
				if (json_format)
				{	sprintf(json_msg, "contents of '%s' in fopen() possibly tainted",
						p->txt);
					json_match("find_taint", json_msg, p, 0);
				} else
				{	printf(" %3d: %s:%d: warning: ", warnings, p->fnm, p->lnr); // n
					printf("contents of '%s' in fopen() possibly tainted", p->txt); // n
					what_mark(stdout, ": ", p->mark);
					printf("\n");	// n
			}	}
			nfopen++;
	}	}

	return cnt;
}

static void
percent_n_check(const char *a, Prim *q, int nr_commas)	// for *printf family
{	Prim *w;
	char *qtr;
	int nr;		// counts nr of format specifiers before %n
	int correction=0;

	w = q->nxt;				// (
	w = w->nxt;				// the format string for printf
	if (strcmp(w->typ, "ident") == 0)	// sprintf/fprintf/snprintf %n check
	{	w = w->nxt;			// skip ,
		w = w->nxt;			// the format string
		correction = 1;			// remember the extra arg
	}

	if (strcmp(q->txt, "snprintf") == 0)	// target is %n check here
	{	correction = 2;			// account for size arg
		while (strcmp(w->txt, ",") != 0)
		{	w = w->nxt;		// skip over size arg
		}
		w = w->nxt;
	}

	// have tainted arg in printf family, check for %n
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
			{	fprintf(stderr, "%d\tsaw %%n, nr %d nr_commas %d correction %d (%s)\n",
					q->lnr, nr, nr_commas, correction, a);
			}
			// the nr-th format conversion is %n
			// check if its that the one that was marked
			if (nr == nr_commas - correction)
			{	warnings++;
				if (json_format)
				{	sprintf(json_msg, "%s() uses '%%n' with corruptable arg '%s'",
						q->txt, a);
					json_match("find_taint", json_msg, q, 0);
				} else
				{	printf(" %3d: %s:%d: warning: %s() ",	// n
						warnings, q->fnm, q->lnr, q->txt);
					printf("uses '%%n' with corruptable arg '%s'\n", // n
						a);
			}	}
			break;
		case '%':	// ignore %%
			nr--;
			break;
		default:
			break;
	}	}
}

static Prim *
find_format_str(Prim *q, Prim *r)
{
	while (q
	&&     q->seq < r->seq
	&&     strcmp(q->typ, "str") != 0)
	{	q = q->nxt;
	}

	if (!q
	||  strcmp(q->typ, "str") != 0
	||  strstr(q->txt, "%s") == NULL)
	{	return NULL;
	}

	return q;
}

static int
propagate_mark(Prim *into, Prim *from)
{	int cnt = 0;
	int delta = 0;
	int m = 0;

	if (!from || !into)
	{	return 0;
	}

	if (from->mark & (ExternalSource | DerivedFromExternal))
	{	m |= DerivedFromExternal;
		delta++;
	}
	if (from->mark & (UseOfTaint | DerivedFromTaint))
	{	m |= DerivedFromTaint;
		delta++;
	}
	if (from->mark & PropViaFct)
	{	m |= PropViaFct;
		delta++;
	}
	if (from->mark & CorruptableSource)
	{	m |= CorruptableSource;
		delta++;
	}
	if (delta)
	{	cnt++;
		into->mark |= m;
		if (verbose > 1)
		{	fprintf(stderr, "%s:%d: marked propagator arg '%s' (+%d)\n",
				into->fnm, into->lnr, into->txt, m);
		}
	} else if (from->mark)
	{	fprintf(stderr, "%s:%d: propagate: '%s' unhandled case (%d)\n",
			from->fnm, from->lnr, from->txt, from->mark);
	}

	return cnt;
}

static int
propagate_str_args(Prim *q, Prim *r, Prim *t, int rev)	// t is dst or src, q is format string, r is bound
{	char *s = q->txt;
	int cnt = 0;

	do {	q = q->nxt;		// first arg after format string
	} while (q && strcmp(q->typ, "ident") != 0 && q->seq < r->seq);

	while (q && q->seq < r->seq && (s = strchr(s, '%')) != 0)
	{	s++;
		switch (*s) {
		case '%':
			s++;
			break;
		case 's':
			if (rev)
			{	cnt += propagate_mark(q, t);
			} else
			{	cnt += propagate_mark(t, q);
			}
			// fall thru
		default:
			q = next_arg(q, r);
			break;
	}	}

	return cnt;
}

static int
handle_propagators(Prim *p)
{	ConfigDefs *x;
	Prim *q, *r, *from, *into;
	int cnt = 0;

	if (!p)
	{	return 0;
	}

	for (x = configged[PROPAGATORS]; x; x = x->nxt)
	{	if (strcmp(p->txt, x->nm) == 0)	// eg memcpy
		{	q = start_args(p);
			r = q->jmp;				// bounds search for args

			// sscanf:   from 1  into 3+  (src, fmt, dst_args)
			// sprintf:  from 3+ into 1   (dst, fmt, src_args)
			// snprintf: from 4+ into 1   (dst, fmt, src_args)
			// the checks should be definable in the cfg.txt file

			// cf:
			// scanf:    (   fmt, dst_args)
			// fscanf:   (-, fmt, dst_args)

			if (strcmp(p->txt, "sscanf") == 0)
			{	from = pick_arg(q, r, 1);	// ignore x->from
				q = find_format_str(q, r);
				// find %s's and propagate from marks into corresponding args
				if (q)
				{	cnt += propagate_str_args(q, r, from, 1); // from is src
				}
			} else if (strcmp(p->txt, "sprintf") == 0
			||	   strcmp(p->txt, "snprintf") == 0) // handle %s
			{	into = pick_arg(q, r, 1);	// ignore x->into
				q = find_format_str(q, r);
				// find %s's and propagate marks from corresponding arg
				if (q)
				{	cnt += propagate_str_args(q, r, into, 0); // into is dst
				}
			} else					// eg strcpy
			{	from = pick_arg(q, r, x->from);
				into = pick_arg(q, r, x->into);
				cnt += propagate_mark(into, from);
	}	}	}

	return cnt;
}

static void
check_uses(const int v)	// used as args of: memcpy, strcpy, sprintf, or fopen
{	Prim *q;			// VulnerableTarget or CorruptableSource
	int cnt = 0;
	int nr_commas;

	for (cur = prim; cur; cur = cur->nxt)	// check suspect uses of tainted data
	{
		if (cur->round == 0
		|| cur->mark == 0
		|| (cur->mark & v))
		{	continue;
		}
		// not marked with v before
		if (verbose > 1)
		{	fprintf(stderr, "%s:%d: Check Uses, mark %d, txt %s round: %d\n",
				cur->fnm, cur->lnr, v, cur->txt, cur->round);
		}

		q = cur;
		nr_commas = 0;
		while (q->round >= cur->round)
		{	if (q->round == cur->round
			&&  strcmp(q->txt, ",") == 0)
			{	nr_commas++;	// nr of actual param
			}
			q = q->prv;		// work back to fct name
		}
		q = q->prv;	// fct name
		if (verbose > 1)
		{	fprintf(stderr, "\t\tfctname? '%s' round %d nrcommas %d (v=%d cm %d)\n",
				q->txt, q->round, nr_commas, v, cur->mark);
		}

		if (to_ignore(q->txt))
		{	continue;
		}

		cnt += handle_targets(q);
		cnt += handle_propagators(q);

		if (strstr(q->txt, "printf") != NULL)
		{	// in the printf family, if a %n format specifier occurs
			// then the corresponding argument ptr is vulnerable and
			// should not be a corruptable: warn if a marked arg links to %n
			percent_n_check(cur->txt, q, nr_commas);
	}	}

	if (verbose)
	{	fprintf(stderr, "%d of the potential targets used in suspicious calls (+%d)\n",
			cnt, v);
	}
}

static int
find_taintable(void)
{	Prim *q, *r;
	int cnt = 0;

	for (cur = prim; cur; cur = cur?cur->nxt:0)	// mark potentially corruptable targets
	{	// dont look inside struct declarations
		if (cur->curly > 0	// we're considering local decls only
		&&  (MATCH("struct")
		||   MATCH("union")))
		{	// may be a ref, not a def
			// check if we get to a ';' before we see a '{'
			while (!MATCH("{") && cobra_nxt())
			{	if (MATCH(";"))		// not a def, dont skip
				{	break;
			}	}
			if (!cur)
			{	break;
			}
			if (!MATCH(";") && cur->jmp)
			{	cur = cur->jmp;
			}
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
			{	fprintf(stderr, "possibly taintable (alloca): %s:%d: %s\n",
					cur->fnm, cur->lnr, cur->txt);
			}
			cur = q;	// restore
			continue;
		}
		if (TYPE("type")
		&&  cur->curly > 0	// local to fct
		&&  cur->round == 0)	// not a type cast
		{	q = cur;
findmore:
			while (!TYPE("ident")
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
					{	fprintf(stderr, "possibly taintable: %s:%d: %s\n",
							r->fnm, r->lnr, r->txt);
					}
					cnt++;
				}
				goto findmore;
			}
			cur = q;	// restore
	}	}

	if (verbose && cnt > 0)
	{	fprintf(stderr, "found %d potentially taintable sources (marked with %d)\n",
			cnt, Target);
	}

	return cnt;
}

static char *
multi_line_from_file(FILE *fd, int cid)
{	char *buf;
	char f[2];
	int c, n = 0;

	fseek(fd, 0L, SEEK_SET);

	while ((c = fgetc(fd)) != EOF)
	{	n++;
		if (c == '\n' || c == '\r')
		{	 n += 3;
	}	}
	if (!n)
	{	return " ";
	}

	n += strlen(json_msg) + 5;
	buf = (char *) hmalloc(n * sizeof(char), cid);
	if (!buf)
	{	return " ";
	}
	f[1] = '\0';
	fseek(fd, 0L, SEEK_SET);
	strcpy(buf, json_msg);
	strcat(buf, "<nl>");
	while ((c = fgetc(fd)) != EOF)
	{	if (c == '\n' || c == '\r')
		{	strcat(buf, "<nl>");
		} else
		{	f[0] = c;
			strcat(buf, f);
	}	}
	return buf;
}

static void
error_overflow(void)
{
	fprintf(stderr, "%s:%d: find_taint: internal error, nbuf too small\n",
		cur->fnm, cur->lnr);
}

static int
find_taints(void)		// completes Step 1 from taint_track.cobra script
{	Prim *q, *r, *s;	// marks UseOfTaint or DerivedFromTaint
	int cnt = 0;

	for (cur = prim; cur; cur = cur->nxt)	// find additonal uses of marked items, forward search
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
				{	if (cur->mset[3])		// was it marked ExternalSource before?
					{	char lbuf[2048];
						int len = 0, noprint = 1;	// by default, don't print these

						sprintf(lbuf, " %3d: %s:%d: '%s' in ", ++warnings,
							cur->fnm, cur->lnr, cur->txt);
						r = cur;
						while (r->round > 0 && r->round == cur->round)
						{	r = r->prv;
						}
						r = r->prv;	// token before ( ... ) if enclosed
						r = r->prv;	// undo first r->nxt
						len = strlen(lbuf);
						do {	r = r->nxt;
							if (len + strlen(r->txt) + 1 < sizeof(lbuf))
							{	strcat(lbuf, r->txt);
							}
							if (strcmp(r->txt, "gets") == 0)
							{	ngets_bad++;
								if (verbose || !no_display || !no_match)
								{	noprint = 0; // print if not terse
							}	}
						} while (r->seq != cur->nxt->seq);

						if (noprint == 0)			// not gets()
						{	if (!banner && !json_format)
							{	printf("=== Potentially dangerous assignments:\n"); // n
								banner = 1;
							}
							if (json_format)
							{	char nbuf[4096];
								sprintf(nbuf, "potentially dangerous: %s", lbuf);
								if (r->txt[0] == ','
								||  r->txt[0] == '[')
								{	strcat(nbuf, "... ");
								}
								// XXX additional detail for json_plus (multi-line)
								if (json_plus)
								{	FILE *fd;
									fd = tmpfile();
									if (fd)
									{	char *str;
										add_details(fd, cur, 1, cur, 3);
										str = multi_line_from_file(fd, 0);
										fclose(fd);
										if (strlen(nbuf)+strlen(str)+1 < sizeof(nbuf))
										{	strcat(nbuf, str);
										} else
										{	error_overflow();
								}	}	}

								json_match("find_taint", nbuf, cur, 0);
							} else
							{	printf("%s", lbuf);	// n
								if (r->txt[0] == ','
								||  r->txt[0] == '[')
								{	printf("..."); // n
								}
								cur->mset[1] |= UseOfTaint; // for add_details XXX
								cur->mbnd[1] = cur->bound;
								add_details(stdout, cur, 1, cur, 3);
							}
						} else
						{	warnings--;		// no warning issued, undo increment
						}
						cur->mark |= UseOfTaint;	// use of the taintable var
						break; // no need to tag subsequent uses of the same var
					}
					cur->mark |= UseOfTaint;	// use of the taintable var
					if (verbose)
					{	show_bindings("find_taints1", cur, q, 0);
					}
					if (!cur->bound)
					{	cur->bound = q;	// remember origin
					} // else fprintf(stderr, "1 does this happen?\n");
				} else
				{	break;	// already searched from here for q->txt
				}

				if (verbose > 1)
				{	fprintf(stderr, "%s:%d: marked place where %s is used (+%d -> %d [%d::%d])\n",
						cur->fnm, cur->lnr, q->txt, UseOfTaint,
						cur->mark, cur->mset[1], cur->mset[3]);
				}

				r = cur->prv;
				s = cur->nxt;
				if (strcmp(s->txt, "[") == 0		// array element
				||  strchr(s->txt, (int) '=') != 0)	// q followed by =
				{	cur = cur->nxt;
					continue;			// if =, q is overwritten
				}
				if (strcmp(r->txt, "&") == 0)		// q's address is taken
				{	r = r->prv;
				}
				if (strcmp(r->txt, "=") == 0)		// q used in assignment
				{	r = r->prv;
					if (strcmp(r->txt, "]") == 0)
					{	r = r->jmp;
						r = r?r->prv:r;
					}
					if (strcmp(r->typ, "ident") == 0
					&&  !(r->mark & DerivedFromTaint))
					{	r->mark |= DerivedFromTaint;	// newly tainted
						if (verbose)
						{	show_bindings("find_taints2", r, q, 0);
						}
						if (!r->bound)
						{	r->bound = q;	// remember origin
						} // else fprintf(stderr, "2 does this happen?\n");

						cnt++;
						if (verbose > 1)
						{	fprintf(stderr, "%s:%d '%s' tainted by asignment from taintable '%s'\n",
								r->fnm, r->lnr, r->txt, s->prv->txt);
				}	}	}
			// doesnt catch less likely assignments, like ptr = offset + buf
			// not yet: n.q = buf n->q = buf
				cur = cur->nxt;
			}
			cur = q;	// restore cur
	}	}

	if (verbose && cnt > 0)
	{	fprintf(stderr, "found %d propagated marks (%d)\n", cnt, UseOfTaint);
	}
	return cnt;
}

static void
add_markings(FILE *fd, Prim *q, const int a, const int b)
{	int none = 1;
	Prim *p;

	fprintf(fd, " %3d: %s:%d: in call to '%s'",	// n
		warnings, cur->fnm, cur->lnr, cur->txt);
	fprintf(fd, " marked arguments:");		// n
	for (p = q->nxt; p && p->round > cur->round; p = p->nxt)
	{	if (strcmp(p->txt, ",") == 0)
		{	continue;
		}
		if (p->mset[a] + p->mset[b] == 0)
		{	continue; // skip this arg
		}
		if (none)
		{	fprintf(fd, "\n");	// n
			none = 0;
		}
		fprintf(fd, "\t%s", p->txt);	// n
		add_details(fd, p, a, p, b);
	}
	if (none)
	{	fprintf(fd, "\n");		// n
	}
}

static void
issue_warning(Prim *q, const int a, const int b, const int cid)
{	FILE *fd = NULL;
	char *str = json_msg;

	warnings++;
	if (json_format)
	{	sprintf(json_msg, "potentially dangerous asgn in call to '%s'", cur->txt);
		if (json_plus)
		{	// XXX additional detail for json_plus (multi-line)
			fd = tmpfile();
			if (fd)
			{	add_markings(fd, q, a, b);
				str = multi_line_from_file(fd, cid);
				fclose(fd);
		}	}
		json_match("find_taint", str, cur, 0);
	} else
	{	if (!banner)
		{	printf("=== Potentially dangerous assignments:\n"); // n
			banner = 1;
		}
		add_markings(stdout, q, a, b);
	}
}

static void
check_contamination(const int a, const int b, const int cid)
{	Prim *p, *q;
	int cnt, nrcommas, nrdots, level;

	// dangerous assignments
	for (cur = prim; cur; cur = cur->nxt)		// assignments and fct calls
	{	if (strcmp(cur->typ, "ident") != 0)
		{	continue;
		}

		if (!is_propagator(cur->txt)
		||  strcmp(cur->txt, "sscanf") == 0)	// handled elsewhere
		{	continue;
		}

		// the first arg should have mset[a] != 0 -- ie a corruptable target
		// some the remaining args should have mset[b] != 0 -- ie an external source

		p = q = move_to(cur, "(");
		cnt = nrcommas = nrdots = 0;
		level = (p && p->nxt)?p->nxt->round:0;
		if (!p)
		{	continue;
		}
		do {
			p = p->nxt;
			if (strcmp(p->txt, ",") == 0)
			{	nrdots = 0;
				nrcommas++;
			} else if (strcmp(p->txt, ".") == 0
			       ||  strcmp(p->txt, "->") == 0)
			{	nrdots++;
			} else if (nrdots == 0 && p->bracket == 0 && p->round == level)
			{	if (p->mset[a] != 0 && nrcommas == 0)	// Taintable
				{	cnt |= 1;
				}
				if (p->mset[b] != 0 && nrcommas != 0)	// External
				{
					if (strstr(cur->txt, "printf") != NULL	// sprintf or snprintf: any arg
					||  nrcommas == 1)			// the others: 2nd arg only
					{	cnt |= 2;
			}	}	}
		} while (cnt < 3 && p && p->nxt && p->round > cur->round); // arg list
		if (cnt == 3)
		{	issue_warning(q, a, b, cid); // q is start of arg list
		}
	}
}

static void
usage(void)
{
	fprintf(stderr, "find_taint %s: unrecognized -- option(s): '%s'\n", tool_version, backend);
	fprintf(stderr, "valid options are:\n");
	fprintf(stderr, "	--json          generate the basic results in json format (has less detail)\n");
	fprintf(stderr, "	--json+         like --json, but includes more detail\n");
	fprintf(stderr, "	--sanity        start with a check for missing links in the input token-stream\n");
	fprintf(stderr, "	--sequential    disable parallel processing in backend (overruling -N settings)\n");
	fprintf(stderr, "	--limitN        with N a number, limit distance from fct call to fct def to Nk tokens\n");
	fprintf(stderr, "	--cfg=file      with file the name of a new configs file (in configs dir, or full path)\n");
	fprintf(stderr, "	--help          print this message\n");
	fprintf(stderr, "	--exit          immediately exit after startup phase in front end is completed\n");
	fprintf(stderr, "all diagnostic and error output is written to stderr (also when json format is used)\n");
	exit(1);
}

static void
sanity_check(void)
{	int B[6] = { 0, 0, 0 };
	int L[6] = { 0, 0, 0 };
	int cnt = 0;
	int i;

	for (cur = prim; cur; cur = cur?cur->nxt:0)
	{	cnt++;
		for (i = 0; i < 6; i++)
		{	if (strcmp(cur->txt, t_links[i]) == 0)
			{	if (cur->jmp == 0)
				{	B[i]++;
					if (i==5)
					{	fprintf(stderr, "missing link on '%s': %s:%d\n",
							t_links[i], cur->fnm, cur->lnr);
					}
				} else if (strcmp(cur->fnm, cur->jmp->fnm) != 0)
				{	L[i]++;
					fprintf(stderr, "cross-file link on '%s': %s:%d -> %s:%d (seq: %d -> %d\n",
						t_links[i],
						cur->fnm, cur->lnr,
						cur->jmp->fnm, cur->jmp->lnr,
						cur->seq, cur->jmp->seq);
	}	}	}	}

	fprintf(stderr, "%d tokens\n", cnt);
	fprintf(stderr, "tokens without link:\n");
	for (i = 0; i < 6; i++)
	{	fprintf(stderr, "\t'%s'=%d", t_links[i], B[i]);
	}
	fprintf(stderr, "\ncross-file links:\n");
	for (i = 0; i < 6; i++)
	{	fprintf(stderr, "\t'%s'=%d", t_links[i], L[i]);
	}
	fprintf(stderr, "\n");
}

static void
handle_args(void)
{	FILE *fd;
	char *arg;
	char *s = backend;

	track_fd = stdout;	// cobra_links.c

	while (strlen(s) > 0)
	{	if (strncmp(s, "limit", 5) == 0)
		{	arg = &s[5];
			if (isdigit((int) *arg))
			{	setlimit = atoi(arg);
				fprintf(stderr, "limiting distance from fct call to fct def to %d,000 tokens\n",
					setlimit);
				while (*s != ' ' && *s != '\0')
				{	s++;
				}
			} else
			{	usage();
			}
		} else if (strncmp(s, "sequential ", strlen("sequential ")) == 0)
		{	Ncore = 1;	// option --sequential, overrides -N from front-end
			fprintf(stderr, "disabled parallel processing in backend\n");
			s += strlen("sequential ");
		} else if (strncmp(s, "sanity ", strlen("sanity ")) == 0)
		{	sanity_check();
			s += strlen("sanity ");
		} else if (strncmp(s, "json", 4) == 0)
		{	json_format = 1;
			if (s[4] == '+')
			{	json_plus = 1;
				s++;
			}
			s += 5;
		} else if (strncmp(s, "cfg=", 4) == 0)
		{	arg = strchr(s, ' ');
			if (!arg)
			{	usage();
			}
			*arg = '\0';
			Cfg = &s[4];
			if ((fd = fopen(Cfg, "r")) == NULL)
			{	fprintf(stderr, "find_taint: no such file '%s'\n",
					Cfg);
				exit(1);
			}
			fclose(fd);
			s = arg;
			s++;
		} else if (strncmp(s, "exit ", 5) == 0)
		{	exit(0);
		} else
		{	usage();
	}	}
}

static int
phase_zero(void)
{
	if (json_format)
	{	printf("[\n");
	}

	set_multi();		// cwe_util.c
	ini_timers();		// front-end

	if (!taint_configs())	// read user-defined configs, if any
	{	if (json_format)
		{	printf("\n]\n");
		}
		return 0;
	}

	mark_fcts();	// prep: executes sequentially
	save_set(2);	// marking function definitions

	return 1;	// success
}

static int
phase_one(void)
{	int cnt = 0, any = 0;

	if (verbose > 1)
	{	fprintf(stderr, "\n=====Phase1 -- mark external sources\n");
	}

	cnt = find_sources();	// check most common types, mark as ExternalSource

	if (!cnt)
	{	fprintf(stderr, "find_taint: no sources of external input found\n");
		if (!p_debug)
		{	return 0;
		} else
		{	fprintf(stderr, "find_taint: debug mode, continuing scan anyway\n");
	}	}

	// mark derived external sources

	while (cnt)
	{	cnt = pre_scope();	// mark DerivedFromExternal	| parallel
		check_scope();		// mark DerivedFromExternal	| parallel
		cnt += prop_iterate();	// mark PropViaFct		| parallel
		any += cnt;
	}

	// check how the marked vars may be used
	// in dangerous calls, and mark dubious cases

	check_uses(VulnerableTarget); // sequential -- do any appear in dangerous calls

	return any;
}

static int
phase_two(void)
{	int cnt = 0;

	if (verbose > 1)
	{	fprintf(stderr, "\n=====Phase2 -- mark taintable targets\n");
	}

	// fixed size buffers declared on stack, marked Target
	// stack variables assigned with alloca; marked Alloca

	if (!find_taintable())
	{	fprintf(stderr, "find_taint: no taintable targets found\n");
		return 0;
	}

	// find anything that can point to Target or Alloca

	cnt = find_taints();	// mark: UseOfTaint, DerivedFromTaint	| not parallel, but not expensive

	// check propagation of marked vars thru fct params or returns
	// and mark vars derived from these

	reset_tables();

	while (prop_iterate() != 0)	// mark PropViaFct	| parallel
	{	cnt++;
	}

	// if any marked variables are there
	// now check if they can get assigned/filled with data
	// coming from untrusted sources
	// e.g., in sprintf, strcpy, or memcpy calls

	check_uses(CorruptableSource);

	// all potentially dangerous targets that appear in copy routines
	// are now marked CorruptableSource

	return cnt+1;	// have at least stack-buffers or we would have exited before
}

static void
phase_three(const int cid)
{
	if (verbose > 1)
	{	fprintf(stderr, "\n=====Phase3 -- check overlap\n");
	}

	check_contamination(1, 3, cid);	// assignment of Sources to Targets | not parallel

	if (verbose || !no_display || !no_match)	// not terse
	{	warnings -= (ngets + nfopen + ngets_bad);
	}
	if (ngets)
	{	fprintf(stderr, "%3d calls to gets() noted, %d writing to taintable objects\n", ngets, ngets_bad);
		// the ngets_bad are reported separately as well, and add to the count in 'warnings'
	}
	if (nfopen)
	{	fprintf(stderr, "%3d uses of potentially tainted arguments in calls to fopen()\n", nfopen);
	}
	if (warnings)
	{	fprintf(stderr, "%3d %staint warnings\n", warnings, (ngets+nfopen>0)?"other ":"");
		fprintf(stderr, "%3d total warnings\n", warnings+ngets+nfopen+ngets_bad);
	}
}

// externally visible

void
show_bindings(const char *s, const Prim *a, const Prim *b, int cid)
{
	do_lock(cid);
	if (a)
	{	fprintf(stderr, "Bind <%s>\t%s:%d: %s (%d)\tto ", s,
			a->fnm, a->lnr, a->txt, a->mark);
	}

	if (b)
	{	fprintf(stderr, "%s:%d: %s ::",
			b->fnm, b->lnr, b->txt);
		what_mark(stderr, " ", b->mark);
		fprintf(stderr, "\n");
	} else
	{	fprintf(stderr, "--\n");
	}

	if (a
	&&  a->bound
	&&  a->bound != b)
	{	fprintf(stderr, "\t\t[already bound to %s:%d: %s]\n",
			a->bound->fnm, a->bound->lnr, a->bound->txt);
	}
	do_unlock(cid);
}

void
cobra_main(void)
{
	handle_args();

	if (!phase_zero())	// prep, setup data structs
	{	return;		// setup failed
	}

	// One
	// 	mark data that could originate from external inputs
	// 	after locating the initial external sources, iterate
	//	to find
	// 	 1. additional uses within the same scope
	// 	 2. assignments to other variables (leading back to 1)
	// 	 3. parameters to function calls (leading back to 1 and 2)

	if (!phase_one() && !ngets)	// find potentially dangerous external sources
	{	if (json_format)
		{	printf("\n]\n");
		}
		return;
	}

	save_set(3);		// uses of VulnerableTarget, eg in memcpy, strcpy, sprintf

	// Two
	// 	look for fixed size buffers declared on the stack
	// 	these can be targets for code injection
	// 	pattern:  @type ^;* @ident [   ^;* ;
	//
	// 	this search for stack-based targets is simpler
	//	since they can only reasonably propagate through
	//	pointers to those targets (in assignments and fct calls)

	if (!phase_two())
	{	fprintf(stderr, "find_taint: no vulnerable targets found\n");
		if (json_format)
		{	printf("\n]\n");
		}
		return;	// no targets found
	}

	save_set(1);	// uses of CorruptableSource | parallel

	// Three
	// 	check if sets 1 and 3 overlap
	// 	meaning that an external source can be used in
	// 	a dangerous operation on a stack-based buffer

	phase_three(0);	// not parallel uet

	if (json_format)
	{	printf("\n]\n");
	}
}
