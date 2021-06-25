/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

#include "cobra.h"
#include <regex.h>

// Thompson's algorithm for converting
// regular expressions into NDFA
// CACM 11:6, June 1968, pp. 419-422.

#define NEW_M	  	0
#define OLD_M	  	1

#define CONCAT		512	// higher than operator values
#define OPERAND		513	// higher than others
#define MAX_CONSTRAINT	256

#define te_regmatch(t, q)	regexec((t), (q), 0,0,0)	// t always nonzero

typedef struct List	List;
typedef struct Nd_Stack	Nd_Stack;
typedef struct Node	Node;
typedef struct Op_Stack	Op_Stack;
typedef struct Range	Range;
typedef struct State	State;
typedef struct Trans	Trans;
typedef struct Rlst	Rlst;
typedef struct PrimStack PrimStack;

struct Node {		// NNODE
	int	  type;	// operator or token code 
	char	 *tok;	// token name
	Node	 *nxt;	// linked list
	Nd_Stack *succ;	// successor state
};

#define IMPLICIT	1
#define EXPLICIT	2

// the type marking in the PrimStack records if a
// brace { ( [ from the source was matches explicitly
// with the same character in the pattern,
// or matched implicitly with a dot, or a negated token
// or bound variable.
// this covers most cases, but not all, because the
// NDA can be in multiple states simultaneously, and
// the history of matches can differ between those states
// example: { .* { .* } .* }

struct PrimStack {
	Prim		*t;
	uint		type;	// IMPLICIT or EXPLICIT
	uint		i_state; // for implicit matches, the state
	PrimStack	*nxt;
};

struct Op_Stack {
	int	  op;
	Op_Stack *lst;
};

struct Nd_Stack {	// CNODE
	int	  seq;
	int	  visited;
	Node	 *n;	// NNODE
	int	  cond;	// constraint
	Nd_Stack *lft;	// CNODE
	Nd_Stack *rgt;	// CNODE
	Nd_Stack *nxt;	// stack
	Nd_Stack *pend;	// processing
};

struct Range {
	char	*pat;
	Range	*or;	// linked list
};

struct Trans {
	int	 match;	 // require match or no match
	int	 dest;	 // next state
	int	 cond;	 // constraint
	char	*pat;	 // !pat: if !match: epsilon else '.'
	char	*saveas; // variable binding
	char	*recall; // bound variable reference
	Range	*or;	 // pattern range [ a b c ]
	regex_t	*t;	 // if regular expression
	Trans	*nxt;	 // linked list
};

struct State {
	int	 seq;
	int	 accept;
	Store	*bindings;
	Trans	*trans;
	State	*nxt;	// for freelist only
};

struct Rlst {
	char	*s;
	regex_t	*trex;
	Rlst	*nxt;
};

struct List {
	State	*s;
	List	*nxt;
};

Lextok	*constraints[MAX_CONSTRAINT];

Store	*e_bindings;	// shared with cobra_eval.y

extern char	*b_cmd;
extern char	*yytext;
extern int	 eol, eof;
extern int	 evaluate(const Prim *, const Lextok *);
extern void	 fix_eol(void);
extern void	 set_cnt(int);

static List	*curstates[3]; // 2 is initial
static List	*freelist;
static Match	*free_match;
static Match	*matches;
static Match	*old_matches;
static Match	*del_matches;
static Bound	*free_bound;
static Nd_Stack	*nd_stack;
static Nd_Stack *expand_q;
static Node	*tokens;
static Node	*rev_pol;
static Node	*rev_end;
static Op_Stack	*op_stack;
static Prim	*q_now;
static State	*states;
static Store	*free_stored;
static State	*free_state;
static Rlst	*re_list;
static char	*glob_te = "";
static char	json_msg[128];
static char	bvars[128];

static PrimStack *prim_stack;
static PrimStack *prim_free;

static int	current;
static int	nr_states;
static int	nrtok = 1;
static int	Seq   = 1;
static int	ncalls;
static int	nerrors;
static int	p_matched;
static int	nr_json;
static int	has_positions;

static Nd_Stack *clone_nd_stack(Nd_Stack *);
static int	 check_constraints(Nd_Stack *);
static void	 clr_matches(int);
static void	 free_list(int);
static void	 mk_active(Store *, Store *, int, int);
static void	 mk_states(int);
static void	 mk_trans(int n, int match, char *pat, int dest, int cond);
static void	 show_state(FILE *, Nd_Stack *);

static void
reinit_te(void)
{	int i;

	for (i = 0; i < 3; i++)
	{	free_list(i);
	}

	nd_stack = (Nd_Stack *) 0;
	expand_q = (Nd_Stack *) 0;
	tokens   = (Node *) 0;
	rev_pol  = (Node *) 0;
	rev_end  = (Node *) 0;
	op_stack = (Op_Stack *) 0;
	q_now    = (Prim *) 0;
	states   = (State *) 0;

	current   = 0;
	nr_states = 0;
	nrtok     = 1;
	Seq       = 1;

	clr_matches(OLD_M);

	old_matches = matches;
	matches = (Match *) 0;

	cur = prim;
}

#if 0
static void
list_states(char *tag, int n)
{	List *c;
	Trans *t;

	printf("%s", tag);
	for (c = curstates[n]; c; c = c->nxt)
	{	if (c->s)
		{	for (t = c->s->trans; t; t = t->nxt)
			{	printf("%d->%d%s%s ", c->s->seq, t->dest, c->s->bindings?"*":" ",
					c->s->accept?"+":" ");
				printf("[m=%d, p=%s] ", t->match, t->pat);
				if (t->nxt)
				{	printf("\n%s", tag);
			}	}
			if (c->s->bindings)
			{	Store *bnd = c->s->bindings;
				while (bnd)
				{	printf("<%s::%s> ", bnd->name, bnd->text);
					bnd = bnd->nxt;
			}	}
			if (c->nxt)
			{	printf("\n%s", tag);
		}	}
	}
	printf("\n");
}
#endif

static void
cleaned_up(const char *tp)
{	const char *p = tp;

	while (p && *p != '\0')
	{	switch (*p) {
		case '\\':	// strip, \ or " characters
			printf(" "); // replace with space
			break;
		case '"':
			printf("\\");	// insert escape
			// fall thru
		default:
			printf("%c", *p);
			break;
		}
		p++;
	}
}

static void
json_match(const char *te, const char *msg, const char *f, int ln)
{
	printf("  { \"type\"\t:\t\"");
	 cleaned_up(te);
	printf("\",\n");

	printf("    \"message\"\t:\t\"");
	 cleaned_up(msg);
	printf("\",\n");

	if (json_plus)
	{	if (strlen(bvars) > 0)
		{	printf("    \"bindings\"\t:\t\"%s\",\n", bvars);
		}
		printf("    \"source\"\t:\t\"");
		show_line(stdout, f, 0, ln, ln, -1);
		printf("\",\n");
	}

	printf("    \"file\"\t:\t\"%s\",\n", f);
	printf("    \"line\"\t:\t%d\n", ln);
	printf("  }");
}

static void
te_error(const char *s)
{
	if (json_format)
	{	memset(bvars, 0, sizeof(bvars));
		sprintf(json_msg, "\"error: %.110s\"", s);
		json_match(glob_te, json_msg, "", 0);	// error
	} else
	{	fprintf(stderr, "error: %s\n", s);
	}
	if (cobra_texpr)	// non-interactive
	{	stop_timer(0, 0, "te");
		noreturn();
	}
	nerrors++;
}

// regular expression matching on token text

static regex_t *
te_regstart(const char *s)
{	Rlst *r;
	int n;

	for (r = re_list; r; r = r->nxt)
	{	if (strcmp(r->s, s) == 0)
		{	return r->trex;
	}	}

	r = (Rlst *) emalloc(sizeof(Rlst), 90);
	r->trex = (regex_t *) emalloc(sizeof(regex_t), 90);
	r->s = (char *) emalloc(strlen(s)+1, 110);
	strcpy(r->s, s);

	n = regcomp(r->trex, s, REG_NOSUB|REG_EXTENDED);
	if (n != 0)	// compile
	{	static char ebuf[64];
		regerror(n, r->trex, ebuf, sizeof(ebuf));
		te_error(ebuf);
		return 0;
	}
	r->nxt = re_list;
	re_list = r;

	return r->trex;
}

static void
te_regstop(void)
{
	while (re_list)
	{	regfree(re_list->trex);
		re_list = re_list->nxt;
	}
}
// end regular expression matching on token text

static void
show_transition(FILE *fd, Nd_Stack *from, Nd_Stack *to)
{
	if (to)
	{	if (to->n
		&&  to->n->tok
		&&  strlen(to->n->tok) == 0)	// epsilon transition
		{	show_transition(fd, from, to->n->succ);
		} else
		{	fprintf(fd, "\tN%d -> N%d;\n", from->seq, to->seq);
			show_state(fd, to);
		}
	} else
	{	fprintf(fd, "\tN%d -> accept;\n", from->seq);
	}
}

