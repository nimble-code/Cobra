/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

#include "cobra.h"
#include <regex.h>

FILE		*track_fd;
char		*b_cmd;
char		CobraDot[64];
char		FsmDot[64];
char		ShowDot[128];
char		ShowFsm[128];
char		*yytext;	// cobra_eval.y and cobra_prog.y
char		*global_t;

int		global_n;
int		inside_range;	// search qualifiers
int		inverse;
int		and_mode;
int		view_mode;
int		top_only;
int		top_up;
int		no_caller_info;	// mode of fcts

extern TokRange	**tokrange;
extern char	*C_TMP;
extern int	runtimes;
extern int	read_stdin;
extern char	*pattern(char *);
extern char	*unquoted(char *);
extern char	*progname;
extern void	clear_file_list(void);
extern void	clear_seen(void);
extern int	has_stop(void);

static FILE	*prog_fd;
static const char *C_MAIN  = "main";
static char	CobraProg[32];
static char	*ScriptArg[MaxArg];
static char	*dflt = "";
static char	ebuf[64];
static char	*global_s;
static char	*prefix;
static char	*suffix;
static char	 specialcase[MAXYYTEXT];

static FList **flst[2];
static History	*h_last;

static int	cnt;
static int	did_prep;	// support eval expressions in all commands, eg mark
static int	inscript;
static int	NrArgs;
static int	p_stop;
static int	raw;
static int	re_set[2];
static int	source;
static int	nowindow = 1;
static int	clearbounds;

static regex_t	rex[2];

static Script	*scripts;
static Script	*s_tail;
static Stack	*stack;
static Stack	*free_stack;

static int  one_line(char *);
static void back(char *, char *);
static void clear(char *, char *);
static void contains(char *, char *);
static void ctokens(void);
static void eval(char *, char *);
static void extend(char *, char *);
static void findtype(char *, char *);
static void help(char *, char *);
static void history(char *, char *);
static void jump(char *, char *);
static void list(char *, char *);
static void mark(char *, char *);
static void inspect(char *, char *);
static void next(char *, char *);
static void prep(char *, char *);
static void prog(FILE *);
static void readf(char *, char *);
static void xchange(char *);
static void show_scripts(char *, char *);
static void stretch(char *, char *);
static void undo(char *, char *);
static void with_eval(char *);

static Commands table[] = {
  { "mark",	mark,     "[q] p [p2] mark tokens matching p (optionally followed by p2)", 1 },
  { "match",	mark,     "[q] p [p2] mark tokens matching p (optionally followed by p2)", 1 },
  { "next",	next,	  "[p]      move marks forward one step, or to a token matching p",	1 },
  { "back",	back,	  "[p]      move marks back one step, or to a token matching p", 1 },
  { "view",     show_scripts, "         list names of known scripts", 2 },
  { "stretch",	stretch,  "[q] p [p2] add range from current marks upto token matching p", 1 },
  { "inspect",  inspect,  "fnm lnr  show the lexical tokens on this line",              1 },
  { "jump",	jump,	  "         move marks to end of range, if specified", 		1 },
  { "contains",	contains, "[q] p [p2]  clear marked ranges not containing token(s)",	1 },
  { "extend",	extend,   "p [p2]   select those marked items followed by p [and p2]",  1 },
  { "reset",	clear,    "         clear marks and ranges", 				1 },
  { "with",	eval,     "(expr)   same as m & (expr): select marks with expr non-zero", 1 },
  { "append",	readf,    "f.c      append f.c to data structure", 			1 },
  { "history",	history,  "         list history of commands", 				1 },
  { "list",	list,     "[n]      print a numbered list of marked tokens", 		1 },
  { "display",	display,  "[n [n2]] display all or a specific marked item from list", 	1 },
  { "pre",	prep,  	  "[n [n2]] like display, but show preprocessed code", 		1 },
  { "undo",	undo,	  "         undo effect of last operation on marks and ranges", 1 },
  { "ff",	findfunction,	  "s        find the source text for function s",       2 },
  { "ft",	findtype,	  "s        find the source text for struct s",		2 },
  { "fcg",	fcg,	  "[s] [t]  print function call graph [from fct s] [to fct t]", 3 },
  { "fcts",	fcts,	  "         print list of all functions defined",               3 },
  { "?",	help,     "[s]      print this list, or print help on command s", 	1 },
  {  0,		0,        "end of list", 0 }
};

static Qual qual[] = {
  {  "no", &inverse      },	// used in contains
  {  "ir", &inside_range },	// used in mark and unmark
  { "and", &and_mode     },	// used in mark
  {   "&", &and_mode     },	// used in mark
  { "top", &top_only     },	// used in contains and stretch
  {  "up", &top_up       },	// used in contains and stretch
  {     0, 0             }
};

static int
regstart(const int p, char *is)
{	char *s = is;
	int n;

	assert(is && p >= 0 && p < 2);
	assert(re_set[p] == 0);
	while (*s == ' ')
	{	s++;
	}
	n = strlen(s) - 1;
	while (n > 0 && s[n] == ' ')
	{	s[n] = '\0';
		n--;
	}
	n = regcomp(&rex[p], s, REG_NOSUB|REG_EXTENDED);
	if (n != 0)	// compile
	{	regerror(n, &rex[p], ebuf, sizeof(ebuf));
		printf("%s\n", ebuf);
		return 0;
	}
	re_set[p] = 1;
	return 1;
}

static int
regmark(const Prim *ref, const Prim *q, const char *s, const int p)
{
	assert(p >= 0 && p < 2);
	if (*s == '$' && *(s+1) == '$')
	{	if (ref->bound && ref->bound->seq < ref->seq)
		{	return (regexec(&rex[p], ref->bound->txt, 0,0,0) == 0);
		} else
		{	return (regexec(&rex[p], ref->txt, 0,0,0) == 0);
	}	}
	// any Script args were set earlier in regstart
	return (regexec(&rex[p], q->txt, 0,0,0) == 0);
}

static void
regstop(void)
{	int p;

	for (p = 0; p < 2; p++)
	{	if (re_set[p])
		{	regfree(&rex[p]);
			re_set[p] = 0;
	}	}
}

static int
apply(const Prim *ref, const Prim *q, const char *s, int unused)
{
	switch (*s) {
	case '\\':	// first character only (eg \/), to escape: \\@ or \\$
		s++;
		break;

	case '@':
		return (strcmp(q->typ, ++s) == 0);

	case '$':	// $$ = ref->txt
		if (*(s+1) == '$')
		{	if (ref->bound && ref->bound->seq < ref->seq)
			{	return (q != ref && strcmp(ref->bound->txt, q->txt) == 0);
			} else
			{	return (q != ref && strcmp(q->txt, ref->txt) == 0);
		}	}
		break;
	default:
		break;
	}
	return (strcmp(q->txt, s) == 0);
}

static int
r_apply(const Prim *ref, const Prim *p, char *s, const int w)
{
	if (!p)
	{	return 0;
	}
	if (*s == '(')	// eval expression
	{	// assert(did_prep != 0);
		if (did_prep == 1)
		{	return do_eval(p);
	}	}

	assert(w >= 0 && w < 2);

	return ((*s == '/' && re_set[w] && regmark(ref, p, s, w))
	||      (*s != '/' && apply(ref, p, s, w)));
}

static void
reproduce(const int seq, const char *s)
{	Prim *q = cur;
	Prim *r = cur;
	Prim *z = (cur->bound && cur->bound->seq > cur->seq)?cur->bound:cur->jmp;
	int i, n = atoi(s) + 1;
	static char src[1024];
	static char tag[1024];
	FILE *fd = track_fd ? track_fd : stdout;

	do {	// back to start of line
		cur = (cur)?cur->prv:0;
	} while (cur && cur != r && cur->lnr > q->lnr - n
	&&       strcmp(cur->fnm, q->fnm) == 0);

	if (!cur)
	{	cur = prim;
	}

	r = (Prim *) 0;
	if (cur != q)	// if we moved back
	{	cur = cur->nxt;
	}

	while (cur && cur->lnr < q->lnr + n
	&&    !p_stop
	&&     strcmp(cur->fnm, q->fnm) == 0)
	{
		if (!r || r->lnr != cur->lnr)
		{	if (r)
			{	fprintf(fd, "%s\n", src);
				if (strchr(tag, '^'))
				{	fprintf(fd, "%s\n", tag);
			}	}
			if (cobra_texpr)
			{	snprintf(src, sizeof(src), "  %3d  ", cobra_lnr());
				snprintf(tag, sizeof(tag), " ");
			} else
			{	if (scrub)
				{	snprintf(src, sizeof(src), "%s:%d: %.2s  ",
					  cobra_fnm(), cobra_lnr(),
					  (q->lnr == cur->lnr && n > 1)?"> ":"  ");
					memset(tag, ' ', strlen(src));
					tag[strlen(src)] = '\0';
				} else
				{	snprintf(src, sizeof(src), "%3d: %.2s%3d  ", seq,
					  (q->lnr == cur->lnr && n > 1)?"> ":"  ",
					  cobra_lnr());
					snprintf(tag, sizeof(tag), "%3d: ", seq);
					for (i = 5; i < strlen(src); i++)
					{	tag[i] = ' ';
					}
					tag[i] = '\0';
			}	}
			r = cur;
		}

		if (strlen(src) + strlen(cobra_txt()) + 1 < sizeof(src))
		{	strcat(src, cobra_txt());
			strcat(src, " ");
	
			for (i = 0; i < strlen(cobra_txt()); i++)
			{	if (cur->mark
				||  (cur > q && cur <= z))
				{	strcat(tag, "^");
				} else
				{	strcat(tag, " ");
			}	}
			strcat(tag, " ");
		} // else silently truncate the output

		cur = (cur)?cur->nxt:0;
	}
	fprintf(fd, "%s\n", src);
	if (strchr(tag, '^'))
	{	fprintf(fd, "%s\n", tag);
	}
	cur = q;	// undo
}

static int
same_level(const Prim *a, const Prim *b, const char *s)
{	// called as same_level(cur, q, s)

	// {     n
	//   x	n+1
	// }     n

	if (strcmp(a->txt, "{") == 0)
	{	if (strcmp(b->txt, "}") != 0)
		{	return (a->curly == b->curly-1);
		}
		return (a->curly == b->curly);
	}

	if (strcmp(a->txt, "(") == 0)
	{	if (strcmp(b->txt, ")") != 0)
		{	return (a->round == b->round-1);
		}
		return (a->round == b->round);
	}

	if (strcmp(a->txt, "[") == 0)
	{	if (strcmp(b->txt, "]") != 0)
		{	return (a->bracket == b->bracket-1);
		}
		return (a->bracket == b->bracket);
	}

	return (a->curly == b->curly);
	// because:
	// xxx {  yyy  } zzz   }
	//  n  n  n+1  n  n   n-1
}

static int
one_up(const Prim *a, const Prim *b, const char *s)
{	// called as one_up(cur, q, s)
	// 	cur is starting pt of search
	// 	q is the current location checked
	//	s is the match searched for

	if (strcmp(a->txt, "{") == 0)
	{	return (a->curly == b->curly + 1);
	}

	if (strcmp(a->txt, "(") == 0)
	{	return (a->round == b->round + 1);
	}

	if (strcmp(a->txt, "[") == 0)
	{	return (a->bracket == b->bracket + 1);
	}

	return (a->curly == b->curly + 1);
}

static void
add_history(const char *s)
{	History *h;

	if (h_last
	&&  strcmp(h_last->s, s) == 0)
	{	return;
	}
	h = (History *) emalloc(sizeof(History), 45);
	h->s = (char *) emalloc(strlen(s)+1, 46);
	strcpy(h->s, s);
	if (h_last)
	{	h_last->nxt = h;
	}
	h->prv = h_last;
	h_last = h;
}

static void
rev_print(const History *h)
{
	if (h)
	{	rev_print(h->prv);
		printf("%3d: %s\n", ++cnt, h->s);
	}
}

static char *
nextarg(char *in)
{	int i, n;

	if (*in == '\0')
	{	return in;
	}

	while (!isspace((uchar) *in) && *in != '\0')
	{	if (*in == '\\'
		&&  *(in+1) != '\0')
		{	in++;	// to escape a space
		}
		in++;
	}

	while (isspace((uchar) *in))
	{	*in++ = '\0';
	}

	for (i = 0; qual[i].s; i++)
	{	n = strlen(qual[i].s);
		assert(n < MAXYYTEXT);
		if (strncmp(in, qual[i].s, n) == 0
		&& (isspace((uchar) in[n]) || in[n] == '\0'))
		{	*qual[i].param = 1;
			return nextarg(in);
	}	}

	// other qualifiers are handled locally:
	//  '*' used in: l, d
	//  '/' used in: m, c, n, b
	//  '@' used in: apply()
	//  '$' used in: apply() and regstart()

	return in;
}

