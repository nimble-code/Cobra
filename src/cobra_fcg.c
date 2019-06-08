/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

#include "cobra.h"

FList **flist;
char *scrub_caption = "";
extern int no_caller_info;

static int path = 1;	// testing
static FList *find_match(FList *);
static FList *last_fct;

static void
add_def(const Prim *c, const Prim *r, const int ix)
{	FList *n, *prev_n = 0;
	const Prim *x;
	char *s = r->txt;	// fct name
	char *y = (char *) 0;	// ns namespace

	assert(ix >= 0 && ix < Ncore);

	if (cplusplus)
	{	x = r;	// fct name
		if (x
		&&  x->prv
		&&  strcmp(x->prv->txt, "::") == 0)
		{	x = x->prv->prv;	// ns
			if (x
			&&  strcmp(x->typ, "ident") == 0)
			{	y = x->txt;
	}	}	}

	// note that the same static fct nm can appear in different files
	for (n = flist[ix]; n; prev_n = n, n = n->nxt)
	{	int cmp = strcmp(n->nm, s);
		if (cmp > 0)
		{	break;	// build sorted list
		}
		if (cmp == 0
		&&  strcmp(n->p->fnm, r->fnm) == 0
		&&  (!cplusplus
		||   !y
		||   !n->ns
		||   strcmp(n->ns, y) == 0))
		{	return;	// already there
	}	}

	n     = (FList *) hmalloc(sizeof(FList), ix);
	n->nm = (char *)  hmalloc(strlen(s)+1, ix);
	strcpy(n->nm, s);

	n->p = (Prim *) r;	// fct name
	n->q = c;		// location of curly (start of body)

	if (cplusplus && y)
	{	n->ns = (char *) hmalloc(strlen(y)+1, ix);
		strcpy(n->ns, y);
	}

	// insert after prev_n
	if (!prev_n)
	{	n->nxt = flist[ix];
		flist[ix] = n;
	} else
	{	n->nxt = prev_n->nxt;
		prev_n->nxt = n;
	}
	last_fct = n;
//	printf("Add fct def %s\n", n->nm);
}

static void
add_caller(Prim *r, Prim *x, const int ix)
{	FList *c = (FList *) 0;
	char *fct = x->txt;
	char *ns = (char *) 0;

	assert(ix >= 0 && ix < Ncore);

	if (!last_fct)
	{	return;
	}

	if (cplusplus	// if the preceding symbol is ::
	&&  x->prv	// then use the ns that precedes it
	&&  x->prv->prv
	&&  strcmp(x->prv->txt, "::") == 0
	&&  strcmp(x->prv->prv->typ, "ident") == 0)
	{	ns = x->prv->prv->txt;
	} else
	{	ns = last_fct->nm;
	}

	for (c = last_fct->calls; c; c = c->nxt)
	{	if (strcmp(c->nm, fct) == 0
		&& (!cplusplus
		 || !ns
		 || !last_fct->ns
		 || strcmp(ns, last_fct->ns) == 0))
		{
//			printf("..nope %s %s\n", fct, ns);
			return; // already there
	}	}
// 	printf("Add caller %s -- %s\n", fct, ns);
	c = (FList *) hmalloc(sizeof(FList), ix);
	c->p   = r;
	c->nm  = fct;
	c->ns  = ns;
	c->nxt = last_fct->calls;
	last_fct->calls = c;
}

