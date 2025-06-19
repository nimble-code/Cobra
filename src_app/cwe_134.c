#include "cwe.h"

// CWE-134: Use of Externally-Controlled Format String

// memcpy with 2nd arg unchecked param

typedef struct ThreadLocal134 ThreadLocal134;

struct ThreadLocal134 {
	TrackVar *params;
};

static ThreadLocal134 *thr;
static int first_e = 1;

extern TokRange **tokrange;	// c_util.c

static void
cwe134_init(void)
{	static int lastN = 0;

	if (lastN < Ncore)
	{	thr = (ThreadLocal134 *) emalloc(Ncore * sizeof(ThreadLocal134));
		lastN = Ncore;
	} else
	{	memset(thr, 0, Ncore * sizeof(ThreadLocal134));
	}
}

static void
cwe134_range(Prim *from, Prim *upto, int cid)
{	Prim *eop, *eob, *mycur;

	mycur = from;
	while (mycur && mycur->seq < upto->seq && mycur_nxt())
	{	if (mycur->mark == 0)
		{	continue;
		}
		// fct name

		mycur_nxt();		// (
		if (!mymatch("("))
		{	continue;
		}			// param list

		eop = mycur->jmp;
		thr[cid].params = 0;

		while (mycur->seq < eop->seq)
		{	if (mytype("ident"))
			{	mycur = mycur->prv;
				if (mymatch("*"))	// pointer
				{	mycur_nxt();
					store_var(&(thr[cid].params), mycur, 0, 0);
				} else
				{	mycur_nxt();
			}	}
			mycur_nxt();
		}
		mycur_nxt();		// {

		while (!mymatch("{"))
		{	 mycur_nxt();
		}
		if (!mymatch("{"))	// fct body
		{	continue;
		}

		eob = mycur->jmp;
		if (!eob)
		{	continue;
		}

		while (mycur->seq < eob->seq)
		{
			if (strstr(mycur->txt, "printf")
			&& !mytype("cmnt"))
			{	if (mymatch("fprintf")
				||  mymatch("sprintf"))
				{	while (!mymatch(","))
					{	mycur_nxt();
					}
					mycur_nxt();	// 2nd arg
				} else if (mymatch("snprintf"))
				{	while (!mymatch(","))
					{	mycur_nxt();
					}
					mycur_nxt();	// 2nd arg
					while (!mymatch(","))
					{	mycur_nxt();
					}
					mycur_nxt();	// 3rd arg
				} else			// assume printf
				{	mycur_nxt();	// (
					mycur_nxt();	// 1st arg
				}
				if (mytype("ident"))
				{	Lnrs *zx = is_stored(thr[cid].params, mycur, 0);
					if (zx)
					{	if (zx->nv)
						{	if (zx->nv->mark != 1344)
							{	zx->nv->mark = 1344;
								mycur->mark = 134;
							}
						} else
						{	mycur->mark = 134;
			}	}	}	}
			if (mymatch("memcpy"))
			{	mycur_nxt();		// (
				while (!mymatch(","))
				{	mycur_nxt();
				}
				mycur_nxt();		// 2nd arg
				if (mytype("ident")
				&& is_stored(thr[cid].params, mycur, 0))
				{	mycur->mark = 1340;
					mycur->bound = mycur;
			}	}
			mycur_nxt();
	}	}
}

static void *
cwe134_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	cwe134_range(from, upto, *i);

	return NULL;
}

static void
cwe134_report(void)
{	Prim *mycur = prim;
	int w_cnt = 0;
	int at_least_one = 0;

	if (json_format && !no_display)
	{	for (; mycur; mycur = mycur->nxt)
		{	if (mycur->mark == 134
			||  mycur->mark == 1340
			||  mycur->mark == 1344)
			{	at_least_one = 1;
				printf("[\n");
				break;
	}	}	}

	for (; mycur; mycur = mycur->nxt)
	{	strcpy(json_msg, "");
		switch (mycur->mark) {
		case 134:
			if (no_display)
			{	w_cnt++;
			} else
			{	sprintf(json_msg, "untrusted format string '%s'",
					mycur->txt);
			}
			mycur->mark = 0;
			break;
		case 1340:
			if (no_display)
			{	w_cnt++;
			} else
			{	sprintf(json_msg, "source of memcpy is untrusted parameter %s",
					mycur->bound?mycur->bound->txt:"");
				// ultimate source could be a user-input
			}
			mycur->mark = 0;
			mycur->bound = 0;
			break;
		case 1344:
			mycur->mark = 0;
			break;
		default:	// 0 or 1
			// leave as is
			break;
		}
		if (strlen(json_msg) > 0)
		{	if (json_format)
			{	json_match("", "cwe_134", json_msg, mycur, 0, first_e);
				first_e = 0;
			} else
			{	printf("%s:%d: cwe_134: %s\n",
					mycur->fnm, mycur->lnr, json_msg);
	}	}	}

	if (no_display && w_cnt > 0)
	{	fprintf(stderr, "cwe_134: %d warnings: untrusted pointer argument\n", w_cnt);
	}
	if (at_least_one)
	{	printf("\n]\n");
	}
}

void
cwe134_0(void)
{
	mark_fcts();
	cwe134_init();
	run_threads(cwe134_run);
	cwe134_report();
}