static const char *
skipwhite(const char *c)
{
	while (isspace((uchar) *c))
	{	c++;
	}
	return c;
}

static char *
trimline(char *buf)
{	char *c;

	if (!prog_fd)
	{	if (((c = strchr(buf, '#'))  != NULL
		&& c > buf && isspace((uchar) *(c-1)))
		||  ((c = strrchr(buf, '#')) != NULL
		&& c > buf && isspace((uchar) *(c-1)))
		||   (c = strchr(buf, '\n')) != NULL)
		{	do {
				*c-- = '\0';
			} while (c >= buf && isspace((uchar) *c));
	}	}
	return (char *) skipwhite(buf);
}

static void *
backup_range(void *arg)
{	Prim *q;
	TokRange *r;
	int e, n, *i = (int *) arg;

	if (verbose == 1)
	{	assert(*i >= 0 && *i < Ncore && tokrange[*i]);
		assert(tokrange[*i]->from && tokrange[*i]->upto);
		assert(tokrange[*i]->upto > tokrange[*i]->from);
	}
	n = global_n;
	r = tokrange[*i];

	if (r && r->from && r->upto)
	{	e = r->upto?r->upto->seq:plst->seq;
		for (q = r->from; q && q->seq <= e; q = q->nxt)
		{	q->mset[n] = (short) q->mark;
			q->mbnd[n] = q->bound;
	}	}

	return NULL;
}

static void
backup(int n)
{
	assert(n >= 0 && n < 4);
	if (Ncore == 1)
	{	for (cur = prim; cur; cur = cur->nxt)
		{	cur->mset[n] = (short) cur->mark;
			cur->mbnd[n] = cur->bound;
		}
	} else
	{	global_n = n;
		run_threads(backup_range, 1);
	}
}

void *
save_range(void *arg)
{	int *i = (int *) arg;
	Prim *r, *from, *upto;
	int n, local_cnt = 0;
	char *t = global_t;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;
	n    = global_n;

	if (*t == '|')		// union
	{	// add marks to n, when not already marked
		for (r = from; r && r->seq <= upto->seq; r = r->nxt)
		{	if (r->mark && !r->mset[n])
			{	r->mset[n] = (short) r->mark;
				local_cnt++;
				if (r->bound)
				{	r->mbnd[n] = r->bound;
		}	}	}
	} else if (*t == '&')	// intersection
	{	// remove marks from n that are *not* current
		for (r = from; r && r->seq <= upto->seq; r = r->nxt)
		{	if (r->mset[n])
			{	if (!r->mark)
				{	r->mset[n] = 0;
					r->mbnd[n] = (Prim *) 0;
				} else
				{	local_cnt++;
		}	}	}
	} else if (*t == '^')	// subtract
	{	// remove marks from n that *are* current
		for (r = from; r && r->seq <= upto->seq; r = r->nxt)
		{	if (r->mset[n])
			{	if (r->mark)
				{	r->mset[n] = 0;
					r->mbnd[n] = (Prim *) 0;
				} else
				{	local_cnt++;
		}	}	}
	} else
	{	// copy current marks
		for (r = from; r && r->seq <= upto->seq; r = r->nxt)
		{	r->mset[n] = (short) r->mark;
			r->mbnd[n] = r->bound;
			if (r->mark)
			{	local_cnt++;
	}	}	}

	tokrange[*i]->param = local_cnt;

	return NULL;
}

int
save(const char *s, const char *t)
{	const char *tmp;
	int n;

	if (*s)
	{	switch (*s) {
		case '&':
		case '|':
		case '^':
			tmp = t;
			t = s;
			s = tmp;
			break;
		default:
			break;
	}	}

	if (!*s || !isdigit((uchar) *s))
	{	printf("invalid command - save n or >n with n: 1..3\n");
		return 0;
	}

	n = atoi(s);
	if (n < 1 || n > 3)
	{	printf("invalid set - save 1..3\n");
		return 0;
	}
	global_t = (char *) t;
	global_n = n;
	run_threads(save_range, 14);
//	backup(n);
	return cnt;
}

void *
clear_range(void *arg)	// folds in backup(0)
{	Prim *q;
	TokRange *z;
	int *i = (int *) arg;

	z = tokrange[*i];

	if (verbose == 1)
	{	assert(*i >= 0 && *i < Ncore && z);
		assert(z->from && z->upto);
		assert(z->upto > z->from);
	}
	for (q = tokrange[*i]->from; q && q->seq <= tokrange[*i]->upto->seq; q = q->nxt)
	{	q->mset[0] = (short) q->mark;
		q->mark  = 0;

		if (clearbounds == 0)
		{	switch (q->txt[0]) {
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'g':
			case 'i':
			case 's':
				if (strcmp(q->txt, "goto") == 0
				||  strcmp(q->txt, "if") == 0
				||  strcmp(q->txt, "else") == 0
				||  strcmp(q->txt, "case") == 0
				||  strcmp(q->txt, "default") == 0
				||  strcmp(q->txt, "switch") == 0
				||  strcmp(q->txt, "break") == 0)
				{	continue;
				}
				// fall thru
			default:
				break;
		}	}

		q->mbnd[0] = q->bound;
		q->bound = 0;
	}
	return NULL;
}

static void
clear(char *unused1, char *unused2)
{
	if (strcmp(unused1, "all") == 0)
	{	clearbounds = 1;
		clear_seen();
	}
	run_threads(clear_range, 2);
	clear_matches();
	cnt = clearbounds = 0;
}

void *
restore_range(void *arg)
{	int *i = (int *) arg;
	Prim *r, *from, *upto;
	int n, local_cnt = 0;
	char *t = global_t;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;
	n    = global_n;

	if (*t == '|')		// union
	{	// add marks from n, when not already marked
		for (r = from; r && r->seq <= upto->seq; r = r->nxt)
		{	if (!r->mark && r->mset[n])
			{	r->mark = (int) r->mset[n];
				local_cnt++;
				if (r->mbnd[n])
				{	r->bound = r->mbnd[n];
		}	}	} // else keep link
	} else if (*t == '&')		// intersection
	{	// remove marks not shared with n
		for (r = from; r && r->seq <= upto->seq; r = r->nxt)
		{	if (r->mark)
			{	if (!r->mset[n])
				{	r->mark  = 0;
					r->bound = (Prim *) 0;
				} else
				{	local_cnt++;
		}	}	}
	} else if (*t == '^')			// subtract
	{	// remove marks that also appear in n
		for (r = from; r && r->seq <= upto->seq; r = r->nxt)
		{	if (r->mark)
			{	if (r->mset[n])
				{	r->mark  = 0;
					r->bound = (Prim *) 0;
				} else
				{	local_cnt++;
		}	}	}
	} else
	{	// use only marks set in n
		for (r = from; r && r->seq <= upto->seq; r = r->nxt)
		{	r->mark  = (int) r->mset[n];
			r->bound = r->mbnd[n];
			if (r->mark)
			{	local_cnt++;
	}	}	}

	tokrange[*i]->param = local_cnt;

	return NULL;
}

int
restore(const char *s, const char *t)
{	const char *tmp;
	int n;

	if (*s)
	{	switch (*s) {
		case '&':
		case '|':
		case '^':
			tmp = t;
			t = s;
			s = tmp;
			break;
		default:
			break;
	}	}

	if (!*s || !isdigit((uchar) *s))
	{	printf("invalid command - restore n or <n with n: 1..3\n");
		return 0;
	}

	n = atoi(s);
	if (n < 1 || n > 3)
	{	printf("invalid set - restore 1..3\n");
		return 0;
	}
	global_t = (char *) t;
	global_n = n;
	run_threads(restore_range, 3);
	return cnt;
}

static int
push_stack(const char *s)
{	Stack *t;
	int i;

	for (t = stack; t; t = t->nxt)
	{	if (t->nm == s)
		{	fprintf(stderr, "error: script is recursive %s\n", s);
			return 0;
	}	}
	if (free_stack)
	{	t = free_stack;
		free_stack = free_stack->nxt;
	} else
	{	t = (Stack *) emalloc(sizeof(Stack), 47);
	}
	t->nm  = s;
	t->nxt = stack;

	t->na = NrArgs;
	for (i = 0; i < NrArgs; i++)
	{	t->sa[i] = ScriptArg[i];
	}

	stack  = t;

	return 1;
}

static int
pop_stack(const char *s)
{	Stack *t = stack;
	int i;

	if (!t || t->nm != s)
	{	fprintf(stderr, "error: stack error %s - %s\n",
			s, t?t->nm:"");
		return 0;
	}
	stack = stack->nxt;

	NrArgs = t->na;
	for (i = 0; i < NrArgs; i++)
	{	ScriptArg[i] = t->sa[i];
	}

	t->nxt = free_stack;
	free_stack = t;
	return 1;
}

static void *
findtype_range(void *arg)
{	int *i = (int *) arg;
	Prim *q, *r, *from, *upto;
	char *s = global_s;
	int local_cnt = 0;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
	{	if (strcmp(r->txt, "struct") != 0
		||  strcmp(r->nxt->txt, s) != 0)
		{	continue;
		}

		for (q = r->nxt; q; q = q->nxt)
		{	if (strcmp(q->txt, ";") == 0)
			{	// didnt see { first
				break;
			}
			if (strcmp(q->txt, "{") == 0)
			{	for (r = q; r; r = r->nxt)
				{	r->mark = 1;
					if (r == q->jmp)
					{	break;
				}	}
				local_cnt++;
				break;
	}	}	}
	tokrange[*i]->param = local_cnt;

	return NULL;
}

void
run_threads(void *(*f)(void*), int which)
{	int i;

	if (Ncore == 1)
	{	i = 0;
		start_timer(0);
		(void) f((void *)&i);
		stop_timer(0, 0, "A");
		cnt = tokrange[0]->param;
	} else
	{	for (i = 0; i < Ncore; i++)
		{	start_timer(i);
			tokrange[i]->param = 0;
			(void) pthread_create(&t_id[i], 0, f,
				(void *) &(tokrange[i]->seq));
		}

		// flaw: if thread j > i finished before i,
		// then timer j is still not stopped before timer i

		for (i = cnt = 0; i < Ncore; i++)
		{	(void) pthread_join(t_id[i], 0);
			stop_timer(i, 0, "B");
			cnt += tokrange[i]->param;
	}	}
}

void
run_bup_threads(void *(*f)(void*))
{
	if (!read_stdin)
	{	if (verbose>1)
		{	printf("backup\n");
		}
		backup(0);
		if (verbose>1)
		{	printf("prog\n");
	}	}

	run_threads(f, 4);
	regstop();

	if (verbose>1)
	{	printf("---\n");
	}
}

static void
findtype(char *s, char *t)
{
	global_s = s;
	global_t = t;
	run_bup_threads(findtype_range);
}

void
fcts(char *unused1, char *unused2)
{	FList *f;
	Prim *z;
	static int o_caller_info = 0;

	assert(no_cpp == 0 || no_cpp == 1);
	flist = flst[no_cpp];

	if (unused1
	&&  strcmp(unused1, "0") == 0)
	{	no_caller_info = 1;
	}

	if (!flist					// not yet created, or
	|| (o_caller_info == 1 && no_caller_info == 0))	// need more info
	{	o_caller_info = no_caller_info;
		fct_defs();
		if (!flist)
		{	return;
		}
		flst[no_cpp] = flist;
	}

	// the lists were merged at this point
	for (f = flist[0]; f; f = f->nxt)
	{	cnt++;
		if (verbose > 1)
		{	printf("%3d %s:%d-%d: %s\n",
				cnt,
				f->p->fnm,
				f->p->lnr,
				(f->q && f->q->jmp)?f->q->jmp->lnr:f->p->lnr,
				f->detail);
		}
		z = f->p;	// fct name
		if (z->jmp)	// c++ fcts?
		{	z = z->jmp;
			if (z->prv)
			{	z->prv->mark++;
			} else
			{	z->mark++;
			}
		} else
		{	z->mark++;
	}	}
}