void *
fct_defs_range(void *arg)
{	Prim *ptr, *q, *z, *t;
	Prim *r, *from, *upto;
	int *i = (int *) arg;
	int preansi = 0;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
	{	if (strcmp(r->typ, "cpp") == 0)
		{	while (strcmp(r->txt, "EOL") != 0)
			{	r = r->nxt;
			}
			continue;	// skip over preprocessing stuff
		}

		if (strcmp(r->typ, "ident") != 0)
		{	continue;	// cannot be a fct name
		}

		if (cplusplus)
		{	if (r->round > 0)
			{	continue;
			}
			if (r->prv
			&& (strcmp(r->prv->txt, ",") == 0
			||  (r->prv->prv
			&&   strcmp(r->prv->prv->txt, ">") == 0)))
			{	continue;		// C++ confusion
			}
			if (strcmp(r->txt, "throw") == 0)
			{	continue;
			}
		} else if (r->curly > 0)
		{	// continue;	// allow, to get caller info
		}

		ptr = r;		// points at fct name candidate
		r = r->nxt;		// look-ahead
		if (!r
		||  strcmp(r->txt, "(") != 0
		||  !r->jmp)
		{	r = ptr;	// undo
			continue;	// not a function
		}
		q = r->jmp;		// end of possible param list

		if (q->nxt
		&& (strcmp(q->nxt->txt, ",") == 0
		||  strcmp(q->nxt->txt, ";") == 0))
		{	// continue;	// not a fct def -- allow to get caller info
		}

		// if there are no typenames in the param list
		// but there are identifiers, could be pre-ansi code
		preansi = 0;
		for (t = r->nxt; t != q; t = t->nxt)
		{	if (strcmp(t->typ, "type") == 0)
			{	preansi |= 1|2;
				break;
			}
			if (strcmp(t->typ, "ident") == 0)
			{	// at least one identifier
				preansi |= 1;
		}	}
		if (!(preansi & 2) && (preansi & 1))
		{	preansi = 1;
		} else
		{	preansi = 0;
		}
// printf("%2d -- %3d: %s -- preansi: %d\n", r->curly, ptr->lnr, ptr->txt, preansi);
		if (cplusplus)
		{	while (strcmp(q->nxt->typ, "qualifier") == 0)
			{	q = q->nxt;
			}
			if (strcmp(q->nxt->txt, "throw") == 0)
			{	int mx_cnt = 50;
				q = q->nxt;
				if (q->jmp)	// (
				{	q = q->jmp;	// )
				} else
				{	while (q->nxt
					&&     q->curly == ptr->curly
					&&     strcmp(q->nxt->txt, "{") != 0
					&&     strcmp(q->nxt->txt, ";") != 0
					&&     mx_cnt-- > 0)
					{	q = q->nxt;
		}	}	}	}

		if (!q
		||  !q->nxt
		||   q->curly != ptr->curly)
		{	continue;	// sanity check
		}

		// ptr points to fct name
		// r points to start of param list
		// q points to end   of param list
		// find { ... }

		z = q->nxt;
		while (z && strcmp(z->typ, "cmnt") == 0)
		{	z = z->nxt;	// skip over cmnts
		}

		// skip over pre-ansi param decls
		if (preansi
		&&  (strcmp(z->typ, "type") == 0
		||   strcmp(z->txt, "struct") == 0
		||   strcmp(z->txt, "register") == 0))
		{	while (z && strcmp(z->txt, "{") != 0)
			{	z = z->nxt;
		}	}

		// z points to first token after param list
		if (z && strcmp(z->txt, "{") == 0)
		{	add_def(z, ptr, *i);
		} else if (r->curly > 0 && !no_caller_info)
		{	add_caller(r, ptr, *i); // must be a fct call
	}	}

	return NULL;
}

static void
print_marked(const int val)
{	FILE *fd = (FILE *) 0;
	FList *f, *g, *h;
	int nodes = 0;
	int edges = 0;
	int ix;

	if (!cobra_target
	&& (fd = fopen(CobraDot, "a")) == NULL)
	{	printf("error: cannot create output file '%s'\n", CobraDot);
		return;
	}

	if (fd)
	{	fprintf(fd, "digraph fcg {\n");
	}

	for (ix = 0; ix < Ncore; ix++)
	for (f = flist[ix]; f; f = f->nxt)
	{	if (f->marked == val && f->calls)
		{	nodes++;
			f->p->mark = 1;
			for (g = f->calls; g; g = g->nxt)
			{	if ((h = find_match(g)) == NULL)
				{	continue;
				}
				if (h->marked == val)
				{	if (fd)
					{	fprintf(fd, "  %s_ -> %s_;\t// :: %s\n",
							f->nm, h->nm, f->ns?f->ns:"");
						edges++;
	}	}	}	}	}
	if (fd)
	{	fprintf(fd, "}\n");
		fclose(fd);
	}
	if (nodes > 0 && edges > 0)
	{	if (nodes <= 32)
		{	if (system(ShowDot) < 0)
			{	perror(ShowDot);
			}
			sleep(1);
			unlink(CobraDot);
		} else
		{	printf("wrote: %s (%d nodes, %d edges)\n",
				CobraDot, nodes, edges);
			printf("view with: !%s\n", ShowDot);
	}	}
}

