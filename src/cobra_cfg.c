/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://codescrub.com/cobra
 */

#include "cobra.h"

#define Match(x)	(strcmp(cur->txt, x) == 0)	// @suppress Macros

	// the comment above is an example of a suppress command
	// that prevents a query command on pattern set Macros
	// from displaying this match

#define Expect(x) { \
	cur = cur->nxt; \
	if (strcmp(cur->txt, (x)) != 0) \
	{	printf("%s:%d: expected '%s', saw '%s'\n", \
			cobra_bfnm(), cobra_lnr()-1, (x), \
			cobra_txt()); \
		rval = 0; \
		break;	\
	} \
}

static int	rval;
static int	nr_edges;
static int	nr_nodes;
static FILE	*fd_cg;

static void	graph(Prim *, Prim *);
static int	block(void);

static int
compound_check(const Prim *n)
{	rval = 1;

	if (!n || !n->jmp)
	{	return 0;
	}

	for (cur = (Prim *) n; rval && cur && cur->seq <= n->jmp->seq; cur = cur->nxt)
	{	if (Match("if")
		||  Match("for")
		||  Match("switch"))
		{	Expect("(");
			cur = cur->jmp;
			Expect("{");
		}
		if (Match("while")
		&&  cur->nxt
		&&  cur->nxt->jmp
		&&  cur->nxt->jmp->nxt
		&&  (strcmp(cur->nxt->jmp->nxt->txt, ";") == 0
		||   strcmp(cur->nxt->jmp->nxt->txt, "}") == 0))
		{	cur = cur->prv->prv;
			Expect("}");
			cur = cur->nxt;
			Expect("(");
			cur = cur->jmp;
		}

		if (Match("while")
		&&  cur > n
		&&  cur->prv
		&&  (strcmp(cur->prv->txt, ";") == 0
		||   strcmp(cur->prv->txt, ")") == 0
		||   strcmp(cur->prv->txt, "}") == 0
		||   strcmp(cur->prv->txt, "{") == 0) )
		{	Expect("(");
			cur = cur->jmp;
			Expect("{");
		}
		if (Match("else")
		||  Match("do"))
		{	Expect("{");
	}	}
	cobra_rewind();
	return rval;
}

static int
node(Prim *p)
{
	if (!p)
	{	return 0;
	}
	if (!p->mark)
	{	p->mark = 1;
		nr_nodes++;
	}
	return p->seq;
}

static void
edge(int a, int b, char *s)
{
	if (a != b)
	{	fprintf(fd_cg, "\tN%d -> N%d [label=\"%s\"];\n",
			a, b, strchr(s, '"')?"--":s);
		nr_edges++;
	}
}

