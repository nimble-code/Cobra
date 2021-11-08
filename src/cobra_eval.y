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
		// fall through
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

static int
dot_match(const Prim *q, Lextok *lft, Lextok *rgt)
{	char *a = lft->s;
	char *b = rgt->s;
	char *compare_with = 0;

	if (lft->typ == '.')
	{	if (strcmp(lft->s, "fnm") == 0)
		{	a = q->fnm;
		} else if (strcmp(lft->s, "txt") == 0)
		{	a = q->txt;
		} else if (strcmp(lft->s, "fct") == 0)
		{	a = fct_which(q);
		}
	} else if (lft->typ == REGEX)
	{	compare_with = b;
	} else if (lft->typ == ':')
	{	a = bound_value(lft->rgt->s);
	}

	if (rgt->typ == '.')
	{	if (strcmp(rgt->s, "fnm") == 0)
		{	b = q->fnm;
		} else if (strcmp(rgt->s, "txt") == 0)
		{	b = q->txt;
		}
	} else if (rgt->typ == REGEX)
	{	compare_with = a;
	} else if (rgt->typ == ':')
	{	b = bound_value(rgt->rgt->s);
	}

	if (compare_with)
	{	return regex_match(0, compare_with);
	}

	if (!a || !b)
	{	return -1;
	}

	return strcmp(a, b);
}

#define binop(op)	(evaluate(q, n->lft) op evaluate(q, n->rgt))

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
			   if (n->rgt->typ == ':'
			   ||  n->lft->typ == ':')
			   {	if (e_bindings)
			   	{	rval = (dot_match(q, n->lft, n->rgt) == 0);
				} else
				{	rval = 0; // no match if bound var undefined
			   	}
				break;
			   }
			   if (n->rgt->typ == NAME
			   ||  n->rgt->typ == REGEX
			   ||  n->lft->typ == NAME
			   ||  n->lft->typ == REGEX)
			   {	rval = (dot_match(q, n->lft, n->rgt) == 0);
			   } else
			   {	rval = binop(==);
			   }
			   break;
		case  NE:
			   if (n->rgt->typ == ':'
			   ||  n->lft->typ == ':')
			   {	if (e_bindings)
			   	{	rval = (dot_match(q, n->lft, n->rgt) != 0);
				} else
				{	rval = 0; // no match if bound var undefined
			   	}
				break;
			   }
			   if (n->rgt->typ == NAME
			   ||  n->rgt->typ == REGEX
			   ||  n->lft->typ == NAME
			   ||  n->lft->typ == REGEX)
			   {	rval = (dot_match(q, n->lft, n->rgt) != 0);
			   } else
			   {	rval = binop(!=);
			   }
			   break;
		case  GT:  rval = binop(>);  break;
		case  LT:  rval = binop(<);  break;
		case  GE:  rval = binop(>=); break;
		case  LE:  rval = binop(<=); break;
		case '.':  rval = lookup(q, n->s); break;
		case NR:   rval = n->val; break;
		case SIZE: rval = nr_marks(n->rgt->val); break;
		case ':':  fprintf(stderr, "invalid use of ':'\n");
			   break;
		default:   fprintf(stderr, "expr: unknown type %d\n", n->typ);
			   break;
	}	}
	return rval;
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
