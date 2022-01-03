#include "cwe.h"

// utility functions for the cwe checkers

extern TokRange	**tokrange;
pthread_t	 *t_id;

static int seen_mark_fcts;

void
clear_marks(Prim *from, Prim *upto)
{	Prim *mycur = from;

	if (upto)
	while (mycur && mycur->seq <= upto->seq)
	{	if (mycur->mark == 57)
		{	mycur->mark = 0;
		}
		mycur = mycur->nxt;
	}
}

void
reset_fcts(void)
{	Prim *mycur = prim;

	seen_mark_fcts = 0;
	while (mycur_nxt())
	{	mycur->mark = 0;
	}
}

void
mark_fcts(void)
{	Prim *mycur, *ptr, *q, *t, *z;
	int preansi;

	if (seen_mark_fcts)
	{	// ran before
		return;
	}

	mycur = prim;
	while (mycur_nxt())
	{	if (mytype("cpp"))
		{	while (!mymatch("EOL"))
			{	mycur_nxt();
			}
			continue;	// skip over preprocessing stuff
		}

		if (!mytype("ident"))
		{	continue;	// cannot be a fct name
		}

		if (cplusplus)
		{	if (mycur->round > 0)
			{	continue;
			}
			if (mycur->prv
			&& (strcmp(mycur->prv->txt, ",") == 0
			||  (mycur->prv->prv
			&&   strcmp(mycur->prv->prv->txt, ">") == 0)))
			{	continue;		// C++ confusion
			}
			if (mymatch("throw"))
			{	continue;
			}
		} else if (mycur->curly > 0)
		{	continue;
		}

		ptr = mycur;		// points at fct name candidate
		mycur_nxt();		// look-ahead
		if (!mycur
		||  !mymatch("(")
		||  !mycur->jmp)
		{	mycur = ptr;	// undo
			continue;	// not a function
		}

		q = mycur->jmp;		// end of possible param list
		if (q && q->nxt
		&& (strcmp(q->nxt->txt, ",") == 0
		||  strcmp(q->nxt->txt, ";") == 0))
		{	continue;	// not a fct def
		}

		// if there are no typenames in the param list
		// but there are identifiers, could be pre-ansi code
		preansi = 0;
		for (t = mycur->nxt; t != q; t = t->nxt)
		{	if (strcmp(t->typ, "type") == 0)
			{	preansi |= 1|2;
				break;
			}
			if (strcmp(t->typ, "ident") == 0)
			{	// at least one identifier
				preansi |= 1;
		}	}
		if (!(preansi & 2) && (preansi & 1))
		{	preansi = 1;
		} else
		{	preansi = 0;
		}

		if (cplusplus)
		{	while (strcmp(q->nxt->typ, "qualifier") == 0)
			{	q = q->nxt;
			}
			if (strcmp(q->nxt->txt, "throw") == 0)
			{	int mx_cnt = 50;
				q = q->nxt;
				if (q->jmp)		// (
				{	q = q->jmp;	// )
				} else
				{	while (q->nxt
					&&     q->curly == ptr->curly
					&&     strcmp(q->nxt->txt, "{") != 0
					&&     strcmp(q->nxt->txt, ";") != 0
					&&     mx_cnt-- > 0)
					{	q = q->nxt;
		}	}	}	}

		if (!q
		||  !q->nxt
		||   q->curly != ptr->curly)
		{	continue;	// sanity check
		}

		// ptr points to fct name
		// mycur points to start of param list
		// q points to end of param list
		// find { ... }

		z = q->nxt;
		while (z && strcmp(z->typ, "cmnt") == 0)
		{	z = z->nxt;	// skip over cmnts
		}

		// skip over pre-ansi param decls
		if (preansi
		&&  (strcmp(z->typ, "type") == 0
		||   strcmp(z->txt, "struct") == 0
		||   strcmp(z->txt, "register") == 0))
		{	while (z && strcmp(z->txt, "{") != 0)
			{	z = z->nxt;
		}	}

		// z points to first token after param list
		if (z && strcmp(z->txt, "{") == 0)
		{	ptr->mark = 1;
			seen_mark_fcts++;
	}	}
}

int
run_threads(void *(*f)(void*))
{	int i, cnt;

	if (Ncore == 1)
	{	i = 0;
		start_timer(0);
		(void) f((void *)&i);
		stop_timer(0, 0, "A");
		cnt = tokrange[0]->param;
	} else
	{	for (i = 0; i < Ncore; i++)
		{	start_timer(i);
			tokrange[i]->param = 0;
			(void) pthread_create(&t_id[i], 0, f,
				(void *) &(tokrange[i]->seq));
		}
		for (i = cnt = 0; i < Ncore; i++)
		{	(void) pthread_join(t_id[i], 0);
			stop_timer(i, 0, "B");
			cnt += tokrange[i]->param;
	}	}

	return cnt;
}

