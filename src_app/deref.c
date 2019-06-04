#include "c_api.h"

static char *last_op = "";

int
s_match(char *t, char *s)	// at least one char from s appears in t
{	char *p;

	for (p = s; *p != '\0'; p++)
	{	if (strchr(t, *p))
		{	return 1;
	}	}
	return 0;
}

void
cobra_main(void)
{
	for ( ; cur; NEXT)
	{	if (MATCH("*"))
		{	LAST;
			if (TYPE("ident"))
			{	last_op = "";
			} else
			{	if (strlen(cobra_txt()) == 1
				&&  s_match(cobra_txt(), "+-*/{};,()"))
				{	last_op = "*";
				}
			}
			NEXT;
			continue;
		}
		if (TYPE("ident"))
		{	if (strcmp(last_op, "*") == 0)
			{	printf("%s:%d: dereference of: %s\n",
					cobra_bfnm(), cobra_lnr(), cobra_txt());
				last_op = "";
			}
		} else
		{	last_op = "";
	}	}
}