static void
show_state(FILE *fd, Nd_Stack *now)
{
	if (!now || (now->visited&1))
	{	return;
	}
	now->visited |= 1;

	if (now->n)	// NNODE
	{	assert(now->n->tok);	// operand
		fprintf(fd, "\tN%d [label=\"%s (%d)\" shape=box];\n",
			now->seq, now->n->tok, now->seq);
		show_transition(fd, now, now->n->succ);
	} else	// CNODE
	{	assert(now->lft);
		fprintf(fd, "\tN%d [label=\"choice (%d)\" shape=circle];\n",
			now->seq, now->seq);
		show_transition(fd, now, now->lft);
		show_transition(fd, now, now->rgt);
	}
}

static int
merge_constraint(Nd_Stack *from, Nd_Stack *to)
{	int rv = 1;

	if (!to)
	{	return rv;
	}

	if (to->n	// NNODE
	&&  to->n->tok)
	{	if (strlen(to->n->tok) == 0)	// skip epsilon
		{	return merge_constraint(from, to->n->succ);
		}

		if (*(to->n->tok) == '<'
		&&  isdigit((int) *(to->n->tok + 1)))	// constraint position parameter
		{	from->cond = atoi(to->n->tok + 1);
			if (verbose)
			{	printf("S%d -> S%d (-> S%d) :: %s -- attach constraint <%d>\n",
					from->seq, to->seq,
					to->n->succ?to->n->succ->seq:-1,
					to->n->tok, from->cond);
			}
			if (from->cond < 0
			||  from->cond > MAX_CONSTRAINT
			||  !constraints[from->cond]
			||  !to->n->succ)
			{	fprintf(stderr, "error: undefined constraint %d %s %s\n",
					from->cond,
					(to->n->succ)?"":"(bad position for <n>)",
					(from->n)?"":"(.*)");
				if (!to->n->succ)
				{	fprintf(stderr, "error: cannot handle the <%d> that follows '%s'\n",
						from->cond, from->n?from->n->tok:"?");
				}
				from->cond = 0;
				return 0;	// the only error return
			}
			// skip state defining constraint position, unless debugging
			if (verbose == 2)
			{	goto db;	// preserve the original automaton (for debug only)
			}
			if (from->n)
			{	from->n->succ = to->n->succ;
			} else	// handle case where the position is bound to .* <p>
			{	if (from->rgt && from->rgt->seq == to->seq)
				{	from->rgt = to->n->succ;		// skip over <.> itself
					if (from->lft)
					{	from->lft->cond = from->cond;	// attach constraint to the . part
					}
				} else	// unexpected
				{	fprintf(stderr, "warning: state %d has no node associated with it\n",
						from->seq);
					fprintf(stderr, "lft: %d, rgt: %d -- to: %d\n",
						from->lft?from->lft->seq:-1,
						from->rgt?from->rgt->seq:-1,
						to->seq);
			}	}
db:			to = to->n->succ;
	}	}

	return check_constraints(to);	// from -> to
}

static int
check_constraints(Nd_Stack *now)
{	// merge constraints at given positions into
	// the immediately preceding state
	int rv = 1;

	if (!now || (now->visited&8))
	{	return rv;
	}
	now->visited |= 8;

	if (now->n)
	{	rv = merge_constraint(now, now->n->succ);
	} else
	{	rv = merge_constraint(now, now->lft)
		&&   merge_constraint(now, now->rgt);
	}
	return rv;
}

static void
show_fsm(void)
{	FILE *fd = fopen(CobraDot, "a");

	if (!fd)
	{	fprintf(stderr, "cannot create '%s'\n", CobraDot);
		return;
	}
	fprintf(fd, "digraph re {\n");
	fprintf(fd, "\tnode [margin=0 fontsize=8];\n");
	fprintf(fd, "\tstart [shape=plaintext];\n");
	fprintf(fd, "\taccept [shape=doublecircle label=\"accept (%d)\"];\n", Seq+1);
	fprintf(fd, "\tstart -> N%d;\n", nd_stack->seq);
	show_state(fd, nd_stack);
	fprintf(fd, "}\n");
	fclose(fd);
	if (preserve
	&& !json_format)
	{	printf("wrote '%s'\n", CobraDot);
	}

	if (system(ShowDot) < 0)
	{	perror(ShowDot);
	}
}

static void
to_expand(Nd_Stack *t)
{
	if (t && !(t->visited&4))
	{	t->pend = expand_q;
		expand_q = t;
	}
}

static int
add_token(char *s)
{	Node *n;

	if (0 && strcmp(s, "^") == 0)
	{	te_error("invalid use of ^");
	}

	for (n = tokens; n; n = n->nxt)
	{	if (n->type > OPERAND
		&&  strcmp(n->tok, s) == 0)
		{	return n->type;
	}	}

	n = (Node *) emalloc(sizeof(Node), 91);
	n->type = OPERAND + nrtok++;
	n->tok  = (char *) emalloc(strlen(s)+1, 91);
	strcpy(n->tok, s);

	n->nxt = tokens;
	tokens  = n;

	return n->type;
}

static void
nnode(int op, char *s)
{	Node *t;

	t = (Node *) emalloc(sizeof(Node), 92);
	t->type = op;
	if (s)
	{	t->tok = (char *) emalloc(strlen(s)+1, 92);
		strcpy(t->tok, s);
	}
	if (!rev_end)
	{	rev_pol = rev_end = t;
	} else
	{	rev_end->nxt = t;
		rev_end = t;
	}
}

static void
emit_operator(int op)
{
	nnode(op, NULL);
}

static void
emit_operand(int n, char *s)
{
	nnode(n, s);
}

static void
show_re(void)
{	Node *t;

	// verbose mode
	for (t = rev_pol; t; t = t->nxt)
	{	if (t->tok)			// operand
		{	printf("%s ", t->tok);
		} else if (t->type != CONCAT)
		{	printf("%c ", t->type);	// operator
		} else
		{	printf(". ");
	}	}
	printf("\n");
}

// operator precedence, high to low:
//	()
//	* + ?
//	concat
//	|

static int
higher_precedence(int a, int b)	// does a have higher precendence than b?
{
	switch (a) {
	case '(':
		break;	// incomplete ()
	case ')':
		return 1;
	case '*':
	case '+':
	case '?':
		return (b == CONCAT || b == '|');
	case CONCAT:
		return (b == '|');
	case '|':
		break;
	default:
		te_error("cannot happen, precedence"); // no return
		break;
	}

	return 0;
}

static int
is_metasymbol(char b)
{
	switch (b) {
	case '(':
	case ')':
	case '*':
	case '+':
	case '?':
	case '|':
	case '.':
		return 1;
	case '\\':
	default:
		break;
	}
	return 0;
}

static char *
new_operand(char *s)
{	char tmp, *b = s;

	if (*s == '.') // any
	{	b++;
	} else
	{	if (*b == '['
		|| (*b == '^' && *(b+1)=='['))
		{	while (*b != ']' && *b != '\0')
			{	if (*b == '\\')
				{	b++;
				}
				b++;
			}
			if (*b == '\0')
			{	te_error("missing ]");
			}
			b++;
		} else
		while (*b != '\0'
		&&     *b != ' '
		&&     *b != '\t'
		&&     !is_metasymbol(*b))
		{
			if (*b == '\\')
			{	b++;
			}
			b++;
	}	}

	tmp = *b;
	*b = '\0';
	emit_operand(add_token(s), s);
	*b = tmp;
	return --b;
}

static void
new_operator(int s)
{	Op_Stack *e;

	if (s == ')')
	{	while (op_stack && op_stack->op != '(')
		{	emit_operator(op_stack->op);
			op_stack = op_stack->lst;
		}
		if (op_stack)	// '('
		{	assert(op_stack->op == '(');
			op_stack = op_stack->lst;
		}
	} else
	{	e = (Op_Stack *) emalloc(sizeof(Op_Stack), 93);
		e->op = s;

		// if the operator on the top of the
		// op_stack has higher precedence than s
		// then it is emitted first

		while (op_stack && higher_precedence(op_stack->op, s))
		{	emit_operator(op_stack->op);
			op_stack = op_stack->lst;
		}
	
		if (!op_stack)
		{	op_stack = e;
		} else
		{	e->lst = op_stack;
			op_stack = e;
	}	}
}

static int
soe(char *s)	// start of a new re fragment?
{
	s++;	// next character
	while (*s == ' ' || *s == '\t')
	{	s++;
	}
	
	return !(*s == '\0' ||
		 *s == '*'  ||
		 *s == '+'  ||
		 *s == '?'  ||
		 *s == '|'  ||
		 *s == ')');
}

static void
reverse_polish(char *s)
{
	for ( ; *s != '\0'; s++)
	{	switch (*s) {
		case '(':
		case '|':
			new_operator(*s);
			break;
		case '*':
			while (*(s+1) == '*')
			{	s++;	// a*** -> a*
			}
			// fall thru
		case '+':
		case '?':
		case ')':
			new_operator(*s);
			if (soe(s))
			{	new_operator(CONCAT);
			}
			break;
		case ' ':
		case '\t':	// ignore
			break;

		case '.':	// any
		default:	// token name
			s = new_operand(s);
			if (soe(s))
			{	new_operator(CONCAT);
			}
			break;
	}	}

	// empty op_stack
	while (op_stack)
	{	emit_operator(op_stack->op);
		op_stack = op_stack->lst;
	}
}