void
set_multi(void)
{	int i, j;
	int span = count/Ncore;
	Prim *a = prim;
	Prim *b = plst;
	Prim *x;

	tokrange = (TokRange **) emalloc(Ncore * sizeof(TokRange *));
	t_id     = (pthread_t *) emalloc(Ncore * sizeof(pthread_t));

	if (Ncore == 1)
	{	tokrange[0] = (TokRange *) emalloc(sizeof(TokRange));
		tokrange[0]->seq  = 0;
		tokrange[0]->from = a;
		tokrange[0]->upto = b;
		return;
	}

	for (i = 0, x = a; i < Ncore; i++)
	{	if (!tokrange[i])
		{	tokrange[i] = (TokRange *) emalloc(sizeof(TokRange));
		}
		tokrange[i]->seq = i;
		tokrange[i]->from = x;
		for (j = 0; x && j < span; j++)
		{	x = x->nxt;
		}
		if (!x)
		{	tokrange[i]->upto = b;
			assert(i == Ncore-1);
		} else
		{	tokrange[i]->upto = x->prv;
		}
		if (0)
		{	fprintf(stderr, "%d: from %d .. %d -- %d - %d\n", i,
				tokrange[i]->from->seq,
				tokrange[i]->upto->seq,
				a->seq, b->seq);
	}	}
	tokrange[Ncore-1]->upto = b;
}

void
store_simple(TrackVar **lst, Prim *v, int cid)
{	TrackVar *p;
	Lnrs *q;

	assert(lst && v);
	p = *lst;
	if (p)
	{	p->cnt++;
		if (!no_display)
		{	q = (Lnrs *) hmalloc(sizeof(Lnrs), cid);
			q->nv  = v;	// location
			q->nxt = p->lst;
			p->lst = q;
		}
	} else
	{	p = (TrackVar *) hmalloc(sizeof(TrackVar), cid);
		p->lst = (Lnrs *) hmalloc(sizeof(Lnrs), cid);
		p->t = p->lst->nv = v;	// location
		p->cnt = 1;
		p->nxt = 0;
		*lst = p;
	}
}

Lnrs *
store_var(TrackVar **lst, Prim *v, int tag, int cid)
{	TrackVar *p, *n, *prv = NULL;
	Lnrs *q = NULL;
	int c;

	// if tag == 0: count occurrences
	// if tag != 0; or in that value

	assert(lst && v);

	do_lock(cid);
	for (p = *lst; p; prv = p, p = p->nxt)
	{	if (!p->t || !p->t->txt)
		{	continue;	// likely multicore bug
		}
		c = strcmp(v->txt, p->t->txt);
		if (c == 0
		&&  strcmp(p->t->fnm, v->fnm) == 0)
		{	if (tag)
			{	p->cnt |= tag;
			} else
			{	p->cnt++;
			}
			if (v->lnr != p->t->lnr)
			{	q = (Lnrs *) hmalloc(sizeof(Lnrs), cid);
				q->nxt = p->lst;
				q->nv  = v;
				p->lst = q;
			}
			do_unlock(cid);

			return q;
		}
		if (c < 0)
		{	break;
	}	}

	n = (TrackVar *) hmalloc(sizeof(TrackVar), cid);
	n->t = v;
	n->lst = (Lnrs *) hmalloc(sizeof(Lnrs), cid);
	n->lst->nv = v;

	if (tag)
	{	n->cnt = tag;
	} else
	{	n->cnt = 1;
	}
	if (prv)
	{	n->nxt = prv->nxt;
		prv->nxt = n;
	} else
	{	n->nxt = *lst;
		*lst = n;
	}
	do_unlock(cid);

	return n->lst;
}

Lnrs *
is_stored(TrackVar *lst, Prim *v, int tag)
{	TrackVar *p;
	int c;
	char *s = v?v->txt:"--- ---";

	for (p = lst; p; p = p->nxt)
	{	c = strcmp(s, p->t->txt);
		if (c == 0
		&& strcmp(v->fnm, p->t->fnm) == 0)
		{
			if (tag)
			{	return (p->cnt == tag)?p->lst:NULL;
			} else
			{	return p->lst;
		}	}
		if (c < 0)
		{	break;
	}	}
	return NULL;
}

void
unstore_var(TrackVar **lst, Prim *v)
{	TrackVar *p, *prv = NULL;
	int c;

	assert(lst && v);

	for (p = *lst; p; prv = p, p = p->nxt)
	{	c = strcmp(v->txt, p->t->txt);
		if (c == 0
		&&  strcmp(p->t->fnm, v->fnm) == 0)
		{	if (prv)
			{	prv->nxt = p->nxt;
			} else
			{	*lst = p->nxt;
			}
			break;
		}
		if (c < 0)
		{	break;
	}	}
}
