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

#define NEW_M	  0
#define OLD_M	  1

#define CONCAT	512	// higher than operator values
#define OPERAND	513	// higher than others

typedef struct List	List;
typedef struct Nd_Stack	Nd_Stack;
typedef struct Node	Node;
typedef struct Op_Stack	Op_Stack;
typedef struct Range	Range;
typedef struct State	State;
typedef struct Store	Store;
typedef struct Trans	Trans;
typedef struct Rlst	Rlst;

struct Node {		// NNODE
	int	  type;	// operator or token code 
	char	 *tok;	// token name
	Node	 *nxt;	// linked list
	Nd_Stack *succ;	// successor state
};

struct Op_Stack {
	int	  op;
	Op_Stack *lst;
};

struct Nd_Stack {	// CNODE
	int	  seq;
	int	  visited;
	Node	 *n;	// NNODE
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
	regex_t	*trex;
	Rlst	*nxt;
};

struct List {
	State	*s;
	List	*nxt;
};

struct Store {
	char	*name;
	char	*text;
	Prim	*ref;	// last place where bound ref was found
	Store	*nxt;
};

static List	*curstates[3]; // 2 is initial
static List	*freelist;
static Match	*free_match;
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
static Prim	*c_lft;
static Prim	*b_lft;
static Prim	*p_lft;
static Prim	 matched;
static State	*states;
static Store	*free_stored;
static State	*free_state;
static Rlst	*re_list;

static int	current;
static int	nr_states;
static int	nrtok = 1;
static int	Seq   = 1;
static int	ncalls;
static int	nerrors;

static void	clr_matches(int);
static void	free_list(int);
static void	mk_active(Store *, Store *, int, int);
static void	mk_states(int);
static void	mk_trans(int n, int match, char *pat, int dest);
static void	show_state(FILE *, Nd_Stack *);

static Match	*matches;

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

	memset(&matched, 0, sizeof(Prim));
	cur = prim;
}

#if 0
static void
list_states(char *tag, int n)
{	List *c;
	Trans *t;

	printf("%s: ", tag);
	for (c = curstates[n]; c; c = c->nxt)
	{	printf("%d%s%s, ", c->s->seq, c->s->bindings?"*":"",
			c->s->accept?"*":"");
		if (c->s)
		for (t = c->s->trans; t; t = t->nxt)
		{	printf("[m=%d, d=%d, p=%s] ",
				t->match, t->dest, t->pat);
	}	}
	printf("\n");
}
#endif

static void
te_error(const char *s)
{
	fprintf(stderr, "error: %s\n", s);
	if (cobra_texpr)	// non-interactive
	{	exit(1);
	}
	nerrors++;
}

// regular expression matching on token text

static regex_t *
te_regstart(const char *s)
{	Rlst *r;
	int n;

	r = (Rlst *) emalloc(sizeof(Rlst));
	r->trex = (regex_t *) emalloc(sizeof(regex_t));

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

#define te_regmatch(t, q)	regexec(t, q, 0,0,0)	// t always nonzero

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
	fprintf(fd, "\taccept [shape=doublecircle];\n");
	fprintf(fd, "\tstart -> N%d;\n", nd_stack->seq);
	show_state(fd, nd_stack);
	fprintf(fd, "}\n");
	fclose(fd);

	if (preserve)
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

	n = (Node *) emalloc(sizeof(Node));
	n->type = OPERAND + nrtok++;
	n->tok  = (char *) emalloc(strlen(s)+1);
	strcpy(n->tok, s);

	n->nxt = tokens;
	tokens  = n;

	return n->type;
}

