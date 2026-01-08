/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at https://codescrub.com/cobra
 */

#include "cobra.h"
#include "cobra_te.h"

static Match	*free_match;

static Match *
get_match(const int ix)
{	Match *m;
	int mylock = 0;
	extern int have_lock(const int);

	if (!have_lock(ix))
	{	do_lock(ix, 2);
		mylock = 1;
	}

	if (free_match)
	{	m = free_match;
		free_match = m->nxt;
		m->bind = (Bound *) 0;
	} else
	{	m = (Match *) Emalloc(sizeof(Match), 105);
	}
	if (mylock)
	{	do_unlock(ix, 3);
	}
	return m;
}

void
put_match(Match *m)
{	Bound *b, *nb;

	m->from = (Prim *) 0;
	m->upto = (Prim *) 0;
	for (b = m->bind; b; b = nb)
	{	nb = b->nxt;
		if (b == nb)	// internal error
		{	fprintf(stderr, "internal error: put_match\n");
			nb = 0;
		}
		clear_bounds(b->aref);
		put_bound(b);
	}
	m->bind = 0;
	m->nxt = free_match;
	free_match = m;
}

void
add_match(Prim *f, Prim *t, Store *bd, const int pe, const int ix)
{	Match *m;
	Store *b;
	Bound *n;
	FILE *fd = (track_fd) ? track_fd : stdout;

	m = get_match(ix);
	m->from = f;
	m->upto = t;
	for (b = bd; b; b = b->nxt)
	{	n = get_bound();
		n->bdef = b->bdef;
		n->ref  = b->ref;
		n->aref = b->aref;
		n->nxt  = m->bind;
		m->bind = n;
	}

	if (pe)
	{	assert(thrl);
		m->nxt = thrl[ix].t_matches;
		thrl[ix].t_matches = m;
	} else
	{	m->nxt = matches;	// add_match
		matches = m;		// add_match
	}
	p_matched++;

	if (stream == 1)	// when streaming, print matches when found
	{	Prim *c, *r;
		if (json_format)
		{	fprintf(fd, "%s {\n", (nr_json>0)?",":"[");
			if (f && t)
			{	if (strcmp(f->fnm, t->fnm) == 0)
				{	sprintf(json_msg, "lines %d..%d",
						f->lnr, t->lnr);
				} else
				{	sprintf(json_msg, "lines %s:%d..%s:%d",
						f->fnm, f->lnr, t->fnm, t->lnr);
				}
			} else
			{	sprintf(json_msg, "lines %d:%d",
					f?f->lnr:0, t?t->lnr:0);
			}
			memset(bvars, 0, sizeof(bvars));
			for (b = bd; b; b = b->nxt)
			{	if (b->ref
				&&  strlen(b->ref->txt) + strlen(bvars) + 3 < sizeof(bvars))
				{	if (bvars[0] != '\0')
					{	strcat(bvars, ", ");	// add_match
					}
					strcat(bvars, b->ref->txt);
			}	}
			json_match("Stream", glob_te, json_msg, f, t, 1); // add_match, streaming
			fprintf(fd, "}");
			nr_json++;
		} else
		{	fprintf(fd, "stdin:%d: ", f->lnr);
			for (c = r = f; c; c = c->nxt)
			{	fprintf(fd, "%s ", c->txt);
				if (c->lnr != r->lnr)
				{	fprintf(fd, "\nstdin:%d: ", c->lnr);
					r = c;
				}
				if (c == t)
				{	break;
			}	}
			fprintf(fd, "\n");
		}
		if (verbose && bd && bd->ref)
		{	fprintf(fd, "bound variables matched : ");
			while (bd && bd->ref)
			{	fprintf(fd, "%s ", bd->ref->txt);
				bd = bd->nxt;
			}
			fprintf(fd, "\n");
	}	}
}

void
add_pattern(const char *s, const char *msg, Prim *from, Prim *upto, int cid)
{	Named *x;
	Match *om;

	do_lock(cid, 3);		// add_pattern

	om = matches;		// add_pattern -- save old value

	matches = NULL;		// add_pattern
	x = findset(s, 0);
	if (!x)
	{	new_named_set((const char *) s);	// add_pattern
		x = namedset;
	} else	// set exists and is non-empty
	{	if (x->m		// no repeated adds of same token range
		&&  x->m->from == from
		&&  x->m->upto == upto)	// same target token
		{	matches = om;	// add_pattern -- restore old value
			do_unlock(cid, 3);
			return;
	}	}
	add_match(from, upto, NULL, 0, cid);	// new pattern now only one in matches

	matches->nxt = x->m;		// add_pattern -- adding to the set

	if (msg)
	{	matches->msg = (char *) emalloc(strlen(msg)+1, 107);
		strcpy(matches->msg, msg);	// add_pattern
	} else
	{	matches->msg = 0;	// add_pattern
	}
	x->m = matches;			// add_pattern there's only one
	x->cloned = NULL;
	matches = om;			// add_pattern -- restore old value
	p_matched++;

	do_unlock(cid, 3);
}
