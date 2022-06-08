/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

%{
#include "cobra.h"

// parser for boolean and
// arithmetic expressions

#define YYSTYPE	Lexptr
#define YYDEBUG 0

static Lextok	*p_tree;
static int	 last_tok;
static void	 yyerror(const char *);
static int	 yylex(void);
static int	 parse_error;

extern YYSTYPE	 yylval;
extern char	*b_cmd;
extern int	 evaluate(const Prim *, const Lextok *);
extern int	 yyparse(void);
%}

%token	SIZE NR NAME EOE REGEX

%left	OR
%left	AND
%left	EQ NE
%left	GT LT GE LE
%left	'+' '-'
%left	'*' '/' '%'
%right	'.' '~' '!' UMIN SIZE
%right	':'

%%
form	: expr EOE	{ p_tree = $1; return 1; }
	;
expr    : '(' expr ')'	{ $$ = $2; }
	| expr '+' expr	{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr '-' expr	{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr '*' expr	{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr '/' expr	{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr '%' expr	{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr GT expr	{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr GE expr	{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr LT expr	{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr LE expr	{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr EQ expr	{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr NE expr	{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr OR expr	{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr AND expr	{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| NAME		{ $$ = $1; }
	| REGEX 	{ $$ = $1; set_regex($1->s+1); }
	| NR		{ $$ = $1; }
	| SIZE NR	{ $1->rgt = $2; $$ = $1; }
	| SIZE '(' NR ')'	{ $1->rgt = $3; $$ = $1; }
	| '!' expr %prec UMIN	{ $1->rgt = $2; $$ = $1; }
	| '-' expr %prec UMIN	{ $1->rgt = $2; $$ = $1; }
	| '~' expr %prec UMIN	{ $1->rgt = $2; $$ = $1; }
	| '.' NAME %prec UMIN	{ $1->s = $2->s; $$ = $1; }
	| ':' NAME		{ $1->rgt = $2; $$ = $1; /* pe expression */ }
	;
%%
static int iscan;

#define Expect(x, y, z)			\
	if (b_cmd[iscan+1] == x)	\
	{	iscan += 2;		\
		yylval->typ = y;	\
		return y;		\
	} else				\
	{	iscan++;		\
		yylval->typ = z;	\
		return z;		\
	}

static int
isregexp(const char *s)
{	const char *t = s;

	while (*s != '\0'
	&& !isspace((uchar) *s))
	{	s++;
	}
	return (int) (s-t);
}

static int
eval_lex(void)
{	int p;

	yylval = (Lextok *) emalloc(sizeof(Lextok), 10);

	while (isspace((uchar) b_cmd[iscan]))
	{	iscan++;
	}
	yylval->s = "";
	yylval->typ = b_cmd[iscan];
	switch (b_cmd[iscan]) {
	case '/':
		if ((last_tok == EQ || last_tok == NE)
		&&  (p = isregexp(&b_cmd[iscan])) > 0)
		{	yylval->s = emalloc((p+1)*sizeof(char), 11);
			strncpy(yylval->s, &b_cmd[iscan], p+1);
			yylval->typ = REGEX;
			iscan += p;
			return REGEX;
		}
		// else fall thru
	case '*': 
	case '+': case '-':
	case '%': case '.':
	case '(': case ')':
	case ':':	// possible pe expression
		return b_cmd[iscan++];
	case '\\':
		iscan++;
		if (b_cmd[iscan] == '|')	// watch for \|\| from cobra_te.c
		{	if (b_cmd[iscan+1] == '\\'
			&&  b_cmd[iscan+2] == '|')
			{	iscan += 3;
				yylval->typ = OR;
				return OR;
		}	}
		// else fall through
	case '{': case '}':
	case '[': case ']':
		// special case of strings
		// since these have no special
		// meaning in eval exprs
		yylval->s = emalloc(2*sizeof(char), 12);
		yylval->s[0] = b_cmd[iscan++];
		yylval->typ = NAME;
		return NAME;
	case '"':
		p = ++iscan;
		while (b_cmd[iscan] != '"'
		  &&   b_cmd[iscan] != '\0'
		  &&   b_cmd[iscan] != '\n')
		{	if (b_cmd[iscan] == '\\')
			{	iscan++;
			}
			iscan++;
		}
		if (b_cmd[iscan] == '\0'
		||  b_cmd[iscan] == '\n')
		{	yylval->typ = EOE;
			return EOE;
		}
		b_cmd[iscan] = '\0';
		yylval->s = emalloc(strlen(&b_cmd[p])+1, 13);
		strcpy(yylval->s, &b_cmd[p]);	// safe
		b_cmd[iscan++] = '"';
		yylval->typ = NAME;
		return NAME;
	case '\n':
	case '\0':
		yylval->typ = EOE;
		return EOE;
	case '|':
		Expect('|', OR, '|');
		break;
	case '&':
		Expect('&', AND, '&');
		break;
	case '=':
		Expect('=', EQ, '=');
		break;
	case '!':
		Expect('=', NE, '!');
		break;
	case '>':
		Expect('=', GE, GT);
		break;
	case '<':
		Expect('=', LE, LT);
		break;
	default:
		break;
	}

	if (isdigit((uchar) b_cmd[iscan]))
	{	while (isdigit((uchar) b_cmd[iscan]))
		{	yylval->val *= 10;
			yylval->val += b_cmd[iscan++] - '0';
		}
		yylval->typ = NR;
		return NR;
	}

	if (strncmp(&b_cmd[iscan], "size", strlen("size")) == 0)
	{	iscan += (int) strlen("size");
		yylval->typ = SIZE;
		return SIZE;
	}
	if (isalpha((uchar) b_cmd[iscan])
	||  b_cmd[iscan] == '_')
	{	p = iscan;
		while ((isalpha((uchar) b_cmd[iscan])
		&&    !isspace((uchar) b_cmd[iscan]))
		||      isdigit((uchar) b_cmd[iscan])
		||	b_cmd[iscan] == '_'
		||      b_cmd[iscan] == '.')
		{	iscan++;
		}
		yylval->s = emalloc((iscan-p+1)*sizeof(char), 14);
		strncpy(yylval->s, &b_cmd[p], iscan-p);
		yylval->typ = NAME;
		return NAME;
	}

	printf("expr: cannot parse: '%s'\n", &b_cmd[iscan]);
	return '?';
}

static void
yyerror(const char *s)
{	int i;
	printf("expr: %s\n%s\n", s, yytext);
	for (i = 0; i < iscan; i++)
	{	printf(" ");
	}
	printf("^\n<%d>", yytext[iscan]);
	iscan = 0;
	parse_error++;
}

static int
lookup(const Prim *q, const char *s)
{	// .txt and .fnm handled elsewhere
	assert(q && s);
	if (strcmp(s, "lnr") == 0)
	{	return q->lnr;
	}
	if (strcmp(s, "curly") == 0)
	{	return q->curly;
	}
	if (strcmp(s, "round") == 0)
	{	return q->round;
	}
	if (strcmp(s, "bracket") == 0)
	{	return q->bracket;
	}
	if (strcmp(s, "len") == 0)
	{	return q->len;
	}
	if (strcmp(s, "seq") == 0)
	{	return q->seq;
	}
	if (strcmp(s, "mark") == 0)
	{	return q->mark;
	}
	if (strcmp(s, "range") == 0)
	{	if (q->bound || q->jmp)
		{	Prim *dest = q->bound?q->bound:q->jmp;
			if (strcmp(dest->fnm, q->fnm) == 0)
			{	return dest->lnr - q->lnr;
		}	}
		return 0;
	}
	printf("expr: unknown symbol .%s\n", s);
	return 0;
}

static const char *
tokenname(int n)
{	static char c_tmp[8];

	switch (n) {
	case SIZE: return "SIZE";
	case   NR: return "NR";
	case NAME: return "NAME";
	case  EOE: return "EOE";
	case   OR: return "||";
	case  AND: return "&&";
	case   NE: return "!=";
	case   EQ: return "==";
	case   LE: return "<=";
	case   GE: return ">=";
	case   LT: return "<";
	case   GT: return ">";
	case UMIN: return "-";
	case REGEX: return "RE";
	}
	snprintf(c_tmp, sizeof(c_tmp), "%c", n);
	return c_tmp;
}

enum {
	fnm_t     = 1,
	typ_t     = 2,
	txt_t     = 3,
	fct_t     = 4,

	lnr_t     = 10,
	seq_t     = 11,
	mark_t    = 12,
	curly_t   = 13,
	round_t   = 14,
	bracket_t = 15,
	len_t     = 16,

	nxt_t     = 20,
	prv_t     = 21,
	bound_t   = 22,
	jmp_t     = 23
};

static char *
dot_derive(const Prim *q, int e)
{
	switch (e) {
	case fnm_t: return q->fnm;
	case txt_t: return q->txt;
	case typ_t: return q->typ;
	case fct_t: return fct_which(q);
	default:    break;
	}
	return q->txt;
}

#define numeric_cmp(op) \
	switch (e) {	\
	case lnr_t:	return (q->lnr     op r->lnr)?0:1;	\
	case seq_t:	return (q->seq     op r->seq)?0:1;	\
	case mark_t:	return (q->mark    op r->mark)?0:1;	\
	case curly_t:	return (q->curly   op r->curly)?0:1;	\
	case round_t:	return (q->round   op r->round)?0:1;	\
	case bracket_t:	return (q->bracket op r->bracket)?0:1;	\
	case len_t:	return (q->len     op r->len)?0:1;	\
	default:	\
		fprintf(stderr, "error: invalid use of %s in constraint\n", #op);	\
		break;	\
	}


static int
field_cmp(const int op, const Prim *q, const Prim *r, int e)
{	// like strcmp, return 0 when equal, non-zero otherwise

	if (!q || !r)
	{	return 1;
	}

	switch (op) {
	case EQ:
	case NE:
		switch (e) {
		case fnm_t:	return strcmp(q->fnm, r->fnm);
		case typ_t:	return strcmp(q->typ, r->typ);
		case txt_t:	return strcmp(q->txt, r->txt);
		case fct_t:	return strcmp(fct_which(q), fct_which(r));

		case nxt_t:	return (q->nxt   == r)?0:1;
		case prv_t:	return (q->prv   == r)?0:1;
		case bound_t:	return (q->bound == r)?0:1;
		case jmp_t:	return (q->jmp   == r)?0:1;

		case lnr_t:
		case seq_t:
		case mark_t:
		case curly_t:
		case round_t:
		case bracket_t:
		case len_t:
			if (op == EQ)
			{	numeric_cmp(==);
			} else
			{	numeric_cmp(!=);
			}
			break;
		default:
			break;
		}
		break;

	case GT: numeric_cmp(>);  break; // if not numeric, complain
	case LT: numeric_cmp(<);  break;
	case GE: numeric_cmp(>=); break;
	case LE: numeric_cmp(<=); break;
	default:
		break;
	}
	fprintf(stderr, "error: invalid operator (%d) in constraint\n", op);
	return 1;
}

static int
field_type(const char *s)
{
	switch (s[0]) {
	case 'b':
		if (strcmp(s, "bound") == 0)
		{	return bound_t;
		}
		return (strcmp(s, "bracket") == 0)?bracket_t:0;
	case 'c':
		return (strcmp(s, "curly")   == 0)?curly_t:0;
	case 'f':	
		if (strcmp(s, "fct") == 0)
		{	return fct_t;
		}
		return (strcmp(s, "fnm")     == 0)?fnm_t:0;
	case 'j':
		return (strcmp(s, "jmp")     == 0)?jmp_t:0;
	case 'l':
		if (strcmp(s, "len") == 0)
		{	return len_t;
		}
		return (strcmp(s, "lnr")  == 0)?lnr_t:0;
	case 'm':
		return (strcmp(s, "mark")  == 0)?mark_t:0;
	case 'n':
		return (strcmp(s, "nxt")   == 0)?nxt_t:0;
	case 'p':
		return (strcmp(s, "prv")   == 0)?prv_t:0;
	case 'r':
		return (strcmp(s, "round") == 0)?round_t:0;
	case 's':
		return (strcmp(s, "seq")   == 0)?seq_t:0;
	case 't':
		if (strcmp(s, "txt") == 0)
		{	return txt_t;
		}
		return (strncmp(s, "typ", 3) == 0)?typ_t:0;
	default:
		break;
	}
	return 0;
}

// dot_match can be called from cobra_te.c to evaluate a
// constraint in a pattern match if there are either
// bound variables, names, or regular expressions

#if 0
static void
dt(Lextok *t, int i)
{

	if (t->s) { printf("'%s' ", t->s); }
	if (t->lft)
	{	printf("L: "); dt(t->lft, i+1);
	}
	if (t->rgt)
	{	printf("R: "); dt(t->rgt, i+1);
	}
}
#endif

static int
dot_match(const Prim *q, const int op, Lextok *lft, Lextok *rgt)
{	char *a = (char *) 0;
	char *b = (char *) 0;
	Prim *r = (Prim *) 0;
	int e = 0, le = 0, re = 0;

	if (lft->typ == '.')
	{	le = field_type(lft->s);
		if (!le)
		{	fprintf(stderr, "bad field type '%s'\n", lft->s);
			return -1;
	}	}
	if (rgt->typ == '.')
	{	re = field_type(rgt->s);
		if (!re)
		{	fprintf(stderr, "bad field type '%s'\n", rgt->s);
			return -1;
	}	}

	if (lft->typ == ':')			// lhs is bound var
	{	if (rgt->typ == ':')		// rhs as well
		{	e = 0;
		} else if (rgt->typ == '.')
		{	r = bound_prim(lft->rgt->s); // origin
			e = re;			// cmp bound fields below
		}
		if (lft->rgt && (a = strchr(lft->rgt->s, '.')))
		{	// lhs bound var contains a field like :x.len
			*a = '\0';
			r = bound_prim(lft->rgt->s);
			if (r) { e = field_type(a+1); }
		}

		else if (lft->rgt && lft->rgt->typ == NAME)
		{	r = bound_prim(lft->rgt->s);	// get the ref
		}
	} else  if (rgt->typ == ':')	// rhs is bound var
	{	if (lft->typ == '.')	// rhs refers to token field
		{	r = bound_prim(rgt->rgt->s);	// origin
			e = le;				// lhs ref
		}

		else if (rgt->rgt && rgt->rgt->typ == NAME)
		{	r = bound_prim(rgt->rgt->s);	// get the ref
	}	}


	if (rgt->typ == ':'
	&&  rgt->rgt
	&& (a = strchr(rgt->rgt->s, '.')))
	{	// rhs bound var contains a field like :x.len
		*a = '\0';
		r = bound_prim(rgt->rgt->s);
		if (r) { e = field_type(a+1); }
	}

	// q is the current token that a constraint applies to
	// r is a ref to the location where a bound var is defined
	// e is the field type to use in the comparison
	// e is zero if both lhs and rhs are bound vars or one is a regex
	if (e)
	{	return field_cmp(op, q, r, e);
		// now supports also >,<,>=,<= if field types are numeric
	}

	// two dot fields, bound vars, or one is a regex
	switch (lft->typ) {
	case '.':	a = dot_derive(q, le); break;
	case ':':	a = bound_text(lft->rgt->s); break;
	case REGEX:	a = rgt->s; // ignored: there should be just one regex
	default:	a = lft->s; break;
	}

	switch (rgt->typ) {
	case '.':	b = dot_derive(q, re); break;
	case ':':	b = bound_text(rgt->rgt->s); break;
	case REGEX:	return regex_match(0, a);
	default:	b = rgt->s; break;	// 4.0: was: a = rgt->s
	}
	if (lft->typ == REGEX)
	{	return regex_match(0, b);
	}

	if (!a || !b)
	{	return -1;
	}

	return strcmp(a, b);
}

static int
yylex(void)
{	int n = eval_lex();

	if (0)
	{	printf("yylex: %s <%d>", tokenname(n), n);
		if (n == NAME
		||  n == REGEX)
		{	printf(" = '%s'", yylval->s);
		}
		printf("\n");
	}
	last_tok = n;
	return n;
}

// externally visible function:

#define binop(op)	(evaluate(q, n->lft) op evaluate(q, n->rgt))

#define is_bound(n)	(n->rgt->typ == ':' || n->lft->typ == ':')

static int
do_bound(const Prim *q, const Lextok *n, const int op)
{
	// there is at least one side is a bound variable
	// because is_bound() is true
	// provided bindings were defined

	if (e_bindings)
	{	switch (op) {
		case LT:
		case LE:
		case GT:
		case GE: return dot_match(q, op, n->lft, n->rgt);
		case EQ: 
			return (dot_match(q, EQ, n->lft, n->rgt) == 0);
		case NE: return (dot_match(q, NE, n->lft, n->rgt) != 0);
		default:
			// cannot happen
			break;
	}	}

	return 0; // no bound vars defined
}

int	// also called in cobra_te.c
evaluate(const Prim *q, const Lextok *n)
{	int rval = 0;

	if (n)
	{	switch (n->typ) {
		case '+':  rval = binop(+); break;
		case '!':  rval = !evaluate(q, n->rgt); break;
		case '-':  if (n->lft)
			   {	rval = binop(-);
			   } else
			   {	rval = -evaluate(q, n->rgt);
			   }
			   break;
		case '*':  rval = binop(*); break;
		case '/':
			   rval = evaluate(q, n->rgt);
			   if (rval == 0)
			   {	// division by zero
				fprintf(stderr, "error: divsion by zero\n");
			   } else
			   {	rval = (evaluate(q, n->lft) / rval);
			   }
			   break;
		case '%':
			   rval = evaluate(q, n->rgt);
			   if (rval == 0)
			   {	// division by zero
				fprintf(stderr, "error: modulo of zero\n");
			   } else
			   {	rval = (evaluate(q, n->lft) % rval);
			   }
			   break;
		case  OR:  rval = binop(||); break;
		case AND:  rval = binop(&&); break;
		case  EQ:
			   if (is_bound(n))
			   {	rval = do_bound(q, n, EQ);
				break;
			   }
			   if (n->rgt->typ == NAME
			   ||  n->rgt->typ == REGEX
			   ||  n->lft->typ == NAME
			   ||  n->lft->typ == REGEX)
			   {	rval = (dot_match(q, EQ, n->lft, n->rgt) == 0);
			   } else
			   {	rval = binop(==);
			   }
			   break;
		case  NE:
			   if (is_bound(n))
			   {	rval = do_bound(q, n, NE);
				break;
			   }
			   if (n->rgt->typ == NAME
			   ||  n->rgt->typ == REGEX
			   ||  n->lft->typ == NAME
			   ||  n->lft->typ == REGEX)
			   {	rval = (dot_match(q, NE, n->lft, n->rgt) != 0);
			   } else
			   {	rval = binop(!=);
			   }
			   break;
		case  GT:
			   if (is_bound(n))
			   {	rval = (dot_match(q, n->typ, n->lft, n->rgt) == 0);
			   } else
			   {	rval = binop(>);
			   }
			   break;
		case  LT:
			   if (is_bound(n))
			   {	rval = (dot_match(q, n->typ, n->lft, n->rgt) == 0);
			   } else
			   {	rval = binop(<);
			   }
			   break;
		case  GE:
			   if (is_bound(n))
			   {	rval = (dot_match(q, n->typ, n->lft, n->rgt) == 0);
			   } else
			   {	rval = binop(>=);
			   }
			   break;
		case  LE:
			   if (is_bound(n))
			   {	rval = (dot_match(q, n->typ, n->lft, n->rgt) == 0);
			   } else
			   {	rval = binop(<=);
			   }
			   break;

		case '.':  rval = lookup(q, n->s); break;
		case NR:   rval = n->val; break;
		case SIZE: rval = nr_marks(n->rgt->val); break;
		case ':':  fprintf(stderr, "%s:%d invalid use of ':', expect int, saw string (%s %s)\n",
				q->fnm, q->lnr, n->lft?n->lft->s:"", n->rgt?n->rgt->s:"");
			   // a bound variable reference defaults to a string result
			   // but we expect an integer value here
			   break;
		default:   fprintf(stderr, "expr: unknown type %d\n", n->typ);
			   break;
	}	}
	return rval;
}

Lextok *
prep_eval(void)
{	Lextok *rval = NULL;

	parse_error = 0;
	iscan = 0;
	if (yyparse())
	{	rval = p_tree;
	}
	iscan = 0;
	return rval;
}

int
do_eval(const Prim *q)
{
	if (parse_error)
	{	return 0;
	}
	return evaluate(q, p_tree);
}