static void
nnode(int op, char *s)
{	Node *t;

	t = (Node *) emalloc(sizeof(Node));
	t->type = op;
	if (s)
	{	t->tok = (char *) emalloc(strlen(s)+1);
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
	{	e = (Op_Stack *) emalloc(sizeof(Op_Stack));
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
mk_el(Node *n)
{	Nd_Stack *t;

	t = (Nd_Stack *) emalloc(sizeof(Nd_Stack));
	t->seq = Seq++;
	t->n = n;
	return t;

}

static void
push(Nd_Stack *t)
{
	t->nxt = nd_stack;
	nd_stack = t;
}

static void
push_node(Node *n)
{	Nd_Stack *t;

	t = mk_el(n);
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

	return t;
}

static void
push_or(Nd_Stack *a, Nd_Stack *b)
{	Nd_Stack *t;

	t = mk_el(0);
	t->lft = a;
	t->rgt = b;
	push(t);
}

static void
loop_back(Nd_Stack *a, Nd_Stack *t)
{
	assert(a != NULL);

	if (a->visited&2)
	{	return;
	}
	a->visited |= 2;

	if (a->n)	// NNODE
	{	if (!a->n->succ)
		{	a->n->succ = t;
		} else
		{	loop_back(a->n->succ, t);
		}
	} else	// CNODE
	{	assert(a->lft != NULL);
		loop_back(a->lft, t);
		if (!a->rgt)
		{	a->rgt = t;
		} else
		{	loop_back(a->rgt, t);
	}	}
}

static void
clear_visit(Nd_Stack *t)
{
	if (t)
	{	t->visited = 0;
		clear_visit(t->lft);
		clear_visit(t->rgt);
	}
}

static void
push_star(Nd_Stack *a)
{	Nd_Stack *t;

	clear_visit(a);

	t = mk_el(NULL);
	loop_back(a, t);
	t->lft = a;
	t->rgt = NULL;	// continuation

	push(t);
}

static Nd_Stack *
epsilon(void)
{	Nd_Stack *t;

	t = mk_el((Node *) emalloc(sizeof(Node)));
	t->n->type = add_token("epsilon");
	t->n->tok  = "";
	return t;
}

static void
prep_transitions(Nd_Stack *t, int src)
{	char *s;
	int dst, match = 1;

	if (t->n)	// NNODE
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
		{	mk_trans(src, 1, 0, dst);
		} else
		{	mk_trans(src, match, s, dst);
		}

		to_expand(t->n->succ);
	} else		// CNODE
	{	prep_transitions(t->lft, src);
		if (t->rgt)
		{	prep_transitions(t->rgt, src);
		} else	// accept
		{	mk_trans(src, 0, 0, Seq+1);
	}	}
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

	e = (Node *) emalloc(sizeof(Node));
	e->type = t->type;
	e->tok  = t->tok;
//	e->succ = t->succ;
	// not e->nxt
	return e;
}

static Nd_Stack *
clone_nd_stack(Nd_Stack *t)
{	Nd_Stack *n;

	if (!t)
	{	return (Nd_Stack *) 0;
	}

	n = (Nd_Stack *) emalloc(sizeof(Nd_Stack));
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
			loop_back(a, b);	// a.a*
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
			loop_back(b, a);
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
	states = (State *) emalloc(sizeof(State)*(nr+1));
	states[nr].accept = 1;
	for (i = 0; i <= nr; i++)
	{	states[i].seq = i;
	}
}

static Range *
new_range(char *s, Range *r)
{	Range *n;

	n = (Range *) emalloc(sizeof(Range));
	n->pat = (char *) emalloc(strlen(s)+1);
	strcpy(n->pat, s);
	n->or = r;

	return n;
}

static Range *
range(char *s)
{	Range *r = (Range *) 0;
	char *a, *b, c;

	assert(*s == '[');
//printf("Here '%s'\n", s);
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
//printf("newrange '%s'\n", a);
			r = new_range(a, r);
			*b-- = c;
		} else // did not advance
		{	break;
	}	}
//printf("Done\n");

	return r;
}

static void
mk_trans(int src, int match, char *pat, int dest)	// called from main.c
{	Trans *t;
	char *b, *g;

	assert(src >= 0 && src < nr_states);
	assert(states != NULL);

	t = (Trans *) emalloc(sizeof(Trans));

	if (pat
	&& (b = strchr(pat, ':')) != NULL)	// possible variable binding
	{	if (b > pat && *(b-1) == '\\')	// x\:zzz => x:zzz
		{	int n = strlen(pat)+1;
			g = emalloc(n*sizeof(char));
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

	if (pat && *pat == '/')	// new 5/27/2019
	{	t->t = te_regstart(pat+1);
	}

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
	{	b = (Store *) emalloc(sizeof(Store));
	}

	if (n)
	{	b->name = n->name;
		b->text = n->text;
		b->ref  = n->ref;
	}
	return b;
}

// Execution

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
			// check if all bindings are already covered
			for (b = bnd; b; b = b->nxt)
			{	for (n = c->s->bindings; n; n = n->nxt)
				{	if (strcmp(b->name, n->name) == 0
					&&  strcmp(b->text, n->text) == 0)
					{	break;
				}	}
				if (!n)	// b not matched
				{	break;
			}	}

			if (b)
			{	continue;	// maybe another state matches
			}

			if (!new)
			{	return;
			}
			for (n = c->s->bindings; n; n = n->nxt)
			{	if (strcmp(new->name, n->name) == 0
				&&  strcmp(new->text, n->text) == 0)
				{	return;
			}	}
	}	}

	// add state

	if (freelist)
	{	c = freelist;
		freelist = freelist->nxt;
		c->nxt = (List *) 0;
	} else
	{	c = (List *) emalloc(sizeof(List));
	}

	if (!bnd && !new)
	{	c->s = &states[src];	// no need to allocate a new struct
	} else
	{	if (free_state)
		{	c->s = free_state;
			free_state = free_state->nxt;
			c->s->nxt = 0;
		} else
		{	c->s = (State *) emalloc(sizeof(State));
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
	{	m = (Match *) emalloc(sizeof(Match));
	}
	m->from = f;
	m->upto = t;

	for (b = bd; b; b = b->nxt)
	{	if (free_bound)
		{	n = free_bound;
			free_bound = n->nxt;
		} else
		{	n = (Bound *) emalloc(sizeof(Bound));
		}
		n->ref = b->ref;
		n->nxt = m->bind;
		m->bind = n;
	}
	m->nxt = matches;
	matches = m;
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

		cur = q_now;	// move ahead, avoid overlap

		return 1;
	}
	return 0;
}

