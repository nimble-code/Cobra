/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include "cobra_pre.h"

#ifdef PC
 #include <time.h>
#else
 #include <sys/times.h>
#endif

int ada;
int all_headers;
int cplusplus;
int Ctok;
int eol;
int gui;
int java;
int Ncore = 1;
int Nfiles;
int no_cpp = 1;
int no_cpp_sticks;
int no_display;
int no_headers;
int no_match;
int parse_macros;
int p_debug;
int preserve;
int python;
int read_stdin;
int stream;
int stream_lim = 8192;
int stream_margin = 1000;
int runtimes;
int scrub;
int verbose;
int with_comments = 1;
int full_comments;

ArgList		*cl_var;	// -var name=xxx, cobra_lib.c
pthread_t	*t_id;

char	*C_BASE  = ".";		// reassigned in main
char	*C_TMP = "";		// reassigned in main
char	*cobra_target;		// -f xxx
char	*cobra_texpr;		// -e xxx
char	*cobra_commands;	// -c xxx
char	*cwe_args;		// standalone cwe checker
char	*progname = "";		// e.g. cobra or cwe
char	*backend = "";		// args passed to backend with --arg

TokRange  **tokrange;

static Pass	*pass_arg;
static char	*CPP       = "gcc";
static char	*lang      = "c";
static char	*preproc   = "";
static int	 handle_typedefs = 1;
static int	 textmode;
static int	 with_qual = 1;
static int	 with_type = 1;

extern Prim	*prim, *plst;

extern void	 set_ranges(Prim *, Prim *);
extern void	 recycle_token(Prim *, Prim *);

static int
process(int cid)
{
	assert(cid >= 0 && cid < Ncore);
	memset(Px.lex_cpp, 0, sizeof(Px.lex_cpp));
	line(cid);	// sets fnm etc

	if (textmode)
	{	t_lex(cid);	// returns on EOF
	} else
	{	while (c_lex(cid) > 0)
		{	;
		}
		return sanitycheck(cid);
	}
	return 0;
}

static void
add_preproc(const char *s)	// before parsing
{
	if (strlen(preproc) == 0)
	{	preproc = (char *) emalloc(strlen(s) + 1, 16);
		strcpy(preproc, s);
		no_cpp = with_comments = 0;
	} else
	{	char *op = preproc; // emalloc uses sbrk
		int    n = strlen(op) + strlen(s) + 2;
		preproc = emalloc(n*sizeof(char), 17);
		snprintf(preproc, n, "%s %s", op, s);
	}
}

// externally visible functions:

void
set_textmode(void)
{
	textmode = 1;
	printf("stream mode: text\n");
}

void
set_tmpname(char *s, const char *t, const int n)
{	int fd;

	snprintf(s, n, "/tmp/%s_XXXXXX", t);
	fd = mkstemp(s);
	if (fd < 0)
	{	printf("warning: cannot create tmpfile '%s'\n", t);
	} else
	{	close(fd);
	}
}

void
ini_pre(int cid)
{
	assert(cid >= 0 && cid < Ncore);
	pre[cid].lex_count = count;
	pre[cid].lex_plst = 0;
	pre[cid].lex_prim = 0;
	pre[cid].lex_last = 0;
}

int
add_file(char *f, int cid, int slno)
{	FILE *tfd = (FILE *) 0;
	char fnm[32];
	char tfn[32];
	int imbalance;
	int lncnt = 0;

	assert(cid >= 0 && cid < Ncore);
	Px.lex_lineno = slno;
	Px.lex_fname  = f;
	*fnm = '\0';
	// fprintf(stderr, "%d Parse %s\n", cid, f);
	if (strlen(f) == 0)		// no filename: read stdin
	{	char buff[1024];

		assert(read_stdin);
		set_tmpname(tfn, "af1", sizeof(tfn));
		f = tfn;
		if ((tfd = fopen(f, "w")) == NULL)
		{	fprintf(stderr, "cannot open tmp file %s\n", f);
			return 0;
		}
		while (fgets(buff, sizeof(buff), stdin))
		{	fprintf(tfd, "%s", buff);
			lncnt += strlen(buff);	// counts bytes
			if (stream == 1
			&&  lncnt > stream_lim)
			{	break;
			}
		}
		if (verbose > 1 && stream == 1)
		{	static long total_read = 0;
			total_read += lncnt;
			printf("	Read %d bytes of %d max\tcumulative: %ld\n",
				lncnt, stream_lim, total_read);
		}
		fclose(tfd);
	}

	if (no_cpp)
	{	if ((Px.lex_yyin = open(f, O_RDONLY)) < 0)
		{	fprintf(stderr, "cannot open file '%s'\n", f);
			return 0;
		}
	} else
	{	char *buf;
		int   n;

		set_tmpname(fnm, "af2", sizeof(fnm));

		n = strlen(CPP)
			    + strlen(" ")
			    + strlen(preproc)
			    + strlen(" -w -E -x c ")	// suppress warnings
			    + strlen(f)
			    + strlen(" > ")
			    + strlen(fnm)
			    + 8;	// some margin

		buf = (char *) hmalloc(n, cid, 124);
		snprintf(buf, n, "%s %s -w -E -x %s %s > %s",
			    CPP, preproc, lang, f, fnm);

		if (verbose == 1)
		{	printf("%s\n", buf);
		}

		if (system(buf) < 0)		// gcc -E
		{	perror("add_file");
			efree(buf);
			unlink(fnm);
			return 0;
		}
		efree(buf);

		if ((Px.lex_yyin = open(fnm, O_RDONLY)) < 0)
		{	fprintf(stderr, "cannot open '%s'\n", fnm);
			unlink(fnm);
			return 0;
		}
	}

	imbalance = process(cid);
	assert(Px.lex_pcnt == 0);

	close(Px.lex_yyin);

	if (strlen(fnm) > 0)
	{	unlink(fnm);
	}

	if (tfd)
	{	unlink(f);	// read_stdin, tmp filename
		return lncnt;
	}

	remember(f, imbalance, cid);	// when the whole file is processed
	// covers only files specified on the command-line
	// not include files, where the real redundancy is

	return 1;
}