static Nd_Stack *
mk_el(Node *n, char *caller)
{	Nd_Stack *t;

	t = (Nd_Stack *) emalloc(sizeof(Nd_Stack), 94);
	t->seq = Seq++;
	t->n = n;
	if (verbose)
	{	printf("---mk_el %d -- tok '%s' typ %d (%s)\n",
			t->seq,
			t->n?t->n->tok:"---",
			t->n?t->n->type:0,
			caller);
	}
	return t;

}

static void
push(Nd_Stack *t)
{
	t->nxt = nd_stack;
	nd_stack = t;

	if (verbose)
	{	printf("\tpush (%s)	<seq %d succ %d, r %d, l %d>\n",
			t->n?t->n->tok:"---",
			t->seq,
			t->n && t->n->succ ? t->n->succ->seq : 0,
			t->rgt?t->rgt->seq:0,
			t->lft?t->lft->seq:0);
	}
}

static void
push_node(Node *n)
{	Nd_Stack *t;

	t = mk_el(n, "push_node");
	push(t);
}

static Nd_Stack *
pop(void)
{	Nd_Stack *t;

	if (!nd_stack)
	{	te_error("cannot happen, nd_stack (did you forget to escape meta-symbols?)");
	}
	t = nd_stack;
	nd_stack = nd_stack->nxt;
	t->nxt = NULL;

	if (verbose && t->n)
	{	printf("\tpop (%s) <seq %d succ %d>\n",
			t->n->tok, t->seq, t->n->succ?t->n->succ->seq:0);
	}

	return t;
}

static void
push_or(Nd_Stack *a, Nd_Stack *b)
{	Nd_Stack *t;

	t = mk_el(NULL, "push_or");
	t->lft = a;
	t->rgt = b;
	push(t);
}

static void
clear_visit(Nd_Stack *t)
{
	if (t)
	{	if (verbose)
		{	printf("---clear-visit %d\n", t->seq);
		}
		t->visited = 0;
		clear_visit(t->lft);
		clear_visit(t->rgt);
	}
}

static void
loop_back(Nd_Stack *a, Nd_Stack *t, int caller)
{
	assert(a != NULL);

	if (verbose)
	{	printf("---connect %d (succ %d) -> %d (succ %d) <caller %d>\n",
			a->seq,
			a->n && a->n->succ?a->n->succ->seq:0,
			t?t->seq:0,
			t->n && t->n->succ?t->n->succ->seq:0,
			caller);
	}
	if (a->visited&2)
	{	if (verbose)
		{	printf("!	%d previously visited!\n", a->seq);
		}
		return;
	}
	a->visited |= 2;

	if (a->n)	// NNODE
	{	if (!a->n->succ)
		{	a->n->succ = t;
			if (verbose)
			{	printf("=\t%d successor is now %d\n",
					a->seq, t->seq);
			}
		} else
		{	int z = a->seq;
			if (verbose)
			{	printf(">\t%d has successor %d, find tail\n",
					z, a->n->succ->seq);
			}
			while (a && a->n && a->n->succ)
			{	a = a->n->succ;
				if (a->seq == z)
				{	fprintf(stderr, "error: loopback cycle on S%d\n", z);
					return;
			}	}
			if (!a || !a->n)	// Cnode
			{	if (a && !a->rgt)
				{	a->rgt = t;
					if (verbose)
					{	printf("Cnode	-- %d attached to rgt of %d\n",
							t?t->seq:-1, a->seq);
					}
				} else if (verbose)
				{	printf("\tCnode (%p) S%d should connect to S%d (l %p, r %p)\n",
						(void *) a, a?a->seq:-1, t->seq,
						(void *) a->lft, (void *) a->rgt);
				}
				goto L;
			}
			a->n->succ = t;
			if (verbose)
			{	printf("=\t%d successor is now %d\n",
					a->seq, t->seq);
				printf("<\n");
		}	}
	} else	// CNODE
	{
L:		assert(a->lft != NULL);
		if (verbose)
		{	printf("Cnode -- loopback lft\n");
		}
		loop_back(a->lft, t, 2);
		if (verbose)
		{	printf("Cnode -- return from loopback lft\n");
		}
		if (!a->rgt)
		{	a->rgt = t;
			if (verbose)
			{	printf("Cnode	-- %d attached to rgt of %d\n",
					t?t->seq:-1, a->seq);
			}
		} else
		{	if (a->rgt->seq == t->seq)
			{	if (verbose)
				{	printf("Cnode -- %d rgt already set correctly to %d\n",
						a->seq, t->seq);
				}
				return;
			}
			if (verbose)
			{	printf("Cnode	-- %d rgt already set to %d, loopback rgt\n",
					a->seq, a->rgt->seq);
			}
			loop_back(a->rgt, t, 3);
			if (verbose)
			{	printf("\tCnode -- return from loopback rgt\n");
	}	}	}
}

static void
push_star(Nd_Stack *a)
{	Nd_Stack *t;

	clear_visit(a);

	t = mk_el(NULL, "push_star");
	loop_back(a, t, 4);
	t->lft = a;
	t->rgt = NULL;	// continuation

	push(t);
}

static Nd_Stack *
epsilon(void)
{	Nd_Stack *t;

	t = mk_el((Node *) emalloc(sizeof(Node), 95), "epsilon");
	t->n->type = add_token("epsilon");
	t->n->tok  = "";
	return t;
}

static void
prep_transitions(Nd_Stack *t, int src)
{	char *s;
	int dst, match = 1;
#ifdef PROTECT
	static int t_depth=0;
	if (t_depth++ > 25000)	// to prevent stack overflow
	{	fprintf(stderr, "cobra: formula too complex, recursion depth exceeded\n");
		memusage();
		noreturn();
	}
#endif
again:	if (t->n)	// NNODE
	{	s = t->n->tok;

		if (*s == '^')
		{	s++;
			if (*s == '.')
			{	te_error("cannot negate '.'");
			} else
			{	match = 0;
		}	}

		if (*s == '\\'
		&& *(s+1) != '"')
		{	s++;
		}
		dst = t->n->succ?t->n->succ->seq:Seq+1;
		if (strcmp(s, ".") == 0)
		{	mk_trans(src, 1, 0, dst, t->cond);
		} else
		{	mk_trans(src, match, s, dst, t->cond);
		}

		to_expand(t->n->succ);
	} else		// CNODE
	{	prep_transitions(t->lft, src);
		if (t->rgt)
		{	t = t->rgt;	// remove tail-recursion
			goto again;	// was: prep_transitions(t->rgt, src);
		} else	// accept
		{	mk_trans(src, 0, 0, Seq+1, t->cond);
	}	}
#ifdef PROTECT
	t_depth--;
#endif
}

static void
prep_state(Nd_Stack *t)
{
	assert(t);
	if (t->visited&4)
	{	return;
	}
	t->visited |= 4;

	prep_transitions(t, t->seq);
}

static void
prep_initial(Nd_Stack *t, int into)
{	int idone = 0;

	assert(!t->n);

	if (t->lft->n)
	{	mk_active(0, 0, t->seq, into);			// initial
		idone++;
	} else
	{	prep_initial(t->lft, into);
	}
	if (t->rgt)
	{	if (t->rgt->n)
		{	if (!idone)
			{	mk_active(0, 0, t->seq, into);	// initial
			}
		} else
		{	prep_initial(t->rgt, into);
		}
	} else	// accept
	{	mk_active(0, 0, Seq+1, into);			// initial
	}
}

static Node *
clone_node(Node *t)
{	Node *e;

	if (!t)
	{	return (Node *) 0;
	}

	e = (Node *) emalloc(sizeof(Node), 96);
	e->type = t->type;
	e->tok  = t->tok;
	e->succ = clone_nd_stack(t->succ);	// 6/11/2021
	// not e->nxt
	return e;
}

static Nd_Stack *
clone_nd_stack(Nd_Stack *t)
{	Nd_Stack *n;

	if (!t)
	{	return (Nd_Stack *) 0;
	}
	if (verbose)
	{	printf("---clone stack (creating seq %d)\n", Seq);
	}
	n = (Nd_Stack *) emalloc(sizeof(Nd_Stack), 97);
	n->seq = Seq++;
	n->n   = clone_node(t->n);
	n->lft = clone_nd_stack(t->lft);
	n->rgt = clone_nd_stack(t->rgt);
	// not n->visited n->nxt or n->pend
	return n;
}

static void
thompson(char *s)
{	Node *re, *re_nxt;
	Nd_Stack *a, *b;

	reverse_polish(s);

	if (verbose > 1)
	{	show_re();
		return;
	}

	for (re = rev_pol; re; re = re_nxt)
	{	re_nxt = re->nxt;
		re->nxt = NULL;
		if (verbose)
		{	switch (re->type) {
			case '*':
			case '+':
			case '?':
			case '|':
				printf("<'%c'>\n", re->type);
				break;
			case 512:
				printf("<'CONCAT'>\n");
				break;
			default:
		//		printf("<'%s'>\n", re->tok);
				break;
		}	}
		switch (re->type) {
		case '|':
			a = pop();
			b = pop();
			push_or(a, b);
			break;
		case '*':
			a = pop();
			push_star(a);
			break;
		case '+':
			a = pop();
			b = clone_nd_stack(a);
			push_star(b);
			b = pop();		// a*
			clear_visit(a);
			loop_back(a, b, 5);	// a.a*
			push(a);
			break;
		case '?':
			a = pop();
			push_or(a, epsilon());
			break;
		case CONCAT:
			a = pop();
			b = pop();
			clear_visit(b);
			loop_back(b, a, 6);

			push(b);
			break;
		default:	// operands
			push_node(re);
			break;
	}	}
}