static void
browse(char *s, char *t)
{	static Files *f = (Files *) 0;
	static int floc = 1;
	static char *lastcall = (char *) 0;
	int n = 1;

	if (isdigit((uchar) *t))
	{	n = atoi(t);
	} else if (isdigit((uchar) *s))
	{	n = atoi(s);
		s = t;
	}

	if (strlen(s) > 0)
	{	f = seenbefore(s, 1);
		floc = n;
	} else if (n > 1)
	{	floc = n;
	} else
	{	floc += 21;
	}

	if (!f)
	{	printf("no such file '%s'\n", s);
		return;
	}
	cur = f->first_token;	// not necessary that file, could be a header

	if (strlen(f->s) > 0
	&& (!lastcall || strcmp(lastcall, f->s) != 0))
	{	lastcall = (char *) emalloc(strlen(f->s)+1, 48);
		strcpy(lastcall, f->s);
		s = f->s;
	} else if (strlen(s) == 0 || strlen(f->s) == 0)
	{	s = lastcall;
	}

	show_line(stdout, s, 0, floc, floc+20, 1);	
}

static ArgList *
parse_args(const char *s)	// comma separated list val nm, str nm
{	ArgList *a = NULL, *t, *last=0;
	int i;
	char b[1024];

	s = skipwhite(s);
	while (strlen(s) > 0)
	{	t = (ArgList *) emalloc(sizeof(ArgList), 49);
		for (i = 0; (isalnum((uchar) *s) || *s == '_') && i < sizeof(b)-1; i++)
		{	b[i] = *s++;
		}
		b[i] = '\0';
		t->nm = emalloc(strlen(b) + 1, 50);
		strcpy(t->nm, b);

		if (!a)
		{	a = t;
		} else
		{	last->nxt = t;
		}
		last = t;

		s = skipwhite(s);
		if (*s != '\0')
		{	if (*s != ',')
			{	fprintf(stderr, "error: expecting ',' saw: '%c'\n", *s);
				break;
			}
			s = skipwhite(s+1);
	}	}
	
	return a;
}

static void
show_scripts(char *unused1, char *unused2)
{	Script *s;
	ArgList *a;
	int n;

	for (s = scripts; s; s = s->nxt)
	{	printf(" %s", s->nm);
		n = 0;
		for (a = s->arg; a; a = a->nxt)
		{	if (n++ > 0)
			{	printf(", ");
			} else
			{	printf("(");
			}
			printf("%s", a->nm);
		}
		printf("%s", (n>0)?")\n":"\n");
	}
}

static int
one_script(FILE *fd, const char *nm, char *buf, const int sz)
{	Script *s;
	Cmd    *x, *last = 0;
	char   *c;

	c = strchr(nm, '(');
	if (c != NULL)
	{	*c = '\0';	// temporary
	}

	for (s = scripts; s; s = s->nxt)
	{	if (strcmp(s->nm, nm) == 0)
		{	break;
	}	}

	if (!s)
	{	s = (Script *) emalloc(sizeof(Script), 51);
		if (!scripts)
		{	scripts = s_tail = s;
		} else
		{	s_tail->nxt = s;
			s_tail = s;
	}	}
	s->nm = (char *) emalloc(strlen(nm)+1, 52);
	strcpy(s->nm, nm);
	if (c != NULL)
	{	*c = '(';	// undo
	}

	c = strchr(buf, '#');
	if (c != NULL && (c == buf || *(c-1) != '\\'))
	{	*c = '\0';	// strip comment
	}
	c = strchr(buf, '(');
	if (c != NULL)		// parameter list
	{	char *d;
		d = strchr(c, ')');
		if (d == NULL)
		{	fprintf(stderr, "error: missing ')' in '%s'\n", buf);
		} else
		{	*d = '\0';
			if (d > c+1)	// non-empty list
			{	s->arg = parse_args(c+1);
			}
		//	c = ++d;	// after param list, not used later
	}	}

	while (fgets(buf, sz, fd))
	{	if (strncmp(buf, "def", 3) == 0)
		{	return 1; // missing 'end'
		}
		if (strncmp(buf, "end", 3) == 0)
		{	strcpy(buf, "");
			return 1;
		}
		c = buf;	// dont call trimline here
		x = (Cmd *) emalloc(sizeof(Cmd), 53);
		x->cmd = (char *) emalloc(strlen(c)+1, 54);
		strcpy(x->cmd, c);
		x->work = (char *) emalloc(strlen(c)+1, 55);
		strcpy(x->work, "");
		if (last)
		{	last->nxt = x;
			last = x;
		} else
		{	s->txt = x;
			last = x;
	}	}

	return 0;
}

static void
check_list(ArgList *lst, char *c, char *s, int len)
{	ArgList *n;

	while (strlen(c) > 0)
	{	for (n = lst; n; n = n->nxt)
		{	if (strncmp(c, n->nm, strlen(n->nm)) == 0
			&&  !isalnum((uchar) c[strlen(n->nm)])
			&&  c[strlen(n->nm)] != '_')
			{	if (n->s && strlen(n->s) > 0)
				{	snprintf(s, len, "%s", n->s);
					s += strlen(n->s);
					len -= strlen(n->s);
				}
				c += strlen(n->nm);
				break;
		}	}
		if (!n)		// no matches
		{	do {	// move forward
				*s++ = *c++;
			} while (isalnum((uchar) *(c-1)) || *(c-1) == '_');
	}	}
	*s = '\0';
}

static int
cnt_delta(ArgList *lst, const char *c, int *len)
{	ArgList *n;
	const char *s, *t;
	int hit = 0;

	for (n = lst; n; n = n->nxt)	// for each actual param value
	{	s = c;			// does it appear in the text?
		while ((t = strstr(s, n->nm)) != NULL)
		{	if (!n->s)
			{	*len += 2 - strlen(n->nm); // missing param
			} else
			{	*len += strlen(n->s) - strlen(n->nm);
			}
			s = t + strlen(n->nm);
			hit++;
	}	}

	return hit;
}

static char *
replace_args(char *c, ArgList *a)	// in def-macro
{	char *s, *t;
	int len = strlen(c)+1;

	if (cnt_delta(cl_var, c, &len) +
	    cnt_delta(a,      c, &len) == 0)
	{	s = (char *) emalloc(len * sizeof(char), 56);
		strcpy(s, c);
		return s;
	}

	assert(len > 0);
	s = t = (char *) emalloc(len * sizeof(char), 57);

	if (cl_var != NULL)	// command-line params, if any
	{	check_list(cl_var, c, s, len);
		c = t;	// take result as the new input
		s = t = (char *) emalloc(len * sizeof(char), 58);
	}

	check_list(a, c, s, len);	// replace actual params

	return t;
}

static void
do_script(char *fnd, char *a, char *b)
{	Script	*s;
	ArgList *x;
	Cmd	*c;
	char	*n;
	char	*f = (char *) skipwhite(fnd);
	char	*z;
	int	i;

	if (!scripts)
	{	printf("no such command: '%s' (no scripts defined)\n", f);
		return;
	}

	i = strlen(fnd)+strlen(a)+strlen(b)+3;
	z = (char *) emalloc(i*sizeof(char), 59);

	snprintf(z, i, "%s %s %s", f, a, b);

	if ((n = strchr(z, '(')) != NULL)	// parameters
	{	*n++ = ' ';	// separate args with white-space
		while ((f = strchr(n, ',')) != NULL)
		{	*f++ = ' ';
			n = f;
		}
		if ((n = strchr(n, ')')) != NULL)
		{	*n = '\0';
	}	}
	f = z;

	if (!push_stack(f))
	{	return;
	}

	n = f;
	for (NrArgs = 0; NrArgs < MaxArg; NrArgs++)
	{	ScriptArg[NrArgs] = nextarg(n);
		if (strlen(ScriptArg[NrArgs]) == 0)
		{	break;
		}
		n = ScriptArg[NrArgs];
	}
	n = nextarg(n);

	if (NrArgs >= MaxArg && strlen(n) > 0)
	{	fprintf(stderr, "error: max nr of script args is %d (ignoring: '%s')\n",
			MaxArg, n);
	}

	inscript++;
	for (s = scripts; s; s = s->nxt)
	{	if (strcmp(s->nm, f) == 0)	// found script
		{	if (verbose == 1)
			{	printf(":%s\n", s->nm);
			}
			for (i = 0, x = s->arg; x; i++) // count formal params
			{	x = x->nxt;
			}
			if (i != NrArgs)		// was <
			{	fprintf(stderr, "error: '%s' takes %d arguments, saw %d\n",
					f, i, NrArgs);
				break;
			}
			for (i = 0, x = s->arg; i < NrArgs; i++)
			{	x->s = ScriptArg[i];
				x = x->nxt;
			}
			for ( ; x; x = x->nxt)
			{	x->s = "";
			}
			for (c = s->txt; c; c = c->nxt)
			{	if (!no_match)
				{	printf("\t%s", c->cmd);
				}
				// c->work was already allocated though
				c->work = replace_args(c->cmd, s->arg);
				(void) one_line(trimline(c->work)); // modifies arg
			}
			break;
	}	}
	inscript--;

	if (!pop_stack(f))
	{	return;
	}

	if (!s)
	{	printf("no such command: '%s'\n", f);
	}
}

static int
try_read(const char *s)
{	FILE *fd;
	char *c, buf[1024];
	char a[1024], b[1024];
	int n;

	if ((fd = fopen(s, "r")) == NULL)
	{	return 0;
	}
	if (strchr(s, '/') && !no_match && verbose)
	{	printf("reading: %s\n", s);
	}

	while (fgets(buf, sizeof(buf), fd))
	{
	nxt:	c = trimline(buf);
		if (strlen(c) == 0)
		{	continue;
		} else if (strncmp(c, "quiet on", strlen("quiet on")) == 0)
		{	no_match = 1;
		} else if (strncmp(c, "quiet off", strlen("quiet off")) == 0)
		{	no_match = 0;
		} else if (strncmp(c, "def", 3) == 0)
		{	c = (char *) skipwhite(c+3);
			n = sscanf(c, "%s %s", a, b);
			if (n <= 0 || n > 2)
			{	fprintf(stderr, "error: bad format for 'def:' '%s'\n", buf);
				strcpy(a, "none");
			}
			if (!no_match && n > 0)
			{	printf("script '%s'", a);
				if (n == 2 && strstr(b, "-n") && !no_cpp)
				{	printf("\trequires -n");
				}
				printf("\n");
			}
			if (one_script(fd, a, buf, sizeof(buf)))
			{	goto nxt; // keep this line, could be 'def'
			}
		} else
		{	if (!one_line(c))	// immediate command
			{	fflush(stdout);
				break;
	}	}	}
	fclose(fd);

	return 1;
}

static void
read_scripts(void)
{	char *buf;
	int n;

	if (!cobra_target)
	{	return;
	}
	if (try_read(cobra_target))
	{	return;
	}

	n = strlen(C_BASE) + 1 +
		strlen(cobra_target) + 1 +
		strlen(C_MAIN) +
		strlen(".cobra") + 1;

	buf = (char *) emalloc(n*sizeof(char), 60);

	// check cobra/rules directory
	snprintf(buf, n, "%s/%s", C_BASE, cobra_target);

	if (strstr(cobra_target, ".cobra") == NULL
	&&  strstr(cobra_target, ".def") == NULL)
	{	strcat(buf, ".cobra");
	}

	if (!try_read(buf))
	{	// check cobra/rules/main directory
		snprintf(buf, n, "%s/%s/%s",
			C_BASE, C_MAIN, cobra_target);
		if (strstr(cobra_target, ".cobra") == NULL
		&&  strstr(cobra_target, ".def") == NULL)
		{	strcat(buf, ".cobra");
		}
		if (!try_read(buf))
		{	printf("cobra: cannot find '%s'\n", cobra_target);
	}	}
	free(buf);	
}

static void
addscriptfile(char *bc)
{	char *oprim = cobra_target;

	cobra_target = nextarg(bc);
	read_scripts();
	cobra_target = oprim;
}

static void
set_default(char *d)
{
	if (strlen(d) < MAXYYTEXT)
	{	dflt = (char *) emalloc(strlen(d)+1, 61);
		strcpy(dflt, d);
	} else
	{	printf("default command too long: %s\n", d);
	}
}

static int
prep_print(char *s)
{	char *t;

	while (*s && !isspace((uchar) *s))
	{	s++;	// allow printf iso print
	}

	s = (char *) skipwhite(s);

	if (*s == '"')
	{	t = ++s;
		while (*t)
		{	if (*t == '\\')
			{	*t = ' ';
			}
			if (*t++ == '"')
			{	break;
		}	}
		if (*(t-1) != '"')
		{	fprintf(stderr, "error: missing closing double-quote\n");
			return 0;
		}
		*(t-1) = '\0';
		prefix = s;
		suffix = t;
	} else
	{	prefix = t = s;
		while (*t && !isspace((uchar) *t))
		{	t++;
		}
		if (isspace((uchar) *t))
		{	*t++ = '\0';
		}
		suffix = (char *) skipwhite(t);
	}

	return 1;
}

