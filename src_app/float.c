#include "c_api.h"

typedef struct Names Names;
struct Names {
	char *nm;
	Names *nxt;
} *names;

static void
remem(char *s)
{	Names *n;

	for (n = names; n; n = n->nxt)
	{	if (strcmp(n->nm, s) == 0)
		{	return;
	}	}
	n = malloc(sizeof(Names));
	n->nm = malloc(strlen(s)+1);
	strcpy(n->nm, s);
	n->nxt = names;
	names = n;
}

static int
isfloat(char *s)
{	Names *n;

	for (n = names; n; n = n->nxt)
	{	if (strcmp(n->nm, s) == 0)
		{	return 1;
	}	}
	return 0;
}

void
cobra_main(void)
{	Prim *pm;
	for ( ; cur; NEXT)
	{	if (MATCH("float")
		||  MATCH("double"))
		{	do {	NEXT;
				while (MATCH("*"))
				{	NEXT;
				}
				while (MATCH("["))
				{	cur = cur->jmp;
					NEXT;
				}
				if (TYPE("ident"))
				{	remem(cobra_txt());
				}
				if (MATCH("="))
				{	do {	NEXT;
					} while (cur && !MATCH(",") && !MATCH(";"));
				}
			} while (MATCH(","));
		}
		if (MATCH("/")
		||  MATCH("/="))
		{	pm = cur;
			do {	NEXT;
				if (TYPE("ident")
				&&  isfloat(cobra_txt()))
				{	cobra_tag(pm);
					printf("division by float in: '");
					cobra_range(pm, cur);
					printf("'\n");
				}
			} while (!MATCH(";") && cur->round >= pm->round);
	}	}
}
