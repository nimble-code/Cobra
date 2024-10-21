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
#include <strings.h>
#ifdef _WINDOWS
 #define strcasecmp	stricmp
#endif

#ifdef PC
 #include <time.h>
#else
 #include <sys/times.h>
#endif

#define check_argc(n, s)	if (argc <= n) \
				{ fprintf(stderr, "error: missing argument for '%s'\n", s); \
				  return usage(argv[1]); \
				}

int ada;
int across_file_match;
int all_headers;
int cplusplus;
int Ctok;
int echo;
int eol;
int eof = 1;	// new default v 3.9
int gui;
int has_suppress_tags;
int html;
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
int pruneifzero;
int python;
int read_stdin;
int stream_lim = 100000;
int stream_margin = 100;
int stream_margin_set;
int stream_override;
int runtimes;
int scrub;
int showprog;
int verbose;
int view;

unsigned long MaxMemory = 24000;	// set default max to 24 GB, override with -MaxMem N

ArgList		*cl_var;	// -var name=xxx, cobra_lib.c
pthread_t	*t_id;

char	*C_BASE  = ".";		// reassigned in main
char	*C_TMP = "";		// reassigned in main
char	*cobra_target;		// -f xxx
char	*cobra_commands;	// -c xxx
char	*cwe_args;		// standalone cwe checker
char	*progname = "";		// e.g. cobra or cwe
char	*backend = "";		// args passed to backend with --arg

TokRange  **tokrange;

static Pass	*pass_arg;
static char	*CPP       = "gcc";
static char	*lang      = "c";
static char	*file_list;	// file with filenames to process
static char	**Preproc;
static int	 handle_typedefs = 1;
static char	*recursive;	// use find to populate file_list
static char	*seedfile;
static int	 textmode;
static char	 tmpf[32];
static int	 with_qual = 1;
static int	 with_type = 1;
static int	 N_max = 128;

extern Prim	*prim, *plst;
extern int	 json_plus;

extern void	 set_ranges(Prim *, Prim *, int);
extern void	 recycle_token(Prim *, Prim *);
extern void	 add_eof(int);

extern void	 update_last_token(const char *, Prim *);
extern void	 update_first_token(const char *, Prim *);

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
		if (eof != 0)
		{	add_eof(cid);
		}
		return sanitycheck(cid);
	}
	return 0;
}

static void
add_preproc(const char *s)	// before parsing
{
	// initial add is to Preproc[0], which is
	// later copied to the other elements

	if (strlen(Preproc[0]) == 0)
	{	Preproc[0] = (char *) emalloc(strlen(s) + 1, 16);
		strcpy(Preproc[0], s);
		no_cpp = 0;
	} else
	{	char *op = Preproc[0]; // emalloc uses sbrk
		int    n = strlen(op) + strlen(s) + 2;
		Preproc[0] = emalloc(n*sizeof(char), 17);
		snprintf(Preproc[0], n, "%s %s", op, s);
	}
}

static Prim *cmnt_head;
static Prim *cmnt_tail;
static Prim *s_prim = 0;
static Prim *s_plst = 0;

static char *Tname[] = { // all except <!DOCTYPE and comments <!--
	"a",		"abbr",		"acronym",
	"address",	"applet",	"area",
	"article",	"aside",	"audio",
	"b",		"base",		"basefont",
	"bdi",		"bdo",		"big",
	"blockquote",	"body",		"br",
	"button",	"canvas",	"caption",
	"center",	"cite",		"code",
	"col",		"colgroup",	"data",
	"datalist",	"dd",		"del",
	"details",	"dfn",		"dialog",
	"dir",		"div",		"dl",
	"dt",		"em",		"embed",
	"fieldset",	"figcaption",	"figure",
	"font",		"footer",	"form",
	"frame",	"frameset",	"h1",
	"h2",		"h3",		"h4",
	"h5",		"h6",		"head",
	"header",	"hgroup",	"hr",
	"html",		"i",		"iframe",
	"img",		"input",	"ins",
	"kbd",		"label",	"legend",
	"li",		"link",		"main",
	"map",		"mark",		"menu",
	"meta",		"meter",	"nav",
	"noframes",	"noscript",	"object",
	"ol",		"optgroup",	"option",
	"output",	"p",		"param",
	"picture",	"pre",		"progress",
	"q",		"rp",		"rt",
	"ruby",		"s",		"samp",
	"script",	"search",	"section",
	"select",	"small",	"source",
	"span",		"strike",	"strong",
	"style",	"sub",		"summary",
	"sup",		"svg",		"table",
	"tbody",	"td",		"template",
	"textarea",	"tfoot",	"th",
	"thead",	"time",		"title",
	"tr",		"track",	"tt",
	"u",		"ul",		"var",
	"video",	"wbr",
	0
};

static char *Selfclosing[] = {
	"area",
	"br",
	"col",
	"embed",
	"hr",
	"img",
	"input",
	"link",
	"meta",
	"source",
	"track",
	"wbr",
	0
};

static char *NestingTags[] = {	// likely needs more entries
	"b",
	"cite",
	"dfn",
	"em",
	"font",
	"i",
	"mark",
	"ol",
	"small",
	"strong",
	"table",
	"tt",
	"ul",
	0
};

static int
istagname(const char *s)
{	int i;

	if (s)
	for (i = 0; Tname[i] != 0 && *Tname[i] <= *s; i++)
	{	if (strcasecmp(Tname[i], s) == 0) // strings.h
		{	return i;
	}	}
	return -1;
}

static int
is_nesting(const char *s)
{	int i;

	for (i = 0; NestingTags[i] != 0; i++)
	{	if (strcasecmp(s, NestingTags[i]) == 0)
		{	return 1;
	}	}
	return 0;
}

static int
is_selfclosing(const char *s)
{	int i;

	for (i = 0; Selfclosing[i] != 0; i++)
	{	if (strcasecmp(s, Selfclosing[i]) == 0)
		{	return 1;
	}	}
	return 0;
}

