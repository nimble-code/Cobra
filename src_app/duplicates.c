#include "c_api.h"

#define REFLIST

// finding exact duplicates of code fragments
// and patch sequences and multiclones
// cc -o duplicates duplicates.c c.ar c_util.c -pthread

#ifndef ulong
#define ulong	unsigned long int
#endif

#define H_BITS	26	// 2^16 = 65536 slots
#define H_SIZE	(1UL<<H_BITS)
#define H_MASK	((H_SIZE)-1)


FILE *track_fd;

static int N        = 100;	// MinLength of duplicate
static int GapLimit = 25;	// MaxLength of gap in tokens
static int Detail;
static int patches;
static int multiclones;
static int dup_cnt;
static int patch_cnt;
static int showonly;

static FILE *patch_a, *patch_b;

typedef struct CRef CRef;
struct CRef {
	Prim *from;
	Prim *upto;
	int cnt;
	char *fnm;
	CRef *nxt;
} *cref;

typedef struct RefList RefList;
struct RefList {
	Prim *r;
	RefList *nxt;
};

typedef struct Fragment Fragment;
static struct Fragment {
	int nr;			// nr of matches
	ulong remainder;	// val - val%(H_SIZE)
	RefList *reflist;	// ptrs to the end of fragments
	Fragment *nxt;
} **frag;

static char **word;
static int maxnr;

static void
printseq(Prim *from, Prim *upto, char *tag)
{	int xn = -1;

	printf("%s%s:%d", tag, from->fnm, from->lnr);
	while (from->seq <= upto->seq)
	{	if (from->lnr > xn && xn >= 0)
		{	printf("\n%s%s:%d", tag, from->fnm, from->lnr);
		}
		xn = from->lnr;
		printf(" %s", from->txt);
		from = from->nxt;
	}
	printf("\n");
}

static void
extend_matches(void)
{	int cnt;
	Fragment *f;
	Prim *b, *g;
	uint64_t i;

	if (verbose)
	{	fprintf(stderr, "Step 2\n");
	}

	for (i = 0; i < H_SIZE; i++)
	{	f = frag[i];
		if (f && f->nr > 0)
		{	// all of the N tokens before each ref[?] match
			// see if we can extend the match forward
			RefList *r;
			for (r = f->reflist->nxt; r; r = r->nxt)
			{	b = f->reflist->r;
				g = r->r;
				cnt = N;
				while (strcmp(b->txt, g->txt) == 0)
				{	g = g->nxt;
					b = b->nxt;
					cnt++;
				}
				g = g->prv;
				g->bound = b->prv;
				g->mark = cnt;
	}	}	}
}

static void
double_check(Prim *a, Prim *b, int cnt)
{	int n = 0;

	while (strcmp(a->txt, b->txt) == 0)
	{	n++;
		a = a->nxt;
		b = b->nxt;
		// printf("%d '%s' == '%s'\n", n, a->txt, b->txt);
	}
	if (n < cnt)
	{	printf("warning: %d <--> %d\n", n, cnt);
		exit(1);
	}
}

static void
show_gap(Prim *t, int n, const char *p1, const char *p2)
{	int m = 1;

	printf("%s %s:%d: ", p1, t->fnm, t->lnr);
	while (n > 1 && m++ < 6)
	{	printf("%s ", t->txt);
		t = t->nxt;
		n--;
	}
	if (n > 1)
	{	printf("...");
	}
	printf("%s", p2);
}

static void
count_ref(Prim *f, Prim *u)
{	// f->fnm : f->seq - u->seq
	CRef *r, *r_prev = 0, *r_new;

	for (r = cref; r; r_prev = r, r = r->nxt)
	{	if (r->from->seq == f->seq)
		{	if (r->upto->seq == u->seq)
			{	int cmp = strcmp(r->fnm, f->fnm);
				if (cmp == 0)
				{	r->cnt++;
					return;	// done
				} else if (cmp > 0)
				{	// insert before
					break;
				}
			} else if (r->upto->seq > u->seq)
			{	// insert before
				break;
			}
		} else if (r->from->seq > f->seq)
		{	// insert before
			break;
		}
	}
	r_new = (CRef *) emalloc(sizeof(CRef), 8);
	r_new->from = f;
	r_new->upto = u;
	r_new->cnt = 1;
	r_new->fnm = f->fnm;
	if (r_prev == 0)
	{	r_new->nxt = cref;
		cref = r_new;
	} else
	{	r_new->nxt = r;
		r_prev->nxt = r_new;
	}
}

