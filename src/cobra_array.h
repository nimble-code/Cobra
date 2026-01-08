/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at https://codescrub.com/cobra
 */

// related to associative arrays

#ifndef COBRA_ARRAY_H
#define COBRA_ARRAY_H

#define NCORE	(Ncore+1)	// debugging

#define H_BITS		9	// 2^9 = 512, 2^12 = 4096
#define H_SIZE		(1<<H_BITS)
#define H_MASK		((H_SIZE)-1)

#define SZ_STATS	512

extern int *Cdepth;

extern char *array_ix(const char *a, const int idx, const int ix);	// T[0] = index(H,3)
extern char *derive_string(Prim **, Lextok *, const int, const char *);

extern int  incr_aname_el(Lextok *p, Rtype *ts, const int tp, Rtype *rv, const int ix);	// get or create
extern int array_sum_el(const char *a, const char *e);
extern int array_sz(const char *a, const int ix);
extern int is_aname(const char *a, const int ix);	// is a an associative array basename

extern uint hasher(const char *s);

extern void array_unify(Lextok *qin, const int cid);
extern void eval_aname(Prim **ref_p, Lextok *q, Rtype *ts, const int ix); // array name, index
extern void eval_prog(Prim **, Lextok *, Rtype *, const int);
extern void ini_arrays(void);
extern void prepopulate(int k, const int ix);
extern void rm_aname(const char *a, int one, const int ix);	// remove array, instead of just one element
extern void rm_aname_el(Prim **ref_p, Lextok *t, const int ix);	// remove array element
extern void rm_var(const char *s, int one, const int ix);
extern void set_aname(const Lextok *p, Rtype *ts, Rtype *rv, const int ix);
extern void what_type(FILE *, Renum);

#endif