static void
call_graph(void)
{	FList *f, *g;
	int ix;

	for (ix = 0; ix < Ncore; ix++)
	for (f = flist[ix]; f; f = f->nxt)
	{	f->marked = 1;
		for (g = f->calls; g; g = g->nxt)
		{	g->marked = 1;
	}	}
	print_marked(1);
}

static void
from_src(FList *f)
{	FList *g, *h;

	if (f->visited)
	{	return;
	}

	f->visited = 1;
	f->marked |= 1;
	for (g = f->calls; g; g = g->nxt)
	{	if ((h = find_match(g)) != NULL)
		{	from_src(h);
	}	}
}

static int
find_dest(FList *p, FList *to)
{	FList *f, *h;

	if (p->visited == 2)
	{	return 0;
	}
	p->visited = 2;

	if (strcmp(p->nm, to->nm) == 0)
	{	fprintf(stderr, " %s", to->nm);
		return 1;
	}

	for (f = p->calls; f; f = f->nxt)
	{	if ((h = find_match(f)) != NULL
		&&  (h->marked & 1))
		{	if (find_dest(h, to))
			{	fprintf(stderr, " <- %s", p->nm);
				return 1;
	}	}	}
	return 0;
}

static void
find_path(FList *fr, FList *to)
{	FList *f, *h;

	fprintf(stderr, "Sample Path:\n");
	for (f = fr->calls; f; f = f->nxt)
	{	if ((h = find_match(f)) == NULL)
		{	continue;
		}
		if (h->marked & 1)
		{	if (find_dest(h, to))
			{	fprintf(stderr, " <- %s\n", fr->nm);
				return;
	}	}	}
	fprintf(stderr, "not found\n");
}

static void
to_dest(FList *t)
{	FList *f, *g, *h;
	int ix;

	// does f call t

	for (ix = 0; ix < Ncore; ix++)
	for (f = flist[ix]; f; f = f->nxt)
	{	for (g = f->calls; g; g = g->nxt)
		{	if ((h = find_match(g)) == NULL)
			{	continue;
			}
			if (strcmp(h->nm, t->nm) == 0
			&&  (!t->ns
			||   !f->ns
			||   strcmp(t->ns, f->ns) == 0)
			&& (!(f->marked&2) || !(h->marked&2)))
			{	f->marked |= 2;
				h->marked |= 2;
				to_dest(f);
				break;
	}	}	}
}

static void
from_src_to_dest(FList *s, FList *t)
{
	from_src(s);
	to_dest(t);

	if (path)
	{	path = 2;
		find_path(s, t);
		path = 1;	// reset value
	}
}

static void
clear_flist(void)
{	FList *f, *g;
	int ix;

	for (ix = 0; ix < Ncore; ix++)
	for (f = flist[ix]; f; f = f->nxt)
	{	f->visited = f->marked = 0;
		for (g = f->calls; g; g = g->nxt)
		{	g->visited = g->marked = 0;
	}	}
}

static FList *
find_match(FList *z)
{
	if (!z->matched)
	{	z->matched = find_match_str(z->nm);
	}

	return z->matched;
}

static void
merge_lists(void)
{	FList *f, *nf;
	int ix;

	if (!flist)
	{	return;
	}

	// move all lists 1..Ncore-1 to ix 0, in case
	// the number of cores changes later

	for (ix = 1; ix < Ncore; ix++)
	for (f = flist[ix]; f; f = nf)
	{	nf = f->nxt;
		f->nxt = flist[0];
		flist[0] = f;	// no longer in sort-order
	}
}
			
// externally visible functions:

