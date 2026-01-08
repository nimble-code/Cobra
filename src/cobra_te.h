/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at https://codescrub.com/cobra
 */

#ifndef COBRA_TE_H
#define COBRA_TE_H

#include <regex.h>

typedef struct Range	Range;
typedef struct PrimStack PrimStack;
typedef struct Snapshot	Snapshot;
typedef struct Trans	Trans;
typedef struct State	State;
typedef struct List	List;
typedef struct ThreadLocal_te ThreadLocal_te;

struct Range {
	char	*pat;
	Range	*or;	// linked list
};

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
	Store	**bindings;	// Ncore copies
	Trans	*trans;
	State	*nxt;	// for freelist only
};

struct List {
	State	*s;
	List	*nxt;
};

struct ThreadLocal_te {
	int		fullmatch;
	int		current;
	List		*curstates[3];	// [2] is initial state
	List		*freelist;
	State		*free_state;
	Store		*free_stored;
	Store		*e_bindings;
	PrimStack	*prim_free;
	PrimStack	*prim_stack;
	Snapshot	**snap_pop;
	Snapshot	**snapshot;
	Prim		*tcur;
	Prim		*q_now;
	Match		*t_matches; // used during parallel pe searches
};

extern ThreadLocal_te	*thrl;

extern Prim	*prim;
extern FILE	*track_fd;
extern Match	*matches;
extern Match	*old_matches;
extern char	*yytext;
extern int	across_file_match;	// -global
extern int	case_insensitive;
extern int	has_suppress_tags;
extern int	lcount;
extern int	tcount;
extern int	tgrep;

extern void	clear_bounds(Prim *);

#endif