typedef struct Map Map;
struct Map {
	char *txt;
	char *typ;
	Map  *nxt;
};

static Map *remap[256];

static void *
map_range(void *arg)
{	int *i = (int *) arg;
	Prim *r, *from, *upto;
	int h, local_cnt = 0;
	Map *m;
	char *ptr;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
	{	h = 0;
		ptr = r->txt;
		while (*ptr != '\0')
		{	h += (int) *ptr++;
		}
		for (m = remap[h&255]; m; m = m->nxt)
		{	if (strcmp(m->txt, r->txt) == 0)
			{	r->typ = m->typ;
				local_cnt++;
	}	}	}

	tokrange[*i]->param = local_cnt;
	return NULL;
}

static void
load_map(char *s)
{	FILE *fd;
	char buf[1024];
	char a[1024], b[1024], *ptr;
	int h;

	if ((fd = fopen(s, "r")) == NULL)
	{	if (strlen(s) + strlen(C_BASE) + 2 < sizeof(a))
		{	snprintf(a, sizeof(a), "%s/%s", C_BASE, s);
			fd = fopen(a, "r");
		}
		if (fd == NULL)
		{	fprintf(stderr, "error: no such file '%s'\n", s);
			return;
	}	}
	// every line has two fields: a pattern and a type
	while (fgets(buf, sizeof(buf), fd))
	{	if (sscanf(buf, "%s %s", a, b) != 2)
		{	fprintf(stderr, "error: bad map input '%s'\n", buf);
			break;
		}
		h = 0;
		ptr = a;
		while (*ptr != '\0')
		{	h += (int) *ptr++;
		}
		Map *m = (Map *) emalloc(sizeof(Map), 62);
		m->txt = (char *) emalloc(strlen(a)+1, 63);
		strcpy(m->txt, a);
		m->typ = (char *) emalloc(strlen(b)+1, 64);
		strcpy(m->typ, b);
		m->nxt = remap[h&255];
		remap[h&255] = m;
	}
	fclose(fd);
	run_threads(map_range, 5);
}

static int
starts_with_qualifier(const char *s)
{	int i, n;

	for (i = 0; s && qual[i].s; i++)
	{	n = strlen(qual[i].s);
		if (strncmp(s, qual[i].s, n) == 0
		&&  isspace((uchar) s[n]))
		{	return 1;
	}	}
	return 0;
}

static char *
check_qualifiers(char *s)
{	int i, n;

	if (!s || *s == '\0')
	{	return s;
	}

	s = (char *) skipwhite(s);
	if (*s == '\\')
	{	if (starts_with_qualifier(s+1)) // dont interpret qualifier
		{	return (s+1);		// if preceded by backslash
		} else
		{	return s;
	}	}

	for (i = 0; s && qual[i].s; i++)
	{	n = strlen(qual[i].s);
		if (strncmp(s, qual[i].s, n) == 0
		&&  isspace((uchar) s[n]))
		{	*qual[i].param = 1;
			s = check_qualifiers(s+n);
	}	}

	if (s && (inside_range || top_only || top_up))
	{	fprintf(stderr, "error: no support for ir or top in this context\n");
		return NULL;
	}
	if (s && and_mode && inverse)
	{	fprintf(stderr, "error: cannot combine & and no\n");
		return NULL;
	}

	return s;
}

static void
popup_window(const char *fnm, const int lnr)
{	char buf[1024];
	static int warned = 0;

	if ((strlen("window.tcl")
	   + strlen(fnm) + 8 + 6) < sizeof(buf))
	{	snprintf(buf, sizeof(buf), "window.tcl %s %d &",
			fnm, lnr);
		if (system(buf) != 0 && !warned)
		{	printf("command '%s' failed\n", buf);
			warned++;
		}
	} else
	{	printf("filename '%s' too long\n", fnm);
	}
}

static void
set_ncore(int n)
{
	if (n < 1)
	{	printf("ncore cannot be %d\n", n);
		return;
	}
	if (n == Ncore)
	{	ctokens();
		return;
	}

	Ncore = n;
	ctokens(); // calls set_ranges
	ini_lock();
	ini_heap();
	ini_timers();
}

static int
pre_scan(char *bc)	// non-generic commands
{	char *a, *b;

	if (prog_fd)
	{	if ((a = strstr(bc, "%}")) != NULL)
		{	*a = ' ';
			fprintf(prog_fd, "%s\n", bc);
	L:		fclose(prog_fd);
			if ((prog_fd = fopen(CobraProg, "r")) == NULL)
			{	printf("cannot open %s\n", CobraProg);
				return 1;
			}
			prog(prog_fd);
			fclose(prog_fd);
			prog_fd = NULL;
			if (preserve)
			{	printf("wrote: %s\n", CobraProg);
			}
		} else
		{	fprintf(prog_fd, "%s\n", bc);
		}
		return 1;
	}

	switch (*bc) {
	case '.':
		addscriptfile(++bc);
		return 1;

	case ':':	// execute a script
		do_script(++bc, "", "");
		return 1;

	case '!': // shell escape
		{	char *c = check_args(bc+1, C_BASE);
			if (!c || system(c) < 0)	// shell escape
			{	printf("error\n");
		}	}
		return 1;

	case '%':	// possible inline program
		if (*(bc+1) != '{')
		{	break;
		}
		if ((prog_fd = fopen(CobraProg, "w")) == NULL)
		{	printf("cannot create %s\n", CobraProg);
			return 1;
		}
		a = strstr(bc+1, "%}");
		if (a != NULL)
		{	*a = '\0';
			fprintf(prog_fd, "%s\n}\n", bc+1);
			goto L;
		} else
		{	fprintf(prog_fd, "%s\n", bc+1);
		}
		return 1;

	case 's':	// save / setlinks
		if (strncmp(bc, "save", strlen("save")) == 0)
		{	bc += strlen("save") - 1;
			while (isspace((uchar) *bc))
			{	bc++;
			}
			bc--;
		} else if (strncmp(bc, "setlinks", strlen("setlinks")) == 0)
		{	set_links();	// cobra_fcg.c
			return 1;
		} else if (strncmp(bc, "stream", 6) == 0)
		{	char *qtr;

			if (!read_stdin)
			{	printf("stream command ignored in non-streaming mode\n");
				return 1;
			}
			if ((qtr = strstr(bc, "mode=")) != NULL)
			{	qtr += 5;
				if (strncmp(qtr, "text", 4) != 0)
				{	printf("mode: unrecognized setting\n");
				} else
				{	extern void set_textmode(void);
					set_textmode();
			}	}
			if ((qtr = strstr(bc, "limit=")) != NULL)
			{	qtr += 6;
				stream_lim = atoi(qtr);
				printf("stream limit: %d lines\n", stream_lim);
			}
			if ((qtr = strstr(bc, "margin=")) != NULL)
			{	qtr += 7;
				stream_margin = atoi(qtr);
				printf("stream margin: %d lines\n", stream_margin);
			}
			return 1;
		} else
		{	break;
		}
			// only save falls thru

	case '>':	// save
		bc++;
		switch (*bc) {
		case '*':
		case '&': b = "&"; bc++; break;
		case '+':
		case '|': b = "|"; bc++; break;
		case '-':
		case '^': b = "^"; bc++; break;
		case '=': bc++;	// fall thru
		default:  b = "="; break;
		}
		if (isdigit((uchar) *bc))	// >1
		{	a = bc;
		} else
		{	a = nextarg(bc);
		}
		(void) save(a, b);
		return 1;

	case 't':	// terse on/off
		if (strncmp(bc, "terse", 5) == 0)
		{	if (strstr(&bc[5], "on"))
			{	no_display = 1;
			} else if (strstr(&bc[5], "off"))
			{	no_display = 0;
			} else
			{	no_display = 1 - no_display;
			}
			return 1;
		}

		// t[rack] start/stop -- write output of [dlp] to a file

		if (!isspace((uchar) *(bc+1))
		&&  strncmp(bc, "track", strlen("track")) != 0)
		{	break;
		}
		if (strstr(bc, "stop") != NULL)
		{	if (track_fd != NULL)
			{	fflush(track_fd);
				fclose(track_fd);
				track_fd = NULL;
			}
			return 1;
		}
		a = strstr(bc, "start");
		if (!a)
		{	printf("usage: t[rack] start filename\n");
			printf("   or: t[rack] stop\n");
			return 1;
		}
		a = (char *) skipwhite(a+strlen("start"));
		if (strlen(a) == 0)
		{	printf("usage: t[rack] start filename\n");
			return 1;
		}
		if ((b = strchr(a, ' ')) != NULL)
		{	*b = '\0';
		}
		if ((b = strchr(a, '\t')) != NULL)
		{	*b = '\0';
		}
		if (track_fd != NULL)
		{	fclose(track_fd);
		}
		if ((track_fd = fopen(a, "r")) != NULL)
		{	fclose(track_fd);
			track_fd = NULL;
			fprintf(stderr, "warning: '%s' exists\n", a);
			fprintf(stderr, "first do:  !rm %s\n", a);
			return 1;
		}
		if ((track_fd = fopen(a, "w")) == NULL)
		{	fprintf(stderr, "cannot create '%s'\n", a);
		}
		return 1;

	case 'p':
		if (strncmp(bc, "pat", 3) == 0)
		{	// printf("pattern expression: '%s'\n", bc+3);
			a = check_qualifiers(bc+3);
			cobra_te(pattern(a), and_mode, inverse);
			return 1;
		}
		if (strncmp(bc, "pe",  2) == 0)
		{	// printf("pattern expression: '%s'\n", bc+2);
			a = check_qualifiers(bc+2);
			cobra_te(pattern(a), and_mode, inverse);
			return 1;
		}
		break;

	case 'r':	// runtimes on off,  restore - reset - regular/token expression
		if (strncmp(bc, "runtimes", 8) == 0)
		{	if (strstr(&bc[5], "on"))
			{	runtimes = 1;
			} else if (strstr(&bc[5], "off"))
			{	runtimes = 0;
			} else
			{	runtimes = 1 - runtimes;
			}
			return 1;
		} else if (strncmp(bc, "restore", strlen("restore")) == 0)
		{	bc += strlen("restore");
			while (isspace((uchar) *bc))
			{	bc++;
			}
			bc--;
			// fall thru
		} else if (strncmp(bc, "re ", 3) == 0)
		{	// printf("token expression: '%s'\n", bc+3);
			a = check_qualifiers(bc+3);
			cobra_te(unquoted(a), and_mode, inverse);
			return 1;
		} else
		{	break;
		}
		// fall thru

	case '<':	// restore
		bc++;
		switch (*bc) {
		case '*':
		case '&': b = "&"; bc++; break;
		case '+':
		case '|': b = "|"; bc++; break;
		case '-':
		case '^': b = "^"; bc++; break;
		case '=': bc++;		// fall thru
		default:  b = "="; break;
		}
		if (isdigit((uchar) *bc))	// <1
		{	a = bc;
		} else
		{	a = nextarg(bc);
		}
		(void) restore(a, b);
		if (!inscript && !no_match)
		{	printf("%d match%s\n", cnt, (cnt==1)?"":"es"); // restore
		}
		return 1;

	case 'B':	// browse file
		a = nextarg(bc);
		b = nextarg(a);
		browse(a, b);
		return 1;

	case 'V':	// view file with window.tcl
		a = nextarg(bc);
		b = nextarg(a);
		popup_window(a, atoi(b));
		return 1;

	case 'F':
		(void) listfiles(1, nextarg(bc));
		return 1;

	case 'G':
		dogrep(nextarg(bc));
		return 1;

	case 'h':
		if (strncmp(bc, "help", strlen("help")) == 0)
		{	char *p = strchr(bc, ' ');
			if (p)
			{	help(++p, "");
			} else
			{	help("", "");
			}
			return 1;
		}
		break;

	case 'c':	// context - function context
		if (strncmp(bc, "context", strlen("context")) == 0)
		{	context(nextarg(bc), "");
			return 1;
		}
		if (strncmp(bc, "cfg", 3) == 0
		&&  is_blank((uchar) bc[3]))
		{	cfg(nextarg(bc), "");
			return 1;
		}
		if (strncmp(bc, "cpp", 3) == 0
		&&  is_blank((uchar) bc[3]))
		{	xchange(nextarg(bc));
			return 1;
		}
		break;

	case 'd':	// default action on empty command
		if (strncmp(bc, "declarations", strlen("declarations")) == 0)
		{	extern void declarations(void);
			declarations();
			return 1;
		}
		if (strncmp(bc, "default", strlen("default")) == 0)
		{	set_default(nextarg(bc));
			return 1;
		}
		if (strncmp(bc, "dp", 2) == 0)
		{	display_patterns(nextarg(bc));
			return 1;
		}
		// else it maps to 'display'
		break;

	case 'f':
		if (strncmp(bc, "fix", strlen("fix")) == 0)
		{	fix_imbalance();
			return 1;
		}
		break;

	case 'l':
		if (strncmp(bc, "lib", strlen("lib")) == 0)
		{	list_checkers();
			return 1;
		}
		break;

	case 'm':
		if (strncmp(bc, "map", strlen("map")) == 0
		&&  is_blank((uchar) bc[3]))
		{	load_map(nextarg(bc));
			if (!inscript && !no_match)
			{	printf("%d match%s\n", cnt, (cnt==1)?"":"es");	// map
			}
			return 1;
		}
		break;

	case 'n':
		if (strncmp(bc, "ncore", strlen("ncore")) == 0)
		{	char *s = nextarg(bc);
			if (isdigit((uchar) *s))
			{	set_ncore(atoi(s));
				return 1;
		}	}
		if (strncmp(bc, "nowindow", strlen("nowindow")) == 0)
		{	nowindow = 1;
			return 1;
		}
		break;

	case '=':
		if (prep_print(bc))	// extract prefix and suffix
		{	view_mode = 1;
			if (suffix && strlen(suffix) > 0)
			{	backup(0);
				with_eval(suffix);	// = "string" $0
				if (cnt < 0)
				{	printf("%s %d\n",
						prefix, -cnt-1);
				}
				undo("", "");
			} else if (prefix && *prefix == '(')
			{	suffix = prefix;	// = (.range)
				prefix = "value";
				backup(0);
				with_eval(suffix);	// =
				if (cnt < 0)
				{	printf("%s %d\n",
						prefix, -cnt-1);
				}
				undo("", "");
			} else
			{	(void) nr_marks(0);	// = "string:" or =
				if (!no_match || cnt > 0)
				{	if (scrub)
					{	scrub_caption = emalloc(strlen(prefix)+1, 65);
						strcpy(scrub_caption, prefix);
					} else if (!track_fd)
					{	printf("%s %d\n",	// =
							prefix, cnt);
					} else
					{	fprintf(track_fd, "%s %d\n",	// =
							prefix, cnt);
			}	}	}
			view_mode = 0;
		}
		return 1;

	case 'e':	// eval
	case 'w':	// with
		if (strncmp(bc, "window", strlen("window")) == 0)
		{	nowindow = 0;
			return 1;
		}
		if (strncmp(bc, "w", 1) == 0 // w[ith]
		||  strncmp(bc, "eval", strlen("eval")) == 0)
		{	// an expression, e.g. .len >= 32
			while (*bc && !isspace((uchar) *bc))
			{	bc++;	// rest of cmd
			}

			and_mode = 1;
			with_eval(bc);

			if (cnt < 0)
			{	if (!no_match)
				{	printf("value: %d\n", -cnt-1);
				}
			} else if (!inscript && !no_match)
			{	printf("%d match%s\n", cnt, (cnt==1)?"":"es");	// w, eval
			}
			return 1;
		}
		break;

	case 'u':	// unmark - alternative to 'mark no'
		if (strncmp(bc, "unmark", strlen("unmark")) == 0)
		{	a = nextarg(bc);
			b = nextarg(a);
			inverse = 1;
			mark(a, b);
			inverse = 0;
			return 1;	// command handled
		}	// else: undo
		break;

	case 'q':	// quit or quiet on/off
		if (strncmp(bc, "quiet", strlen("quiet")) == 0)
		{	if (strstr(&bc[5], "on"))
			{	no_match = 1;
			} else if (strstr(&bc[5], "off"))
			{	no_match = 0;
			} else
			{	no_match = 1 - no_match;
			}
			return 1;
		}
		if (strcmp(bc, "quit") == 0
		||  strcmp(bc, "q") == 0)
		{	return 0;		// session ends
		}
		break;

	case 'j':	// json [msg]
		if (strncmp(bc, "json", 4) == 0)
		{	nr_marks(0);
			json_plus = (bc[4] == '+');
			if (cnt > 0)
			{	if (strlen(bc) > strlen("json "))
				{	json(bc + strlen("json ") + json_plus);
				} else
				{	json(" ");
			}	}
			return 1;
		} // else it maps to j[ump]
		break;

	default:
		break;
	}
	return 2;	// not handled yet
}

