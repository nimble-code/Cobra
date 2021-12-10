#include "c_api.h"
#include <sys/times.h>

// requires preprocessing

char *Rule = "declare data objects at smallest possible level of scope";

typedef struct Fcts	Fcts;
typedef struct Ident	Ident;
typedef struct Typenm	Typenm;
typedef struct MyRange MyRange;

struct Fcts {
	int seq;
	Prim *p;	// fct name
	Prim *extent;	// closing }
	Fcts *nxt;
} *fcts, *lst;

// questions:
//	identifier appears at least once in global scope
//	is the identifier used in more than one file
//	is the identifier used in more than one fct

struct Ident {
	Prim *p;	// symbol name
	Fcts *scope;	// fct def if any
	char *fnm;	// filename, if unique
	int   global;	// saw in global scope
	int   tagged;	// saw with a static tag
	int   nrscopes;
	int   nrfiles;
	Ident *nxt;
} *htab[4096];

struct Typenm {
	int h;
	char *s;
	Typenm *nxt;
} *typenm;

struct MyRange {
	int seq;
	int fct_only;
	int file_only;
} **myrange;

TokRange	**tokrange;	// for cobra_prep.c

// the following shouldn't be necessary here
// because they are defined in c.ar
// but on the mac, scope_check doesn't compile
// without these; they are otherwise not used here

int json_format;
int json_plus;
int stream;
char *cobra_texpr;

// end of redundancies

int
hash_s(char *v)
{	unsigned int h = 0x88888EEFL;
 
	while (*v != '\0')
	{	h ^= ((h << 4) ^ (h >> 28))+ *v++;
	}
	return (int) (h ^ (h>>12));
}

void
record_type(char *s)
{	Typenm *t;
	int h = hash_s(s);

	for (t = typenm; t; t = t->nxt)
	{	if (t->h == h
		&&  strcmp(t->s, s) == 0)
		{	return;
	}	}
	t = (Typenm *) emalloc(sizeof(Typenm));
	t->h = h;
	t->s = s;
	t->nxt = typenm;
	typenm = t;
}

int
is_typenm(char *s)
{	Typenm *t;
	int h = hash_s(s);

	for (t = typenm; t; t = t->nxt)
	{	if (t->h == h
		&&  strcmp(t->s, s) == 0)
		{	return 1;
	}	}
	return 0;
}

int
likely_static(Prim *p)
{
	if (!p || !p->prv)
	{	return 0;
	}
	p = p->prv;
	while (p
	&&     (strcmp(p->txt, "}") != 0 || p->curly > 0)
	&&     (strcmp(p->txt, ";") != 0 || p->curly > 0)
	&&     strcmp(p->txt, "static") != 0)
	{	p = p->prv;
	}
	if (p && strcmp(p->txt, "static") == 0)
	{	return 1;
	}
	return 0;
}

void
new_ident(Fcts *sc, Prim *id, int is_static)
{	Ident *i;
	int h = hash_s(id->txt)&4095;

	assert(h >= 0 && h < 4096);

	for (i = htab[h]; i; i = i->nxt)
	{	if (strcmp(i->p->txt, id->txt) == 0)
		{	// same name
			if (i->scope && sc && i->scope != sc)
			{	i->nrscopes++;
			}
			if (strcmp(i->fnm, id->fnm) != 0)
			{	i->nrfiles++;
			}
			if (!sc)
			{	i->global++;
				i->p = id;
			} else if (!i->scope)
			{	i->scope = sc;
				i->nrscopes = 1;
			}
			break;
	}	}

	if (!i)
	{	i = (Ident *) emalloc(sizeof(Ident));
		i->p = id;
		i->scope = sc;
		i->fnm = id->fnm;
		if (sc)
		{	i->nrscopes = 1;
		} else
		{	i->global = 1;
		}
		i->nrfiles = 1;
		i->tagged = is_static || likely_static(id->prv);
		i->nxt = htab[h];
		htab[h] = i;
	}
}

