/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

// associative array routines for the cobra scripting language
// the parser for the language is in cobra_prog.y

#include "cobra.h"
#include "cobra_array.h"
#include "cobra_prog.h"

typedef struct Arr_var	Arr_var;
typedef struct Arr_el	Arr_el;
typedef struct Alist	Alist;

struct Arr_el {		// an array element
	uint	 h;	// the full hash of a_index
	char	*a_index; // the array index
	int	 len;	// maximum length if string
	int	 val;	// VAL -- integer value
	char	*s;	// STR -- string value
	Prim	*p;	// PTR -- token reference
	Arr_el	*forw;	// doubly-linked list of all elements in array
	Arr_el  *back;	// doubly-linked list
	Arr_el	*nxt;	// linked list, e.g. in free_els
};

struct Arr_var {	// Basename[A_Index] == A_Value
	char	*name;	// array basename
	int	len;	// max length of name
	uint	size;	// nr of current elements
 #ifdef STATS
	uint	maxsize; // when generating statistics
 #endif
	int 	cdepth;	// fct call depth where defined
	Renum	typ;	// type of values stored, e.g. VAL, STR, PTR
	uint	h_mask;	// mask for indexing ht
	uint	h_limit; // when to resize ht
	Arr_el	*all;	// doubly-linked list of all elements in array
	Arr_el	**ht;	// linked list of H_SIZE elements, by hash
	Arr_var	*nxt;	// linked list of array variables (typically short)
};

struct Alist {	// list of all array names across cores
	const char *nm;	// just the names
	Alist	*nxt;
};

static Arr_var **arr_vars;	// Ncore copies, lists of base-names
static Arr_var **free_arr;
static Arr_el  **free_els;
static Alist    *alist;
static char	**statstring;

#ifdef STATS
 static int stats_scn;	// assuming single core
 static int stats_escn;
 static int stats_atotal;
 static int stats_etotal;
#endif

static const char *
looktyp(Renum x)
{	const char *s;

	switch (x) {
	case UNDEF: s = "undefined"; break;
	case VAL:   s = "value";     break;
	case STR:   s = "string";    break;
	case PTR:   s = "pointer";   break;
	default:    s = "?";         break;
	}
	return s;
}

static Arr_var *
find_array(const char *nm, const int ix, const int mk)	// find or create
{	Arr_var *a, *bm = NULL;
	int len;
	int slv = -1;	// scope stack level

 #ifdef STATS
	stats_atotal++;
 #endif
	for (a = arr_vars[ix]; a; a = a->nxt)	// typically a short list
	{	if (strcmp(a->name, nm) == 0
		&&  a->cdepth > slv
		&&  a->cdepth <= Cdepth[ix])
		{	bm = a;
			slv = a->cdepth;
		}
 #ifdef STATS
		else stats_scn++;
 #endif
	}
	a = bm;	// best match

	if (!a && mk)	// add
	{	len = strlen(nm);
		if (free_arr[ix])
		{	a = free_arr[ix];
			free_arr[ix] = a->nxt;
			if (a->len < len)
			{	a->name = (char *) hmalloc(len+1, ix, 110);
				a->len = len;
			}
			a->typ = 0;
		} else
		{	a = (Arr_var *) hmalloc(sizeof(Arr_var), ix, 111);
			a->name = (char *) hmalloc(len+1, ix, 111);
			a->len  = len;
			a->h_mask = H_MASK;	// initial values
			a->h_limit = H_SIZE<<3;
			a->ht = (Arr_el **) emalloc(H_SIZE * sizeof(Arr_el *), 1);
		}
		strcpy(a->name, nm);
		a->cdepth = Cdepth[ix];
		a->nxt = arr_vars[ix];
		a->size = 0;
		a->all  = 0;
 #ifdef STATS
		a->maxsize = 0;
 #endif
		arr_vars[ix] = a;
	}

	return a;
}

