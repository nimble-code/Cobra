#include "cwe.h"

typedef struct ThreadLocal416 ThreadLocal416;

struct ThreadLocal416 {
	int depth;
	int canstop;
	Prim *olimit;
	Prim *t[1024];
	Prim *v[1024];
	Prim *r[1024];
};

static ThreadLocal416 *thr;
static int first_e = 1;

extern TokRange **tokrange;	// cwe_util.c

static void
cwe416_init(void)
{	static int lastN = 0;

	if (lastN < Ncore)
	{	thr = (ThreadLocal416 *) emalloc(Ncore * sizeof(ThreadLocal416));
		lastN = Ncore;
	} else
	{	memset(thr, 0, Ncore * sizeof(ThreadLocal416));
	}
}

void
dfs_var(char *nm, Prim *from, Prim *upto, int cid)
{	Prim *b4, *af, *b44, *tt;
	Prim *vv, *rr, *yy, *zz, *ww;

	thr[cid].depth++;
N:
	if (!from
	||  from->mark == 57	// visited before
	||  !upto
	||  thr[cid].canstop)
	{	thr[cid].depth--;
		return;
	}
	from->mark = 57;

	// target check for this statement
	if (strcmp(from->txt, nm) == 0)	// the target var
	{	b4 = from->prv;
		af = from->nxt;
		if (strcmp(af->txt, ".") == 0	// part of struct ref
		||  strcmp(af->txt, "=") == 0)	// assigned a new value
		{	thr[cid].canstop = 1;
			thr[cid].depth--;
			return;			// assumed ok
		}
		b44 = b4->prv;
		if (strcmp(b4->txt, ".") != 0
		&&  strcmp(b4->txt, "->") != 0	// field in struct
		&&  strcmp(b44->txt, "sizeof") != 0	// not an eval
		&&  strcmp(b44->txt, "get_user") != 0)
		{	// ignore if we're in the increment field of a for stmnt
			if (from->round == 1)
			{	b4 = from->nxt;
				while (strcmp(b4->txt, ")") != 0)
				{	b4 = b4->nxt;
				}
				b4 = b4->jmp;	// start of for stmnt, if thats what it is
				if (!b4)
				{	thr[cid].depth--;
					return;
				}
				b4 = b4->prv;
			}
			if (strcmp(b4->txt, "for") != 0)
			{	from->mark = 416;
				if (!no_display)
				{	from->bound = (Prim *) hmalloc(sizeof(Prim), cid);
					from->bound->txt = nm;
				}
				thr[cid].canstop = 2;
				thr[cid].depth--;
				return;
	}	}	}

	if (strcmp(from->txt, "return") == 0
	||  strcmp(from->txt, "continue") == 0
	||  strcmp(from->txt, "exit") == 0)
	{	thr[cid].canstop = 1;
		thr[cid].depth--;
		return;				// no problem on this path
	}
						// end check
	thr[cid].t[ thr[cid].depth ] = from->bound;
	tt = from->bound;			// check if it was set
	if (!tt)				// not a goto or if/else statement
	{	from = from->nxt;
		if (from && from->seq < upto->seq)
		{	goto N;
		}
		thr[cid].depth--;
		return;
	}

	if (strcmp(from->txt, "goto") == 0)	// follow goto
	{	from = from->bound;
		upto = thr[cid].olimit;
		goto N;
	}

	if (strcmp(from->txt, "if") == 0)
	{	// check the condition first
		vv = from->nxt;
		if (strcmp(vv->txt, "(") != 0)
		{	from = vv;
			goto N;
		}
		thr[cid].v[ thr[cid].depth ] = from->nxt;	// the opening brace
		vv = vv->jmp;
		if (!vv)
		{	thr[cid].depth--;
			return;
		}
		dfs_var(nm, thr[cid].v[ thr[cid].depth ], vv, cid);
		if (thr[cid].canstop)
		{	thr[cid].depth--;
			return;
		}

		// if there's an else, first stmnt in else
		// if not, first stmnt after then

		thr[cid].t[ thr[cid].depth ] = from->bound;
		tt = from->bound;
		thr[cid].r[ thr[cid].depth ] = tt->prv;
		rr = tt->prv;
		if (strcmp(rr->txt, "else") == 0)
		{	dfs_var(nm, from->bound, upto, cid);	// else path, to end
			if (thr[cid].canstop == 0)		// no asgn and no err
			{	dfs_var(nm, from, from->bound, cid);	// then path
				// up to from->bound because after else didnt have the var
			} else
			{	if (thr[cid].canstop == 1)	// saw asgn on else branch, but no error yet
				{	thr[cid].canstop = 0;	// explore new, this time the then branch
					vv = thr[cid].v[ thr[cid].depth ];
					yy = vv->jmp;	// end of condition
					if (!yy)
					{	thr[cid].depth--;
						return;
					}
					yy = yy->nxt;
					while (strcmp(yy->typ, "cmnt") == 0)
					{	yy = yy->nxt;
					}
	
					if (strcmp(yy->txt, "{") == 0)	// then body
					{	zz = yy->jmp;		// end of then part
						if (!zz)
						{	thr[cid].depth--;
							return;
						}
					} else
					{	zz = yy->nxt;	// then stmnt
					}
					dfs_var(nm, yy, zz, cid);	// then path
					if (thr[cid].canstop == 0)	// no asgn and no error
					{	rr = thr[cid].r[ thr[cid].depth ];
						rr = rr->bound;
						rr = thr[cid].r[ thr[cid].depth ];
						dfs_var(nm, rr->bound, upto, cid);	// after else
			}	}	}
		} else
		{	dfs_var(nm, from->bound, upto, cid);		// then part, no else, to end
		}
		thr[cid].depth--;
		return;		// checked all paths up to end
	}

	if (strcmp(from->txt, "switch") == 0)
	{	// check the condition first
		thr[cid].v[ thr[cid].depth ] = from->nxt;		// the opening brace
		vv = from->nxt;
		dfs_var(nm, vv, vv->jmp, cid);	// check switch expression
		if (thr[cid].canstop)
		{	thr[cid].depth--;
			return;
		}
		thr[cid].t[ thr[cid].depth ] = from->bound;			// set if there's no default
		tt = from->bound;
		if (tt)
		{	dfs_var(nm, tt, upto, cid);	// skip switch
			if (thr[cid].canstop)
			{	thr[cid].depth--;
				return;
		}	}
		vv = thr[cid].v[ thr[cid].depth ];
		from = vv->jmp;			// end of condition
		from = from->nxt;		// continue scanning for cases
		if (from->seq < upto->seq)
		{	goto N;
	}	}

	if (strcmp(from->txt, "case") == 0
	||  strcmp(from->txt, "default") == 0)
	{	thr[cid].t[ thr[cid].depth ] = from->bound;	// should always be set
		tt = from->bound;
		if (tt)					// explore this case
		{	while (strcmp(from->txt, ":") != 0)
			{	from = from->nxt;
			}
			from = from->nxt;		// start of body of case clause
			dfs_var(nm, from, tt, cid);	// from here to the next case
			// ignoring break and continue for now
			if (thr[cid].canstop == 2)	// reported error on this path
			{	thr[cid].depth--;
				return;
			}
			if (thr[cid].canstop == 0)	// no assignment in the case clause
			{	// find end of switch statement
				ww = from;
				while (ww->curly != from->curly - 1
				  ||   strcmp(ww->txt, "}") != 0)
				{	ww = ww->nxt;
				}
				ww = ww->nxt;
				dfs_var(nm, ww, upto, cid);	// from after switch up to end
				if (thr[cid].canstop == 2)
				{	thr[cid].depth--;
					return;
			}	}
	
			// wrong for the last case...
			tt = thr[cid].t[ thr[cid].depth ];
			if (tt->curly == from->curly)	// next case in same switch
			{	from = tt->prv;
				thr[cid].canstop = 0;		// to check the other cases
				if (from->seq < upto->seq)
				{	goto N;
		}	}	}
	}
	from = from->nxt;
	thr[cid].depth--;			
}