static void
mk_fsa(void)
{	Nd_Stack *p;

	mk_states(Seq+1);

	to_expand(nd_stack);
	while (expand_q)
	{	p = expand_q;
		expand_q = p->pend;
		prep_state(p);
	}

	if (nd_stack->n)
	{	mk_active(0, 0, 1, 0);			// initial
	} else
	{	prep_initial(nd_stack, 0);
	}
}

static void
mk_states(int nr)					// called from main.c
{	int i;

	nr_states = nr;
	states = (State *) emalloc(sizeof(State)*(nr+1), 98);
	states[nr].accept = 1;
	for (i = 0; i <= nr; i++)
	{	states[i].seq = i;
	}
}

static Range *
new_range(char *s, Range *r)
{	Range *n;

	n = (Range *) emalloc(sizeof(Range), 99);
	n->pat = (char *) emalloc(strlen(s)+1, 99);
	strcpy(n->pat, s);
	n->or = r;

	return n;
}

static Range *
range(char *s)
{	Range *r = (Range *) 0;
	char *a, *b, c;

	assert(*s == '[');

	for (b = s+1; ; b++)
	{	while (*b == ' ' || *b == '\t')
		{	b++;
		}
		a = b;
		while (*b != ' '
		       &&  *b != '\t'
		       &&  *b != ']'
		       &&  *b != '\0')
		{	b++;
		}
		if (b > a)
		{	c = *b;
			*b = '\0';
			r = new_range(a, r);
			*b-- = c;
		} else // did not advance
		{	break;
	}	}

	return r;
}

static void
mk_trans(int src, int match, char *pat, int dest,int cond)	// called from main.c
{	Trans *t;
	char *b, *g;

	assert(src >= 0 && src < nr_states);
	assert(states != NULL);

	t = (Trans *) emalloc(sizeof(Trans), 100);

	if (pat
	&& *pat != '['
	&& (b = strchr(pat, ':')) != NULL)	// possible variable binding
	{	if (b > pat && *(b-1) == '\\')	// x\:zzz => x:zzz
		{	int n = strlen(pat)+1;
			g = emalloc(n*sizeof(char), 101);
			*(b-1) = '\0';
			strncpy(g, pat, n);
			strncat(g, b,   n);
			pat = g;
		} else if (*(b+1) != '\0')	 // ignore x: and :
		{	if (b == pat)		 // variable reference :x
			{	pat = b+1;	 // not really needed
				t->recall = pat; // match the current text to string under this name
			} else
			{	*b++ = '\0';	 // variable binding x:@ident
				t->saveas = pat; // on match, bind current text to "x"
				pat = b;	 // the pattern to match (eg @ident)
	}	}	}

	if (pat
	&& *pat == '/'
	&&  strlen(pat) > 1)
	{	t->t = te_regstart(pat+1);
	}

	t->cond = cond;
	t->match = match;
	t->pat   = pat;
	t->dest  = dest;
	t->nxt   = states[src].trans;
	states[src].trans = t;

	if (pat
	&& *pat == '[')
	{	t->or = range(pat);
	}

	if (verbose > 1)
	{	printf("N%d - %s%s -> N%d;\n",
			src,
			match?"":"!",
			pat?pat:"-nil-",
			dest);
	}
}

static Store *
get_store(Store *n)
{	Store *b;

	if (free_stored)
	{	b = free_stored;
		free_stored = free_stored->nxt;
		b->ref = (Prim *) 0;
	} else
	{	b = (Store *) emalloc(sizeof(Store), 102);
	}

	if (n)
	{	b->name = n->name;
		b->text = n->text;
		b->ref  = n->ref;
	}
	return b;
}

// Execution

char *
bound_value(const char *s)
{	Store*b;

	for (b = e_bindings; b; b = b->nxt)
	{	if (strcmp(s, b->name) == 0)
		{	return b->text;
	}	}
	return "";
}

static void
leave_state(int src, int into)
{	List *c, *lc = NULL;
	Store *b, *nxt_b;

	assert(states != NULL);
	assert(into >= 0 && into < 3);

	for (c = curstates[into]; c; lc = c, c = c->nxt)
	{	assert(c->s);
		if (c->s->seq != src)
		{	continue;
		}

		for (b = c->s->bindings; b; b = nxt_b)
		{	nxt_b = b->nxt;
			b->nxt = free_stored;
			free_stored = b;
		}
		c->s->bindings = 0;

		if (c->s != &states[c->s->seq])
		{	c->s->nxt = free_state;
			free_state = c->s;
		}
		c->s = 0;

		if (lc)
		{	lc->nxt = c->nxt;
		} else
		{	curstates[into] = c->nxt;
		}

		c->nxt = freelist;
		freelist = c;
		break;
	}	
}

static void
mk_active(Store *bnd, Store *new, int src, int into)
{	List *c;
	Store *b, *n;

	// bnd are bindings to be carried forward
	// new is a possible new binding created at this state

	assert(src >= 0 && src <= nr_states);
	assert(states != NULL);
	assert(into >= 0 && into < 3);

	for (c = curstates[into]; c; c = c->nxt)
	{	assert(c->s);
		if (c->s->seq == src)
		{	if (!bnd && !new)
			{	return;
			}
			// first check, if a new binding is to be added, if
			// that binding is already there
			if (new)
			{	for (n = c->s->bindings; n; n = n->nxt)
				{	if (strcmp(new->name, n->name) == 0
					&&  strcmp(new->text, n->text) == 0)
					{	break;	// yes it is
				}	}
				if (!n)	// not there, add it now
				{	Store *x = get_store(new);
					x->nxt = c->s->bindings;
					c->s->bindings = x;
			}	}

			// check if all existing bindings are matched
			for (b = bnd; b; b = b->nxt)	// known bindings
			{	for (n = c->s->bindings; n; n = n->nxt)
				{	if (strcmp(b->name, n->name) == 0
					&&  strcmp(b->text, n->text) == 0)
					{	break;	// found match
				}	}
				if (!n)	// checked all bindings and found NO match for b
				{	Store *x = get_store(b);
					x->nxt = c->s->bindings;
					c->s->bindings = x;	// add it
			}	}
			return;
	}	}

	// add state

	if (freelist)
	{	c = freelist;
		freelist = freelist->nxt;
		c->nxt = (List *) 0;
	} else
	{	c = (List *) emalloc(sizeof(List), 103);
	}

	if (!bnd && !new)
	{	c->s = &states[src];	// no need to allocate a new struct
	} else
	{	if (free_state)
		{	c->s = free_state;
			free_state = free_state->nxt;
			c->s->nxt = 0;
		} else
		{	c->s = (State *) emalloc(sizeof(State), 104);
		}
		memcpy(c->s, &states[src], sizeof(State));

		if (new)
		{	n = get_store(new);
			n->nxt = c->s->bindings;
			c->s->bindings = n;
		}
		for (b = bnd; b; b = b->nxt)
		{	n = get_store(b);
			n->nxt = c->s->bindings;
			c->s->bindings = n;
	}	}

	c->nxt = curstates[into];
	curstates[into] = c;
//	list_states("			", into);

}

static void
copy_list(int src, int dst)
{	List *c;

	assert(src >= 0 && src < 3);
	for (c = curstates[src]; c; c = c->nxt)
	{	mk_active(c->s->bindings, 0, c->s->seq, dst);
	}
}

static void
free_list(int n)
{	List  *c, *nxt_s;
	Store *b, *nxt_b;

	assert(n >= 0 && n < 3);
	for (c = curstates[n]; c; c = nxt_s)
	{	nxt_s = c->nxt;
		c->nxt = freelist;
		freelist = c;
		for (b = c->s->bindings; b; b = nxt_b)
		{	nxt_b = b->nxt;
			b->nxt = free_stored;
			free_stored = b;
		}
		c->s->bindings = 0;
		if (c->s != &states[c->s->seq])
		{	c->s->nxt = free_state;
			free_state = c->s;
		}
		c->s   = 0;
	}
	curstates[n] = 0;
}

