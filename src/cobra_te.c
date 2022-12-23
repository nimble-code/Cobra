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
typedef struct Snapshot	Snapshot;

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

struct Snapshot {
	int		 n;	// nr explicit { seen
	PrimStack	*p;	// ptr to the details
};	// one for each of the nr_states

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

extern Match	*matches;	// cobra_json.c
extern Match	*old_matches;	// cobra_json.c


extern char	*b_cmd;
extern char	*yytext;
extern int	 echo;
extern int	 eol, eof;
extern int	 across_file_match;	// global flag
extern int	 evaluate(const Prim *, const Lextok *);
extern int	 has_suppress_tags;

extern void	fix_eol(void);
extern int	matches_suppress(const Prim *, const char *);

static List	*curstates[3]; // 2 is initial
static List	*freelist;
static Nd_Stack	*nd_stack;
static Nd_Stack *expand_q;
static Node	*tokens;
static Node	*rev_pol;
static Node	*rev_end;
static Op_Stack	*op_stack;
static Prim	*q_now;
static Snapshot	**snapshot;
static Snapshot **snap_pop;
static State	*states;
static Store	*free_stored;
static State	*free_state;
static Rlst	*re_list;
static char	SetName[256];	// for a new named set of pattern matches
static char	*pattern_filter;	// filename filter on dp output, pattern_full and pattern_matched

static PrimStack *prim_stack;
static PrimStack *prim_free;

static int	current;
static int	nr_states;
static int	nrtok = 1;
static int	opened;		// opened json object
static int	closed;		// closed json object
static int	Seq   = 1;
static int	ncalls;
static int	nerrors;
static int	has_positions;

static Nd_Stack *clone_nd_stack(Nd_Stack *);
static int	 check_constraints(Nd_Stack *);
static void	 free_list(int);
static void	 mk_active(Store *, Store *, int, int, int);
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

static void
te_error(const char *s)
{
	fprintf(stderr, "error: %s...\n", s); // errors always to stderr, not in json format

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
		fprintf(stderr, "saw: '%s'\n", s);
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
			{	if (!to->n->succ)
				{	if (0)	// it's recoverable
					{	fprintf(stderr, "error: cannot handle the <%d> that follows '%s'\n",
							from->cond, from->n?from->n->tok:"?");
					}
				} else
				{	fprintf(stderr, "error: undefined constraint %d %s %s\n",
						from->cond,
						(to->n->succ)?"":"(bad position for <n>)",
						(from->n)?"":"(.*)");
					from->cond = 0;
					return 0;	// the only error return
			}	}

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
	if (verbose>1)
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

	if (verbose>1)
	{	printf("\tpush (%s)	<seq %d succ %d, r %d, l %d>\n",
			t->n?t->n->tok:"---",
			t->seq,
			t->n && t->n->succ ? t->n->succ->seq : 0,
			t->rgt?t->rgt->seq:0,
			t->lft?t->lft->seq:0);
	}
}

static void
strip_backslash(Node *n)
{	char *q, *p = n->tok;

	// if there's a backslash before a +, * or ?
	// inside the token text (not at the start)
	// we should now remove it, because we've
	// already passed interpretation of meta-symbols

	while (p)
	{	q = ++p;
		if ((p = strstr(q, "\\+")) != NULL	// eg ++
		||  (p = strstr(q, "\\*")) != NULL)	// eg **
		{	strcpy(p, p+1);
	}	}
}

