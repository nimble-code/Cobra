#include "c_api.h"

typedef struct Count Count;

struct Count {
	char *typ;
	int cnt;
	Prim *sample;
	Count *nxt;
};
Count *counts[128];

void
add_type(Prim *p)
{	Count *z;

	for (z = counts[p->curly]; z; z = z->nxt)
	{	if (strcmp(p->txt, z->typ) == 0)
		{	z->cnt++;
			return;
	}	}

	z = (Count *) malloc(sizeof(Count));
	z->typ = p->txt;
	z->cnt = 1;
	z->sample = p;
	z->nxt = counts[p->curly];
	counts[p->curly] = z;
}

void
report(void)
{	Count *z;
	int c;

	printf("scope\ttype\tcnt\tsample ref\n");
	for (c = 0; c < 128; c++)
	for (z = counts[c]; z; z = z->nxt)
	{	printf("%3d\t%s\t%3d\t%s:%d\n",
			c, z->typ, z->cnt,
			z->sample->fnm, z->sample->lnr);
	}
}

void
cobra_main(void)
{	Prim *q;

	for (cur = prim; cur; cur = cur->nxt)
	{	if (strcmp(cur->typ, "cpp") == 0
		&&  cur->curly == 0)
		{	q = cur;
			while (cur && q->lnr == cur->lnr)
			{	cur = cur->nxt;
			}
		}
		if (strcmp(cur->txt, "(") == 0
		&&  cur->jmp)
		{	cur = cur->jmp;
			continue;
		}
		if (strcmp(cur->typ, "type") == 0
		&&  strcmp(cur->prv->txt, "extern") != 0)
		{	q = cur->nxt;
			while (strcmp(q->txt, "*") == 0
			||     strncmp(q->txt, "__", 2) == 0)
			{	q = q->nxt;
			}
			if (strcmp(q->typ, "ident") == 0
			&&  strcmp(q->nxt->txt, "(") != 0
			&&  cur->curly >= 0
			&&  cur->curly < 128)
			{	add_type(cur);
	}	}	}
	report();
}