void
new_fct(Prim *p)
{	Fcts *f = (Fcts *) emalloc(sizeof(Fcts));

	f->p = p;
	p = p->nxt;		// (
	p = p->jmp;		// )
	while (strcmp(p->nxt->typ, "cmnt") == 0)
	{	p = p->nxt;
	}
	p = p->nxt;		// {
	f->extent = p->jmp;	// }

	if (!lst)
	{	f->seq = 0;
		fcts = lst = f;
	} else
	{	f->seq = lst->seq + 1;
		lst->nxt = f;
		lst = f;
	}
}

int
in_range(Fcts *f)
{
	// cobra_eval.c <-> cobra_eval.y problem
	if (!f
	||  !f->extent
	||  (cur->curly == 0 && strcmp(f->p->fnm, cur->fnm) != 0))
	{	return 0;
	}
	return (f
	&&      cur->lnr >= f->p->lnr
	&&      cur->lnr <= f->extent->lnr);
}

int
bad_pre(char *s)
{
	if (strcmp(s, "goto") == 0
	||  strcmp(s, "->") == 0
	||  strcmp(s, ".") == 0)
	{	return 1;
	}
	return 0;
}

int
bad_post(char *s)
{
	if (strcmp(s, ":") == 0)
	{	return 1;
	}
	return 0;
}

int
static_fct(char *s)
{	Fcts *f;

	for (f = fcts; f; f = f->nxt)
	{	if (strcmp(f->p->txt, s) == 0)
		{	return likely_static (f->p->prv);
	}	}
	return -1;	// not a function
}

void
checkit(const int cpu, Ident *i)
{	int x;

	if (!i->global	// never seen in global scope
	||  i->tagged	// static
	|| strncmp(i->p->txt, "YY_", 3) == 0
	|| strncmp(i->p->txt, "yy", 2) == 0
	|| strncmp(i->p->txt, "flex_", 5) == 0)
	{	return;
	}
	assert(cpu >= 0 && cpu < Ncore);

	// seen in global scope
	// and i->p points to one such use
	// if i->p->curly > 0, it is a structure or enum field
	// and we only report on the structure itself

	if (i->p->curly > 0		// in structure or enum
	||  i->p->round > 0		// in parameter list
	||  is_typenm(i->p->txt))
	{	return;
	}

	if (i->nrscopes == 1)
	{	myrange[cpu]->fct_only++;
		i->p->mark = 1;
		i->p->typ = (i->scope && i->scope->p)?i->scope->p->txt:"global";
		if (verbose)
		{	printf("%s\tis only used in scope %s\n",
				i->p->txt, i->scope->p->txt);
		}
		return;
	}

	if (i->nrfiles == 1)
	{	x = static_fct(i->p->txt);
		if (x == 1)
		{	return;
		}
		myrange[cpu]->file_only++;
		i->p->mark = 2;
	//	printf("%s", (x==0)?"fct ":"");
		if (verbose)
		{	printf("%s\tis only used in file %s\n",
				i->p->txt, i->fnm);
	}	}
}

static void *
range_check(void *arg)
{	Ident *z;
	int y, from, upto;
	int *i = (int *) arg;

	from = (*i) * (4096/Ncore);
	upto = (*i == Ncore-1) ? 4096 : from + 4096/Ncore;

	for (y = from; y < upto; y++)
	for (z = htab[y]; z; z = z->nxt)
	{	checkit(*i, z);
	}

	return NULL;
}

void *(*fct)(void*) = range_check;