void
set_ranges(Prim *a, Prim *b)
{	static short tokmaxrange = 0;
	int i, j, span = count/Ncore;
	Prim *x;

	if (verbose == 1 && count == 0)
	{	fprintf(stderr, "cobra: no input?\n");
	}

	if (verbose > 1 && !cobra_texpr)
	{	printf("span %d mult %d < %d\n",
			span, span*Ncore, count);
	}

	ini_par();

	if (!tokrange || Ncore > tokmaxrange)
	{	tokrange = (TokRange **) emalloc(Ncore * sizeof(TokRange *), 18);
		if (Ncore > tokmaxrange)
		{	tokmaxrange = Ncore;
	}	}

	for (i = 0, x = a; i < Ncore; i++)
	{	if (!tokrange[i])
		{	tokrange[i] = (TokRange *) emalloc(sizeof(TokRange), 19);
		}
		tokrange[i]->seq = i;
		tokrange[i]->from = x;
		for (j = 0; x && j < span; j++)
		{	x = x->nxt;
		}
		if (!x)
		{	tokrange[i]->upto = b;
	//		assert(i == Ncore-1);
		} else
		{	tokrange[i]->upto = x->prv;
		}
	}
	tokrange[Ncore-1]->upto = b;

	if (!a || !b || cobra_texpr)
	{	return;
	}

	if (Ncore > 1)
	{	for (i = 0; verbose == 1 && i < Ncore; i++)
		{	printf("set %d: %d-%d <%d> (%s:%d - %s:%d)\n",
				i, tokrange[i]->from->seq, tokrange[i]->upto->seq,
				(int)(tokrange[i]->upto->seq - tokrange[i]->from->seq),
				tokrange[i]->from->fnm, tokrange[i]->from->lnr,
				tokrange[i]->upto->fnm, tokrange[i]->upto->lnr);
	}	}
}

int
add_stream(Prim *pt)
{	static short l_bracket;	// initially 0
	static short l_curly;
	static short l_roundb;
	Prim *notnew = plst;
	int correct = textmode?0:1;

	if (verbose > 1)
	{	if (plst)
		{	printf("Add Stream %d -- %d :: %s\n", prim->lnr, plst->lnr, plst->txt);
			if (prim && pt)
			{	printf("pre_: lnr: %d seq: %d -- lnr: %d seq: %d\n",
					pt->lnr - prim->lnr,
					pt->seq - prim->seq,
					plst->lnr - pt->lnr,
					plst->seq - pt->seq);
			}
		} else
		{	printf("Add Stream\n");
	}	}

	if (pt && plst && prim)	// can free tokens between prim and pt
	{	recycle_token(prim, pt);
		prim = pt;
	}

	ini_pre(0);

	pre[0].lex_bracket = l_bracket;
	pre[0].lex_curly   = l_curly;
	pre[0].lex_roundb  = l_roundb;

	if (!add_file("", 0, plst?(plst->lnr+correct):1))
	{	if (0)
		{	printf("	Nothing to add\n");
		}
		return 0;
	}

	l_bracket = pre[0].lex_bracket;
	l_curly   = pre[0].lex_curly;
	l_roundb  = pre[0].lex_roundb;

	if (pre[0].lex_plst)
	{	post_process(notnew?0:1);
		set_ranges(prim, plst);
	}

	if (verbose > 1)
	{	if (plst)
		{	printf("Done Add Stream %d -- %d :: %s\n", prim->lnr, plst->lnr, plst->txt);
			if (prim && pt)
			{	printf("post: lnr: %d seq: %d -- lnr: %d seq: %d\n",
					pt->lnr - prim->lnr,
					pt->seq - prim->seq,
					plst->lnr - pt->lnr,
					plst->seq - pt->seq);
			}
		} else
		{	printf("Done Add Stream\n");
	}	}

	return 1;
}

static void
Nthreads(char *s)
{
	if (!isdigit((uchar) *s))
	{	fprintf(stderr, "error: usage -N[0-9]+\n");
		exit(1);
	}
	Ncore = atoi(s);
	if (Ncore < 1)
	{	fprintf(stderr, "error: usage -N[0-9]+\n");
		exit(1);
	}
}