static void
cwe416_range(Prim *from, Prim *upto, int cid)
{	Prim *limit, *start, *q, *mycur;

	mycur = from;
	while (mycur && mycur->seq < upto->seq && mycur_nxt())
	{	int cnt;
		if (mycur->mark != 1)	// for each function
		{	continue;
		}

		// print "function " .txt "\n";
		cnt = 0;
		while (!mymatch("{") && cnt++ < 100)
		{	mycur_nxt();
		}
		if (cnt >= 100)
		{	continue;
		}

		start = mycur;		// start of fct body
		limit = mycur->jmp;	// end of fct body
		if (!limit)
		{	continue;
		}

		while (mycur->seq < limit->seq)
		{	if (!mymatch("free"))
			{	mycur_nxt();
				continue;
			}
			mycur_nxt();
			if (!mymatch("("))
			{	mycur_nxt();
				continue;
			}

			// use of free(...)

			mycur_nxt();
			if (!mytype("ident"))
			{	mycur_nxt();
				continue;
			}

			q = mycur;		// our target freed pointer variable
			mycur_nxt();		// look for another free
			if (!mymatch(")"))	// freeing a field in a struct?
			{	continue;
			}

			clear_marks(start, limit);
			thr[cid].canstop = 0;
			thr[cid].olimit = limit;
			dfs_var(q->txt, mycur, limit, cid);	// check if name is reused
			// not be cause the free could be inside an if-then-else
	}	}
}

static void
cwe416_report(void)
{	Prim *mycur = prim;
	int w_cnt = 0;
	int at_least_one = 0;

	if (json_format && !no_display)
	{	for (; mycur; mycur = mycur->nxt)
		{	if (mycur->mark == 416)
			{	at_least_one = 1;
				printf("[\n");
				break;
	}	}	}

	for (; mycur; mycur = mycur->nxt)
	{	if (mycur->mark == 416)
		{	if (no_display)
			{	w_cnt++;
			} else if (mycur->bound)
			{	sprintf(json_msg, "use after free of %s?",
					mycur->bound->txt);
				if (json_format)
				{	json_match("", "cwe_416", json_msg, mycur, 0, first_e);
					first_e = 0;
				} else
				{	printf("%s:%d: cwe_416: %s?\n",
						mycur->fnm, mycur->lnr, json_msg);
				}
				mycur->bound = 0;
			}
			mycur->mark = 0;
		}
		if (mycur->mark == 57)
		{	mycur->mark = 0;
	}	}
	if (no_display && w_cnt > 0)
	{	fprintf(stderr, "cwe_416: %d warnings: potential heap memory use after free\n",
			w_cnt);
	}
	if (at_least_one)	// implies json_format
	{	printf("\n]\n");
	}
}

static void *
cwe416_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	cwe416_range(from, upto, *i);

	return NULL;
}

void
cwe416_0(void)
{
	set_links();
	mark_fcts();

	cwe416_init();
	run_threads(cwe416_run);
	cwe416_report();
}
