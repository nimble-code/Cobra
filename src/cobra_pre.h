/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://codescrub.com/cobra
 */

#ifndef COBRA_PRE
#define COBRA_PRE

#define CURLY_b		   0
#define ROUND_b		   1
#define BRACKET_b	   2

#define NHASH		 256
#define NDEPTH		 256	// max nesting of parens etc
#define NOUT		4096
#define NIN		8192

#ifndef HEADER
 #define HEADER		"/usr/"	// ignore header files with this prefix
#endif

#define Px		pre[cid]

#include "cobra_fe.h"

typedef struct Pre	Pre;
typedef struct Pass	Pass;
typedef struct Triple	Triple;

struct Triple {
	short	 lex_bracket;
	short	 lex_curly;
	short	 lex_roundb;
	Triple	*nxt;
};

struct Pre {	// one struct allocated per core
	char	*lex_fname;
	char	 lex_yytext[MAXYYTEXT];
	Prim	*lex_range[BRACKET_b+1][NDEPTH];
	Prim	*lex_last;
	Prim	*lex_plst;
	Prim	*lex_prim;
	Triple	*triples;	// stack, for handling ifdef nesting with nocpp

	Files	*lex_files[NHASH];
	Typedef	*lex_tps[NAHASH];

	int	 lex_yyin;
	int	 lex_put;
	int	 lex_got;
	int	 lex_count;
	int	 lex_lineno;

	char	 lex_in[NIN];
	char	 lex_out[NOUT];
	char	 lex_cpp[8];		// cpp flags on # directives: 1,2,3,4
	char	 lex_pback[8];		// pushback chars

	uchar	 lex_pcnt;		// 0..7 pushback count
	uchar	 lex_dontcare;		// 0, 1
	uchar	 lex_preprocessing;	// 0, 1, 2

	short	 lex_bracket;
	short	 lex_curly;
	short	 lex_roundb;
};

struct Pass {
	int who;
	int start_nr;
};

enum {
	c_t      =  1,
	cpp_t    =  2,
	ada_t    =  4,
	java_t   =  8,
	python_t = 16
};

extern Pre	*pre;

extern void	line(int);
extern void	par_scan(void);
extern void	process_line(char *, int);
extern void	rem_file(char *, int);
extern void	clr_files(void);

extern char	*get_file(int);
extern char	*strip_directives(char *, int);
extern char	*get_preproc(int);
extern void	 set_preproc(char *, int);
extern Files	*seenbefore(const char *, int);

#endif
