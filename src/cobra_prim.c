/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

#include "cobra_pre.h"

#define Put(a,b)	\
	{	if (b>=0 && b<NDEPTH) \
		{	Px.lex_range[a][b] = Px.lex_plst; \
		}	\
		b++;	\
	}
#define Lnk(a,b)	\
	{	if (b > 0) { b--; }	\
		if (b>=0 && b<NDEPTH && Px.lex_range[a][b]) \
		{	Px.lex_range[a][b]->jmp = Px.lex_plst; \
			Px.lex_plst->jmp = Px.lex_range[a][b]; \
	}	}

typedef struct JumpTbl JumpTbl;

struct JumpTbl {
	Prim *b;
	JumpTbl *nxt;
};

Pre	*pre;	// parsing, one per core
Prim	*plst;	// post parsing
Prim	*cur;
Prim	*prim;
int	 count;

static	JumpTbl	*jmptbl;
static	Files	*files[NHASH];
static	Files	*frr = (Files *) 0;
static	int	 nfh = -1;

extern	int	 full_comments;
extern	int	 read_stdin;
extern	int	 stream;

// utility functions for use in standalone checkers:

int
cobra_nxt(void)
{
	cur = cur?cur->nxt:NULL; return (cur != NULL);
}

int
cobra_prv(void)
{
	cur = cur?cur->prv:NULL; return (cur != NULL);
}

char *
cobra_txt(void)
{
	return (cur && cur->txt)?cur->txt:"";
}

char *
cobra_bfnm(void)		// base of filename
{	char *z = "";

	if (read_stdin)
	{	return "stdin";
	}

	if (cur && cur->fnm)
	{	z = strrchr(cur->fnm, '/');
		if (z)
		{	z++;
		} else
		{	z = cur->fnm;
	}	}

	return z;
}

char *
cobra_fnm(void)
{
	return (cur && cur->fnm)?cur->fnm:"";
}

int
cobra_lnr(void)
{
	return cur?cur->lnr:0;
}

char *
cobra_typ(void)
{
	return (cur && cur->typ)?cur->typ:"";
}

int
cobra_rewind(void)
{
	cur = prim; return (cur != NULL);
}

void
cobra_range(Prim *a, Prim *b)
{	Prim *q;

	for (q = a; q->seq <= b->seq; q = q->nxt)
	{	printf("%s ", q->txt);
	}
}

void
cobra_tag(Prim *q)
{	Prim *ocur = cur;
	cur = q;
	printf("%s:%d: ", cobra_bfnm(), cobra_lnr());
	cur = ocur;
}

// end of utility functions

static int
fhash(const char *v)
{	unsigned long h = 0x88888EEFL;
 
	while (*v != '\0')
	{	h ^= ((h << 4) ^ (h >> 28))+ *v++;
	}
	return h ^ (h>>12);
}

void
dogrep(const char *s)
{	Files *f;
	int n;
	char cmd[MAXYYTEXT];

	for (n = 0; n < NHASH; n++)
	for (f = files[n]; f; f = f->nxt)
	{	if (strlen(s)
		+   strlen(f->s)
		+   strlen("grep -n -e -q \"\"")
		+   1 >= sizeof(cmd))
		{	printf("search pattern too long\n");
			return;
		}
		snprintf(cmd, sizeof(cmd), "grep -q -n -e  \"%s\" %s",
			s, f->s);
		if (system(cmd) == 0)	// dogrep
		{	printf("%s:\n", f->s);
		}
		cmd[5] = cmd[6] = ' ';
		if (system(cmd) < 0)	// dogrep
		{	printf("cmd '%s' failed\n", cmd);
	}	}
}

