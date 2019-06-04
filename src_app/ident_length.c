#include "c_api.h"

// ident_length -z -n `ls Pre/*` > out_i

void
cobra_main(void)
{
	for (cur = prim; cur; NEXT)
	{	if (TYPE("ident"))
		{	printf("%d\t%s\t%s:%d\n",
				cur->len,
				cur->txt,
				cur->fnm,
				cur->lnr);
	}	}
}