static Store *
saveas(State *s, char *name, char *text)
{	Store *b;

	for (b = s->bindings; b; b = b->nxt)
	{	if (strcmp(b->name, name) == 0)
		{	// override
			b->text = text;
			return (Store *) 0;
	}	}

	b = get_store(0);
	b->name = name;
	b->text = text;
	b->nxt  = (Store *) 0;
	return b;
}

static int
recall(State *s, Trans *t, char *text)
{	Store *b;
	char *name = t->recall;

	if (t->match)	// match any
	{	for (b = s->bindings; b; b = b->nxt)
		{	if (strcmp(b->name, name) == 0)
			{	if (strcmp(b->text, text) == 0)
				{	b->ref = q_now;
//printf("+recall '%s' '%s' '%s'\n", name, text, q_now->txt);
					return 1;
				}
				return 0;
		}	}
		return 0;
	} else	// match none
	{	for (b = s->bindings; b; b = b->nxt)
		{
//printf("-+recall '%s' '%s'\n", b->name, name);
			if (strcmp(b->name, name) == 0)
			{	if (strcmp(b->text, text) != 0)
				{	b->ref = q_now;
//printf("-recall '%s' '%s'\n", name, text);
					return 1;
				}
				return 0;
		}	}
//printf("--recall '%s' '%s'\n", name, text);
		return 1;
	}
}

static void
set_level(char *p)
{
	switch (*p) {
	case '{': c_lft = q_now; break;
	case '(': p_lft = q_now; break;
	case '[': b_lft = q_now; break;
	}
}

