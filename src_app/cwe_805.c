#include "cwe.h"

// CWE-805: buffer access with incorrect length value
// some simple cases that can be checked quickly

typedef struct ThreadLocal805 ThreadLocal805;

struct ThreadLocal805 {
	int w_cnt;
	int b_cnt;
	TrackVar *Names;
	char cacher[4096-3*sizeof(int)];
};

static ThreadLocal805 *thr;
static int first_e = 1;

extern TokRange **tokrange;	// c_util.c

static void
cwe805_init(void)
{	static int lastN = 0;

	if (lastN < Ncore)
	{	thr = (ThreadLocal805 *) emalloc(Ncore * sizeof(ThreadLocal805), 30);
		lastN = Ncore;
	} else
	{	memset(thr, 0, Ncore * sizeof(ThreadLocal805));
	}
}

static int
contains_pair(Prim *here, char *s, char *t, char *u)	// s followed by t or u
{	Prim *p;

	for (p = here; p != here->jmp; p = p->nxt)
	{	if (strcmp(p->txt, s) == 0)
		{	p = p->nxt;
			if (strcmp(p->txt, t) == 0
			||  strcmp(p->txt, u) == 0)
			{	return 1;
			}
			p = p->prv;
	}	}

	return 0;
}

static Prim *
file_extent(Prim *from)
{	Prim *upto = from;

	while (upto && strcmp(from->fnm, upto->fnm) == 0)
	{	upto = upto->nxt;
	}
	if (!upto)
	{	upto = plst;
	} else
	{	upto = upto->prv;
	}
	return upto;
}

static void
part1(Prim *from, Prim *upto, int cid)
{	Prim *p, *mycur = from;

	while (mycur_nxt() && mycur-> seq <= upto->seq)
	{	if (mycur->mark == 1)		// fct name
		{	p = mycur;
			while (!mymatch("{"))
			{	mycur_nxt();	// n {
			}
			if (mycur->jmp == NULL
			||  !contains_pair(mycur, "return", "0", "-"))
			{	continue;
			}
			while (!mymatch("("))
			{	mycur = mycur->prv;	// b )
			}
			if (mycur->jmp == NULL)
			{	mycur = p;
				continue;
			}
			mycur = mycur->jmp;		// j
			mycur = mycur->prv;		// b
			// remember names of fcts that contain return 0 or return -...
			store_var(&(thr[cid].Names), mycur, 0, cid);
	}	}
}

static void
part2(Prim *from, Prim *upto, int cid)
{	Prim *limit, *fc, *dest;
	Lnrs *q;
	Prim *mycur = from;

	while (mycur_nxt() && mycur->seq <= upto->seq)
	{	if (!mymatch("strncpy")
		&&  !mymatch("strncat")
		&&  !mymatch("memcpy"))
		{	continue;
		}
		mycur_nxt();
		if (!mymatch("(")
		||  mycur->jmp == NULL)
		{	continue;
		}
		limit = mycur->jmp;
		mycur_nxt();
		if (!mytype("ident"))
		{	continue;
		}
		dest = mycur;
		mycur_nxt();
		if (!mymatch(","))
		{	continue;
		}

		fc = mycur;		// first comma
		mycur_nxt();
		while (!mymatch(",") || mycur->round != fc->round)
		{	mycur_nxt();
		}
					// second comma
		mycur_nxt();
		while (mycur->seq < limit->seq)
		{	if (mymatch("sizeof"))
			{	mycur_nxt();	// (
				mycur_nxt();	// sizeof( ident ...
				if (mytype("ident")
				&&  !mymatch(dest->txt))
				{	if (no_display)
					{	thr[cid].w_cnt++;
					} else
					{	dest->mark = 805;
				}	}
				continue;
			}
			if (strcmp(mycur->nxt->txt, "(") != 0)	// not a fct call
			{	mycur_nxt();
				continue;
			}
			q = is_stored(thr[cid].Names, mycur, 0);
			while (q)
			{	if (no_display)
				{	thr[cid].b_cnt++;
				} else if (q->nv)
				{	q->nv->mark = 8050;
				}
				q = q->nxt;
			}
			mycur_nxt();
	}	}
}

void
cwe805_range(Prim *from, Prim *upto, int cid)
{	Prim *mycur = from;
	Prim *lst;

	do {	lst = file_extent(mycur);
		part1(mycur, lst, cid);		// one file at a time
		part2(mycur, lst, cid);
		thr[cid].Names = NULL;
		mycur = lst->nxt;
	} while (mycur && mycur->seq < upto->seq);
}

void *
cwe805_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	cwe805_range(from, upto, *i);

	return NULL;
}

void
cwe805_report(void)
{	Prim *mycur = prim;
	int at_least_one = 0;

	if (json_format && !no_display)
	{	for (; mycur; mycur = mycur->nxt)
		{	if (mycur->mark == 805
			|| mycur->mark == 8050)
			{	at_least_one = 1;
				printf("[\n");
				break;
	}	}	}

	for (; mycur; mycur = mycur->nxt)
	{	if (mycur->mark == 805)
		{	sprintf(json_msg,
			"suspicious sizeof in strncpy, strncat, or memcpy for other var than %s",
				mycur->txt);
			if (json_format)
			{	json_match("", "cwe_805", json_msg, mycur, 0, first_e);
				first_e = 0;
			} else
			{	printf("%s:%d: cwe_805: %s\n",
					mycur->fnm, mycur->lnr, json_msg);
			}
			mycur->mark = 0;
		} else if (mycur->mark == 8050)
		{	sprintf(json_msg, "fct '%s' may return a zero or negative value",
				mycur->txt);
			if (json_format)
			{	json_match("", "cwe_805", json_msg, mycur, 0, first_e);
				first_e = 0;
			} else
			{	printf("%s:%d: cwe_805: %s\n",
					mycur->fnm, mycur->lnr, json_msg);
			}
			mycur->mark = 0;
	}	}
	if (at_least_one)	// implies json_format
	{	printf("\n]\n");
	}
}

void
cwe805_0(void)
{
	cwe805_init();
	mark_fcts();

	run_threads(cwe805_run);

	if (no_display)
	{	int i, cnt1 = 0, cnt2 = 0;

		for (i = 0; i < Ncore; i++)
		{	cnt1 += thr[i].w_cnt;
			cnt2 += thr[i].b_cnt;
		}

		if (cnt1 > 0)
		{	fprintf(stderr, "cwe_805: %d warnings: suspicious ", cnt1);
			fprintf(stderr, "sizeof in strncpy, strncat, or memcpy\n");
		}

		if (cnt2 > 0)
		{	fprintf(stderr, "cwe_805: %d warnings: fct may return zero ", cnt2);
			fprintf(stderr, "or negative value (used in array index)\n");
		}
	} else
	{	cwe805_report();
	}
}