static inline int
s_hash(const char *p)
{	int h = 0;
	do {	h = (h<<1) ^ (*p++);
	} while (*p != '\0');
	return h;
}

static void
record_typename(Prim *c, int cid)	// single-core
{	// if we see p again, change its typ to "type"
	Typedef *t;
	Stack   *n;
	char	*p     = c->txt;
	char	 level = c->curly;
	int	 nah;

	n = (Stack *) hmalloc(sizeof(Stack), cid, 125);
	n->nm = p;
	n->na = s_hash(p);
	nah = n->na & (NAHASH-1);

	assert(cid >= 0 && cid < Ncore);
	if (Px.lex_tps[nah]
	&&  Px.lex_tps[nah]->level == level)
	{	n->nxt = Px.lex_tps[nah]->lst;
		Px.lex_tps[nah]->lst = n;
	} else
	{	t = (Typedef *) hmalloc(sizeof(Typedef), cid, 126);
		t->level = level;
		t->lst = n;
		t->up = Px.lex_tps[nah];
		Px.lex_tps[nah] = t;
	}
}

static int
is_typename(Prim *c, int cid)
{	Typedef	*t;
	Stack	*s;
	int h = s_hash(c->txt);
	int nah = h & (NAHASH-1);

	assert(cid >= 0 && cid < Ncore);
	for (t = Px.lex_tps[nah]; t; t = t->up)
	{	for (s = t->lst; s; s = s->nxt)
		{	if (s->na == h
			&&  strcmp(s->nm, c->txt) == 0)
			{	if (0)
				{	printf("%s:%d: typedefname %s\n",
						c->fnm, c->lnr, c->txt);
				}
				return 1;
	}	}	}
#if 1
	// catch a few common cases, where we may not have seen the typedef
	// e.g. because of multi-core splits
	if (c->txt[0] == 'u')
	{	if (strcmp(&(c->txt[1]), "8") == 0
		||  strcmp(&(c->txt[1]), "16") == 0
		||  strcmp(&(c->txt[1]), "32") == 0
		||  strcmp(&(c->txt[1]), "64") == 0)
		{	return 1;
	}	}
#endif
	return 0;
}

static void
scope_up(int level, int cid)	// single-core
{	int nah;

	assert(cid >= 0 && cid < Ncore);
	for (nah = 0; nah < NAHASH; nah++)
	{	while (Px.lex_tps[nah]
		&&     Px.lex_tps[nah]->level > level)
		{	// could recycle lex_tps and lex_tps->lst
			Px.lex_tps[nah] = Px.lex_tps[nah]->up;
	}	}
}

static void
typedefs(int cid)
{	Prim *p; // recognizes basic typedefed names
	Prim *c;

	assert(cid >= 0 && cid < Ncore);
	for (c = Px.lex_prim; c != Px.lex_plst; c = c->nxt)
	{	if (strcmp(c->txt, "}") == 0)
		{	scope_up(c->curly, cid);
			continue;
		}
		if (strcmp(c->typ, "ident") == 0
		&&  is_typename(c, cid))
		{	c->typ = "type";
		}
		if (strcmp(c->txt, "typedef") == 0)
		{	// check if single and non-array
			c = c->nxt;
			for (p = c; p; p = p->nxt)
			{	if (p->curly > c->curly)
				{	continue;
				}
				if (strcmp(p->txt, ";") == 0
				||  strcmp(p->txt, ",") == 0
				||  strcmp(p->txt, "[") == 0)
				{	break;
			}	}
			if (!p || strcmp(p->txt, ";") != 0)
			{	continue;
			}
			p = p->prv;	// the last identifier
			if (strcmp(p->typ, "ident") != 0)
			{	continue;
			}
			record_typename(p, cid); // at this scope level
			if (0)
			{	printf("%s == ", p->txt);
				for (; c; c = c->nxt)
				{	if (c == p)
					{	break;
					}
					printf(" %s", c->txt);
				}
				printf("\n");
	}	}	}
}

static void
set_par(const char *varname, const char *value)	// single-core
{	ArgList *a;

	a     = (ArgList *) emalloc(sizeof(ArgList), 20);
	a->nm =    (char *) emalloc(strlen(varname)+1, 21);
	strcpy(a->nm, varname);
	a->s  =    (char *) emalloc (strlen(value)+1, 22);
	strcpy(a->s, value);
	a->nxt = cl_var;
	cl_var = a;
}