static int
one_command(char *bc)
{	char *a, *b;
	int i, n;
	static int dfd = 0;
	static char s[16];

	if (strlen(bc) == 0)
	{	return 1;
	}

	// reset defaults
	if (strcmp(bc, "%{") != 0 && !prog_fd)
	{	inverse  = inside_range = 0;
		and_mode = top_only = top_up = 0;
	}
	prefix = suffix = 0;
	did_prep = cnt = 0;

	if (dfd != 0)
	{	strcat(bc, "\n");
		if (write(dfd, bc, strlen(bc)) != strlen(bc))
		{	perror("write");
			close(dfd);
			dfd = 0;
			return 1;
		}
		if (strncmp(bc, "end", 3) == 0)
		{	close(dfd);
			dfd = 0;
			(void) try_read(s);
			unlink(s);
		}
		return 1;
	}

	if (strncmp(bc, "def", 3) == 0
	&& (bc[3] == ' ' || bc[3] == '\t'))
	{	strcpy(s, "/tmp/_c_XXXXXX");
		if ((dfd = mkstemp(s)) < 0)
		{	fprintf(stderr, "error: cannot create tmp file for script definition\n");
			return 1;
		}
		strcat(bc, "\n");
		if (write(dfd, bc, strlen(bc)) != strlen(bc))
		{	perror("write");
			close(dfd);
			dfd = 0;
		}
		return 1;
	}

	if (strlen(bc) == 0)
	{	strncpy(bc, dflt, MAXYYTEXT);
	}

	switch (pre_scan(bc)) {	// some commands are handled here
	case 0:  return 0;	// quit session
	case 1:  return 1;	// command handled
	default: break;		// handle below by table lookup
	}

	a = nextarg(bc);
	b = nextarg(a);
	n = strlen(bc);

	if (*a == '(' && *b == '(')
	{	printf("can have only one expression per command\n");
		return 1;
	}

	for (i = 0; n > 0 && table[i].cmd; i++)
	{	if (strncmp(bc, table[i].cmd, n) == 0)
		{	if (n < table[i].n)
			{	fprintf(stderr, "error: ambiguous, need");
				fprintf(stderr, " at least %d chars\n",
					table[i].n);
				break;
			}
			// handle a special case, e.g.:
			//   m /.; m & (.fnm == cobra_lib.c && .lnr == 1867)
			int xx = strlen(b);
			int yy = strlen(a);

			if (xx == 0
			&&  yy > 2		// minimally ( ... )
			&&  *a == '('
			&&  a[yy-1] == ')')	// whole expr in first arg, no spaces)
			{	a[yy-1] = '\0';
				goto X;
			}

			if (xx > 2		// minimally   ... )
			&& *a == '('
			&&  b[xx-1] == ')'
			&&  strlen(a) + xx < MAXYYTEXT-2)
			{	b[xx-1] = ' ';	// to insert a space before the )

				// put a and b together in a
			X:	snprintf(specialcase, sizeof(specialcase), "%s %s)", a, b);
		
				 a = specialcase;
				*b = '\0';
		
				{	char *ob = b_cmd;
					char *oy = yytext;
					b_cmd = yytext = specialcase;
					if (prep_eval())
					{	did_prep = 1;
					} else
					{	did_prep = -1;
					}
					b_cmd = ob;
					yytext = oy;
			}	}

			table[i].f(a, b);

			if (*bc != 'a'
			&&  *bc != 'd'
			&&  *bc != 'h'
			&&  *bc != 'i'
			&&  *bc != 'l'
			&&  *bc != 'p'
			&&  (*bc != 'r'
			 || (strncmp(bc, "re", 2) == 0
			 &&  strncmp(bc, "reset", 5) != 0))
			&&  *bc != '?'
			&&  *bc != '>'
			&&  !inscript
			&&  !no_match)
			{	printf("%d match%s\n", cnt, (cnt==1)?"":"es"); // cmd
			}
			break;
	}	}

	if (!table[i].cmd && n > 0)
	{	do_script(bc, a, b);
	}

	return 1;
}

static int
one_line(char *buf)
{	char *r, *z;

	if (verbose>1)
	{	printf("one_line: '%s'\n", buf);
	}
	if ((r = strchr(buf, '\n')) != NULL)
	{	*r = '\0';
	}
	b_cmd = (char *) skipwhite(buf);

	if (!prog_fd)
	{	if (*b_cmd != '!'
		&& (r = strchr(b_cmd, '#')) != NULL
		&& (r == b_cmd || *(r-1) != '\\'))
		{	z = strstr(b_cmd, "%{");
			if (!z || z > r)
			{	*r = '\0';	// strip comment
		}	}
		if (strcmp(b_cmd, "%{") != 0)
		{	add_history(b_cmd);
	}	}

	do {	r = b_cmd;
		if (*r == '!' || prog_fd)
		{	r = NULL;
		} else
		{	char *ep = NULL;
			char *sp = strstr(r, "%{");
			if (sp)
			{	ep = strstr(sp, "%}");
			}
			while ((r = strchr(r, ';')) != NULL)
			{	if (sp
				&&  r > sp
				&& (!ep || r < ep))
				{	r++;
					continue;
				}
				if (*(r-1) == '\\')
				{	for (z = r-1; *z; z++)
					{	*z = *(z+1);
					}
				} else
				{	*r++ = '\0';
					r = (char *) skipwhite(r);
					break;
		}	}	}
		if (verbose>1)
		{	printf("one_command: '%s' -- '%s'\n",
				b_cmd, r);
		}
		if (!one_command(b_cmd))
		{	return 0;
		}
		fflush(stdout);
		b_cmd = r;
	} while (r && strlen(r) > 0);

	return 1;
}

static void
list(char *s, char *t)
{	char *lstfnm = NULL;
	int lstlnr = 0;
	int hit = 0, n = 0, tlnr = 0;
	int lastn = -1, plus = 0, lcnt = 0;
//	int from=0, upto=0;
	char *c, *lastfnm = "";
	FILE *fd = (track_fd) ? track_fd : stdout;


	if (strcmp(s, "*") != 0)
	{	if (strchr(s, '-'))
		{	// range option pending
		//	if (sscanf(s, "%d-%d", &from, &upto) != 2)
		//	{	fprintf(stderr, "error: bad arg '%s'\n", s);
		//		return;
		//	}
		} else
		{	n = (*s)?atoi(s):0;
//			from = upto = n;
	}	}

	if (*t)
	{	if (*t == '+')
		{	plus++;
			t++;
		}
		tlnr = atoi(t);
		if (!raw && !source)
		{	fprintf(stderr, "error: usage list [n]\n");
			return;
	}	}

	if (no_display)
	{	return;
	}

	for (cur = prim; !p_stop && cur; cur = cur?cur->nxt:0)
	{	if (!cur->mark)
		{	lstfnm = NULL; // force fnm on each match
			continue;
		}

		if (cobra_lnr() == lastn
		&&  strcmp(lastfnm, cobra_bfnm()) == 0)
		{	continue;
		}

		lastfnm = cobra_bfnm();
		lastn = cobra_lnr();
		lcnt++;

		if (n != 0 && lcnt != n)
		{	continue;
		}

		if (!scrub
		&&  (!lstfnm
		||   strcmp(lstfnm, cobra_bfnm()) != 0))
		{	fprintf(fd, "%s:%d", cobra_bfnm(), lastn);
			if (cur->bound)
			{	char *z = strrchr(cur->bound->fnm, '/');
				z = (z)?z+1:cur->bound->fnm;
				fprintf(fd, "<->%s:%d", z, cur->bound->lnr);
			} else
			{	fprintf(fd, ":");
			}
			fprintf(fd, "\n");
			lstfnm = cobra_bfnm();
			hit = 0;
		}

		if (raw)
		{	reproduce(lcnt, (*t)?t:"0");
		}

		if (!scrub
		&&  hit
		&& (tlnr > 0 || lstlnr+1 != cobra_lnr()))
		{	if (verbose && !gui)
			{	fprintf(fd, "\n"); // to separate records
			}
		} else
		{	hit++;
		}

		lstlnr = cobra_lnr();

		if (raw)
		{	continue;
		}

		if (source
		&&  cur
		&&  strlen(cur->fnm) > 0
		&&  strcmp(cur->fnm, "stdin") != 0)
		{	if (tlnr < 0)
			{	show_line(fd, cobra_fnm(), lcnt, lastn+tlnr, lastn, lastn);
			} else if (plus)
			{	show_line(fd, cobra_fnm(), lcnt, lastn, lastn+tlnr, lastn);
			} else
			{	show_line(fd, cobra_fnm(), lcnt, lastn-tlnr, lastn+tlnr, lastn);
			}

			if (n != 0
			&& !nowindow
			&& !cobra_texpr)
			{	popup_window(cobra_fnm(), cobra_lnr());
			}
		} else
		{	if (scrub)
			{	fprintf(fd, "%s:%d: ", cobra_fnm(), cobra_lnr());
			} else
			{	if (!cobra_texpr)
				{	fprintf(fd, "%3d: ", lcnt);
				}
				if (gui)
				{	fprintf(fd, "%s:%05d:  ", cobra_fnm(), cobra_lnr());
			}	}
			if (count < 500000)
			{	c = fct_which(cur);
				if (!scrub && c && !cobra_texpr)
				{	if (strcmp(c, "global") == 0)
					{	printf("  ");
					}
					fprintf(fd, "%12s%s:\t", c, strcmp(c, "global")?"()":"");
			}	}
			if (scrub)
			{	fprintf(fd, "'%s'\n", cobra_txt());
			} else
			{	if (!cobra_texpr)
				{	fprintf(fd, " %5d", cobra_lnr());
				}
				fprintf(fd, " \t%s", cobra_txt());
				if (cobra_texpr)
				{	Prim *q = cur->nxt;
					while (q && q->mark && q->lnr == cur->lnr)
					{	fprintf(fd, " %s", q->txt);
						q = q->nxt;
				}	}
				fprintf(fd, "\n");
		}	}
		if (n != 0)
		{	break;
	}	}
	p_stop = 0;
}