static void
report_multi(void)
{	CRef *r;
	int cnt = 0;

	for (r = cref; r; r = r->nxt)
	{	if (r->cnt >= multiclones)
		{	cnt++;
			if (showonly == 0 || showonly == cnt)
			{	printf("%s:%d-%d (has %d duplicates)", r->fnm, r->from->lnr, r->upto->lnr, r->cnt);
				if (Detail)
				{	Prim *t;
					int olnr = r->from->lnr;
					printf(":\n\t...");
					for (t = r->from; t->seq <= r->upto->seq; t = t->nxt)
					{	while (olnr < t->lnr)
						{	olnr++;
							printf("\n\t");
						}
						printf(" %s", t->txt);
				}	}
				printf("\n");
	}	}	}
	printf("found %d multiclones\n", cnt);
}

static int
overlaps(int from1, int upto1, int from2, int upto2)
{
	return (from2 <= upto1 && upto2 >= from1);
}

static void      //  q          ref            p          cur
report_out(Prim *from1, Prim *upto1, Prim *from2, Prim *upto2)
{	static int cnt = 1;
	static Prim *last_from1, *last_upto1;
	static Prim *last_from2, *last_upto2;

	dup_cnt++;
	if (!patches)
	{	if (multiclones)
		{	count_ref(from1, upto1);
			count_ref(from2, upto2);
		} else
		{	if (showonly == 0 || showonly == cnt)
			{	printf("%d match of %d tokens %s:%d-%d <-> %s:%d-%d\n",
					cnt, upto2->mark,
					from1->fnm, from1->lnr, upto1->lnr,
					from2->fnm, from2->lnr, upto2->lnr);
				if (Detail)
				{	printseq(from2, upto2, "<");
					printf("---\n");
					printseq(from1, upto1, ">");
					printf("\n");
			}	}
			cnt++;
		}
	} else
	{	if (last_from2
		&&  strcmp(from1->fnm, last_from1->fnm) == 0
		&&  strcmp(from2->fnm, last_from2->fnm) == 0
		&&  !overlaps(last_from1->seq, last_upto1->seq, from1->seq, upto1->seq)
		&&  !overlaps(last_from2->seq, last_upto2->seq, from2->seq, upto2->seq)
		&&  ((from2->seq > last_upto2->seq && from2->seq - last_upto2->seq < GapLimit)
		 ||  (upto2->seq < last_from2->seq && last_from2->seq - upto2->seq < GapLimit))
		&&  ((from1->seq > last_upto1->seq && from1->seq - last_upto1->seq < GapLimit)
		 ||  (upto1->seq < last_from1->seq && last_from1->seq - upto1->seq < GapLimit)))
		{	patch_cnt++;

			if (showonly == 0 || showonly == patch_cnt)
			{	printf("%d %s:%d-%d and %s:%d-%d differ in one location:\n",
					patch_cnt,
					from1->fnm, last_from1->lnr, upto1->lnr,
					from2->fnm, last_from2->lnr, upto2->lnr);

				show_gap(last_upto1->nxt, from1->seq - last_upto1->seq, " ", " <-> ");
				show_gap(last_upto2->nxt, from2->seq - last_upto2->seq, " ", "\n");

				if (Detail)
				{	fprintf(patch_a, "echo \"\"; echo %d %s:%d...%d\n",
						patch_cnt, from1->fnm, last_from1->lnr, upto1->lnr);
					fprintf(patch_a, "lines %s %d %d | sed 's;^[><]* *[0-9]*;;' \n",
						from1->fnm, last_from1->lnr, upto1->lnr);

					fprintf(patch_b, "echo \"\"; echo %d %s:%d...%d\n",
						patch_cnt, from2->fnm, last_from2->lnr, upto2->lnr);
					fprintf(patch_b, "lines %s %d %d | sed 's;^[><]* *[0-9]*;;' \n",
						from2->fnm, last_from2->lnr, upto2->lnr);
		}	}	}

		last_from1 = from1; last_upto1 = upto1;
		last_from2 = from2; last_upto2 = upto2;
	}

	if (verbose)
	{	double_check(from1, from2, upto2->mark);
	}
}

