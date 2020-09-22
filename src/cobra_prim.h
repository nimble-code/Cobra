#ifndef COBRA_PRIM_H
#define COBRA_PRIM_H

// definitions shared with ../src_app/c_api.h

typedef struct Prim	Prim;
typedef struct TokRange	TokRange;

struct Prim {
	char	*fnm;
	char	*typ;
	char	*txt;

	int	 lnr;
	int	 seq;		// sequence nr
	int	 mark;		// user-definable

	short	curly;		// {} level of nesting
	short	round;		// ()
	short	bracket;	// []
	short	len;

	Prim	*nxt;
	Prim	*prv;

	short	 mset[4];	// 4 sets of marks;  0=undo
	Prim	*mbnd[4];	// 4 sets of bounds; 0=undo

	Prim	*bound;		// bound symbol
	Prim	*jmp;		// from { or } to } or { etc
};

struct TokRange {
	int	 seq;
	int	 param;
	Prim	*from;
	Prim	*upto;
};
#endif