char *
check_args(char *s, const char *c_base)	// single-core
{	Files *f;
	char *c;
	int m, n = strlen(s);
	char *p = strstr(s, "$COBRA");
	char *a = strstr(s, "$ARGS");
	char *q = strstr(s, "$FLAGS");

	if (!a && !p && !q)
	{	return s;
	}

//	printf("s: '%s'\nc_base: '%s'\nn=%d\np: '%s'\na: '%s'\nq: '%s'\n",
//		s, c_base, n, p?q:"-----", a?a:"=====", q?q:"_____");

	if (p)
	{	n += strlen(c_base) + strlen("/../bin") - strlen("$COBRA");
	}
	if (a)
	{	n += listfiles(0, "") - strlen("$ARGS");
		assert(a > p);
	}

	if (q)
	{	if (!no_cpp)
		{	n += strlen(" -cpp ");
		}
		if (no_display)
		{	n += strlen(" -terse ");
		}
		n += strlen(anypp());	// new 12/2019
		// should also check java/C++,python etc.
	}

	c = (char *) emalloc((n+1)*sizeof(char));	// single core
	if (p)
	{	*p = '\0';
		strncpy(c, s, n);	// upto $COBRA
		strcat(c, c_base);	// expansion of $COBRA (the rules dir)
		strcat(c, "/../bin");	// to bin dir by default
		p += strlen("$COBRA");
	} else
	{	p = s;
	}

	if (q)
	{	*q = '\0';	// flags preceed args
	}

	if (a)
	{	*a = '\0';
		assert(strlen(c)+strlen(p) < n);
		strcat(c, p);	// up to $ARGS or $FLAGS

		if (q)
		{	if (!no_cpp)
			{	char *x = anypp();
				strcat(c, " -cpp ");
				assert(strlen(c)+strlen(x) < n);
				strcat(c, x);
			}
			if (no_display)
			{	strcat(c, " -terse ");
		}	}

		if (Nfiles == 0)	// new 12/2019: no args to expand
		{	printf("error: no $ARGS for '%s'\n", s);
			return (char *) 0;
		}

		for (m = 0; m < NHASH; m++)	// bug fix: was n
		for (f = files[m]; f; f = f->nxt)
		{	strcat(c, " ");
			assert(strlen(c)+strlen(f->s) < n);
			strcat(c, f->s);
		}
		p = a + strlen("$ARGS");
	}

	assert(strlen(c)+strlen(p) < n);
	strcat(c, p);	// the rest, if any

	if (!a && q)
	{	if (!no_cpp)
		{	strcat(c, " -cpp ");
		}
		if (no_display)
		{	strcat(c, " -terse ");
	}	}

	return c;
}

int
listfiles(int verb, const char *s)
{	Files *f;
	int n, len = 0;

	if (verb && s && strlen(s) > 1)
	{	for (n = 0; n < NHASH; n++)
		for (f = files[n]; f; f = f->nxt)
		{	if (strstr(f->s, s) != NULL)
			{	printf("  %s\n", f->s);
		}	}
		return 0;
	}

	for (n = 0; n < NHASH; n++)
	for (f = files[n]; f; f = f->nxt)
	{	if (verb)
		{	printf("  %s\n", f->s);
		} else
		{	len += strlen(f->s)+1;
	}	}

	return len;
}

static void
rebind_curly(Prim *from, Prim *upto)
{	Prim *p;
	JumpTbl *j;
	int cl = 0;

	jmptbl = 0;
	for (p = from; p && p->seq <= upto->seq; p = p->nxt)
	{	p->curly = cl;
		if (strcmp(p->txt, "{") == 0)
		{	cl++;
			j = (JumpTbl *) emalloc(sizeof(JumpTbl));
			j->b = p;
			j->nxt = jmptbl;
			jmptbl = j;
		}
		if (strcmp(p->txt, "}") == 0)
		{	cl = (cl > 0)?cl-1:0;
			p->curly = cl;
			if (jmptbl)
			{	jmptbl->b->jmp = p;
				p->jmp = jmptbl->b;
				jmptbl = jmptbl->nxt;
	}	}	}
}