static void
resize_array(Arr_var *a, const int ix)
{	Arr_el **n_ht, *e, *f, *g, *lst;
	uint h1, h2;
	uint  h_size, h_mask;

	h_size = 2*(a->h_mask + 1);
	h_mask = (h_size-1);
 #ifdef STATS
	printf("resizing array %s (size %u) to %d hash-slots\n",
		a->name, a->size, h_size);
 #endif
	n_ht   = (Arr_el **) hmalloc(h_size * sizeof(Arr_el *), ix, 112);

	for (h2 = 0; h2 <= a->h_mask; h2++)
	{	for (e = a->ht[h2]; e; e = f)	// in sort-order on h0
		{	f = e->nxt;
			h1 = (e->h) & h_mask;	// new mask
			lst = NULL;		// insert in order
			for (g = n_ht[h1]; g; lst = g, g = g->nxt)
			{	if (g->h > e->h)
				{	break;
			}	}
			if (lst)
			{	e->nxt = lst->nxt;
				lst->nxt = e;
			} else
			{	e->nxt = n_ht[h1];
				n_ht[h1] = e;
	}	}	}

	a->h_mask = h_mask;
	a->h_limit = (h_mask+1)<<3;
	a->ht = n_ht;
}

static Arr_el *
get_array_element(Arr_var *a, const char *el, const int mk, const int ix)
{	Arr_el  *e, *prev = NULL;
	uint h0 = hasher(el);
	uint h1 = h0&(a->h_mask);
	int len;

 #ifdef STATS
	stats_etotal++;
 #endif
	for (e = a->ht[h1]; e; prev = e, e = e->nxt)
	{	if (e->h > h0)
		{	break;
		}
		if (e->h == h0
		&&  strcmp(e->a_index, el) == 0)
		{	return e;
		}
 #ifdef STATS
		stats_escn++;
 #endif
	}

	if (!mk)
	{	return NULL;
	}

	// add it
	len = strlen(el);
	if (free_els[ix])
	{	e = free_els[ix];
		free_els[ix] = e->nxt;
		if (e->len < len)
		{	e->a_index = (char *) hmalloc(len+1, ix, 113);
			e->len = len;
		}
		e->val = 0;
		e->p = NULL;
	} else
	{	e = (Arr_el *) hmalloc(sizeof(Arr_el), ix, 114);
		e->a_index = (char *) hmalloc(len+1, ix, 114);
		e->len = len;
	}
	e->h   = h0;
	strcpy(e->a_index, el);
	e->s   = "";
	a->size++;

	e->forw = a->all;
	e->back = 0;
	if (a->all)
	{	a->all->back = e;
	}
	a->all = e;

	if (0)
	{	printf("add element %s -> %s, size now %d\n",
			a->name, el, a->size);
	}
 #ifdef STATS
	if (a->size > a->maxsize)
	{	a->maxsize = a->size;
	}
 #endif

	if (prev)	// insert after prev
	{	e->nxt = prev->nxt;
		prev->nxt = e;
	} else		// insert at start
	{	e->nxt = a->ht[h1];
		a->ht[h1] = e;
	}

	// best performance if ht size <= 10*array size/h_size
	if (a->size > a->h_limit)
	{	resize_array(a, ix);
	}

	return e;
}

static void
set_array_element(Arr_var *a, const char *el, Rtype *rv, const int ix)
{	Arr_el *e;

	e = get_array_element(a, el, 1, ix);	// set

	if (rv->rtyp == UNDEF)
	{	rv->rtyp = a->typ;
	}
	switch (rv->rtyp) {
	case UNDEF:
		rv->rtyp = VAL;
		// fall through
	case VAL:
		e->val = rv->val;
		break;
	case STR:
		e->s = rv->s;
		if (isdigit((int) rv->s[0]))
		{	e->val = atoi(e->s);
			rv->rtyp = VAL;
		} else
		{	e->val = 1;
		}
		break;
	case PTR:
		e->p = rv->ptr;
		e->val = 1;	// simplifies array_sum
		break;
	case STP:
	case PRCD:
		break;
	default:
		break;		// cannot happen
	}
	a->typ = rv->rtyp;	// could change the type
}

static Arr_el *
find_array_element(const char *nm, const char *el, const int ix)	// find or create
{	Arr_var *a;

	a = find_array(nm, ix, 1);
	return get_array_element(a, el, 1, ix);	// find
}

