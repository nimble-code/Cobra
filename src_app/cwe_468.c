#include "cwe.h"

// CWE-468: incorrect pointer scaling
// https://cwe.mitre.org/data/definitions/468.html

// pattern: adding a constant to an identifier, in a parenthesized expression,
// which is cast to a pointer of some type

extern TokRange **tokrange;	// cwe_util.c

static int first_e = 1;

void
cwe468_range(Prim *from, Prim *upto, int cid)
{	Prim *q, *mycur;

	mycur = from;
	while (mycur && mycur->seq < upto->seq && mycur_nxt())
	{	if (mycur->round == 0
		||  !mytype("const_int"))
		{	continue;
		}
		q = mycur->prv;
		if (strcmp(q->txt, "+") != 0)
		{	continue;
		}
		while (strcmp(q->txt, "(") != 0)
		{	q = q->prv;
		}
		q = q->prv;	// (... *) ( ... + const_int ...)
		if (strcmp(q->txt, ")") != 0)
		{	continue;
		}
		q = q->prv;
		if (strcmp(q->txt, "*") != 0)
		{	continue;
		}
		mycur->mark = 468;
	}	
}

void *
cwe468_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	cwe468_range(from, upto, *i);

	return NULL;
}

void
cwe468_report(void)
{	Prim *mycur = prim;
	int w_cnt = 0;
	int at_least_one = 0;

	if (json_format && !no_display)
	{	for (; mycur; mycur = mycur->nxt)
		{	if (mycur->mark == 468)
			{	at_least_one = 1;
				printf("[\n");
				break;
	}	}	}

	for (; mycur; mycur = mycur->nxt)
	{	if (mycur->mark == 468)
		{	mycur->mark = 0;
			if (no_display)
			{	w_cnt++;
			} else
			{	sprintf(json_msg, "'%s': risky cast using pointer arithmetic",
					mycur->txt);
				if (json_format)
				{	json_match("", "cwe_468", json_msg, mycur, 0, first_e);
					first_e = 0;
				} else
				{	printf("%s:%d: cwe_468: %s\n",
						mycur->fnm, mycur->lnr, json_msg);
	}	}	}	}

	if (no_display && w_cnt > 0)
	{	fprintf(stderr, "cwe_468: %d warnings: risky cast using pointer arithmetic\n",
			w_cnt);
	}
	if (at_least_one)	// implies json_format
	{	printf("\n]\n");
	}
}

void
cwe468_0(void)
{
	run_threads(cwe468_run);
	cwe468_report();
}