static int
try_fix(Prim *from, Prim *upto)
{	Prim *p, *q;
	JumpTbl *j;
	int changes_made = 0;

	for (p = from; p && p->seq <= upto->seq; p = p->nxt)
	{	if (strcmp(p->txt, "{") == 0)
		{	j = (JumpTbl *) emalloc(sizeof(JumpTbl));
			j->b = p;
			j->nxt = jmptbl;
			jmptbl = j;
		}
		if (strcmp(p->txt, "}") == 0)
		{	jmptbl = jmptbl?jmptbl->nxt:0;
		}
		if (strcmp(p->typ, "type") != 0
		&&  strcmp(p->txt, "*") != 0)
		{	continue;
		}
		q = p->nxt;
		while (strcmp(q->txt, "*") == 0)
		{	q = q->nxt;
		}
		if (strcmp(q->typ, "ident") != 0)
		{	continue;
		}
		q = q->nxt;
		if (strcmp(q->txt, "(") != 0
		||  q->jmp == NULL)
		{	continue;
		}
		q = q->jmp->nxt;
		if (strcmp(q->txt, "{") != 0)
		{	continue;
		}
		if (p->curly != 0)
		{	if (0) printf("%s:%d seems wrong: %d\n",
				p->fnm, p->lnr, p->curly);

			// stub open { ranges in jmpbl
			while (jmptbl)
			{	jmptbl->b->jmp = jmptbl->b; // avoid null
				jmptbl = jmptbl->nxt;
			}
			// reset curly count to 0
			// and  rebind later { } pairs
			rebind_curly(p, upto);
			changes_made = 1;
			break;
	}	}

	return changes_made;
}

void
fix_imbalance(void)
{	Files *f;
	int n, r;
	int cnt = 10;	// 3 passes on 15M linux files
	
	do {
		r = 0;
		for (n = 0; n < NHASH; n++)
		for (f = files[n]; f; f = f->nxt)
		{	if (f->imbalance)
			{	if (verbose)
				{	printf("%d\t%s\t%d\t%d\n",
						f->imbalance,
						f->s,
						f->first_token->seq,
						f->last_token->seq);
				}
				r += try_fix(f->first_token, f->last_token);
		}	}
		if (r && verbose && !no_display)
		{	printf("%d files fixed\n", r);
		}
	} while (r > 0 && cnt-- > 0);

	if (cnt <= 0 && r > 0 && !no_display)
	{	printf("warning: not all {} imbalances were patched\n");
		printf("         run 'fix' for another pass\n");
	}

	if (verbose && !no_display)
	{	printf("done\n");
	}
}

static Files *
seen_one(int n, const char *s, int partial)
{	Files *r;
	int i;

	assert(n >= 0 && n < NHASH);

	if (partial)	// post parsing
	{	for (r = files[n]; r; r = r->nxt)
		{	if (strstr(r->s, s) != NULL)
			{	return r;
		}	}
	} else
	{	for (i = 0; i < Ncore; i++)
		for (r = pre[i].lex_files[n]; r; r = r->nxt)
		{	if (strcmp(r->s, s) == 0)
			{	return r;
	}	}	}

	return (Files *) 0;
}

#if defined(STREAM_LIM) && (STREAM_LIM > 0)
static Prim *free_tokens;

void
recycle_token(Prim *from, Prim *stopat)
{	Prim *w, *v;

	for (w = from; w && w != stopat; w = v)
	{	v = w->nxt;
		memset(w, 0, sizeof(Prim));
		w->nxt = free_tokens;
		free_tokens = w;
	}
}
#endif