static void
push_node(Node *n)
{	Nd_Stack *t;

	strip_backslash(n);
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

	if (verbose>1 && t->n)
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
	{	if (verbose>1)
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

	if (verbose>1)
	{	printf("---connect %d (succ %d) -> %d (succ %d) <caller %d>\n",
			a->seq,
			a->n && a->n->succ?a->n->succ->seq:0,
			t?t->seq:0,
			t->n && t->n->succ?t->n->succ->seq:0,
			caller);
	}
	if (a->visited&2)
	{	if (verbose>1)
		{	printf("!	%d previously visited!\n", a->seq);
		}
		return;
	}
	a->visited |= 2;

	if (a->n)	// NNODE
	{	if (!a->n->succ)
		{	a->n->succ = t;
			if (verbose>1)
			{	printf("=\t%d successor is now %d\n",
					a->seq, t->seq);
			}
		} else
		{	int z = a->seq;
			if (verbose>1)
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
					if (verbose>1)
					{	printf("Cnode	-- %d attached to rgt of %d\n",
							t?t->seq:-1, a->seq);
					}
				} else if (verbose>1)
				{	printf("\tCnode (%p) S%d should connect to S%d (l %p, r %p)\n",
						(void *) a, a?a->seq:-1, t->seq,
						(void *) a->lft, (void *) a->rgt);
				}
				goto L;
			}
			a->n->succ = t;
			if (verbose>1)
			{	printf("=\t%d successor is now %d\n",
					a->seq, t->seq);
				printf("<\n");
		}	}
	} else	// CNODE
	{
L:		assert(a->lft != NULL);
		if (verbose>1)
		{	printf("Cnode -- loopback lft\n");
		}
		loop_back(a->lft, t, 2);
		if (verbose>1)
		{	printf("Cnode -- return from loopback lft\n");
		}
		if (!a->rgt)
		{	a->rgt = t;
			if (verbose>1)
			{	printf("Cnode	-- %d attached to rgt of %d\n",
					t?t->seq:-1, a->seq);
			}
		} else
		{	if (a->rgt->seq == t->seq)
			{	if (verbose>1)
				{	printf("Cnode -- %d rgt already set correctly to %d\n",
						a->seq, t->seq);
				}
				return;
			}
			if (verbose>1)
			{	printf("Cnode	-- %d rgt already set to %d, loopback rgt\n",
					a->seq, a->rgt->seq);
			}
			loop_back(a->rgt, t, 3);
			if (verbose>1)
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
	{	mk_active(0, 0, t->seq, t->seq, into);			// initial
		idone++;
	} else
	{	prep_initial(t->lft, into);
	}
	if (t->rgt)
	{	if (t->rgt->n)
		{	if (!idone)
			{	mk_active(0, 0, t->seq, t->seq, into);	// initial
			}
		} else
		{	prep_initial(t->rgt, into);
		}
	} else	// accept
	{	mk_active(0, 0, Seq+1, Seq+1, into);			// initial
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

	if (verbose > 2)
	{	show_re();
		return;
	}

	for (re = rev_pol; re; re = re_nxt)
	{	re_nxt = re->nxt;
		re->nxt = NULL;
		if (verbose>1)
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
	{	mk_active(0, 0, 1, 1, 0);		// initial
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
#if 1
	snapshot = (Snapshot **) emalloc(sizeof(Snapshot *)*(nr+1), 98);
	snap_pop = (Snapshot **) emalloc(sizeof(Snapshot *)*(nr+1), 98);
	for (i = 0; i < nr+1; i++)
	{	snapshot[i] = (Snapshot *) emalloc(sizeof(Snapshot), 98);
		snap_pop[i] = (Snapshot *) emalloc(sizeof(Snapshot), 98);
	}
#endif
}

static void
copy_snapshot(int from, int into)
{
	if (from == into)
	{	return;
	}
	assert(from >= 0 && from <= Seq+1);
	assert(into >= 0 && into <= Seq+1);
#if 0
	snapshot[into] = snapshot[from];				// move, or copy?
	snapshot[from] = (Snapshot *) emalloc(sizeof(Snapshot), 68);	// recycle?
#else
	memcpy(snapshot[into], snapshot[from], sizeof(Snapshot));
#endif
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
mk_trans(int src, int match, char *pat, int dest, int cond)	// called from main.c
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
		if (!t->t)
		{	return;
		}
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
		b->bdef = (Prim *) 0;
		b->ref  = (Prim *) 0;
	} else
	{	b = (Store *) emalloc(sizeof(Store), 102);
	}

	if (n)
	{	b->name = n->name;
		b->text = n->text;
		b->bdef = n->bdef;
		b->ref  = n->ref;
	}
	return b;
}

// Execution

Prim *
bound_prim(const char *s)
{	Store*b;

	for (b = e_bindings; b; b = b->nxt)
	{	if (strcmp(s, b->name) == 0)
		{	return b->bdef;
	}	}
	return (Prim *) NULL;
}

char *
bound_text(const char *s)
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
mk_active(Store *bnd, Store *new, int orig, int src, int into)
{	List *c;
	Store *b, *n;

	// moving from state orig to state src
	// if orig == src then it's an initialization

	// bnd are bindings to be carried forward
	// new is a possible new binding created at this state

	assert(orig >= 0 && orig <= nr_states);
	assert(src  >= 0 && src  <= nr_states);
	assert(states != NULL);
	assert(into >= 0 && into < 3);
#if 1
	copy_snapshot(orig, src);
#endif
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
	if (verbose)
	{	list_states("			", into);
	}
}

static void
copy_list(int src, int dst)
{	List *c;

	assert(src >= 0 && src < 3);
	for (c = curstates[src]; c; c = c->nxt)
	{	mk_active(c->s->bindings, 0, c->s->seq, c->s->seq, dst);
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

static int
is_accepting(Store *b, int s)
{
	if (states[s].accept)
	{	add_match(cur, q_now, b);

		if (verbose)
		{	printf("%s:%d..", cur->fnm, cur->lnr);
			if (strcmp(cur->fnm, q_now->fnm) != 0)
			{	printf("%s:", q_now->fnm);
			}
			printf("%d: matches (state %d) bindings: %p\n",
				q_now->lnr, s, (void *) b);
		}

		free_list(1 - current);		// remove next states
		copy_list(2, 1 - current);	// replace with initial states

//		cur = q_now;	// move ahead, avoid overlap

		return 1;
	}
	return 0;
}

static Store *
saveas(State *s, char *name, char *text)	// name binding
{	Store *b;

	for (b = s->bindings; b; b = b->nxt)
	{	if (strcmp(b->name, name) == 0
		&&  strcmp(b->text, text) == 0)
		{	return (Store *) 0;	// already there
	}	}

	b = get_store(0);
	b->name = name;
	b->text = text;
	b->bdef = q_now;
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
	} else		// do *not* match at least one bound variable
	{	int matched = 0;
		int not_matched = 0;

		for (b = s->bindings; b; b = b->nxt)
		{	if (strcmp(b->name, name) == 0
			&&  strcmp(b->text, text) == 0)
			{	matched++;
			} else
			{	not_matched++;
		}	}

		if (matched > 0
		&&  not_matched > 0)	// remove matched variables
		{	c = NULL;
			for (b = s->bindings; b; b = b->nxt)
			{	if (strcmp(b->name, name) == 0
				&&  strcmp(b->text, text) == 0)
				{	if (c)
					{	c->nxt = b->nxt;
					} else
					{	s->bindings = b->nxt;
					}
					if (--matched <= 0)
					{	break;
					}
				} else
				{	c = b;
		}	}	}

		return (not_matched > 0);
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
#if 1
	List *c;
	int d;
	for (c = curstates[current]; c; c = c->nxt)
	{	d = c->s->seq;
		assert(d >= 0 && d <= Seq);
		assert(snapshot[d] != NULL);
//		move2free(snapshot[d]->p);
		snapshot[d]->p = (PrimStack *) 0;
		snapshot[d]->n = 0;
	}
#endif
}

PrimStack *
get_prim(void)
{	PrimStack *n;

	if (prim_free)
	{	n = prim_free;
		n->type = n->i_state = 0;
		prim_free = prim_free->nxt;
	} else
	{	n = (PrimStack *) emalloc(sizeof(PrimStack), 109);
	}
	n->t   = q_now;
	return n;
}

static void
mark_prim(int d, int m)
{
	prim_stack->type |= m;
#if 1
	assert(d >= 0 && d <= Seq);
	assert(snapshot[d]->p != NULL);
	snapshot[d]->p->type |= m;
#endif
}

static int
check_match(int d, int m, int old)
{
	assert(d >= 0 && d <= Seq);

	if (snap_pop[d]->p == NULL)
	{	if (verbose)
		{	fprintf(stderr, "assert fails: snap_pop[d]->p == NULL\n");
		}
		return old;
	}

	assert(snap_pop[d]->p != NULL);

	if (((snap_pop[d]->p->type & m) == m) != (old == 0))
	{	if (verbose)
		{	printf("\n%s:%d: MATCH  new: %s <%d>  old: %s <%d>  -- %d %d --)\n",
				q_now->fnm, q_now->lnr,
				(snap_pop[d]->p->type & m)?"match ":"mismatch",
				snap_pop[d]->p->type,
				old?"mismatch":"match",
				prim_free->i_state,
				d, (1<<d));
		}
		return !(snap_pop[d]->p->type & m);
	} // else

	return old;
}

static void
push_prim(void)
{	PrimStack *n;
	List *c;
	int d;
	n      = get_prim();
	n->nxt = prim_stack;
	prim_stack = n;
#if 1
	// a shadow struct for now
	for (c = curstates[current]; c; c = c->nxt)
	{	if (!c->s)
		{	fprintf(stderr, "push_prim: error\n");
		} else
		{	d = c->s->seq;
			assert(d >= 0 && d <= Seq);
			n = get_prim();
			n->nxt = snapshot[d]->p;
			snapshot[d]->p = n;
			snapshot[d]->n += 1;
			if (verbose)
			{	printf("Push_prim %s >>> %d\n", n->t->txt, d);
	}	}	}
#endif
}

static void
pop_prim(void)
{	PrimStack *n = prim_stack;
	List *c;

	if (!n)
	{	return;
	}
	prim_stack = n->nxt;
	n->nxt = prim_free;
	prim_free = n;
#if 1
	int d;
	// a shadow struct for now
	for (c = curstates[current]; c; c = c->nxt)
	{	if (!c->s)
		{	fprintf(stderr, "pop_prim: error\n");
		} else
		{	d = c->s->seq;
			assert(d >= 0 && d <= Seq);
			n = snapshot[d]->p;
			if (n)
			{	snapshot[d]->p = n->nxt;
 #if 1
				snap_pop[d]->p = n; // remember last pop
 #else
				n->nxt = prim_free;
				prim_free = n;
 #endif
				snapshot[d]->n -= 1;
				if (verbose)
				{	printf("Pop_prim %s <<< %d\n", n->t->txt, d);
				}
			} else if (0)	// happens after accept and restart
			{	printf("Bad Pop, state %d\n", d);
	}	}	}
#endif
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
	b->bdef = (Prim *) 0;
	b->ref  = (Prim *) 0;
	b->nxt  = free_bound;
	free_bound = b;
}

static Match *
free_m(Match *m)
{	Bound *b, *nb;
	Match *nm;

	nm = m->nxt;

	m->msg  = (char *) 0;
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
modify_match(void)
{	Match *m, *prv;
	Prim *q;

	// keep only matches that are contained
	// in the current set of basic markings
again:
	prv = (Match *) 0;
	for (m = matches; m; prv = m, m = m->nxt)
	{	if (!m->from
		||  !m->upto)
		{	continue;
		}
		for (q = m->from; q && q->seq <= m->upto->seq; q = q->nxt)
		{	if (q->mark)
			{	continue;
			}
			// else, drop the match
			if (!prv)
			{	matches = free_m(m);
			} else
			{	prv->nxt = free_m(m);
			}
			goto again;
	}	}
}

// externally visible

void
undo_matches(void)
{	Match *tmp;

	tmp = matches;
	matches = old_matches;
	old_matches = tmp;
}

void
patterns_help(void)
{
	printf("ps caption n message	# add (or replace) a caption to an existing pattern set 'n'\n");
	printf("ps convert n		# convert a pattern set 'n' into basic token markings\n");
	printf("ps create n		# convert basic token markings into a pattern set named 'n'\n");
	printf("ps delete n		# delete pattern set 'n'\n");
	printf("ps list [n]		# list a specific or all pattern sets, and their membership\n");
	printf("ps rename n newname	# renames an existing pattern set n to newname\n");
	printf("ps n1 = n2 [& + - * m < >] n3	# define n1 as the intersection (&), union (+), difference (-) of n2 and n3\n");
	printf("ps help			# print this message\n");
}

void
patterns_caption(char *s)
{	Named *x;
	char *t = s;

	// setname message ...
	// printf("caption: '%s'\n", s);
	while (*t != '\0' && !isspace((int) *t))
	{	t++;
	}
	if (!isspace((int) *t))
	{	fprintf(stderr, "error: bad format\n");
		return;
	}
	*t = '\0';
	x = findset(s, 0, 12);
	if (!x)
	{	if ((!json_format && !no_display && !no_match) && verbose)
		{	fprintf(stderr, "warning: no such set '%s'\n", s);
		}
		return;
	}
	t++;	// now points at message
	if (!x->msg
	|| strlen(x->msg) < strlen(t))
	{	x->msg = (char *) emalloc(strlen(t)+1, 100);
	}
	strcpy(x->msg, t);
}

void
patterns_create(void)		// convert marks to pattern matchess in SetName
{	Match *om = matches;
	Match *p;
	Prim *w;
	int p_cnt = 0;
	int m_cnt = 0;

	matches = (Match *) 0;	// build list
	for (w = prim; w; w = w?w->nxt:w)
	{	if (w->mark == 0)
		{	continue;
		}
		m_cnt++;
		// start of a range
		p = (Match *) emalloc(sizeof(Match), 100);
		p->from = w;
		while (w && w->mark)
		{	w = w->nxt;
			m_cnt++;
		}
		p->upto = (w && w->prv)?w->prv:plst;
		p->nxt = matches;
		matches = p;
		p_cnt++;
	}
	if (p_cnt > 0)
	{	new_named_set(SetName);		// convert marks to set
	}
	matches = om;	// restore

	if (gui)
	{	printf("%d matches stored in %s (patterns_create)\n", p_cnt, SetName);
	} else
	{	if (verbose
		||  (!json_format
		 &&  !no_match
		 &&  !no_display))
		{	printf("%d marks -> %d patterns stored in %s\n", m_cnt, p_cnt, SetName);
	}	}
}

void
patterns_json(char *s)
{	Named *q;
	Match *m;
	char *t;
	int first_entry = 1;

	q = findset(s, 0, 11);
	if (!q)
	{	if (!json_format && !no_display && !no_match)
		{	fprintf(stderr, "pattern set '%s' not found\n", s);
		}
		return;
	}
	for (m = q->m; m; m = m->nxt)
	{	if (has_suppress_tags
		&&  matches_suppress(m->from, m->msg))
		{	if (verbose)
			{	fprintf(stderr, "%s:%d: warning suppressed by tag match\n",
					m->from->fnm, m->from->lnr);
			}
			continue;
		}
		if (m->msg)
		{	strncpy(json_msg, m->msg, sizeof(json_msg)-1);
		} else if (strcmp(m->from->fnm, m->upto->fnm) == 0)
		{	sprintf(json_msg, "lines %d..%d",
				m->from->lnr, m->upto->lnr);
		} else
		{	sprintf(json_msg, "%s:%d..%s:%d",
				m->from->fnm, m->from->lnr,
				m->upto->fnm, m->upto->lnr);
		}
		t = q->msg?q->msg:q->nm;
		json_match(s, t, json_msg, m->from, m->upto, first_entry);
		first_entry = 0;
	}
}

static void
patterns_show(Named *q, int cnt)
{	FILE *fd = track_fd?track_fd:stdout;
	Match *r;
	int sz = 0;

	if (!q)
	{	return;
	}
	for (r = q->m; r; r = r->nxt)
	{	sz++;
	}
	if (!gui)
	{	if (cnt >= 0)
		{	fprintf(fd, "%3d: ", cnt);
		}
		fprintf(fd, "%s, %d patterns", q->nm, sz);
	}
	if (q->msg && strlen(q->msg) > 0)
	{	if (gui)
		{	fprintf(fd, "%d title %s = %s",
				cnt, q->nm, q->msg);
		} else
		{	fprintf(fd, " :: %s", q->msg);
	}	}
	fprintf(fd, "\n");
}

static int
patterns_reversed(Named *q, int cnt)
{
	if (!q)
	{	return cnt;
	}
	cnt = patterns_reversed(q->nxt, cnt);
	patterns_show(q, cnt);
	return cnt+1;
}

void
patterns_list(char *s)
{
	if (strlen(s) > 0)
	{	patterns_show(findset(s, 1, 13), -1);
	} else
	{	patterns_reversed(namedset, 1);
	}
}

void
patterns_rename(char *s)
{	Named *q;
	char *p = s;

	if (!s)
	{	fprintf(stderr, "usage: ps rename A B\n");
		return;
	}
	while (*p != ' ' && *p != '\0')
	{	p++;
	}
	if (*p == ' ')
	{	*p = '\0';
		p++;
	} else
	{	fprintf(stderr, "usage: ps rename A B\n");
		return;
	}
	q = findset(s, 1, 14);
	if (q)
	{	if (strlen(p) > strlen(s))
		{	q->nm = (char *) emalloc(strlen(p)+1, 145);
		}
		strcpy(q->nm, p);
	}
}

static void
reverse_delete(Named *q)
{	Match *r;

	if (q)
	{	reverse_delete(q->nxt);
		r = q->m;
		while (r)
		{	r = free_m(r);	// recycle
	}	}
}

void
patterns_delete(void)
{	Named *q, *pq = NULL;
	Match *r;

	if (strcmp(SetName, "*") == 0
	||  strcmp(SetName, "all") == 0)
	{	reverse_delete(namedset);
		namedset = 0;
		return;
	}

	for (q = namedset; q; pq = q, q = q->nxt)	// delete
	{	if (strcmp(q->nm, SetName) == 0
		&&  strlen(SetName) == strlen(q->nm))
		{	if (pq)
			{	pq->nxt = q->nxt;
			} else
			{	namedset = q->nxt;
			}
			q->nxt = NULL;		// recycle Named items q as well?
			r = q->m;
			while (r)
			{	r = free_m(r);	// recycle Match elements
			}			// and bindings
			if (!json_format && !no_match && !no_display)
			{	printf("pattern set '%s' deleted\n", SetName);
			}
			return;
	}	}
	if (!json_format && !no_match && !no_display)
	{	fprintf(stderr, "pattern set '%s' not found\n", SetName);
	}
}

static int
preserve_matches(int n)
{	int p;

	if (!matches)
	{	return 0;
	}

	p = do_markups((const char *) SetName);

	if (strlen(SetName) > 0 && p > 0)
	{	new_named_set(SetName);		// preserve_matches end of pe
		matches = (Match *) 0;
		if (gui)
		{	// printf("%d matches stored in %s (preserve_matches)\n", p, SetName);
			// do_markups already reported the matches
		} else if (!json_format && !no_match && !no_display)
		{	printf("%d patterns stored in set %s\n", p, SetName);
		}
		return p;	// dont convert patterns to marks
	}

//	if (no_match && no_display && p > 0)
//	{	return p;
//	}

	return matches2marks(1);
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
pattern_matched(Named *curset, int which, int N, int M)
{	Match *m;
	int r, n = 0, p = 0, a, b;
	int first_entry = 1;
	FILE *fd = track_fd?track_fd:stdout;
	char *t;
	int notsamefile = 0;

	if (verbose)
	{	fprintf(stderr, "genmatch>%s\n", curset?curset->nm:"??");
	}
	for (m = matches; m; m = m->nxt)
	{	if (!m->from
		||  !m->upto)
		{	continue;
		}
		if (pattern_filter != NULL
		&&  strcmp(pattern_filter, m->from->fnm) != 0)
		{	continue;
		}

		if (has_suppress_tags
		&& matches_suppress(m->from, m->msg))
		{	if (verbose)
			{	fprintf(stderr, "%s:%d: warning suppressed by tag match\n",
					m->from->fnm, m->from->lnr);
			}
			continue;
		}

		if (m->from->seq > m->upto->seq)
		{	Prim *tmp = m->from;
			m->from = m->upto;
			m->upto = tmp;	// from should precede upto
		}
		if (strcmp(m->from->fnm, m->upto->fnm) == 0)
		{	r = m->upto->lnr - m->from->lnr;	// in same file...
		} else
		{	Prim *tmp;
			notsamefile = 1;
			int ln = m->from->lnr;
			r = 1;					// across files
			for (tmp = m->from; tmp && tmp->seq <= m->upto->seq; tmp = tmp->nxt)
			{	if (tmp->lnr != ln)
				{	ln = tmp->lnr;
					r++;			// count lines
			}	}
		}
		if (N >= 0)
		{	if (M < 0)	// json which N
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

		if (gui && pattern_filter)
		{	fprintf(fd, "tagged %d %d %d %s\n",
				m->from->lnr, m->upto->lnr,
				r, curset->nm);
			// if r > (upto-from) then it's cross file
			continue;
		}

		p++;
		if (json_format)
		{	if (m->msg)
			{	strncpy(json_msg, m->msg, sizeof(json_msg)-1);
			} else if (strcmp(m->from->fnm, m->upto->fnm) == 0)
			{	sprintf(json_msg, "lines %d..%d",
					m->from->lnr, m->upto->lnr);
			} else
			{	sprintf(json_msg, "%s:%d..%s:%d",
					m->from->fnm, m->from->lnr,
					m->upto->fnm, m->upto->lnr);
			}
			memset(bvars, 0, sizeof(bvars));
			if (m->bind)
			{	Bound *u = m->bind;
				while (u)
				{	char vb[512];
					memset(vb, 0, sizeof(vb));
					if (u->bdef
					&&  u->ref
					&&  strcmp(u->bdef->txt, u->ref->txt) == 0)
					{	snprintf(vb, sizeof(vb), "%s (ln %d..%d)",
							u->bdef->txt, u->bdef->lnr, u->ref->lnr);
						if (strlen(bvars) + strlen(vb) + 3 < sizeof(bvars))
						{	if (bvars[0] != '\0')
							{	strcat(bvars, ", ");
							}
							strcat(bvars, vb);
					}	}
					u = u->nxt;
			}	}
			t = curset->msg?curset->msg:curset->nm;
			if (!opened)
			{	fprintf(fd, "[\n");
				opened = 1;
				first_entry = 1;
				closed = 0;
			} else if (closed)
			{	fprintf(fd, ",[\n"); // add separator
				opened = 1;
				first_entry = 1;
				closed = 0;
			}
			json_match(curset->nm, t, json_msg, m->from, m->upto, first_entry);
			first_entry = 0;
		} else
		{	if (p == 1 && curset->msg)
			{	fprintf(fd, "%s=== %s ===\n",
					gui?"\n":"",
					curset->msg);
			}
			if (p > 1)
			{	fprintf(fd, "\n");
			}
			fprintf(fd, "%d: %s:%d..%s%d", n,
				m->from?m->from->fnm : "?",
				m->from?m->from->lnr:0,
				notsamefile?"*":"", // not same file
				m->upto?m->upto->lnr:0);
			// fprintf(fd, " <<%d-%d>> ", m->from?m->from->seq:-23, m->upto?m->upto->seq:-32);
			if (r > 0)
			{	fprintf(fd, " (%d lines)\n", r);
			}
	
			if (m->bind)
			{	Bound *q;
				int heading = 0;
	
				if (m->bind->bdef
				&&  m->bind->ref)
				{	heading++;
					fprintf(fd, "\tbound variable matches:");
					fprintf(fd, " %s: (ln %d)",
						m->bind->bdef->txt, m->bind->bdef->lnr);
					fprintf(fd, " :%s (ln %d)",
						m->bind->ref->txt, m->bind->ref->lnr);
				}
				for (q = m->bind->nxt; q; q = q->nxt)
				{	if (!q->bdef
					|| !q->ref)
					{	continue;
					}
					if (!heading)
					{	heading++;
						fprintf(fd, "\tbound variable matches:");
					}
					fprintf(fd, ", %s: (ln %d)",
						q->bdef->txt, q->bdef->lnr);
					fprintf(fd, ", :%s (ln %d)",
						q->ref->txt, q->ref->lnr);
			}	}
			fprintf(fd, "\n");

			a = m->from->lnr;
			b = m->upto->lnr;
			if (no_display && r > 5)	// terse mode
			{	if (gui)
				{	show_line(fd, m->from->fnm, 0, a, a+2, 0);
					fprintf(fd, " ...\n");
					show_line(fd, m->upto->fnm, 0, b-2, b, 0);
				} else
				{	show_line(fd, m->from->fnm, 0, a, a+1, 0);
					fprintf(fd, " ...");
					if (notsamefile)
					{	char *bnm = strrchr(m->upto->fnm, '/');
						fprintf(fd, "%s", bnm?bnm:m->upto->fnm);
					}
					show_line(fd, m->upto->fnm, 0, b-1, b, 0);
				}
			} else
			{	if (r == 0 && gui)
				{	int omark;

					cur = m->from;
					omark = cur->mark;
					cur->mark = 1;

					switch (display_mode) {
					case 1: // tok
						list("0", "");
						break;
					case 3: // pre
						reproduce(n, "0");
						break;
					case 2: // src
					default:
						show_line(fd, m->from->fnm, 0, a-5, a+5, a);
						break;
					}
					m->from->mark = omark; // cur can change
				} else
				{	if (strcmp(m->from->fnm, m->upto->fnm) == 0)
					{	show_line(fd, m->from->fnm, 0, a, b, 0);
					} else
					{	show_line(fd, m->from->fnm, 0, a-1, a+5, a);
						fprintf(fd, " ...");
						if (notsamefile)
						{	char *bnm = strrchr(m->upto->fnm, '/');
							fprintf(fd, "%s", bnm?bnm:m->upto->fnm);
						}
						fprintf(fd, "\n");
						show_line(fd, m->upto->fnm, 0, b-5, b+1, a);
		}	}	}	}
fprintf(track_fd, "which %d n %d\n", which, n);
		if (which != 0
		&&  n == which)
		{	break;
		}
	}
	if (opened && p > 0)
	{	fprintf(fd, "]\n");
		closed = 1;
	}
	if (!gui && !no_display)
	{	fprintf(fd, "\n%d pattern%s\n", p, p!=1?"s":"");
	}
	fflush(fd);
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

		if (pattern_filter != NULL
		&&  strcmp(pattern_filter, cur->fnm) != 0)
		{	continue;
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

		fprintf(fd, "%s:%d..", cur->fnm, lst);
		if (strcmp(cur->fnm, cur->bound->fnm) != 0)
		{	fprintf(fd, "%s:", cur->bound->fnm);
		}
		fprintf(fd, "%d", cur->bound->lnr);

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
		
		if (which != 0
		&&  N < 0
		&&  M < 0)
		{	break;
	}	}
	if (which == 0
	||  M < 0)
	{	fprintf(fd, "%d of %d patterns printed\n", hits, total);
	}
}

void
dp_help(void)
{
	printf("dp *			# display all results from all current pattern sets\n");
	printf("dp setname		# display all results of one specific pattern set\n");
	printf("dp [setname *] n		# display only the n-th match\n");
	printf("dp [setname *] N M	# display only matches of between N and M lines long (inclusive)\n");
	printf("dp [setname *] n N M	# display only the n-th match within the given length range\n");
	printf("dp filter file.c	# restrict output of dp commands to matches in file.c\n");
	printf("dp filter off		# remove filter from dp output\n");
	printf("dp help			# print this message\n");
	printf("when used in 'terse' mode dp prints only the first and last 2 lines of each multi-line match\n");
}

static void
dp_usage(const char *s, int nr)
{
	printf("dp: unrecognized option '%s'", s);
	if (nr)
	{	printf(" (nr=%d)", nr);
	}
	printf("\n");
	dp_help();
}

static void
dp_all(int a, int b, int c)
{	Match *pm = matches;
	Named *x;
//	FILE *fd = track_fd?track_fd:stdout;

	for (x = namedset; x; x = x->nxt)	// dp *
	{	matches = x->m;
		if (matches)
		{	// if (!json_format)
			// {	fprintf(fd, "%s:\n", x->msg?x->msg:"Matches");
			// }
			pattern_matched(x, a, b, c); // checks named sets
	}	}

	matches = pm; // restore
}

void
patterns_display(const char *te)	// dp [name help *] [n [N [M]]]
{	int j = 0;
	int nr, a, b, c;
	Named *ns;
	Match *pm = matches;
	char nm[512];

	while (te[j] == ' ' || te[j] == '\t')
	{	j++;
	}

	if (strncmp(te, "help", strlen("help")) == 0
	||  strcmp(te, "?") == 0)
	{	dp_help();
		return;
	}
	if (strncmp(te, "filter", strlen("filter")) == 0)
	{	FILE *tfd;
		int k = j + strlen("filter");
		nr = sscanf(&te[k], "%s", nm);
		if (nr != 1)
		{	fprintf(stderr, "usage: dp filter filename.c\n");
			return;
		}
		if (strcmp(nm, "off") == 0)
		{	pattern_filter = NULL;
			return;
		}
		if ((tfd = fopen(nm, "r")) == NULL)
		{	fprintf(stderr, "error: no such file '%s'\n", nm);
			return;
		}
		fclose(tfd);
		pattern_filter = (char *) emalloc(strlen(nm)+1, 143);
		strcpy(pattern_filter, nm);
		return;
	}

	if (isalpha((uchar) te[j]) || te[j] == '*' || te[j] == '_')	// named set
	{	nr = sscanf(&te[j], "%s %d %d %d", nm, &a, &b, &c);
		switch (nr) {
		case 1: a =  0;
		case 2: b = -1;
		case 3: c = -1;
		case 4:
			if (te[j] == '*')	 // dp * -- all sets
			{	dp_all(a, b, c); // all named sets
				break;
			}
			ns = findset(nm, 1, 2);
			if (ns && ns->m)
			{	matches = ns->m;
				pattern_matched(ns, a, b, c); // checks named sets
				matches = pm; // restore
			}
			break;
		default:
			dp_usage(&te[j], nr);
			break;
		}
	} else if (isdigit((int) te[j]))
	{	nr = sscanf(&te[j], "%d %d %d", &a, &b, &c);
		switch (nr) {
		case  1: b = -1;	// fall thru
		case  2: c = -1;	// fall thru
		case  3: pattern_full(a, b, c);	// checks token markings, not pattern sets
			 break;
		default: dp_usage(&te[j], nr);
			 break;
		}
	} else if (te[j] == '\0')
	{	pattern_full(0, -1, -1);	// checks token markings, not pattern sets
	} else
	{	dp_usage(&te[j], 0);
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
	&&  p != constraints[n]
	&& verbose
	&& !json_format
	&& !no_match
	&& !no_display)
	{	fprintf(stderr, "warning: constraint %d redefined\n", n);
	}
	constraints[n] = p;
}

static int
is_brace(const char t)
{
	switch (t) {
	case '{':
	case '}':
	case '(':
	case ')':
	case '[':
	case ']':
		return 1;
	default:
		break;
	}
	return 0;
}

static int
get_positions(char *t)	// formula includes position parameters
{	int n;

	// stop looking when a constraint @n is seen
	while (*t != '\0')
	{	if ((*t == '<' || *t == '@')
		&& isdigit((int) *(t+1)))
		{	if (*t == '@'
			&&  is_brace(*(t-1)))
			{	t++;
				continue; // not a constraint marker
			}
			n = atoi(t+1);
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
		&& isdigit((int) *(s+1))
		&& !is_brace(*(s-1)))
		{	break;
	}	}
	if (*s == '\0')
	{	for (nr = 0; nr < MAX_CONSTRAINT; nr++)
		{	constraints[nr] = 0;
		}
		return t;	// no constraints
	}

	v = r = (char *) emalloc(strlen(t)+1, 110);
	for (s = t; *s != '\0'; s++)
	{	if (*s != '@'
		||  is_brace(*(s-1))
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
				if (verbose>1)
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
setname(char *s)
{
	strncpy(SetName, s, sizeof(SetName)-1);
}

static void
set_union(Match *a, Match *b)
{	Match *m, *p, *q = a;
	Named *n;
	Match *om = matches;
	int cnt = 0;

	matches = (Match *) 0;
	new_named_set(SetName);	// set union
	n = namedset;

L:	for (m = q; m; m = m->nxt)
	{	p = (Match *) emalloc(sizeof(Match), 100);
		p->from = m->from;
		p->upto = m->upto;
		p->bind = m->bind;
		p->nxt = n->m;
		n->m = p;
		cnt++;
	}
	if (q == a)
	{	q = b;
		goto L;
	}
	matches = om;
	if (gui)
	{	printf("%d matches stored in %s\n", cnt, SetName);
	} else if (!json_format && !no_match && !no_display)
	{	printf("%d patterns stored in set %s\n", cnt, SetName);
	}
}

#if 0
static int
same_bindings(Bound *a, Bound *b)
{
	while (a && b)
	{	if (a->ref != b->ref)
		{	return 0;
		}
		a = a->nxt;
		b = b->nxt;
	}
	if ((a && !b)
	||  (b && !b))
	{	return 0;
	}
	return 1;
}
#endif

static int
has_element(Match *b, Match *m)
{	Match *n;

	for (n = b; n; n = n->nxt)
	{	if (n->from == m->from
		&&  n->upto == m->upto)
	//	&&  same_bindings(n->bind, m->bind))
		{	return 1;
	}	}
	return 0;
}

static void
set_difference(Match *a, Match *b, int how)	// how==1: a-b, how==0: a&b
{	Match *m, *p;
	Named *n;
	Match *om = matches;
	int cnt = 0;

	matches = (Match *) 0;
	new_named_set(SetName);		// set difference
	n = namedset;

	for (m = a; m; m = m->nxt)
	{	if (has_element(b, m) == how)
		{	continue;
		}
		p = (Match *) emalloc(sizeof(Match), 100);
		p->from = m->from;
		p->upto = m->upto;
		p->bind = m->bind;
		p->nxt = n->m;
		n->m = p;
		cnt++;
	}
	matches = om;

	if (gui)
	{	printf("%d matches stored in %s\n", cnt, SetName);
	} else if (!json_format && !no_match && !no_display)
	{	fprintf(stderr, "%d patterns stored in set %s\n", cnt, SetName);
	}
}

static void
copy_set(const char *s)
{	Match *m, *p;
	Named *n, *x;
	Match *om = matches;
	int cnt = 0;

	x = findset(s, 0, 9);
	assert(x);

	matches = (Match *) 0;
	new_named_set(SetName);
	matches = om;
	n = namedset;
	n->msg = x->msg;

	for (m = x->m; m; m = m->nxt)
	{	p = (Match *) emalloc(sizeof(Match), 100);
		p->from = m->from;
		p->upto = m->upto;
		p->bind = m->bind;
		p->nxt = n->m;
		n->m = p;
		cnt++;
	}

	if (gui)
	{	printf("%d patterns stored in %s\n", cnt, SetName);
	} else if (!json_format && !no_match && !no_display)
	{	fprintf(stderr, "%d patterns stored in set %s\n", cnt, SetName);
	}
}

static int
a_contains_b(Match *a, Match *b)	// a contains at least one b
{	Match *n;

	for (n = b; n; n = n->nxt)
	{	if (n->from >= a->from
		&&  n->upto <= a->upto)
		{	return 1;
	}	}
	return 0;
}

static int
a_meets_b(Match *a, Match *b)		// a meets at least one b
{	Match *n;
	Prim *p;

	if (!a->upto)
	{	return 0;
	}
	p = a->upto->nxt;
	for (n = b; n; n = n->nxt)
	{	if (n->from == p)
		{	return 1;
	}	}
	return 0;
}

static int
a_precedes_b(Match *a, Match *b)	// (all of) a precedes at least one of (all of) b
{	Match *n;

	for (n = b; n; n = n->nxt)
	{	if (n->upto
		&&  n->upto < a->from)
		{	return 1;
	}	}
	return 0;
}

static int
a_overlaps_b(Match *a, Match *b)	//  a starts before and ends during at least one b
{	Match *n;

	for (n = b; n; n = n->nxt)
	{	if (a->from < n->from
		&&  a->upto >= n->from
		&&  a->upto <= n->upto)
		{	return 1;
	}	}
	return 0;
}

static void
set_during(Match *a, Match *b)		// a contains (all of) b
{	Match *m, *p, *om = matches;
	Named *n;
	int cnt = 0;

	matches = (Match *) 0;
	new_named_set(SetName);
	n = namedset;

	for (m = a; m; m = m->nxt)
	{	if (!a_contains_b(m, b))
		{	continue;
		}
		p = (Match *) emalloc(sizeof(Match), 100);
		p->from = m->from;
		p->upto = m->upto;
		p->bind = m->bind;
		p->nxt = n->m;
		n->m = p;
		cnt++;
	}
	matches = om;

	if (gui)
	{	printf("%d patterns stored in %s\n", cnt, SetName);
	} else if (!json_format && !no_match && !no_display)
	{	fprintf(stderr, "%d patterns stored in set %s\n", cnt, SetName);
	}
}

static void
set_meets(Match *a, Match *b)		// the end of a is immediately followed by the start of b
{	Match *m, *p, *om = matches;
	Named *n;
	int cnt = 0;

	matches = (Match *) 0;
	new_named_set(SetName);
	n = namedset;

	for (m = a; m; m = m->nxt)
	{	if (!a_meets_b(m, b))
		{	continue;
		}
		p = (Match *) emalloc(sizeof(Match), 100);
		p->from = m->from;
		p->upto = m->upto;
		p->bind = m->bind;
		p->nxt = n->m;
		n->m = p;
		cnt++;
	}
	matches = om;

	if (gui)
	{	printf("%d patterns stored in %s\n", cnt, SetName);
	} else if (!json_format && !no_match && !no_display)
	{	fprintf(stderr, "%d patterns stored in set %s\n", cnt, SetName);
	}

}

static void
set_precedes(Match *a, Match *b)	// (all of) a precedes (all of) b
{	Match *m, *p, *om = matches;
	Named *n;
	int cnt = 0;

	matches = (Match *) 0;
	new_named_set(SetName);
	n = namedset;

	for (m = a; m; m = m->nxt)
	{	if (!a_precedes_b(m, b))
		{	continue;
		}
		p = (Match *) emalloc(sizeof(Match), 100);
		p->from = m->from;
		p->upto = m->upto;
		p->bind = m->bind;
		p->nxt = n->m;
		n->m = p;
		cnt++;
	}
	matches = om;

	if (gui)
	{	printf("%d patterns stored in %s\n", cnt, SetName);
	} else if (!json_format && !no_match && !no_display)
	{	fprintf(stderr, "%d patterns stored in set %s\n", cnt, SetName);
	}


}

static void
set_overlaps(Match *a, Match *b)	//  a starts before and ends during b
{	Match *m, *p, *om = matches;
	Named *n;
	int cnt = 0;

	matches = (Match *) 0;
	new_named_set(SetName);
	n = namedset;

	for (m = a; m; m = m->nxt)
	{	if (!a_overlaps_b(m, b))
		{	continue;
		}
		p = (Match *) emalloc(sizeof(Match), 100);
		p->from = m->from;
		p->upto = m->upto;
		p->bind = m->bind;
		p->nxt = n->m;
		n->m = p;
		cnt++;
	}
	matches = om;

	if (gui)
	{	printf("%d patterns stored in %s\n", cnt, SetName);
	} else if (!json_format && !no_match && !no_display)
	{	fprintf(stderr, "%d patterns stored in set %s\n", cnt, SetName);
	}


}

void
set_operation(char *s)
{	int nr;
	char name1[512];
	char name2[512];
	char op;
	Named *ns1, *ns2;
	Match *m1, *m2;

	if (strlen(SetName) == 0)
	{	printf("no target set name define\n");
		return;
	}
	while (isspace((uchar) *s))
	{	s++;
	}
	if (!isalpha((uchar) *s))
	{	printf("bad target setname '%s'\n", s);
		return;
	}

	nr = sscanf(s, "%s %c %s", name1, &op, name2);
	if (nr != 3
	|| !isalpha((uchar) name1[0])
	|| !isalpha((uchar) name2[0]))
	{	printf("undefined set operation '%s'\n", s);
		return;
	}
	ns1 = findset(name1, 0, 3);
	ns2 = findset(name2, 0, 4);
	m1 = ns1?ns1->m:0;
	m2 = ns2?ns2->m:0;
#if 0
	if (!m1 || !m2)
	{	if ((!json_format && !no_match && !no_display) && verbose)
		{	fprintf(stderr, "no such set %s %s (check: ps list)\n",
				m1?"":name1,
				m2?"":name2);
		}
		if (m1)
		{	copy_set(name1);
		} else if (m2)
		{	copy_set(name2);
		}
		return;
	}
#endif
	switch (op) {
	case '+':
	case '|':	// set union
		if (!m1 || !m2)
		{	if (m1)
			{	copy_set(name1);
			} else if (m2)
			{	copy_set(name2);
			}
			break;
		}
		set_union(m1, m2);
		break;
	case '\\':	// a \ b = a - b = a ^ b
	case '-':
	case '^':	// set difference
		if (!m1)
		{	break;	// return empty set
		}
		if (!m2)
		{	copy_set(name1);
			break;
		}
		set_difference(m1, m2, 1);	// a-b
		break;
	case '&':	// intersection
		if (!m1 || !m2)
		{	break;	// empty set
		}
		set_difference(m1, m2, 0);	// a&b
		break;

	// The follong 7 were added in v 4.1, 6/28/22
	// loosely based on Allen Interval Logic
	case 'd':	// a d b  means a during b (cf allen interval algebra: a during b)
	case '*':	// a * d matches in a that contain at least one b
		if (!m1 || !m2)
		{	break;	// empty set
		}
		set_during(m1, m2);
		break;
	case 'm':	// a m b  means a meets b (the start of b immediately follows the end of a is the start of b)
			// for instance:  pe A: for ( .* ); ps A m A
		if (!m1 || !m2)
		{	break;	// empty set
		}
		set_meets(m1, m2);
		break;
	case '<':	// (all of) a precedes (all of) b
		if (!m1 || !m2)
		{	break;	// empty set
		}
		set_precedes(m1, m2);
		break;
	case '>':	// (all of) a follows  (all of) b
		if (!m1 || !m2)
		{	break;	// empty set
		}
		set_precedes(m2, m1);
		break;
	case '(':	// a ( b  a starts before and ends during b
		if (!m1 || !m2)
		{	break;	// empty set
		}
		set_overlaps(m1, m2);
		break;
	case ')':	// a ) b  a starts during b and ends after
		if (!m1 || !m2)
		{	break;	// empty set
		}
		set_overlaps(m2, m1);
		break;
	// end of added operators 6/28/22

	default:
		printf("unrecognized set operation '%c'\n", op);
		break;
	}
	return;
}

static void
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

static void
clone_set(Named *x, int ix)
{	Prim *q = NULL;
	Match *y;
	Prim  *r;

	if (x->cloned)
	{	return;
	}

	for (y = x->m; y; y = y->nxt)
	{	r = (Prim *) hmalloc(sizeof(Prim), ix, 144);
		r->seq = Seq++;
		r->typ   = x->nm;	// name of the set
		r->jmp   = y->from;	// w synonym 'p_start'
		r->bound = y->upto;	// w synonym 'p_end'
		r->nxt = q;
		if (q)
		{	q->prv = r;
		}
		q = r;
	}
	x->cloned = q;
}

static Prim *
clone_all(int ix)		// create a single list of matches from all sets
{	Named *x;
	Prim *t, *h = NULL;

	for (x = namedset; x; x = x->nxt)
	{	clone_set(x, ix);
		if (x->cloned)	// concat lists
		{	if (h)
			{	t = x->cloned;
				while (t->nxt)
				{	t = t->nxt;
				}
				t->nxt = h;
			}
			h = x->cloned;
			x->cloned = NULL;
	}	}
	if (!h)
	{	h = (Prim *) hmalloc(sizeof(Prim), ix, 143);
		h->seq = 0; // meaning empty list
		// returning null would indicate a syntax error
	}
	
	return h;
}

Prim *
cp_pset(char *s, int ix)
{	Named *x;

	if (!s)
	{	if (verbose)
		{	fprintf(stderr, "no set reference given\n");
		}
		return 0;
	}

	if (strcmp(s, "*") == 0)
	{	return clone_all(ix);
	}

	x = findset(s, 0, 10);
	if (!x || !x->m)
	{	if (verbose)
		{	fprintf(stderr, "warning: cp_pset: no such (or empty) set '%s'\n", s);
		}
		return 0;
	}

	clone_set(x, ix);

	return x->cloned;
}

void
pe_help(void)
{
	printf("pe [&] [setname:] pattern_expr [constraint]*\n");
	printf(" where pattern_expr is a regex on token names, optionally\n");
	printf(" containing name bindings and/or position references\n");
	printf(" position_reference: < [0-9]+ >\n");
	printf(" constraint        : @ [0-9]+ ( token_expr )\n");
	printf(" the token_expr in a constraint is as defined in the query language\n");
	printf(" but it can also contain bound variable references\n");
}

#if 0
static Prim *
ps_fix(Prim *m, int pickup, int whatchanged)
{	static Prim *x = (Prim *) 0;
	static int ncnt = 0;
	Prim *y = m;

	if (!m)
	{	return (Prim *) 0;
	}

	if (!pickup || !x)
	{	x = prim;
		ncnt = 0;
	}

	// the new sequence in x has EOF tokens added
	// so the sequence number looked for is higher
	while (x && x->seq != m->seq + ncnt)
	{	switch (whatchanged) {
		case 2:	if (strcmp(x->txt, "EOF") == 0
			&&  strcmp(x->typ, "cpp") == 0)
			{	ncnt++;
			}
			break;
		case 3:	if (strcmp(x->txt, "EOF") == 0
			&&  strcmp(x->typ, "cpp") == 0)
			{	ncnt++;
				break;
			}
			// else fall thru
		case 1:	if (strcmp(x->txt, "EOL") == 0
			&&  strcmp(x->typ, "cpp") == 0)
			{	ncnt++;
			}
			break;
		default:
			; // cannot happen
		}
		y = x;
		x = x->nxt;
	}
	if (!x)
	{	if (verbose)
		{	fprintf(stderr, "warning: pickup %d at eof\n", m->seq);
			// the new token is the last one in the input
		}
		return y;
	}
	return x;
}

static void
ps_update(int whatchanged)
{	Named *x;
	Match *m;
	Bound *b;

	// update the from, upto, and bind fields in matches

	for (x = namedset; x; x = x->nxt)	// update matches
	for (m = x->m; m; m = m->nxt)
	{	m->from = ps_fix(m->from, 0, whatchanged);
		m->upto = ps_fix(m->upto, 1, whatchanged);
		for (b = m->bind; b; b = b->nxt)
		{	b->bdef = ps_fix(b->bdef, 0, whatchanged);
			b->ref  = ps_fix(b->ref, 1, whatchanged);
	}	}
}
#endif

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
	int	n_cnt;
	Trans	dummy;

	if (inv)
	{	printf("the qualifier 'no' is not supported for pattern searches\n");
		return;
	}

	memset(&dummy, 0, sizeof(Trans));

	if (!te)
	{	return;
	}

	if (strcmp(te, "help") == 0
	||  strcmp(te, "?") == 0
	||  strcmp(te, "\\?") == 0)
	{	pe_help();
		return;
	}

	has_positions = get_positions(te);
	if (has_positions < 0)
	{	return;
	}

	te = get_constraints(te);

	if (!te)
	{	return;
	}

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
		if (!no_cpp)
		{	fprintf(stderr, "error: -cpp removes preprocessing directives\n");
			fprintf(stderr, "       cannot check '%s'\n", te);
			return;
		}
		// with nocpp, there is an EOL at the end of every #define macro
		if (!q1)
		{	fprintf(stderr, "error: expression requires -eol flag\n");
			return;
		}
		if (q1 > q2)
		{	fprintf(stderr, "error: define should precede EOL marker\n");
			fprintf(stderr, "       cannot check '%s'\n", te);
			return;
	}	}

	if (!eof && strstr(te, "EOF"))
	{	fprintf(stderr, "error: use of EOF requires -eof cmdln arg\n");
		return;
	//	fix_eol();		// changes location of tokens
	//	ps_update(anychange);	// updates matches in pattern sets
	//				// not completely reliable
	}

	if (ncalls++ > 0)
	{	reinit_te();
		nerrors = 0;
	}
	// printf("IN: '%s'\n", te);
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

	prune_if_zero();	// remove code in #if 0 blocks

	for (cur = prim; cur; cur = cur->nxt)
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
			if (!across_file_match	// unless we allow this
			&&  q_now->fnm != cur->fnm)	// no match across files
			{	if (verbose)
				{	printf("stop match attempt at file boundary\n");
				}
				break;
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
					{	e_bindings = s->bindings;
						if (!evaluate(q_now, constraints[t->cond]))
						{	if (verbose)
							{	printf("\tconstraint %d fails (S%d -> S%d) <%s>\n",
									t->cond, s->seq, t->dest, q_now->txt);
							}
							continue;
						} else if (verbose)
						{	printf("\t%s:%d: '%s' constraint %d holds (%d) (S%d -> S%d)\n",
								q_now->fnm, q_now->lnr, q_now->txt,
								t->cond,
								(q_now->jmp?q_now->jmp->lnr:0) - q_now->lnr,
								s->seq, t->dest);
					}	}

					if (!t->pat)	// epsilon move, eg .*
					{	if (!t->match) // can this happen?
						{	if (q_now->prv)
							{ q_now = q_now->prv;
							} else
							{ fprintf(stderr, "%s:%d: re error for . (s%d->s%d) <%p>\n",
								cur->fnm, cur->lnr,
								s->seq, t->dest,
								(void *) t->nxt);
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
								mark_prim(c->s->seq, IMPLICIT);
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
#if 1
								mismatch = check_match(s->seq, IMPLICIT, mismatch);
#endif
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
						mk_active(s->bindings, 0, s->seq, t->dest, 1-current);
						anychange = 1;
						if (is_accepting(s->bindings, t->dest))
						{	goto L;
						}
						continue;
					} // !t->pat

					if (t->recall)	// bound variable ref
					{
						if (recall(c->s, t, q_now->txt))
						{	if (!t->match
							&&  (q_now->txt[0] == '{'
							  || q_now->txt[0] == '('
							  || q_now->txt[0] == '['))
							{	if (verbose)
								{	printf("\timplicit ^ match of %s\n",
										q_now->txt);
								}
								assert(prim_stack);
								mark_prim(c->s->seq, IMPLICIT);
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
#if 1
								mismatch = check_match(s->seq, IMPLICIT, mismatch);
#endif
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
							mk_active(s->bindings, 0, s->seq, t->dest, 1-current);
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
									tmx = z?te_regmatch(z, m):0;
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
										tmx = z?te_regmatch(z, m):0;
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
									mark_prim(c->s->seq, EXPLICIT);
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
						{	Store *b;
							b = saveas(s, t->saveas, q_now->txt);
							if (verbose)
							{	printf("==%s:%d: New Binding: %s -> %s (%p)\n",
									q_now->fnm, q_now->lnr,
									t->saveas, q_now->txt, (void *) b);
							}
							mk_active(s->bindings, b, s->seq, t->dest, 1-current);
						} else
						{	mk_active(s->bindings, 0, s->seq, t->dest, 1-current);
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
				}		// for each possible transition
			}			// for each current state

		L:	mustbreak = 0;
			free_list(current);
			if (anychange)
			{	current = 1 - current;	// move forward
			} else
			{	break;			// no match of pattern, Next
			}
		}	// for-loop look-ahead match attempt on token q_now
	}		// for-loop starting match attempt cur

	te_regstop();

	if (!cobra_texpr)	// interactive
	{	Match *x;

		if (and)
		{	modify_match();
		}
		for (rx = 0, x = matches; x; x = x->nxt)
		{	rx++;
		}
		n_cnt = preserve_matches(0);
		set_cnt(n_cnt);
		if (n_cnt)
		{	if (json_format)
			{	if (!no_display && !no_match)
				{	json(te);	// te reporting, interactive
				}
				clear_matches();
		}	}
		rx = 0;
	} else if (stream <= 0)	// not streaming
	{	if (preserve_matches(0) > 0)
		{	if (json_format)
			{	json(te);	// te reporting, not streaming
			} else
			{	pattern_full(0, -1, -1); // shows bound vars too
			}	// check token markings, not pattern sets
		} else
		{	if (json_format)
			{	if (nr_json > 0)
				{	printf("]\n");
					closed = 1;
				}
			} else if (!no_display && !no_match)
			{	printf("0 patterns matched\n");
		}	}
	} else			// streaming input
	{	if (json_format)
		{	if (nr_json > 0)
			{	printf("]\n");
				closed = 1;
			}
		} else if (!no_display && !no_match)
		{	printf("%d pattern%s matched\n",
				p_matched, p_matched==1?"":"s");
	}	}
	stop_timer(0, 0, "te");
}