static void
add_match(Prim *f, Prim *t, Store *bd)
{	Match *m;
	Store *b;
	Bound *n;

	if (free_match)
	{	m = free_match;
		free_match = m->nxt;
		m->bind = (Bound *) 0;
	} else
	{	m = (Match *) emalloc(sizeof(Match), 105);
	}
	m->from = f;
	m->upto = t;

	for (b = bd; b; b = b->nxt)
	{	if (free_bound)
		{	n = free_bound;
			free_bound = n->nxt;
		} else
		{	n = (Bound *) emalloc(sizeof(Bound), 106);
		}
		n->ref = b->ref;
		n->nxt = m->bind;
		m->bind = n;
	}
	m->nxt = matches;
	matches = m;
	p_matched++;

	if (stream == 1)	// when streaming, print matches when found
	{	Prim *c, *r;
		if (json_format)
		{	printf("%s {\n", (nr_json>0)?",":"[");
			sprintf(json_msg, "lines %d..%d",
				f?f->lnr:0, t?t->lnr:0);
			memset(bvars, 0, sizeof(bvars));
			for (b = bd; b; b = b->nxt)
			{	if (b->ref
				&&  strlen(b->ref->txt) + strlen(bvars) + 3 < sizeof(bvars))
				{	if (bvars[0] != '\0')
					{	strcat(bvars, ", ");
					}
					strcat(bvars, b->ref->txt);
			}	}
			json_match(glob_te, json_msg, f?f->fnm:"", f?f->lnr:0);
			printf("}");
			nr_json++;
		} else
		{	printf("stdin:%d: ", f->lnr);
			for (c = r = f; c; c = c->nxt)
			{	printf("%s ", c->txt);
				if (c->lnr != r->lnr)
				{	printf("\nstdin:%d: ", c->lnr);
					r = c;
				}
				if (c == t)
				{	break;
			}	}
			printf("\n");
		}
		if (verbose && bd && bd->ref)
		{	printf("bound variables matched: ");
			while (bd && bd->ref)
			{	printf("%s ", bd->ref->txt);
				bd = bd->nxt;
			}
			printf("\n");
	}	}
}

static int
is_accepting(Store *b, int s)
{
	if (states[s].accept)
	{	add_match(cur, q_now, b);

		if (verbose)
		{	printf("%s:%d..%d: matches\n",
				cur->fnm, cur->lnr, q_now->lnr);
		}

		free_list(1 - current);		// remove next states
		copy_list(2, 1 - current);	// replace with initial states

//		cur = q_now;	// move ahead, avoid overlap

		return 1;
	}
	return 0;
}

static Store *
saveas(State *s, char *name, char *text)
{	Store *b;

	for (b = s->bindings; b; b = b->nxt)
	{	if (strcmp(b->name, name) == 0
		&&  strcmp(b->text, text) == 0)
		{	return (Store *) 0;	// already there
	}	}

	b = get_store(0);
	b->name = name;
	b->text = text;
	b->nxt  = (Store *) 0;

	return b;
}

static int
recall(State *s, Trans *t, char *text)
{	Store *b, *c;
	char *name = t->recall;

	if (t->match)	// match any
	{	for (b = s->bindings; b; b = b->nxt)
		{	if (strcmp(b->name, name) == 0
			&&  strcmp(b->text, text) == 0)
			{	b->ref = q_now;
				if (verbose)
				{	printf(">> matching var %s : %s\n", name, text);
					printf(">> other bindings:\t");
					for (c = s->bindings; c; c = c->nxt)
					{	if (c != b)
						{	printf("%s :: %s ", c->name, c->text);
					}	}
					printf("\n");
				}
#if 0
				b->nxt = 0;
				s->bindings = b;
#endif
				return 1;
		}	}
		return 0;
	} else		// match none
	{	for (b = s->bindings; b; b = b->nxt)
		{	if (strcmp(b->name, name) == 0
			&&  strcmp(b->text, text) == 0)
			{	b->ref = q_now;
				return 0;
		}	}
		return 1;
	}
}

static void
move2free(PrimStack *n)
{
	if (n)
	{	PrimStack *m = n;
		while (m->nxt)
		{	m = m->nxt;
		}
		m->nxt = prim_free;
		prim_free = n;
	}
}

static void
clr_stacks(void)
{
	move2free(prim_stack);
	prim_stack = (PrimStack *) 0;
}

static void
push_prim(void)
{	PrimStack *n;

	if (prim_free)
	{	n = prim_free;
		n->type = n->i_state = 0;
		prim_free = prim_free->nxt;
	} else
	{	n = (PrimStack *) emalloc(sizeof(PrimStack), 109);
	}
	n->t   = q_now;
	n->nxt = prim_stack;
	prim_stack = n;
}

static void
pop_prim(void)
{	PrimStack *n = prim_stack;

	if (!n)
	{	return;
	}

	prim_stack = n->nxt;

	n->nxt = prim_free;
	prim_free = n;
}

static int
check_level(void)
{	int rv = 0;

	switch (q_now->txt[0]) {	// we know it's 1 character
	case '}':
	case ')':
	case ']':
		if (!prim_stack
		||  !prim_stack->t)
		{	break;		// report error?
		}
		if (prim_stack->t->jmp == q_now)
		{	rv = 2;
		} else
		{	rv = 1;
		}
		pop_prim();
		break;
	case '{':
	case '(':
	case '[':
		push_prim();
		break;
	default:
		break;
	}

	return rv;
}

static void
free_bind(Bound *b)
{
	b->ref = (Prim *) 0;
	b->nxt = free_bound;
	free_bound = b;
}

static Match *
free_m(Match *m)
{	Bound *b, *nb;
	Match *nm;

	nm = m->nxt;

	m->from = (Prim *) 0;
	m->upto = (Prim *) 0;
	m->nxt  = free_match;
	free_match = m;

	for (b = m->bind; b; b = nb)
	{	nb = b->nxt;
		free_bind(b);
	}

	return nm;
}

static void
and_match(void)
{	Match *m, *o, *prv;

	// keep only matches that are *also* in old_matches
	// does not change old_matches, so that undo works
startover:
	prv = (Match *) 0;
	for (m = matches; m; prv = m, m = m->nxt)
	{	for (o = old_matches; o; o = o->nxt)
		{	if (m->from == o->from
			&&  m->upto == o->upto)
			{	break;
		}	}
		if (o)	// it was there
		{	continue;
		}
		if (!prv)
		{	matches = free_m(m);
			goto startover;
		} else
		{	prv->nxt = free_m(m);
	}	}
}

static void
not_match(void)
{	Match *m, *o, *prv;

	// after swapping old_matches and matches
	// keep only matches that are *not* in old_matches

	m = old_matches;
	old_matches = matches;
	matches = m;
	del_matches = (Match *) 0;
			
startover:
	prv = (Match *) 0;
	for (m = matches; m; prv = m, m = m->nxt)
	{	for (o = old_matches; o; o = o->nxt)
		{	if (m->from == o->from
			&&  m->upto == o->upto)
			{	break;
		}	}
		if (!o)	// not there
		{	continue;
		}
		if (!prv)
		{	matches = m->nxt;
			m->nxt = del_matches;
			del_matches = m;
			goto startover;
			return;
		} else
		{	prv->nxt = m->nxt;
			m->nxt = del_matches;
			del_matches = m;
			m = prv;
	}	}

	// make old_matches be the same as the old version of matches
	// to make undo work correctly
	// because add_match works on matches, not old_matches
	// we swap the two, before and after

	clr_matches(OLD_M);		// release old_matches
	old_matches = matches;		// swap
	matches = del_matches;		// the deleted matches
	del_matches = (Match *) 0;	// no longer needed

	for (m = old_matches; m; m = m->nxt)	// add back the rest
	{	Bound *b, *n;

		add_match(m->from, m->upto, 0);	// makes matches non-null

		for (b = m->bind; b && matches; b = b->nxt)
		{	if (free_bound)
			{	n = free_bound;
				free_bound = n->nxt;
			} else
			{	n = (Bound *) emalloc(sizeof(Bound), 107);
			}
			n->ref = b->ref;
			n->nxt = matches->bind;
			matches->bind = n;
	}	}

	m = old_matches;		// swap back
	old_matches = matches;		// the correct undo
	matches = m;			// the new set
}

// externally visible

void
undo_matches(void)
{	Match *tmp;

	tmp = matches;
	matches = old_matches;
	old_matches = tmp;
}

static int
matches2marks(void)
{	Match *m;
	Prim  *p;
	int cnt=0;

	for (m = matches; m; m = m->nxt)
	{	p = m->from;
		p->bound = m->upto;
		if (p)
		{	if (!p->mark)
			{	cnt++;
			}
			p->mark |= 2;	// start of pattern
			for (; p; p = p->nxt)
			{	if (!p->mark)
				{	cnt++;
				}
				p->mark |= 1;
				if (p == m->upto)
				{	p->mark |= 8;
					break;
	}	}	}	}

	clr_matches(NEW_M);
	if (!json_format && !no_match && !no_display)
	{	printf("%d token%s marked\n", cnt, (cnt==1)?"":"s");
	}
	set_cnt(cnt);
	return cnt;
}

int
convert_matches(int n)
{	Match *m;
	Bound *b;
	int seq = 1;
	int w = 0;
	int p = 0;

	if (!matches)
	{	return 0;
	}
	for (m = matches; m; seq++, m = m->nxt)
	{	if (n != 0 && n != seq)
		{	continue;
		}
		p++;
		if (m->bind && !json_format)
		{	if (verbose && w++ == 0)
			{	printf("bound variables matched:\n");
			}
			for (b = m->bind; b; b = b->nxt)
			{	if (!b->ref)
				{	continue;
				}
				b->ref->mark |= 4;	// the bound variable
				w++;
				if (verbose)
				{	printf("\t%d: %s:%d: %s\n", seq,
						b->ref->fnm,
						b->ref->lnr,
						b->ref->txt);
	}	}	}	}

	if (!json_format && !no_match && !no_display)
	{	printf("%d patterns matched\n", p);
		if (w > 0)
		{	printf("%d match%s of bound variables\n",
				w, (w==1)?"":"es");
	}	}

	if (no_match && no_display && w>0)	// ?
	{	return w;
	}

	return matches2marks();
}

