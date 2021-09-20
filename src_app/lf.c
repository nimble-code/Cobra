#include "c_api.h"

// print preprocessed code for a function
// e.g., ./lf -v -t fct_start lf.c

static Prim *stop;

static void
fct_start(void)
{
	if (!cobra_target)
	{	return;	// whole file
	}

	for ( ; cur; NEXT)
	{	if (TYPE("ident")
		&&  MATCH(cobra_target))
		{	NEXT;
			FIND("(");
			cur = cur->jmp;
			NEXT;
			if (MATCH("{"))
			{	stop = cur->jmp->nxt;
				do {	LAST;
				} while (!MATCH(";")
				      && !MATCH("}")
				      && !TYPE("cpp"));
				NEXT;
				break;
	}	}	}
}

static void
cobra_line(Prim *q)
{	static int last = 0;
	Prim *r;

	if (!q || q->lnr == last)
	{	return;
	}
	last = q->lnr;

	if (verbose)
	{	cobra_tag(q);
		printf("\t");
	}
	for (r = prim; r; r = r->nxt)
	{	if (r->lnr == q->lnr
		&&  r->fnm == q->fnm)
		{	printf("%s ", r->txt);
	}	}
	printf("\n");
}

void
cobra_main(void)
{
	if (!cobra_target)
	{	fprintf(stderr, "usage: ./lf -f fctname file.c\n");
		exit(1);
	}
	do {
		fct_start();
		for ( ; cur && cur < stop; NEXT)
		{	cobra_line(cur);
		}
	} while (cur);
}
