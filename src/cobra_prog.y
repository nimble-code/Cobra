/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

%{
#include "cobra.h"
#include <regex.h>
#include "cobra_array.h"
#include "cobra_list.h"

// parser for inline programs

#define YYSTYPE	Lexptr
#define YYDEBUG 1
#define yyparse xxparse
#define NONE	-9
#define ISVAR	-7

typedef struct	Block	Block;
typedef struct	Cstack	Cstack;	// function call stack
typedef struct	Lexlst	Lexlst;
typedef struct	Var_nm	Var_nm;	// variables: int, str, ptr
typedef struct	REX	REX;

struct Lexlst {
	Lextok	*t;
	Lextok	*dst;
	Lexlst	*nxt;
};

struct Block {
	int	btyp;	// non-zero only for while-blocks
	Lextok *enter;
	Lextok *leave;
	Block  *nxt;
};

struct Cstack {
	const char *nm;
	Lextok	*formals;
	Lextok	*actuals;
	Lextok	*ra;	// return address
	Cstack	*nxt;
};

struct REX {
	regex_t *rexpr;
	char	*s;
	REX	*nxt;
};

struct Var_nm {
	Renum	rtyp;
	ulong	h2;		// name hash
	const char *nm;
		Prim	*pm;	// PTR
		char	*s;	// STR
		int	v;	// VAL
	int	cdepth;		// fct call depth where defined
	Var_nm	*nxt;
};

#ifndef MAX_STACK
  #define MAX_STACK	512	// recursive fct calls
#endif

static REX	*re_lst;
static REX	*re_free;

static Block	**block;
static Block	**b_free;	// freelist for block

static Cstack	**cstack;
static Cstack	**c_free;	// freelist for cstack

static Var_nm	**v_names;
static Var_nm	**v_free;	// freelist for v_names

static Function	 *functions;
static Prim	  none;
static Lextok	 *p_tree;
static Var_nm	 *lab_lst;
Separate	 *sep;		// thread local copies of globals

int	*Cdepth;

static int	 nest;
static int	 p_lnr = 1;
static int	 p_seq = 1;
static int	 n_blocks;	// used during parsing
static int	 a_cnt;		// conservative cnt of nr of arrays
static int	 v_cnt;		// conservative cnt of nr of vars
static int	*t_stop;

char	*derive_string(Prim **, Lextok *, const int, const char *);

static Block	*pop_context(int, int);
static Lextok	*mk_for(Lextok *, Lextok *, Lextok *, Lextok *);
static Lextok	*new_lex(int, Lextok *, Lextok *);
static Var_nm	*mk_var(const char *, const int, const int);
static Var_nm	*check_var(const char *, const int);

// to reduce the number of cache-misses in multicore mode:
	#define DUP_TREES

#ifdef DUP_TREES
 static void	 clear_dup(void);
 static Lextok	*dup_tree(Lextok *);
#endif

static Lextok	*add_return(Lextok *);
static void	handle_global(Lextok *);
static void	add_fct(Lextok *);
static void	fixstr(Lextok *);
static void	mk_fsm(Lextok *, const int);
static void	mk_lab(Lextok *, Lextok *);
static void	push_context(Lextok *, Lextok *, int, int);
static void	tok2txt(Lextok *, FILE *);
       void	what_type(FILE *, Renum);
static void	yyerror(const char *);

void	eval_prog(Prim **, Lextok *, Rtype *, const int);

extern void	new_array(char *, int);	// cobra_array.c
extern void	show_error(FILE *, int);
extern int	xxparse(void);
extern Prim	*cp_pset(char *, int);	// cobra_te.c
static int	yylex(void);

extern int	stream;
extern int	stream_override;
%}

%token	NR STRING NAME IF IF2 ELSE WHILE FOR IN PRINT ARG SKIP GOTO
%token	BREAK CONTINUE STOP NEXT_T BEGIN END SIZE RETRIEVE FUNCTION CALL
%token	ROUND BRACKET CURLY LEN MARK SEQ LNR RANGE FNM FCT
%token	BOUND MBND_D MBND_R NXT PRV JMP UNSET RETURN RE_MATCH FIRST_T LAST_T
%token	TXT TYP NEWTOK SUBSTR SPLIT STRLEN SET_RANGES CPU N_CORE SUM
%token	A_UNIFY LOCK UNLOCK ASSERT TERSE TRUE FALSE VERBOSE
%token	FCTS MARKS SAVE RESTORE RESET
%token  ADD_PATTERN DEL_PATTERN CP_PSET
%token	ADD_TOP ADD_BOT POP_TOP POP_BOT TOP BOT
%token	OBTAIN_EL RELEASE_EL UNLIST LLENGTH GLOBAL
%token	DISAMBIGUATE

%right	'='
%left	OR
%left	AND
%left   B_OR B_AND
%left	B_XOR
%left	EQ NE
%left	GT LT GE LE
%left	LSH RSH
%left	'+' '-'
%left	'*' '/' '%'
%right	'~' '!' '^'
%right	UMIN
%right	'@'
%left	INCR DECR
%left	'.'

%%
all	: c_prog	 { p_tree = $1; return 1; }
	;
c_prog	: '{' prog '}'	 { $$ = $2;  }
	;
prog	: stmnt		 { $$ = $1; }
	| stmnt prog	 { $$ = new_lex(';', $1, $2);
			   n_blocks++;
			   if ($1->typ == CALL
			   ||  $1->typ == FUNCTION)
			   {	$1->c = $2;
			   }
			 }
	;
stmnt	: IF '(' expr ')' c_prog optelse {
			   $1->lft = $3;	// cond
			   $1->rgt = new_lex(0, $5, $6);
			   $1->lnr = p_lnr;
			   $$ = $1; }
	| WHILE '(' expr ')' c_prog {
			   $1->lft = $3;	// cond
			   $1->rgt = $5;	// body
			   n_blocks++;
			   $$ = $1; }
	| FOR '(' NAME IN NAME ')' c_prog {
			   n_blocks++; v_cnt++; a_cnt++;
			   $$ = mk_for($1, $3, $5, $7); }
	| FUNCTION NAME '(' params ')' c_prog {
			   $1->lft = new_lex(0, $2, $4);

			   if ((!$6->rgt || !$6->rgt->typ)
			   &&   $6->typ != RETURN)
			   {	$1->rgt = new_lex(';', $6, new_lex(RETURN, 0, 0));
			   } else
			   {	if ($6->typ != WHILE)
				{	$1->rgt = add_return($6);
				} else // fixed after fix_while in find_fct
				{	$1->rgt = $6;
			   }	}

			   add_fct($1);
			   n_blocks++;
			   $$ = $1;
			}
	| NAME ':' stmnt { v_cnt++; mk_lab($1, $3); $$ = $3; }
	| b_stmnt ';'
	;