static void
report_matches(void)
{	int i;
	Prim *r, *p, *q;
	Prim *last_p = 0, *last_q = 0;

	if (verbose)
	{	fprintf(stderr, "Step 3\n");
	}

	cur = prim;
	while (cobra_nxt())
	{	if (cur->mark >= N)
		{	r = cur->bound;
			i = cur->mark;

			p = cur;
			q = r;

			if (last_p)
			if (overlaps(q->seq, q->seq + i, p->seq, p->seq + i)
			||  overlaps(last_p->seq, last_p->seq + N,  p->seq, p->seq + N)
			||  overlaps(last_q->seq, last_q->seq + N,  q->seq, q->seq + N))
			{	continue;
			}
			last_p = p;
			last_q = q;

			for ( ; i >= 0; i--)
			{	p = p->prv;	// move to start of fragment
				q = q->prv;	// but only upto first difference
				if (strcmp(p->txt, q->txt) != 0)
				{	p = p->nxt;
					q = q->nxt;
					cur->mark -= i;
					break;
			}	}

			if (strcmp(p->fnm, cur->fnm) != 0	// we overshot and
			||  strcmp(q->fnm, r->fnm) != 0)	// crossed a file boundary
			{	int nrt = 0;
				Prim *ptr = p;

				// check in which file the largest part of the match is
				while (strcmp(ptr->fnm, cur->fnm) != 0 && ptr != cur)
				{	ptr = ptr->nxt;
					nrt++;	// nr tokens up to file bound
				}

				ptr = cur->bound; // r
				if (nrt >= cur->mark / 2)
				{	nrt = cur->mark; // old length
					cur->mark = 0;
					while (strcmp(p->fnm, cur->fnm) != 0)
					{	cur = cur->prv;
						r = r->prv;
						nrt--;
					}
				} else // nrt < cur->mark / 2
				{	nrt = cur->mark;
					cur->mark = 0;
					while (strcmp(p->fnm, cur->fnm) != 0)
					{	p = p->nxt;
						q = q->nxt;
						nrt--;
				}	}
				if (nrt < N)
				{	continue; // now too short
				}
				cur->mark = nrt;
				cur->bound = ptr;
			}

			report_out(q, r,  p, cur);
		}
	}
	if (multiclones)
	{	report_multi();
	}
}

static void
setref(uint64_t val, Prim *r)
{	Fragment *f;
	uint64_t h, rem;

	h = val & H_MASK;
	rem = val >> H_BITS;

	for (f = frag[h]; f; f = f->nxt)
	{	if (f && f->remainder == rem)
		{	RefList *z;
#if 0
			if (f->reflist
			&&  f->reflist->r
			&&  f->reflist->r->seq == r->seq)
			{	return;
			}
#endif
			z = (RefList *) emalloc(sizeof(RefList), 9);
			z->r = r;
			z->nxt = f->reflist;
			f->reflist = z;
			f->nr++;
			if (f->nr > maxnr)
			{	maxnr = f->nr;
			}
			return;
	}	}
	f = (Fragment *) emalloc(sizeof(Fragment), 10);
	f->reflist = (RefList *) emalloc(sizeof(RefList), 11);
	f->reflist->r = r;
	f->nr = 0;	// base ref
	f->remainder = rem;
	f->nxt = frag[h];
	frag[h] = f;
}

static uint64_t
hash2_cum(const char *s, const int seq) // cumulative
{	static uint64_t h = 0;
	static char t = 0;

	if (seq == 0)
	{	t = *s++;
		h = 0x88888EEFL ^ (0x88888EEFL << 31);
	} else  // add a space as separator
	{	h ^= ((h << 7) ^ (h >> (57))) + ' ';
	}
	while (*s != '\0')
	{	h ^= ((h << 7) ^ (h >> (57))) + *s++;
	}
	return ((h << 7) ^ (h >> (57))) ^ t;
}


static uint64_t
hashword(int ix)
{	int j;
	uint64_t cumulative = 0;
 
        // cumulative hash over all N words
        // backwards from ix

   	for (j = 0; j < N; j++)
	{       cumulative = hash2_cum(word[ix], j);
		ix = (ix+N-1)%N;
	}
        return cumulative;
}

