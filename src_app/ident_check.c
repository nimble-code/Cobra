#include "c_api.h"

static Prim *q;

void
cobra_main(void)
{
	q = prim;
	for ( ; cur; NEXT)
	{	if (TYPE("ident"))
		{	if (cur->len > q->len)
			{	q = cur;
	}	}	}
	printf("longest identifier: %s = %d chars\n", q->txt, q->len);
}