static Prim *
find_close_tag(Prim *q, int tn)
{	Prim *b, *f, *oq = q;
	int ntn, tag_closed = 0;

	if (tn < 0)
	{	return (Prim *) 0;
	}

	q = q->nxt; // scan ahead starting with the next token after the tag field

	if (oq->prv != NULL)
	for ( ; q && strcmp(q->fnm, oq->fnm) == 0; q = q->nxt)
	{
		// we're searching forward starting at a tagname
		// and first try to find the closing > of that tag

		if (strcmp(q->typ, "cmnt") == 0) // html comment
		{	continue;
		}

		if (!tag_closed
		&&  strcmp(q->txt, ">") == 0
		&&  oq->prv->jmp == NULL)
		{	tag_closed = 1;
			oq->prv->jmp = q;
			q->jmp = oq->prv;
			continue;
		}

		if (!q->txt
		|| (ntn = istagname(q->txt)) < 0)
		{	continue;
		}
		if (is_nesting(q->txt))
		{	if (q->bound	// nesting
			&&  q->bound->seq > q->seq)
			{
				q = q->bound;
				// skip over nesting tag structures
			}
			continue;
		}

		if (!q->txt
		||  tn != ntn)
		{	continue;
		}

		b = q->prv;
		f = q->nxt;

		if (!b
		||  !b->txt
		||  !f
		||  !f->txt
		||  strcmp(f->txt, ">") != 0)
		{	continue;
		}

		if (strcmp(b->txt, "<") == 0)
		{	// not </...: ran into the next tag of the same type
			// before seeing a closing tag -- use this
			// as an implicit close, without bound field
			break;
		}
		if (strcmp(b->txt, "/") != 0
		||  !b->prv
		||  !b->prv->txt
		||  strcmp(b->prv->txt, "<") != 0)
		{	continue;
		}
		if (b->prv->jmp == NULL)
		{	b->prv->jmp = f;
			f->jmp = b->prv;
		}
		// found a proper closing tag
		// oq is the opening tag
		oq->typ = "tag";
		oq->bound = q;
		q->typ = "tag";
		q->bound = oq;
#if 0
	printf("%d::%d:%s (%s) -> -b-> %d::%d:%s (%s)\n",
		oq->lnr, oq->seq, oq->prv->txt, oq->txt,
		 q->lnr,  q->seq,  q->prv->txt,  q->txt);
#endif
		return q;
	}

	oq->typ = "tag";
	return (Prim *) 0;			
}

static Prim *
find_next(Prim *q, char *s)	// >
{
	while (q)
	{	if (q->txt && strcmp(q->txt, s) == 0)
		{	return q;
		}
		q = q->nxt;
	}
	return (Prim *) 0;
}

static Prim *
find_end(Prim *q)	// -- >
{	Prim *oq = q;
	Prim *r = (Prim *) 0;

	while (q)
	{	if (q->txt && strcmp(q->txt, "--") == 0)
		{	q = q->nxt;
			if (q->txt && strcmp(q->txt, ">") == 0)
			{	r = q;
				break;
		}	}
		q = q->nxt;
	}
	if (r)
	{	for (q = oq; q->seq <= r->seq; q = q->nxt)
		{	q->typ = "cmnt";
	}	}
	return r;
}

#define ISOPEN	1
#define ISCLOSE	2

static int
istag_field(Prim *p)
{
	if (strcmp(p->txt, "<") == 0
	&& p->nxt != NULL)
	{	if (isalpha((int) p->nxt->txt[0])
		&& istagname(p->nxt->txt) >= 0)
		{	return ISOPEN;
		}
		if (strcmp(p->nxt->txt, "/") == 0
		&&  p->nxt->nxt
		&&  isalpha((int) p->nxt->nxt->txt[0])
		&&  istagname(p->nxt->nxt->txt) >= 0)
		{	return ISCLOSE;
	}	}
	return 0;
}

typedef struct T_Stack T_Stack;
struct T_Stack {
	Prim *p;
	T_Stack *nxt;
};
T_Stack *tstack;
T_Stack *tstack_free;

static void
push_tstack(Prim *ptr)
{	T_Stack *t;

	if (tstack_free)
	{	t = tstack_free;
		tstack_free = t->nxt;
	} else
	{	t = emalloc(sizeof(T_Stack), 3);
	}
	t->p = ptr;
	t->nxt = tstack;
	tstack = t;
}

static Prim *
pop_tstack(Prim *ptr)
{	Prim *t = (Prim *) 0;
	T_Stack *q;

	if (!tstack)
	{	fprintf(stderr, "%s:%d: tstack error (%s)\n",
			ptr->fnm, ptr->lnr, ptr->txt);
		return t;
	}
	if (strcmp(tstack->p->txt, ptr->txt) != 0)
	{	if (tstack->nxt && strcmp(tstack->nxt->p->txt, ptr->txt) == 0)
		{	// it's out of order with the next tag
			fprintf(stderr, "%s:%d: tag </%s> is out of order, expected </%s>\n",
				ptr->fnm, ptr->lnr, ptr->txt, tstack->p->txt);
			q = tstack->nxt;
			t = q->p;
			tstack->nxt = tstack->nxt->nxt;
			q->nxt = tstack_free;
			tstack_free = q;
		} else
		{	fprintf(stderr, "%s:%d: expected </%s> matching line %d saw </%s>\n",
				ptr->fnm, ptr->lnr, tstack->p->txt, tstack->p->lnr, ptr->txt);
		}
		return t;
	}
	t = tstack->p;
	q = tstack;
	tstack = tstack->nxt;
	q->nxt = tstack_free;
	tstack_free = q;
	return t;
}

static void
clear_stack(void)
{	T_Stack *t;

	while (tstack)
	{	t = tstack;
		tstack = tstack->nxt;
		t->nxt = tstack_free;
		tstack_free = t;
	}
}

static void
warn_stack(const char *fnm)
{	T_Stack *t;

	if (tstack)
	{	fprintf(stderr, "%s: error: unclosed tags at end of file:", fnm);
		for (t = tstack; t; t = t->nxt)
		{	fprintf(stderr, " <%s> (ln %d)", t->p->txt, t->p->lnr);
		}
		fprintf(stderr, "\n");
	}
}

static void
set_jmp(Prim *p)
{	Prim *op = p;
	assert(p != NULL && p->txt[0] == '<');
	while (p && strcmp(p->txt, ">") != 0)
	{	p = p->nxt;
	}
	if (p)
	{	op->jmp = p;
		p->jmp = op;
	}
}

static int
spaces_only(const char *s)
{
	while (*s != '\0')
	{	if (*s != ' ' && *s != '\"')
		{	return 0;
		}
		s++;
	}
	return 1;
}

static void
end_of_block(Prim *ptr)
{	Prim *q;

	// restrict to first line of block only
	// check that the next indent is larger
	for (q = ptr->nxt; q; q = q->nxt)
	{	if (q->mark == 0)
		{	continue;
		}
		if (q->mark <= ptr->mark)
		{	return;
		}
		break;
	}
	if (!q)
	{	return; // does not start a new range
	}

	// find first token with q->mark <= ptr->mark or eof
	for (q = ptr->nxt; q && strcmp(q->txt, "EOF") != 0; q = q->nxt)
	{	if (q->mark <= ptr->mark
		&& (q->mark > 0
		 || strcmp(q->txt, "IND") == 0))
		{	ptr->jmp = q;
			if (q->jmp == (Prim *) 0)
			{	q->jmp = ptr;
			}
			break;
	}	}

	if (q && strcmp(q->txt, "EOF") == 0)
	{	ptr->jmp = q;
		if (q->jmp == (Prim *) 0)
		{	q->jmp = ptr;
	}	}
}

#if 0
static void
python_show(const char *s)
{	Prim *ptr;

	// debugging
	for (ptr = prim; ptr; ptr = ptr->nxt)
	{	printf("%s seq:%3d: lnr:%3d: txt:%s\ttyp:%s\tj:%d:%p :: m:%d l:%d\n",
			s, ptr->seq, ptr->lnr, ptr->txt, ptr->typ,
			ptr->jmp?ptr->jmp->seq:0,
			(void *) ptr->jmp, ptr->mark, ptr->len);
	}
}
#endif