static void
clr_matches(int which)
{	Match *m, *nm;
	Bound *b, *nb;

	m = (which == NEW_M) ? matches : old_matches;

	for (; m; m = nm)
	{	nm = m->nxt;
		m->from = (Prim *) 0;
		m->upto = (Prim *) 0;
		m->nxt = free_match;
		free_match = m;
		for (b = m->bind; b; b = nb)
		{	nb = b->nxt;
			b->ref = (Prim *) 0;
			b->nxt = free_bound;
			free_bound = b;
	}	}

	if (which == NEW_M)
	{	matches     = (Match *) 0;
	} else
	{	old_matches = (Match *) 0;
	}
}

void
clear_matches(void)
{
	clr_matches(NEW_M);
}

static
char *
tp_desc(char *t)
{
	if (t && *t != '\0')
	{	return t;
	}
	return " ";
}

static void
show_curstate(int rx)
{	List *c;
	PrimStack *p;
	Store *b;

	// verbose mode
	printf("%d/%d %d :: %s :: %s :: ", cur->lnr, q_now->lnr, rx, cur->txt, q_now->txt);
	for (c = curstates[current]; c; c = c->nxt)
	{	State *s = c->s;
		printf("S%d%s | ", s->seq, s->accept?"*":"");
		for (b = s->bindings; b; b = b->nxt)
		{	printf("<%s::%s> ", b->name, b->text);
		}
	}
	for (p = prim_stack; p; p = p->nxt)
	{	printf("%s... ", p->t?p->t->txt:"?...");
	}
	printf("\n");
}

static void
pattern_full(int which, int N, int M)
{	Prim *p;
	int r = 0, n = 0, k;
	int lst, bv = 0;
	int hits = 0;
	int total = 0;
	FILE *fd = track_fd?track_fd:stdout;

	for (cur = prim; cur; cur = cur?cur->nxt:0)
	{	if (!(cur->mark & 2))	// find start of match
		{	continue;
		}
		if (!cur->bound)
		{	fprintf(fd, "%s:%d: bound not set, skipping\n",
				cur->fnm, cur->lnr);
			continue;
		}
		total++;
		if (N >= 0)
		{	k = cur->lnr;
			for (p = cur, r = 1; p; p = p->nxt)
			{	if (p->lnr != k)
				{	k = p->lnr;
					r++;
				}
				if (p == cur->bound)
				{	break;
			}	}
			if (M < 0)	// json which N
			{	if (r < which || r > N)
				{	continue; // not in range
				}
			} else 		// json which N M
			{	if (r < N || r > M)
				{	continue;
		}	}	}

		n++;	// nr of match

		if ((N < 0 || M > 0)
		&&  which != 0
		&&  n != which)
		{	continue;
		}
		hits++;

		lst = cur->lnr;
		fprintf(fd, "%s:%d..%d", cur->fnm, lst, cur->bound->lnr);
		if (r > 0)
		{	fprintf(fd, " (%d lines)\n", r);
		} else
		{	fprintf(fd, "\n");
		}

		for (p = cur; p; p = p->nxt)
		{	if (p->mark & 4)
			{	fprintf(fd, "bound var '%s', line %d\n", p->txt, p->lnr);
				bv = p->lnr; // keep last one
			}
			if (p == cur->bound)
			{	break;
		}	}
		if (no_display && cur->bound->lnr+4 > lst)	// terse mode
		{	show_line(fd, cur->fnm, 0, lst, lst+1, bv);
			fprintf(fd, " ...\n");
			show_line(fd, cur->fnm, 0, cur->bound->lnr-1, cur->bound->lnr, bv);
		} else
		{	show_line(fd, cur->fnm, 0, lst, cur->bound->lnr, bv);
		}
		
		if (which != 0)
		{	break;
	}	}
	if (which == 0)
	{	fprintf(fd, "%d of %d patterns printed\n", hits, total);
	}
}

static void
dp_usage(const char *s, int nr)
{
	printf("dp: unrecognized option '%s' (nr=%d)\n", s, nr);
	printf("usage (with n N and M numbers):\n");
	printf("    dp  \t	# print all patterns fully\n");
	printf("    dp n\t	# print only the n-th pattern fully\n");
	printf("    dp N M\t	# print all patterns between N and M lines long\n");
	printf("    dp n N M	# print the n-th pattern between N and M lines long\n");
	printf("in terse mode only the first and last 2 lines of each pattern are printed\n");
}

void
display_patterns(const char *te)
{	int j = 0;
	int nr, a, b, c;

	while (te[j] == ' ' || te[j] == '\t')
	{	j++;
	}
	if (isdigit((int) te[j]))
	{	nr = sscanf(&te[j], "%d %d %d", &a, &b, &c);
		switch (nr) {
		case  1: b = -1;	// fall thru
		case  2: c = -1;	// fall thru
		case  3: pattern_full(a, b, c);
			 break;
		default: dp_usage(&te[j], nr);
			 break;
		}
	} else if (te[j] == '\0')
	{	pattern_full(0, -1, -1);
	} else
	{	dp_usage(&te[j], 0);
	}
}

static void
check_bvar(Prim *c)
{
	if ((c->mark & 4)	// bound variable
	&&  strlen(c->txt) + strlen(bvars) + 3 < sizeof(bvars))
	{	if (bvars[0] != '\0')
		{	strcat(bvars, ", ");
		}
		strcat(bvars, c->txt);
	}
}

static Lextok *
parse_constraint(char *c)
{	char *ob = b_cmd;
	char *oy = yytext;
	Lextok *rv = NULL;

	b_cmd = yytext = c;

	rv = prep_eval();

	b_cmd  = ob;
	yytext = oy;

	return rv;
}

static void
store_constraint(int n, Lextok *p)
{
	// when in state n (or at position n), make sure the constraint holds:
	// evaluate(cur, p); // to evaluate expression for current token

	if (n < 0 || n >= MAX_CONSTRAINT)	// the state nr
	{	fprintf(stderr, "error: constraint %d out of range (max %d)\n",
			n, MAX_CONSTRAINT);
		return;
	}
	if (constraints[n]
	&&  p != constraints[n])
	{	fprintf(stderr, "warning: constraint %d redefined\n", n);
	}
	constraints[n] = p;
}

static int
get_positions(char *t)	// formula includes position parameters
{	int n;

	// stop looking when a constraint @n is seen
	while (*t != '\0')
	{	if ((*t == '<' || *t == '@')
		&& isdigit((int) *(t+1)))
		{	n = atoi(t+1);
			if (n >= MAX_CONSTRAINT
			||  n < 0)
			{	printf("error: bad position or constraint nr %d\n", n);
				return -1;
			}
			break;
		}
		t++;
	}

	return (*t == '<');
}

static char *
get_constraints(char *t)
{	char *s, *r, *v, *c, *d;
	int nr, nest;
	Lextok *p;

	for (s = t; *s != '\0'; s++)
	{	if (*s == '@'
		&& isdigit((int) *(s+1)))
		{	break;
	}	}
	if (*s == '\0')
	{	return t;	// no constraints
	}

	v = r = (char *) emalloc(strlen(t)+1, 110);
	for (s = t; *s != '\0'; s++)
	{	if (*s != '@'
		||  !isdigit((int) *(s+1)))
		{	*v++ = *s;
		} else
		{	s++;
			nr = atoi(s);
			while (isdigit((int) *s) || isblank((int) *s))
			{	s++;
			}
			if (*s == '\\')
			{	s++;
			}
			if (*s != '(' || !strchr(s, ')'))
			{	fprintf(stderr, "error: missing '(...)' after @%d, saw '%c'\n", nr, *s);
				r = NULL;
				break;
			}
			c = d = (char *) emalloc(strlen(s)+1, 110);
			nest = 0;
			while (*s != '\0' && (*s != ')' || nest > 1))
			{	if (*s == '(')
				{	nest++;
				} else if (*s == ')')
				{	nest--;
				}
				*d++ = *s++;
			}
			if (*(d-1) == '\\')
			{	*(d-1) = ')';
			} else
			{	*d++ = ')';
			}
			*d = '\0';
			fflush(stdout);
			*(d-1) = '\0';	// omit ()
			p = parse_constraint(c+1);
			if (p)
			{	store_constraint(nr, p);
				if (verbose)
				{	printf("constraint %d = '%s' -- parses: %p\n",
						nr, c+1, (void *) p);
				}
			} else
			{	if (verbose)
				{	fprintf(stderr, "constraint %d (%s) fails to parse\n", nr, c+1);
				}
				r = NULL;
				break;
	}	}	}

	return r;	
}