static int
usage(char *s)
{
	fprintf(stderr, "%s %s\n", progname, tool_version);
	fprintf(stderr, "unrecognized option -'%s'\n", s);
	fprintf(stderr, "usage: %s [-option]* [file]*\n", progname);
	fprintf(stderr, "\t-Ada                -- recognize Ada keywords (implies -nocpp)\n");
	fprintf(stderr, "\t-allheaderfiles     -- process all header files (also system headers)\n");
	fprintf(stderr, "\t-C++                -- recognize C++ keywords\n");
	fprintf(stderr, "\t-c \"commands\"       -- execute commands and stop (cf -e, -f)\n");
	fprintf(stderr, "\t-c \"m /regex; p\"     -- find tokens matching a regular expr\n");
	fprintf(stderr, "\t-comments           -- do not truncate c-style comments at first newline\n");
	fprintf(stderr, "\t-configure dir      -- set and remember the name for the cobra rules directory\n");
	fprintf(stderr, "\t-cpp                -- enable C preprocessing%s\n", no_cpp?"":" (default)");
	fprintf(stderr, "\t-d and -v -d        -- debug cobra inline program executions\n");
	fprintf(stderr, "\t-eol                -- treat newlines as EOL tokens\n");
	fprintf(stderr, "\t-e name             -- (or -expr or -regex) print lines with tokens matching name\n");
	fprintf(stderr, "\t-e \"token_expr\"     -- print lines matching a token_expr (cf -view)\n");
	fprintf(stderr, "\t                       use meta-symbols: ( | ) . * + ?\n");
	fprintf(stderr, "\t                       use ^name for not-matching name\n");
	fprintf(stderr, "\t                       var-binding:   name:@ident\n");
	fprintf(stderr, "\t                       var-reference: :name\n");
	fprintf(stderr, "\t                       see also -pe\n");
	fprintf(stderr, "\t-f file             -- execute commands from file and stop (cf -view)\n");
	fprintf(stderr, "\t-Idir, -Dstr, -Ustr -- preprocessing directives\n");
	fprintf(stderr, "\t-Java               -- recognize Java keywords\n");
	fprintf(stderr, "\t-lib                -- list available predefined cobra -f checks\n");
	fprintf(stderr, "\t-m or -macros       -- parse text of macros (implies -nocpp)\n");
	fprintf(stderr, "\t-n or -nocpp        -- do not do any C preprocessing%s\n", !no_cpp?"":" (default)");
	fprintf(stderr, "\t-noqualifiers       -- do not tag qualifiers\n");
	fprintf(stderr, "\t-noheaderfiles      -- do not process header files\n");
	fprintf(stderr, "\t-nostream           -- do not enable default input streaming when reading from stdin\n");
	fprintf(stderr, "\t-notypedefs         -- do not process typedefs\n");
	fprintf(stderr, "\t-notypes            -- do not tag type names\n");
	fprintf(stderr, "\t-Nn                 -- use n threads\n");
	fprintf(stderr, "\t-pattern \"tokens\"   -- (or -pat) like -expr but simplified: (|)+? are regular symbols\n");
	fprintf(stderr, "\t-pe \"tokens\"        -- same as -pattern: pattern expression matching\n");
	fprintf(stderr, "\t                       * and ] are meta-symbols unless preceded by a space\n");
	fprintf(stderr, "\t                       [ is a meta-symbol unless followed by a space\n");
	fprintf(stderr, "\t                       a tokenpattern preceded by / is a regular expr, escape with \\\n");
	fprintf(stderr, "\t-preserve           -- preserve temporary files in /tmp\n");
	fprintf(stderr, "\t-Python             -- recognize Python keywords\n");
	fprintf(stderr, "\t-quiet              -- do not print script commands executed or nr of matches\n");
	fprintf(stderr, "\t-regex \"expr\"       -- see -e\n");
	fprintf(stderr, "\t-runtimes           -- report runtimes of commands executed, if >1s\n");
	fprintf(stderr, "\t-scrub              -- produce output in scrub-format\n");
	fprintf(stderr, "\t-stream N           -- set stdin stream buffer-limit to N bytes (default %d)\n", stream_lim);
	fprintf(stderr, "\t-stream_margin N    -- set stdin window margin to N tokens (default %d)\n", stream_margin);
	fprintf(stderr, "\t-terse              -- disable output from d, l, and p commands, implies -quiet\n");
	fprintf(stderr, "\t-text               -- no token types, just text-strings and symbols\n");
	fprintf(stderr, "\t-tok                -- only tokenize the input\n");
	fprintf(stderr, "\t-version            -- print version number and exit\n");
	fprintf(stderr, "\t-v                  -- more verbose\n");
	fprintf(stderr, "\t-view -f file       -- show dot-graph of DFA(s) of inline program(s)\n");
	fprintf(stderr, "\t-view -e \"token_expr\" -- show dot-graph of NDFA for expr\n");
	fprintf(stderr, "\t-V                  -- print version number and exit\n");
	fprintf(stderr, "\t-var name=value     -- set name in def-script to value (cf. -f)\n");
	fprintf(stderr, "all arguments starting with -- are passed to standalone backends, e.g. --debug\n");
	return 1;
}

void
list_checkers(void)	// also used in cobra_lib.c
{	char buf[1024];

	printf("predefined checks include:\n");
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "ls %s/main > ._cobra_tmp_", C_BASE);
	if (system(buf) >= 0)		// list checkers
	{	snprintf(buf, sizeof(buf), "sort ._cobra_tmp_ | grep -v -e .def$ | sed 's;^;  ;'");
		if (system(buf) < 0)	// list checkers
		{	perror("lib");
	}	}
	unlink("._cobra_tmp_");
}

static char *
get_work(int cid)
{	char *s;

	do_lock(cid);
	s = get_file();
	do_unlock(cid);
	return s;
}