void
display(char *s, char *t)
{
	source = 1;
	list(s, t);
	source = 0;
}

static void
prep(char *s, char *t)
{
	raw = 1;
	list(s, t);
	raw = 0;
}

static void *
undo_range(void *arg)
{	int *i = (int *) arg;
	Prim *of, *r, *from, *upto;
	int om, local_cnt = 0;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
	{	om         = r->mark;
		r->mark    = (int) r->mset[0];
		r->mset[0] = (short) om;

		of	   = r->bound;
		r->bound   = r->mbnd[0];
		r->mbnd[0] = of;

		if (r->mark)
		{	local_cnt++;
	}	}

	tokrange[*i]->param = local_cnt;

	return NULL;
}

static void
undo(char *s, char *t)
{
	if (*s || *t)
	{	fprintf(stderr, "warning: undo takes no arguments\n");
	}
	run_threads(undo_range, 6);
	undo_matches();
}

static char *
preamble(char *s, int n)
{
	if (*s == '/')
	{	(void) regstart(n, s+1);
	}
	if (strcmp(s, "@cmnt") == 0
	&&  with_comments == 0)
	{	static int warned = 0;
		if (!warned)
		{	warned = 1;
			fprintf(stderr, "error: @cmnt requires command-line option -comments\n");
	}	}
	return s;
}

void *
nr_marks_range(void *arg)
{	int *i = (int *) arg;
	Prim *r, *from, *upto;
	int local_cnt = 0;
	int n = global_n;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	if (n == 0)
	{	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
		{	if (r->mark)
			{	local_cnt++;
		}	}
	} else
	{	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
		{	if (r->mset[n])
			{	local_cnt++;
	}	}	}

	tokrange[*i]->param = local_cnt;

	return NULL;
}

static void *
contains_range(void *arg)
{	int *i = (int *) arg;
	Prim *q, *r, *stop, *from, *upto;
	char *s = global_s;
	char *t = global_t;
	int found;
	int local_cnt = 0;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	// uses negative marks as temporary place-holders

	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
	{	if (r->mark <= 0)
		{	continue;
		}
		r->mark = 0;
		if (!r->jmp && !r->bound)
		{	continue;
		}
		stop = (r->bound && r->bound->seq > r->seq)?r->bound:r->jmp;
		if (!stop)
		{	break;
		}
		found = 0;
		for (q = r->nxt; q && q->seq < stop->seq; q = q->nxt)
		{	// not reliable for yacc generated files
			// could check filename prefix up to .
			// if (q->lnr < r->lnr)
			// {	break;	// likely file boundary crossed
			// }
			if ((top_up && one_up(r, q, s))
			|| (!top_up && (!top_only || same_level(r, q, s))))
			{	if (r_apply(r, q, s, 0)
				&& (!*t || r_apply(r, q->nxt, t, 1)))
				{	found = 1;
					if (and_mode && !inverse)
					{	local_cnt++;
						q->mark = -1;	// hide from test at loop-start
					}
					break;
		}	}	}
		if (( found && !inverse)
		||  (!found &&  inverse))
		{	if (!and_mode || inverse)
			{	local_cnt++;
				r->mark++;
	}	}	}
	tokrange[*i]->param = local_cnt;

	// now safet to flip temporary marks to positive
	if (and_mode && !inverse && local_cnt > 0)
	{	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
		{	if (r->mark < 0)
			{	r->mark = -(r->mark);
	}	}	}

	return NULL;
}

static void
contains(char *s, char *t)	// 1, assume a range is selected
{
	if (inside_range)
	{	fprintf(stderr, "error: c[ontains]: unsupported qualifier\n");
		return;
	}
	if (!*s)
	{	fprintf(stderr, "error: invalid query -- missing pattern\n");
		return;
	}
	global_s = preamble(s, 0);
	global_t = preamble(t, 1);

	run_bup_threads(contains_range);
}

static void *
extend_range(void *arg)
{	int *i = (int *) arg;
	Prim *q, *r, *from, *upto;
	char *s = global_s;
	char *t = global_t;
	int local_cnt = 0;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
	{	if (!r->mark)
		{	continue;
		}
		r->mark = 0;
		q = r;
		r = r->nxt;
		if (r_apply(r, r, s, 0))
		{	if (*t && !r_apply(r, r->nxt, t, 1))
			{	continue;
			}
			local_cnt++;
			q->mark++;
		}
		r = r->prv;
	}

	tokrange[*i]->param = local_cnt;
	return NULL;
}

static void
extend(char *s, char *t)
{
	if (inverse || top_only || top_up || inside_range || and_mode)
	{	fprintf(stderr, "error: e[xtend] does not support qualifiers\n");
		return;
	}
	if (!*s)
	{	printf("invalid query - extend\n");
		return;
	}
	global_s = preamble(s, 0);
	global_t = preamble(t, 1);

	run_bup_threads(extend_range);
}

static void *
stretch_range(void *arg)
{	int *i = (int *) arg;
	Prim *q, *r, *from, *upto;
	char *s = global_s;
	char *t = global_t;
	int local_cnt = 0;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
	{	if (!r->mark)
		{	continue;
		}
		for (q = r->nxt; q; q = q->nxt)
		{	if ((top_up && one_up(r, q, s))
			|| (!top_up && (!top_only || same_level(r, q, s))))
			{	if (r_apply(r, q, s, 0)
				&&  (!*t || r_apply(r, q->nxt, t, 1)))
				{	local_cnt++;
					r->bound = q;
					q->bound = r;
					if (0)
					{ printf("%s:%d -> %s:%d\n",
						r->fnm, r->lnr, q->fnm, q->lnr);
					}
					break;
		}	}	}
		if (!q)	// not found
		{	r->mark = 0;
	}	}
	tokrange[*i]->param = local_cnt;

	return NULL;
}

static void
stretch(char *s, char *t)
{
	if (inverse || inside_range || and_mode)
	{	fprintf(stderr, "error: s[tretch]: unsupported qualifier\n");
		return;
	}
	if (!*s)
	{	fprintf(stderr, "invalid query - s[tretch] (missing arg)\n");
		return;
	}
	global_s = preamble(s, 0);
	global_t = preamble(t, 1);

	run_bup_threads(stretch_range);
}

static void *
mark_range(void *arg)
{	int *i = (int *) arg;
	Prim *q, *r, *stop, *from, *upto;
	char *s = global_s;
	char *t = global_t;
	int local_cnt = 0;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	if (inside_range)
	{	if (and_mode)
		{	for (r = upto; r && r->seq >= from->seq; r = r->prv)
			{	if (!r->mark)
				{	continue;
				}
				if (r->bound && r->bound->seq > r->seq)
				{	stop = r->bound;
				} else
				{	stop = r->jmp;
				}
				if (!stop)
				{	break;
				}
				for (q = r->nxt; q && q->seq < stop->seq; q = q->nxt)
				{	if (!q->mark)
					{	continue;
					}
					if (!r_apply(r, q, s, 0)
					||  (*t && !r_apply(r, q->nxt, t, 1)))
					{	q->mark = 0;
					} else
					{	local_cnt++;
			}	}	}
		} else
		{	for (r = upto; r && r->seq >= from->seq; r = r->prv)
			{	if (!r->mark)
				{	continue;
				}
				if (r->bound && r->bound->seq > r->seq)
				{	stop = r->bound;
				} else
				{	stop = r->jmp;
				}
				if (!stop)
				{	break;
				}
				r->mark = 0;
				for (q = r->nxt; q && q->seq < stop->seq; q = q->nxt)
				{	if (r_apply(r, q, s, 0))
					{	if (*t && !r_apply(r, q->nxt, t, 1))
						{	continue;
						}
						local_cnt++;
						q->mark = 1;
		}	}	}	}
	} else
	{	if (and_mode)
		{	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
			{	if (!r->mark)
				{	continue;
				}
				if (!r_apply(r, r, s, 0)
				||  (*t && !r_apply(r, r->nxt, t, 1)))
				{	r->mark = 0;
				} else
				{	local_cnt++;
			}	}
		} else	// the nominal case, with or without inverse
		{	//start_timer(*i);
			for (r = from; r && r->seq <= upto->seq; r = r->nxt)
			{	if (r_apply(r, r, s, 0)
				&&  (( inverse &&  r->mark)
				||   (!inverse && !r->mark)))
				{	if (!(*t && !r_apply(r, r->nxt, t, 1)))
					{	if (inverse)
						{	r->mark = 0;
						} else
						{	r->mark = 1;
				}	}	}
				if (r->mark)
				{	local_cnt++;
			}	}
			//stop_timer(*i, 0, "C");
	}	}
	tokrange[*i]->param = local_cnt;

	return NULL;
}

static void
mark(char *s, char *t)
{
	if (top_only || top_up)
	{	fprintf(stderr, "error: m[ark] does not support qualifiers top or up\n");
		return;
	}

	if (inverse)
	{	if (inside_range)
		{	fprintf(stderr, "error: m[ark]: cannot combine 'not' and 'ir'\n");
			return;
		}
		if (and_mode)
		{	fprintf(stderr, "error: m[ark]: cannot combine 'not' and '&'\n");
			return;
	}	}

	if (!*s)
	{	fprintf(stderr, "invalid query - mark '%s' - '%s'\n", s, t);
		return;
	}
	global_s = preamble(s, 0);
	global_t = preamble(t, 1);

	run_bup_threads(mark_range);
}

static void *
inspect_range(void *arg)
{	int *i = (int *) arg;
	Prim *r, *from, *upto;
	int local_cnt = 0;
	int lnr;
	int a, b;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;
	lnr = atoi(global_t);

	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
	{	if (strcmp(r->fnm, global_s) == 0
		&&  r->lnr == lnr)
		{	a = strlen(r->txt);	// 1
			b = strlen(r->typ);	// 0
			printf("%s ", r->txt);
			while (a++ < b)
			{	printf(" ");
			}
			local_cnt = 1;
	}	}
	if (!local_cnt)
	{	return NULL;
	}
	printf("\n");
	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
	{	if (strcmp(r->fnm, global_s) == 0
		&&  r->lnr == lnr)
		{	a = strlen(r->txt);	// 1
			b = strlen(r->typ);	// 0
			printf("%s ", r->typ);
			while (b++ < a)
			{	printf(" ");
			}
			local_cnt++;
	}	}
	printf("\n");
	tokrange[*i]->param = 0; // local_cnt-1;
	return NULL;
}