static void
rm_array_element(const char *nm, const char *el, const int ix)
{	Arr_var *a;
	Arr_el  *e, *prv = NULL;
	uint h0, h1;

	a = find_array(nm, ix, 0);
	if (!a)
	{	return;
	}
	h0 = hasher(el);			// uses el
	h1 = h0&(a->h_mask);
	for (e = a->ht[h1]; e; prv = e, e = e->nxt)
	{	if (e->h > h0)
		{	break;
		}
		if (e->h == h0
		&&  strcmp(e->a_index, el) == 0	// one of two places where el is needed
		&&  a->cdepth == Cdepth[ix])
		{
			if (0)
			{	int cnt = 1;
				Arr_el *x;
				printf("prep remove from %s e->back %p e->forw %p  a->all %p\n",
					nm, (void *) e->back, (void *) e->forw, (void *) a->all);
				for (x = a->all; x; x = x->forw)
				{	printf("%d	%s	e: %p	f: %p	b: %p\n",
						cnt++, x->a_index, (void *) x,
						(void *) x->forw, (void *) x->back);
			}	}

			if (e->back)
			{	e->back->forw = e->forw;
			} else
			{	a->all = e->forw;
			}
			if (e->forw)
			{	e->forw->back = e->back;
				e->forw = 0;
			}
			e->back = 0;
			if (prv)
			{	prv->nxt = e->nxt;
			} else
			{	a->ht[h1] = e->nxt;
			}
			e->nxt = free_els[ix];
			free_els[ix] = e;
			a->size--;

			if (0)
			{	int cnt = 1;
				Arr_el *x;
				printf("removed element %s %s, new size %d -- all %p\n", nm, el,
					a->size, (void *) a->all);
				for (x = a->all; x; x = x->forw)
				{	printf("%d	%s	e: %p	f: %p	b: %p\n",
						cnt++, x->a_index, (void *) x,
						(void *) x->forw, (void *) x->back);
			}	}

			break;
	}	}
}

static void
recycle_els(Arr_var *a, const int ix)
{	Arr_el *e, *f;
	int h;

	for (e = a->all; e; e = f)
	{	f = e->forw;
		e->forw = e->back = 0;
		e->nxt = free_els[ix];
		free_els[ix] = e;
	}
	for (h = 0; h <= a->h_mask; h++)
	{	a->ht[h] = NULL;
	}

	a->all  = 0;
	a->size = 0;
	if (0)
	{	printf("recycle elements %s\n", a->name);
	}
#ifdef STATS
	a->maxsize = 0;
#endif
}

static Arr_el *
find_array_index(const char *nm, const int n, const int ix)
{	Arr_var *a;
	int cnt;
	static Arr_el *last_e = NULL;	// has to be NCORE copies
	static const char *last_nm = "";
	static int last_n = 1;

	if (n == last_n + 1
	&&  strcmp(nm, last_nm) == 0
	&&  last_e != NULL)
	{	last_n++;
		last_e = last_e->forw;
		return last_e;
	}

	a = find_array(nm, ix, 0);
	if (a && a->size >= n)
	{	last_e = a->all;
		for (cnt = 0; cnt < n; cnt++)
		{	if (!last_e)	// shouldnt happen (cwe_119_2)
			{	printf("cobra: error, this cannot happen, find_array_index");
				printf(" %s cnt %d n %d size %d all %p\n",
					nm, cnt, n, a->size, (void *) a->all);
				last_n = 1;
				last_nm = "";
				return NULL;
			}
			last_e = last_e->forw;
		}
		last_n = n;
		last_nm = nm;
		return last_e;
	}
	return NULL;
}

static void
array_unify_name(const char *nm, const int ix)
{	Arr_var *a, *g;
	Arr_el  *e, *f;
	int n, j;

	// make sure all associative arrays acros cores
	// have the same indices, though not necessarily
	// the same stored values

	if (Ncore == 1 || !nm)
	{	return;
	}
	a = find_array(nm, ix, 1);
	for (n = 0; a && n < Ncore; n++)
	{	if (n == ix)
		{	continue;
		}
		g = find_array(nm, n, 0);
		if (!g)
		{	continue;
		}
		if (a->typ == 0)	// newly created
		{	a->typ = g->typ;
		} else if (a->typ != g->typ)
		{	printf("type conflict on %s, cores %d and %d, types: %s <-> %s\n",
				a->name, ix, n, looktyp(a->typ), looktyp(g->typ));
			// resolve this automically?
		} else
		{	// types match, all good
		}
		for (j = 0; j <= a->h_mask; j++)
		{	for (e = g->ht[j]; e; e = e->nxt)
			{	f = get_array_element(a, e->a_index, 1, ix);	// unify
				f->val += e->val;
				f->s   = e->s;	// may overwrite version in a
				f->len = e->len;
				f->p   = e->p;
	}	}	}
}