void
fct_defs(void)
{	static int maxn = 0;
	Prim *o_cur = cur;

	if (!flist || Ncore > maxn)
	{	flist = (FList **) emalloc(Ncore * sizeof(FList *));
		if (Ncore > maxn)
		{	maxn = Ncore;
		}
		run_bup_threads(fct_defs_range);

		cur = o_cur;	// run_bup_threads in backup(0) modifies it

		merge_lists();
	}
}

char *
fct_which(const Prim *p)
{	FList *f;
	int ix;

	if (!flist)
	{	fct_defs();
	}

	for (ix = 0; ix < Ncore; ix++)
	for (f = flist[ix]; f; f = f->nxt)
	{	if (f->p && f->q && f->q->jmp
		&&  p->lnr >= f->p->lnr
		&&  p->lnr <= f->q->jmp->lnr
		&& strcmp(f->p->fnm, p->fnm) == 0)
		{	return f->nm;
	}	}
	return "global";
}

FList *
find_match_str(char *s)
{	FList *f, *g = (FList *) 0;
	char *ns = (char *) 0;
	char *os = (char *) 0;
	int cnt = 0;
	int ix;

	if ((os = strstr(s, "::")) != NULL)
	{	*os = '\0';
		 ns = s;
		  s = os+strlen("::");
	}

	for (ix = 0; ix < Ncore; ix++)
	for (f = flist[ix]; f; f = f->nxt)
	{	if (strcmp(f->nm, s) == 0)
		{	if (!cplusplus
			||  !ns
			||  !f->ns
			||  strcmp(ns, f->ns) == 0)
			{	g = f;
				cnt++;
	}	}	}

	if (cnt > 1)
	{	if (path != 2)
		{	fprintf(stderr, "there are %d matches for '%s%s%s':\n",
				cnt, ns?ns:"", ns?"::":"", s);
			cnt = 1;
			for (ix = 0; ix < Ncore; ix++)
			for (f = flist[ix]; f; f = f->nxt)
			{	if (strcmp(f->nm, s) != 0)
				{	continue;
				}
				if (!cplusplus
				||  !ns
				||  !f->ns
				||  strcmp(ns, f->ns) == 0)
				{	fprintf(stderr, "%d\t%s%s%s\n",
						cnt++,
						f->ns?f->ns:"",
						f->ns?"::":"",
						f->nm);
		}	}	}
		g = (FList *) 0;
	}

	if (os)
	{	*os = ':';	// undo
	}

	return g;
}

void
fcg(char *from_f, char *to_f)
{	FList *src = (FList *) 0;
	FList *dst = (FList *) 0;

	if (ada)
	{	fprintf(stderr, "builtin function fcg is not (yet) available for Ada\n");
		return;
	}
	if (cplusplus)
	{	fprintf(stderr, "warning: fcg builtin support is still poor for C++\n");
	}

	if (!flist)
	{	fct_defs();
	} else
	{	clear_flist();
	}

	if (*from_f && *from_f != '*'
	&& (src = find_match_str(from_f)) == NULL)
	{	return;
	}
	if (*to_f && *to_f != '*'
	&& (dst = find_match_str(to_f)) == NULL)
	{	return;
	}

	if (src && !dst)
	{	from_src(src);
		print_marked(1);
	} else if (dst && !src)
	{	to_dest(dst);
		print_marked(2);
	} else if (src && dst)	
	{	from_src_to_dest(src, dst);
		path = (path==1)?2:0;
		print_marked(1|2);
		path = (path==2)?1:0;
	} else
	{	call_graph();
	}
}

void
findfunction(char *s, char *unused)
{	FList *f;

	if (!flist)
	{	fct_defs();
	}
	f = find_match_str(s);
	if (!f)
	{	printf("function '%s' not found\n", s);
		return;
	}

	printf("%s:%d-%d:\n",
		f->p->fnm, f->p->lnr,
		f->q->jmp?f->q->jmp->lnr:f->p->lnr);

	if (f->q->jmp
	&&  f->q->jmp->lnr > f->p->lnr)
	{	show_line(stdout, f->p->fnm, 0,
			f->p->lnr-1,
			f->q->jmp->lnr,
			f->p->lnr);
	}
}