static void
inspect(char *fnm, char *lnr)
{
	if (inverse || inside_range || and_mode)
	{	fprintf(stderr, "error: i[nspect] does not take qualifiers\n");
		return;
	}
	if (!*fnm || !*lnr)
	{	fprintf(stderr, "invalid query -- i[nspect] fnm lnr\n");
		return;
	}
	global_s = preamble(fnm, 0);
	global_t = preamble(lnr, 1);
	run_bup_threads(inspect_range);
}

static void *
jump_range(void *arg)
{	int *i = (int *) arg;
	Prim *q = NULL, *r, *dest, *from, *upto;
	int local_cnt = 0;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
	{	if (!r->mark)
		{	continue;
		}
		dest = (r->bound && r->bound->seq > r->seq) ? r->bound : r->jmp;
		if (dest && r->mset[0] == (short) r->mark)
		{	q = dest;
		} else if (r->jmp && r->mset[0] == (short) r->mark)
		{	q = r->jmp;
		} else
		{	continue;
		}
		q->mark = r->mark;
		r->mark = 0;
		local_cnt++;
	}
	tokrange[*i]->param = local_cnt;
	return NULL;
}

static void
jump(char *s, char *t)
{
	if (*s || *t)
	{	printf("invalid query - j[ump] (redundant args)\n");
		return;
	}

	run_bup_threads(jump_range);
}

static void *
next_range(void *arg)
{	int *i = (int *) arg;
	Prim *q, *r, *from, *upto;
	char *s = global_s;
	char *t = global_t;
	int local_cnt = 0;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	for (r = upto; r && r->seq >= from->seq; r = r->prv)
	{	if (!r->mark)
		{	continue;
		}
		r->mark = 0;
		if (*s)	// if s is given, move to that token
		{	for (q = r; q; q = q->nxt)
			{	if (r_apply(r, q, s, 0)
				&& (!*t || r_apply(r, q->nxt, t, 1)))
				{	local_cnt++;
					q->mark = 1;
					break;
			}	}
		} else
		{	if (r->nxt)
			{	r->nxt->mark = 1;
				local_cnt++;
	}	}	}

	tokrange[*i]->param = local_cnt;

	return NULL;
}

static void
next(char *s, char *t)
{
	if (inverse || top_only || top_up || inside_range || and_mode)
	{	fprintf(stderr, "error: n[ext] does not support qualifiers\n");
		return;
	}
	global_s = preamble(s, 0);
	global_t = preamble(t, 1);

	run_bup_threads(next_range);
}

static void *
back_range(void *arg)
{	int *i = (int *) arg;
	Prim *q, *r, *from, *upto;
	char *s = global_s;
	char *t = global_t;
	int local_cnt = 0;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
	{	if (!r->mark)
		{	continue;
		}
		r->mark = 0;
		if (*s)	// if s is given, move to that token
		{	for (q = r->prv; q && q->seq >= prim->seq; q = q->prv)
			{	if (r_apply(r, q, s, 0)
				&& (!*t || r_apply(r, q->nxt, t, 1)))
				{	q->mark = 1;
					local_cnt++;
					break;
			}	}
		} else
		{	if (r->prv)
			{	r->prv->mark = 1;
				local_cnt++;
	}	}	}
	tokrange[*i]->param = local_cnt;

	return NULL;
}

static void
back(char *s, char *t)
{
	if (inverse || top_only || top_up || inside_range || and_mode)
	{	fprintf(stderr, "error: b[ack] does not support qualifiers\n");
		return;
	}
	global_s = preamble(s, 0);
	global_t = preamble(t, 1);

	run_bup_threads(back_range);
}

static void
readf(char *s, char *t)
{	char *as;
	if (!*s || *t)
	{	printf("invalid command - append f.c\n");
		return;
	}
	ini_pre(0);
	as = (char *) emalloc(strlen(s)+1, 66);
	strcpy(as, s);
	if (add_file(as, 0, 1))	// cid: single-core
	{	post_process(0);
		ctokens();
	}
}

static void
history(char *unused1, char *unused2)
{
	cnt = 0;
	rev_print(h_last);
}

static void *
prog_range(void *arg)
{	int *i = (int *) arg;
	Prim *r, *from, *upto;
	int j = 0, local_cnt = 0;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

//	start_timer(Ncore + *i);

	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
	{	j = exec_prog(&r, *i);

		if (stream == 1)
		{	if (r->seq + 100 > upto->seq)	// getting too close to the end
			{	Prim *place = r;
				if (add_stream(r))	// can free up to cur
				{	r = place;
					upto = plst;
				} else	// exhausted input stream
				{	stream = 0;
					if (verbose)
					{	printf("exhausted input\n");
		}	}	}	}

		if (j == -2)
		{	continue;
		}
		if (j == -1)
		{	break;
		}
		local_cnt += j;
	}
#ifdef DEBUG_MEM
	extern void report_memory_use(void);
	report_memory_use();
#endif
//	stop_timer(Ncore + *i, 1, "Program");

	tokrange[*i]->param = local_cnt;

	if (verbose>1)
	{	printf("cpu%d\n", *i);
		fflush(stdout);
	}

	extern void wrap_stats(void);
	wrap_stats();
#if 1
	extern void list_stats(void);
	list_stats();
#endif

	return NULL;
}

static void
prog(FILE *fd)
{
	if (prep_prog(fd))
	{	// start_timer(0);
		if (stream == 1)
		{	static int script_nr = 0;
			static int first_has_stop = -1;
			script_nr++;
			if (script_nr == 1)
			{	while (add_stream(0)) // make sure we have enough tokens
				{	if (plst && plst->seq > stream_lim)
					{	break;
				}	}
				first_has_stop = has_stop();
			}
			if (script_nr > 1 && first_has_stop == 0)
			{	fprintf(stderr, "error: saw multiple scripts (can only stream one)\n");
				fprintf(stderr, "error: and initial script does not end with STOP\n");

			}
			if (verbose)
			{	printf("cobra: running script #%d\n", script_nr);
		}	}

		run_bup_threads(prog_range);
		if (verbose>1)
		{	printf("\n");
		}
		// stop_timer(0,0, "D");
	}
}

static void *
eval_range(void *arg)
{	int *i = (int *) arg;
	Prim *r, *from, *upto;
	int local_cnt = 0, x;

	from = tokrange[*i]->from;
	upto = tokrange[*i]->upto;

	if (and_mode)
	{	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
		{	if (!r->mark)
			{	continue;
			}
			r->mark = 0;
			if ((x = do_eval(r)) != 0)	// thread-safe
			{	r->mark = 1;
				local_cnt++;
				if (prefix)
				{	printf("%s:%d\t%s %d\n",
						r->fnm, r->lnr,
						prefix, x);
		}	}	}
	} else
	{	for (r = from; r && r->seq <= upto->seq; r = r->nxt)
		{	if (view_mode)
			{	if (!r->mark)
				{	continue;
				}
				x = do_eval(r);
				printf("%s:%d\t%s %d\n",
					r->fnm, r->lnr,
					prefix?prefix:"", x);
				continue;
			}

			if (r->mark)
			{	continue;		// already set
			}
			if ((x = do_eval(r)) != 0)	// thread-safe
			{	r->mark = 1;
				local_cnt++;
				if (prefix)
				{	printf("%s:%d\t%s %d\n",
						r->fnm, r->lnr,
						prefix, x);
	}	}	}	}

	tokrange[*i]->param = local_cnt;
	return NULL;
}

static void
eval(char *s, char *unused)
{	char *ob = b_cmd;
	char *oy = yytext;

	if (inverse || top_only || top_up || inside_range)
	{	fprintf(stderr, "error: w[ith]/eval does not support qualifiers\n");
		return;	// only an implicit &
	}
	if (!*s)
	{	fprintf(stderr, "error: w[ith]/eval missing argument\n");
		return;
	}
	b_cmd = yytext = s;
	if (prep_eval())		// calls regstart if needed
	{	if (!strchr(s, '.'))	// no refs to tokens
		{	cnt = -(do_eval(0) + 1); // once
			// always < 0
		} else if (strstr(s, "size"))
		{	fprintf(stderr, "error: dot equation contains size()\n");
			// would be quadratic
		} else
		{	run_bup_threads(eval_range); // calls regstop
	}	}
	b_cmd  = ob;
	yytext = oy;
}

static void
with_eval(char *s)
{
	while (isspace((uchar) *s))
	{	s++;	// leading spaces
	}

	if (verbose == 1)
	{	printf("eval: '%s'\n", s);
	}

	if (*s == '(')
	{	int ln = strlen(s)-1;
		assert(ln < MAXYYTEXT-2);
		if (s[ln] != ')')
		{	fprintf(stderr, "error: missing ) at end of '%s'\n", s);
			return;
		}
		s[ln++] = ' ';
		s[ln++] = ')';
		s[ln] = '\0';
	}

	eval(s, 0);	 // with_eval
}

static void
help(char *s, char *unused)	// 1
{	int i;
	int j = 0;

	printf("Command Summary\n");
	printf("short-hand / full-text / arguments / explanation\n");
	printf("q is a qualifier (see below), s, t, and f are strings\n");
	printf("p and p2 are strings or expressions matching the text of a token\n");

	for (i = 0; table[i].cmd; i++)
	{	if (table[i].n > 1)
		{	continue;
		}
		if (!*s
		||  strstr(table[i].cmd, s))
		{	printf(" %c  %8s  %s\n",
				table[i].cmd[0],
				table[i].cmd,
				table[i].explanation);
			j++;
	}	}
	if (*s)
	{	if (j)
		{	return;
	}	}

	printf(" %c  %8s  %s\n", 'q', "quit", "         end cobra session");
	printf("\n");
	printf("  %8s  %c %s\n", ">n",  ' ', "        save marks and ranges in set n: 1..3");
	printf("  %8s  %c %s\n", ">=n", ' ', "        same as >n");
	printf("  %8s  %c %s\n", "<n",  ' ', "        restore marks and ranges from set n: 1..3");
	printf("  %8s  %c %s\n", "<=n", ' ', "        same as <n");
	printf("  %8s  %c %s\n", "<|n", ' ', "        or <+n, add marks and ranges from set n (union)");
	printf("  %8s  %c %s\n", "<&n", ' ', "        or <*n, keep only marks and ranges also in set n (intersect)");
	printf("  %8s  %c %s\n", "<^n", ' ', "        or <-n, keep only marks and ranges not in set n (subtract)");
	printf("  %8s  %c %s\n", ": s", ' ', "        execute the script named s (cf cmdline arg -f)");
	printf("  %8s  %c %s\n", ". f", ' ', "        load and execute commands from file f (cf cmdline arg -f)");
	printf("  %8s  %c %s\n", "! c", ' ', "        execute command(s) c in a background shell");
	printf("  %8s  %c %s\n", "%{  ...  %}", ' ', "     execute an inline Cobra program");
	printf("  %8s  %c %s\n", "def ... end", ' ', "     define a named script");

	printf("\nAdditional commands that cannot be abbreviated:\n");
	printf("  %8s  %c %s\n", "cfg", ' ',            "f         show control-flow graph for fct f (cf fcg)");
	printf("  %8s  %c %s\n", "cpp", ' ',		"off|on    disable/re-enable preprocessing");
	printf("  %8s  %c %s\n", "context", ' ',	"f         show callers and callees of a named function");
//	printf("  %8s  %c %s\n", "declarations", ' ',	"      print type @ident sequences");
	printf("  %8s  %c %s\n", "default", ' ',	"c         set c as the default command on empty line (eg 'default !date'");
//	printf("  %8s  %c %s\n", "eval", ' ',		"(expr)    same as: m & (expr) and also same as: w[ith] (expr)");
	printf("  %8s  %c %s\n", "fcts", ' ',		"          mark all fct definitions");
	printf("  %8s  %c %s\n", "fcg", ' ',            "[f|*] [g] show fct call graph [from fct f] [to fct g]");
	printf("  %8s  %c %s\n", "ff", ' ',		"f         find function f");
	printf("  %8s  %c %s\n", "ft", ' ',		"t         mark the defintion of structure type t");
	printf("  %8s  %c %s\n", "json", ' ',		"[msg]     print results of a pattern search (pe) in json format");
	printf("  %8s  %c %s\n", "ncore", ' ',		"n         set the number of cores to use to n");
	printf("  %8s  %c %s\n", "quiet", ' ',		"on|off    more/less verbose in script executions");
	printf("  %8s  %c %s\n", "save", ' ',		"n         alternative syntax for: >n");
	printf("  %8s  %c %s\n", "re", ' ',             "expr      match a token expression (cf commandline option -e expr)");
	printf("  %8s  %c %s\n", "pat", ' ',            "pattern   match a pattern (cf commandline option -pe expr)");
	printf("  %8s  %c %s\n", "pe", ' ',             "pattern   same as pat or pattern (pattern expression)");
	printf("  %8s  %c %s\n", "restore", ' ',	"n         alternative syntax for: <n");
	printf("  %8s  %c %s\n", "setlinks", ' ',	"          set .bound field for if/else/switch/case/break stmnts");
	printf("  %8s  %c %s\n", "stream"  , ' ',	"[mode=text] [limit=N] [margin=N] when streaming input from stdin");
	printf("  %8s  %c %s\n", "terse", ' ',		"on|off    enable/disable display of details with d/l/p");
	printf("  %8s  %c %s\n", "track", ' ',		"start fnm|stop temporarily divert all output to fnm");
	printf("  %8s  %c %s\n", "unmark", ' ',		"p [p2]    alternative syntax for: mark no p [p2]");
	printf("  %8s  %c %s\n", "view", ' ',           "          list known command script names");
	printf("  %8s  %c %s\n", "=", ' ',		"[string [(expr)]    print nr of matches, with optional string/expr");

	printf("\nFile Browsing\n");
	printf("  B file.c		browse file.c, starting at line 1\n");
	printf("  B file.c N		browse file.c, starting at line N\n");
	printf("  B N			continue browsing, moving to line N\n");
	printf("  B			continue browsing, advancing 20 lines\n");
	printf("  V file [N]		view the file with a popup tcl/tk window\n");
	printf("  F			list all currently open files\n");
	printf("  F name		print the full name of filenames containing name\n");
	printf("  G expr		grep -e \"expr\" in the source of all current files\n");

	printf("\nQualifiers\n");
	printf("  in the above list [q] refers to an option qualifier, see below\n");
	printf("  p, and p2 can be strings or regular expressions\n");
	printf("  regular expressions must start with a forward slash '/'\n");
	printf("  n, n1, and n2 are integer numbers, and s is a string\n");
	printf("  qualifiers [q] can be:\n");
	printf("    no  -- to find non-matches (supported in: contains and mark)\n");
	printf("    ir  -- restrict to matching in ranges (supported in: mark)\n");
	printf("    &   -- (or 'and') restrict to marks that also match a new pattern (mark)\n");
	printf("    top -- restrict to matching at same nesting level as mark (contains, stretch)\n");
	printf("    up  -- restrict to matching one nesting level up as mark (contains, stretch)\n");

	printf("\nToken Patterns\n");
	printf("  token-names can be identifiers, operators, keywords, or tokens\n");

	printf("\nToken Types\n");
	printf("  names starting with @ are considered typenames, with the following meaning:\n");
	printf("    @modifier  (long, short, signed, unsigned)\n");
	printf("    @qualifier (const, volatile)\n");
	printf("    @storage   (static, extern, register, auto)\n");
	printf("    @type      (int, char, float, double, void)\n");
	printf("    @ident (identifier), @chr, @str (string), @key (C keyword), @oper (operator)\n");
	printf("    @const_int, @const_hex, @const_oct, const_flt,\n");
	printf("    @cmnt (when using command-line option -comments)\n");
	printf("  struct, union, and enum are categorized as @key (i.e., keywords)\n");
	printf("  commands can be separated by newlines or semi-colons\n");

	printf("\nExpressions\n");
	printf("  expressions are boolean clauses enclosed in round braces\n");
	printf("  e.g.: mark (a <= b && (.lnr < 100 || .fnm == cobra_lib.c))\n");
	printf("  the usual arithmetic and boolean operators can be used.\n");
	printf("  operands can be constants or .len, .curly, .round, .bracket, .fct or .fnm\n");

	printf("\nSpecial Symbols\n");
	printf("  the symbol $$ refers to the text of the current token,\n");
	printf("  which can be useful with mark ir in scripts\n");
}