static int
new_prim(const char *s, const char *t, int cid)
{	Prim *p;

	assert(cid >= 0 && cid < Ncore);

	if (!all_headers
	&&  strstr(Px.lex_fname, HEADER) != NULL)
	{	return 0;
	}
#if defined(STREAM_LIM) && (STREAM_LIM > 0)
	if (stream == 1 && free_tokens)
	{	p = free_tokens;
		free_tokens = p->nxt;
		p->nxt = (Prim *) 0;
	} else
#endif
	p = (Prim *) hmalloc(sizeof(Prim), cid);
	p->seq     = Px.lex_count;
	p->lnr     = Px.lex_lineno;
	p->fnm     = Px.lex_fname;
	p->jmp     = 0;
	p->bound   = 0;
	p->len     = s?strlen(s):0;
	p->curly   = Px.lex_curly;
	p->round   = Px.lex_roundb;
	p->bracket = Px.lex_bracket;

	Px.lex_count++;

	if (s)
	{	p->txt = (char *) hmalloc(strlen(s)+1, cid);
		strcpy(p->txt, s);
	} else
	{	p->txt = "";
	}
	if (t)
	{	p->typ = (char *) hmalloc(strlen(t)+1, cid);
		strcpy(p->typ, t);
	} else
	{	p->typ = "";
	}

	if (Px.lex_plst)
	{	Px.lex_plst->nxt = p;
		p->prv = Px.lex_plst;
	} else
	{	Px.lex_prim = p;
	}
	Px.lex_plst = p;

	return 1;
}

void
basic_prim(const char *s, int cid)
{
	(void) new_prim(s, 0, cid);
}

static char *
nxt_field(char *s, int cid)
{	char *n;

	assert(cid >= 0 && cid < Ncore);

	n = strchr(s, '\t');
	if (!n)
	{	fprintf(stderr, "%s:%d: cnt %d: bad input '%s'\n",
			Px.lex_fname,
			Px.lex_lineno,
			Px.lex_count, s);
		return (char *) 0;
	}
	*n = '\0';
	return ++n;
}

static void
single(const char *buf, int cid)
{
	assert(cid >= 0 && cid < Ncore);

	if (!new_prim(buf, 0, cid))
	{	return;
	}
	switch (buf[0]) {
	case '{': Put(CURLY_b, Px.lex_curly);
		  break;
	case '}': Lnk(CURLY_b, Px.lex_curly);
		  if (Px.lex_plst->curly > 0)
		  {	Px.lex_plst->curly--;
		  }
		  break;
	case '(': Put(ROUND_b, Px.lex_roundb);
		  break;
	case ')': Lnk(ROUND_b, Px.lex_roundb);
		  if (Px.lex_plst->round > 0)
		  {	Px.lex_plst->round--;
		  }
		  break;
	case '[': Put(BRACKET_b, Px.lex_bracket);
		  break;
	case ']': Lnk(BRACKET_b, Px.lex_bracket);
		  if (Px.lex_plst->bracket > 0)
		  {	Px.lex_plst->bracket--;
		  }
		  break;
	default:
		  break;
	}
}

static void
triple(char *buf, int cid)
{	char *n;

	assert(cid >= 0 && cid < Ncore);

	n = nxt_field(buf, cid);
	if (n)
	{	n = nxt_field(n, cid);
		if (n)
		{	(void) new_prim(n, buf, cid);
		}
	} else
	{	printf("cannot happen? %s:%d\n",
			Px.lex_fname, Px.lex_lineno);
	}
}

#ifndef PC
static void
reset_fp(void)
{
	frr = (Files *) 0;
	nfh = -1;
}
#endif

Files *
seenbefore(const char *s, int partial)
{	Files *r = (Files *) 0;
	int n;

	if (partial)
	{	for (n = 0; n < NHASH; n++)
		{	r = seen_one(n, s, 1);
			if (r)
			{	break;
		}	}
	} else
	{	n = fhash(s)&(NHASH-1);
		r = seen_one(n, s, 0);
	}

	return r;
}

