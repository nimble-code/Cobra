/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

#include "cobra.h"
#include "cobra_list.h"

static TList **tlst;		// list of lists, one per core
static Prim  **freed_els;	// free tokens
static TList **freed_tlists;	// free TList elements

static int nalloc, nfree_els, nrecycle;

static TList *
find_list(const char *nm, const int ix)
{	TList *t;

	for (t = tlst[ix]; t; t = t->nxt)
	{	if (strcmp(t->nm, nm) == 0)
		{	return t;
	}	}
	return 0;
}

void
ini_lists(void)
{	static int vmax = 0;

	if (Ncore > vmax)
	{	tlst = (TList **) emalloc(Ncore * sizeof(TList *), 108);
		freed_els = (Prim **) emalloc(Ncore * sizeof(Prim *), 108);
		freed_tlists = (TList **) emalloc(Ncore * sizeof(TList *), 108);
		vmax = Ncore;
	}
}

Prim *
top(const char *nm, const int ix)		// return first element
{	TList *t = find_list(nm, ix);

	return t?t->head:0;
}

Prim *
bot(const char *nm, const int ix)		// return last element
{	TList *t = find_list(nm, ix);

	return t?t->tail:0;
}

void
pop_top(const char *nm, const int ix)	// remove top
{	TList *t = find_list(nm, ix);
	Prim *p;

	if (t && t->head)
	{	p = t->head;
		if (t->head == t->tail)
		{	t->head = t->tail = 0;
		} else
		{	t->head = p->nxt;
			if (t->head)
			{	t->head->prv = 0;
		}	}
		t->len--;
		p->nxt = freed_els[ix];
		freed_els[ix] = p;
		nfree_els++;
	}
}

void
pop_bot(const char *nm, const int ix)	// remove bot
{	TList *t = find_list(nm, ix);
	Prim *p;

	if (t && t->tail)
	{	p = t->tail;
		if (t->head == t->tail)
		{	t->head = t->tail = 0;
		} else
		{	t->tail = p->prv;
			if (t->tail)
			{	t->tail->nxt = 0;
		}	}
		t->len--;
		p->nxt = freed_els[ix];
		freed_els[ix] = p;
		nfree_els++;
	}
}

int
llength(const char *nm, const int ix)	// return length of list
{	TList *t = find_list(nm, ix);

	return t?t->len:0;
}

Prim *
obtain_el(const int ix)
{	Prim *n = 0;

	if (freed_els[ix])
	{	n = freed_els[ix];
		freed_els[ix] = n->nxt;
		n->nxt = n->prv = 0;
		nrecycle++;
	} else
	{	n = (Prim *) hmalloc(sizeof(Prim), ix, 121);
		memset(n, 0, sizeof(Prim));
		nalloc++;
	}
	return n;
}

void
release_el(Prim *n, const int ix)
{
	if (freed_els[ix] == n)
	{	fprintf(stderr, "list: double free of token\n");
		// eg when following a list_del_ (pop_top or pop_bot)
	} else
	{	n->nxt = freed_els[ix];
		freed_els[ix] = n;
		nfree_els++;
	}
}

static void
newlist(const char *nm, Prim *n, const int ix)
{	TList *t;

	n->nxt = n->prv = 0;

	if (freed_tlists[ix])
	{	t = freed_tlists[ix];
		freed_tlists[ix] = freed_tlists[ix]->nxt;
	} else
	{	t = (TList *) hmalloc(sizeof(TList), ix, 122);
		memset(t, 0, sizeof(TList));
	}
	t->nxt = tlst[ix];
	tlst[ix] = t;
	t->nm = (char *) hmalloc(strlen(nm)+1, ix, 123);
	strcpy(t->nm, nm);
	t->head = t->tail = n;
	t->len = 1;
}

void
add_top(const char *nm, Prim *n, const int ix)		// add at start
{	TList *t = find_list(nm, ix);

	if (!t)
	{	newlist(nm, n, ix);
	} else
	{	n->nxt = t->head;
		n->prv = 0;
		if (t->head != NULL)
		{	t->head->prv = n;
		}
		t->head = n;
		t->len++;
	}
}

void
add_bot(const char *nm, Prim *n, const int ix)		// add to end
{	TList *t = find_list(nm, ix);

	if (!t)
	{	newlist(nm, n, ix);
	} else
	{	n->prv = t->tail;
		n->nxt = 0;
		t->tail->nxt = n;
		t->tail = n;
		t->len++;
	}
}

void
unlist(const char *nm, const int ix)	// remove list
{	TList *t, *last = 0;
	Prim *p, *pn;

	for (t = tlst[ix]; t; last = t, t = t->nxt)
	{	if (strcmp(t->nm, nm) == 0)
		{	break;
	}	}

	if (!t)
	{	return;
	}

	if (last)
	{	last->nxt = t->nxt;
	} else
	{	tlst[ix] = t->nxt;
	}
	t->nxt = freed_tlists[ix];
	freed_tlists[ix] = t;

	p = t->head;
	while (p)
	{	pn = p->nxt;
		p->prv = 0;
		p->nxt = freed_els[ix];
		freed_els[ix] = p;
		nfree_els++;
		p = pn;
	}
	t->head = t->tail = 0;
	t->len = 0;
}

void
list_stats(void)
{	if (verbose>1)
	{	printf("nalloc: %d nfree: %d, nrecycle %d\n", nalloc, nfree_els, nrecycle);
	}
}
