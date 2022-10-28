#include "c_api.h"

// nr cases+default in switch statements
// nr_cases -z -n `ls Pre/*` > out_c

static void
do_count(Prim *f, Prim *q)
{	int cnt = 0;
	int nest = q->curly;

	while (q && q->curly >= nest) {
		if (strcmp(q->txt, "case") == 0
		||  strcmp(q->txt, "default") == 0)
		{	cnt++;
		}
		q = q->nxt;
	}
	printf("%3d\t%s:%d\n", cnt, f->fnm, f->lnr);
}

void
cobra_main(void)
{	Prim *pm;

	for (cur = prim; cur; NEXT)
	{	if (cur->curly > 0
		&&  MATCH("switch"))
		{	pm = cur;
			NEXT;
			if (!MATCH("("))
			{	cur = pm;
				continue;
			}
			cur = cur->jmp;
			NEXT;
			if (!MATCH("{"))
			{	cur = pm;
				continue;
			}
			do_count(pm, cur->nxt);
	}	}
}
