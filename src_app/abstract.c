#include "c_api.h"

typedef struct Names Names;
struct Names {
	char *nm;
	int  cnt;
	Names *nxt;
} *names;

int
newname(char *s)
{	Names *n;
	static int cnt=1;

	for (n = names; n; n = n->nxt)
	{	if (strcmp(n->nm, s) == 0)
		{	return n->cnt;
	}	}
	n = (Names *) malloc(sizeof(Names));
	n->cnt = cnt++;
	n->nm = malloc(strlen(s)+1);
	strcpy(n->nm, s);
	n->nxt = names;
	names = n;
	return n->cnt;
}

void
cobra_main(void)
{
	for ( ; cur; NEXT)
	{	if (TYPE("ident"))
		{	if (verbose)
			{	printf("n_%d ", newname(cobra_txt()));
			} else
			{	printf("ident ");
			}
		} else
		{	printf("%s ", cobra_txt());
		}
		if (MATCH(";")
		||  MATCH("}")
		||  TYPE("cpp"))
		{	printf("\n");
		}
	}
}
