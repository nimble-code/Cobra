/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at https://codescrub.com/cobra
 */

#ifndef COBRA_LIST_H
#define COBRA_LIST_H

typedef struct TList TList;	// lists of tokens

struct TList {
	char	*nm;	// name of list
	Prim	*head;
	Prim	*tail;
	int	len;	// nr of elements
	TList	*nxt;	// next list
};

extern Prim	*top(const char *, const int);	// return first element
extern Prim	*bot(const char *, const int);	// return last element
extern Prim	*obtain_el(const int);		// generate new token, possibly recycled

extern void	pop_top(const char *, const int); // remove top
extern void	pop_bot(const char *, const int); // remove bot
extern void	add_top(const char *, Prim *, const int); // add at start
extern void	add_bot(const char *, Prim *, const int); // add to end
extern void	unlist(const char *, const int); // remove list
extern void	ini_lists(void);		// initialize
extern void	release_el(Prim *, const int);	// undo new_el

extern int	llength(const char *, const int); // return length of list

#endif
