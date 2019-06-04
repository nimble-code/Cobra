#include "c_api.h"

// see also: basic/ifelseif

static void
bad(Prim *ptr)
{
	printf("%s:%d: missing else - saw '%s' at line %d\n",
		ptr->fnm, ptr->lnr - 1, cur->txt, cur->lnr);
}

void
cobra_main(void)
{	Prim *ptr;

	for ( ; cur; NEXT)
	{	FIND("else"); NEXT;
		FIND("if");   NEXT;
		FIND("(");
		ptr = cur = cur->jmp;	// skip to end of condition
		NEXT;			// start of if-body
		FIND("{"); NEXT;	// should be a match
		for (;;)
		{	NEXT;	
			if (cur->curly == ptr->curly)
			{	if (!MATCH("else"))
				{	bad(ptr);
				}
				break;
			} else if (cur->curly < ptr->curly)
			{	bad(ptr);
				break;
		}	}		// else continue
		cur = ptr;		// resume check from here
	}
}