static void *
one_core(void *arg)
{	int  *i, cid;
	char *s;

	i = (int *) arg;
	cid = *i;

	while ((s = get_work(cid)) != NULL)
	{	add_file(s, cid, 1);
	}

	if (handle_typedefs)
	{	// printf("thread%d: typedefs\n", cid);
		typedefs(cid);
	}

	return NULL;
}

static void *
renumber(void *arg)
{	int *i, cid, start;
	Prim *c;

	i = (int *) arg;
	cid = *i;
	start = pass_arg[cid].start_nr;

	for (c = Px.lex_prim; c; c = c->nxt)
	{	c->seq = start++;
		if (c == Px.lex_plst)
		{	break;
	}	}

	return NULL;
}

static void
ini_files(int argc, char *argv[])
{
	if (verbose)
	{	printf("%d files, using %d cores\n", argc, Ncore);
	}
	argv++;
	while (--argc > 0)
	{	rem_file(*argv++);
	}
}

void
ini_par(void)
{	static short t_idmax = 0;
	static short passmax = 0;
	static short premax = 0;

	if (!t_id || Ncore > t_idmax)
	{	t_id = (pthread_t *) emalloc(Ncore * sizeof(pthread_t), 23);
		if (Ncore > t_idmax)
		{	t_idmax = Ncore;
	}	}

	if (!pass_arg || Ncore > passmax)
	{	pass_arg =  (Pass *) emalloc(Ncore * sizeof(Pass), 24);
		if (Ncore > passmax)
		{	passmax = Ncore;
	}	}

	if (!pre || Ncore > premax)
	{	pre = (Pre *) emalloc(Ncore * sizeof(Pre), 25);
		if (Ncore > premax)
		{	premax = Ncore;
	}	}
}

void
par_scan(void)
{	int i, cid, n_s;

	ini_par();

	if (verbose>1) printf("parse\n");
	for (i = 0; i < Ncore; i++)
	{	pass_arg[i].who = i;
		start_timer(i);
		(void) pthread_create(&t_id[i], 0, one_core,
			(void *) &(pass_arg[i].who));
	}
	for (i = 0; i < Ncore; i++)
	{	(void) pthread_join(t_id[i], 0);
		stop_timer(i, 0, "E");
	}

	if (verbose>1) printf("renumber\n");
	for (cid = n_s = 0; cid < Ncore; cid++)
	{	if (verbose>1) printf("%d	%d\n", cid, n_s);
		pass_arg[cid].start_nr = n_s;
		n_s += (Px.lex_plst)?Px.lex_plst->seq + 1:0;
	}
	// printf("total:	%d\n", n_s);

	for (i = 1; i < Ncore; i++)
	{	pass_arg[i].who = i;
	//	start_timer(i);
		(void) pthread_create(&t_id[i], 0, renumber,
			(void *) &(pass_arg[i].who));
	}
	for (i = 1; i < Ncore; i++)
	{	(void) pthread_join(t_id[i], 0);
	//	stop_timer(i, 0, "F");
	}
}

static void
seq_scan(int argc, char *argv[])
{	int i;

	if (verbose>1) printf("parse\n");
	start_timer(0);
	for (i = 1; i < argc; i++)
	{	add_file(argv[i], 0, 1);
	}
	stop_timer(0, 0, "G");
	start_timer(0);
	if (verbose>1) printf("typedefs\n");
	if (handle_typedefs)
	{	typedefs(0);
	}
	stop_timer(0, 0, "H");
}

#if 0
 when using -pattern instead of -e
 meta-symbols: ( | ) + ? are not used
 escape rules for the remaining meta-symbols: . * ? [ ] ^ and :
	
	: is : iff preceded and followed by space, else name binding
	[ is [ iff followed by a space, else range
	] is ] iff preceded by a space, else range
	* is * iff preceded by a space, else Kleene star
	. is . iff preceded by \
	^ is ^ iff preceded by \

 to convert to a standard regex we need to do these mappings:
	( -> \(
	) -> \)
	| -> \|
	+ -> \+
	? -> \?
	[ space -> \[ space
	space ] -> space \]
	space * -> space \*
#endif

char *
unquoted(char *p)
{	int len = strlen(p);

	if ((*p == '\'' || *p == '\"')
	&&   p[len-1] == *p)
	{	p[len-1] = '\0';
		p++;
	}
	return p;
}

char *
pattern(char *p)
{	char *n = (char *) emalloc(2*strlen(p)+1, 26);
	char *m = n;
	char *q = p;
	int len = strlen(p);
	int inrange = 0;

	// first check if the pattern is quoted:
	// e.g. when typed inline, and remove quotes

	if ((*p == '\'' || *p == '\"')
	&&   p[len-1] == *p)
	{	p[len-1] = '\0';
		p++;
	}

	while (len > 0)
	{	switch (*p) {
		case '\\':
			*n++ = *p++;
			*n++ = *p++;
			len -= 2;
			break;
		case '(':
		case ')':
		case '|':
		case '+':
		case '?':
			if (!inrange)
			{	*n++ = '\\';
			}
			*n++ = *p++;
			len--;
			break;
		case ']':
		case '*':
			if (p == q || *(p-1) == ' ')
			{	*n++ = '\\';
			} else if (*p == ']')
			{	inrange--;
			}
			*n++ = *p++;
			len--;
			break;
		case '[':
			if (*(p+1) == ' ')
			{	*n++ = '\\';
			} else
			{	inrange++;
			}
			// fall through
		default:
			*n++ = *p++;
			len--;
			break;
		}
	}
	// printf("inp: %s\nout: %s\n", q, m); exit(1);
	return m;
}