static void
add_alist(const char *nm)
{	Alist *v;
	for (v = alist; v; v = v->nxt)
	{	if (strcmp(v->nm, nm) == 0)
		{	return;
	}	}
	v = (Alist *) emalloc(sizeof(Alist), 2);
	v->nm = nm;
	v->nxt = alist;
	alist = v;
}

static void
mk_alist(void)
{	Arr_var *a;
	int j;

	alist = NULL;
	for (j = 0; j < Ncore; j++)
	{	for (a = arr_vars[j]; a; a = a->nxt)
		{	if (a->cdepth == Cdepth[j])
			{	add_alist(a->name);
	}	}	}
}

// externally visible functions

int
array_sum_el(const char *nm, const char *el)	// sum across cores
{	Arr_var *a;
	Arr_el  *e;
	uint h0 = hasher(el);
	uint h1;
	int n, cnt = 0;

	for (n = 0; n < Ncore; n++)
	{	a = find_array(nm, n, 0);
		if (!a)
		{	continue;
		}
		h1 = h0&(a->h_mask);
		for (e = a->ht[h1]; e; e = e->nxt)
		{	if (e->h > h0)
			{	break;
			}
			if (e->h == h0
			&&  strcmp(e->a_index, el) == 0)
			{	cnt += e->val; // independent of type
	}	}	}

	return cnt;
}

char *
array_ix(const char *a, const int idx, const int ix)
{	Arr_el *e;

	e = find_array_index(a, idx, ix);
	return e?e->a_index:"";
}

int
array_sz(const char *nm, const int ix)
{	Arr_var *a;

	a = find_array(nm, ix, 0);
	if (!a)
	{	return 0;
	}
	return (int) a->size;
}

int
is_aname(const char *a, const int ix)	// is 'a' an associative array basename?
{
	return find_array(a, ix, 0) ? 1 : 0;
}

int
incr_aname_el(Prim **ref_p, Lextok *p, Rtype *ts, const int tp, Rtype *rv, const int ix)
{	Arr_var *a;
	Arr_el  *e;
	char *nm = p->s;	// p->s  = array name
	char *el = ts->s;	// ts->s = index type and value

	a = find_array(nm, ix, 1);	// create if it doesn't exist
	if (a->typ && a->typ != VAL)
	{	printf("array %s converted from type \"%s\" to \"value\"\n",
			a->name, looktyp(a->typ));
	}
	a->typ = VAL;

	e = find_array_element(nm, el, ix);
	if (!e)
	{	return 0;
	}

	rv->rtyp = VAL;
	if (tp == INCR)
	{	e->val++;
	} else if (tp == DECR)
	{	e->val--;
	} else
	{	// cannot happen
	}
	rv->val = e->val;

	return 1;
}

void
array_unify(Lextok *qin, const int ix)	// make array qin->rgt->s in core q->val contain all indices
{	Lextok *q = qin->lft;
	Lextok *b = qin->rgt;
	Alist   *m;
	int which = ix;

	if (Ncore <= 1)
	{	return;
	}
	if (q->typ != CPU)
	{	which = q->val;
	}
	if (b)
	{	array_unify_name(b->s, which);
	} else
	{	mk_alist();	// unify all arrays
		for (m = alist; m; m = m->nxt)
		{	array_unify_name(m->nm, which);
	}	}
}

void
rm_aname(const char *a, int one, const int ix)	// remove array, instead of just one element
{	Arr_var *b, *nxt, *lst = NULL;
	int found = 0;

	// all variables, including arrays, are defined at a specific call level
	// which is 0 in global scope, 1 at the first fct-call level, etc.
	// when returning from a fct call, this routine is called with 'one==0'
	// (that's the only case where one==0), which omits the parameters and
	// locals declared in the last level only (not globals)

	for (b = arr_vars[ix]; b; b = nxt)
	{	nxt = b->nxt;
		if ((one && strcmp(b->name, a) == 0
			 && b->cdepth == Cdepth[ix])
		|| (!one && b->cdepth > Cdepth[ix]))
		{	found++;
			if (lst)
			{	lst->nxt = nxt;
			} else
			{	arr_vars[ix] = nxt;
			}
			recycle_els(b, ix);
			b->nxt = free_arr[ix];
			free_arr[ix] = b;
		} else
		{	lst = b;
	}	}

	if (!found)	// must have been a scalar variable
	{	rm_var(a, one, ix);
	}
}

