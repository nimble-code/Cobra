#include "c_api.h"

// this compiled version is faster than play/flatten.cobra
// mostly because every line displayed from the
// script within cobra is a separate system-call

void
cobra_main(void)
{	int ln = 0;
	for (cur = prim; cur; NEXT)
	{	if (cur->curly
		 +  cur->round
		 +  cur->bracket == 0)
		{	if (cur->lnr != ln)
			{	printf("\n%3d ", cur->lnr);
				ln = cur->lnr;
			}
			printf("%s ", cur->txt);
	}	}
	printf("\n");
}