void
context(char *s, char *unused)
{	FList *f, *g;
	FILE *fd;
	int ix;

	if (!flist)
	{	fct_defs();
	}

	f = find_match_str(s);
	if (!f)
	{	printf("function '%s' not found\n", s);
		return;
	}

	fd = fopen(CobraDot, "a");
	if (fd)
	{	fprintf(fd, "digraph fcontext {\n");
	} else
	{	printf("could not create digraph output in %s\n", CobraDot);
	}
	printf("%s:%d-%d\n",
		f->p->fnm, f->p->lnr,
		f->q->jmp?f->q->jmp->lnr:f->p->lnr);
	printf("calls:\n");
	for (g = f->calls; g; g = g->nxt)
	{	printf("\t%s:%d: %s()",
			g->p?g->p->fnm:"?",
			g->p?g->p->lnr:0,
			g->nm);
		if (cplusplus
		&&  g->ns
		&&  f->ns
		&&  strcmp(f->ns, g->ns) != 0)
		{	printf("	// %s::", g->ns);
		}
		printf("\n");
		if (fd)
		{	fprintf(fd, "	%s -> %s;\n",
				f->nm, g->nm);
	}	}
	printf("is called by:\n");
	for (ix = 0; ix < Ncore; ix++)
	for (f = flist[ix]; f; f = f->nxt)
	{	for (g = f->calls; g; g = g->nxt)
		{	if (strcmp(g->nm, s) == 0)
			{	printf("\t%s:%d: %s()",
					g->p?g->p->fnm:"?",
					g->p?g->p->lnr:0,
					f->nm);
				if (cplusplus
				&&  f->ns
				&&  g->ns
				&&  strcmp(f->ns, g->ns) != 0)
				{	printf("	// %s::", g->ns);
				}
				printf("\n");
				if (fd)
				{	fprintf(fd, "	%s -> %s;\n",
						f->nm, g->nm);
				}
				break;
	}	}	}
	if (fd)
	{	fprintf(fd, "}\n");
		fclose(fd);
		if (system(ShowDot) < 0)
		{	perror(ShowDot);
		}
		sleep(1);
		unlink(CobraDot);
	}
	if (0)
	{	printf("Dump:\n");
		for (ix = 0; ix < Ncore; ix++)
		for (f = flist[ix]; f; f = f->nxt)
		for (g = f->calls; g; g = g->nxt)
		{	if (strcmp(g->nm, "check_args") == 0
			||  strcmp(f->nm, "check_args") == 0)
			printf("ix %d, fct %s, calls %s\n", ix, f->nm, g->nm);
	}	}
}

void
show_line(FILE *fdo, const char *fnm, int n, int from, int upto, int tag)
{	FILE *fdi;
	char buf[MAXYYTEXT];
	int ln = 0;
	static int wcnt = 1;

	if (scrub && strlen(scrub_caption) > 0)
	{	fprintf(fdo, "cobra%05d <Med> :%s:%d: %s\n",
			wcnt++, fnm, tag, scrub_caption);
		if (upto == from)
		{	return;
	}	}

	if ((fdi = fopen(fnm, "r")) == NULL)
	{	printf("cannot open '%s'\n", fnm);
		return;
	}

	while (fgets(buf, sizeof(buf), fdi) && ++ln <= upto)
	{	if (ln >= from)
		{	if (scrub)
			{	if (buf[0] == '\n' || buf[0] == '\r')
				{	continue;
				}
				if (strlen(scrub_caption) > 0)
				{	fprintf(fdo, "\t");
				} else
				{	fprintf(fdo, "cobra%05d <Med> :%s:%d: ",
						wcnt++, fnm, ln);
				}
				fprintf(fdo, "%c", (ln == tag && upto > from)?'>':' ');
				goto L;
			}
			if (n > 0 && !cobra_texpr)
			{	fprintf(fdo, "%3d: ", n);
			}
			if (gui)
			{	fprintf(fdo, "%c%s:%05d:  ",
					(ln == tag && upto > from)?'>':' ',
					fnm, ln);
			} else
			{	fprintf(fdo, "%c%5d  ",
					(ln == tag && upto > from)?'>':' ',
					ln);
			}
L:			fprintf(fdo, "%s", buf);
	}	}

	fclose(fdi);
}