void
rm_aname_el(Prim **ref_p, Lextok *t, const int ix)	// remove array element
{	char *s;	// t->lft name, t->rgt index

	assert(t->lft->typ == NAME);		// basename
	if (!t->rgt)
	{	rm_aname(t->lft->s, 1, ix);
		return;
	}
	s = derive_string(ref_p, t->rgt, ix, statstring[ix]); // avoid leaking mem on s
	rm_array_element(t->lft->s, s, ix);
}

void
set_aname(Prim **ref_p, const Lextok *p, Rtype *ts, Rtype *rv, const int ix)
{       Arr_var *a;

	// p->s = array name
        // ts = index type and value
        // rv = value to be stored
	a = find_array(p->s, ix, 1);
	set_array_element(a, ts->s, rv, ix);
}

void
eval_aname(Prim **ref_p, Lextok *q, Rtype *ts, int ix)	// array name, index
{	Arr_var *a;
	Arr_el  *e;
	Rtype tmp;

	if (q->core)
	{	eval_prog(ref_p, q->core, &tmp, ix);
		assert(tmp.rtyp == VAL);
		ix = tmp.val;
	}

	a = find_array(q->lft->s, ix, 0);
	if (!a)
	{	ts->rtyp = VAL;
		ts->val = 0;	// no such element
		return;
	}

	e = get_array_element(a, ts->s, 0, ix);		// eval
	if (!e)
	{	ts->rtyp = VAL;
		ts->val = 0;	// no such element
		return;
	}

	ts->rtyp = a->typ;        // return type: value stored
	switch (a->typ) {
	case VAL:
		ts->val = e->val;
		break;
	case STR:
		ts->s = e->s;
		break;
	case PTR:
		ts->ptr = e->p;
		break;
	case STP:
	case PRCD:
		break;
	default:
		if (0)
		{	fprintf(stderr, "line %d: warning: unexpected type for array %s: '",
				q->lnr, q->lft->s);
			what_type(stderr, a->typ);
			fprintf(stderr, "'\n");
		}
		ts->rtyp = VAL;
		ts->val  = e->val;
		break;
	}
}

void
new_array(char *s, int ix)
{	Arr_var *a = find_array(s, ix, 1);

	if (a)
	{	printf("global array %s[]\n", s);
		a->cdepth = 0;
	} else
	{	fprintf(stderr, "error: global decl of %s[] failed\n", s);
	}
}

void
ini_arrays(void)
{	static int vmax;

	if (Ncore > vmax)
	{	vmax = Ncore;
		arr_vars = NULL;
	}
	if (!arr_vars)
	{	arr_vars = (Arr_var **) emalloc(NCORE * sizeof(Arr_var *), 3);
		free_arr = (Arr_var **) emalloc(NCORE * sizeof(Arr_var *), 4);
		free_els = (Arr_el  **) emalloc(NCORE * sizeof(Arr_el  *), 5);

		statstring = (char **) emalloc(NCORE * sizeof(char *), 6);
		int i;
		for (i = 0; i < NCORE; i++)
		{	statstring[i] = (char *) emalloc(SZ_STATS * sizeof(char), 7); // shouldnt be fixed
		}
	}
}

void
prepopulate(int k, const int ix)
{	Arr_var *a;

	for (a = free_arr[ix]; a; a = a->nxt)
	{	k--;
	}
	while (k-- >= 0)
	{	a = (Arr_var *) emalloc(sizeof(Arr_var), 8);
		a->h_mask = H_MASK;
		a->h_limit = H_SIZE<<3;
		a->ht = (Arr_el **) emalloc(H_SIZE * sizeof(Arr_el *), 9);
		a->nxt = free_arr[ix];
		free_arr[ix] = a;
	}
}

#ifdef STATS
void
wrap_stats(void)
{	Arr_var *a;

	printf("stats: a %d / %d (%.3f) e %d / %d (%.3f)\n",
		stats_scn, stats_atotal, (float)stats_scn/(float)stats_atotal,
		stats_escn, stats_etotal, (float)stats_escn/(float)stats_etotal);

	for (a = arr_vars[0]; a; a = a->nxt)
	{	printf("\t%s -- size %u -- max %u\n", a->name, a->size, a->maxsize);
	}
}
#else
void
wrap_stats(void)
{
	// do nothing
}
#endif
