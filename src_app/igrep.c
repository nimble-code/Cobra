#include "c_api.h"

static Prim *stop;

void
cobra_line(Prim *q)
{	static int last = 0;
	Prim *r;

	if (q->lnr == last)
	{	return;
	}
	last = q->lnr;

	cobra_tag(q);
	for (r = prim; r; r = r->nxt)
	{	if (r->lnr == q->lnr
		&&  r->fnm == q->fnm)
		{	printf("%s ", r->txt);
	}	}
	printf("\n");
}

Prim *
fct_start(void)
{	Prim *pm = (Prim *) 0;

	if (!cobra_target)
	{	return prim;	// whole file
	}

	for ( ; cur; NEXT)
	{	if (TYPE("ident")
		&&  MATCH(cobra_target))
		{	NEXT;
			if (!MATCH("("))
			{	continue;
			}
			pm = cur;
			cur = cur->jmp;
			NEXT;
			if (MATCH("{"))
			{	stop = cur->jmp;
				return pm;
	}	}	}

	return (Prim *) 0;
}

void
cobra_main(void)
{
	if (!cobra_texpr)
	{	fprintf(stderr, "usage: igrep -e name [-f fctname] file.c\n");
		exit(1);
	}
	cur = fct_start();
	for ( ; cur != stop; NEXT)
	{	if (TYPE("ident")
		&&  strcmp(cobra_texpr, cobra_txt()) == 0)
		{	cobra_line(cur);
	}	}
}