#if 0

#ifndef NMax
 #define NMax 4	// likely optimal current speedup
#endif

static void
autosetcores(void)
{	FILE *fd;
	char buf[1024];

	if ((fd = fopen("/proc/cpuinfo", "r")) == NULL)
	{	return;
	}

	while (fgets(buf, sizeof(buf), fd))
	{	if (strncmp(buf, "processor", strlen("processor")) == 0)
		{	Ncore++;
	}	}
	fclose(fd);

	if (Ncore > 1)
	{	Ncore--;	// we started with Ncore==1
		if (Ncore > NMax)	
		{	Ncore = NMax;
	}	}
}
#endif

int
set_base(void)
{	FILE *fd;
	char *h;
	char *f;
	char buf[1024];
	int   n;

	if ((h = getenv("C_TMP")) != NULL)	// eg on older cygwin: C_TMP=C:/cygwin
	{	C_TMP = (char *) emalloc(strlen(h)+1, 27);
		strcpy(C_TMP, h);
		if (verbose)
		{	printf("cobra: using env variable $C_TMP=%s\n", C_TMP);
	}	}

	if ((h = getenv("C_BASE")) != NULL)	// env setting wins
	{	C_BASE = (char *) emalloc(strlen(h)+1, 28);
		strcpy(C_BASE, h);
		if (verbose)
		{	printf("cobra: using env variable $C_BASE=%s\n", C_BASE);
		}
		return (strlen(C_BASE) > 1);
	}

	if ((h = getenv("HOME")) == NULL)
	{	return 0;
	}
	n = strlen(h) + strlen("/.cobra") + 1;
	f = (char *) emalloc(n*sizeof(char), 29);
	snprintf(f, n, "%s/.cobra", h);
	if ((fd = fopen(f, "r")) == NULL)
	{	return 0;
	}
	while (fgets(buf, sizeof(buf), fd) != NULL)
	{	if ((h = strrchr(buf, '\n')) != NULL)
		{	*h = '\0';
		}
		if ((h = strrchr(buf, '\r')) != NULL)
		{	*h = '\0';
		}
		if (strncmp(buf, "Rules:", 6) == 0)
		{	h = &buf[6];
			while (is_blank((uchar)*h))
			{	h++;
			}
			C_BASE = (char *) emalloc(strlen(h)+1, 30);
			strcpy(C_BASE, h);
		}
		if (strncmp(buf, "ncore:", 6) == 0)
		{	h = &buf[6];
			while (is_blank((uchar)*h))
			{	h++;
			}
			Ncore = atoi(h);
	}	}
	fclose(fd);

	return (strlen(C_BASE) > 1);
}

int
do_configure(const char *s)
{	FILE *fd;
	char *h;
	char *f;

	f = (char *) emalloc(strlen(s) + strlen("/basic.cobra") + 1, 31);
	sprintf(f, "%s/basic.cobra", s);

	if ((fd = fopen(s, "r")) == NULL)
	{	fprintf(stderr, "cobra: cannot find '%s/basic.cobra'\n", s);
F:		fprintf(stderr, "cobra: configuration failed\n");
		return 1;
	}
	fclose(fd);

	if ((h = getenv("HOME")) == NULL)
	{	fprintf(stderr, "cobra: env variable $HOME not set\n");
		goto F;
	}
	f = (char *) emalloc(strlen(h) + strlen("/.cobra") + 1, 32);
	sprintf(f, "%s/.cobra", h);

	if ((fd = fopen(f, "w")) == NULL)
	{	perror("fopen");
		fprintf(stderr, "cobra: cannot create ~/.cobra\n");
		goto F;
	}
	fprintf(fd, "Rules: %s/rules\n", s);
	fprintf(fd, "# ncore: 1\n");
	fclose(fd);
	fprintf(stderr, "cobra: configuration completed\n");
	return 0;
}

