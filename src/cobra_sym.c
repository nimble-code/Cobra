/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://codescrub.com/cobra
 */

#include "cobra.h"

#define R_MATCH(x)	(strcmp(r->txt, (x)) == 0)
#define R_TYPE(x)	(strcmp(r->typ, (x)) == 0)

// modifiers : long, short, signed, unsigned
// qualifiers: const, volatile
// storage   : static, extern, register, auto
// type      : int, char, float, double, void
// other     : struct, enum, union

static void *
declarations_range(void *arg)
{	int *i = (int *) arg;
	Prim *q, *r, *from, *upto;
	int local_cnt = 0;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
	{	if (R_MATCH("typedef"))
		{	do {	r = r->nxt;
			} while (!R_MATCH(";"));
			continue;
		}
		if (R_TYPE("type")
		&& !R_MATCH("void"))
		{	r->mark = 1;
			continue;
		}
		if (R_MATCH("struct")
		||  R_MATCH("union")
		||  R_MATCH("enum"))
		{	r->mark = 2;
			continue;
		}
		// add modifiers without explicit type (defaulting to int)
		if (!R_TYPE("modifier"))
		{	continue;
		}
		q = r;
		r = r->nxt;
		while (R_TYPE("qualifier"))
		{	r = r->nxt;
		}
		if (R_TYPE("type"))
		{	r = r->prv;
			continue;
		}
		q->mark = 3; // defaults to int
	}
	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
	{	if (!r->mark)
		{	continue;
		}
		r->mark = 0;
		q = r;
		r = r->nxt;
		if (R_MATCH("{"))	// unnamed struct, union, or enum
		{	continue;
		}
		while (R_MATCH("*")
		    || R_MATCH("&")
		    || R_TYPE("qualifier"))
		{	r = r->nxt;
		}

more:		if (R_TYPE("ident")
		&&  strcmp(r->nxt->txt, "(") != 0)
		{	// add_symtab(q, r);

			printf("%s:%d: %s %s\n",
				q->fnm, q->lnr,
				q->txt, r->txt);

			// so that it can be found later:
			r->bound = q;
			r->mark  = 1;

			local_cnt++;
		}
		r = r->nxt;

		if (R_MATCH("(")	// fct or fct ptr?
		||  R_MATCH("{"))	// struct, union, enum
		{	r = r->jmp;
			continue;
		}

//		if (R_MATCH("["))	{}	// array

		while (!R_MATCH(",")
		    && !R_MATCH(";")
		    && r->round >= q->round)
		{	r = r->nxt;
		}
		if (R_MATCH(","))
		{	r = r->nxt;
			goto more;
		}
	}

	tokrange[*i]->param = local_cnt;

	return NULL;
}

void
declarations(void)
{
	run_threads(declarations_range, 8);
}