b_stmnt	: p_lhs '=' expr { $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| p_lhs '=' NEWTOK '(' ')' { $2->lft = $1; $2->rgt = $3; $$ = $2; };
	| ASSERT '(' expr ')'	{ $1->lft = $3; $$ = $1; }
	| GLOBAL glob_list	{ $$ = $1; handle_global($2); }
	| GOTO   NAME		{ $1->lft = $2; $$ = $1; }
	| PRINT args		{ $1->lft = $2; $$ = $1; }
	| UNSET NAME '[' e_list ']' {
			   $1->lft = $2;
			   $1->rgt = $4;
			   $$ = $1;
			}
	| UNSET NAME		{ $1->lft = $2; $$ = $1; }
	| FCTS '(' ')'		{ $$ = $1; }
	| RESET '(' ')'		{ $$ = $1; }
	| SAVE '(' expr ',' expr ')'	{ $1->lft = $3; $1->rgt = $5; $$ = $1; }
	| RESTORE '(' expr ',' expr ')'	{ $1->lft = $3; $1->rgt = $5; $$ = $1; }
	| LOCK '(' ')'		{ $$ = $1; }
	| UNLOCK '(' ')'	{ $$ = $1; }
	| NAME '(' actuals ')'  { $$ = new_lex(CALL, $1, $3); }
	| p_lhs INCR	{ $2->lft = $1; $$ = $2; }
	| p_lhs DECR	{ $2->lft = $1; $$ = $2; }
	| SET_RANGES '(' expr ',' expr ')' { $1->lft = $3; $1->rgt = $5; $$ = $1; }
	| A_UNIFY '(' NAME ',' nr_or_cpu ')' { $1->lft = $5; $1->rgt = $3; $$ = $1; }
	| A_UNIFY '(' nr_or_cpu ')'	{ $1->lft = $3; $1->rgt = 0; $$ = $1; }

	| ADD_PATTERN '(' NAME ',' t_ref ',' t_ref ')'	{
				$1->lft = $3;
				$1->rgt = new_lex(0, $5, $7);
				$$ = $1;
			}
	| DEL_PATTERN '(' NAME ',' t_ref ',' t_ref ')'	{
				$1->lft = $3;
				$1->rgt = new_lex(0, $5, $7);
				$$ = $1;
			}

	| RELEASE_EL '(' expr ')'	{ $1->lft = $3; $$ = $1;}
	| POP_TOP '(' NAME ')'		{ $1->lft = $3; $$ = $1;}
	| POP_BOT '(' NAME ')'		{ $1->lft = $3; $$ = $1;}
	| UNLIST  '(' NAME ')'		{ $1->lft = $3; $$ = $1;}
	| ADD_TOP '(' NAME ',' expr ')'	{ $1->lft = $3; $1->rgt = $5; $$ = $1; }
	| ADD_BOT '(' NAME ',' expr ')'	{ $1->lft = $3; $1->rgt = $5; $$ = $1; }

	| RETURN expr	{ $1->lft = $2; $$ = $1; }
	| RETURN
	| BREAK
	| CONTINUE
	| NEXT_T
	| STOP
	;
t_ref	: NAME
	| p_ref
	| '.'
	;
nr_or_cpu: NR
	| CPU
	;
params	: /* empty */		{ $$ =  0; }
	| par_list
	;
actuals	: /* empty */		{ $$ = 0; }
	| e_list
	;

glob_nm : NAME
	| NAME '[' ']'		{ $$ = $1; $$->rgt = $2; }
	;
glob_list: glob_nm
	| glob_list ',' glob_nm	{ $$ = $2; $$->lft = $1; $$->rgt = $3; }
	;

par_list: NAME
	| params ',' NAME	{ $$ = $2; $$->lft = $1; $$->rgt = $3; v_cnt++; }
	;

optelse	: /* empty */		{ $$ = 0; }
	| ELSE c_prog		{ $1->rgt = $2; $$ = $1; }
	;

args	: expr
	| args expr		{ $$ = new_lex(ARG, $1, $2); }
	;

b_name	: NAME			{ $$ = $1; v_cnt++; }
	| NAME '[' e_list ']'	{ $2->lft = $1; $2->rgt = $3; $$ = $2; a_cnt++; }
	; 

expr	:'(' expr ')'		{ $$ = $2; }
	| expr GT expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr GE expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr LT expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr LE expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr EQ expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr NE expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr OR expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr AND expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr '+' expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr '-' expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr '*' expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr '/' expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr '%' expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr B_OR expr	{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr B_AND expr	{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr '^' expr		{ $2->lft = $1; $2->rgt = $3; $2->typ = B_XOR; $$ = $2; }
	| expr LSH expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr RSH expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| '!' expr %prec UMIN	{ $1->rgt = $2; $$ = $1; }
	| '-' expr %prec UMIN	{ $1->typ = UMIN; $1->rgt = $2; $$ = $1; }
	| '~' expr %prec UMIN	{ $1->rgt = $2; $$ = $1; }
	| '^' expr %prec UMIN	{ $1->rgt = $2; $$ = $1; }
	| STRING		{ fixstr($1); $$ = $1; }
	| CP_PSET '(' s_ref ')'	{ $$->lft = $3; $$ = $1; }
	| SUBSTR '(' expr ',' expr ',' expr ')' {
				  $1->lft = $3;
				  $1->rgt = new_lex(0, $5, $7);
				  $$ = $1;
				}
	| SPLIT '(' expr ',' NAME ')' { $1->lft = $3; $1->rgt = $5; $$ = $1; }
	| LLENGTH '(' NAME ')'	{ $1->lft = $3; $$ = $1;}
	| TOP '(' NAME ')'	{ $1->lft = $3; $$ = $1; }
	| BOT '(' NAME ')'	{ $1->lft = $3; $$ = $1; }
	| OBTAIN_EL '(' ')'	{ $$ = $1; }
	| MARKS '(' expr ')'	{ $1->lft = $3; $$ = $1; }
	| STRLEN '(' expr ')'	{ $1->lft = $3; $$ = $1; }
	| SIZE '(' NAME ')'	{ $1->rgt = $3; $$ = $1; }
	| SUM '(' p_lhs ')'	{ $1->rgt = $3; $$ = $1; }
	| RETRIEVE '(' NAME ',' expr ')' { $1->rgt = $3; $1->lft = $5; $$ = $1; }
	| RE_MATCH '(' p_lhs ',' STRING ')' { $1->lft = $3; $1->rgt = $5; $$ = $1; }
	| RE_MATCH '(' STRING ')' { $1->lft = 0; $1->rgt = $3; $$ = $1; }
	| DISAMBIGUATE '(' NAME ')' { $1->rgt = $3; $$ = $1; }
	| NAME '(' actuals ')'  { $$ = new_lex(CALL, $1, $3); }
	| '@' NAME		{ $1->rgt = $2; $$ = $1; }
	| '@' TYP		{ $1->rgt = $2; $$ = $1; }
	| '#' NAME		{ $1->rgt = $2; $$ = $1; }
	| b_name '@' expr	{ $1->core = $3; $$ = $1; /* qualified name */ }
	| p_ref  '.' fld	{ $2->lft = $1; $2->rgt = $3; $$ = $2; v_cnt++; }
	| p_ref
	| p_lhs
	| CPU
	| TERSE
	| VERBOSE
	| N_CORE
	| NR
	| TRUE
	| FALSE
	;
s_ref	: NAME
	| '*'			{ $$ = new_lex(NAME, 0, 0); $$->s = "*"; }
	;
p_ref	: BEGIN
	| END
	| FIRST_T
	| LAST_T
	;
p_lhs	: NAME '.' fld  	{ $2->lft = $1; $2->rgt = $3; $$ = $2; v_cnt++; }
	| '.' fld %prec UMIN	{ $1->lft =  0; $1->rgt = $2; $$ = $1; }
	| b_name
	| '.'
	;
e_list	: expr
	| e_list ',' expr	{ $$ = $2; $$->lft = $1; $$->rgt = $3; }
	;
fld	: BOUND
	| MBND_D
	| MBND_R
	| BRACKET
	| CURLY
	| FCT
	| FNM
	| JMP
	| LEN
	| LNR
	| MARK
	| NXT
	| PRV
	| RANGE
	| ROUND
	| SEQ
	| TXT
	| TYP
	;
%%
static FILE *pfd;

static struct Keywords {
	char *s; int t;
} key[] = {
	{ "a_unify",	A_UNIFY },
	{ "add_pattern", ADD_PATTERN },
	{ "assert",	ASSERT },
	{ "Begin",	BEGIN },
	{ "bound",	BOUND },
	{ "bracket",	BRACKET },
	{ "break",	BREAK },
	{ "continue",	CONTINUE },
	{ "core",	CPU },
	{ "cpu",	CPU },
	{ "curly",	CURLY },
	{ "del_pattern", DEL_PATTERN },
	{ "disambiguate", DISAMBIGUATE },
	{ "else",	ELSE },
	{ "End",	END },
	{ "false",	FALSE },
	{ "fct",	FCT },
	{ "fcts",	FCTS },
	{ "first_t",	FIRST_T },
	{ "fnm",	FNM },
	{ "for",	FOR },
	{ "function",	FUNCTION },
	{ "global",	GLOBAL },
	{ "goto",	GOTO },
	{ "if",		IF },
	{ "in",		IN },
	{ "jmp",	JMP },
	{ "last_t",	LAST_T },
	{ "len",	LEN },
	{ "list_add_top", ADD_TOP },	{ "list_push",    ADD_TOP },
	{ "list_add_bot", ADD_BOT },	{ "list_append",  ADD_BOT },
	{ "list_del_top", POP_TOP },	{ "list_pop",     POP_TOP },
	{ "list_del_bot", POP_BOT },	{ "list_chop",    POP_BOT },
	{ "list_get_top", TOP },	{ "list_top",         TOP },
	{ "list_get_bot", BOT },	{ "list_bot",         BOT },
	{ "list_new_tok", OBTAIN_EL },	{ "list_tok",   OBTAIN_EL },
	{ "list_rel_tok", RELEASE_EL },	{ "list_tok_rel", RELEASE_EL },
	{ "list_rel",	UNLIST },
	{ "list_len",	LLENGTH },
	{ "lnr",	LNR },
	{ "lock", 	LOCK },
	{ "marks",	MARKS },
	{ "mark",	MARK },
	{ "match",	RE_MATCH },
	{ "ncore",	N_CORE },
	{ "ncpu",	N_CORE },
	{ "newtok",	NEWTOK },
	{ "Next",	NEXT_T },
	{ "nxt",	NXT },
	{ "p_start",	JMP },
	{ "p_end",	BOUND },
	{ "p_bdef",	MBND_D },
	{ "p_bref",	MBND_R },
	{ "print",	PRINT },
	{ "prv",	PRV },
	{ "pset",	CP_PSET },
	{ "range",	RANGE },
	{ "reset",	RESET },
	{ "restore",	RESTORE },
	{ "retrieve",	RETRIEVE },
	{ "return",	RETURN },
	{ "round",	ROUND },
	{ "save",	SAVE },
	{ "seq",	SEQ },
	{ "set_ranges",	SET_RANGES },
	{ "size",	SIZE },
	{ "split",	SPLIT },
	{ "Stop",	STOP },
	{ "strlen",	STRLEN },
	{ "substr",	SUBSTR },
	{ "sum",	SUM },
	{ "terse",	TERSE },
	{ "true",	TRUE },
	{ "txt",	TXT },
	{ "type",	TYP },
	{ "typ",	TYP },
	{ "unlock",	UNLOCK },
	{ "unset",	UNSET },
	{ "verbose",	VERBOSE },
	{ "while",	WHILE },
	{ 0, 0 }
};

static struct Keywords ops[] = {	// for tok2txt only
	{ "&&", AND },
	{ "==", EQ },
	{ ">=", GE },
	{ ">",  GT },
	{ "<=", LE },
	{ "<",  LT },
	{ "!=", NE },
	{ "||", OR },
	{ "|",  B_OR },
	{ "&",  B_AND },
	{ "^",  B_XOR },
	{ "++", INCR },
	{ "--", DECR },
	{ ">>", RSH },
	{ "<<", LSH },
	{ "@call", CALL },		// internal use
	{ "skip", SKIP },		// internal use
	{ "arg", ARG },			// internal use
	{ 0, 0}
};

ulong
hash2(const char *s)
{
	if (sizeof(ulong) == 8)
	{	uint64_t h = 0x88888EEFL ^ (0x88888EEFL << 31);
		const char t = *s++;
 
		while (*s != '\0')
		{	h ^= ((h << 7) ^ (h >> (57))) + *s++;
		}
		return ((h << 7) ^ (h >> (57))) ^ t;
	} else
	{	ulong h = 0x88888EEFL;
		const char t = *s++;

		while (*s != '\0')
		{	h ^= ((h << 8) ^ (h >> (24))) + *s++;
		}
		return ((h << 7) ^ (h >> (25))) ^ t;
	}
}

static Lextok *
add_return(Lextok *p)
{	Lextok *e;

	if (!p->rgt)
	{	return p;
	}
	if (p->typ == '=')
	{	e = new_lex(RETURN, 0, 0);
		return new_lex(';', p, e);
	}
	if (p->rgt->typ == ';')
	{	p->rgt = add_return(p->rgt);
	} else if (p->rgt->typ != RETURN)
	{	e = new_lex(RETURN, 0, 0);
		p->rgt = new_lex(';', p->rgt, e);
	}
	return p;
}

static Lextok *
new_lex(int tp, Lextok *lft, Lextok *rgt)
{	Lextok *p;
	p = (Lextok *) emalloc(sizeof(Lextok), 73);
	p->typ = tp;
	p->lft = lft;
	p->rgt = rgt;
	p->lnr = p_lnr;
	p->tag = p_seq++;
	return p;
}

static Lextok *
mk_for(Lextok *a, Lextok *nm, Lextok *ar, Lextok *body)
{	Lextok *mrk, *seq, *asgn1, *asgn2, *loop;
	Lextok *cond, *whl, *txt;
	Lextok *setv, *one, *incr, *zero;
	Lextok *s1, *s2, *s3, *s4, *gt;
	static int ln = 0;	// parse time only
	int ix;

	// printf("for %s in %s\n", nm->s, ar->s);
	//	 asgn ;		nm.seq = size(ar);
	// loop: whl  ;		nm.mark < nm.seq
	//	 setv ;		nm.txt = retrieve(ar, nm.mark)
	//	 incr ;		nm.mark++
	//	 body ;
	//	 goto loop

	for (ix = 0; ix < Ncore; ix++)
	{	Var_nm *vn = mk_var(nm->s, PTR, ix);		// nm
		if (vn) { vn->rtyp = PTR; } // in case its an older var
	}	// so that we can use the var to cnt in nm.mark and nm.seq

	one  = new_lex(NR, 0, 0); one->val = 1;		// 1
	zero = new_lex(NR, 0, 0); 			// 0
	mrk  = new_lex('.', nm, new_lex(MARK, 0, 0));	// nm.mark
	seq  = new_lex('.', nm, new_lex(SEQ,  0, 0));	// nm.seq
	txt  = new_lex('.', nm, new_lex(TXT,  0, 0));	// nm.txt

	asgn1 = new_lex('=', seq, new_lex(SIZE, 0, ar));	// seq = size(ar)
	asgn2 = new_lex('=', mrk, zero);			// mrk = 0

	cond = new_lex(LT,  mrk, seq);			// mrk < seq
	incr = new_lex('=', mrk, new_lex('+', mrk, one)); // mrk++
	setv = new_lex('=', txt, new_lex(RETRIEVE, mrk, ar)); // txt = retrieve(ar, mrk)

	loop = new_lex(NAME, 0, 0);
	loop->s = (char *) emalloc(8*sizeof(char), 74);
	snprintf(loop->s, 8, "_@%d_", ln++);

	gt  = new_lex(GOTO,  loop, 0);	// shouldnt be needed
	s1  = new_lex(';',   body, gt);
	s2  = new_lex(';',   incr, s1);
	s3  = new_lex(';',   setv, s2);
	whl = new_lex(WHILE, cond, s3);
	mk_lab(loop, whl);

	s4  = new_lex(';',  asgn2, whl);

	return new_lex(';',  asgn1, s4);
}

static void
mk_lab(Lextok *t, Lextok *p)
{	Var_nm *n;

	assert(p);
	for (n = lab_lst; n; n = n->nxt)
	{	if (strcmp(n->nm, t->s) == 0)
		{	if (!no_match)
			{	Lextok *op = (Lextok *) n->pm;
				if (p
				&&  op
				&&  op->lnr != p->lnr)
				{	fprintf(stderr, "line %d: warning: label '%s' redefined\n",
						p->lnr, t->s);
			}	}
			if (n->pm != (Prim *) p)
			{	n->pm = (Prim *) p;
				n->cdepth = Cdepth[0];
			}
			return;
	}	}
	n = (Var_nm *) emalloc(sizeof(Var_nm), 75);	// mk_lab
	n->nm = t->s;
	n->pm = (Prim *) p; // slight abuse of type
	n->cdepth = Cdepth[0];	// during parsing only
	n->nxt = lab_lst;
	lab_lst = n;
}

static Lextok *
find_label(char *s)
{	Var_nm *n;

	for (n = lab_lst; n; n = n->nxt)
	{	if (strcmp(n->nm, s) == 0)
		{	return (Lextok *) n->pm;
	}	}
	fprintf(stderr, "error: label '%s' undefined\n", s);
	return (Lextok *) 0;
}

static void
expect(int if_t, int then_t, int else_t)
{	int n;

	n = fgetc(pfd);
	if (n == if_t)
	{	yylval->typ = then_t;
	} else
	{	ungetc(n, pfd);
		yylval->typ = else_t;
	}
}

static void
string(void)
{	int n, i = 0;

	yylval->typ = STRING;
	yylval->s = "";
	memset(yytext, 0, MAXYYTEXT);
	while ((n = fgetc(pfd)) != '"')
	{	if (n == '\\')
		{	yytext[i++] = (char) n;
			n = fgetc(pfd);
		}
		if (n == EOF || i > 1024)
		{	yyerror("unterminated string");
			break;
		}
		yytext[i++] = (char) n;
	}
	yytext[i] = '\0';
	yylval->s = emalloc(strlen(yytext)+1, 76);
	strcpy(yylval->s, yytext);
}

static void
fixstr(Lextok *t)
{	char *s;
	assert(t->typ == STRING);
	for (s = t->s; *s != '\0'; s++)
	{	if (*s == '\\')
		{	if (*(s+1) == 't')
			{	*s++ = ' ';
				*s = '\t';
			} else if (*(s+1) == '\\')
			{	s++;
			} else if (*(s+1) == 'n')
			{	if (*(s+2) == '\0')
				{	*s++ = '\n';
					*s = '\0';
				} else
				{	*s++ = ' ';
					*s = '\n';
	}	}	}	}
}

static int
prog_lex(void)
{	int n, i;

	yylval = (Lextok *) emalloc(sizeof(Lextok), 77);
	yylval->lnr = p_lnr;
	yytext[0] = '\0';
	for (;;)
	{	n = fgetc(pfd);
		if (n == '\n')
		{	p_lnr++;
			yylval->lnr++;
		}
		if (isspace((uchar) n))
		{	continue;
		}
		if (isdigit((uchar) n))
		{	for (i = 0; isdigit((uchar) n); i++)
			{	yytext[i] = (char) n;
				n = fgetc(pfd);
			}
			ungetc(n, pfd);
			yytext[i] = '\0';
			yylval->val = atoi(yytext);
			yylval->typ = NR;
		} else if (isalpha((uchar) n))
		{	for (i = 0; isalnum((uchar) n) || n == '_'; i++)
			{	yytext[i] = (char) n;
				n = fgetc(pfd);
			}
			ungetc(n, pfd);
			yytext[i] = '\0';
			yylval->s = emalloc(strlen(yytext)+1, 78);
			strcpy(yylval->s, yytext);
			yylval->typ = NAME;
			for (i = 0; key[i].s; i++)
			{	if (strcmp(yytext, key[i].s) == 0)
				{	yylval->typ = key[i].t;
					break;
			}	}
		} else
		{	if (n == '#')
			{	n = fgetc(pfd);
				if (n == '\n' || n == '\r')
				{	p_lnr++;
					yylval->lnr++;
					continue;
				}
				if (n == '#' || isspace((uchar) n)) // comment
				{	while ((n = fgetc(pfd)) != '\n')
					{	if (n == EOF)
						{	break;
					}	}
					p_lnr++;
					yylval->lnr++;
					continue;
				}
				ungetc(n, pfd);
				yylval->typ = '#';
				break;
			}
			switch (n) {
			case '\\': yylval->typ = fgetc(pfd); break;
			case '"': string(); break;
			case '>':
				  n = fgetc(pfd);
				  switch (n) {
				  case '=': yylval->typ = GE; break;
				  case '>': yylval->typ = RSH; break;
				  default:  yylval->typ = GT; ungetc(n, pfd); break;
				  }
				  break;
			case '<':
				  n = fgetc(pfd);
				  switch (n) {
				  case '=': yylval->typ = LE; break;
				  case '<': yylval->typ = LSH; break;
				  default:  yylval->typ = LT; ungetc(n, pfd); break;
				  }
				  break;
			case '=': expect('=',  EQ,  n); break;
			case '!': expect('=',  NE,  n); break;
			case '|': expect('|',  OR,  B_OR); break;
			case '&': expect('&', AND, B_AND); break;
			case '+': expect('+', INCR, n); break;
			case '-': expect('-', DECR, n); break;
			default : yylval->typ = n; break;
		}	}
		break;
	}
	return yylval->typ;
}

static void
tok2txt(Lextok *t, FILE *x)
{	int i;
	char buf[1024], *s;

	if (!t)
	{	return;
	}

	if (x != stdout)
	{	fprintf(x, "N%d:%d: ", t->tag, t->lnr);
	}
	for (i = 0; key[i].s; i++)
	{	if (t->typ == key[i].t)
		{	fprintf(x, "%s", key[i].s);
			break;
	}	}

	if (t->typ == NAME || t->typ == STRING)
	{	strncpy(buf, t->s, sizeof(buf)-1);
		buf[sizeof(buf)-1] = '\0';
		for (s = buf; *s != '\0'; s++)
		{	if (*s == '\n')
			{	*s = ' ';
		}	}
		fprintf(x, " %s", buf);
	} else if (t->typ == NR)
	{	fprintf(x, " %d", t->val);
	} else if (t->typ == CALL)
	{	fprintf(x, "  %s()", t->lft->s);
	} else
	{	if (!key[i].s)
		{	for (i = 0; ops[i].s; i++)
			{	if (t->typ == ops[i].t)
				{	break;
			}	}
			if (ops[i].s)
			{	fprintf(x, "%s", ops[i].s);
			} else if (isprint(t->typ))
			{	if ((t->typ == '#'
				||   t->typ == '@')
				&&   t->rgt && t->rgt->s)
				{	fprintf(x, "%s", t->rgt->s);
				} else
				{	fprintf(x, "'%c'", t->typ);
				}
			} else
			{	fprintf(x, "%d", t->typ);
	}	}	}

	if (x == stdout)
	{	fprintf(x, "\n");
	}
}

static void
draw_ast(Lextok *t, FILE *x)
{
	if (!t || t->tag)
	{	return;
	}
	t->tag = p_seq++;

	fprintf(x, "n%d [label=\"", t->tag);
	tok2txt(t, x);
	fprintf(x, "\"];\n");

	if (t->lft)
	{	draw_ast(t->lft, x);
		fprintf(x, "n%d -> n%d [label=L];\n", t->tag, t->lft->tag);
	}
	if (t->rgt)
	{	draw_ast(t->rgt, x);
		fprintf(x, "n%d -> n%d [label=R];\n", t->tag, t->rgt->tag);
	}
}

static void
opt_fsm(Lextok *t)	// optimize the fsm a bit
{
	if (!t || (t->visit & 8))
	{	return;
	}
	t->visit |= 8;

	while (t->a && t->a->typ == SKIP)
	{	t->a = t->a->a;
	}

	while (t->b && t->b->typ == SKIP)
	{	t->b = t->b->a;
	}

	while (t->a && t->a->typ == ';' && !t->a->b)
	{	t->a = t->a->a;
	}

	opt_fsm(t->a);
	opt_fsm(t->b);
}

static void
draw_fsm(Lextok *t, FILE *x)
{
	if (!t || (t->visit & 4))
	{	return;
	}
	t->visit |= 4;

	if (!t->tag)
	{	t->tag = p_seq++;
	}

	fprintf(x, "n%d [label=\"", t->tag);
	tok2txt(t, x);
	fprintf(x, "\"];\n");

	if (t->typ == NEXT_T || t->typ == STOP)
	{	return;
	}
	if (t->typ == CALL)
	{	if (t->c)
		{	draw_fsm(t->c, x);
			fprintf(x, "n%d -> n%d [label=nxt];\n", t->tag, t->c->tag);
		}
		return;
	}

	if (t->a)
	{	draw_fsm(t->a, x);
		fprintf(x, "n%d -> n%d [label=nxt];\n", t->tag, t->a->tag);
	}
	if (t->b)
	{	draw_fsm(t->b, x);
		fprintf(x, "n%d -> n%d [label=else];\n", t->tag, t->b->tag);
	}
}

#if 0
static void
clr_tags(Lextok *t)
{
	if (!t || t->tag == 0)
	{	return;
	}
	t->tag = 0;
	clr_tags(t->lft);
	clr_tags(t->rgt);
}
#endif

#undef DEBUG
#ifdef DEBUG
static void
indent(int level, const char *s)
{	int i;

	for (i = 0; i < level; i++)
	{	printf(" \t");
	}
	printf("%s", s);
}

static void
dump_tree(Lextok *t, int i)
{
	if (!t || (t->visit&16))
	{	indent(i, "");
		printf("<<%p::%d>>\n", (void *) t, t?t->typ:-1);
		return;
	}
	t->visit |= 16;
	indent(i, "");

	printf("<%p> ", (void *) t);

	if (t->s) { printf("%s%s ", t->f?"fct ":"", t->s); }
	tok2txt(t, stdout);
	if (t->lft)
	{	indent(i, "L:\n");
		dump_tree(t->lft, i+1);
	}
	if (t->rgt)
	{	indent(i, "R:\n");
		dump_tree(t->rgt, i+1);
	}
	if (t->a)
	{	indent(i, "a:\n");
		dump_tree(t->a, i+1);
	}
	if (t->b)
	{	indent(i, "b:\n");
		dump_tree(t->b, i+1);
	}
	if (t->c)
	{	indent(i, "c:\n");
		dump_tree(t->c, i+1);
	}
}

#if 0
static void
clean_tree(Lextok *t)
{
	if (!t || (t->visit&64))
	{	return;
	}
	t->visit &= ~16;
	t->visit |= 64;

	if (t->lft)
	{	clean_tree(t->lft);
	}
	if (t->rgt)
	{	clean_tree(t->rgt);
	}
	if (t->a)
	{	clean_tree(t->a);
	}
	if (t->b)
	{	clean_tree(t->b);
	}
	if (t->c)
	{	clean_tree(t->c);
	}
}
#endif
#endif

static Lextok *
fix_while(Lextok *t)
{	// convert the WHILE into an IF2

	if (!t || (t->visit & 1))
	{	return t;
	}
	t->visit |= 1;
	t->lft = fix_while(t->lft);
	t->rgt = fix_while(t->rgt);

#if 0
           while                             ;d1 ';'
           /    \                           / \
        cond   body               .------>if t \
                                  |      /  \   \
                                  |   cond  f2   \
                                  |       t/  \   .
       create 5 extra nodes:      |       ;g3  .->skip e4
       returns top 1              |      / \
                                  |   body  .
                                  |         |
                                  .---------.
#endif
	if (t->typ == WHILE)
	{	Lextok *d = (Lextok *) emalloc(sizeof(Lextok), 79); // 1
		Lextok *e = (Lextok *) emalloc(sizeof(Lextok), 79); // 4
		Lextok *f = (Lextok *) emalloc(sizeof(Lextok), 79); // 2
		Lextok *g = (Lextok *) emalloc(sizeof(Lextok), 79); // 3

		d->tag = p_seq++; d->lnr = t->lnr; d->typ = ';';  // d1
		e->tag = p_seq++; e->lnr = t->lnr; e->typ = SKIP; // e4
		f->tag = p_seq++; f->lnr = t->lnr; f->typ = 0;    // f2
		g->tag = p_seq++; g->lnr = t->lnr; g->typ = ';';  // g3

		d->visit |= 1;
		e->visit |= 1;
		g->visit |= 1;

		d->lft = t;
		d->rgt = e;

		t->typ = IF2;	// keep cond on t->lft
		t->a   = g;	// replaced in mk_fsm
		// t->b   = e;	// causes problems

		g->lft = t->rgt;
		g->rgt = t;

		t->rgt = f;	// needed for mk_fsm
		f->lft = g;	// needed for mk_fsm
		// f->rgt = e;	// causes problems
		f->visit |= 1;

		return d;
	}

	if (t->typ == IF)
	{	t->a = t->rgt->lft;
		t->b = t->rgt->rgt;
	}
	return t;
}

static void
add_fct(Lextok *t)
{	Function *f;

	assert(t->lft && t->rgt);

	f = (Function *) emalloc(sizeof(Function), 80);
	f->nm	  = t->lft->lft;
	f->formal = t->lft->rgt;
	f->body	  = t->rgt;
	f->nxt	  = functions;
	functions = f;
}

static void
map_var(Prim **ref_p, const char *fnm, Lextok *name, Lextok *expr, const int ix)
{	Var_nm *n;
	Rtype tmp;

	memset(&tmp, 0, sizeof(Rtype));

	assert(name->typ == NAME);		// name of formal param
	assert(ix >= 0 && ix < Ncore);

	Cdepth[ix]--;				// evaluate param in callers context
	eval_prog(ref_p, expr, &tmp, ix);
	Cdepth[ix]++;				// restore context

	if (expr->typ == NAME && is_aname(expr->s, ix))
	{	fprintf(stderr, "error: the basename of an associative array (%s) cannot be passed as a parameter\n",
			expr->s);
		show_error(stderr, expr->lnr);
	}

	n = mk_var(name->s, tmp.rtyp, ix);

	if (n->cdepth == 0)
	{	fprintf(stderr, "error: parameter name '%s' clashes with a global variable named '%s'\n",
			n->nm, n->nm);
	}

	if (sep[ix].Verbose>1)
	{	assert(ix >= 0 && ix < Ncore);
		printf("level %d, created parameter %s of type: ",
			Cdepth[ix], name->s);
		what_type(stdout, tmp.rtyp);
		printf(": ");
	}

	switch (tmp.rtyp) {
	case VAL:
		n->v  = tmp.val;
		if (sep[ix].Verbose>1)
		{	printf("%d\n", tmp.val);
		}
		break;
	case STR:
#if 1
		n->s = tmp.s;
#else
		n->s = (char *) hmalloc(strlen(tmp.s) + 1, ix, 131);		// map_var
		strcpy(n->s, tmp.s);
#endif
		if (sep[ix].Verbose>1)
		{	printf("%s\n", tmp.s);
		}
		break;
	case PTR:
		if (tmp.ptr)
		{	n->pm = tmp.ptr;
		} else
		{	n->pm = &none;
			n->pm->seq = 0;
		}
		if (sep[ix].Verbose>1 && n->pm->fnm)
		{	printf("%s:%d\n",
				n->pm->fnm, n->pm->lnr);
		}
		break;
	default:
		fprintf(stderr, "line %d: error: unknown type for %s\n",
			name->lnr, name->s);
		break;
	}
}

static void
set_actuals(Prim **ref_p, const char *fnm, Lextok *formal, Lextok *actual, const int ix)
{	// formal -- names on right, tree on left
	// actual -- expr  on right, tree on left

	if (!formal || !actual)
	{	if (formal)
		{	fprintf(stderr, "error: %s(): missing parameter(s)\n", fnm); // allow?
		} else if (actual)
		{	fprintf(stderr, "error: %s(): too many parameters\n", fnm);
		}
		return;
	}
	// descend tree left to leafs
	if (formal->lft && actual->lft)
	{	set_actuals(ref_p, fnm, formal->lft, actual->lft, ix);
	}

	if (formal->rgt)
	{	if (actual->rgt)
		{	map_var(ref_p, fnm, formal->rgt, actual->rgt, ix);
		}
	} else // if (!actual->rgt)
	{	map_var(ref_p, fnm, formal, actual, ix);
	}
}

static void
find_fct(Lextok *t, int ix)
{	Function *f;

	// fct name in t->lft
	// actual params, if any, in t->rgt
	// link call to formals and fct body

	assert(t && t->lft && t->lft->s);

	for (f = functions; f; f = f->nxt)
	{	if (strcmp(f->nm->s, t->lft->s) == 0)
		{	t->f = f;	// for access to f->formal
			t->a = f->body;

			if (t->a->typ == IF2
			&& !t->a->rgt->rgt)	// pending return fix
			{	t->a->rgt->rgt = new_lex(RETURN, 0, 0);
			}

			if (!f->has_fsm)
			{	f->has_fsm++;
				mk_fsm(f->body, ix);
			}
			break;
	}	}
	if (!f)
	{	fprintf(stderr, "line %d: error: no such function '%s'\n",
			t->lnr, t->lft->s);
	}
}

static void
mk_fsm(Lextok *t, const int ix)
{	Block *b;

	if (!t || (t->visit & 2))
	{	return;
	}
	t->visit |= 2;

	assert(ix >= 0 && ix < Ncore);
	
	switch (t->typ) {
	case CALL:	// function call to t->lft->s
		if (!t->c && block[ix])
		{	t->c = block[ix]->leave;
		}
		push_context(0, t->c, ix, 0); // for value of t->c, for RETURN
		block[ix]->btyp = CALL;
		 find_fct(t, ix);	// sets t->f and t->a, calls mk_fsm once
		(void) pop_context(ix, 0);
		break;
	case ';':
		t->a = t->lft;
		push_context(t->lft, t->rgt, ix, 0);
		 mk_fsm(t->lft, ix);
		(void) pop_context(ix, 0);
		mk_fsm(t->rgt, ix);
		break;
	case IF:
	case IF2:
		t->a = t->rgt->lft;
		t->b = t->rgt->rgt;
		if (!t->b && block[ix])
		{	t->b = block[ix]->leave;
		}
		mk_fsm(t->rgt->lft, ix);	// then
		mk_fsm(t->rgt->rgt, ix);	// else
		break;
	case ELSE:
		t->a = t->rgt;
		mk_fsm(t->rgt, ix);
		break;
	case WHILE:	// replace with IF2
		mk_fsm(t->rgt, ix);
		break;
	case CONTINUE:	// repeat while
		b = block[ix];
		while (b && b->btyp != IF2)
		{	b = b->nxt;
		}
		if (!b)
		{	fprintf(stderr, "error:%d: continue outside while-loop\n", t->lnr);
			t->a = new_lex(STOP, 0, 0);
			break;
		}
		t->a = b->enter;
		t->b = 0;
		break;
	case BREAK:	// complete while
		b = block[ix];
		while (b && b->btyp != IF2)
		{	b = b->nxt;
		}
		if (!b)
		{	fprintf(stderr, "error:%d: break outside while-loop\n", t->lnr);
			for (b = block[ix]; b; b = b->nxt)
			{	printf("\t%d - %2d,%2d\n",
					b->btyp,
					b->enter?b->enter->tag:0,
					b->leave?b->leave->tag:0);
			}
			sep[ix].T_stop++;
			t->a = new_lex(STOP, 0, 0);
			break;
		}
		t->a = b->leave;
		t->b = 0;
		break;
	case GOTO:
		t->a = find_label(t->lft->s);
		t->b = 0;
		break;
	case FUNCTION:
		t->a = t->c;
		break;
	case RETURN:
		b = block[ix];
		while (b && b->btyp != CALL)
		{	b = b->nxt;
		}
		t->a = b?b->leave:0;
		t->b = 0;
		break;
	default:
		b = block[ix];
		if (!b)
		{	t->a = t->b = 0;
			break;
		}
		if (b->btyp == WHILE)
		{	t->a = b->enter;
		} else
		{	t->a = b->leave;
		}
		break;
	}
}

static void
yyerror(const char *s)
{
	printf("line %d: %s ", p_lnr, s);
	if (yylval)
	{	tok2txt(yylval, stdout);
	} else
	{	printf("near '%s'\n", yytext);
	}
	sep[0].T_stop++;
}

static int
yylex(void)
{	int n = prog_lex();

	if (p_debug == 1)
	{	tok2txt(yylval, stdout);
	}
	return n;
}

static void
eval_eq(int eq, Rtype *a, Rtype *rv)	// eq=1: EQ, eq=0: NE
{
	if (a->rtyp != rv->rtyp)
	{	// can happen for a failed array lookup
		// e.g. . == 0 or . != 0
		if (a->rtyp == VAL
		&&  rv->rtyp == STR
		&&  rv->s
		&&  isdigit((uchar) rv->s[0]))
		{	rv->rtyp = VAL;
			rv->val = atoi(rv->s);
		} else if (rv->rtyp == VAL
		&& a->rtyp == STR
		&& a->s
		&& isdigit((uchar) a->s[0]))
		{	a->rtyp = VAL;
			a->val = atoi(a->s);
		} else
		{	rv->rtyp = VAL;
			rv->val  = eq?0:1;
			return;
		}
	}
	rv->rtyp = VAL;
	switch (a->rtyp) {
	case STR:
		if (!a->s)
		{	a->s = "";
		}
		// V4.4: if we compare a token text (as a string) with another string
		// as in .txt == "\\"
		// the token could be a single character "\" but the comparison text
		// cannot be written to match that, it'll be "\\"
		// the next if handles this special case:
		if (a->s[0]  == '\\' && a->s[1]  == '\0'
		&&  rv->s[0] == '\\' && rv->s[1] == '\\' && rv->s[2] == '\0')
		{	rv->val = (eq)?1:0;
			break;
		}
		if (eq)
		{	rv->val = (strcmp(a->s, rv->s) == 0);
		} else
		{	rv->val = (strcmp(a->s, rv->s) != 0);
		}
		break;
	case VAL:
		if (eq)
		{	rv->val = (a->val == rv->val);
		} else
		{	rv->val = (a->val != rv->val);
		}
		break;
	case PTR:
		if (eq)
		{	rv->val = (a->ptr == rv->ptr);
		} else
		{	rv->val = (a->ptr != rv->ptr);
		}
		break;
	default:
		yyerror("bad eq/ne types");
		break;
	}
}

void
what_type(FILE *fd, Renum t)
{
	switch (t) {
	case VAL:
		fprintf(fd, "number");
		break;
	case STR:
		fprintf(fd, "string");
		break;
	case PTR:
		fprintf(fd, "pointer");
		break;
	case STP:
		fprintf(fd, "stop");
		break;
	case PRCD:
		fprintf(fd, "proceed");
		break;
	default:
		fprintf(fd, "-unknown-");
		break;
	}
}

#define Assert(s, e, q)	\
	if (!(e))	\
	{	if (Ncore > 1) fprintf(stderr, "(%d) ", ix);	\
		fprintf(stderr, "line %d: %s: expected %s, in ", q->lnr, s, #e); \
		tok2txt(q, stderr); \
		show_error(stderr, q->lnr); \
		rv->rtyp = STP; \
		sep[ix].T_stop++;	\
		return;		\
	}

static void
set_var(Lextok *q, Rtype *rv, const int ix)
{	Var_nm *n;

	assert(ix >= 0 && ix < Ncore);
	Assert("set_var", q->typ == NAME, q);
	n = mk_var(q->s, rv->rtyp, ix);

	if (!rv->rtyp)
	{	rv->rtyp = n->rtyp;
	}

	if (n->rtyp != rv->rtyp)
	{	if (sep[ix].Verbose>1)
		{	fprintf(stderr, "\nline %d: warning: type coercion '%s'",
				q->lnr, q->s);
			fprintf(stderr, " from "); what_type(stderr, n->rtyp);
			fprintf(stderr, " to "); what_type(stderr, rv->rtyp);
			fprintf(stderr, "\n");
		}
		n->rtyp = rv->rtyp;
	}
		
	if (rv->rtyp == n->rtyp)
	{	switch (n->rtyp) {
		case STR:
			n->s = rv->s;
			break;
		case VAL:
			n->v = rv->val;
			break;
		case PTR:
			if (rv->ptr)
			{	n->pm = rv->ptr;
			} else
			{	n->pm = &none;
				n->pm->seq = 0;
			}
			break;
		default:
			break;
		}
		return;
	}

	fprintf(stderr, "line %d: error: variable '%s' has type '",
		q->lnr, q->s);
	what_type(stderr, n->rtyp);
	fprintf(stderr, "' not '");
	what_type(stderr, rv->rtyp);
	fprintf(stderr, "'\n");
	sep[ix].T_stop++;
	rv->rtyp = STP;
}

static Var_nm *
get_var(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)
{	Var_nm *n = 0;
	Rtype tmp;

	if (q)
	{	assert(ix >= 0 && ix < Ncore);
		if (q->typ == NAME)
		{	if (q->core)	// qualified name
			{	eval_prog(ref_p, q->core, &tmp, ix);
				assert(tmp.rtyp == VAL);
				n = check_var(q->s, tmp.val);
			} else
			{	n = mk_var(q->s, 0, ix);
			}
			rv->rtyp = n->rtyp;
		} else
		{	if (Ncore > 1) fprintf(stderr, "(%d) ", ix);
			fprintf(stderr, "line %d: unexpected variable type, in ", q->lnr);
			tok2txt(q, stderr);
			show_error(stderr, q->lnr);
			rv->rtyp = STP;
			sep[ix].T_stop++;
		}
	}
	if (0 && n)
	{	printf("get_var %s, depth %d\n", n->nm, n->cdepth);
	}
	return n;
}

extern int do_split(char *, const char *, Rtype *, const int);	// cobra_array.c

static void
split(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)
{	// q->lft: source string to split on commas
	// q->rgt: name of target array,
	//  create if it doesn't exist, unset if it exists
	// returns nr of fields
	Rtype a, b, c;

	assert(q->lft && q->rgt);
	eval_prog(ref_p, q->lft, &a, ix);
	eval_prog(ref_p, q->rgt, &b, ix);

	if (a.rtyp != STR)
	{	fprintf(stderr, "error: 1st arg of split is not a string\n");
		rv->rtyp = STP;
		return;
	}
	if (q->rgt->typ != NAME)
	{	fprintf(stderr, "error: 2nd arg of split is not a name %d, but %d\n", NAME, q->rgt->typ);
		rv->rtyp = STP;
		return;
	}

	rv->val = do_split(a.s, q->rgt->s, &c, ix);	// cobra_array.c
	rv->rtyp = VAL;					// the nr of fields
}

static void
substr(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)
{	// q->lft: string
	// q->rgt->lft: starting index
	// q->rgt->rgt: nr of chars
	Rtype a, b, c;

	assert(q->lft && q->rgt);
	assert(q->rgt->lft && q->rgt->rgt);

	eval_prog(ref_p, q->lft, &a, ix);
	eval_prog(ref_p, q->rgt->lft, &b, ix);
	eval_prog(ref_p, q->rgt->rgt, &c, ix);

	rv->rtyp = STR;
	rv->s = "?";

	if (a.rtyp != STR)
	{	fprintf(stderr, "error: 1st arg of substr is not a string\n");
		rv->rtyp = STP;
		return;
	}

	if (b.rtyp != VAL
	||  c.rtyp != VAL)
	{	fprintf(stderr, "error: 2nd or 3rd arg of substr is not a value\n");
		rv->rtyp = STP;
		return;
	}

	if (strlen(a.s) <= b.val)
	{	rv->s = "";
		return;
	}
	rv->s = (char *) hmalloc((c.val+1) * sizeof(char), ix, 132);		// substr
	strncpy(rv->s, &a.s[b.val], c.val);
	return;
}

static int
nr_marks_int(int a, const int ix)
{
	global_n = a;
	nr_marks_range((void *) &ix);
	return tokrange[ix]->param;
}

static char *
disambiguate(const char *s, const int ix)	// autosar checker support fct, rule M2-10-1
{	char *m, *om;
	const char *os = s;

	if (!s || *s == '\0') { return ""; }

	om = m = (char *) hmalloc(strlen(s)+1, ix, 69);

	while (*s != '\0')
	{	*m = (char) tolower((int) *s);
		switch (*s) {
		case '_': // skip
			m--;
			break;
		case 'O':		// uppercase O
		case 'o':		// lowercase o
			*m = '0';	// zero
			break;
		case 'I':
		case 'l':
			*m = '1';
			break;
		case 'S':
			*m = '5';
			break;
		case 'Z':
			*m = '2';
			break;
		case 'B':
			*m = '8';
			break;
		case 'r':
			if (*(s+1) == 'n')
			{	*m = 'm';
				s++;
			}
			break;	
		default:
			break;
		}
		m++;
		s++;
	}
	*m = '\0';

	if (verbose)
	{	printf("disambiguate: '%s' -> '%s'\n", os, om);
	}
	return om;
}

static void
print_args(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)
{	FILE *tfd = (track_fd) ? track_fd : stdout;

	assert(ix >= 0 && ix < Ncore);
	switch (q->typ) {
	case ARG:
		print_args(ref_p, q->lft, rv, ix);
		print_args(ref_p, q->rgt, rv, ix);
		return;
	case MARKS:
		eval_prog(ref_p, q->lft, rv, ix);
		Assert("marks", rv->rtyp == VAL, q->lft);
		rv->val = nr_marks_int(rv->val, ix);
		fprintf(tfd, "%d", rv->val);
		return;
	case SIZE:
		fprintf(tfd, "%d", q->rgt?array_sz(q->rgt->s, ix):0);
		return;
	case DISAMBIGUATE:
		{ Var_nm *n = get_var(ref_p, q->rgt, rv, ix);
		  Assert("disambiguate", rv->rtyp == STR, q->rgt);
		  fprintf(tfd, "%s",  disambiguate((char *) n->s, ix));
		}
		return;
	case RETRIEVE:
		eval_prog(ref_p, q->lft, rv, ix);
		Assert("retrieve", rv->rtyp == VAL, q->lft);
		fprintf(tfd, "%s", array_ix(q->rgt->s, rv->val, ix));
		return;
	case SUBSTR:
		substr(ref_p, q, rv, ix);
		fprintf(tfd, "%s", rv->s);
		return;
	case SPLIT:
		split(ref_p, q, rv, ix);
		fprintf(tfd, "%s", rv->s);	// tab separated fields
		return;
	case STRLEN:
		eval_prog(ref_p, q->lft, rv, ix);
		Assert("strlen", rv->rtyp == STR, q->lft);
		rv->rtyp = VAL;
		rv->val = (int) strlen(rv->s);
		break;

	case NAME:
		if (q->rgt)	// q.? on q->rgt
		{	eval_prog(ref_p, q->rgt, rv, ix);
		} else		// string variable
		{	eval_prog(ref_p, q, rv, ix);
		}
		break;
	default:
		eval_prog(ref_p, q, rv, ix);
		break;
	}

	switch (rv->rtyp) {
	case VAL:
		fprintf(tfd, "%d", rv->val);
		break;
	case PTR:
		fprintf(tfd, "%p", (void *) rv->ptr);
		break;
	case STR:
		if (rv->s && strchr(rv->s, '\\'))
		{ char *s;
		  if (rv->s[1] == '\0')	// a single backslash character
		  {	fprintf(tfd, "\\");
		  } else
		  for (s = rv->s; *s != '\0'; s++)
		  {	if (*s != '\\' || *(s+1) == '\\')
			{	fprintf(tfd, "%c", *s);
		  }	}
		} else
		{ fprintf(tfd, "%s", rv->s);
		}
		break;
	default:
		break; // cannot happen
	}
}

static void
push_context(Lextok *ent, Lextok *out, int ix, int n)
{	Block *b;

	if (0 && sep[ix].P_debug == 2)
	{	printf("Push(%d:%d) N%d N%d\n", ix, n,
			ent?ent->tag:-1,
			out?out->tag:-1);
	}

	assert(ix >= 0 && ix < Ncore);

	if (b_free[ix])
	{	b = b_free[ix];
		b_free[ix] = b->nxt;
	} else
	{	if (Ncore != 1)
		{	b = (Block *) hmalloc(sizeof(Block), ix, 133);
		} else
		{	b = (Block *) emalloc(sizeof(Block), 81);
	}	}

	if (ent && ent->typ == IF2)
	{	b->btyp = IF2;
	} else
	{	b->btyp = 0;
	}
	b->enter  = ent;
	b->leave  = out;
	b->nxt    = block[ix];
	block[ix] = b;
}

static void
bfree(Block *b, int ix)
{
	assert(b);
	assert(ix >= 0 && ix < Ncore);

	b->nxt = b_free[ix];
	b_free[ix] = b;
}

static Block *
pop_context(int ix, int n)
{	Block *b;

	assert(ix >= 0 && ix < Ncore);

	if (!block[ix])
	{	return (Block *) 0;
	}
	b = block[ix];
	if (0 && sep[ix].P_debug == 2)
	{	printf("	Pop(%d:%d) N%d N%d", ix, n,
			b->enter?b->enter->tag:-1,
			b->leave?b->leave->tag:-1);
		if (b->btyp == WHILE)
		{	printf(" <while>");
		}
		printf("\n");
	}
	block[ix] = block[ix]->nxt;
	return b;
}

static void
str2val(Rtype *rv)
{
	if (rv->rtyp == STR
	&&  isdigit((uchar) rv->s[0]))
	{	rv->rtyp = VAL;
		rv->val = atoi(rv->s);
	} else if (!rv->rtyp)
	{	rv->rtyp = VAL;
		rv->val  = 0;
	}
}

static void
plus(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)	// strings or values
{	Rtype tmp;
	memset(&tmp, 0, sizeof(Rtype));

	assert(ix >= 0 && ix < Ncore);

	eval_prog(ref_p, q->lft, &tmp, ix);
	eval_prog(ref_p, q->rgt,   rv, ix);

	if (tmp.rtyp == STP
	||  rv->rtyp == STP)
	{	return;
	}

	if (!tmp.rtyp)
	{	tmp.rtyp = VAL;
		tmp.val = 0;
	}
	if (!rv->rtyp)
	{	rv->rtyp = VAL;
		rv->val = 0;
	}

	if (tmp.rtyp == VAL
	&&  rv->rtyp == STR)
	{	str2val(rv);
	} else if (tmp.rtyp == STR
	&&  rv->rtyp == VAL)
	{	str2val(&tmp);
	}

	Assert("plus1", tmp.rtyp == rv->rtyp, q->lft);
	Assert("plus2", tmp.rtyp == VAL || tmp.rtyp == STR, q->rgt);

	if (tmp.rtyp == VAL)
	{	rv->val = tmp.val + rv->val;
	} else if (tmp.rtyp == STR)
	{	int n = strlen(tmp.s)+strlen(rv->s)+1;
		char *s = (char *) hmalloc(n, ix, 134);	// plus
		snprintf(s, n, "%s%s", tmp.s, rv->s);	// plus
		rv->s = s;
	} else
	{	fprintf(stderr, "line %d: error: invalid addition attempt\n", q->lnr);
		rv->rtyp = STP;
		sep[ix].T_stop++;
	}
}

#define binop(op)				\
	eval_prog(ref_p, q->lft, &tmp, ix);	\
	if (tmp.rtyp == STP) { break; }		\
	str2val(&tmp); 				\
	if (tmp.rtyp != VAL) { \
		printf("%d binop='	lft: ", q->lnr); \
		tok2txt(q->lft, stdout); \
		printf(" 	'%s'	rgt: ", #op); \
		tok2txt(q->rgt, stdout); \
	} \
	Assert("binop1", tmp.rtyp == VAL, q->lft);	\
	eval_prog(ref_p, q->rgt, rv, ix);	\
	if (rv->rtyp == STP) { break; }		\
	str2val(rv); 				\
	Assert("binop2", rv->rtyp == VAL, q->rgt);	\
	rv->val = tmp.val op rv->val

#define rbinop(op)				\
	eval_prog(ref_p, q->lft, &tmp, ix);	\
	if (tmp.rtyp == STP) { break; }		\
	if (tmp.rtyp != VAL) { printf("rbinop='%s'\t", #op); } \
	Assert("rbinop1", tmp.rtyp == VAL, q->lft);	\
	eval_prog(ref_p, q->rgt, rv, ix);	\
	if (rv->rtyp == STP) { break; }		\
	Assert("rbinop2", rv->rtyp == VAL && rv->val != 0, q->rgt); \
	rv->val = tmp.val op rv->val

#define unop(op)				\
	eval_prog(ref_p, q->rgt, rv, ix);	\
	str2val(rv);				\
	Assert("unop", rv->rtyp == VAL, q->rgt);	\
	rv->val = op rv->val

static void
match_anywhere(Prim **ref_p, Lextok *q, Rtype *rv, const int ix, Prim *p)
{	char *y;

	if (q->rgt->typ == NAME)
	{	rv->val = 0;
		y = p?p->txt:"";
		while ((y = strstr(y, q->rgt->s)) != NULL)
		{	rv->val++;
			y++;
		}
		return;
	}
	eval_prog(ref_p, q->rgt, rv, ix);
	switch (rv->rtyp) {
	case STR:
		rv->rtyp = VAL;
		rv->val = 0;
		y = p?p->txt:"";
		while ((y = strstr(y, rv->s)) != NULL)
		{	rv->val++;
			y++;
		}
		break;
	case VAL:
		rv->val = ~(rv->val);
		break;
	default:
		assert(ix >= 0 && ix < Ncore);
	  	printf("bad arg to ~, saw: ");
		tok2txt(q->rgt, stdout);
		printf("\n");
		sep[ix].T_stop++;
		rv->rtyp = STP;
		break;
	}
}

static void
match_at_start(Prim **ref_p, Lextok *q, Rtype *rv, const int ix, Prim *p)
{
	if (q->rgt->typ == NAME)
	{	rv->val = 0;
		if (strncmp(p?p->txt:"", q->rgt->s, strlen(q->rgt->s)) == 0)
		{	rv->val = 1;
		}
		return;
	}
	eval_prog(ref_p, q->rgt, rv, ix);
	if (rv->rtyp == STR)
	{	rv->rtyp = VAL;
		rv->val  = 0;
		if (strncmp(p?p->txt:"", rv->s, strlen(rv->s)) == 0)
		{	rv->val = 1;
		}
	} else
	{	rv->rtyp = STP;
	  	printf("bad arg to ^, saw: ");
		tok2txt(q->rgt, stdout);
		printf("\n");
		assert(ix >= 0 && ix < Ncore);
		sep[ix].T_stop++;
	}
}

static void
do_and(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)
{	Rtype tmp;
	memset(&tmp, 0, sizeof(Rtype));

	assert(ix >= 0 && ix < Ncore);
	eval_prog(ref_p, q->lft, &tmp, ix);
	Assert("lhs &&", tmp.rtyp == VAL, q->lft);
	if (!tmp.val)
	{	rv->val = 0;	// shortcut
		return;
	}
	eval_prog(ref_p, q->rgt, rv, ix);
	if (rv->rtyp == PTR
	&&  rv->ptr != NULL
	&&  q->rgt->typ == '[')
	{	rv->val = rv->ptr->mark;
		rv->rtyp = VAL;
	}
	Assert("rhs &&", rv->rtyp == VAL, q->rgt);
}

static void
do_or(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)
{	Rtype tmp;
	memset(&tmp, 0, sizeof(Rtype));

	assert(ix >= 0 && ix < Ncore);
	eval_prog(ref_p, q->lft, &tmp, ix);
	Assert("OR1", tmp.rtyp == VAL, q->lft);
	if (tmp.val)
	{	rv->val = 1;	// shortcut
		return;
	}
	eval_prog(ref_p, q->rgt, rv, ix);
	Assert("OR2", rv->rtyp == VAL, q->rgt);
}

#define LHS	q->lft
#define RHS	q->rgt

static void
do_incr_decr(Prim **ref_p, Lextok *q, Rtype *rv, const int ix, Prim *p)
{	Rtype tmp;
	memset(&tmp, 0, sizeof(Rtype));

	assert(ix >= 0 && ix < Ncore);
	// LHS must store a value, like .mark or q.mark
	if (LHS->typ == '[')
	{	Assert("++/--", LHS->lft && (LHS->lft->typ == NAME), LHS->lft);

		tmp.s = derive_string(ref_p, LHS->rgt, ix, 0);
		tmp.rtyp = STR;

		if (LHS->core)
		{	fprintf(stderr, "error: invalid incr/decr of %s[%s] ^ expr\n",
				LHS->lft->s, tmp.s);
			rv->rtyp = STP;
			show_error(stderr, q->lnr);
			return;
		}
		if (!incr_aname_el(ref_p, LHS->lft, &tmp, q->typ, rv, ix))	// do_incr LHS->lft->s: array name
		{	fprintf(stderr, "error: unexpected array type: expected value\n");
			show_error(stderr, q->lnr);
			rv->rtyp = STP;
		}
		return;
	}

	if (LHS->typ != '.' || !LHS->rgt || LHS->rgt->typ != MARK || RHS)
	{	if (LHS->typ == NAME)
		{	Var_nm *n;
			n = get_var(ref_p, LHS, rv, ix);
			if (n->rtyp == 0)	// first use
			{	n->rtyp = VAL;
			}
			if (n->rtyp == STR && isdigit((uchar) n->s[0]))
			{	n->rtyp = VAL;
				n->v = atoi(n->s);
			}
			if (n->rtyp == VAL)
			{	if (q->typ == INCR)
				{	rv->val = n->v + 1;
				} else if (q->typ == DECR)
				{	rv->val = n->v - 1;
				}
				n->v = rv->val;
				return;
			}
			fprintf(stderr, "line %d: error: invalid use of ++ or -- with lhs: ",
				LHS->lnr);
			tok2txt(LHS, stdout);
			fprintf(stderr, "	unexpected type: ");
			what_type(stderr, n->rtyp);
			fprintf(stderr, "\n");
		} else
		{	fprintf(stderr, "line %d: error: invalid use of ++ or -- with lhs: ",
				LHS->lnr);
			tok2txt(LHS, stderr);
		}
		rv->rtyp = STP;
		return;
	}

	eval_prog(ref_p, LHS, rv, ix);
	if (!rv->rtyp)
	{	rv->rtyp = VAL;
		rv->val  = 0;
	}
	Assert("INCR/DECR", rv->rtyp == VAL, LHS);

	if (q->typ == INCR)
	{	rv->val++;
	} else if (q->typ == DECR)
	{	rv->val--;
	}

	if (LHS->lft)	// q.mark++ or q.mark--
	{	Var_nm *n = get_var(ref_p, LHS->lft, rv, ix);

		if (!n)
		{	rv->rtyp = STP;
			return;
		}

		if (!n->rtyp)
		{	rv->rtyp = n->rtyp = PTR; // from context
		}

		if (rv->rtyp == VAL)	// stores a value
		{	n->v = rv->val;	// XX was missing before
			return;		// XX was missing before
		}

		if (rv->rtyp == PTR)
		{	Prim *z = n->pm;
			if (z)
			{	z->mark = rv->val;
				return;
			}
			fprintf(stderr, "line %d: error: incr/decr of invalid ref '%s.%s'\n",
				LHS->lnr, LHS->lft->s, LHS->rgt->s);
		} else
		{	fprintf(stderr, "line %d: error: unexpected type '", LHS->lnr);
			what_type(stderr, rv->rtyp);
			fprintf(stderr, "' in incr/decr '%s.%s'\n",
				LHS->lft->s, LHS->rgt->s);
		}
		rv->rtyp = STP;
		sep[ix].T_stop++;
		return;
	}

	if (p)	// .mark++ or .mark--
	{	p->mark = rv->val;
	}
}


static void
do_assignment(Prim **ref_p, Lextok *q, Rtype *rv, const int ix, Prim *p)
{	Rtype tmp;
	Var_nm dummy;
	memset(&tmp, 0, sizeof(Rtype));

	// restrict lhs to [q].mark = ... or . = ... or name = name:
	// LHS can be .mark or . or name
	// RHS can be .nxt, .prv, .bound, .jmp
	assert(ix >= 0 && ix < Ncore);

	if (!RHS)
	{	fprintf(stderr, "line %d: error: missing rhs in assignment\n", q->lnr);
		rv->rtyp = STP;
		return;
	}
	if (!LHS)
	{	fprintf(stderr, "line %d: error: missing lhs in assignment", q->lnr);
		rv->rtyp = STP;
		return;
	}

	if (LHS->typ == '[')
	{	Assert("set", LHS->lft && (LHS->lft->typ == NAME), LHS->lft);
		assert(RHS);
		eval_prog(ref_p, RHS, rv, ix);
		tmp.s = derive_string(ref_p, LHS->rgt, ix, 0);
		tmp.rtyp = STR;

		if (LHS->core)
		{	fprintf(stderr, "error: invalid assignment to %s[%s] ^ expr\n",
				LHS->lft->s, tmp.s);
			rv->rtyp = STP;
			show_error(stderr, q->lnr);
			return;
		}

		// lft->s: basename
		// tmp: index
		// rv: value, string, or ptr to store
		set_aname(ref_p, LHS->lft, &tmp, rv, ix);
		return;
	}

	if (LHS->typ == NAME)		// q = ... lhs
	{	eval_prog(ref_p, RHS, rv, ix);
		set_var(LHS, rv, ix);
		return;
	}

	if (LHS->typ != '.' || (LHS->lft && !LHS->rgt))
	{	printf("%d: bad lhs in assignment, expecting [name].[mark], saw: ",
			q->lnr);
		tok2txt(LHS, stdout);
		rv->rtyp = STP;
		return;
	}

	if (LHS->rgt)	// .mark = ... , q.mark = ..., .txt = ..., q.txt = ...
	{
		if (LHS->rgt->typ == TXT
		||  LHS->rgt->typ == TYP
		||  LHS->rgt->typ == FNM)
		{	eval_prog(ref_p, RHS, rv, ix);
			Assert("rhs = (a)", rv->rtyp == STR, RHS);
			if (LHS->lft)	// q.mark = ...
			{	Var_nm *n;

				switch (LHS->lft->typ) {
				case END:
				case BEGIN:
				case FIRST_T:
				case LAST_T:
					eval_prog(ref_p, LHS->lft, rv, ix);
					Assert("special", rv->rtyp == PTR, LHS->lft);
					n = &dummy;
					n->pm = rv->ptr;
					break;
				default:
					n = get_var(ref_p, LHS->lft, rv, ix);
					break;
				}

				if (rv->rtyp == 0)	// uninitialized
				{	rv->rtyp = PTR;
				}
				if (rv->rtyp == PTR)
				{	switch (LHS->rgt->typ) {
					case TXT:
						n->pm->txt = rv->s;
						n->pm->len = strlen(rv->s);
						break;
					case TYP:
						n->pm->typ = rv->s;
						break;
					case FNM:
						n->pm->fnm = rv->s;
						break;
					}
					return;
				}
				fprintf(stderr, "line %d: error: unexpected type '", LHS->lnr);
				what_type(stderr, rv->rtyp);
				fprintf(stderr, "' (%d) in assignment to '%s.%s'\n",
					rv->rtyp, LHS->lft->s, LHS->rgt->s);
				show_error(stderr, LHS->lnr);
			} else if (p)
			{	switch (LHS->rgt->typ) {
				case TXT:
					p->txt = rv->s;
					p->len = strlen(rv->s);
					break;
				case TYP:
					p->typ = rv->s;
					break;
				case FNM:
					p->fnm = rv->s;
					break;
			}	}
			return;
 		}

		if (LHS->rgt->typ == BOUND
		||  LHS->rgt->typ == PRV
		||  LHS->rgt->typ == NXT
		||  LHS->rgt->typ == JMP)
		{	Var_nm *n;
			eval_prog(ref_p, RHS, rv, ix);
			if (rv->rtyp != PTR)
			{	fprintf(stderr, "line %d: bad rhs in asgn (type %d), saw ", RHS->lnr, rv->rtyp);
				tok2txt(RHS, stderr);
				rv->rtyp = STP;
				show_error(stderr, RHS->lnr);
				return;
			}
			if (!LHS->lft)	// eg: .bound = ptr;
			{	n = &dummy;
				n->pm = p;
			} else
			{	n = get_var(ref_p, LHS->lft, &tmp, ix);
				if (!n->rtyp)			// new variable
				{	n->rtyp = PTR;		// its the q in q.mark
				}
				if (n->rtyp != PTR || !n->pm)
				{	fprintf(stderr, "line %d: error: type error in assignment to %s '",
						LHS->lnr, n->nm);
					what_type(stderr, n->rtyp);
					fprintf(stderr, "' in '%s.%s'\n", LHS->lft->s, LHS->rgt->s);
					rv->rtyp = STP;
					return;
			}	}
			switch (LHS->rgt->typ) {
			case BOUND:
				n->pm->bound = rv->ptr;
				break;
			case PRV:
				n->pm->prv = rv->ptr;
				break;
			case NXT:
				n->pm->nxt = rv->ptr;
				break;
			case JMP:
				n->pm->jmp = rv->ptr;
				break;
			}
			return;
		}

		if (LHS->rgt->typ != MARK
		&&  LHS->rgt->typ != LEN
		&&  LHS->rgt->typ != ROUND
		&&  LHS->rgt->typ != CURLY
		&&  LHS->rgt->typ != BRACKET
		&&  LHS->rgt->typ != LNR
		&&  LHS->rgt->typ != SEQ)
		{	fprintf(stderr, "line %d: error: bad lhs in asgn, saw: .",LHS->lnr);
			tok2txt(LHS->rgt, stderr);
			show_error(stderr, LHS->lnr);
			rv->rtyp = STP;
			return;
		}

		eval_prog(ref_p, RHS, rv, ix);
		if (rv->rtyp == STR && isdigit((uchar) rv->s[0]))
		{	rv->rtyp = VAL;
			rv->val = atoi(rv->s);
		} else if (rv->rtyp != VAL)
		{	printf("token: '%s'\n", rv->s);
		}
		Assert("rhs = (2)", rv->rtyp == VAL, RHS);
// rhs is VAL
		if (LHS->lft)	// get the q in q.mark = ...
		{	Var_nm *n = get_var(ref_p, LHS->lft, &tmp, ix);

			if (!n->rtyp)			// new variable
			{	n->rtyp = PTR;		// its the q in q.mark
			}
			if (!n->pm)
			{	n->pm = (Prim *) hmalloc(sizeof(Prim), ix, 135);	// do_assignment
				n->pm->fnm = n->pm->txt = n->pm->typ = "";
				n->pm->len = 0;
				n->pm->seq = -1;
			}

			if (n->rtyp == PTR)
			{	switch (LHS->rgt->typ) {
				case MARK:
					n->pm->mark = rv->val;
					break;
				case SEQ:
					n->pm->seq = rv->val;
					break;
				case LEN:
					n->pm->len = rv->val;
					break;
				case ROUND:
					n->pm->round = rv->val;
					break;
				case CURLY:
					n->pm->curly = rv->val;
					break;
				case BRACKET:
					n->pm->bracket = rv->val;
					break;
				case LNR:
					n->pm->lnr = rv->val;
					break;
				}
				return;
			}

			if (n->rtyp != VAL)
			{	fprintf(stderr, "line %d: error: type error in assignment to %s '",
					LHS->lnr, n->nm);
				what_type(stderr, tmp.rtyp);
				fprintf(stderr, "' in '%s.%s'\n", LHS->lft->s, LHS->rgt->s);
				rv->rtyp = STP;
				return;
			}
			switch (n->rtyp) {
			case VAL:
				n->v = rv->val;
				// fprintf(stderr, "%d: when can this happen? %s.%s\n",
				//	LHS->lnr, LHS->lft->s, LHS->rgt->s);
				break;
			case STP:
			case PRCD:
				break;
			case STR:
			case PTR:	// cant happen, intercepted above
			default:
				fprintf(stderr, "cannot happen, unknown type in assignment\n");
				rv->rtyp = STP;
				break;
			}
		} else if (p)	// .mark = ...
		{	switch (LHS->rgt->typ) {
			case MARK:
				p->mark = rv->val;
				break;
			case LNR:
				p->lnr = rv->val;
				break;
			case SEQ:
				p->seq = rv->val;
				break;
			case LEN:
				p->len = rv->val;
				break;
			case ROUND:
				p->round = rv->val;
				break;
			case CURLY:
				p->curly = rv->val;
				break;
			case BRACKET:
				p->bracket = rv->val;
				break;
			default:
				printf("cannot happen -- unknown typ in assign\n");
				break;
		}	}
		return;
	}

	// last case: !LHS->lft && !LHS->rgt, eg: . = ...

	eval_prog(ref_p, RHS, rv, ix);
	Assert("rhs = (3)", rv->rtyp == PTR, RHS);

	if (!(*ref_p) || !rv->ptr)
	{	if (p == tokrange[ix]->upto
		&&  RHS->typ == '.'
		&&  RHS->rgt->typ == NXT)
		{	// stay on last token
			return;
		}
		if (p == tokrange[ix]->from
		&&  RHS->typ == '.'
		&&  RHS->rgt->typ == PRV)
		{	// stay on first token
			return;
		}
		sep[ix].T_stop++;
		rv->rtyp = STP;
	} else
	{	*ref_p = rv->ptr;
	}
}

#undef LHS
#undef RHS

static void
do_dot(Prim **ref_p, Lextok *q, Rtype *rv, const int ix, Prim *p)
{
	if (!q->lft && !q->rgt)
	{	rv->rtyp = PTR;
		rv->ptr = p;
	} else if (!q->lft)
	{	eval_prog(ref_p, q->rgt, rv, ix);
	} else // q->lft	e.g.:	q.txt
	{	Var_nm *n, dummy;

		assert(ix >= 0 && ix < Ncore);
		switch (q->lft->typ) {
		case END:
		case BEGIN:
		case FIRST_T:
		case LAST_T:
			eval_prog(ref_p, q->lft, rv, ix);
			Assert("dodot", rv->rtyp == PTR, q->lft);
			n = &dummy;
			n->pm = rv->ptr;
			break;
		default:
			n = get_var(ref_p, q->lft, rv, ix);
			break;
		}
		if (!n)
		{	return;
		}
		switch (rv->rtyp) {
		case VAL:
			rv->val = n->v;
			break;
		case STR:
			rv->s = (char *) n->s;
			break;
		case PTR:
			rv->ptr = n->pm;
			if (rv->ptr)
			{	Prim *z = n->pm;
				eval_prog(&z, q->rgt, rv, ix);
				break;
			}
			rv->val  = 0;	// assume uninitialized ptr ref
			rv->rtyp = VAL;
			break;
		default:	// STP oor PRCD
			break;
	}	}
}

static void
doindent(void)
{	int i;
	for (i = 0; i < nest; i++)
	{	printf("  ");
	}
}

static char *nr_tbl[] = {
	"0", "1", "2", "3", "4",
	"5", "6", "7", "8", "9"
};

static void
convert2string(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)
{	int aw = 64;

	assert(q->typ != ',');

	eval_prog(ref_p, q, rv, ix);

	switch (rv->rtyp) {
	case STR:
		return;
	case PTR:
		rv->val = rv->ptr->seq;	// was ->mark before
		// fall thru
	case VAL:
		if (rv->val >= 0 && rv->val <= 9)
		{	rv->s = nr_tbl[rv->val];
			break;
		}
		if (rv->val >= 0)
		{	if (rv->val < 1000)
			{	aw = 4;
			} else if (rv->val < 10000000)
			{	aw = 8;
		}	}
		rv->s = (char *) hmalloc(aw, ix, 136);	// convert2string
		snprintf(rv->s, aw, "%d", rv->val);	// convert2string
		break;
	default:	// STP or PRCD
		fprintf(stderr, "line %d: error: unexpected type of index (%d)\n",
			q->lnr, rv->rtyp);
		assert(ix >= 0 && ix < Ncore);
		sep[ix].T_stop++;
		rv->s = "?";
		break;
	}
	rv->rtyp = STR;
}

char *
derive_string(Prim **ref_p, Lextok *q, const int ix, const char *usethis) // q: comma separated list
{	Rtype tmp;
	char *s, *t, *r;
	int   n;

	memset(&tmp, 0, sizeof(Rtype));

	if (q->typ != ',')	// leaf
	{	convert2string(ref_p, q, &tmp, ix);
		if (usethis && strlen(tmp.s) < SZ_STATS)
		{	r = (char *) usethis;
		} else
		{	if (tmp.rtyp == VAL)
			{	r = tmp.s;	// V4.4: tmp.s was newly allocated
			} else
			{	r = (char *) hmalloc(strlen(tmp.s) + 1, ix, 137);	// derive_string 1
		}	}
		strcpy(r, tmp.s);
	} else
	{	s = derive_string(ref_p, q->rgt, ix, 0);
		t = derive_string(ref_p, q->lft, ix, 0);
		if (strlen(t) == 0) { printf("%s:%d: OH NO!!, unexpected error\n", (*ref_p)->fnm, (*ref_p)->lnr); }
		n = strlen(t)+strlen(s)+2;
		r = (char *) hmalloc(n, ix, 138);			// derive_string 2
		snprintf(r, n, "%s,%s", t, s);
	}
	return r;
}

static void
cpush(Prim **ref_p, const char *s, Lextok *formals, Lextok *actuals, Lextok *ra, const int ix)
{	Cstack *cframe;
	static int warned = 0;

	assert(ix >= 0 && ix < Ncore);
	for (cframe = cstack[ix]; cframe; cframe = cframe->nxt)
	{	if (strcmp(cframe->nm, s) == 0)
		{	if (!warned && sep[ix].Verbose>1)
			{	printf("line %d: warning: recursive call of %s()\n",
					actuals?actuals->lnr:0, s);
				warned = 1;
			}
			break;
	}	}
	if (Cdepth[ix] >= MAX_STACK)	// protect against infinite recursion
	{	fprintf(stderr, "error: max stackdepth of %d exceeded\n", MAX_STACK);
		sep[ix].T_stop++;
		return;
	}

	if (c_free[ix])
	{	cframe = c_free[ix];
		c_free[ix] = c_free[ix]->nxt;
	} else
	{	cframe = (Cstack *) hmalloc(sizeof(Cstack), ix, 139);
	}
	cframe->nm = s;
	cframe->ra = ra;
	cframe->formals = formals;
	cframe->actuals = actuals;
	cframe->nxt = cstack[ix];
	cstack[ix] = cframe;
	Cdepth[ix]++;	// fct call

	set_actuals(ref_p, s, formals, actuals, ix);
}

static Lextok *
cpop(const char *s, const int ix)
{	Cstack *c;

	assert(ix >= 0 && ix < Ncore);
	if (cstack[ix] && (!s || strcmp(cstack[ix]->nm, s) == 0))
	{	c = cstack[ix];
		cstack[ix] = cstack[ix]->nxt;
		c->nxt = c_free[ix];
		c_free[ix] = c;
		Cdepth[ix]--;		// fct return
		rm_aname("", 0, ix);	// all non-array vars now out of scope
		return c->ra;
	}
	fprintf(stderr, "error(%d): cannot happen: POP %s fails\n", ix, s?s:"");
	return 0;
}

static void
unwind_stack(const int ix)
{
	assert(ix >= 0 && ix < Ncore);
	while (cstack[ix])
	{	(void) cpop(0, ix);
	}
}

static int
ref_at_type(Prim *p, const char *s)
{
	if (strcmp(s, "no") == 0)
	{	return inverse;
	}
	if (strcmp(s, "ir") == 0)
	{	return inside_range;
	}
	if (strcmp(s, "and") == 0)
	{	return and_mode;
	}
	if (strcmp(s, "top") == 0)
	{	return top_only;
	}
	if (strcmp(s, "up") == 0)
	{	return top_up;
	}
	if (!p)
	{	return 0;
	}
	if (strcmp(s, "const") == 0)
	{	return (strncmp(p->typ, "const", 5) == 0);
	}
	return (strcmp(p->typ, s) == 0);
}

static int
prog_regstart(const char *s)
{	REX *r;
	int n;

	if (re_free)
	{	r = re_free;
		re_free = r->nxt;
	} else
	{	r = (REX *) emalloc(sizeof(REX), 82);
		r->rexpr = (regex_t *) emalloc(sizeof(regex_t), 82);
	}

	n = regcomp(r->rexpr, s, REG_NOSUB|REG_EXTENDED);
	if (n != 0)	// compile
	{	char ebuf[64];
		regerror(n, r->rexpr, ebuf, sizeof(ebuf));
		printf("%s\n", ebuf);
		return 0;
	}
	r->s = (char *) emalloc(strlen(s)+1, 82);
	strcpy(r->s, s);
	r->nxt = re_lst;
	re_lst = r;
	return 1;
}

static int
prog_regmatch(const char *q, const char *s)
{	REX *t;

	for (t = re_lst; t; t = t->nxt)
	{	if (strcmp(t->s, s) == 0)
		{	break;
	}	}
	if (!t)
	{	if (!prog_regstart(s))
		{	return 0;
		}
		t = re_lst;
	}
	return (regexec(t->rexpr, q, 0,0,0) == 0);
}
#if 0
static void
prog_regstop(void)
{	REX *t;

	for (t = re_lst; t; t = t->nxt)
	{	regfree(t->rexpr);
	}
	re_free = re_lst;
	re_lst  = 0;
}
#endif

// End RegEx

static char *
set_pre(Prim **ref_p, Lextok *t, int ix, int txt)
{	Prim *w = *ref_p;
	Rtype tmp;
	char *s;

	memset(&tmp, 0, sizeof(Rtype));

	assert(t->rgt && t->rgt->typ == STRING);
	if (!t->lft)
	{	if (!w)
		{	return "";
		}
		return txt?w->txt:w->typ;
	}
	eval_prog(ref_p, t->lft, &tmp, ix);
	assert(tmp.rtyp == STR);
	s = (char *) hmalloc(strlen(tmp.s)+1, ix, 140);	// set_pre
	strcpy(s, tmp.s);
	return s;
}


static int
re_matches(Prim **ref_p, Lextok *t, int cid)
{	char *v;
	char *s = t->rgt->s;
	int isre = 0;
	int istxt = 1;
	int rv;


	if (*s == '\\')
	{	s++;
	} else if (*s == '/')
	{	s++;
		isre++;
	} else if (*s == '@')
	{	s++;
		istxt = 0;
	}

	do_lock(cid);

	v = set_pre(ref_p, t, cid, istxt);
	if (isre)
	{	rv = prog_regmatch(v, s);
	} else
	{	if (!istxt
		&&  strcmp(v, "const") == 0)
		{	rv = (strncmp(v, "const", 5) == 0);
		} else
		{	rv = (strcmp(v, s) == 0);
	}	}

	do_unlock(cid);

	return rv;
}

static int
sum_var(const char *s)
{	Var_nm *n;
	int i, j, sum = 0;
	ulong h2 = hash2(s);

	// sum a variable's value across all cores

	for (i = 0; i < Ncore; i++)
	{	for (n = v_names[i]; n; n = n->nxt)	// mk_var
		{	if (n->h2 > h2)
			{	break;
			}
			if (n->h2 == h2
			&&  strcmp(n->nm, s) == 0)
			{	j = 0;
				switch (n->rtyp) {
				case VAL:
					j = n->v;
					break;
				case STR:
					if (isdigit((uchar) n->s[0]))
					{	j = atoi(n->s);
					} else if (strlen(n->s) > 0)
					{	j = 1;
					}
					break;
				case PTR:
					if (n->pm && n->pm->lnr > 0)
					{	j = 1;
					}
					break;
				default:
					break;
				}
				sum += j;
	}	}	}

	return sum;
}

static int
do_sum(Prim **ref_p, Lextok *q, Rtype *rv, int ix)
{	char *s;

	// q->lft is array name
	// q->rgt is array index, if any
	// or q->typ == NAME

	rv->rtyp = VAL;

	if (q->typ == NAME)
	{	return sum_var(q->s);
	}

	if (q->typ == '[')
	{	s = derive_string(ref_p, q->rgt, ix, 0);
		return array_sum_el(q->lft->s, s);
	}

	fprintf(stderr, "%d: bad arg for sum, saw: ", q->lnr);
	tok2txt(q, stderr);
	show_error(stderr, q->lnr);
	rv->rtyp = STP;

	return 0;
}

// static Prim *zz;	// debugging

static int
save_int(int a, char *b, Rtype *rv, const int ix)
{
	if (strcmp(b, "|") == 0
	||  strcmp(b, "&") == 0
	||  strcmp(b, "^") == 0
	||  strlen(b) == 0)
	{	global_n = a;
		global_t = b;
		save_range((void *) &ix);
		return tokrange[ix]->param;
	}
	rv->rtyp = STP;
	printf("usage: save(n, \"\" or \"|\" or \"&\" or \"^\"\n");
	return 0;
}

static int
restore_int(int a, char *b, Rtype *rv, const int ix)
{	char as[32];

	sprintf(as, "%d", a);
	if (strcmp(b, "|") == 0
	||  strcmp(b, "&") == 0
	||  strcmp(b, "^") == 0
	||  strlen(b) == 0)
	{	global_n = a;
		global_t = b;
		restore_range((void *) &ix);
		return tokrange[ix]->param;
	}
	rv->rtyp = STP;
	printf("usage: restore(n, \"\" or \"|\" or \"&\" or \"^\"\n");
	return 0;
}

void
fcts_int(const int ix)
{	FList *f;
	Prim *z;

	if (!flist)
	{	flist = (FList **) emalloc(Ncore * sizeof(FList *), 83);
		fct_defs_range((void *) &ix);
	}

	for (f = flist[ix]; f; f = f->nxt)
	{	z = f->p;	// fct name
		if (z->jmp)
		{	z = z->jmp;
			if (z->prv)
			{	z->prv->mark++;
			} else
			{	z->mark++;
			}
		} else
		{	z->mark++;
	}	}
}

void
reset_int(const int ix)
{
	(void) clear_range((void *) &ix);
}

static void
new_scalar(char *s, int ix)
{	Var_nm *g = mk_var(s, 0, ix);

	if (g)
	{	if (verbose)
		{	printf("global scalar %s\n", s);
		}
		g->cdepth = 0;
	} else
	{	fprintf(stderr, "error: global decl of %s failed, cpu %d\n", s, ix);
	}
}

static void
new_global(Lextok *q)
{	int i;
	if (q->typ != NAME)
	{	printf("new_global: unexpected type %d\n", q->typ);
		return;
	}
	for (i = 0; i < Ncore; i++)
	{	if (q->rgt && q->rgt->typ == '[')
		{	new_array(q->s, i);
		} else
		{	new_scalar(q->s, i);
	}	}
}

static void
handle_global(Lextok *t)
{
	if (!t)
	{	return;
	}

	handle_global(t->lft);

	if (t->typ == ',')
	{	new_global(t->rgt);
	} else if (t->typ == NAME)
	{	new_global(t);
	}
}

static void
bad_ref(const Var_nm *n, const Rtype *rv, const Lextok *from)
{
	if (!n)
	{	fprintf(stderr, "error: '%s' not found\n", from->s);
	} else if (rv->rtyp != PTR)
	{	fprintf(stderr, "error: '%s' bad token ref\n", from->s);
	}
}

static Prim *
handle_arg(Prim **ref_p, Lextok *from, Rtype *rv, const int ix)
{	Var_nm *n;

	if (!from)
	{	fprintf(stderr, "error: add/del_pattern (internal error)\n");
		return NULL;
	}

	if (from->typ == '.')
	{	return *ref_p;
	}

	if (from->typ != NAME)
	{	fprintf(stderr, "error: bad add/del_pattern command\n");
		return NULL;
	}

	n = get_var(ref_p, from, rv, ix);
	if (!n || rv->rtyp != PTR || !n->pm)
	{	bad_ref(n, rv, from);
		return NULL;
	}

	return n->pm;
}

void
eval_prog(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)
{	Prim  *p;
	Rtype tmp;

	memset(&tmp, 0, sizeof(Rtype));
	assert(ix >= 0 && ix < Ncore);
	sep[ix].Nest++;
next:
	if (!q || sep[ix].T_stop)
	{	if (sep[ix].P_debug == 2)
		{	nest = sep[ix].Nest;
			doindent();
			printf("--%s\n", !q?"path ends":"stopped");
		}
		sep[ix].Nest--;
		if (!rv->rtyp)
		{	rv->rtyp = VAL;
			rv->val  = 0;
		}
		return;		// the only return from the function
	}

	p        = *ref_p;
	rv->rtyp = VAL;		// default type
	rv->val  = 0;		// default value
	p_lnr    = q->lnr;

	if (sep[ix].Verbose > 2)
	{	lock_print(ix);
		printf("%d: [seq %d ln %d] EVAL_PROG typ %3d ",
			ix, p->seq, p->lnr, q->typ);
		tok2txt(q, stdout);
		fflush(stdout);
		unlock_print(ix);
	}
	if (sep[ix].P_debug == 2)
	{	doindent();
		printf("%d | %d | %d ::%d:'%s:%d: %s:%s'\t",
			q->tag,
			q->a?q->a->tag:0,
			q->b?q->b->tag:0,
			p->seq, p->fnm, p->lnr,
			p->typ, p->txt);
		tok2txt(q, stdout);
	}
	switch (q->typ) {
	case     NR: rv->val = q->val; break;
	case   TRUE: rv->val = 1; break;
	case  FALSE: rv->val = 0; break;
	case    '#': rv->val = !strcmp(p?p->txt:"", q->rgt->s); break;
	case    '@': rv->val = ref_at_type(p, q->rgt->s); break; 
	case    CPU: rv->val = ix; break;
	case N_CORE: rv->val = Ncore; break;

	case   BEGIN: rv->ptr = tokrange[ix]->from; rv->rtyp = PTR; break;
	case     END: rv->ptr = tokrange[ix]->upto; rv->rtyp = PTR; break;
	case FIRST_T: rv->ptr = prim; rv->rtyp = PTR; break;
	case  LAST_T: rv->ptr = plst; rv->rtyp = PTR; break;

	case MARKS:
		eval_prog(ref_p, q->lft, rv, ix);
		Assert("marks", rv->rtyp == VAL, q->lft);
		rv->val = nr_marks_int(rv->val, ix);
		break;

	case  SIZE:
		Assert("size", q->rgt && q->rgt->typ == NAME, q->rgt);
		rv->val = array_sz(q->rgt->s, ix);
		break;

	case SUM:
		rv->val = do_sum(ref_p, q->rgt, rv, ix);
		break;

	case RETRIEVE:
		Assert("retrieve", q->rgt && q->rgt->typ == NAME, q->rgt);
		eval_prog(ref_p, q->lft, rv, ix);
		Assert("index2", rv->rtyp == VAL, q->lft);
		rv->rtyp = STR;
		rv->s = array_ix(q->rgt->s, rv->val, ix);
		break;

	case SET_RANGES:
		{	Prim *a, *b;
			eval_prog(ref_p, q->lft, rv, ix);	// from
			Assert("set_range1", rv->rtyp == PTR, q->lft);
			a = rv->ptr;
			eval_prog(ref_p, q->rgt, rv, ix);	// upto
			Assert("set_range2", rv->rtyp == PTR, q->rgt);
			b = rv->ptr;
			set_ranges(a, b, 10);
		}
		rv->rtyp = VAL;
		rv->val  = 0;
		break;

	case SUBSTR:
		substr(ref_p, q, rv, ix);
		break;

	case SPLIT:
		split(ref_p, q, rv, ix);
		break;

	case STRLEN:
		eval_prog(ref_p, q->lft, rv, ix);
		Assert("strlen", rv->rtyp == STR, q->lft);
		rv->rtyp = VAL;
		rv->val  = strlen(rv->s);
		break;

	case FCTS:
		fcts_int(ix);
		rv->rtyp = VAL;
		rv->val = 0;
		break;

	case RESET:
		reset_int(ix);
		rv->rtyp = VAL;
		rv->val = 0;
		break;

	case SAVE:	// eg: save(2, "|")
		{	int tv;
			eval_prog(ref_p, q->lft, rv, ix);
			Assert("save", rv->rtyp == VAL, q->lft);
			tv = rv->val;
			eval_prog(ref_p, q->rgt, rv, ix);
			Assert("save", rv->rtyp == STR, q->rgt);
			rv->rtyp = VAL;
			rv->val = save_int(tv, rv->s, rv, ix);
		}
		break;

	case RESTORE:
		{	int tv;
			eval_prog(ref_p, q->lft, rv, ix);
			Assert("restore", rv->rtyp == VAL, q->lft);
			tv = rv->val;
			eval_prog(ref_p, q->rgt, rv, ix);
			Assert("restore", rv->rtyp == STR, q->rgt);
			rv->rtyp = VAL;
			rv->val = restore_int(tv, rv->s, rv, ix);
		}
		break;

	case RE_MATCH:
		rv->rtyp = VAL;
		rv->val  = re_matches(ref_p, q, ix);
		break;

	case '[':
		if (q->rgt && q->rgt->typ == ',')
		{	rv->rtyp = STR;
			rv->s = derive_string(ref_p, q->rgt, ix, 0); // '['
		} else
		{	Assert(q->lft?q->lft->s:"?", q->lft && (q->lft->typ == NAME), q->lft);
			Assert("[", q->rgt, q);
			convert2string(ref_p, q->rgt, rv, ix); // index
		}
		eval_aname(ref_p, q, rv, ix); // '[' array name, index value
		break;

	case  OR: do_or( ref_p, q, rv, ix); break;
	case AND: do_and(ref_p, q, rv, ix); break;

	case  B_OR: binop(|); break;
	case B_AND: binop(&); break;

	case LSH: binop(<<); break;
	case RSH: binop(>>); break;

	case B_XOR: binop(^); break;

	case  GT: binop(>);  break;
	case  LT: binop(<);  break;
	case  GE: binop(>=); break;
	case  LE: binop(<=); break;
	case '+': plus(ref_p, q, rv, ix); break;
	case '-': binop(-); break;
	case '*': binop(*);  break;
	case '/': rbinop(/);  break;
	case '%': rbinop(%);  break;
	case UMIN: unop(-);  break;
	case '!':  unop(!);  break;

	case '~': match_anywhere(ref_p, q, rv, ix, p); break;
	case '^': match_at_start(ref_p, q, rv, ix, p); break;

	case   ROUND: rv->val = p?p->round:0; break;
	case BRACKET: rv->val = p?p->bracket:0; break;
	case   CURLY: rv->val = p?p->curly:0; break;
	case     LEN: rv->val = p?p->len:0; break;
	case	 LNR: rv->val = p?p->lnr:0; break;
	case    MARK: rv->val = p?p->mark:0; break;
	case     SEQ: rv->val = p?p->seq:0; break;

	case	RANGE:
		  if (p && (p->bound || p->jmp))
		  {	Prim *d = p->bound?p->bound:p->jmp;
			if (strcmp(d->fnm, p->fnm) == 0)
			{	rv->val = d->lnr - p->lnr;
				break;
		  }	}
		  rv->val = 0;
		  break;

	case EQ: eval_prog(ref_p, q->lft, &tmp, ix);
		 eval_prog(ref_p, q->rgt, rv, ix);
		 eval_eq(1, &tmp, rv);
		 break;

	case NE: eval_prog(ref_p, q->lft, &tmp, ix);
		 eval_prog(ref_p, q->rgt, rv, ix);
		 eval_eq(0, &tmp, rv);
		 break;

	case '.':
		 if (!q->lft && q->rgt)
		 {	q = q->rgt;
			goto next; // avoid recursion
		 }
		 do_dot(ref_p, q, rv, ix, p);
		 break;

	case PRINT:
		print_args(ref_p, q->lft, rv, ix);
		fflush(stdout);
		break;

	case NAME:
		{ Var_nm *n = get_var(ref_p, q, rv, ix);
		  switch (rv->rtyp) {
		  case VAL:
			rv->val = n->v;
			break;
		  case STR:
			rv->s = (char *) n->s;
			break;
		  case PTR:
			rv->ptr = n->pm;
			break;
		  default:
			break;
		  }
		}
		break;

	case STRING: rv->rtyp = STR; rv->s   = q->s; break;
	case    FCT: rv->rtyp = STR; rv->s   = fct_which(p); break;
	case    FNM: rv->rtyp = STR; rv->s   = p?p->fnm:"";  break;
	case    TYP: rv->rtyp = STR; rv->s   = p?p->typ:"";  break;
	case    TXT: rv->rtyp = STR; rv->s   = p?p->txt:"";  break;
	case    JMP: rv->rtyp = PTR; rv->ptr = p?p->jmp:p;  break;
	case  BOUND: rv->rtyp = PTR; rv->ptr = p?p->bound:p; break;
	case MBND_D: rv->rtyp = PTR; rv->ptr = p?p->mbnd[0]:p; break;
	case MBND_R: rv->rtyp = PTR; rv->ptr = p?p->mbnd[1]:p; break;
	case    NXT: rv->rtyp = PTR; rv->ptr = p?p->nxt:p;   break;
	case    PRV: rv->rtyp = PTR; rv->ptr = p?p->prv:p;   break;

	case INCR:
	case DECR: do_incr_decr(ref_p,  q, rv, ix, p); break;
	case  '=': do_assignment(ref_p, q, rv, ix, p); p = *ref_p; break;

	case IF:
	case IF2:
		eval_prog(ref_p, q->lft, rv, ix);
		if (rv->rtyp == STP)
		{	break;
		}
		Assert("if", rv->rtyp == VAL, q->lft);
		if (rv->val)		// then part
		{	if (sep[ix].P_debug == 2)
			{	doindent();
				printf("Then\n");
			}
			q = q->a;
			goto next;
		} else if (q->b)	// else part
		{	if (sep[ix].P_debug == 2)
			{	doindent();
				printf("Else\n");
			}
			q = q->b;
			goto next;
		} else			// no else
		{	rv->rtyp = PRCD;
			if (sep[ix].P_debug == 2)
			{	doindent();
				printf("Up\n");
		}	}
		break;

	case BREAK:
		if (!q->a)
		{	rv->rtyp = PRCD;
			if (sep[ix].P_debug == 2)
			{	doindent();
				printf("Up (break/return)\n");
		}	}
	case ';':
	case CONTINUE:
	case ELSE:
	case SKIP:
	case GOTO:
		break;

	case ADD_PATTERN:	// name q->lft, from q->rgt->lft, upto q->rgt->rgt
	case DEL_PATTERN:
		if (q->lft
		&&  q->rgt
		&&  q->rgt->lft
		&&  q->rgt->rgt)
		{	Prim *f, *u;

			f = handle_arg(ref_p, q->rgt->lft, rv, ix); // from
			u = handle_arg(ref_p, q->rgt->rgt, rv, ix); // upto

			if (f && u)
			{	if (verbose)
				{	printf("%s_pattern to set '%s' from %s:%d upto %s:%d\n",
						(q->typ == ADD_PATTERN) ? "add" : "del",
						q->lft->s, f->fnm, f->lnr, u->fnm, u->lnr);
				}
				if (q->typ == ADD_PATTERN)
				{	add_pattern(q->lft->s, 0, f, u, ix);
				} else
				{	del_pattern(q->lft->s, f, u, ix);
			}	}
			break;
		}
		// error case
		show_error(stderr, q->lnr);
		unwind_stack(ix);
		sep[ix].T_stop++; 
		rv->rtyp = STP;
		break;
	case CP_PSET:
		rv->ptr = cp_pset(q->lft->s, ix);
		if (!rv->ptr)
		{	show_error(stderr, q->lnr);
			unwind_stack(ix);
			sep[ix].T_stop++; 
			rv->rtyp = STP;
		} else
		{	rv->rtyp = PTR;
		}
		break;

	case ASSERT:
		eval_prog(ref_p, q->lft, rv, ix);
		Assert("assert", rv->rtyp == VAL, q->lft);
		if (rv->val == 0)
		{	fprintf(stderr, "%s:%d: assertion violated at: ",
				p->fnm, p->lnr);
			Prim *z = p;
			while (z->prv && z->lnr == z->prv->lnr) { z = z->prv; }
			while (z)
			{	fprintf(stderr, "%s", z->txt);
				z = z->nxt;
				if (!z || z->lnr != p->lnr)
				{	break;
			}	}
			fprintf(stderr, "\n");
#ifdef DEBUG
			dump_tree(q->lft, 0);
#endif
			if (0) fprintf(stderr, "%d: [seq %d ln %d] EVAL_PROG typ %3d ",
				ix, p->seq, p->lnr, q->typ);

			show_error(stderr, q->lnr);
			unwind_stack(ix);
			sep[ix].T_stop++; 
			rv->rtyp = STP;
			break;
		}
		break;

	case TERSE:
		rv->val = no_display;
		break;

	case VERBOSE:
		rv->val = sep[ix].Verbose;
		break;

	case ',':	// associate array index, list of exprs
			// replace with single string
		rv->rtyp = STR;
		rv->s = derive_string(ref_p, q, ix, 0);
		break;

	case NEXT_T: unwind_stack(ix); sep[ix].T_stop++; rv->rtyp = PRCD; break;
	case STOP:   unwind_stack(ix); sep[ix].T_stop++; rv->rtyp = STP;  break;

	case WHILE:		// replaced with IF2
		assert(0);	// not reachable
		break;
	case FUNCTION:	// function definition
		break;
	case CALL:	// function call, body in q->a, actuals in q->rgt
		if (!q->f)	// fct call in expr
		{	find_fct(q, ix);	// sets q->f and q->a, calls mk_fsm once
		}
#if 0
		assert(q->f);	// was set in mk_fsm
		assert(q->a);
#endif	
		if (!q->f || !q->a)
		{	rv->rtyp = STP;
		} else
		{	cpush(ref_p, q->f->nm->s, q->f->formal, q->rgt, q->c, ix);
			eval_prog(ref_p, q->a, rv, ix);
		}
		q = 0;
		break;

	case RETURN:
		if (q->lft)
		{	eval_prog(ref_p, q->lft, rv, ix);
		} else
		{	rv->rtyp = VAL;
			rv->val  = 0;
		}
		// q->a value was set in mk_fsm
		q->a = cpop(0, ix);
		break;

	case UNSET:
		rm_aname_el(ref_p, q, ix);
		break;

	case LOCK:
		do_lock(ix);
		break;
	case UNLOCK:
		do_unlock(ix);
		break;

	case A_UNIFY:
		array_unify(q, ix);
		break;

	case NEWTOK:
		rv->ptr = (Prim *) hmalloc(sizeof(Prim), ix, 141);		// newtok
		rv->ptr->seq = 0;
		rv->ptr->txt = rv->ptr->typ = rv->ptr->fnm = "";
		rv->ptr->len = 0;
		rv->rtyp = PTR;
		break;

	case DISAMBIGUATE:
		// q->rgt->s is of type NAME (a variable)
		// a variable that should be of type STR
		{ Var_nm *n = get_var(ref_p, q->rgt, rv, ix);
		  Assert("disambiguate", rv->rtyp == STR, q->rgt);
		  rv->s = disambiguate((char *) n->s, ix);
		}
		break;

	// list functions
	case TOP:
		//Assert("top", q->lft != NULL, q);
		rv->rtyp = PTR;
		rv->ptr = top(q->lft->s, ix);
		break;
	case BOT:
		//Assert("bot", q->lft != NULL, q);
		rv->rtyp = PTR;
		rv->ptr = bot(q->lft->s, ix);
		break;
	case OBTAIN_EL:
		rv->rtyp = PTR;
		rv->ptr = obtain_el(ix);
		break;
	case RELEASE_EL:
		eval_prog(ref_p, q->lft, rv, ix);
		//Assert("release_el", rv->rtyp == PTR, q->lft);
		release_el(rv->ptr, ix);
		rv->rtyp = VAL;
		rv->val = 0;
		break;
	case POP_TOP:
		//Assert("pop_top", q->lft != NULL, q);
		pop_top(q->lft->s, ix);
		rv->rtyp = VAL;
		rv->val = 0;
		break;
	case POP_BOT:
		//Assert("pop_bot", q->lft != NULL, q);
		pop_bot(q->lft->s, ix);
		rv->rtyp = VAL;
		rv->val = 0;
		break;
	case UNLIST:
		//Assert("unlist", q->lft != NULL, q);
		unlist(q->lft->s, ix);
		rv->rtyp = VAL;
		rv->val = 0;
		break;
	case LLENGTH:
		//Assert("unlist", q->lft != NULL, q);
		rv->rtyp = VAL;
		rv->val = llength(q->lft->s, ix);
		break;
	case ADD_TOP:
		eval_prog(ref_p, q->rgt, rv, ix);
		//Assert("add_top1", rv->rtyp == PTR, q->rgt);
		//Assert("add_top2", q->lft != NULL, q);
		add_top(q->lft->s, rv->ptr, ix);
		rv->rtyp = VAL;
		rv->val = 0;
		break;
	case ADD_BOT:
		eval_prog(ref_p, q->rgt, rv, ix);
		//Assert("add_bot1", rv->rtyp == PTR, q->rgt);
		//Assert("add_bot2", q->lft != NULL, q);
		add_bot(q->lft->s, rv->ptr, ix);
		rv->rtyp = VAL;
		rv->val = 0;
		break;
	// end list functions

	case GLOBAL:
		break;

	case FOR:
	default:
		printf("line %d: cannot happen: %d (%s) ", q->lnr, q->typ, q->s);
		tok2txt(q, stdout);
		rv->rtyp = STP;
		break;
	}

	if (rv->rtyp == STP
	||  rv->rtyp == PRCD
	|| !q)
	{	q = 0;
	} else
	{	switch (q->typ) {
		case ';':
		case '=':
		case ADD_PATTERN:
		case DEL_PATTERN:
		case ASSERT:
		case INCR:
		case DECR:
		case PRINT:
		case BREAK:
		case CONTINUE:
		case GLOBAL:
		case GOTO:
		case SKIP:
		case ELSE:
		case UNSET:
		case FUNCTION:
		case RETURN:
		case A_UNIFY:
		case LOCK:
		case UNLOCK:
		case FCTS:
		case SAVE:
		case RESET:

		case TOP:
		case BOT:
		case ADD_TOP:
		case ADD_BOT:
		case POP_TOP:
		case POP_BOT:
		case OBTAIN_EL:
		case RELEASE_EL:
		case UNLIST:
		case LLENGTH:

		case RESTORE:
			q = q->a;
			break;
		case CALL:
			break;
		default:
			q = 0;
			break;
		}
	}
	if (sep[ix].P_debug == 2)
	{	doindent();
		printf("--end eval_prog rv: '");
		what_type(stdout, rv->rtyp);
		printf("' val %d --> str: %s\n",
			rv->val, (rv->rtyp == STR)?rv->s:"");
	}
	goto next;
}

static Var_nm *
check_var(const char *s, const int ix)
{	static Var_nm dummy;
	Var_nm *n, *g = &dummy;
	ulong h2 = hash2(s);

	assert(ix >= 0 && ix < Ncore);
	for (n = v_names[ix]; n; n = n->nxt)	// check_var
	{	if (n->h2 > h2)
		{	break;
		}
		if (n->h2 == h2
		&&  strcmp(n->nm, s) == 0)
		{	if (n->cdepth == Cdepth[ix])
			{	return n;	// found
			}
			if (n->cdepth == 0)
			{	g = n;
	}	}	}

	return g;
}

static Var_nm *
mk_var(const char *s, const int t, const int ix)
{	Var_nm *n, *g = (Var_nm *) 0;
	Var_nm *lastn = NULL;
	ulong h2 = hash2(s);

	assert(ix >= 0 && ix < Ncore);
	for (n = v_names[ix]; n; lastn = n, n = n->nxt)	// mk_var
	{	if (n->h2 > h2)
		{	break;
		}
		if (n->h2 == h2
		&&  strcmp(n->nm, s) == 0)
		{	if (n->cdepth == Cdepth[ix])
			{	return n;	// found
			}
			if (n->cdepth == 0)
			{	g = n;
	}	}	}

	if (g)
	{	// printf("mk_var: global match %s depth %d\n", g->nm, g->cdepth);
		return g;	// global match
	}

	if (v_free[ix])
	{	n = v_free[ix];
		n->rtyp = n->v = 0;
		v_free[ix] = v_free[ix]->nxt;
		// n->pm may be pointing to a token from the actual
		// input sequence, which should not be reused
		if (!n->pm
		||   n->pm->seq != ISVAR)
		{	n->pm = (Prim *) hmalloc(sizeof(Prim), ix, 142);	// mk_var
		}
	} else
	{	n = (Var_nm *) hmalloc(sizeof(Var_nm), ix, 143);
		n->pm = (Prim *) hmalloc(sizeof(Prim), ix, 143);	// mk_var
	}

	n->pm->fnm = n->pm->txt = n->pm->typ = "";
	n->pm->len = 0;
	n->pm->seq = ISVAR;	// to identify origin
	n->nm = s;
	n->h2 = h2;
	n->rtyp = t;
	n->s  = "";
	n->cdepth = Cdepth[ix];
//	printf("mk_var: new %s depth %d\n", n->nm, n->cdepth);

	if (lastn)
	{	n->nxt = lastn->nxt;
		lastn->nxt = n;
	} else
	{	n->nxt = v_names[ix];
		v_names[ix] = n;
	}
	return n;
}

void
rm_var(const char *s, int one, const int ix)
{	Var_nm *n, *nxt, *lst = (Var_nm *) 0;
	ulong h2 = hash2(s);

	assert(ix >= 0 && ix < Ncore);
	for (n = v_names[ix]; n; n = nxt)		// rm_var
	{	nxt = n->nxt;
		if (one && n->h2 > h2)
		{	break;
		}
		if ((one && n->h2 == h2 && strcmp(n->nm, s) == 0
			 && n->cdepth == Cdepth[ix])
		||  (!one && n->cdepth > Cdepth[ix]))
		{	if (lst)
			{	lst->nxt = nxt;
			} else
			{	v_names[ix] = nxt;
			}
			n->nxt = v_free[ix];
			v_free[ix]  = n;
		} else
		{	lst = n;
	}	}
}

static void
ini_vars(void)
{	static int vmax = 0;
	int i;

	if (Ncore > vmax)
	{	vmax = Ncore;
		v_names = (Var_nm **) 0;
		cstack  = (Cstack **) 0;
		Cdepth  = (int *)     0;
		t_stop  = (int *)     0;
	}
		
	if (!v_names)
	{	v_names = (Var_nm **) emalloc(NCORE * sizeof(Var_nm *), 84);
		v_free  = (Var_nm **) emalloc(NCORE * sizeof(Var_nm *), 84);
	}

	if (!cstack)
	{	cstack = (Cstack **) emalloc(NCORE * sizeof(Cstack *), 84);
		c_free = (Cstack **) emalloc(NCORE * sizeof(Cstack *), 84);
	}

	if (!Cdepth)
	{	Cdepth = (int *) emalloc(NCORE * sizeof(int), 84);
	}

	if (!t_stop)
	{	t_stop = (int *) emalloc(NCORE * sizeof(int), 84);
	}
	if (!sep)
	{	sep = (Separate *) emalloc(NCORE * sizeof(Separate), 84);
		for (i = 0; i < NCORE; i++)
		{	sep[i].Nest    = nest;
			sep[i].T_stop  = t_stop[i];
	}	}
	// the following can change during a run
	for (i = 0; i < NCORE; i++)
	{	sep[i].Verbose = verbose;
		sep[i].P_debug = p_debug;
	}
	ini_arrays();
	ini_lists();
}

static void
mk_varpool(void)
{	Var_nm *n;
	int i, k;

	// avoid having to call emalloc in mk_var
	// during multi-core program runs

	for (i = 0; i < Ncore; i++)
	{	k = v_cnt;
		for (n = v_free[i]; n; n = n->nxt)
		{	k--;
		}
		while (k-- >= 0)
		{	n = (Var_nm *) emalloc(sizeof(Var_nm), 85); // mk_varpool
			n->pm = &none;
			none.seq = NONE;	// to know source
			n->nxt = v_free[i];
			v_free[i] = n;
		}
		prepopulate(a_cnt, i);
	}
}

static void
ini_blocks(void)
{	Block *b;
	int have_blocks, i;
	static int maxb = 0;

	if (!block || Ncore > maxb)
	{	block  = (Block **) emalloc(NCORE * sizeof(Block *), 86);
		b_free = (Block **) emalloc(NCORE * sizeof(Block *), 86);
		if (Ncore > maxb)
		{	maxb = Ncore;
		}
	} else
	{	for (i = 0; i < Ncore; i++)
		{	while ((b = pop_context(i, 5)) != NULL)	// cleanup
			{	bfree(b, i);
				printf("recover block %d\n", i); // shouldnt happen
	}	}	}


	for (i = 0; i < Ncore; i++)
	{	have_blocks = 0;
		for (b = b_free[i]; b; b = b->nxt)
		{	have_blocks++;
		}
		while (have_blocks++ < n_blocks + 2)
		{	b = (Block *) emalloc(sizeof(Block), 87);
			bfree(b, i);
	}	}
}

#ifdef DUP_TREES
static unsigned long loc_copy;	// avoid sharing access to parse tree between cores

static void
clear_dup(void)	// when p_tree changes
{
	loc_copy = 0;
}

static Lextok *
dup_tree(Lextok *p)
{	Lextok *n;

	if (!p)
	{	return NULL;
	}
	if (p->visit & 32)
	{	return p;
	}
	p->visit |= 32;

	n = (Lextok *) emalloc(sizeof(Lextok), 88);
	memcpy(n, p, sizeof(Lextok));

	if (p->s)
	{	n->s = (char *) emalloc(strlen(p->s)+1, 89);
		strcpy(n->s, p->s);
	}

	n->a = dup_tree(p->a);
	n->b = dup_tree(p->b);
	n->c = dup_tree(p->c);
	n->core = dup_tree(p->core);

	// leave n->f as is

	n->lft = dup_tree(p->lft);
	n->rgt = dup_tree(p->rgt);

	return n;
}

static void
clr_marks(Lextok *p)
{
	if (!p || p->visit == 0)
	{	return;
	}
	p->visit = 0;
	clr_marks(p->a);
	clr_marks(p->b);
	clr_marks(p->c);
	clr_marks(p->core);
	clr_marks(p->lft);
	clr_marks(p->rgt);
}
#endif

// externally visible functions:

int
exec_prog(Prim **q, int ix)
{	Rtype rv;

	assert(ix >= 0 && ix < Ncore);
	sep[ix].T_stop = 0;
	memset(&rv, 0, sizeof(Rtype));

#ifdef DUP_TREES
	// reduce cache misses
	if (Ncore > 1 && ix > 0 && !(loc_copy & (1<<ix)))
	{	Lextok *tmp;
		do_lock(ix);
		tmp = dup_tree(p_tree);
		clr_marks(p_tree);
		p_tree = tmp;
		loc_copy |= (1<<ix);
	//	printf("%d Dups\n", ix);
		do_unlock(ix);
	}
#endif
	eval_prog(q, p_tree, &rv, ix);

	if (sep[ix].P_debug == 2)
	{	printf("===%d\n", rv.rtyp);
	}

	switch (rv.rtyp) {
	case STP:
		return -1;
	case PRCD:
		return -2;
	default:
		break;
	}

	return rv.val;
}

static int
streamable(Lextok *t)
{
	if (!stream_override
	&&  t
	&&  !(t->visit & 4))
	{	t->visit |= 4;
		switch (t->typ) {
#if 0
		case FIRST_T:
		case LAST_T:
		case BEGIN:
		case END:
#endif
		case PRV:
		case JMP:
		case N_CORE:
			fprintf(stderr, "script contains .jmp or .prv and is therefore not streamable:\n");
			show_error(stderr, t->lnr);
			return 0;
		case CALL:
			if (!streamable(t->c))
			{	return 0;
			}
		default:
			if (!streamable(t->lft)
			||  !streamable(t->rgt))
			{	return 0;
	}	}	}

	return 1;
}

int
prep_prog(FILE *nfd)
{	static int uniq = 0;	// parse time only

	pfd = nfd;		// prog_fd
	n_blocks = 1;
	p_lnr = 1;
	v_cnt = 0;

	ini_vars();

	sep[0].T_stop = 0;
	if (!xxparse() || sep[0].T_stop)
	{	fprintf(stderr, "[%d]\n", sep[0].T_stop);
		show_error(stderr, p_lnr);
		return 0;
	}

	mk_varpool();

	none.fnm = none.typ = none.txt = "";
	ini_blocks();

	if (preserve || p_debug == 3)
	{	FILE *x = fopen(CobraDot, "a");
		if (!x)
		{	printf("cannot create '%s'\n", CobraDot);
		} else
		{	fprintf(x, "digraph X%d {\n", uniq++);
			draw_ast(p_tree, x);
			fprintf(x, "}\n");
			fclose(x);
			if (preserve)
			{	printf("wrote: %s\n", CobraDot);
			}
			if (p_debug == 3 && system(ShowDot) < 0)
			{	perror(ShowDot);
			}
			sleep(1);
	}	}

	p_tree = fix_while(p_tree);			// visit |= 1
#ifdef DUP_TREES
	clear_dup();
#endif
	mk_fsm(p_tree, 0);				// visit |= 2
	opt_fsm(p_tree);				// visit |= 8
//	dump_tree(p_tree, 0);

	if (stream == 1
	&& !streamable(p_tree))
	{	return 0;
	}

	if (preserve || p_debug == 4)
	{	FILE *x = fopen(FsmDot, "a");
		if (!x)
		{	printf("cannot create '%s'\n", FsmDot);
		} else
		{	fprintf(x, "digraph X%d {\n", uniq++);
			draw_fsm(p_tree, x);		// visit |= 4
			fprintf(x, "}\n");
			fclose(x);
			if (preserve)
			{	printf("wrote: %s\n", FsmDot);
			}
			if (p_debug == 4 && system(ShowFsm) < 0)
			{	perror("ShowFsm");
			}
			sleep(1); // in case there's more than one
	}	}

	return 1;
}

void
stop_threads(void)
{	int i;

	for (i = 0; i < Ncore; i++)
	{	sep[i].T_stop++;
	}
}

int
has_stop_cmd(Lextok *p)
{
	if (!p || (p->visit & 128))
	{	return 0;
	}
	p->visit |= 128;
	if (p->typ == STOP)
	{	return p->lnr+1;
	}
	return	has_stop_cmd(p->lft)
	||	has_stop_cmd(p->rgt)
	||	has_stop_cmd(p->a)
	||	has_stop_cmd(p->b)
	||	has_stop_cmd(p->c);
}

int
has_stop(void)
{
	return has_stop_cmd(p_tree);
}