void
handle_python(void)
{	Prim *ptr, *b;
	int tnr;

	// added 2/2024 -- notes:
	// remove type cpp sequences of spaces that do not follow an EOL token
	// use the remaining markers to define the ranges for blocks at the same
	// level of nesting
	// then remove the remaining space sequences of type cpp and EOL markers

	for (ptr = prim; ptr; ptr = ptr->nxt)
	{	if (strcmp(ptr->typ, "cpp") != 0
		||  strcmp(ptr->txt, "EOL") != 0)
		{	continue;
		}
		b = ptr->nxt;

		// remove empty lines:
		// if the next two tokens are spaces
		// followed by EOL then remove that pair
		while (strcmp(b->typ, "cpp") == 0
		      && spaces_only(b->txt)
		      && b->nxt
		      &&  strcmp(b->nxt->typ, "cpp") == 0
		      &&  strcmp(b->nxt->txt, "EOL") == 0)
		{	b = b->nxt->nxt;
			// leave links of skipped tokens in place
			// to preserve links from cmnts to source stream
		}

		if (b != ptr->nxt)	// skip over these pairs
		{	ptr->nxt = b;
			b->prv = ptr;
		}
		if (strcmp(b->typ, "cpp") != 0
		||  spaces_only(b->txt) == 0
		||  b->len < 2)
		{	continue; // not an indent
		}

		if (b->len == 2
		&&  b->nxt
		&&  strcmp(b->nxt->txt, "EOL") == 0)
		{	b->txt = "IND";	// mark as zero indent
			continue;
		}
		// level of nesting is: b->len - 2
		// (accounting for the quotes around the space string)
		b->mark = b->len - 1;	// keep it +1, reset later
	}

	// look for marked tokens only and build ranges
	for (ptr = prim; ptr; ptr = ptr->nxt)
	{	if (ptr->mark > 0)
		{	end_of_block(ptr); // connect jmp to next_lower_or_eof
			//  search forward to the first level of nesting
			//  that is less than this one and connect it in .jmp
			// when two nested blocks have the same end point
			// they backward jump for the end is for the first of these only
	}	}

	// reset .mark to zero and replace spaces with token "IND"
	// with the nesting level recorded in the .len field
	// there's a small risk here that the .len field can
	// indicate a string longer than 3 chars, but the
	// string is an uneditable constant string so it is safe
	for (ptr = prim; ptr; ptr = ptr->nxt)
	{	if (ptr->mark > 0)
		{	ptr->len = ptr->mark; // still +1
			ptr->mark = 0;
			ptr->txt = "IND";
	}	}

	// drop the EOL markers unless -eol was set
	// and drop indented empty lines
A:	for (ptr = prim; ptr; ptr = ptr->nxt)
	{	if (strcmp(ptr->txt, "IND") == 0
		&&  ptr->len >= 2
		&&  ptr->nxt
		&&  strcmp(ptr->nxt->txt, "EOL") == 0)
		{	if (ptr == prim)
			{	prim = ptr->nxt->nxt;
				prim->prv = (Prim *) 0;
				goto A;
			}
			assert(ptr->prv);
			b = ptr->prv;
			b->nxt = ptr->nxt->nxt;
			if (b->nxt)
			{	b->nxt->prv = b;
			}
			ptr = b;
		}
		if (eol != 0
		||  strcmp(ptr->txt, "EOL") != 0)
		{	continue;
		}
		if (ptr == prim)
		{	prim = ptr->nxt;
			if (prim)
			{	prim->prv = (Prim *) 0;
			}
			goto A;
		}
		assert(ptr->prv);
		b = ptr->prv;
		b->nxt = ptr->nxt;
		if (b->nxt)
		{	ptr->nxt->prv = b;
		}
		ptr = b;
	}

	// move startpt of each block forward to first ':'
	for (ptr = prim; ptr; ptr = ptr->nxt)
	{	if (strcmp(ptr->txt, "IND") == 0
		&&  strcmp(ptr->typ, "cpp") == 0
		&&  ptr->jmp != (Prim *) 0)
		{	for (b = ptr; b /* && b->lnr == ptr->lnr */; b = b->nxt)
			{	if (strcmp(b->txt, ":") == 0)
				{	b->jmp = ptr->jmp;
					break;
			}	}
			ptr->jmp = 0;
	}	}

	// renumber
	count = 0;
	for (ptr = prim; ptr; ptr = ptr->nxt)
	{	ptr->seq = ++count;
	}
	tnr = 0;
	for (ptr = cmnt_head; ptr; ptr = ptr->nxt)
	{	ptr->seq = ++tnr;
	}
	if (!no_match)
	{	printf("python: %5d src tokens\n", count);
		printf("python: %5d cmnt tokens\n", tnr);
	}
}

void
handle_html(void)
{	Prim *ptr, *ct, *prm;
#if 0
	added 1/2024 -- notes:
	the identifier of all html tags are marked with type 'tag'
	the .jmp field of the preceding < points with the matching > of the same tag
	and vice versa (they point to each other)
	the .bound field points to the tag identifier of the corresponding closing tag,
	and vice versa (they point to each other)
	if the closing tag can be found; it remains 0 if not found or if self-closing

	year7/show_tags.cobra shows links (cobra -html -f year7/prog.cobra ...)
	year7/html_check performs basic checks on html tag
#endif
	// new: first tag lists and table, doctype and comments
	clear_stack();
	prm = prim;
	for (ptr = prim; ptr; ptr = ptr->nxt)
	{	int tg = istag_field(ptr);	// '<' @ident or '<' / @ident

		if (strcmp(prm->fnm, ptr->fnm) != 0)
		{	warn_stack(prm->fnm);
			clear_stack();
			prm = ptr;
		}

		switch (tg) {
		case ISOPEN:
			ct = ptr->nxt;
			break;
		case ISCLOSE:
			ct = ptr->nxt->nxt;
			break;
		default:
			ct = ptr->nxt;
			// <!DOCTYPE and <!-- comments
			if (ct
			&&  strcmp(ct->txt, "!") == 0
			&&  ct->nxt)
			{	if (strcmp(ct->nxt->txt, "DOCTYPE") == 0)
				{	ptr->typ = "tag";
					ptr->jmp = find_next(ct->nxt, ">");
				} else
				if (strcmp(ct->nxt->txt, "--") == 0)
				{	// comment
					ptr->typ = "cmnt";
					ptr->jmp = find_end(ct->nxt); // -- >
			}	}
			if (ptr->jmp)
			{	ptr->jmp->jmp = ptr;
			}
			continue;
		}

		if (is_nesting(ct->txt))
		{	if (tg == ISOPEN)
			{	push_tstack(ct);
				ct->typ = "tag";
				set_jmp(ptr);
			} else
			{	Prim *t;
				t = pop_tstack(ct);
				if (t)
				{	ct->typ = "tag";
					ct->bound = t;
					t->bound = ct;
	}	}	}	}
	warn_stack(prm->fnm);
	// second pass: non-nesting html tags, skip over nestables while
	// looking for matching close tag
	int tn = 0;
	for (ptr = prim; ptr; ptr = ptr->nxt)
	{	int tg = istag_field(ptr);
		switch (tg) {
		case ISOPEN:
			ct = ptr->nxt;
			break;
		case ISCLOSE:
			ct = ptr->nxt->nxt;
			break;
		default:
			continue;
		}

		if (is_selfclosing(ct->txt))
		{	continue;
		}
		if (tg == ISOPEN)
		{	tn = istagname(ct->txt);
			find_close_tag(ct, tn); // set bound fields, skip nestings and comments
	}	}
}

