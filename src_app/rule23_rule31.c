#include "c_api.h"

// requires -n

char *Rule23 = "=== Place #else, #elif, and #endif in the same file as the matching #if or #ifdef";
char *Rule31 = "=== Do not place code or declarations before an #include directive";

enum X { IF, ELSE, ENDIF };

typedef struct Cpp Cpp;
struct Cpp {
	enum X typ;
	Prim *tok;
	Cpp *nxt;
} *stack, *rel;

void
handle(enum X t)
{	Cpp *s = (Cpp *) 0;

	if (t == ENDIF)
	{	while (stack != NULL)
		{	s = stack;
			stack = stack->nxt; // pop
			s->nxt = rel;
			rel = s;
			if (s->typ == IF)
			{	break;
		}	} // else its an else or elif
		if (!s || s->typ != IF)
		{	printf("%s\n", Rule23);
			printf("%s:%d: #endif without matching #if\n",
				cur->fnm,
				cur->lnr);
		}
	} else	// push IF or ELSE
	{	if (rel)
		{	s = rel;
			rel = rel->nxt;
			memset(s, 0, sizeof(Cpp));
		} else
		{	s = emalloc(sizeof(Cpp), 15);
		}
		s->typ = t;
		s->tok = cur;
		s->nxt = stack;
		stack = s;
	}
}

void
reverse_print(Cpp *s)
{
	if (!s)
	{	return;
	}
	reverse_print(s->nxt);
	printf("  %s:%d: %s\n",
		s->tok->fnm,
		s->tok->lnr,
		s->tok->txt);
}

void
check_stack(char *f)
{
	if (!stack)
	{	return;
	}
	printf("%s\n", Rule23);
	printf("%s:1: missing #endif after:\n", f);
	reverse_print(stack);
	stack = (Cpp *) 0;
}

void
cobra_main(void)
{	char *fnm;
	int sawcode = 0;

	if (!no_cpp)
	{	fprintf(stderr, "usage: rule23_rule31 -nocpp file.c\n");
		return;
	}

	fnm = "";
	for (cur = prim; cur; NEXT)
	{	if (strcmp(fnm, cur->fnm) != 0)
		{	check_stack(fnm);
			fnm = cur->fnm;
			sawcode = 0;
		}
		if (MATCH(";"))
		{	sawcode++;
		}
		if (TYPE("cpp"))
		{	if (strstr(cur->txt, "#ifdef") != NULL
			||  strstr(cur->txt, "#if") != NULL)
			{	handle(IF);
			} else if (strstr(cur->txt, "#else") != NULL
			|| strstr(cur->txt, "#elif") != NULL)
			{	handle(ELSE);
			} else if (strstr(cur->txt, "#endif") != NULL)
			{	handle(ENDIF);
			}
			if (strstr(cur->txt, "#include") != NULL
			&&  sawcode)
			{	printf("%s\n", Rule31);
				printf("%s:%d: #include preceded by code\n",
					fnm, cur->lnr);
				sawcode = 0;
	}	}	}

	check_stack(fnm);
}
