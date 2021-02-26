/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

// related to associative arrays

#ifndef COBRA_ARRAY_H
#define COBRA_ARRAY_H

#define NCORE	(Ncore+1)	// debugging

#define H_BITS		9	// 2^9 = 512, 2^12 = 4096
#define H_SIZE		(1<<H_BITS)
#define H_MASK		((H_SIZE)-1)

enum Renum { UNDEF = 0, VAL = 1, STR, PTR, STP, PRCD };

typedef enum	Renum	Renum;
typedef struct	Rtype	Rtype;
typedef struct  Separate Separate;

struct Rtype {
	Renum	rtyp;
	int	val;	// VAL
	char	*s;	// STR
	Prim	*ptr;	// PTR
};

struct Separate {	// thread-local copies, to avoid cache misses
	int Verbose;
	int P_debug;
	int Nest;
	int T_stop;
	char spacer[4096];
};

extern int *Cdepth;

extern void rm_var(const char *s, int one, const int ix);
extern uint hasher(const char *s);
extern ulong hash2(const char *s);
extern char *derive_string(Prim **, Lextok *, const int, const char *);
extern void what_type(FILE *, Renum);
extern void eval_prog(Prim **, Lextok *, Rtype *, const int);

extern int  incr_aname_el(Prim **ref_p, Lextok *p, Rtype *ts, const int tp, Rtype *rv, const int ix);	// get or create
extern void set_aname(Prim **ref_p, const Lextok *p, Rtype *ts, Rtype *rv, const int ix);

extern void eval_aname(Prim **ref_p, Lextok *q, Rtype *ts, const int ix); // array name, index

extern int is_aname(const char *a, const int ix);		// is a an associative array basename?
extern char *array_ix(const char *a, const int idx, const int ix);	// T[0] = index(H,3)

extern void rm_aname(const char *a, int one, const int ix);	// remove array, instead of just one element
extern void rm_aname_el(Prim **ref_p, Lextok *t, const int ix);	// remove array element

extern void ini_arrays(void);
extern void prepopulate(int k, const int ix);

extern int array_sz(const char *a, const int ix);
extern int array_sum_el(const char *a, const char *e);
extern void array_unify(Lextok *qin, const int ix);

#endif