int
main(int argc, char *argv[])
{	int view = 0;

	progname = argv[0];
//	autosetcores();	// default nr cores to use

	while (argc > 1 && argv[1][0] == '-')
	{	switch (argv[1][1]) {
		case 'A':
			  if (strcmp(argv[1], "-Ada") == 0)
			  {	ada = no_cpp = 1;
				break;
			  }
			  return usage(argv[1]);

		case 'a':
			  if (strcmp(argv[1], "-allheaderfiles") == 0)
			  {	all_headers = 1;
			  	no_cpp = with_comments = 0;
				break;
			  }
			  return usage(argv[1]);

		case 'C':
			  if (strcmp(argv[1], "-C++") == 0)
			  {	cplusplus = 1;
				CPP = "g++";
				lang = "c++";
				break;
			  }
			  return usage(argv[1]);

		case 'c': if (strcmp(argv[1], "-cpp") == 0)
			  {	no_cpp = with_comments = 0;
				break;
			  }
			  if (strcmp(argv[1], "-comments") == 0)
			  {	full_comments = 1;
				break;
			  }
			  if (strcmp(argv[1], "-configure") == 0)
			  {	return do_configure(argv[2]);
			  }
			  if (strncmp(argv[1], "-cwe", 4) == 0
			  &&  strstr(progname, "cwe") != NULL)
			  {	argv[1]++; // remove hyphen
				goto cwe_mode;
			  }
			  // -c commands
			  cobra_commands = argv[2];
			  no_match = 1;
			  argc--; argv++;
			  break;

		case 'd': if (verbose)
			  {	p_debug = 2;
			  } else
			  {	p_debug = 1;
			  }
			  break;

		case 'D': add_preproc(argv[1]);
			  break;

		case 'e':
			  if (strcmp(argv[1], "-eol") == 0)
			  {	eol = 1;
				break;
			  }
RegEx:			  no_match = 1;		// -expr or -regex
			  cobra_texpr = argv[2];
			  argc--; argv++;
			  if (view)
			  {	p_debug = 5;
			  }
			  break;

		case 'f': cobra_target = argv[2];
			  argc--; argv++;
			  no_match = 1;	// quiet mode
			  if (view)
			  {	if (verbose)
				{	p_debug = 3;
				} else
				{	p_debug = 4;
			  }	}
			  break;

		case 'g': gui = 1;
			  break;

		case 'I': add_preproc(argv[1]);
			  break;

		case 'J':
			  if (strcmp(argv[1], "-Java") == 0)
			  {	java = 1;
				break;
			  }
			  return usage(argv[1]);

		case 'l':	// lib or list
			  if (set_base())
			  {	list_checkers();
			  } else
			  {	printf("error: check tool installation\n");
			  }
			  return 0;

		case 'm': parse_macros = no_cpp = 1;
			  break;

		case 'n': if (strcmp(argv[1], "-notypes") == 0)
			  {	with_type = 0;
			  	break;
			  }
			  if (strcmp(argv[1], "-noqualifiers") == 0)
			  {	with_qual = 0;
			  	break;
			  }
			  if (strcmp(argv[1], "-noheaderfiles") == 0)
			  {	no_headers = 1;
			  	break;
			  }
			  if (strcmp(argv[1], "-nostream") == 0)
			  {	stream = -1;
			  	break;
			  }
			  if (strcmp(argv[1], "-notypedefs") == 0)
			  {	handle_typedefs = 0;
			  	break;
			  }
			  no_cpp = with_comments = 1;	// -nocpp
			  no_cpp_sticks = 1;
			  break;

		case 'z': no_headers = 1; // deprecated, backward compatibility
			  break;

		case 'N': Nthreads(&argv[1][2]);
			  break;

		case 'p': if (strncmp(argv[1], "-pre", 4) == 0)	// preserve
			  {	preserve = 1;
				break;
			  }
			  if (strncmp(argv[1], "-pat", 4) == 0
			  ||  strncmp(argv[1], "-pe", 3) == 0)	// pattern expression
			  {	no_match = 1;
				if (argc > 2)
				{	cobra_texpr = pattern(argv[2]);
					argc--; argv++;
					if (view)
					{	p_debug = 5;
						(void) set_base();
					}
					break;
			  }	}
			  return usage(argv[1]);

		case 'P': if (strcmp(argv[1], "-Python") == 0)
			  {	python = no_cpp = 1;
				break;
			  }
			  return usage(argv[1]);

		case 'q': if (strcmp(argv[1], "-quiet") == 0)
			  {	no_match = 1;
				break;
			  }
			  return usage(argv[1]);

		case 'r': if (strcmp(argv[1], "-runtimes") == 0)
			  {	runtimes++;
				break;
			  }
			  if (strcmp(argv[1], "-regex") == 0)
			  {	goto RegEx;
			  }
			  return usage(argv[1]);

		case 's': if (strcmp(argv[1], "-scrub") == 0)
			  {	scrub = 1;
				break;
			  }
			  if (strcmp(argv[1], "-stream") == 0)
			  {	argc--; argv++;
				if (isdigit((uchar) argv[1][0]))
				{	stream_lim = atoi(argv[1]);
					printf("cobra: stream limit: %d lines\n", stream_lim);
					break;
			  	}
			  } else if (strcmp(argv[1], "-stream_margin") == 0)
			  {	argc--; argv++;
				if (isdigit((uchar) argv[1][0]))
				{	stream_margin = atoi(argv[1]);
					printf("cobra: stream margin: %d lines\n", stream_margin);
					break;
			  	}
			  }
			  return usage(argv[1]);

		case 't': if (strcmp(argv[1], "-tok") == 0)
			  {	Ctok = 1;
				break;
			  }
			  if (strcmp(argv[1], "-terse") == 0)
			  {	no_display = 1;
				no_match = 1;
				break;
			  }
			  if (strcmp(argv[1], "-text") == 0)
			  {	textmode = 1;
				handle_typedefs = 0;
				break;
			  }
			  return usage(argv[1]);

		case 'U': add_preproc(argv[1]);
			  break;

		case 'v': if (strcmp(argv[1], "-version") == 0)
			  {	fprintf(stderr, "%s\n", tool_version);
				return 0;
			  }
			  if (strcmp(argv[1], "-var") == 0)
			  {	char *p;
				argc--;
				argv++;
				p = strchr(argv[1], '=');
				if (!p)
				{	return usage(argv[1]);
				}
				*p++ = '\0';
				set_par(argv[1], p);
			  } else if (strcmp(argv[1], "-view") == 0)
			  {	view = 1;
				preserve = 1;
			  } else
			  {	verbose++;
			  }
			  break;

		case 'V': printf("%s\n", tool_version);
			  return 0;

		case '-': // -- arguments are passed to backend, if any
			  if (strlen(argv[1]) > 2)
			  {	int n = strlen(backend) + strlen(&argv[1][2]) + 2;
				char *z = (char *) emalloc(n*sizeof(char), 33);
				sprintf(z, "%s%s ", backend, &argv[1][2]);
				backend = z; // no need to free the old ptr
				break;
			  } // else fall thru

		default:
			return usage(argv[1]);
		}
		argv++;
		argc--;
	}

	if (!set_base())
	{	printf("error: cannot open ~/.cobra : check tool installation\n");
	}

	if (strstr(progname, "taint") != NULL) // shouldn't be hardcoded
	{	handle_typedefs = 0;
	}

	if (strstr(progname, "cwe") != NULL)
	{
cwe_mode:	no_match = 1;	// for consistency with -f
		if (cobra_target)
		{	int n = strlen(cobra_target)+2;
			cwe_args = (char *) emalloc(n*sizeof(char), 34);
			snprintf(cwe_args, n, "%s ", cobra_target); // add space
		}

		while (argc > 1
		&&     strchr(argv[1], '.') == NULL)	// not a filename arg
		{	int n = strlen(argv[1]) + 2; // plus space and null byte
			char *foo = cwe_args;
			// keep a space at the end of each arg
			if (!cwe_args)
			{	cwe_args = (char *) emalloc(n*sizeof(char), 35);
				snprintf(cwe_args, n, "%s ", argv[1]);
			} else
			{	n += strlen(cwe_args) + 1;
				cwe_args = (char *) emalloc(n*sizeof(char), 36);
				snprintf(cwe_args, n, "%s%s ", foo, argv[1]);
			}
			argc--; argv++;
	}	}

	if (cobra_target)
	{	if ((strstr(cobra_target, ".c") != NULL
		&&   strstr(cobra_target, ".cobra") == NULL)
		||  strstr(cobra_target, ".h") != NULL)
		{	printf("error: bad filename suffix for: -f %s\n",
				cobra_target);
			cobra_target = (char *) 0;
	}	}

	if (strlen(preproc) > 0 && no_cpp)
	{	fprintf(stderr, "warning: ignoring '%s'\n", preproc);
	}

	umask(022);
	prep_pre();
	ini_timers();
	ini_lock();

	if (Ncore > 1 && argc > 2)	// at least 2 files and cores
	{	ini_files(argc, argv);
		par_scan();
		clr_files();
	} else
	{	seq_scan(argc, argv);
	}

	if (argc == 1)
	{	read_stdin = 1;
		if (stream == 0) // no override with -nostream
		{	stream = 1;
	}	}

	if (stream == 1
	&& (!(cobra_texpr || cobra_target || cobra_commands || Ctok)
	||  Ncore != 1
	||  !no_cpp))
	{	if (strstr(progname, "cobra") != NULL)
		{	fprintf(stderr, "%s: error: streaming input\n", progname);
			fprintf(stderr, "    requires -expr, -pat or -f, no -cpp, and ncore=1\n");
			return 1;
	}	}

	if (read_stdin)
	{	if (no_cpp)
		{	fprintf(stderr, "%s: reading stdin\n", progname);
			add_file("", 0, 1);	// keep single-core
			if (stream == 1 && Ctok)
			{	Prim *rp;
				do {	rp = plst;
					if (!add_file("", 0, pre[0].lex_lineno))
					{	break;
					}
				} while (!rp || plst->seq > rp->seq);
			}
		} else
		{	fprintf(stderr, "%s: error: cannot use -cpp when reading stdin\n",
				progname);
			return 1;
	}	}

	if (Ctok)
	{	return 0;
	}
	post_process(1);	// collect info from cores
	cobra_main();		// cobra and cobra_checkers

	return 0;
}

char *
anypp(void)
{
	return preproc;
}

void
do_typedefs(int cid)
{
	if (handle_typedefs)
	{	typedefs(cid);
	}
}

#if 0
int
yywrap(void)
{
	return 1;
}

void
yywarn(char *s)
{
	if (verbose == 1)
	{	fprintf(stderr, "%s:%d: warning - %s [%s]\n",
			Px.lex_fname, Px.lex_lineno, s, lex_buf);
	}
}

int
yyerr(char *s)
{
	fprintf(stderr,"%s:%d %s (%s)\n",
		Px.lex_fname, Px.lex_lineno, s, lex_yytext);

	return 0;
}
#endif