static int
check_level(char *p)
{
	switch (*p) {
	case '}':
		if (c_lft)
		{	if (q_now->curly != c_lft->curly)
			{	return 1;
			}
			c_lft = &matched;
			return 2;
		}
		break;
	case ')':
		if (p_lft)
		{	if (q_now->round != p_lft->round)
			{	return 1;
			}
			p_lft = &matched;
			return 2;
		}
		break;
	case ']':
		if (b_lft)
		{	if (q_now->bracket != b_lft->bracket)
			{	return 1;
			}
			b_lft = &matched;
			return 2;
		}
		break;
	}
	return 0;
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
			{	n = (Bound *) emalloc(sizeof(Bound));
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
	{
// printf("%s:%d -- %s:%d\n",
//	m->from->fnm, m->from->lnr,
//	m->upto->fnm, m->upto->lnr);

		p = m->from;
		if (p)
		{	p->mark++;	// start of pattern, one extra increment
			for (p = m->from; ; p = p->nxt)
			{	p->mark++;
				cnt++;
				if (p == m->upto)
				{	break;
	}	}	}	}

	clr_matches(NEW_M);
	printf("%d match%s\n", cnt, (cnt==1)?"":"es");
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
		if (m->bind)
		{	if ((verbose || !no_match) && w++ == 0)
			{	printf("bound variables matched:\n");
			}
			for (b = m->bind; b; b = b->nxt)
			{	if (!b->ref)
				{	continue;
				}
				b->ref->mark += 2;	// the bound variable mark=3
				w++;
				if (verbose || !no_match)
				{	printf("\t%d: %s:%d: %s\n", seq,
						b->ref->fnm,
						b->ref->lnr,
						b->ref->txt);
	}	}	}	}

	printf("%d patterns matched\n", p);

	if (no_match && no_display && w>0)
	{	printf("%d match%s of bound variables\n", w, (w==1)?"":"es");
		return w;
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

void
cobra_te(char *te, int and, int inv)
{	List  *c;
	Trans *t;
	State *s;
	char  *m;
	char  *p;
	int    anychange;
	int    rx;
	int    tmx;

	if (!te)
	{	return;
	}

	if (ncalls++ > 0)
	{	reinit_te();
		nerrors = 0;
	}

	thompson(te);
	if (!nd_stack || nerrors > 0)
	{	te_regstop();
		return;
	}

	assert(!nd_stack->nxt);

	if (p_debug)
	{	show_fsm();
		return;
	}
	mk_fsa();

	copy_list(current, 2);	// remember initial states, 2 was empty so far
	matched.curly = matched.round = matched.bracket = -1;
	for ( ; cur; cur = cur->nxt)
	{	free_list(current);
		free_list(1 - current);
		copy_list(2, current);	// initial state
		c_lft = b_lft = p_lft = (Prim *) 0;

		for (q_now = cur; q_now; q_now = q_now->nxt)
		{	if (q_now->fnm != cur->fnm)	// no match across files
			{
				break;
			}
			anychange = 0;
			if (*(q_now->txt+1) == '\0')
			{	rx = check_level(q_now->txt);
			} else
			{	rx = 0;
			}
			for (c = curstates[current]; c; c = c->nxt)
			{	s = c->s;
				for (t = s->trans; t; t = t->nxt)
				{
					if (!t->pat)
					{	if (!t->match)	// epsilon move
						{	if (q_now->prv)
							{	q_now = q_now->prv;
							} else
							{	printf("%s:%d: unrecoverable re error for .\n",
									cur->fnm, cur->lnr);
								return;
						}	}
						mk_active(s->bindings, 0, t->dest, 1-current);
						anychange = 1;
						if (is_accepting(s->bindings, t->dest))
						{	goto L;
						}
						continue;
					}

					if (t->recall)	// bound variable ref
					{	if (recall(c->s, t, q_now->txt))
						{	mk_active(s->bindings, 0, t->dest, 1-current);
							anychange = 1;
							if (is_accepting(s->bindings, t->dest))
							{	goto L;
						}	}
						continue;
					}

					if (t->or)		// a range
					{	Range *r;
						if (t->match)	// normal or-series
						{
							for (r = t->or; r; r = r->or)
							{	if (*(r->pat) == '@')
								{	m = tp_desc(q_now->typ);
									p = r->pat+1;
								} else
								{	m = q_now->txt;
									p = r->pat;
								}
								if (strcmp(m, p) == 0)
								{	goto is_match;
							}	}
							continue;	  // no match
						} else		// negated
						{
							for (r = t->or; r; r = r->or)
							{	if (*(r->pat) == '@')
								{	m = tp_desc(q_now->typ);
									p = r->pat+1;
								} else
								{	m = q_now->txt;
									p = r->pat;
								}
								if (strcmp(m, p) == 0)
								{	break;
							}	}
							if (r)
							{
								continue;
							}
							goto is_match;
					}	}

					if (*(t->pat) == '@')
					{	m = tp_desc(q_now->typ);
						p = t->pat+1;
					} else
					{	m = q_now->txt;
						p = t->pat;
						// {@0
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

					if (*p == '/') // regexpr, could be: t->t != NULL
					{	tmx = te_regmatch(t->t, m);
					} else if (*p == '\\')
					{	tmx = strcmp(m, p+1);
					} else
					{	tmx = strcmp(m, p);
					}

					if (t->match != (tmx != 0))
					{	if (t->match && *(m+1) == '\0')
						{	set_level(m); // if bracket
							if (rx == 1)
							{	// saw closing bracket
								// opening was set but doesnt match
								continue; // not a true match
							}
							if (rx == 2)	// match, must advance
							{	leave_state(c->s->seq, 1-current);
						}	}

		is_match:			if (t->saveas)
						{	Store *b = saveas(s, t->saveas, q_now->txt);
							mk_active(s->bindings, b, t->dest, 1-current);
						} else
						{	mk_active(s->bindings, 0, t->dest, 1-current);
						}
						anychange = 1;
						if (is_accepting(s->bindings, t->dest)
						|| rx == 2)
						{
							goto L;
						}
			}	}	}	// for loop over transitions

		L:	free_list(current);
			if (anychange)
			{	current = 1 - current;	// move forward
			} else
			{	break;			// no match, Next
			}
		}				// for-loop match attempt
	}					// for-loop starting point of match attempt

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
		(void) convert_matches(0);
		rx = 0;
	} else
	{	(void) convert_matches(0);
		display("", "");
	}
}