// externally visible functions:

void
strip_comments_and_renumber(int reset_ranges)	// split streams
{	int scnt = 0, ccnt = 0;
	Prim *ptr, *ct, *lst = 0, *nxt;
	char *lfnm;

	// can be called repeatedly when files are added

	for (ptr = prim; ptr; ptr = nxt)
	{	nxt = ptr->nxt;
		if (strcmp(ptr->typ, "cmnt") != 0)
		{	lst = ptr;
			continue;
		}
		if (strstr(ptr->txt, "@suppress ") != NULL)
		{	has_suppress_tags = 1;
		}
		scnt++;
		ct = ptr;
		// remove from stream
		if (!lst)
		{	prim = ct->nxt;
			if (prim)
			{	prim->prv = 0;
			}
		} else
		{	lst->nxt = ct->nxt;
			if (ct->prv)
			{	lst = ct->prv;
			}
			if (nxt)
			{	nxt->prv = lst;
		}	}

		if (ct == plst) // next == 0
		{	plst = ct->prv;
			plst->nxt = 0;
		}

		ct->bound = nxt; // preserve a link to source
		// add to separate list
		ct->nxt = 0;
		ct->prv = 0;
		if (!cmnt_tail)
		{	cmnt_head = cmnt_tail = ct;
		} else
		{	cmnt_tail->nxt = ct;
			ct->prv = cmnt_tail;
			cmnt_tail = ct;
	}	}
	if (verbose>1)
	{	printf("%d comments stripped\n", scnt);
	}

	// renumber -- and reset first_token and last_token
	scnt = 0;
	lfnm = NULL;
	for (ptr = prim; ptr; ptr = ptr->nxt)
	{	ptr->seq = scnt++;
		if (!lfnm || strcmp(lfnm, ptr->fnm) != 0)
		{	if (lfnm && ptr->prv)
			{	update_last_token(lfnm, ptr->prv);
			}
			lfnm = ptr->fnm;
			update_first_token(lfnm, ptr);
		}
		if (ptr->nxt && ptr->nxt->prv != ptr)
		{	ptr->nxt->prv = ptr;
	}	}
	ccnt = 0;
	for (ct = cmnt_head; ct; ct = ct->nxt)
	{	ct->seq = ccnt++;
		if (ct->nxt && ct->nxt->prv != ct)
		{	ct->nxt->prv = ct;
	}	}

	if (reset_ranges)
	{	set_ranges(prim, plst, 1);
	}
	if (verbose)
	{	printf("%d source tokens and %d comments\n", scnt, ccnt);
	}
}

int
is_comments_or_source(void)
{
	return (int) ((s_prim)?CMNT:SRC);
}

void
comments_or_source(int to_comments)	// switch between these
{
	if (!s_prim && !cmnt_head)	// neither stream exists
	{	if (prim && plst)
		{	s_prim = prim;
			s_plst = plst;
		} else
		{	fprintf(stderr, "error: no tokens defined\n");
			return;
	}	}
	if (to_comments)
	{	if (s_prim)
		{	return;		// already set
		}
		s_prim = prim;
		s_plst = plst;
		prim = cmnt_head;
		plst = cmnt_tail;
	} else
	{	if (!s_prim)
		{	return;		// already set
		}
		prim = s_prim;
		plst = s_plst;
		s_prim = s_plst = 0;
	}
	set_ranges(prim, plst, 2);
}