void
json(const char *te)
{	Prim *sop = (Prim *) 0;
	Prim *q;
	int seen = 0;

	// usage:
	//	json		# prints json format summary of all matches
	//	json string	# puts string the type field

	// tokens that are part of a match are marked         &1
	// the token at start of each pattern match is marked &2
	// tokens that match a bound variable are marked      &4
	// the token at end of each pattern match is marked   &8

	// if the json argument is a number or a single *, then
	// the output is verbose for that match (or all, for a *)

	memset(bvars, 0, sizeof(bvars));
	printf("[\n");
	for (cur = prim; cur; cur = cur?cur->nxt:0)
	{	if (cur->mark)
		{	seen |= 2;
		}
		if (!(cur->mark & 2))	// find start of match
		{	continue;
		}
		if (sop)
		{	printf(",\n");
		}
		// start of match
		sop = cur;

		q = cur;
		if (q->bound)	// assume this wasnt set for other reasons...
		{	sprintf(json_msg, "lines %d..%d", q->lnr, q->bound->lnr);
			while (q && q->mark > 0)
			{	check_bvar(q);
				q = q->nxt;
			}
		} else
		{	while (q && q->mark > 0)
			{	check_bvar(q);
				if (q->nxt)
				{	q = q->nxt;
				} else
				{	break;
			}	}
			sprintf(json_msg, "lines %d..%d", sop->lnr, q?q->lnr:0);
		}
		json_match(te, json_msg, sop?sop->fnm:"", sop?sop->lnr:0);
		seen |= 4;
	}
	if (seen == 2)	// saw marked tokens, but no patterns, find ranges to report
	{	for (cur = prim; cur; cur = cur?cur->nxt:0)
		{	if (!cur->mark)
			{	continue;
			}
			sop = cur;
			while (cur && cur->mark)
			{	if (cur->nxt)
				{	cur = cur->nxt;
				} else
				{	break;
			}	}
			sprintf(json_msg, "lines %d..%d", sop->lnr, cur?cur->lnr:0);
			json_match(te, json_msg, sop?sop->fnm:"", sop?sop->lnr:0);
		}
	}
	printf("\n]\n");
}

