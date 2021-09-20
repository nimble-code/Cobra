#include "c_api.h"

// ./cfg -t block | dot -Tx11 &

static int rval, nr_edges, nr_nodes;

static void graph(Prim *, Prim *);
static int block(void);

static int
compound_check(void)
{	rval = 1;

	for (cur = prim ; cur; NEXT)
	{	if (MATCH("if")
		||  MATCH("for")
		||  MATCH("switch"))
		{	EXPECT("(");
			cur = cur->jmp;
			EXPECT("{");
		}

		if (MATCH("else")
		||  MATCH("do"))
		{	EXPECT("{");
	}	}
	cobra_rewind();
	return rval;
}

static int
node(Prim *p)
{
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
	{	if (!no_match)
		{	printf("\t%d -> %d [label=\"%s\"];\n",
				a, b, strchr(s, '"')?"--":s);
		}
		nr_edges++;
	}
}

static void
graph(Prim *from, Prim *upto)
{	int scb = node(from);	// stack of current block
	int ecb = node(upto);	// end of current block -- not yet connected
	int ssb;	// start of sub block
	int esb = -1;	// end of sub block
	int essb;	// end of sub-sub block
	char str[128], typ[32];

	for (cur = from; cur && cur < upto; NEXT)
	{	if (MATCH("if"))
		{	EXPECT("(");
			cur = cur->jmp;	// ")"
			NEXT;			// "{"
			ssb = node(cur);
			snprintf(str, sizeof(str), "if #%d", cur->lnr-1);
			edge(scb, ssb, str);	// from start to this if
			esb = block();		// there is always a then block
			NEXT;
			if (MATCH("else"))
			{	NEXT;		// "{"
				essb = block();
				edge(essb, esb, "{e}"); // tie else to same endpoint
			} else
			{	LAST;
				essb = node(cur);
				edge(ssb, esb, "skip");
			}
			scb = esb;		// the new start
			continue;
		}
		if (MATCH("for")
		||  MATCH("switch")
		||  MATCH("while"))
		{	snprintf(typ, sizeof(typ), "%s", cur->txt);
			EXPECT("(");
			cur = cur->jmp;	// ")"
			NEXT;			// "{"
			if (MATCH("{"))
			{	ssb = node(cur);
				snprintf(str, sizeof(str), "%s #%d", typ, cur->lnr-1);
				edge(scb, ssb, str);
				esb = block();
				scb = esb;
			}	// else: do { ... } while (...);
			continue;
		}
		if (MATCH("do"))
		{	EXPECT("{");
			ssb = node(cur);
			snprintf(str, sizeof(str), "do #%d", cur->lnr-1);
			edge(scb, ssb, str);
			esb = block();
			scb = esb;
			continue;
		}

		snprintf(typ, sizeof(typ), "#%d", cur->lnr-1);
		do {	NEXT;
		} while (cur
		&&  cur < upto
		&& !MATCH("if")
		&& !MATCH("for")
		&& !MATCH("switch")
		&& !MATCH("while")
		&& !MATCH("do"));
		LAST;

		ssb = node(cur);
		snprintf(str, sizeof(str), "%s ... #%d", typ, cur->lnr-1);
		edge(scb, ssb, str);
		scb = ssb;
	}
	edge(scb, ecb, "+");
	cur = upto;
}

static int
block(void)
{
	if (!MATCH("{"))
	{	fprintf(stderr, "%s:%d: expected {, saw %s\n",
			cobra_bfnm(), cobra_lnr()-1,
			cur?cur->txt:"?");
		exit(1);
	}
	graph(cur, cur->jmp);
	return node(cur);
}

void
cobra_main(void)
{
	if (!cobra_target)
	{	fprintf(stderr, "usage: cfg -f fctname file.c\n");
		exit(1);
	}

// should only do the compound_check for the target fct
	if (compound_check())
	{	for ( ; cur; NEXT)
		{	if (TYPE("ident")
			&&  MATCH(cobra_target))
			{	EXPECT("(");
				cur = cur->jmp;
				NEXT;
				if (!MATCH("{"))
				{	continue;
				}
				if (!no_match)
				{	printf("digraph cfg {\n");
				}
				block();
				if (!no_match)
				{	printf("}\n");
				}
				break;
	}	}	}
	fprintf(stderr, "edges %d, nodes %d, cyclomatic complexity %d\n",
		nr_edges, nr_nodes, nr_edges - nr_nodes + 2);
}
