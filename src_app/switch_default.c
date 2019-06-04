#include "c_api.h"

// see also play/switch_default.cobra

void
cobra_main(void)
{	Prim *pm, *q;

	for ( ; cur; NEXT)
	{	FIND("switch"); NEXT;
		FIND("(");
		cur = cur->jmp;	// closing )
		NEXT; FIND("{");
		pm = cur;
		q = cur->jmp;		// closing }
		for ( ; cur != q; NEXT)
		{	if (MATCH("default")
			&&  cur->curly == q->curly)
			{	break;	// good
		}	}
		if (cur == q)		// bad
		{	printf("%s:%d: switch without default clause\n",
				cobra_bfnm(), pm->lnr);
		}
		cur = pm;
	}
}