int
matches_suppress(Prim *r, const char *s)
{	Prim *q;
	char *ptr;
	char *qtr;

	// tag example: @suppress Macros
	// for pattern set Macros in rules/main/basic.cobra

	for (q = cmnt_head; q; q = q->nxt)
	{	if (q->lnr != r->lnr
		||  strcmp(q->fnm, r->fnm) != 0)
		{	continue;
		}

		ptr = strstr(q->txt, "@suppress ");
		if (!ptr)
		{	continue;
		}

		// there's a warning at r, and
		// there's a comment on the same line, and
		// the comment contains @suppress

		ptr += strlen("@suppress ");
		while (*ptr == ' ')
		{	ptr++;
		}	// now at start of tag

		while (strlen(ptr) > 0)
		{	qtr = strchr(ptr, ' ');		// multiple tags?
			if (qtr)
			{	*qtr = '\0';
			}
			if (strstr(s, ptr) != NULL)	// tag in set-name
			{	if (qtr)
				{	*qtr = ' ';	// restore
				}
				return 1;		// match
			}
			if (qtr)
			{	ptr += strlen(ptr)+1;	// next tag
				*qtr = ' ';		// restore
			} else
			{	break;
	}	}	}
	return 0;					// no match
}

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
	{	fprintf(stderr, "warning: cannot create tmpfile '%s'\n", t);
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
	// printf("%d: Add file '%s' preproc '%s'\n", cid, f, preproc);
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
		fclose(tfd);	// gh: was missing, added 6/18/2021
		if (stream == 1)
		{	static int warned = 0;
			if (verbose > 1)
			{	static long total_read = 0;
				total_read += lncnt;
				printf("	Read %d bytes of %d max\tcumulative: %ld\n",
					lncnt, stream_lim, total_read);
			}
			if (stream_margin_set)
			{	if (1000*stream_margin > stream_lim
				&&  !warned)
				{	printf("cobra: reduce memory with -stream_margin %d",
						stream_lim/1000);
					printf(" (< 0.1%% of the window size of -stream %d)\n", stream_lim);
					warned = 1;
				}
			} else if (stream_lim >= 10000)
			{	stream_margin = stream_lim/1000;
				if (!warned)
				{	warned = 1;
					printf("cobra: adjusted stream_margin to %d\n", stream_margin);
		}	}	}
	}

	if (no_cpp)
	{	if ((Px.lex_yyin = open(f, O_RDONLY)) < 0)
		{	if (!view)
			{  fprintf(stderr, "cannot open file '%s'\n", f);
			}
			return 0;
		}
	} else
	{	char *buf;
		int   n;

		set_tmpname(fnm, "af2", sizeof(fnm));
		assert(cid < N_max);

		n = strlen(CPP)
			    + strlen(" ")
			    + strlen(Preproc[cid])
			    + strlen(" -w -E -x c ")	// suppress warnings
			    + strlen(f)
			    + strlen(" > ")
			    + strlen(fnm)
			    + 8;	// some margin

		buf = (char *) hmalloc(n, cid, 124);
		snprintf(buf, n, "%s %s -w -E -x %s %s > %s",
			    CPP, Preproc[cid], lang, f, fnm);

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
	}	}

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
set_ranges(Prim *a, Prim *b, int who)
{	static short tokmaxrange = 0;
	int i, j, span = count/Ncore;
	Prim *x;

	if (!a || !b)
	{	if (verbose)
		{	fprintf(stderr, "set_ranges: %p -> %p\n",
				(void *) a, (void *) b);
		}
		return;
	}

	// printf("set_ranges %d -> %d count %d span %d\n", a->seq, b->seq, count, span);
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
		{	printf("set %d: %d-%d <%d> (%s:%d - %s:%d) -- %p -> %p\n",
				i, tokrange[i]->from->seq, tokrange[i]->upto->seq,
				(int)(tokrange[i]->upto->seq - tokrange[i]->from->seq),
				tokrange[i]->from->fnm, tokrange[i]->from->lnr,
				tokrange[i]->upto->fnm, tokrange[i]->upto->lnr,
				(void *) tokrange[i]->from, (void *) tokrange[i]->upto);
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
		set_ranges(prim, plst, 4);
		// when streaming we cannot separate comment tokens
		// from the main stream
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

void
Nthreads_set(int n)
{
	if (n < 1)
	{	fprintf(stderr, "error: the number of cores must be positive\n");
		return;
	}
	Ncore = n;
	if (Ncore > N_max)
	{	int i;
		char **Npre;
		Npre = (char **) emalloc(Ncore * sizeof(char *), 16);

		for (i = 0; i < Ncore; i++)
		{	Npre[i] = (i < N_max) ? Preproc[i] : "";
		}
		N_max = Ncore;
		Preproc = Npre;
	}
}

static void
Nthreads(char *s)
{
	if (!isdigit((uchar) *s))
	{	fprintf(stderr, "error: usage -N[0-9]+\n");
		exit(1);
	}
	Nthreads_set(atoi(s));
}

static void
preproc_setdefault(void)
{	int i;

	for (i = 1; i < N_max; i++)
	{	Preproc[i] = Preproc[0];
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

	for (a = cl_var; a; a = a->nxt)
	{	if (strcmp(a->nm, varname) == 0)	// previously defined
		{	a->s  =    (char *) emalloc (strlen(value)+1, 22);
			strcpy(a->s, value);
			return;
	}	}

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
	fprintf(stderr, "\t-comments           -- deprecated (see Comment Handling in the command 'help' response)\n");
	fprintf(stderr, "\t-configure dir      -- set and remember the name for the Cobra installation directory\n");
	fprintf(stderr, "\t-cpp                -- enable C preprocessing%s\n", no_cpp?"":" (default)");
	fprintf(stderr, "\t-cpp=clang          -- enable C preprocessing and use clang instead of gcc\n");
	fprintf(stderr, "\t-d and -v -d        -- debug cobra inline program executions\n");
	fprintf(stderr, "\t-echo               -- echo script command names when called (eg from query libraries)\n");
	fprintf(stderr, "\t-eof                -- omit EOF tokens at the end of files\n");
	fprintf(stderr, "\t-eol                -- add EOL tokens at all newlines\n");
	fprintf(stderr, "\t-e name             -- print lines with tokens matching name\n");
	fprintf(stderr, "\t-e \"token_expr\"     -- print lines matching a token_expr (cf -view)\n");
	fprintf(stderr, "\t                       use meta-symbols: ( | ) . * + ?\n");
	fprintf(stderr, "\t                       use ^name for not-matching name\n");
	fprintf(stderr, "\t                       var-binding:   name:@ident\n");
	fprintf(stderr, "\t                       var-reference: :name\n");
	fprintf(stderr, "\t                       same as -re, -regex,-expr, see also -pe\n");
	fprintf(stderr, "\t-f file             -- execute commands from file and stop (cf -view)\n");
	fprintf(stderr, "\t-F file             -- read file names to process from file instead of command line (see also -recursive)\n");
	fprintf(stderr, "\t-global             -- allow pattern matches (pe) to cross file boundaries\n");
	fprintf(stderr, "\t-html               -- recognize html tags\n");
	fprintf(stderr, "\t-Idir, -Dstr, -Ustr -- preprocessing directives\n");
	fprintf(stderr, "\t-Java               -- recognize Java keywords\n");
	fprintf(stderr, "\t-json               -- generate json output for -pattern/-pe matches (only)\n");
	fprintf(stderr, "\t-json+              -- generate more detailed json output for -pattern/-pe matches\n");
	fprintf(stderr, "\t-l or -lib          -- list available predefined cobra -f checks\n");
	fprintf(stderr, "\t-MaxMem N           -- set the maximum memory use to N MB (default: 24000)\n");
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
	fprintf(stderr, "\t                       (|)+? can be turned back into meta-symbols by preceding them with \\\n");
	fprintf(stderr, "\t                       * and ] are meta-symbols unless preceded by a space\n");
	fprintf(stderr, "\t                       [ is a meta-symbol unless followed by a space\n");
	fprintf(stderr, "\t                       a tokenpattern preceded by / is a regular expr, escape with \\\n");
	fprintf(stderr, "\t-preserve           -- preserve temporary files in /tmp\n");
	fprintf(stderr, "\t-prune              -- after reading in files, remove code between #if 0 and #endif\\\n");
	fprintf(stderr, "\t-Python             -- recognize Python keywords\n");
	fprintf(stderr, "\t-quiet              -- do not print script commands executed or nr of matches\n");
	fprintf(stderr, "\t-recursive \"pattern\" -- use find on pattern to populate a list of files to process (see also -F)\n");
	fprintf(stderr, "\t                        eg -recursive '*.[ch]' or -recursive '*.java'\n");
	fprintf(stderr, "\t-regex \"expr\"       -- see -e\n");
	fprintf(stderr, "\t-runtimes           -- report runtimes of commands executed, if >1s\n");
	fprintf(stderr, "\t-scrub              -- produce output in scrub-format\n");
	fprintf(stderr, "\t-seed file          -- read JSON formatted output from file to seed initial pattern sets\n");
	fprintf(stderr, "\t-showprog           -- show a dot graph of the code generated for the first inline program\n");
	fprintf(stderr, "\t-stream N           -- set stdin stream buffer-limit to N  (default %d)\n", stream_lim);
	fprintf(stderr, "\t-stream_margin N    -- set stdin window margin to N tokens (default %d)\n", stream_margin);
	fprintf(stderr, "\t-stream_override    -- override warning about a non-streamable script\n");
	fprintf(stderr, "\t-terse              -- disable output from d, l, and p commands, implies -quiet\n");
	fprintf(stderr, "\t-text               -- no token types, just text-strings and symbols\n");
	fprintf(stderr, "\t-tok                -- only tokenize the input\n");
	fprintf(stderr, "\t-version            -- print version number and exit\n");
	fprintf(stderr, "\t-v                  -- more verbose\n");
	fprintf(stderr, "\t-view -f file       -- show dot-graph of DFA(s) of inline program(s)\n");
	fprintf(stderr, "\t-view -e  \"token_expr\" -- show dot-graph of NDFA for expr\n");
	fprintf(stderr, "\t-view -pe \"token_expr\" -- show dot-graph of NDFA for pattern expr\n");
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
	s = get_file(cid);
	do_unlock(cid);
	return s;
}

static void *
one_core(void *arg)
{	int  *i, cid;
	char *s;
	char *op;

	i = (int *) arg;
	cid = *i;

	assert(cid < N_max);
	op = Preproc[cid];

	while ((s = get_work(cid)) != NULL)
	{	(void) add_file(s, cid, 1);
		Preproc[cid] = op;	// get_work/get_file may change it
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
{	char *op = Preproc[0];

	if (verbose)
	{	printf("%d files, using %d cores\n", argc, Ncore);
	}
	argv++;
	while (--argc > 0)
	{	rem_file(*argv++, 0);	// may change Preproc
		Preproc[0] = op;	// restore
	}
}

char *
get_preproc(int cid)
{
	assert(cid >= 0 && cid < N_max);
	return Preproc[cid];
}

void
set_preproc(char *s, int cid)
{
	assert(cid >= 0 && cid < N_max);
	Preproc[cid] = s;
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

	if (verbose>1) { printf("parse\n"); }
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

	if (verbose>1) { printf("renumber\n"); }
	for (cid = n_s = 0; cid < Ncore; cid++)
	{	if (verbose>1) { printf("%d	%d\n", cid, n_s); }
		pass_arg[cid].start_nr = n_s;
		n_s += (Px.lex_plst)?Px.lex_plst->seq + 1:0;
	}
	if (verbose>1) { printf("total:	%d\n", n_s); }

	for (i = 1; i < Ncore; i++)
	{	pass_arg[i].who = i;
		(void) pthread_create(&t_id[i], 0, renumber,	// renumber
			(void *) &(pass_arg[i].who));
	}
	for (i = 1; i < Ncore; i++)
	{	(void) pthread_join(t_id[i], 0);
	}
}

char *
strip_directives(char *f, int cid)
{	char *ptr = f;
	int n = strlen(f);

	// skip any leading and trailing whitespace
	assert(cid >= 0 && cid < N_max);

	while (*ptr == ' ' || *ptr == '\t')
	{	ptr++;
	}
	n--;
	while (n >= 0 && (f[n] == ' ' || f[n] == '\t'))
	{	f[n--] = '\0';
	}

	// compilation directives, if any,
	// precede the filename, which is last
	// but the filename may contain spaces
	// provided they are escaped with a backslash

	do {	ptr = strrchr(ptr, ' '); // last space on line
		if (!ptr)
		{	break;
		}
		if (*(ptr-1) == '\\')
		{	ptr--;
		}
	} while (*ptr == '\\');

	if (ptr && *ptr == ' ')
	{	*ptr = '\0';
		if (strchr(f, '-'))	// really a directive?
		{	// there must be at least
			// one of these recognized
			// directives
			if (strstr(f, "-D")
			||  strstr(f, "-I")
			||  strstr(f, "-U"))
			{	Preproc[cid] = f; // to be saved
			}
			// point at filename proper
			f = ptr+1;
		} else
		{	*ptr = ' ';	// restore
	}	}
	if (verbose)
	{	printf("prep: '%s' file: '%s'\n", Preproc[cid], f);
	}
	return f;
}

static void
seq_scan(int argc, char *argv[])
{	int i;
	char *fn, *op = Preproc[0];

	if (verbose>1)
	{	printf("parse\n");
	}
	start_timer(0);
	for (i = 1; i < argc; i++)
	{	fn = strip_directives(argv[i], 0);	// single-core
		(void) add_file(fn, 0, 1);
		Preproc[0] = op;			// restore
	}
	stop_timer(0, 0, "G");
}

static char **
prep_args(const char *f, int *ac)
{	FILE *fd;
	char *q, **aa, buf[512];
	int n, cnt = 0;

	// read file arguments from a file
	// and make it look like they came from the command line
	
	*ac = 0;
	if ((fd = fopen(f, "r")) == NULL)
	{	return NULL;
	}
	while (fgets(buf, sizeof(buf), fd))
	{	if (buf[0] != '#')	// not a comment line
		{	cnt++;		// count nr of files
	}	}
	fclose(fd);

	aa = (char **) emalloc((cnt+1) * sizeof(char *), 111);
	aa[0] = progname;

	if (cnt == 0)			// no files
	{	*ac = 1;
		return aa;
	}

	if ((fd = fopen(f, "r")) == NULL)
	{	return NULL;
	}
	for (n = 1; n <= cnt; n++)
	{	if (!fgets(buf, sizeof(buf), fd))
		{	fclose(fd);
			return NULL;
		}
		if (buf[0] == '#')	// skip comment line
		{	n--;
			continue;
		}
		while ((q = strchr(buf, '\n')) || (q = strchr(buf, '\r')))
		{	*q = '\0';
		}
		aa[n] = (char *) emalloc(strlen(buf)+1, 111);
		strcpy(aa[n], buf);
	}
	fclose(fd);

	*ac = cnt+1;

	return aa;
}

#if 0
static void
ddebug(int n, char *v[])
{	int m;
	printf("cnt %d:\n", n); fflush(stdout);
	for (m = 0; m < n; m++)
	{	printf("%d\t", m); fflush(stdout);
		printf("%s\n", v[m]); fflush(stdout);
	}
}
#endif
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
check_negations(char *p)
{	char *m, *n;

	// check that in a regular expression negations of
	// meta-symbols ( | ) . * + ? don't occur

	if (strlen(p) == 0)
	{	return p;
	}

	m = n = (char *) emalloc(2*strlen(p) + 1, 110);
	while (*p != '\0')
	{	*n++ = *p;
		if (*p == '^')
		{	p++;
			switch (*p) {
			case '(':
			case ')':
			case '|':
			case '.':
			case '*':
			case '+':
			case '?':
				*n++ = '\\';	// insert escape
				*n++ = *p;
				break;
			default:
				p--;		// undo lookahead
				break;
		}	}
		p++;
	}
	*n = '\0';
	return m;
}

char *
unquoted(char *p)
{	int len = strlen(p);

	// called before a regular expression is
	// passed to cobra_te in an interactive session

	if ((*p == '\'' || *p == '\"')
//	&&   len > 0			// guaranteed, since *p is non-zero
	&&   p[len-1] == *p)
	{	p[len-1] = '\0';
		p++;
	}
	return check_negations(p);
}

char *
pattern(char *p)
{	char *n = (char *) emalloc(2*strlen(p)+1, 26);
	char *m = n;
	char *q = p;
	int len;
	int inrange = 0;

	// check if the pattern is quoted:
	// e.g. when typed inline, and remove quotes

	while (isspace((uchar) *p))
	{	p++;
	}

	len = strlen(p);
	if ((*p == '\'' || *p == '\"')
	&&   p[len-1] == *p)
	{	p[len-1] = '\0';
		p++;
		len -= 2;
	}

	// insert or remove escapes for
	// intepretation as a code pattern
	// instead of a standard regex

	while (len > 0)
	{	switch (*p) {
		case '\\':
			// escaped symbols (|)+?
			// are now not-literal but meta
			switch (*(p+1)) {
			case '(':
			case ')':
			case '|':
			case '+':
			case '?':
				// remove the '\\'
				p++;
				break;
			default:
				*n++ = *p++;
			}
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
			if (p == q || *(p-1) == ' ')
			{	*n++ = '\\';
			} else
			{	inrange--;
			}
			*n++ = *p++;
			len--;
			break;
		case '*':
			if (p == q
			|| (*(p-1) == ' ' && !inrange))
			{	*n++ = '\\';
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
	char *e = q;
	inrange = 0;
//	printf("inp: %s\nout: %s\n", q, m);
//
//	do a final check to see if the number of \( and \)
//	symbols are properly matched in a pattern expression
//	1/21/24: note that for pe we added \\, for use as re
//	so we must check for the matching of unescaped ( and )
//	8/11/24: but dont count them inside ranges [ ... ]

	q = m;
	int cnt1 = 0, cnt2 = 0;
	while (*q != '\0')
	{	if (*q == '[' && *(q-1) != '\\')
		{	inrange++;
		} else if (*q == ']' && *(q-1) != '\\')
		{	inrange--;
		}
		if (!inrange)
		if ((*q == '(' || *q == ')')
		&&  (q == m || *(q-1) != '\\'))
		{	if (*q == '(')
			{	cnt1++;
			} else
			{	cnt2++;
		}	}
		q++;
	}
	if (cnt1 != cnt2)
	{	fprintf(stderr, "error: the number of \\( (%d) and \\) (%d) meta-symbols don't match\n",
			cnt1, cnt2);
		fprintf(stderr, "input: %s\n", e);
		return NULL;
	}

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

	if (!s || strlen(s) == 0)
	{	fprintf(stderr, "usage: cobra -configure dirname\n");
		return 1;
	}

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
{	int i;

	progname = (char *) emalloc(strlen(argv[0]) + 1, 112);
	strcpy(progname, argv[0]);

	Preproc = (char **) emalloc(N_max * sizeof(char *), 112);
	for (i = 0; i < N_max; i++)
	{	Preproc[i] = "";
	}

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
			  	no_cpp = 0;
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

		case 'c': if (strncmp(argv[1], "-cpp", 4) == 0)
			  {	no_cpp = 0;
				if (argv[1][4] == '='
				&&  strlen(argv[1]) > 6)
				{	fprintf(stderr, "setting preprocessor to '%s'\n", &argv[1][5]);
					CPP = &argv[1][5];
				}
				break;
			  }
			  if (strcmp(argv[1], "-comments") == 0)
			  {	if (verbose)
				{	fprintf(stderr, "warning: -comments is decprecated\n");
				}
				break;
			  }
			  if (strcmp(argv[1], "-configure") == 0)
			  {	check_argc(2, "-configure");
				return do_configure(argv[2]);
			  }
			  if (strncmp(argv[1], "-cwe", 4) == 0
			  &&  strstr(progname, "cwe") != NULL)
			  {	argv[1]++; // remove hyphen
				goto cwe_mode;
			  }
			  // -c commands
			  check_argc(2, "-c");
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
			  {	eol = 1 - eol; // default value is 0
				break;
			  }
			  if (strcmp(argv[1], "-eof") == 0)
			  {	eof = 1 - eof;	// default value is 1
				break;
			  }
			  if (strcmp(argv[1], "-echo") == 0)
			  {	echo = 1;
				break;
			  }
RegEx:			  no_match = 1;		// -e -expr -re or -regex
			  check_argc(2, "-e -re or -regex");
			  cobra_texpr = check_negations(argv[2]);
			  argc--; argv++;
			  if (view)
			  {	p_debug = 5;
			  }
			  break;

		case 'f': check_argc(2, "-f");
			  cobra_target = argv[2];
			  argc--; argv++;
			  no_match = 1;	// quiet mode
			  if (view)
			  {	if (verbose)
				{	p_debug = 3;
				} else
				{	p_debug = 4;
			  }	}
			  break;

		case 'F': check_argc(2, "-F");
			  file_list = argv[2];	// file args read from file_list
			  argc--; argv++;
			  break;

		case 'g': if (strcmp(argv[1], "-global") == 0)
			  {	across_file_match = 1;
			  } else
			  {	gui = 1;
			  }
			  break;

		case 'h': if (strcmp(argv[1], "-html") == 0)
			  {	html = 1;
			  	handle_typedefs = 0;
			  	break;
			  }
			  return usage(argv[1]);

		case 'I': add_preproc(argv[1]);
			  break;

		case 'j':
			  if (strcmp(argv[1], "-json") == 0)
			  {	json_format = 1;
				break;
			  }
			  if (strcmp(argv[1], "-json+") == 0)
			  {	json_format = json_plus = 1;
				break;
			  }
			  return usage(argv[1]);

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
			  {	fprintf(stderr, "error: check tool installation\n");
			  }
			  return 0;

		case 'M':
			  if (strcmp(argv[1], "-MaxMem") == 0)
			  {	argc--; argv++;
				MaxMemory = (unsigned long) atoi(argv[1]);
				break;
			  }
			  return usage(argv[1]);

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
			  // -nocpp
			  no_cpp = no_cpp_sticks = 1;
			  break;

		case 'N': Nthreads(&argv[1][2]);
			  break;

		case 'p': if (strncmp(argv[1], "-pre", 4) == 0)	// preserve temp files
			  {	preserve = 1;
				break;
			  }
			  if (strncmp(argv[1], "-prune", 6) == 0) // prune_if_zero: #if 0 .. #endif
			  {	pruneifzero = 1;
				break;
			  }
			  if (strncmp(argv[1], "-python", 7) == 0)
			  {	python = no_cpp = 1;
				break;
			  }
			  if (strncmp(argv[1], "-pat", 4) == 0
			  ||  strncmp(argv[1], "-pe", 3) == 0)	// pattern expression
			  {	no_match = 1;
				check_argc(2, "-p");
			 	cobra_texpr = pattern(argv[2]);
				if (!cobra_texpr)	// there was an error
				{	return 1;
				}
				argc--; argv++;
				if (view)
				{	p_debug = 5;
					(void) set_base();
				}
				break;
			  }
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
			  if (strcmp(argv[1], "-regex") == 0
			  ||  strcmp(argv[1], "-re") == 0)
			  {	goto RegEx;
			  }
			  if (strcmp(argv[1], "-recursive") == 0)
			  {	check_argc(2, "-recursive");
			  	if (argv[2][0] != '-')
			  	{	recursive = argv[2]; // populate file_list
					argc--; argv++;
					break;
			  }	}
			  return usage(argv[1]);

		case 's': if (strcmp(argv[1], "-scrub") == 0)
			  {	scrub = 1;
				break;
			  }
			  if (strcmp(argv[1], "-seed") == 0)
			  {	argc--; argv++;
				seedfile = argv[1];
				break;
			  }
			  if (strcmp(argv[1], "-showprog") == 0)
			  {	showprog = 1;
			//	preserve = 1;
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
					stream_margin_set = 1;
					printf("cobra: stream margin: %d lines\n", stream_margin);
					break;
			  	}
			  } else if (strcmp(argv[1], "-stream_override") == 0)
			  {	stream_override++;
				break;
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

		case 'z': no_headers = 1; // deprecated, backward compatibility
			  break;

		case '-': // -- arguments are passed to backend, if any
			  if (strlen(argv[1]) > 2
			  &&  strcmp(progname, "cobra") != 0)
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
#ifdef PC
	if (no_cpp == 0
	&&  Ncore > 1)	// triggers cygwin bug
	{	fprintf(stderr, "cobra: on cygwin (only), -cpp -N%d triggers a cygwin multi-threading bug\n", Ncore);
		fprintf(stderr, "       instead, start with -cpp -N1 and type \"ncore %d\" ", Ncore);
		fprintf(stderr, "once running\n");
		exit(1);
	}
#endif
	preproc_setdefault();

	if (strstr(backend, "help") != NULL)
	{	// means progname != cobra
		// let the backend show the usage rules
		cobra_main();
		return 0;
	}

	if (!set_base())
	{	fprintf(stderr, "error: cannot open ~/.cobra : check tool installation\n");
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
		{	fprintf(stderr, "error: bad filename suffix for: -f %s\n",
				cobra_target);
			cobra_target = (char *) 0;
	}	}

	if (strlen(Preproc[0]) > 0 && no_cpp)
	{	fprintf(stderr, "warning: cpp is off, ignoring '%s'\n", Preproc[0]);
	}

	if (cobra_texpr)
	{	if (strstr(cobra_texpr, "EOL") && !eol)
		{	if (verbose)
			{	fprintf(stderr, "%s: enabling -eol\n", progname);
			}
			eol = 1;
		}
		if (strstr(cobra_texpr, "EOF") && !eof)
		{	if (verbose)
			{	fprintf(stderr, "%s: enabling -eof\n", progname);
			}
			eof = 1;
	}	}

	umask(022);
	prep_pre();
	ini_timers();
	ini_lock();

	if (recursive)
	{	int dfd;
		char *buf;

		strcpy(tmpf, "/tmp/_f_XXXXXX");
		if ((dfd = mkstemp(tmpf)) < 0)
		{	fprintf(stderr, "error: cannot create tmp file for -recursive option\n");
			return 1;
		}
		close(dfd);
		buf = (char *) emalloc(strlen(recursive) + strlen("find . -type f -name ''") + 1, 113);
		sprintf(buf, "find . -type f -name '%s' > %s 2>&1", recursive, tmpf);
		if (verbose)
		{	printf("writing file_list to %s\n", tmpf);
		}
		if (system(buf))
		{	fprintf(stderr, "error: find command (-recursive option) failed\n");
			return 1;
		}
		file_list = tmpf;
	}

	if (file_list)	// read filenames from file
	{	if (argc > 1)
		{	fprintf(stderr, "%s: error, %d redundant args, starting with '%s'\n",
				progname, argc - 1, argv[1]);
			return 1;
		}
		argv = prep_args(file_list, &argc);
		// ddebug(argc, argv);
		if (!argv || argc <= 1)
		{	fprintf(stderr, "%s: error reading '%s'\n", progname, file_list);
			return 1;
	}	}

	if (Ncore > 1 && argc > 2)	// at least 2 files and cores
	{	ini_files(argc, argv);	// seed files[0] temporarily
		par_scan();
		clr_files();
	} else
	{	seq_scan(argc, argv);
		if (handle_typedefs)
		{	if (verbose>1)
			{	printf("typedefs\n");
			}
			start_timer(0);
			typedefs(0);
			stop_timer(0, 0, "H");
	}	}

	if (argc == 1 && !gui)
	{	read_stdin = 1;
		if (stream == 0) // no override with -nostream
		{	stream = 1;
	}	}

	if (stream == 1
	&& (!(cobra_texpr || cobra_target || cobra_commands || Ctok)
	||  Ncore != 1
	||  !no_cpp))
	{	if (strstr(progname, "cobra") != NULL)
		{	if (!no_cpp)
			{	fprintf(stderr, "%s: error: cannot use -cpp with streaming input\n", progname);
			}
			if (Ncore != 1)
			{	fprintf(stderr, "%s: error: cannot use Ncore>1 with streaming input\n", progname);
			}
			if (no_cpp && Ncore == 1)
			{	fprintf(stderr, "%s: error: streaming input\n", progname);
				fprintf(stderr, "    requires -e, -pe or -f\n");
			}
			return 1;
	}	}

	if (read_stdin && !view)
	{	if (no_cpp)
		{	fprintf(stderr, "%s: reading stdin\n", progname);
			(void) add_file("", 0, 1);	// keep single-core
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
	post_process(1);		// collect info from cores
	strip_comments_and_renumber(1);	// set cmnt_head/cmnt_tail
	if (html)
	{	handle_html();
	} else if (python)
	{	handle_python();
	}
	if (pruneifzero)
	{	prune_if_zero();
	}

	if (seedfile)
	{	json_import(seedfile, 0);
	}

	if (strstr(progname, "cobra") == NULL
	|| !check_config())	// no ./.cobra file
	{	cobra_main();
	}

	return 0;
}

void
clear_file_list(void)
{
	if (recursive
	&&  file_list)
	{	unlink(file_list);
	}
}

void
do_typedefs(int cid)
{
	if (handle_typedefs)
	{	typedefs(cid);
	}
}

void
prune_if_zero(void)	// omit tokens between #if 0 and #endif
{	Prim *q, *r, *s;
	int cnt = 0, nr = 0, n;
	static int pruned = 0;

	if (pruned)
	{	return;
	}

	if (verbose)
	{	printf("Prune If Zero\n");
	}
	if (!stream)
	for (q = prim; q; q = q->nxt)
	{	if (strcmp(q->txt, "#if") != 0
		||  !q->nxt
		||  strcmp(q->nxt->txt, "0") != 0)
		{	continue;
		}
		s = r = q;
		n = 0;
		do {
			s = s->nxt;
			n++;
		} while (s && s->txt[0] != '#');

		if (!s
		||  strcmp(s->txt, "#endif") != 0
		||  strcmp(s->fnm, r->fnm) != 0)
		{	continue;
		}
		s = s->nxt;
		if (!s
		||  strcmp(s->txt, "EOL") != 0)
		{	continue;
		}
		// remove from r to s->nxt;
		cnt += n;
		nr++;
		if (verbose)
		{	printf("%s:%d: prune %d tokens\n", q->fnm, q->lnr, n);
		}
		if (r == prim)
		{	prim = q = s->nxt;
		} else if (r->prv)
		{	r = r->prv;
			r->nxt = s->nxt;
			if (s->nxt)
			{	s->nxt->prv = r;
	}	}	}
	pruned++;

	if (verbose)
	{	printf("prune: removed %d tokens in %d fragments between #if 0 and #endif\n",
			cnt, nr);
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