static void
graph(Prim *from, Prim *upto)
{	Prim *q;
	int scb = node(from);	// stack of current block
	int ecb = node(upto);	// end of current block -- not yet connected
	int ssb;	// start of sub block
	int esb;	// end of sub block
	int essb;	// end of sub-sub block
	int ln;
	char str[256];
	char cond[256];
	char typ[128];
	char *z;

	for (cur = from; cur && cur->seq < upto->seq; cur = cur->nxt)
	{	if (Match("if"))
		{	Expect("(");

			memset(cond, 0, sizeof(cond));
			ln = 0;
			for (q = cur->nxt; q->seq < cur->jmp->seq; q = q->nxt)
			{	ln += strlen(q->txt);
				if (ln < sizeof(cond))
				{	strcat(cond, q->txt);
			}	}
			z = cond;
			while (*z != '\0')
			{	if (*z == '"')
				{	*z = '\'';
				}
				z++;
			}

			cur = cur->jmp;		// ")"
			cur = cur->nxt;		// "{"
			ssb = node(cur);
			memset(str, 0, sizeof(str));
			snprintf(str, sizeof(str), "%.32s:%d: if", cobra_bfnm(), cur->lnr-1);

			fprintf(fd_cg, "	N%d [label=\"%s\"];\n", scb, str);
			fprintf(fd_cg, "	N%d [label=\"%s\"];\n", ssb, cond);

			edge(scb, ssb, "");	// from start to this if
			esb = block();		// there is always a then block

			fprintf(fd_cg, "	N%d [label=\".\"];\n", esb);

			cur = cur->nxt;
			if (Match("else"))
			{	cur = cur->nxt;		// "{"
				essb = block();
				edge(essb, esb, "{else}"); // tie else to same endpoint
			} else
			{	cur = cur->prv;
				(void) node(cur);
				edge(ssb, esb, "else");
			}
			scb = esb;		// the new start
			continue;
		}
		if (Match("for")
		||  Match("switch")
		||  Match("while"))
		{	memset(typ, 0, sizeof(typ));
			if (strlen(cur->txt) < sizeof(typ))
			{	snprintf(typ, sizeof(typ), "%s", cur->txt);
			} else
			{	strcpy(typ, "--");
			}
			Expect("(");
			cur = cur->jmp;	// ")"
			cur = cur->nxt;			// "{"
			if (Match("{"))
			{	ssb = node(cur);
				memset(str, 0, sizeof(str));
				snprintf(str, sizeof(str), "ln %d: %.32s", cur->lnr-1, typ);

				fprintf(fd_cg, "	N%d [label=\"%s\"];\n", scb, typ);
				fprintf(fd_cg, "	N%d [label=\".\"];\n", ssb);

				edge(scb, ssb, str);
				esb = block();
				scb = esb;
			}	// else: do { ... } while (...);
			continue;
		}
		if (Match("do"))
		{	Expect("{");
			ssb = node(cur);
			memset(str, 0, sizeof(str));
			snprintf(str, sizeof(str), "ln %d: do", cur->lnr-1);

			fprintf(fd_cg, "	N%d [label=\"do\"];\n", scb);
			fprintf(fd_cg, "	N%d [label=\".\"];\n", ssb);

			edge(scb, ssb, str);
			esb = block();
			scb = esb;
			continue;
		}

		memset(typ, 0, sizeof(typ));
		snprintf(typ, sizeof(typ), "(ln %d-", cur->lnr-1);
		do {	cur = cur->nxt;
		} while (cur
		&&  cur < upto
		&& !Match("if")
		&& !Match("for")
		&& !Match("switch")
		&& !Match("while")
		&& !Match("do"));
		cur = cur->prv;

		ssb = node(cur);
		fprintf(fd_cg, "	N%d [label=\".\"];\n", ssb);
		memset(str, 0, sizeof(str));
		snprintf(str, sizeof(str), "%.32s-%d)", typ, cur?cur->lnr-1:0);
		edge(scb, ssb, str);
		scb = ssb;
	}
	fprintf(fd_cg, "	N%d [label=\"end\"];\n", ecb);
	edge(scb, ecb, " ");
	cur = upto;
}

static int
block(void)
{
	if (!Match("{"))
	{	printf("%s:%d: expected {, saw %s\n",
			cobra_bfnm(), cobra_lnr()-1,
			cur?cur->txt:"?");
		noreturn();
	}
	graph(cur, cur->jmp);
	return node(cur);
}

// externally visible

void
cfg(char *s, char *unused)
{	FList *f;

	nr_edges = 0;
	nr_nodes = 0;

	if (!flist)
	{	fct_defs();
	}

	f = find_match_str(s);
	if (!f)
	{	printf("error: function '%s' not found\n", s);
		return;
	}

	if (compound_check(f->q))
	{	cur = (Prim *) f->q;
		if ((fd_cg = fopen(CobraDot, "a")) == NULL)
		{	printf("cannot create '%s'\n", CobraDot);
			return;
		}
		fprintf(fd_cg, "digraph cfg {\n");
		(void) block();
		printf("edges %d, nodes %d, cyclomatic complexity %d\n",
			nr_edges, nr_nodes, nr_edges - nr_nodes + 2);
		fprintf(fd_cg, "}\n");
		fclose(fd_cg);
		if (nr_nodes > 0 && nr_edges > 0 && !preserve)
		{	if (system(ShowDot) < 0)
			{	perror(ShowDot);
		}	}
		sleep(2);
		unlink(CobraDot);
	} else
	{	printf("error: not all compound statements are enclosed in curly braces\n");
	}
}