void
cobra_te(char *te, int and, int inv)	// fct is too long...
{	List	*c;
	Trans	*t;
	State	*s;
	char	*m;
	char	*p;
	int	anychange;
	int	rx;
	int	tmx;
	int	mustbreak = 0;
	Trans	dummy;

	memset(&dummy, 0, sizeof(Trans));

	if (!te)
	{	return;
	}

	has_positions = get_positions(te);
	if (has_positions < 0)
	{	return;
	}

	te = get_constraints(te);

	start_timer(0);
	glob_te = te;

	p = te;
	for (p = te; *p != '\0'; p++)
	{	if (*p == ':'
		&&   p > te+1
		&& *(p+1) != '@'
		&& *(p+1) != ' ' 
		&& *(p-1) != ' '
		&& *(p-1) != '^'
		&& *(p-1) != '[')
		{	fprintf(stderr, "warning: is a space missing before : in '%c:%c'?\n",
				*(p-1), *(p+1));
	}	}
	anychange = 0;
	if (!eol && strstr(te, "EOL"))
	{	char *q1 = strstr(te, "define");
		char *q2 = strstr(te, "EOL");
		// with nocpp, there is already an EOL at the end
		// of every #define macro
		if (!no_cpp || (!q1 || q1 > q2))
		{	anychange = 1;
			eol = 1;
	}	}
	if (!eof && strstr(te, "EOF"))
	{	anychange = 1;
		eof = 1;
	}
	if (anychange)
	{	fix_eol();
		if (!json_format && !no_match && !no_display)
		{	fprintf(stderr, "warning: use of EOL/EOF requires -eol/-eof cmdln arg\n");
			fprintf(stderr, "         rescanned input\n");
	}	}

	if (ncalls++ > 0)
	{	reinit_te();
		nerrors = 0;
	}

	thompson(te);
	if (!nd_stack || nerrors > 0)
	{	te_regstop();
		stop_timer(0, 0, "te");
		return;
	}
	if (nd_stack->nxt)	// error
	{	Nd_Stack *q = nd_stack;
		while (q)
		{	printf("\tseq %d\n", q->seq);
			printf("\tvis %d\n", q->visited);
			printf("\tn   %s\n", q->n?q->n->tok:"--");
			printf("\tlft %p\n", (void *) q->lft);
			printf("\trgt %p\n", (void *) q->rgt);
			printf("\tpnd %p\n", (void *) q->pend);
			printf("---\n");
			q = q->nxt;
		}
		assert(!nd_stack->nxt);
	}

	if (has_positions)
	{	// merge constraints into the right states
		if (!check_constraints(nd_stack))
		{	return;	// reported an error
	}	}

	if (p_debug)
	{	printf("in: \"%s\"\n", te);
		show_fsm();
		stop_timer(0, 0, "te");
		return;
	}
	mk_fsa();

	copy_list(current, 2);	// remember initial states, 2 was empty so far

	// if reading from stdin (stream) this gives us an
	// initial token stream fragment of stream_lim lines
	// which allows for the required read-ahead to match the
	// pattern in the nested for-loop on q_now

	if (stream == 1)
	{	while (add_stream(0)) // make sure we have enough tokens
		{	if (plst && plst->seq > stream_lim)
			{	break;	// stream could start with many comments
	}	}	}

	// when the main for-loop below gets to the end of the initial range,
	// we extend the sequence further with a call to add_stream(),
	// until that returns null; the value of cur can change in that call

	for ( ; cur; cur = cur->nxt)
	{	free_list(current);
		free_list(1 - current);
		copy_list(2, current);	// initial state
		clr_stacks();

		if (stream == 1
		&&  cur->seq + 100 > plst->seq)
		{	Prim *place = cur;
			if (add_stream(cur))	// can free up to cur
			{	cur = place;
			} else	// exhausted input stream
			{	stream = 2;
		}	}

		for (q_now = cur; q_now; q_now = q_now->nxt)
		{
			if (q_now->fnm != cur->fnm)	// no match across files
			{	break;
			}
			anychange = 0;
			if (*(q_now->txt+1) == '\0')
			{	rx = check_level();	// try to match }, ), ] to {, (, [
			} else
			{	rx = 0;
			}
			if (verbose)
			{	show_curstate(rx);
			}

			for (c = curstates[current]; c; c = c->nxt)
			{	s = c->s;

				if (!has_positions
				&&  s->seq < MAX_CONSTRAINT
				&&  constraints[s->seq])
				{	// setup bindings env for cobra_eval.y
					// in case the constraint contains bound vars
					e_bindings = s->bindings;
					if (!evaluate(q_now, constraints[s->seq]))
					{	if (verbose)
						{	printf("S%d: constraint does not hold\n", s->seq);
						}
						continue;
				}	}

				for (t = s->trans; t; t = t->nxt)
				{	if (verbose)
					{	printf("\tcheck %d->%d	%s, pat: '%s',  %s,  %s\t:: nxt S%d",
							s->seq, t->dest,
							t->match?"match":"-nomatch-",
							t->pat?t->pat:" -nopat- ",
							t->recall?"recall":"-norecall-",
							t->or?"or":"no_or",
							(c->nxt && c->nxt->s)?c->nxt->s->seq:-1);

						if (t->cond)
						{	printf(" <constraint %d>\n", t->cond);
						} else
						{	printf("\n");
					}	}

					if (has_positions
					&&  t->cond)
					{	if (!evaluate(q_now, constraints[t->cond]))
						{	if (verbose)
							{	printf("\tconstraint %d fails (S%d -> S%d)\n",
									t->cond, s->seq, t->dest);
							}
							continue;
						} else if (verbose)
						{	printf("\t%s:%d: constraint %d holds (%d) (S%d -> S%d)\n",
								q_now->fnm, q_now->lnr,
								t->cond,
								(q_now->jmp?q_now->jmp->lnr:0) - q_now->lnr,
								s->seq, t->dest);
					}	}

					if (!t->pat)	// epsilon move, eg .*
					{	if (!t->match) // can this happen?
						{	if (q_now->prv)
							{ q_now = q_now->prv;
							} else
							{ if (json_format)
							  { sprintf(json_msg, "\"re error for . (S%d->S%d)\"",
								s->seq, t->dest);
							    memset(bvars, 0, sizeof(bvars));
							    json_match(te, json_msg, cur->fnm, cur->lnr); // error
							  } else
							  { printf("%s:%d: re error for . (s%d->s%d) <%p>\n",
								cur->fnm, cur->lnr,
								s->seq, t->dest,
								(void *) t->nxt);
							  }
							  break;
							}
						} else
						{	// intercept special case where
							// the source is } ) or ], and the pattern is . (dot)
							// no match if the brace closed an interval (rx != 0)
							if (q_now->txt[0] == '{'
							||  q_now->txt[0] == '('
							||  q_now->txt[0] == '[')
							{	if (verbose)
								{	printf("\timplicit . match of %s\n",
										q_now->txt);
								}
								assert(prim_stack);
								prim_stack->type |= IMPLICIT;
								if (Seq < 8*sizeof(uint))
								{	prim_stack->i_state |= (1 << s->seq);
								} else
								{	prim_stack->i_state = s->seq;
								}
							} else if (rx)
							{	int mismatch;
								// implicit match
								if (verbose)
								{	printf("\timplicit . match of '%s' :: ",
										q_now->txt);
								}
								assert(prim_free);
								if (Seq < 8*sizeof(uint))
								{	mismatch = !(prim_free->i_state & (1 << s->seq));
								} else
								{	mismatch = (prim_free->i_state != s->seq);
								}
								if (!(prim_free->type & IMPLICIT)
								||  mismatch)
								{	// not a match
									if (verbose)
									{	printf("rejected [%d %d %d]\n",
											prim_free->type,
											prim_free->i_state,
											s->seq);
									}
									continue;
								} else
								{	if (verbose)
									{	printf("accepted (%d)\n",
											prim_free->type);
						}	}	}	}

						mk_active(s->bindings, 0, t->dest, 1-current);
						anychange = 1;
						if (is_accepting(s->bindings, t->dest))
						{	goto L;
						}
						continue;
					}

					if (t->recall)	// bound variable ref
					{	if (recall(c->s, t, q_now->txt))
						{
							if (!t->match
							&&  (q_now->txt[0] == '{'
							  || q_now->txt[0] == '('
							  || q_now->txt[0] == '['))
							{	if (verbose)
								{	printf("\timplicit ^ match of %s\n",
										q_now->txt);
								}
								assert(prim_stack);
								prim_stack->type |= IMPLICIT;
								if (Seq < 8*sizeof(uint))
								{	prim_stack->i_state |= (1 << s->seq);
								} else
								{	prim_stack->i_state = s->seq;
								}
							}
							if (!t->match && rx)
							{	int mismatch;
								// implicit match, prim_stack just popped
								if (verbose)
								{	printf("\timplicit ^ MATCH of '%s' :: ",
										q_now->txt);
								}
								assert(prim_free);
								if (Seq < 8*sizeof(uint))
								{	mismatch = !(prim_free->i_state & (1 << s->seq));
								} else
								{	mismatch = (prim_free->i_state != s->seq);
								}
								if (!(prim_free->type & IMPLICIT)
								||   mismatch)
								{	// not a match
									if (verbose)
									{	printf("rejected.\n");
									}
									continue;
								}
								if (verbose)
								{	printf("accepted (%d)\n",
										prim_free->type);
								}
							}
							mk_active(s->bindings, 0, t->dest, 1-current);
							anychange = 1;
							if (is_accepting(s->bindings, t->dest))
							{	goto L;
						}	}
						continue;
					}

					if (t->or)		// a range
					{	Range *r;
						if (t->match)	// normal or-series
						{	for (r = t->or; r; r = r->or)
							{	if (*(r->pat) == '@')
								{	m = tp_desc(q_now->typ);
									p = r->pat+1;
								} else
								{	m = q_now->txt;
									p = r->pat;
									if (strcmp(p, ".") == 0)
									{	goto is_match;
									}
								}

								// embedded regex inside range
								if (*p == '/'
								&&  strlen(p) > 1)
								{	regex_t *z = te_regstart(p+1);
									tmx = te_regmatch(z, m);
								} else if (*p == '\\')
								{	tmx = strcmp(m, p+1);
								} else
								{	if (*(r->pat) == '@'
									&&  strcmp(p, "const") == 0)
									{	tmx = strncmp(m, "const", 5);
									} else
									{	tmx = strcmp(m, p);
								}	}

								if (tmx == 0)
								{	if (verbose)
									{	printf("regex matches -> accept\n");
									}
									goto is_match;
								}

								if (*p == ':' && *(p+1) != '\0')
								{	dummy.recall = (p+1);
									if (verbose)
									{	printf("\tbound var ref '%s' vs '%s'\n",
											p, m);
									}
									if (recall(c->s, &dummy, m))
									{	if (verbose)
										{	printf(" matched\n");
										}
										goto is_match;
									} else if (verbose)
									{	printf(" not matched\n");
									}
								} else if (strcmp(m, p) == 0)
								{	goto is_match;
							}	}
							continue;	// no match
						} else			// negated
						{	for (r = t->or; r; r = r->or)
							{	if (*(r->pat) == '@')
								{	m = tp_desc(q_now->typ);
									p = r->pat+1;
									if (strcmp(m, p) == 0
									||  (strcmp(p, "const") == 0
									&&   strncmp(m, "const", 5) == 0))
									{	break;
									}
								} else
								{	m = q_now->txt;
									p = r->pat;

									// embedded regex inside range
									if (*p == '/'
									&&  strlen(p) > 1)
									{	regex_t *z = te_regstart(p+1);
										tmx = te_regmatch(z, m);
									} else if (*p == '\\')
									{	tmx = strcmp(m, p+1);
									} else
									{	tmx = strcmp(m, p);
									}
									if (tmx == 0)
									{	if (verbose)
										{	printf("regex matches -> reject\n");
										}
										break; // no match
									}

									if (*p == ':' && *(p+1) != '\0')
									{	dummy.recall = (p+1);
										if (verbose)
										{	printf("\tbound var ^'%s' vs '%s'\t",
												p, m);
										}
										if (recall(c->s, &dummy, m))
										{	if (verbose)
											{	printf(" matching\n");
											}
											continue;
										} else if (verbose)
										{	printf(" not matching\n");
										}
										break;
									} else if (strcmp(m, p) == 0)
									{	break;
							}	}	}
							if (r)
							{	continue;
							}
							goto is_match;
					}	}

					if (*(t->pat) == '@')
					{	m = tp_desc(q_now->typ);
						p = t->pat+1;
					} else
					{	m = q_now->txt;
						p = t->pat;
						// e.g., {@0
						if (p[1] == '@' && isdigit((int) p[2]))
						{	int nr = atoi(&p[2]);
							switch (p[0]) {
							case '{':
								if (q_now->curly == nr)
								{	p = "{";
								} // else leave as nonmatch
								break;
							case '(':
								if (q_now->round == nr)
								{	p = "(";
								} // else leave as nonmatch
								break;
							case '[':
								if (q_now->bracket == nr)
								{	p = "[";
								} // else leave as nonmatch
								break;
							default:
								break;
							}
						}
					}

					// match==0 && m != p	m: tokentext, p: pattern
					// match==1 && m == p

					if (*p == '/'
					&&  strlen(p) > 1) // regexpr, could be: t->t != NULL
					{	tmx = te_regmatch(t->t, m);
					} else if (*p == '\\')
					{	tmx = strcmp(m, p+1);
					} else
					{	if (*(t->pat) == '@'
						&&  strcmp(p, "const") == 0)
						{	tmx = strncmp(m, "const", 5);
						} else
						{	tmx = strcmp(m, p);
					}	}
					if (t->match != (tmx != 0))
					{	if (*(m+1) == '\0')
						{	// if it was a } ) or ] then rx could be non-zero
							if (t->match)
							{	if (*m == '{' || *m == '(' || *m == '[')
								{	// explicit match of { ( [
									if (verbose)
									{	printf("\texplicit match of %s\n", m);
									}
									assert(prim_stack);
									prim_stack->type |= EXPLICIT;
								}
								if (rx)
								{	// explicit match of } ) ]
									if (verbose)
									{	printf("\texplicit match of '%s' :: ", m);
									}
									// prim_stack popped
									assert(prim_free);
									if (!(prim_free->type & EXPLICIT))
									{	// not a match
										if (verbose)
										{	printf("rejected..\n");
										}
										continue;
									}
									if (verbose)
									{	printf("accepted\n");
									}
									if (rx == 2)
									{	mustbreak = 1;
										leave_state(c->s->seq, 1-current);
						}	}	}	}
		is_match:
						if (t->saveas)
						{	Store *b = saveas(s, t->saveas, q_now->txt);
							if (verbose)
							{	printf("==%s:%d: New Binding: %s -> %s\n",
									q_now->fnm, q_now->lnr,
									t->saveas, q_now->txt);
							}
							mk_active(s->bindings, b, t->dest, 1-current);
						} else
						{	mk_active(s->bindings, 0, t->dest, 1-current);
						}
						anychange = 1;
						if (is_accepting(s->bindings, t->dest))
						{	if (verbose)
							{	printf("\t\tgoto L (%d -> %d) anychange: %d\n",
									s->seq, t->dest, anychange);
							}
							// we found a match at the current token
							// so even though there may be others, we
							// move on; is_accepting() already cleared
							// the states
							goto L;
						}
					}	// if matching, may set anychange
					if (mustbreak)	// forced exit from this state
					{	mustbreak = 0;
						break;
					}
				}		// for each possible transistion
			}			// for each current state

		L:	mustbreak = 0;
			free_list(current);
			if (anychange)
			{	current = 1 - current;	// move forward
			} else
			{	break;			// no match of pattern, Next
			}
		}	// for-loop match attempt on token q_now
	}		// for-loop starting match attempt

	te_regstop();

	if (!cobra_texpr)	// interactive
	{	Match *x;

		if (and)
		{	and_match();
		} else if (inv)
		{	not_match();
		}
		for (rx = 0, x = matches; x; x = x->nxt)
		{	rx++;
		}
		if (convert_matches(0))
		{	if (json_format)
			{	json(te);
				clear_matches();
		}	}
		rx = 0;
	} else if (stream <= 0)	// not streaming
	{	if (convert_matches(0) > 0)
		{	if (json_format)
			{	json(te);
			} else
			{	display("", "");
			}
		} else
		{	if (json_format)
			{	if (nr_json > 0)
				{	printf("]\n");
				}
			} else if (!no_display && !no_match)
			{	printf("0 patterns matched\n");
		}	}
	} else			// streaming input
	{	if (json_format)
		{	if (nr_json > 0)
			{	printf("]\n");
			}
		} else if (!no_display && !no_match)
		{	printf("%d pattern%s matched\n",
				p_matched, p_matched==1?"":"s");
	}	}
	stop_timer(0, 0, "te");
}
