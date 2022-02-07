#include "cwe.h"

// CWE-131: incorrect calculation of buffer size
// check for two common cases

#define skipto(s) { while (!mymatch(s)) { mycur_nxt(); } }

typedef struct ThreadLocal131 ThreadLocal131;

struct ThreadLocal131 {
	int w_cnt;
	int b_cnt;
	char cacher[4096-2*sizeof(int)];
};

static ThreadLocal131 *thr;

extern TokRange **tokrange;	// cwe_util.c

static void
cwe131_init(void)
{	static int lastN = 0;

	if (lastN < Ncore)
	{	thr = (ThreadLocal131 *) emalloc(Ncore * sizeof(ThreadLocal131));
		lastN = Ncore;
	} else
	{	memset(thr, 0, Ncore * sizeof(ThreadLocal131));
	}
}

static void
cwe131_range(Prim *from, Prim *upto, int cid)
{	Prim *mycur;
	Prim *limit = NULL, *bname;
	Prim *r, *q, *uptot, *nmm = NULL;
	int hasizeof  = 0;
	int notsimple = 0;
	int hastwoargs = 0;

	thr[cid].w_cnt = thr[cid].b_cnt = 0;

	mycur = from;
	while (mycur && mycur->seq < upto->seq && mycur_nxt())
	{	if (mycur->mark == 1)		// set in mark_fcts()
		{	skipto("{");
			limit = mycur->jmp;	// end of fct body
			if (!limit || !limit->seq)
			{	continue;
		}	}

		if (mycur->curly == 0
		||  strstr(mycur->txt, "malloc") == NULL) // look for malloc
		{	continue;
		}
		bname = mycur;		// as a default

		mycur = mycur->prv;
		if (mymatch("*"))	// likely a prototype declaration
		{	mycur_nxt();	// undo .prv
			continue;
		}
		///
		r = mycur;			// the token before malloc
		if (strcmp(r->txt, ")") == 0)	// a cast x = (...) malloc(...)
		{	r = mycur->jmp->prv;
		}
		if (strcmp(r->txt, "=") == 0)
		{	r = r->prv;		// match ident assigned
			if (strcmp(r->typ, "ident") == 0)
			{	bname = r;	// to match against later: bname = malloc
		}	}			// else it keeps default value "malloc"
		///

		mycur_nxt();	// undo .prv
		mycur_nxt();	// malloc (
		if (!mymatch("("))
		{	continue;
		}

		uptot = mycur->jmp;
		if (!uptot)
		{	continue;
		}
		// uptot points at end of malloc args

		hasizeof   = 0;
		notsimple  = 0;
		hastwoargs = 0;

		while (mycur->seq < uptot->seq)
		{
			if (mymatch("sizeof")
			||  mymatch("strlen"))	// technically strlens should also multiply by sizeof(char)
			{	hasizeof = 1;
				mycur_nxt();
				if (mymatch("("))
				{	mycur = mycur->jmp;
			}	}
			if (mymatch(","))
			{	hastwoargs++;
				break;	// should have just one arg
			}
			if (mytype("ident"))
			{	nmm = mycur;
			}
			if (mytype("oper") && !mymatch("*"))
			{	notsimple = 1;
			}
			mycur_nxt();
		}
		if (hastwoargs)
		{	continue;
		}
		if (hasizeof == 0)
		{	if (no_display)
			{	thr[cid].w_cnt++;
			} else
			{	mycur->mark = 131;
			}
			continue;
		}
	
		// next: look for the multiplier of sizeof in a ~malloc call
		//       and check for the use of that identifier standalone as an array index
	
		if (notsimple == 1)
		{	continue;
		}
	
		// nm is the identifier name to check for in the rest of the fct body
		q = mycur->nxt;		// one token after malloc (...)

		if (!limit)
		{	if (0)
			{	fprintf(stderr, "%s:%d: assertion violated: no limit\n",
					mycur->fnm, mycur->lnr);
			}
			continue;
		}

		while (q->seq < limit->seq)	// check from here to end of fct, flow insensitive
		{	if (nmm			// nmm is an ident
			&&  strcmp(q->txt, nmm->txt) == 0)
			{	r = q->nxt;
				if (strcmp(r->txt, "]") != 0)	// immediately following name
				{	q = q->nxt;
					continue;
				}
				r = q->prv;
				if (strcmp(r->txt, "[") != 0)	// immediately preceding name
				{	q = q->nxt;
					continue;
				}
				if (strcmp(bname->txt, "malloc") != 0)	// not the defaul value
				{	r = r->prv;			// point at array basename
					if (strcmp(r->txt, bname->txt) != 0)
					{	q = q->nxt;
						continue;		// not a match
				}	}
	
				// found a match
				if (no_display)
				{	thr[cid].b_cnt++;
				} else
				{	q->mark = 1310;
					q->bound = bname;
					// nmm is @ident, which means that q is @ident
					// abuse the format slightly to save the ptr
					q->jmp = nmm;
			}	}
			q = q->nxt;
		}
		// mycur now points to the place after the malloc() we just checked
	}
}