static void
ctokens(void)
{
	if (!no_match
	&& !cobra_commands
	&& !cobra_target
	&& !cobra_texpr)
	{	printf("%d core%s, %d files, %d tokens\n",
			Ncore, (Ncore > 1)?"s":"", Nfiles, count);
	}
	set_ranges(prim, plst);
}

static void
ihandler(int unused)
{
	p_stop++;
	stop_threads();
	printf("\ninterrupt\n: ");
	fflush(stdout);
}

static void
done(void)
{
	if (!preserve)
	{	unlink(CobraProg);
		unlink(CobraDot);
		unlink(FsmDot);
		clear_file_list();
	}
	if (track_fd != NULL)
	{	fclose(track_fd);
	}
	memusage();
}

static void
cleanup(int unused)
{
	printf("interrupted\n");
	done();
	exit(1);
}

// externally visible functions:

void
fix_eol(void)
{	// rescan with -eol
	prim = plst = (Prim *) 0;
	count = 0;
	rescan();
	ctokens();
//	fct_defs();
}

void
set_cnt(int n)
{
	cnt = n;
}

void
show_error(FILE *fd, int p_lnr)
{	char buf[MAXYYTEXT];
	int lns = 0;

	assert(prog_fd);

	rewind(prog_fd);
	while (fgets(buf, sizeof(buf), prog_fd))
	{	lns++;
		fprintf(stderr, "%c%2d %s",
			(lns==p_lnr)?'>':' ',
			lns, buf);
	}
}

void
set_regex(char *s)
{
	if (re_set[0])
	{	fprintf(stderr, "error: too many regular expressions in eval (%s)\n", s);
		return;
	}
	(void) regstart(0, s);
}

int
regex_match(const int n, const char *s)
{
	assert(n >= 0 && n < 2);
	return regexec(&rex[n], s, 0,0,0);
}

int
nr_marks(const int n)
{
	if (n >= 0 && n <=3)
	{	global_n = n;
		run_threads(nr_marks_range, 7);
		return cnt;
	}
	printf("expr: invalid query - size 1..3\n");
	return -1;
}

#include <termios.h>

static struct termios n_tio;

void
re_enable(void)
{
	n_tio.c_lflag |= (ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &n_tio);
}

void
noreturn(void)
{
	printf("cobra: fatal error\n");
	re_enable();
	exit(1);
}

void
cobra_main(void)
{	int ch, i, n, m;
	char buf[MAXYYTEXT];
	History *h_ptr = (History *) 0;

	ctokens();	// calls set_ranges

	if (cobra_target && cobra_texpr)
	{	printf("cobra: cannot specify both -f and -expr or -pattern\n");
		return;
	}

	set_tmpname(CobraProg, "cobra_prg", sizeof(CobraProg));
	set_tmpname(CobraDot,  "cobra_dot", sizeof(CobraDot));
	set_tmpname(FsmDot,    "cobra_fsm", sizeof(FsmDot));
	signal(SIGINT, cleanup);

	snprintf(ShowDot, sizeof(ShowDot), "%s %s%s &", DOT, C_TMP, CobraDot);
	snprintf(ShowFsm, sizeof(ShowFsm), "%s %s%s &", DOT, C_TMP, FsmDot);

	yytext = (char *) emalloc(MAXYYTEXT*sizeof(char), 67);
	
	if (cobra_texpr)
	{	cobra_te(cobra_texpr, 0, 0);
		done();
		return;
	}

	if (cobra_target)
	{	no_match = 1;	// silent mode
		read_scripts();
		if (!cobra_commands)
		{	done();
			return;
	}	}

	if (cobra_commands)
	{	if (strlen(cobra_commands) >= sizeof(buf))
		{	printf("cobra: command too long (max %d chars)\n",
				(int) sizeof(buf));
			done();
			return;
		}
		if (verbose>1)
		{	printf("execute cobra commands\n");
		}
		strcpy(buf, cobra_commands);
		(void) one_line(trimline(buf));
		done();
		return;
	}

	if (!gui)
	{	tcgetattr(STDIN_FILENO, &n_tio);
		// disable canonical mode and local echo
		n_tio.c_lflag &= (~ICANON & ~ECHO);
		// pass along SIGINT
		n_tio.c_lflag |= (ISIG);
		tcsetattr(STDIN_FILENO, TCSANOW, &n_tio);
		signal(SIGINT, ihandler);
	}

	for (;;)
	{
		if (gui)
		{  printf("Ready: \n");
		   fflush(stdout);
		   if (!fgets(buf, sizeof(buf), stdin))
		   {	break;
		   }
		} else
		{  if (!prog_fd)
		   {	printf("%c: ", 13);
		   }
		   n = 0;
		   memset(buf, 0, sizeof(buf));

		   do {
			m = 0;
			signal(SIGINT, SIG_IGN); // without this fgetc segfaults on control-c
			ch = fgetc(stdin);
			signal(SIGINT, ihandler);

			buf[n++] = (char) ch;
			switch (ch) {
			case 127:
			case 8:		// backspace
				buf[--n] = '\0'; // erase
erase:				if (n > 0)
				{	buf[--n] = '\0'; // prv char
					m = 1;
				}
				break;
			case 21:	// ctl-u
				n = 0;
				m = strlen(buf);
				memset(buf, 0, sizeof(buf));
				break;
			case 23:	// ctl-w
				buf[--n] = '\0'; // erase
				m = strlen(buf);
				while (n >= 0 && buf[n] != ' ')
				{	buf[n--] = '\0';
				}
				if (n > 0)
				{	n++;
				}
				break;

			case 27:	// esc
				// 27 91 65	up arrow
				// 27 91 66	down arrow
				// 27 91 67	right arrow
				// 27 91 68	left arrow
				buf[--n] = '\0'; // erase
				if (fgetc(stdin) != 91)
				{	continue; // ignore
				}
				switch (fgetc(stdin)) {
				case 65:	// up arrow
					if (h_ptr && h_ptr->prv)
					{	h_ptr = h_ptr->prv;
					} else if (!h_ptr)
					{	h_ptr = h_last;
					}
					break;
				case 66:	// down arrow
					if (h_ptr)
					{	if (h_ptr->nxt)
						{	h_ptr = h_ptr->nxt;
						} else
						{	n = 0;
							m = strlen(buf);
							memset(buf, 0, sizeof(buf));
							h_ptr = 0;
							goto X;
					}	}
					break;
				case 67:	// right arrow
					continue; // ignore
				case 68:	// left arrow
					goto erase;
				default:
					break;
				}
				if (h_ptr
				&&  strlen(h_ptr->s) < sizeof(buf))
				{	m = strlen(buf);
					memset(buf, 0, sizeof(buf));
					snprintf(buf, sizeof(buf), "%s", h_ptr->s);
					n = strlen(buf);
				} else
				{	continue;
				}
				break;
			case EOF:
				goto out;
			default:
				break;
			}
		  X:	printf("%c%s %s", 13, prog_fd?"":":", buf);
			for (i = 0; i < m; i++)
			{	putchar(' ');
			}
			for (i = 0; i < m; i++)
			{	putchar(8);
			}
			// checking p_stop gets us a way out on interrupts
		  } while (ch != '\n' && n < sizeof(buf) && p_stop < 10);
		} // else
		if (!one_line(trimline(buf)))
		{	break;
		}

		h_ptr = (History *) 0;
		p_stop = 0;
	}
out:
	if (!gui) // re-enable
	{	re_enable();
	}
	done();
}

static void
xchange(char *s)
{	static Prim *o_prim, *o_plst, *tmp;
	static int o_count = 0, o_tmp;

	if (s)					// else swap
	{	if (strcmp(s, "off") == 0)	// want no_cpp
		{	if (no_cpp)
			{	return;		// already have it
			}
		} else if (strcmp(s, "on") == 0) // want cpp = !no_cpp
		{	if (!no_cpp)
			{	return;		// already have it
			}
		} else
		{	printf("%s flag '%s' (expecting on or off)\n",
				strlen(s)>0?"unrecognized":"missing", s);
			return;
		}
	} else
	{	printf("missing flag, expecting on or off\n");
		return;
	}

	if (no_cpp && no_cpp_sticks)
	{	static int warned;
		if (!warned)
		{	fprintf(stderr, "error: cpp was disabled on the command-line\n");
			warned=1;
		}
		return;
	}

	no_cpp = 1 - no_cpp;

	if (!no_match)
	{	printf("preprocessing: %s\n",
			no_cpp?"disabled":"enabled");
	}

	if (!o_prim || !o_plst)
	{	o_prim = prim;
		o_plst = plst;
		o_count = count;
		prim = plst = (Prim *) 0;
		count = 0;
		rescan();
	} else
	{	tmp = o_prim;
		o_prim = prim;
		prim = tmp;
		tmp = o_plst;
		o_plst = plst;
		plst = tmp;
		o_tmp = count;
		count = o_count;
		o_count = o_tmp;
	}

	ctokens();
	fct_defs();
}
