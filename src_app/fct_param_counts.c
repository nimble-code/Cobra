#include "c_api.h"

// time fct_param_counts -z -n `ls Pre/*` > out
// on m2020: 112 sec.

static void
count_param(Prim *f)
{	Prim *q = f->nxt;	// (
	int cnt = 0;

	for (q = q->nxt; q; q = q->nxt)
	{	if (q->round == 0)	// matching )
		{	break;
		}
		if (q->round == 1	// top-level
		&&  strcmp(q->txt, ",") == 0)
		{	cnt++;
	}	}
	printf("%2d\t%s\n", (cnt==0)?0:cnt+1, f->txt);
}

void
cobra_main(void)
{	Prim *pm;

	// find function definitions
	for (cur = prim; cur; NEXT)
	{	if (cur->curly == 0		// eval !.curly
		&&  TYPE("ident"))		// m @ident
		{	pm = cur;
			NEXT;			// n
			if (!MATCH("("))
			{	cur = pm;
				continue;
			}
			cur = cur->jmp;	// j
			NEXT;
			if (!MATCH("{"))	// e {
			{	cur = pm;
				continue;
			}
			(void) count_param(pm);
	}	}
}
