#include "cwe.h"

// CWE-457: Use of uninitialized variable
// fast check to find some types of uninitialized variables

typedef struct ThreadLocal457 ThreadLocal457;

struct ThreadLocal457 {
	int depth;
	int canstop;
	int sawgoto;
	Prim *olimit;
	Prim *t[1024];	// thr[cpu_id].t[ thr[cpu_id].depth ]
	Prim *v[1024];
	Prim *r[1024];
};

static ThreadLocal457 *thr;

extern TokRange **tokrange;	// cwe_util.c

void
cwe457_init(void)
{	static int lastN = 0;

	if (lastN < Ncore)
	{	thr = (ThreadLocal457 *) emalloc(Ncore * sizeof(ThreadLocal457));
		lastN = Ncore;
	} else
	{	memset(thr, 0, Ncore * sizeof(ThreadLocal457));
	}
}

void
dfs_457(const char *nm, Prim *from, Prim *upto, int cpu_id)
{	Prim *b4, *af, *b5, *b44, *tt, *vv, *rr;
	Prim *yy, *zz, *ww;
	ThreadLocal457 *my;

	assert(cpu_id >= 0 && cpu_id < Ncore);

	my = &thr[cpu_id];
	my->depth++;

N:
	if (!upto
	||  !from
	||  my->canstop
	||  from->curly == 0	// outside fct somehow
	||  from->mark == 57)	// visited before
	{	my->depth--;
		return;
	}
// printf("%d-%d [%s] check %s -- sawgoto %d\n", from->lnr, upto->lnr, from->txt, nm, my->sawgoto);
	from->mark = 57;

	if (strcmp(from->typ, "cpp") == 0)
	{	while (from && strcmp(from->txt, "EOL") != 0)
		{	from = from->nxt;
			from->mark = 57;
		}
		if (!from)
		{	my->depth--;
			return;
		}
		from = from->nxt;
		goto N;
	}

	if (strcmp(from->txt, nm) == 0)	// target var
	{	b4 = from->prv;
		af = from->nxt;
		if (strcmp(b4->txt, "&") == 0	// address taken
		||  strcmp(b4->txt, ")") == 0	// cast?
		||  strcmp(b4->typ, "type") == 0
		|| (strcmp(b4->txt, ",") == 0 && from->round == 0)
		||  strcmp(af->txt, "*") == 0
		||  strcmp(af->txt, ".") == 0	// part of struct
		||  strcmp(af->txt, "=") == 0)	// assigned
		{	my->canstop = 1;
			my->depth--;
			return;		// assume ok
		}

		if (from->round >= 1)
		{	b5 = b4;
			while (b5->round > 0)	// was: strcmp(b5->txt, "(") != 0
			{	if (strcmp(b5->txt, "(") == 0)
				{	b5 = b5->prv;
					// in case it's inside a condition
					if (strstr(b5->txt, "get_user"))
					{	my->canstop = 1;
						my->depth--;
						return;
					}
				} else
				{	b5 = b5->prv;
			}	}
			b5 = b5->prv;
			if (strstr(b5->txt, "get_user")
			||  strstr(b5->txt, "spin_lock")
			||  strcmp(b5->txt, "volatile") == 0
			||  strstr(b5->txt, "for_each")
			||  strstr(b5->txt, "FOR_EACH"))	// linux-isms
			{	my->canstop = 1;
				my->depth--;
				return;
		}	}
		b44 = b4->prv;
		if ( strcmp(b4->txt, ".") != 0
		&&   strcmp(b4->txt, "->") != 0		// field in struct
		&&  strcmp(b44->txt, "sizeof") != 0	// not an eval
		&&  strstr(b44->txt, "get_user") == NULL)
		{	// ignore if we're in the increment field of a for stmnt
			if (from->round == 1)
			{	b4 = from->nxt;
				while (strcmp(b4->txt, ")") != 0)
				{	b4 = b4->nxt;
				}
				b4 = b4->jmp;	// start of for stmnt, if thats what it is
				if (!b4)
				{	my->depth--;
					return;
				}
				b4 = b4->prv;
			}
			if (strcmp(b4->txt, "for") != 0
			&&  strcmp(b4->typ, "type") != 0)
			{	from->mset[2] = 457; // avoid stepping on from->mark
// printf(">>>	mark (%s) %d\n", nm, from->lnr);
				my->canstop = 2;
				my->depth--;
				return;
	}	}	}

	if (strcmp(from->txt, "return") == 0
	||  strcmp(from->txt, "continue") == 0
	||  strcmp(from->txt, "exit") == 0)
	{	my->canstop = 1;
		my->depth--;
		return;		// no problem on this path
	}
	// end check

	my->t[my->depth] = from->bound;
	tt = from->bound;		// check if it was set
	if (!tt)			// not a goto or if/else/switch statement
	{	from = from->nxt;	// next statement in sequence
		if (from
		&&  from->seq < upto->seq)
		{	goto N;
		}
		my->depth--;
		return;
	}

	if (strcmp(from->txt, "goto") == 0)	// follow goto
	{	from = from->bound;
		upto = my->olimit;
		my->sawgoto = 1;	// doesnt handle nested switch statements with gotos
// printf("	Saw Goto\n");
		goto N;
	}

	if (strcmp(from->txt, "if") == 0)
	{	// check the condition first
		vv = from->nxt;
		if (strcmp(vv->txt, "(") != 0)
		{	from = vv;
			goto N;
		}
		my->v[my->depth] = from->nxt;	// the opening brace
		vv = vv->jmp;
		if (!vv)
		{	my->depth--;
			return;
		}
// printf("%d <condition %s>\n", from->lnr, nm);
		dfs_457(nm, my->v[my->depth], vv, cpu_id);
// printf("%d >condition %s<\n", from->lnr, nm);
		if (my->canstop)
		{	my->depth--;
			return;
		}
		// if there's an else, first stmnt in else
		// if not, first stmnt after then
		my->t[my->depth] = from->bound;
		tt = from->bound;
		my->r[my->depth] = tt->prv;
		rr = tt->prv;
		if (strcmp(rr->txt, "else") == 0)
		{
// printf("%d -> %d <else %s>\n", from->lnr, from->bound->lnr, nm);
			dfs_457(nm, from->bound, upto, cpu_id);	// else path, to end
// printf(">else %s<\n", nm);
			if (my->canstop == 0)		// no asgn and no err
			{
// printf("%d -- %d <then %s>\n", from->lnr, from->bound->lnr, nm);
				dfs_457(nm, from, from->bound, cpu_id);	// then path
// printf(">then %s<\n", nm);
				// up to from->bound because after else didnt have the var
			} else
			{	if (my->canstop == 1)	// saw asgn on else branch, but no error yet
				{	my->canstop = 0;	// explore new, this time the then branch
					vv = my->v[my->depth];
					yy = vv->jmp;	// end of condition
					if (!yy)
					{	my->depth--;
						return;
					}
					yy = yy->nxt;
					while (strcmp(yy->typ, "cmnt") == 0)
					{	yy = yy->nxt;
					}

					if (strcmp(yy->txt, "{") == 0)	// then body
					{	zz = yy->jmp;	// end of then part
					} else
					{	zz = yy->nxt;	// then stmnt
					}
// printf("<then2 %s>\n", nm);
					dfs_457(nm, yy, zz, cpu_id);	// then path
// printf(">then2 %s<\n", nm);
				// up to from->bound because after else didnt have the var
					if (my->canstop == 0)	// no asgn and no error
					{	rr = my->r[my->depth];
						rr = rr->bound;
						if (!rr)
						{	my->depth--;
							return;
						}
						rr = my->r[my->depth];
// printf("<after else %s>\n", nm);
						dfs_457(nm, rr->bound, upto, cpu_id);	// after else
// printf(">after else %s<\n", nm);
			}	}	}
		} else
		{
// printf("%d [%s] %s <else-less then %s>\n", from->lnr, from->txt, from->bound->txt, nm);
			dfs_457(nm, from->bound, upto, cpu_id);		// then part, no else, to end
// printf(">else-less then %s<\n", nm);
		}
		my->depth--;
		return;		// checked all if paths up to end
	}

	if (strcmp(from->txt, "switch") == 0)
	{	// check the condition first
		my->v[my->depth] = from->nxt;		// the opening brace
		vv = from->nxt;
// printf("Switch cond\n");
		dfs_457(nm, vv, vv->jmp, cpu_id);	// check switch expression
// printf("cond Switch\n");
		if (my->canstop)
		{	my->depth--;
			return;
		}
		my->t[my->depth] = from->bound;		// set if there's no default
		tt = from->bound;
		if (tt)
		{
// printf("Skip switch (no default\n");
			dfs_457(nm, tt, upto, cpu_id);	// skip switch
// printf("Skipped\n");
			if (my->canstop)
			{	my->depth--;
				return;
		}	}
		vv = my->v[my->depth];
		from = vv->jmp;			// end of condition
		if (!from)
		{	my->depth--;
			return;
		}
		from = from->nxt;		// continue scanning for cases
		if (from->seq < upto->seq)
		{	goto N;
		} else
		{	my->depth--;
			return;
	}	}

	if (strcmp(from->txt, "case") == 0
	||  strcmp(from->txt, "default") == 0)
	{	my->t[my->depth] = from->bound;	// should always be set
		tt = from->bound;
assert(tt);
		if (tt)			// explore this case
		{	while (strcmp(from->txt, ":") != 0)
			{	from = from->nxt;
			}
// printf("Case\n");
			dfs_457(nm, from, tt, cpu_id);	// from here to the next case
// printf("esaC%d\n", my->canstop);
			// ignoring break and continue for now
			if (my->canstop == 2)	// reported error on this path
			{	my->depth--;
				return;
			}
			if (my->canstop == 0)	// no assignment in the case clause
			{	// find end of switch statement
				ww = from;
				while (strcmp(ww->txt, "}") != 0
				||     ww->curly != from->curly - 1)
				{	ww = ww->nxt;
				}
				ww = ww->nxt;
// But, dont do this if the last case contained a goto....
// printf("aSuE (%d)\n", my->sawgoto);
				if (my->sawgoto == 0)
				{	dfs_457(nm, ww, upto, cpu_id);	// from after switch up to end
				} else
				{	my->sawgoto = 0;
				}
// printf("EuSa\n");
				if (my->canstop == 2)
				{	my->depth--;
					return;
			}	}
			// but wrong for the last case...
			tt = my->t[my->depth];
			if (tt->curly == from->curly)	// next case in same switch
			{	from = tt->prv;
				my->canstop = 0;		// to check the other cases
				if (from->seq < upto->seq)
				{	goto N;
	}	}	}	}
	from = from->nxt;
	my->depth--;			
}