void
cobra_main(void)
{	Fcts *f, *scope;
	pthread_t *t_id;
	Prim *q;
	int fct_only = 0, file_only = 0;
	int i, is_static = 0;
	clock_t start_time, stop_time;
	struct tms start_tm, stop_tm;
	double delta_time;

	if (!prim)
	{	fprintf(stderr, "scope_check: no tokens\n");
		return;
	}

	if (!cobra_commands)
	{	cobra_commands = "Rule";
	}

	start_time = times(&start_tm);

	// find functions and user-defined typenames
	for (cur = prim; cur; NEXT)
	{	if (TYPE("ident"))
		{	if (cur->curly == 0
			&&  cur->nxt
			&&  strcmp(cur->nxt->txt, "(") == 0
			&&  cur->nxt->jmp
			&&  cur->nxt->jmp->nxt
			&&  strcmp(cur->nxt->jmp->nxt->txt, "{") == 0)
			{	// fct definition
				new_fct(cur);
		}	}
		if (MATCH("struct") && cur->nxt)
		{	record_type(cur->nxt->txt);
		} else if (MATCH("typedef"))
		{	for (q = cur->nxt; q; q = q->nxt)
			{	if (strcmp(q->txt, ";") == 0)
				{	record_type(q->prv->txt);
					break;
	}	}	}	}

	// associate identifiers with enclosing scope
	f = fcts;	// points to fct that we could be in
	for (cur = prim; cur; NEXT)
	{	if (in_range(f))
		{	scope = f;
		} else if (f && in_range(f->nxt))
		{	f = f->nxt;
			scope = f;
		} else
		{	scope = (Fcts *) 0; // global
		}
		if (strcmp(cur->txt, "static") == 0)
		{	is_static = 1;
		}
		if (cur->round > 0
		||  strcmp(cur->txt, ";") == 0)
		{	is_static = 0;
		}
		if (TYPE("ident")
		&& strncmp(cur->fnm, "/usr", 3) != 0)
		{	if (cur->nxt && !bad_post(cur->nxt->txt)
			&&  cur->prv && !bad_pre(cur->prv->txt))
			{	new_ident(scope, cur, is_static);
	}	}	}

	// check each identifier found, multicore
	t_id    = (pthread_t *) malloc(Ncore * sizeof(pthread_t));
	myrange = (MyRange **)  malloc(Ncore * sizeof(MyRange *));

	for (i = 0; i < Ncore; i++)
	{	myrange[i] = (MyRange *) malloc(sizeof(MyRange));
		myrange[i]->seq = i;
		myrange[i]->fct_only = 0;
		myrange[i]->file_only = 0;
		(void) pthread_create(&t_id[i], 0, fct,
			(void *) &(myrange[i]->seq));
	}

	for (i = 0; i < Ncore; i++)
	{	(void) pthread_join(t_id[i], 0);
		fct_only += myrange[i]->fct_only;
		file_only += myrange[i]->file_only;
	}

	stop_time = times(&stop_tm);

	if (fct_only + file_only > 0)
	{	printf("=== %s: %s\n", cobra_commands, Rule);
	}

	printf("	globals used in one scope only: %3d\n", fct_only);
	if (!no_display)
	{	for (cur = prim; cur; NEXT)
		{	if (cur->mark == 1)
			{	printf("	%s\tused in only one %s (%s)\n",
					cur->txt, cur->curly==0?"scope":"function",
					cur->typ);
	}	}	}

	printf("	globals used in one file  only: %3d\n", file_only);
	if (!no_display)
	{	for (cur = prim; cur; NEXT)
		{	if (cur->mark == 2)
			{	printf("	%s\tused in only file %s\n",
					cur->txt, cur->fnm);
	}	}	}

	delta_time = ((double) (start_time -stop_time))
		   / ((double) sysconf(_SC_CLK_TCK));

	if (!no_match
	&&  !no_display
	&&  delta_time > 1.0)
	{	printf("    (%.3g sec)\n", delta_time);
	}

	// for all Linux 2.4.1 sources (8,301 files, 3.7 Mlines, 2.6 M NCS)
	// scope_check -n -z -N? `cat c_files`
	// 35 sec to build the data structure
	//  9 sec for the sequential part
	//  9 sec -N16..30 to do the parallel part of the check
	// 11 sec -N8
	// 20 sec -N4
	// 38 sec -N2
	// 52 sec -N1
}