void
remember(const char *s, int imbalance, int cid)
{	Files *r;
	int n = fhash(s)&(NHASH-1);

	assert(cid >= 0 && cid < Ncore);

	for (r = Px.lex_files[n]; r; r = r->nxt)
	{	if (strcmp(r->s, s) == 0)
		{	return;
	}	}
//printf("Remember %s\n", s);
	r    = (Files *) hmalloc(sizeof(Files), cid);
	r->s =  (char *) hmalloc(strlen(s)+1, cid);
	strcpy(r->s, s);
	r->last_token = Px.lex_plst;
	r->imbalance = imbalance;

	if (Px.lex_last)
	{	r->first_token = Px.lex_last->nxt;
	} else
	{	r->first_token = Px.lex_prim;
	}
	Px.lex_last = Px.lex_plst;

	r->nxt = Px.lex_files[n];
	assert(n < NHASH);
	Px.lex_files[n] = r;
}

int
sanitycheck(int cid)
{	int imbalance = 0;	// at end of each file

	assert(cid >= 0 && cid < Ncore);

	if (Px.lex_curly != 0)
	{	imbalance |= (1<<CURLY_b);
		Px.lex_curly   = 0;
	}
	if (Px.lex_roundb != 0)
	{	imbalance |= (1<<ROUND_b);
		Px.lex_roundb  = 0;
	}
	if (Px.lex_bracket != 0)
	{	imbalance |= (1<<BRACKET_b);
		Px.lex_bracket = 0;
	}

	return imbalance;
}

void
process_line(char *buf, int cid)
{	int y;
	char z[1024], *n;

	if (Ctok)
	{	printf("%s\n", buf);
		return;
	}

	if (!buf
	||  *buf == '\n'
	||  *buf == '\0'
	||  strstr(buf, "<built-in>")
	||  strstr(buf, "<command-line>")
	||  strlen(buf) > sizeof(z))
	{	return;
	}

	assert(cid >= 0 && cid < Ncore);

	if (strncmp(buf, "line", strlen("line")) == 0)
	{	if (sscanf(buf, "line	%d	%s", &y, z) == 2)
		{	assert(Px.lex_lineno == y);
			n = strrchr((const char *) buf, (int) '\t');
			strcpy(z, n+1); // in case filename has spaces
			assert(strcmp(Px.lex_fname, z) == 0);
		}
		return;
	}

	if ((no_headers
	&&   strstr(Px.lex_fname, ".h"))
	||  (Px.lex_dontcare
	    && !verbose
	    && !no_cpp))
	{	return;
	}

	if ((n = strchr(buf, '\n')) != NULL)
	{	if (strncmp(buf, "cmnt", 4) != 0
		||  full_comments == 0)
		{	*n = '\0';
	}	}

	if (strchr(buf, '\t') == NULL)
	{	single(buf, cid);
	} else
	{	triple(buf, cid);
	}
}

void
rescan(void)
{	int n;
#ifdef PC
	// cygwin crashes on multi-core calls to gcc

	Files *f;
	extern void do_typedefs(int);

	ini_pre(0);
	for (n = 0; n < NHASH; n++)
	for (f = files[n]; f; f = f->nxt)
	{	if (!no_match)
		{	printf("rescan '%s'\n", f->s);
		}
		(void) add_file(f->s, 0, 1);
	}
	do_typedefs(0);
#else
	for (n = 0; n < Ncore; n++)
	{	ini_pre(n);
	}
	memset(pre, 0, Ncore * sizeof(Pre));
	reset_fp();
	par_scan();
	clr_files();
#endif
	post_process(1);
}

void
prep_pre(void)
{	int cid;

	if (verbose>1) printf("ini_heap()\n");
	ini_heap();
	ini_par();

	for (cid = 0; cid < Ncore; cid++)
	{	Px.lex_lineno = 1;
		Px.lex_fname = "stdin";
	}
}

void
rem_file(const char *s)
{	Files *r;

	r = (Files *) emalloc(sizeof(Files));
	r->s = (char *) s;	// comes from argv
	r->nxt = files[0];
	files[0] = r;
}

char *
get_file(void)
{
	if (!frr || !frr->nxt)
	{	while (++nfh < NHASH)
		{	frr = files[nfh];
			if (frr)
			{	return frr->s;
		}	}
		return (char *) 0;
	}
	if (frr->nxt)
	{	frr = frr->nxt;
		return frr->s;
	}
	return (char *) 0;
}