void
cwe457_range(Prim *from, Prim *upto, int cpu_id)
{	Prim *start, *limit, *q;
	Prim *mycur;

	mycur = from;
	assert(cpu_id >= 0 && cpu_id < Ncore);
	while (mycur_nxt() && mycur->seq <= upto->seq)
	{
		if (mycur->mark != 1)	// for each function
		{	continue;
		}
		while (!mymatch("{"))
		{	mycur_nxt();
		}

		start = mycur;		// start of fct body
		limit = mycur->jmp;	// end of fct body
		if (!limit)
		{	continue;
		}
		thr[cpu_id].canstop = 0;
		clear_marks(start, limit);	// from any previous dfs

		while (mycur->seq < limit->seq)
		{
			if (mytype("cpp"))
			{	while (mycur && !mymatch("EOL"))
				{	mycur_nxt();
				}
				mycur_nxt();
				continue;
			}

			if (!mytype("type"))
			{	mycur_nxt();
				continue;
			}

			// start of a local declaration

			q = mycur->prv;
			if (strcmp(q->txt, "signed") == 0
			||  strcmp(q->txt, "unsigned") == 0)
			{	q = q->prv;
			}
			if (strcmp(q->txt, "static") == 0
			||  strcmp(q->txt, "extern") == 0)
			{	mycur_nxt();
				continue;	// not fct local
			}
			mycur_nxt();

			while (mymatch("*"))	// ignore decorations
			{	mycur_nxt();
			}

			// for each identifier
			while (mytype("ident") && mycur->round == 0 && mycur->curly == 1)
			{	if (strncmp(mycur->txt, "yy", 2) == 0)
				{	goto L;	// yacc
				}
				q = mycur;
				mycur_nxt();
				if (!mymatch("=")
				&&  !mymatch("[")
				&&  !mymatch("("))
				{	thr[cpu_id].canstop = 0;
					clear_marks(start, limit);
					thr[cpu_id].olimit = limit;
					thr[cpu_id].sawgoto = 0;
					dfs_457(q->txt, mycur, limit, cpu_id);
				}
				mycur = q;	// back to declaration
			L:	while (!mymatch(",")
				    && !mymatch(";"))
				{	mycur_nxt();
				}
				if (mymatch(","))
				{	mycur_nxt(); // move to next identifier
					while (mymatch("*"))
					{	mycur_nxt();
			}	}	}
			mycur_nxt();
	}	}
}

void
report_457(void)
{	int cnt = 0;

	cur = prim;
	while (cobra_nxt())
	{	if (cur->mset[2] == 457)
		{	if (!no_display)
			{	printf("%s:%d: cwe_457: uninitialized var %s?\n",
					cur->fnm, cur->lnr, cur->txt);
			}
			cur->mset[2] = 0;
			cnt++;
	}	}
	if (no_display && cnt > 0)
	{	printf("cwe_457: %d warnings: potentially uninitialized variable\n", cnt);
	}
}

void *
cwe457_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	cwe457_range(from, upto, *i);

	return NULL;
}

void
cwe457_0(void)
{
	cwe457_init();			// single-core

 	set_links();			// multi-core
	mark_fcts();

	run_threads(cwe457_run);	// multi-core

	report_457();			// single-core
}
