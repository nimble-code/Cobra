#include <stdlib.h>
// #include <malloc.h>
#include <string.h>
#include <assert.h>
#include "c_api.h"

// bin: #binary operators
// bop: #boolean operators
// aop: #arithmetic operators

int bin, bop, aop;

Prim	*pm;
Prim	*L[32];		// only 14 needed
int	 N[256];	// 256: max nesting level of () + [] braces

void
reset(void)
{	int i;

	for (i = 0; i < 32; i++)
	{	L[i] = NULL;
		N[i] = 0;
	}
	bin = bop = aop = 0;
	pm  = NULL;
}

int
check_range(Prim *lo, Prim *hi)
{	Prim *q;

	for (q = lo->nxt; q < hi; q = q->nxt)
	{	if (strcmp(q->txt, "(") == 0
		||  strcmp(q->txt, ")") == 0
		||  strcmp(q->txt, "[") == 0
		||  strcmp(q->txt, "]") == 0)
		{	return 0;
	}	}
	return 1;
}

void
check(void)
{	Prim *ptr, *a = 0, *b = 0;
	int aj=0, bj, i, j, k = 0;

	// check if more than one operator appears
	// within the same level of parenthesis

	for (i = 0; i < 32; i++)
	{	N[i] = 0;
	}

	for (i = 1; i < 16; i++)
	{	if (!L[i])		// no operators at precedence level i
		{	continue;
		}
		// count operators at this level, per brace level
		
		if (++N[ L[i]->round + L[i]->bracket ] > 1)
		{	k++;		// more than one at at least one level
	}	}
	if (!k)
	{	return;
	}

	for (j = 1; j < 16; j++)
	{	if (L[j]
		&&  N[ L[j]->round + L[j]->bracket ] > 1)
		{	if (!a)
			{	a  = L[j];
				aj = j;
			} else
			{	b  = L[j];
				bj = j;
				if (a > b)
				{	Prim *tmp = a;
					int tj = aj;
					a = b;
					b = tmp;
					aj = bj;
					bj = tj;
				}
				if (!check_range(a, b))
				{	L[aj] = 0;
	}	}	}	} // else continue checking

	k = 0;
	for (j = 1; j < 16; j++)
	{	if (L[j]
		&&  N[ L[j]->round + L[j]->bracket ] > 1)
		{	k++;
	}	}
	if (k>1)
	{	printf("%s:%d: ", cobra_bfnm(), cobra_lnr()-1);
		for (ptr = pm; ptr && ptr <= cur; ptr = ptr->nxt)
		{	printf("%s ", ptr->txt);
		}
		printf("\n	precedence (high to low):  ");
		for (j = 1; j < 16; j++)
		{	if (L[j]
			&&  N[ L[j]->round + L[j]->bracket ] > 1)
			{	printf("%s, ", L[j]->txt);
		}	}
		printf("\n");
	}
}

void
cobra_main(void)
{
	for ( ; cur; NEXT)
	{	if (MATCH("++") || MATCH("--"))
		{	aop++; LAST;
			if (TYPE("ident"))
			{	NEXT; L[1] = cur;
			} else
			{	L[2] = cur; NEXT;
			}
			continue;
		}
		if (MATCH("~"))  { bin++; L[ 2] = cur; continue; }
	   	if (MATCH("*"))  { aop++; L[ 3] = cur; continue; }
	   	if (MATCH("/"))  { aop++; L[ 3] = cur; continue; }
	   	if (MATCH("%"))  { aop++; L[ 3] = cur; continue; }
	   	if (MATCH("+"))  { aop++; L[ 4] = cur; continue; }
	   	if (MATCH("-"))  { aop++; L[ 4] = cur; continue; }
	   	if (MATCH("<<")) { bin++; L[ 5] = cur; continue; }
	   	if (MATCH(">>")) { bin++; L[ 5] = cur; continue; }
		if (MATCH(">"))  { bop++; L[ 6] = cur; continue; }
		if (MATCH(">=")) { bop++; L[ 6] = cur; continue; }
		if (MATCH("<"))  { bop++; L[ 6] = cur; continue; }
		if (MATCH("<=")) { bop++; L[ 6] = cur; continue; }
	   	if (MATCH("==")) { bop++; L[ 7] = cur; continue; }
	   	if (MATCH("!=")) { bop++; L[ 7] = cur; continue; }
	   	if (MATCH("&"))  { bin++; L[ 8] = cur; continue; }
	   	if (MATCH("^"))  { bin++; L[ 9] = cur; continue; }
	   	if (MATCH("|"))  { bin++; L[10] = cur; continue; }
	   	if (MATCH("&&")) { bop++; L[11] = cur; continue; }
	   	if (MATCH("||")) { bop++; L[12] = cur; continue; }
	   	if (MATCH("?"))  { bop++; L[13] = cur; continue; }

	   	if (MATCH("="))	 // 14
		{	reset();
			pm = cur;	// start check here
			continue;
		}
		if (!pm)
		{	continue;
		}
	   	if (MATCH(";")
		||  MATCH(",")	 // 15
		||  cur->round < pm->round
		||  cur->curly < pm->curly)
		{	if ((aop + bop) && bin)
			{	check();
			}
			reset();
		}
	}
}