static void *
cwe131_run(void *arg)
{	int *i = (int *) arg;
	Prim *from, *upto;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	cwe131_range(from, upto, *i);

	return NULL;
}

static void
cwe131_report(void)
{
	if (no_display)
	{	int i, cnt1 = 0, cnt2 = 0;

		for (i = 0; i < Ncore; i++)
		{	cnt1 += thr[i].w_cnt;
			cnt2 += thr[i].b_cnt;
		}

		if (cnt1 > 0)
		{	fprintf(stderr, "cwe_131: %d warnings: missing sizeof() in memory allocation?\n",
				cnt1);
		}

		if (cnt2 > 0)
		{	fprintf(stderr, "cwe_131: %d warnings: potential out of bound array indexing\n",
				cnt2);
		}
	} else
	{	Prim *mycur = prim;
		int at_least_one = 0;

		if (json_format)
		{	for (; mycur; mycur = mycur->nxt)
			{	if (mycur->mark == 131
				||  mycur->mark == 1310)
				{	at_least_one = 1;
					printf("[\n");
					break;
		}	}	}

		for (; mycur; mycur = mycur->nxt)
		{	if (mycur->mark == 131)
			{	sprintf(json_msg, "missing sizeof() in memory allocation?");
				if (json_format)
				{	json_match("cwe_131", json_msg, mycur, 0);
				} else
				{	printf("%s:%d: cwe_131: %s\n",
						mycur->fnm, mycur->lnr, json_msg);
				}
				mycur->mark = 0;
			} else if (mycur->mark == 1310)
			{	Prim *q = mycur;
				Prim *b = mycur->bound;
				Prim *nmm = mycur->jmp;
				char ample[512];
				char more[1024];

				sprintf(ample, "out of bound array indexing error on %s?",
					nmm?nmm->txt:"-");

				if (json_format)
				{	if (json_plus
					&&  b
					&&  strcmp(b->txt, "malloc") != 0)
					{	strncpy(json_msg, ample, sizeof(json_msg));
						json_msg[sizeof(json_msg)-1] = '\0';
						sprintf(more, "%s<nl>array %s was allocated at %s:%d",
							json_msg,
							b->txt, b->fnm, b->lnr);
					} else
					{	strcpy(more, ample);
					}
					json_match("cwe_131", more, q, 0);
				} else
				{	printf("%s:%d:  cwe_131: %s\n",
						q->fnm, q->lnr, ample);
					if (b
					&&  strcmp(b->txt, "malloc") != 0)
					{	printf("%s:%d:  cwe_131: array %s was allocated at %s:%d\n",
							q->fnm, q->lnr, b->txt, b->fnm, b->lnr);
				}	}

				mycur->mark  = 0;
				mycur->bound = NULL;
				mycur->jmp   = NULL;
		}	}
		if (at_least_one)
		{	printf("\n]\n");
	}	}
}

void
cwe131_0(void)
{
	mark_fcts();
	cwe131_init();
	run_threads(cwe131_run);
	cwe131_report();
}