static void
handle_args(void)
{	char *s = backend;

	while (strlen(s) > 0)
	{	if (strncmp(s, "Detail", 6) == 0)
		{	Detail = 1;
			s += strlen("Detail");
			// fprintf(stderr, "Detail enabled\n");
		} else if (strncmp(s, "MinLength=", 10) == 0)
		{	s += 10;
			N = atoi(s);
			// fprintf(stderr, "MinLength = %d\n", N);
		} else if (strncmp(s, "GapLimit=", 9) == 0)
		{	s += 9;
			GapLimit = atoi(s);
			// fprintf(stderr, "GapLimit = %d\n", GapLimit);
		} else if (strncmp(s, "verbose", 7) == 0)
		{	s += 7;
			verbose++;
		} else if (strncmp(s, "Patches", 7) == 0)
		{	s += 7;
			patches++;
		} else if (strncmp(s, "MultiClones=", 12) == 0)
		{	s += 12;
			multiclones = atoi(s);	// min nr of clones
			if (multiclones <= 1)
			{	fprintf(stderr, "usage: --MultiClones=n with n>=2\n");
				exit(1);
			}
		} else if (strncmp(s, "ShowOnly=", 9) == 0)
		{	s += 9;
			showonly = atoi(s);
			Detail = 1;
		} else
		{	if (strncmp(s, "help", strlen("help")) != 0)
			{ fprintf(stderr, "duplicates: unrecognized argument '%s'\n", s);
			}
			fprintf(stderr, "usage: duplicates [--verbose] ");
			fprintf(stderr, "[--MinLength=%d] [--Detail] ", N);
			fprintf(stderr, "[--Patches] [--GapLimit=%d] ", GapLimit);
			fprintf(stderr, "[--ShowOnly=0] ");
			fprintf(stderr, "[--MultiClones=%d]\n", multiclones);
			exit(1);
		}
		while (*s != ' ' && *s != '\0')
		{	s++;
		}
		while (*s == ' ')
		{	s++;
		}
	}
	if (multiclones && patches)
	{	patches = 0;
		fprintf(stderr, "error: cannot combine --MultiClones=%d and --Patches\n", multiclones);
		exit(1);
	}
}

static void
collect_matches(void)
{	int ix = 0;
	int startup = 0;
	uint64_t val;

	if (verbose)
	{	fprintf(stderr, "Step 1\n");
	}
	cur = prim;
	while (cobra_nxt())
	{	word[ix] = cur->txt;
		ix = (ix+1)%N;
		if (startup < N)
		{	startup++;
			continue;
		}
		val = hashword(ix);
		setref(val, cur);
	}
}

static void
dup_init(void)
{
	assert(N > 0);
	handle_args();
	word = (char **) emalloc(N * sizeof(char *), 12);
	frag = (Fragment **) emalloc(sizeof(Fragment *) * H_SIZE, 13);

	if (patches && Detail)
	{	patch_a = fopen("tmp_A", "w");
		patch_b = fopen("tmp_B", "w");
		if (!patch_a || !patch_b)
		{	fprintf(stderr, "duplicates: cannot create tmp files for detailed output\n");
			exit(1);
	}	}
}

static void
dup_close(void)
{
	if (!patches)
	{	return;
	}
	fprintf(stderr, "%d duplicates found, %d are patches\n",
		dup_cnt, patch_cnt);

	if (Detail)
	{	fclose(patch_a);
		fclose(patch_b);
		if (patch_cnt > 0)
		{	if (system("sh ./tmp_A > patch_A") >= 0
			&&  system("sh ./tmp_B > patch_B") >= 0)
			{	fprintf(stderr, "wrote files: patch_A and patch_B\n");
			} else
			{	fprintf(stderr, "error writing patch_A and patch_B\n");
		}	}
		if (system("rm -f tmp_A tmp_B") < 0)
		{	fprintf(stderr, "error removing tmp files\n");
	}	}
}

void
cobra_main(void)
{
	dup_init();
	collect_matches();
	extend_matches();
	report_matches();
	dup_close();

	if (verbose)
	{	fprintf(stderr, "max nr of dups seen: %d (including overlapping with other fragments)\n", maxnr);
		// overlapping regions aren't separately reported, so the count can be misleading
	}
}