void
clr_files(void)
{	int i;

	Nfiles = 0;
	for (i = 0; i < NHASH; i++)
	{	files[i] = (Files *) 0;
	}
}

static int
already_there(int n, const char *s)
{	Files *r;

	assert(n >= 0 && n < NHASH);
	for (r = files[n]; r; r = r->nxt)
	{	if (strcmp(r->s, s) == 0)
		{	return 1;
	}	}

	return 0;
}

void
post_process(int fromscratch)
{	Files *r, *rnxt;
	int j, cid, c_imbalance = 0;

	if (fromscratch)
	{	Nfiles = 0;
	}

	for (cid = 0; cid < Ncore; cid++)
	for (j = 0; j < NHASH; j++)
	{	for (r = Px.lex_files[j]; r; r = rnxt)
		{	rnxt = r->nxt;

			if (!r->first_token
			||  !r->last_token
			||  already_there(j, r->s))
			{	if (verbose > 1)
				{	printf("--%s\n", r->s);
				}
				continue;
			}
			r->nxt = files[j];
			files[j] = r;
			Nfiles++;

			if (!no_match && r->imbalance)
			{	if (r->imbalance & (1<<CURLY_b))
				{	if (verbose && !no_display)
					{	if (Ncore > 1)
						{	printf("%d: ", cid);
						}
						printf("warning: %s unbalanced pairs: {}\n", r->s);
					}
					c_imbalance++;
				}
				if (r->imbalance & (1<<ROUND_b))
				{	if (Ncore > 1)
					{	printf("%d: ", cid);
					}
					printf("warning: %s unbalanced pairs: ()\n", r->s);
				}
				if (r->imbalance & (1<<BRACKET_b))
				{	if (Ncore > 1)
					{	printf("%d: ", cid);
					}
					printf("warning: %s unbalanced pairs: []\n", r->s);
				}
		}	}
		Px.lex_files[j] = 0;
	}
	if (verbose>1)
	{	printf("%d files\n", Nfiles);
	}
	if (c_imbalance)
	{	fix_imbalance();
	}

	if (!fromscratch)	// append from cid=0 at the end
	{	assert(prim != NULL && plst != NULL);
		assert(pre[0].lex_prim != NULL);
		plst->nxt = pre[0].lex_prim;
		plst      = pre[0].lex_plst;
		count     = pre[0].lex_count;
	} else
	{	cur = prim = pre[0].lex_prim;
		plst = pre[0].lex_plst;

		if (verbose>1) printf("connect\n");
		for (cid = 1; plst && cid < Ncore; cid++)
		{	if (!Px.lex_prim)
			{	assert(!Px.lex_plst);
				continue;
			}
			if (verbose > 1)
			{	printf("%d	%d --> %d\n",
					cid, plst->seq,
					Px.lex_prim->seq);
			}
			plst->nxt = Px.lex_prim;
			if (plst->nxt)
			{	plst->nxt->prv = plst;
			}
			plst = Px.lex_plst;
		}
	
		for (cid = count = 0; cid < Ncore; cid++)
		{	count += Px.lex_count;
			if (verbose > 1)
			{	printf("%d	%d	%d\n",
					cid, Px.lex_count, count);
		}	}
	
		if (Ncore > 1 && verbose > 1)
		{	int cnt = 0;
			printf("checking seq nrs and counts\n");
			start_timer(0);
			for (cur = prim; cur; cur = cur->nxt)
			{	if (cur->seq != cnt)
				{	printf("wrong count: %d != %d\n", cur->seq, cnt);
					break;
				}
				cnt++; 
			}
			stop_timer(0, 0, "I");
			if (cnt != count)
			{	printf("bad count %d != %d\n", cnt, count);
		}	}
		
		cur = prim;
	}

	if (verbose>1)
	{	printf("ready\n");
		memusage();
	}
}
