/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://codescrub.com/cobra
 */

%{
#include "cobra.h"
#include <regex.h>
#include <string.h>
#include <math.h>
#include "cobra_array.h"
#include "cobra_list.h"

// parser for inline programs

#define YYMAXDEPTH 32000
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
		C_TYP	v;	// VAL
	int	cdepth;		// fct call depth where defined
	Var_nm	*nxt;
};

static char *nr_tbl[] = {
	"0", "1", "2", "3", "4",
	"5", "6", "7", "8", "9"
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
static Lextok	 *break_out;
static Lextok	 *break_nm;
static Lextok	 *in_fct_def;
static Var_nm	 *lab_lst;
Separate	 *sep;		// thread local copies of globals
Lextok		 **my_tree;	// thread local copies of p_tree

int	*Cdepth;
int	*has_lock;

static int	 nest;
static int	 p_lnr = 1;
static int	 p_seq = 1;
static int	 n_blocks;	// used during parsing
static int	 a_cnt;		// conservative cnt of nr of arrays
static int	 v_cnt;		// conservative cnt of nr of vars
static int	*t_stop;

char	*derive_string(Prim **, Lextok *, const int, const char *);

static Block	*pop_context(int, int);
static Lextok	*mk_for(Lextok *, Lextok *, Lextok *, Lextok *, Lextok *);
static Lextok	*mk_foreach(Lextok *, Lextok *, Lextok *, Lextok *, Lextok *);
static Lextok	*new_lex(int, Lextok *, Lextok *);
static Var_nm	*mk_var(const char *, const int, const int);
static Var_nm	*check_var(const char *, const int);
static void	 set_break(void);

// to reduce the number of cache-misses in multicore mode
#ifndef NO_DUP
 #define DUP_TREES
#endif

static Lextok	*add_return(Lextok *);
#ifdef DEBUG
static void	dump_tree(Lextok *, int);
#endif
static void	dump_graph(FILE *, Lextok *, char *);
static void	check_global(Lextok *);
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
extern int	setexists(const char *s);
extern void	show_error(FILE *, int);
extern int	xxparse(void);
extern Prim	*cp_pset(char *, Lextok *, int);	// cobra_te.c

static char	*strrstr(const char *, const char *);
static int	yylex(void);

extern int	solo;
extern int	stream;
extern int	stream_override;
extern int	showprog;
%}

%token	NR STRING NAME IF IF2 ELSE ELIF WHILE FOR FOREACH IN PRINT ARG SKIP GOTO
%token	BREAK CONTINUE STOP NEXT_T BEGIN END SIZE RETRIEVE FUNCTION CALL
%token	ROUND BRACKET CURLY LEN MARK SEQ LNR RANGE FNM FCT ITOSTR ATOI
%token	BOUND MBND_D MBND_R NXT PRV JMP UNSET RETURN RE_MATCH FIRST_T LAST_T
%token	TXT TYP NEWTOK SUBSTR GSUB INT FLOAT SPLIT SET_RANGES CPU N_CORE SUM
%token	STRLEN STRCMP STRSTR STRRSTR
%token	A_UNIFY LOCK UNLOCK ASSERT TERSE TRUE FALSE VERBOSE
%token	FCTS MARKS SAVE RESTORE RESET SRC_LN SRC_NM HASH HASHARRAY
%token  ADD_PATTERN DEL_PATTERN IS_PATTERN CP_PSET
%token	ADD_TOP ADD_BOT POP_TOP POP_BOT TOP BOT
%token	OBTAIN_EL RELEASE_EL UNLIST LLENGTH GLOBAL LIST2SET
%token	DISAMBIGUATE WITH
%token	OPEN CLOSE GETS PUTS UNLINK EXEC

%right	'='
%left	OR
%left	AND
%left   B_OR B_AND
%left	B_XOR
%left	EQ NE
%left	GT LT GE LE
%left	LSH RSH
%left	EXP
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
cmpnd_stmnt: IF '(' expr ')' c_prog optelse {
			   $1->lft = $3;	// cond
			   $1->rgt = new_lex(0, $5, $6);
			   $1->lnr = p_lnr;
			   $$ = $1; }
	| WHILE '(' expr ')' c_prog {
			   $1->lft = $3;	// cond
			   $1->rgt = $5;	// body
			   n_blocks++;
			   $$ = $1; }
	;
stmnt	: cmpnd_stmnt	{ $$ = $1; }
	| FOR '(' NAME IN NAME ')' c_prog {
			   n_blocks++; v_cnt++; a_cnt++;
			   // $5 is assumed to be an array name
			   // if not, this is a skip
			   set_break();
			   $$ = new_lex(';', mk_for($1, $3, $5, $7, break_nm), break_out); }
	| FOREACH opt_type '(' NAME IN NAME ')' c_prog {
			    n_blocks++; v_cnt++;
			    set_break();
			    $$ = new_lex(';', mk_foreach($2, $4, $6, $8, break_nm), break_out); }
	| FUNCTION NAME '(' params ')' { check_global($2); } c_prog {
			   in_fct_def = (Lextok *) 0;
			   $1->lft = new_lex(0, $2, $4);

			   if ((!$7->rgt || !$7->rgt->typ)
			   &&   $7->typ != RETURN)
			   {	$1->rgt = new_lex(';', $7, new_lex(RETURN, 0, 0));
			   } else
			   {	if ($7->typ != WHILE)
				{	$1->rgt = add_return($7);
				} else // fixed after fix_while in find_fct
				{	$1->rgt = $7;
			   }	}

			   add_fct($1);
			   n_blocks++;
			   $$ = $1;
			}
	| NAME ':' stmnt { v_cnt++; mk_lab($1, $3); $$ = $3; }
	| b_stmnt ';'
	;
opt_type: /* empty */	{ $$ = 0; }
	| NAME		{ $$ = $1; }
	;
b_stmnt	: p_lhs '=' expr	{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| p_lhs '=' NEWTOK '(' ')' { $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| ASSERT '(' expr ')'	{ $1->lft = $3; $$ = $1; }
	| src_ln '(' expr ',' expr ',' expr ')'	{
				$1->lft = new_lex(0, $3, $5);
				$1->rgt = $7;
				$$ = $1;
			}
	| GLOBAL glob_list	{ $$ = $1; handle_global($2); }
	| GOTO NAME		{ $1->lft = $2; $$ = $1; }
	| PRINT args		{ $1->lft = $2; $$ = $1; }
	| UNSET NAME '[' e_list ']' {
			   $1->lft = $2;
			   $1->rgt = $4;
			   $$ = $1;
			}
	| UNSET NAME		{ $1->lft = $2; $$ = $1; }
	| LIST2SET '(' expr ',' STRING ')' { $1->lft = $3; $1->rgt = $5; $$ = $1; }
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
	| A_UNIFY '(' NAME ',' expr ')' { $1->lft = $5; $1->rgt = $3; $$ = $1; }
	| A_UNIFY '(' expr ')'	{ $1->lft = $3; $1->rgt = 0; $$ = $1; }

	| ADD_PATTERN '(' s_ref ',' t_ref ',' t_ref ')'	{
				$1->lft = $3;
				$1->rgt = new_lex(0, $5, $7);
				$$ = $1;
			}
	| DEL_PATTERN '(' s_ref ',' t_ref ',' t_ref ')'	{
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

	| PUTS NAME args	{ $1->rgt = $2; $1->lft = $3; $$ = $1; }
	| GETS NAME NAME	{ $1->rgt = $2; $1->lft = $3; $$ = $1; }
	| CLOSE NAME		{ $1->rgt = $2; $$ = $1; }
	| UNLINK expr		{ $1->lft = $2; $$ = $1; }
	| EXEC expr		{ $1->lft = $2; $$ = $1; }

	| RETURN expr	{ $1->lft = $2; $$ = $1; }
	| RETURN
	| BREAK
	| CONTINUE
	| NEXT_T
	| STOP
	;
src_ln	: SRC_LN
	| SRC_NM
	;
t_ref	: p_ref
	| NAME  '.' fld		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| NAME
	| '.' fld %prec UMIN	{ $1->lft =  0; $1->rgt = $2; $$ = $1; }
	| '.'
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
	| ELSE cmpnd_stmnt	{ $1->rgt = $2; $$ = $1; }
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
	| expr EXP expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr '^' expr		{ $2->lft = $1; $2->rgt = $3; $2->typ = B_XOR; $$ = $2; }
	| expr LSH expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| expr RSH expr		{ $2->lft = $1; $2->rgt = $3; $$ = $2; }
	| '!' expr %prec UMIN	{ $1->rgt = $2; $$ = $1; }
	| '-' expr %prec UMIN	{ $1->typ = UMIN; $1->rgt = $2; $$ = $1; }
	| '~' expr %prec UMIN	{ $1->rgt = $2; $$ = $1; }
	| '^' expr %prec UMIN	{ $1->rgt = $2; $$ = $1; }
	| STRING		{ fixstr($1); $$ = $1; }
	| CP_PSET '(' s_ref ')' optcond	{ $1->lft = $3; $1->rgt = $5; $$ = $1; }
	| HASH '(' expr ')'	{ $1->lft = $3; $$ = $1; }
	| HASHARRAY '(' expr ',' expr ')' { $1->lft = $3; $$->rgt = $5; $$ = $1; }
	| IS_PATTERN '(' s_ref ')'	{ $1->lft = $3; $$ = $1; }
	| SUBSTR '(' expr ',' expr ',' expr ')' {
				  $1->lft = $3;
				  $1->rgt = new_lex(0, $5, $7);
				  $$ = $1;
				}

	| OPEN expr n_or_s	{ $1->rgt = $2; $1->lft = $3; $$ = $1; }

	| SPLIT '(' expr ',' NAME ')'            { $1->lft = $3; $1->rgt = $5; $1->s = ",";   $$ = $1; }
	| SPLIT '(' expr ',' STRING ',' NAME ')' { $1->lft = $3; $1->rgt = $7; $1->s = $5->s; $$ = $1; }
	| GSUB  '(' expr ',' expr ',' expr ')'	 { $1->lft = new_lex(0, $3, $5); $1->rgt = $7; $$ = $1; }
	| STRSTR '(' expr ',' STRING ')'	 { $1->lft = $3; $1->rgt =  0; $1->s = $5->s; $$ = $1;  }
	| STRCMP '(' expr ',' expr ')'		 { $1->lft = $3; $1->rgt = $5; $$ = $1;  }
	| STRRSTR '(' expr ',' STRING ')'	 { $1->lft = $3; $1->rgt =  0; $1->s = $5->s; $$ = $1;  }
	| INT '(' expr ')'			 { $1->lft = $3; $$ = $1; }
	| FLOAT '(' expr ')'			 { $1->lft = $3; $$ = $1; }
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
	| ITOSTR '(' expr ')'	{ $1->lft = $3; $$ = $1; }
	| ATOI '(' expr ')'	{ $1->lft = $3; $$ = $1; }
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
n_or_s	: NAME
	| STRING
	;
optcond : /* empty */		{ $$ = 0; }
	| WITH '(' expr ')'	{ $$ = $3; }
	| WITH STRING		{ $$ = $2; }
	;
s_ref	: expr
	| '*'			{ $$ = new_lex(NAME, 0, 0); $$->s = "*"; }
	;
p_ref	: BEGIN
	| END
	| FIRST_T
	| LAST_T
	;
p_lhs	: b_name opt_dot	{ if ($2 == 0)
				  {	$$ = $1;
				  } else
				  {	if ($2->lft == 0)
					{	$$ = new_lex('.', $1, $2->rgt);
					} else
					{	$$ = new_lex('.', $1, $2);
				  }	}
				}
	| '.'			{ $$ = $1; }
	| '.' fld opt_dot %prec UMIN {
				  if ($3 == 0)
				  {	$1->lft = 0;
					$1->rgt = $2;
					$$ = $1;
				  } else
				  {	$1->lft = 0;
					$1->rgt = new_lex('.', $2, $3);
					$$ = $1;
				} }
	;

opt_dot: /* empty */		{ $$ = 0; }
	| '.' fld opt_dot	{
				  if ($3 == 0)
				  {	$1->lft = 0;
					$1->rgt = $2;
					$$ = $1;
				  } else
				  {	$1->lft = $2;
					$1->rgt = $3;
					$$ = $1;
				} }
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

	{ "open", 	OPEN },
	{ "gets", 	GETS },
	{ "puts", 	PUTS },
	{ "close", 	CLOSE },
	{ "unlink",	UNLINK },

	{ "a_unify",	A_UNIFY },
	{ "add_pattern", ADD_PATTERN },
	{ "assert",	ASSERT },
	{ "atoi",	ATOI },
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
	{ "elif",	ELIF },
	{ "else",	ELSE },
	{ "End",	END },
	{ "exec",	EXEC },
	{ "false",	FALSE },
	{ "fct",	FCT },
	{ "fcts",	FCTS },
	{ "first_t",	FIRST_T },
	{ "fnm",	FNM },
	{ "for",	FOR },
	{ "foreach",	FOREACH },
	{ "function",	FUNCTION },
	{ "global",	GLOBAL },
	{ "goto",	GOTO },
	{ "gsub",	GSUB },
	{ "hash",	HASH },
	{ "hasharray",	HASHARRAY },
	{ "if",		IF },
	{ "in",		IN },
	{ "is_pattern",	IS_PATTERN },
	{ "itostr",	ITOSTR },
	{ "jmp",	JMP },
	{ "last_t",	LAST_T },
	{ "len",	LEN },
	{ "list2set",	LIST2SET },
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
	{ "pattern_exists", IS_PATTERN },
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
	{ "src_ln",	SRC_LN },
	{ "src_nm",	SRC_NM },
	{ "Stop",	STOP },
	{ "strchr",	STRSTR },	// for backward compatibility
	{ "strstr",	STRSTR },
	{ "strcmp",	STRCMP },
	{ "strrchr",	STRRSTR },	// for backward compatibility
	{ "strrstr",	STRRSTR },
	{ "strlen",	STRLEN },
	{ "substr",	SUBSTR },
	{ "sum",	SUM },
	{ "terse",	TERSE },
	{ "tofloat",	FLOAT },
	{ "toint",	INT },
	{ "tostr",	ITOSTR },
	{ "true",	TRUE },
	{ "txt",	TXT },
	{ "type",	TYP },
	{ "typ",	TYP },
	{ "unlock",	UNLOCK },
	{ "unset",	UNSET },
	{ "verbose",	VERBOSE },
	{ "while",	WHILE },
	{ "with",	WITH },
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
	{ "**", EXP },
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
hash2_cum(const char *s, const int seq)	// cumulative
{	static uint64_t h = 0;
	static char t = 0;

	if (seq == 0)
	{	t = *s++;
		h = 0x88888EEFL ^ (0x88888EEFL << 31);
	} else	// add a space as separator
	{	h ^= ((h << 7) ^ (h >> (57))) + ' ';
	}
	while (*s != '\0')
	{	h ^= ((h << 7) ^ (h >> (57))) + *s++;
	}
	return ((h << 7) ^ (h >> (57))) ^ t;
}

ulong
hash3(const char *s)	// debugging version of hash2
{
	if (sizeof(ulong) == 8)
	{	uint64_t h = 0x88888EEFL ^ (0x88888EEFL << 31);
		const char t = *s++;
		while (*s != '\0')
		{
			h ^= ((h << 7) ^ (h >> (57))) + *s++;
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

static void
set_break(void)
{
	break_out = new_lex(SKIP, 0, 0);
	break_nm  = new_lex(NAME, 0, 0);
	break_nm->s = (char *) emalloc(8*sizeof(char), 74);
	snprintf(break_nm->s, 8, "_L%d_", v_cnt);
	mk_lab(break_nm, break_out); // destination for break
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

static void
replace_break(Lextok *b, Lextok *out)
{
	// replace break with jump to out
	// but not in enclosed statements

	if (!b)
	{	return;
	}
	if (b->typ == BREAK)
	{	b->typ = GOTO;
		b->lft = out;
	}
	replace_break(b->lft, out);
	if (b->typ != WHILE)
	{	replace_break(b->rgt, out);
	}
}

static void
replace_continue(Lextok *b, Lextok *stub)
{
	if (!b)
	{	return;
	}
	if (b->typ == CONTINUE)
	{	b->typ = ';';
		b->lft = stub->lft;
		b->rgt = stub->rgt;
	}
	replace_continue(b->lft, stub);
	replace_continue(b->rgt, stub);
}

static Lextok *
mk_foreach_token_in_pattern(Lextok *nm, Lextok *p, Lextok *body, Lextok *out)
{	static int ln = 0;

	// foreach (nm in p)
	// p->typ == NAME and p->s is the name of the pset variable
	// that points to a token with p_start and p_end set (jmp and bound)
	// with jmp.seq <= bound.seq

	Lextok *from = new_lex('.', p, new_lex(JMP,    0, 0));	// from = p.jmp
	Lextok *upto = new_lex('.', p, new_lex(BOUND,  0, 0));	// upto = p.bound

	Lextok *temp  = new_lex(NAME, 0, 0); temp->s = "temp";
	Lextok *init1 = new_lex('=', temp, upto);		// temp = upto
	Lextok *init2 = new_lex('=', nm, from);			// nm = from (loop init)

	Lextok *seq1 = new_lex('.',   nm, new_lex(SEQ, 0, 0));	// nm.seq
	Lextok *seq2 = new_lex('.', temp, new_lex(SEQ, 0, 0));	// temp.seq (p.bound.seq)
	Lextok *seq3 = new_lex('.', from, new_lex(SEQ, 0, 0));	// from.seq
	Lextok *nxt  = new_lex('.',   nm, new_lex(NXT, 0, 0));	// nm.nxt

	Lextok *cond = new_lex(LE, seq1, seq2);			// nm.seq <= temp.seq (p.bound.seq)
	Lextok *incr = new_lex('=', nm, nxt);			// nm = nm.nxt        (loop increment)

	Lextok *loop = new_lex(NAME, 0, 0);
	loop->s = (char *) emalloc(8*sizeof(char), 74);
	snprintf(loop->s, 8, "_%d@_", ln++);
	Lextok *gt  = new_lex(GOTO, loop, 0);			// goto _%d@_ (loop start)

	Lextok *s1  = new_lex(';', body, incr);			// body ; nm = nm.nxt
	Lextok *s2  = new_lex(';', s1, gt);			// body ; nm = nm.nxt ; goto loop
	Lextok *whl = new_lex(WHILE, cond, s2);			// while cond (nm.seq <= p.bound.seq)
	mk_lab(loop, whl);					// loop : while (nm.seq <= p.bound.seq)

	Lextok *both = new_lex(';', init2, whl);		// nm = from ; loop: while ...
	Lextok *prep = new_lex(';', init1, both);		// temp = upto ; nm = from ; loop: while ...
								// which is the complete loop
	// 'continue'
	Lextok *c_nxt  = new_lex('.', nm, new_lex(NXT, 0, 0));	// nm.nxt
	Lextok *c_incr = new_lex('=', nm, c_nxt);		// nm = nm.nxt
	Lextok *c_gt  = new_lex(GOTO, loop, 0);			// goto _%d@_ (loop start)
	Lextok *c_continue = new_lex(';', c_incr, c_gt);
	replace_continue(body, c_continue);

	// prepend a validity check:
	// if a pattern set is referenced that is empty or undefined
	// we check that the starting sequence number from.seq in seq3 is not zero

	Lextok *zero  = new_lex(NR, 0, 0);
	Lextok *brk   = new_lex(GOTO, out, 0);
	Lextok *cc    = new_lex(EQ, seq3, zero);
	Lextok *ff    = new_lex(IF, cc, brk);	// if (seq3 == 0) goto out
	Lextok *top   = new_lex(';', ff, prep);

	return top;
}

static Lextok *
mk_foreach(Lextok *opt_type, Lextok *nm, Lextok *p, Lextok *body, Lextok *out)
{	Lextok *whl, *seq, *cond, *asgn, *nxt, *zero, *loop;
	Lextok *s1, *s2, *gt;
	static int ln = 0;
	int is_set;

	if (opt_type)
	{	assert(opt_type->typ == NAME);
		if (strcmp(opt_type->s, "token") == 0)
		{	opt_type = 0;
		} else if (strcmp(opt_type->s, "pattern") == 0)
		{	;	// ok
		} else if (strcmp(opt_type->s, "index") == 0
		       ||  strcmp(opt_type->s, "element") == 0)
		{
			if (1)
			{	return mk_for(0, nm, p, body, out);
			} else
			{	fprintf(stderr, "error: foreach %s (...) should be: for (...)\n",
					opt_type->s);
				return new_lex(STOP, 0, 0);
			}
		} else
		{	fprintf(stderr, "error: usage: foreach [pattern token index] (...)\n");
			return new_lex(STOP, 0, 0);
	}	}

	if (!p || !p->s)
	{	fprintf(stderr, "foreach: bad format: missing name\n");
		return new_lex(STOP, 0, 0);
	}

	// replace 'break' with 'goto out' and 'continue' with 'skip'
	replace_break(body, out);

	// p is either a Set name or a Pattern from a set
	// if a pattern set p->s must exist before the inline program
	// starts -- i.e., it cannot be created first as part of
	// the inline program....

	is_set = is_pset(p->s);
	if (!is_set && !opt_type) // using foreach 'pattern' bypasses this check
	{	if (p->typ == NAME)
		{	return mk_foreach_token_in_pattern(nm, p, body, out);
		}
		fprintf(stderr, "error: unexpected type '%d' of '%s'\n", p->typ, p->s);
		return new_lex(STOP, 0, 0);
	}

	// mk_foreach_pattern_set:
	// printf("foreach %s\n", p->s);
	//	 a1   ;		pset(p)
	//	 nm   ;		nm = pset(p)
	// loop: whl  ;		nm.seq > 0
	//	 body ;
	//	 asgn ;		nm = nm.nxt
	//	 goto loop

	zero = new_lex(NR, 0, 0);

	Lextok *a1 = new_lex(CP_PSET, p, 0);
	Lextok *a2 = new_lex('=', nm, a1);		// creates var p->s

	seq  = new_lex('.', nm, new_lex(SEQ, 0, 0));	// nm.seq
	nxt  = new_lex('.', nm, new_lex(NXT, 0, 0));	// nm.nxt
	cond = new_lex(GT,  seq, zero);			// seq > 0
	asgn = new_lex('=', nm, nxt);			// nm = nm.nxt

	loop = new_lex(NAME, 0, 0);
	loop->s = (char *) emalloc(8*sizeof(char), 74);
	snprintf(loop->s, 8, "_@%d@_", ln++);		// start of loop

	gt  = new_lex(GOTO,  loop, 0);
	s1  = new_lex(';',   body, asgn);
	s2  = new_lex(';',   s1, gt);

	whl = new_lex(WHILE, cond, s2);
	mk_lab(loop, whl);

	Lextok *both = new_lex(';', a2, whl);

	// 'continue'
	Lextok *c_nxt  = new_lex('.', nm, new_lex(NXT, 0, 0));	// nm.nxt
	Lextok *c_incr = new_lex('=', nm, c_nxt);		// nm = nm.nxt
	Lextok *c_gt  = new_lex(GOTO, loop, 0);			// goto _%d@_ (loop start)
	Lextok *c_continue = new_lex(';', c_incr, c_gt);
	replace_continue(body, c_continue);

	return both;
}

static Lextok *
mk_for(Lextok *a, Lextok *nm, Lextok *ar, Lextok *body, Lextok *out)
{	Lextok *mrk, *seq, *asgn1, *asgn2, *loop;
	Lextok *cond, *whl, *txt;
	Lextok *setv, *one, *incr, *zero;
	Lextok *s1, *s2, *s3, *s4, *gt;
	Lextok *tt, *asgn3, *c_gt, *c_skp, *c_continue;
	static int ln = 0;	// parse time only
	int ix = 0;

	// printf("for %s in %s\n", nm->s, ar->s);
	//	 asgn1 ;	nm.seq = size(ar);
	//	 asgn2 ;	nm.mark = 0;
	// loop: whl   ;	nm.mark < nm.seq
	//	 setv  ;	nm.txt = retrieve(ar, nm.mark)
	//	 incr  ;	nm.mark++
	//	 body  ;
	//	 goto loop

	one  = new_lex(NR, 0, 0); one->val = 1;		// 1
	zero = new_lex(NR, 0, 0); 			// 0

	replace_break(body, out);

	// target index variable
	(void) mk_var(nm->s, STR, ix);	// creates or casts nm to STR

	// loop info
	for (ix = 0; ix < Ncore; ix++)	// separate vars to support independent scans
	{	Var_nm *vn = mk_var("__tmp__", PTR, ix); // internal loop counts
		if (vn) { vn->rtyp = PTR; } // in case its an existing var
	}
	tt = new_lex(NAME, 0, 0); tt->s = "__tmp__";

	mrk   = new_lex('.', tt, new_lex(MARK, 0, 0));	// tt.mark
	seq   = new_lex('.', tt, new_lex(SEQ,  0, 0));	// tt.seq
	txt   = new_lex('.', tt, new_lex(TXT,  0, 0));	// tt.txt

	asgn1 = new_lex('=', seq, new_lex(SIZE, 0, ar));// seq = size(ar)
	asgn2 = new_lex('=', mrk, zero);		// mrk = 0
	asgn3 = new_lex('=', nm, txt);	// nm.s = tt.txt

	cond = new_lex(LT,  mrk, seq);			// mrk < seq
	incr = new_lex('=', mrk, new_lex('+', mrk, one)); // mrk++
	setv = new_lex('=', txt, new_lex(RETRIEVE, mrk, ar)); // txt = retrieve(ar, mrk)

	loop = new_lex(NAME, 0, 0);
	loop->s = (char *) emalloc(8*sizeof(char), 74);
	snprintf(loop->s, 8, "_@%d_", ln++);

	gt  = new_lex(GOTO,  loop, 0);	// shouldnt be needed
	s1  = new_lex(';',   body, gt);
	s2  = new_lex(';',   incr, s1);
	s3  = new_lex(';',   asgn3, s2);
	s4  = new_lex(';',   setv, s3);
	whl = new_lex(WHILE, cond,  s4);
	mk_lab(loop, whl);
	s4  = new_lex(';',  asgn2, whl);

	// 'continue'
	c_skp = new_lex(SKIP, 0, 0);		// shouldnt be needed
	c_gt  = new_lex(GOTO, loop, 0);		// goto _%d@_ (loop start)
	c_continue = new_lex(';', c_skp, c_gt);

	replace_continue(body, c_continue);

	return new_lex(';',  asgn1, s4);
}

static void
check_global(Lextok *nm)
{	// make sure function definitions are not nested
	if (nm
	&&  in_fct_def != NULL)
	{	fprintf(stderr, "fct def %s is nested in fct def %s\n",
			nm->s, in_fct_def->s);
		yyerror("nested function definition");
	}
	in_fct_def = nm;
}

static void
mk_lab(Lextok *t, Lextok *p)
{	Var_nm *n;

	// fprintf(stderr, "mk_lab '%s' ln %d -- fct: %s\n",
	//	t->s, p?p->lnr:-1, in_fct_def?in_fct_def->s:"global");
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
	n->s = in_fct_def?in_fct_def->s:"global";
	n->cdepth = Cdepth[0];	// during parsing only
	n->nxt = lab_lst;
	lab_lst = n;
}

static Lextok *
find_label(char *s)
{	Var_nm *n;
	char *scp = in_fct_def?in_fct_def->s:"global";

	for (n = lab_lst; n; n = n->nxt)
	{	if (strcmp(n->nm, s) == 0)
		{	// current scope has to match that of the label
			assert(n->s != NULL);
			// fprintf(stderr, "find_lab '%s' scope: %s <-> %s\n", s, n->s, scp);
			if (strcmp(n->s, scp) == 0)
			{	return (Lextok *) n->pm;
			}
	}	}
	fprintf(stderr, "error: label '%s' undefined in current scope (%s)\n", s, scp);
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
	static int deferred_if = 0;

	yylval = (Lextok *) emalloc(sizeof(Lextok), 77);
	yylval->lnr = p_lnr;
	yytext[0] = '\0';

	if (deferred_if)
	{	yylval->s = emalloc(strlen("if")+1, 78);
		strcpy(yylval->s, "if");
		yylval->typ = IF;
		deferred_if = 0;
	} else

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
		{
#ifdef C_FLOAT
			for (i = 0; isdigit((uchar) n) || n == '.'; i++)
#else
			for (i = 0; isdigit((uchar) n); i++)
#endif
			{	yytext[i] = (char) n;
				n = fgetc(pfd);
			}
			ungetc(n, pfd);
			yytext[i] = '\0';
#ifdef C_FLOAT
			if (strchr(yytext, '.') != NULL)
			{	yylval->s = emalloc(strlen(yytext)+1, 78);
				strcpy(yylval->s, yytext);
				yylval->typ = STRING;
			} else
#endif
			{	yylval->val = atoi(yytext);
				yylval->typ = NR;
			}
		} else if (isalpha((uchar) n))
		{	for (i = 0; isalnum((uchar) n) || n == '_'; i++)
			{	yytext[i] = (char) n;
				n = fgetc(pfd);
			}
			ungetc(n, pfd);
			yytext[i] = '\0';

			if (strcmp(yytext, "elif") == 0)	// version 5.0
			{	strcpy(yytext, "else");
				deferred_if++;
			}

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
			case '*': expect('*', EXP,  n); break;
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

	if (!t || !x)
	{	return;
	}

	if (x != stdout && x != stderr)
	{	fprintf(x, "N%d:%d: ", t->tag, t->lnr);
	}

	for (i = 0; key[i].s; i++)
	{	if (t->typ == key[i].t)
		{	fprintf(x, "%s", key[i].s);
			break;
	}	}

	if (!t->s) { t->s = "?"; }

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

	if (x == stdout || x == stderr)
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

#ifdef DEBUG
static void
indent(int level, const char *s)
{	int i;

	for (i = 0; i < level; i++)
	{	fprintf(stderr, "   ");
	}
	fprintf(stderr, "%s", s);
}

static void
dump_tree(Lextok *t, int i)
{
	if (!t || (t->visit&16))
	{	indent(i, "");
		fprintf(stderr, "<<%p::%d>> %s\n", (void *) t, t?t->typ:-1, t && t->typ == NAME?t->s:"");
		return;
	}
	t->visit |= 16;
	indent(i, "");

	fprintf(stderr, "<%p><%d> ", (void *) t, t?t->typ:-1);

	if (t->s) { fprintf(stderr, "%s%s ", t->f?"fct ":"", t->s); }
	tok2txt(t, stderr);
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
#endif

static void
dump_graph(FILE *fd, Lextok *t, char *tag)
{	int i;

	// similar to draw_fsm, but simpler

	if (!t || (t->visit&16))
	{	fprintf(fd, "N%p;\n", (void *) t);
		return;
	}
	t->visit |= 16;
	if (t && t->s && strchr(t->s, '\n'))
	{	if (t->s[0] != '\n')
		{ fprintf(fd, "N%p [label=\"%s %d %c..<nl> ", (void *) t, tag, t?t->typ:-1, t->s[0]);
		} else
		{ fprintf(fd, "N%p [label=\"%s %d %s ", (void *) t, tag, t?t->typ:-1, "...<nl>");
		}
	} else
	{	fprintf(fd, "N%p [label=\"%s %d %s ", (void *) t, tag, t?t->typ:-1, t && t->s?t->s:"");
	}
        for (i = 0; key[i].s; i++)
        {       if (t->typ == key[i].t)
                {       fprintf(fd, "%s", key[i].s);
                        break;
        }       }
	switch (t->typ) {	// needs more cases
	case NR: fprintf(fd, "NR %d", t->val); break;
	case EQ: fprintf(fd, "EQ"); break;
	case LE: fprintf(fd, "<="); break;
	case GE: fprintf(fd, ">="); break;
	case LT: fprintf(fd, "<"); break;
	case GT: fprintf(fd, ">"); break;
	case '.': fprintf(fd, "."); break;
	case ';': fprintf(fd, ";"); break;
	case '=': fprintf(fd, "="); break;
	case ARG: fprintf(fd, "arg"); break;
	case STRING: fprintf(fd, "str"); break;
	case SKIP: fprintf(fd, "skip"); break;
	case IF2: fprintf(fd, "if2"); break;
	case IF:  fprintf(fd, "if"); break;
	case GOTO: fprintf(fd, "goto"); break;
	}
	fprintf(fd, "\" shape=box];\n");

	if (t->lft)
	{	fprintf(fd, "N%p -> N%p;\n", (void *) t, (void *) t->lft);
		dump_graph(fd, t->lft, "L");
	}
	if (t->rgt)
	{	fprintf(fd, "N%p -> N%p;\n", (void *) t, (void *) t->rgt);
		dump_graph(fd, t->rgt, "R");
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
		{
#ifdef C_FLOAT
			printf("%f\n", tmp.val); // verbose>1
#else
			printf("%d\n", tmp.val);
#endif
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
		} else
		{	fprintf(stderr, "error: actual parameter for %s() missing\n", fnm);
			show_error(stderr, formal->rgt->lnr);
			sep[ix].T_stop++;
		}
	} else // if (!actual->rgt)
	{
	//	if (!actual->rgt || actual->typ == '.')
		{	map_var(ref_p, fnm, formal, actual, ix);
		}
	//	else
	//	{	fprintf(stderr, "error: too many parameters for %s() -- %d\n", fnm, actual->typ);
	//		show_error(stderr, actual->rgt->lnr);
	//		sep[ix].T_stop++;
	//	}
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
			{	Lextok *prev_fct = in_fct_def;
				f->has_fsm++;
			//	assert(in_fct_def == NULL);
				in_fct_def = t->lft;
				mk_fsm(f->body, ix);
				in_fct_def = prev_fct; // added 12/24
			//	assert(in_fct_def == t->lft);
				in_fct_def = (Lextok *) 0;
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
		if (!t->a)
		{	t->a = new_lex(STOP, 0, 0);
			sep[ix].T_stop++;
			show_error(stderr, t->lnr);	
		}
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
	{	if (yylval->typ == EOF)
		{	printf("near EOF\n");
		} else
		{	tok2txt(yylval, stdout);
		}
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
#ifdef C_FLOAT
			rv->val = atof(rv->s);
#else
			rv->val = atoi(rv->s);
#endif
		} else if (rv->rtyp == VAL
		&& a->rtyp == STR
		&& a->s
		&& isdigit((uchar) a->s[0]))
		{	a->rtyp = VAL;
#ifdef C_FLOAT
			a->val = atof(a->s);
#else
			a->val = atoi(a->s);
#endif
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
			{	if (q->s)
				{	n = mk_var(q->s, 0, ix);
				} else
				{	return n;
			}	}
			rv->rtyp = n->rtyp;
		} else
		{	if (q->typ == '['	// array
			&&  q->lft
			&&  q->lft->typ == NAME) // index is q->rgt
			{	static Var_nm vtmp;
				// try to cover a case that fail otherwise
				// as in: Name[index].mark = 9
				eval_prog(ref_p, q, rv, ix);
				if (rv->rtyp == PTR)
				{	// if succeeds, and a PTR, we have
					// something that can be returned as
					// a dummy variable of type PTR, so
					// that it can be referenced elsewhere
					memset(&vtmp, 0, sizeof(Var_nm));
					vtmp.rtyp = PTR;
					vtmp.pm = rv->ptr;
					vtmp.nm = "noname";
					return &vtmp;	// dubious but static
				} else
				{	static Prim notoken;
					memset(&notoken, 0, sizeof(Prim));
					vtmp.rtyp = PTR;
					vtmp.pm = &notoken;
					vtmp.nm = "nothing";
					return &vtmp;		
				}
			}

			if (q->lft
			&&  q->lft->typ == NAME
			&&  q->rgt
			&&  (q->rgt->typ == JMP || q->rgt->typ == BOUND))
			{	// static Prim notoken;
				// memset(&notoken, 0, sizeof(Prim));
				// refers to an aatribute of a non-token name
				return get_var(ref_p, q->lft, rv, ix);	// new 2024/3/14
			}

			if (Ncore > 1) fprintf(stderr, "(%d) ", ix);
			fprintf(stderr, "line %d: unexpected variable type %d, in ", q->lnr, q->typ);
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

extern int do_split(char *, const char *, const char *, Rtype *, const int);	// cobra_array.c

static void
split(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)
{	// q->lft: source string to split on commas
	// q->rgt: name of target array,
	//  create if it doesn't exist, unset if it exists
	// returns nr of fields
	Rtype a, b, c;
	// q->s == string to split on, defaults to ","

	if (strlen(q->s) != 1)
	{	fprintf(stderr, "error: split argument '%s' is not a single character\n", q->s);
		rv->rtyp = STP;
		return;
	}

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

	rv->val = do_split(a.s, q->rgt->s, q->s, &c, ix);	// cobra_array.c
	rv->rtyp = VAL;						// nr of fields
}

static void
gsub(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)
{	// q->lft->lft: text to be replaced
	// q->lft->rgt: replacement text
	// q->rgt: target string
	Rtype a, b, c;
	int cnt = 0;
	char *p, *r;

	assert(q->lft && q->lft->lft && q->lft->rgt && q->rgt);
	eval_prog(ref_p, q->lft->lft, &a, ix);
	if (a.rtyp != STR || strlen(a.s) == 0)
	{	fprintf(stderr, "error: 1st arg of gsub is not a string, or empty\n");
		rv->rtyp = STP;
		return;
	}
	eval_prog(ref_p, q->lft->rgt, &b, ix);
	if (b.rtyp != STR)
	{	fprintf(stderr, "error: 2nd arg of gsub is not a string\n");
		rv->rtyp = STP;
		return;
	}
	eval_prog(ref_p, q->rgt, &c, ix);
	if (b.rtyp != STR || strlen(c.s) == 0)
	{	fprintf(stderr, "error: 3rd arg of gsub is not a string\n");
		rv->rtyp = STP;
		return;
	}
	rv->rtyp = STR;

	p = c.s;
	while ((r = strstr(p, a.s)) != NULL)
	{	cnt++;
		p = r + strlen(a.s);
	}
	if (cnt == 0)
	{	rv->s = c.s;
		return;
	}
	rv->s = (char *) hmalloc(strlen(c.s) + cnt * (strlen(b.s) - strlen(a.s))  + 1, ix, 140);

	p = c.s;
	while (strlen(p) > 0 && (r = strstr(p, a.s)) != NULL)
	{	int d = *r;
		*r = '\0';
		strcat(rv->s, p);
		strcat(rv->s, b.s);
		*r = d;
		p = r + strlen(a.s);
	}
	if (strlen(p) > 0)
	{	strcat(rv->s, p);
	}
}

static char *
strrstr(const char *haystack, const char *needle)
{	char *p = NULL, *q;

	do {	q = p;
		p = strstr(haystack, needle);
	} while (p);

	return q;
}

static void
do_common(Prim **ref_p, Lextok *q, Rtype *rv, int which, const int ix)
{	Rtype a;
	char *ptr;
	// q->s == string to find

	if (q->s == 0 || strlen(q->s) == 0)
	{	fprintf(stderr, "error: %s bad argument '%s'\n",
			(which == STRSTR)?"strstr":"strrstr", q->s);
		rv->rtyp = STP;
		return;
	}

	assert(q->lft && !q->rgt);
	eval_prog(ref_p, q->lft, &a, ix);

	if (a.rtyp != STR)
	{	fprintf(stderr, "error: 1st arg of str[r]chr is not a string\n");
		rv->rtyp = STP;
		return;
	}
	if (which == STRSTR)
	{	ptr = strstr(a.s, q->s);
	} else
	{	ptr = strrstr(a.s, q->s);
	}

	if (!ptr)
	{	rv->val = 0;
	} else
	{	rv->val = 1 + (int) (ptr - a.s);
	}
	rv->rtyp = VAL;		// index of first or last match
}

static void
do_strstr(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)	// index of q->s in q->lft
{
	do_common(ref_p, q, rv, STRSTR, ix);
}

static void
do_strrstr(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)
{
	do_common(ref_p, q, rv, STRRSTR, ix);
}

static void
do_strcmp(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)
{	Rtype a, b;

	assert(q->lft && q->rgt);
	eval_prog(ref_p, q->lft, &a, ix);
	if (a.rtyp != STR)
	{	fprintf(stderr, "error: 1st arg of strcmp is not a string\n");
		show_error(stderr, q->lnr);
		sep[ix].T_stop++;
		rv->rtyp = STP;
		return;
	}
	eval_prog(ref_p, q->rgt, &b, ix);
	if (b.rtyp != STR)
	{	fprintf(stderr, "error: 2nd arg of strcmp is not a string\n");
		show_error(stderr, q->lnr);
		sep[ix].T_stop++;
		rv->rtyp = STP;
		return;
	}
	rv->rtyp = VAL;
	rv->val  = strcmp(a.s, b.s);
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
	strncpy(rv->s, &a.s[(int) b.val], c.val);
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
	if (!q)
	{	return;
	}
	switch (q->typ) {
	case ARG:
		print_args(ref_p, q->lft, rv, ix);
		print_args(ref_p, q->rgt, rv, ix);
		return;
	case MARKS:
		eval_prog(ref_p, q->lft, rv, ix);
		Assert("marks", rv->rtyp == VAL, q->lft);
		rv->val = nr_marks_int((int) rv->val, ix);
		fprintf(tfd, "%d", (int) rv->val);
		return;
	case INT:
		eval_prog(ref_p, q->lft, rv, ix);
		Assert("toint", rv->rtyp == STR, q->lft);
#ifdef C_FLOAT
		rv->val = (C_TYP) round(atof(rv->s));
#else
		rv->val = (C_TYP) atoi(rv->s);
#endif
		fprintf(tfd, "%d", (int) rv->val);
		return;
	case FLOAT:
#ifdef C_FLOAT
		eval_prog(ref_p, q->lft, rv, ix);
		Assert("tofloat", rv->rtyp == STR, q->lft);
		rv->val = (C_TYP) atof(rv->s);
		fprintf(tfd, "%f", rv->val);
		return;
#else
		fprintf(stderr, "error: cobra was compiled with NOFLOAT\n");
		goto error_case;
#endif
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
		fprintf(tfd, "%s", array_ix(q->rgt->s, (int) rv->val, ix));
		return;
	case SUBSTR:
		substr(ref_p, q, rv, ix);
		fprintf(tfd, "%s", rv->s);
		return;
	case SPLIT:
		split(ref_p, q, rv, ix);
		fprintf(tfd, "%s", rv->s);	// tab separated fields
		return;
	case GSUB:
		gsub(ref_p, q, rv, ix);
		fprintf(tfd, "%s", rv->s);
		return;
	case STRSTR:
		do_strstr(ref_p, q, rv, ix);
		fprintf(tfd, "%d", (int) rv->val);
		return;
	case STRRSTR:
		do_strrstr(ref_p, q, rv, ix);
		fprintf(tfd, "%d", (int) rv->val);
		return;
	case STRCMP:
		do_strcmp(ref_p, q, rv, ix);
		fprintf(tfd, "%d", (int) rv->val);
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
#ifdef C_FLOAT
		if (((int) rv->val) == rv->val)
		{	fprintf(tfd, "%d", (int) rv->val);
		} else
		{	fprintf(tfd, "%.2f", rv->val);	// adjust precision
		}
#else
		fprintf(tfd, "%d", rv->val);
#endif
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
		  {	int in_single = 0;
			int in_double = 0;
			// print \\ as is, but drop the \ in front of
			// other characters like \n or \t
			// unless enclosed in quotes
			for (s = rv->s; *s != '\0'; s++)
		  	{	if (s == rv->s
				||  *(s-1) != '\\')
				{	if (*s == '"')
					{	in_double = 1 - in_double;
					} else if (*s == '\'')
					{	in_single = 1 - in_single;
				}	}
				if (in_single
				||  in_double
				||  *s != '\\'
				||  *(s+1) == '\\')
				{	fprintf(tfd, "%c", *s);
					if (*s == '\\')
					{	s++;
					}
		  }	}	}
		} else
		{ fprintf(tfd, "%s", rv->s);
		}
		break;
	default:
		break; // cannot happen
	}
}

static int
format_string(Lextok *q)
{	char *s;

	while (q && q->typ == ARG)
	{	q = q->lft;
	}

	if (q
	&&  q->typ == STRING)
	for (s = q->s; *s != '\0'; s++)
	{	if (*s == '%')
		{	s++;
			while (isdigit((int) *s)
			   ||  *s == '.')
			{	s++;
			}
			if (*s == 'd'
			||  *s == 's'
			||  *s == 'f')
			{	return 1;
	}	}	}

	return 0;
}

typedef struct ArgLst ArgLst;
struct ArgLst {
	Lextok *e;
	ArgLst *nxt;
};
static ArgLst *argfrst;
static ArgLst *arglast;
static int argcnt;

static void
addargs(Lextok *q)
{	ArgLst *n;

	if (q->typ == ARG)
	{	addargs(q->lft);
		addargs(q->rgt);
	} else
	{	argcnt++;
		n = (ArgLst *) emalloc(sizeof(ArgLst), 150);
		n->e = q;
		if (arglast)
		{	arglast->nxt = n;
			arglast = n;
		} else
		{	argfrst = arglast = n;
	}	}
}
static void convert2string(Prim **, Lextok *, Rtype *, const int);

static void
print_format(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)
{	FILE *tfd = (track_fd) ? track_fd : stdout;
	ArgLst *w;
	char *s;
	int minwidth = 0;
	int precision = 0;
	char fms[64];

	arglast = argfrst = (ArgLst *) 0;
	argcnt = 0;
	addargs(q);

	assert(argcnt > 1 && argfrst->e && argfrst->e->typ == STRING);

	// if there are fewer arguments than format specifiers:
	//   the extra format specifiers are printed as is
	// formats recognized:
	//	%d, %s, %f
	//	%4d %4s %4f		minwidth of 4, padded on left with spaces
	//	%.4d %.4s %.4f		maxwidth of 4, for %f nr of decimals
	//	%5.4d %5.4s %5.4f	both

	s = argfrst->e->s;	// the format string
	w = argfrst->nxt;	// the first argument
	for ( ; *s != '\0'; s++)
	{	if (*s == '%' && w != NULL)
		{	minwidth = 0;
			precision = 0;
			 if (isdigit((int) *(s+1)))
			{	minwidth = atoi(s+1);
				while (isdigit((int) *(s+1)))
				{	s++;
			}	}
			if (*(s+1) == '.')
			{	s++;
				if (!isdigit((int) *(s+1)))
				{	fprintf(stderr, "error: format, unexpected '.'\n");
					show_error(stderr, q->lnr);
					return;
				}
				precision = atoi(s+1);
				while (isdigit((int) *(s+1)))
				{	s++;
			}	}
			if (minwidth)
			{	if (precision)
				{	sprintf(fms, "%%%d.%d", minwidth, precision);
				} else
				{	sprintf(fms, "%%%d", minwidth);
				}
			} else if (precision)
			{	sprintf(fms, "%%.%d", precision);
			} else
			{	strcpy(fms, "%");
			}
			switch (*(s+1)) {
			case 'd':
				strcat(fms, "d");
				eval_prog(ref_p, w->e, rv, ix);
				switch (rv->rtyp) {
				case VAL: fprintf(tfd, fms, (int) rv->val); break;
				case STR: fprintf(tfd, fms, (int) atoi(rv->s)); break;
				case PTR: fprintf(tfd, fms, (long) rv->ptr); break;
				default:  fprintf(tfd, fms, 0); break;
				}
				s++;
				w = w->nxt;
				argcnt--;
				continue;
			case 's':
				strcat(fms, "s");
				convert2string(ref_p, w->e, rv, ix);
				fprintf(tfd, fms, rv->s);
				s++;
				w = w->nxt;
				argcnt--;
				continue;
			case 'f':
				strcat(fms, "f");
				eval_prog(ref_p, w->e, rv, ix);
				switch (rv->rtyp) {
				case VAL: fprintf(tfd, fms, (float) rv->val); break;
				case STR: fprintf(tfd, fms, (float) atof(rv->s)); break;
				case PTR: fprintf(tfd, fms, (long) rv->ptr); break;
				default:  fprintf(tfd, fms, 0); break;
				}
				s++;
				w = w->nxt;
				argcnt--;
				continue;
			default:
				break;
		}	}
		if (*s != '\0')
		{	fprintf(tfd, "%c", *s);
		}
	}
	if (w)
	{	fprintf(stderr, "error: %d too many arguments for format\n", argcnt-1);
		show_error(stderr, q->lnr);
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
#ifdef C_FLOAT
		rv->val = atof(rv->s);
#else
		rv->val = atoi(rv->s);
#endif
	} else if (!rv->rtyp)
	{	rv->rtyp = VAL;
		rv->val  = 0;
	}
}

static void
val2str(Rtype *rv, const int ix)
{	int aw = 64;

	if (rv->rtyp == VAL)
	{	if (rv->val >= 0 && rv->val <= 9)
#ifdef C_FLOAT
		if (((int) rv->val) == rv->val)
#endif
		{       rv->s = nr_tbl[(int) rv->val];
			rv->rtyp = STR;
                        return;
                }
                if (rv->val >= 0)
                {       if (rv->val < 1000)
                        {       aw = 4;
                        } else if (rv->val < 10000000)
                        {       aw = 8;
                }       }
#ifdef C_FLOAT
		// was: if (has_lock[ix] == PRINT)
		if (((int) rv->val) == rv->val)
		{	rv->s = (char *) hmalloc(aw, ix, 135); // val2str
			snprintf(rv->s, aw, "%d", (int) rv->val);
		} else
		{	aw += 8;
			rv->s = (char *) hmalloc(aw, ix, 135);	// val2str
			snprintf(rv->s, aw, "%f", rv->val);
		}
#else
                rv->s = (char *) hmalloc(aw, ix, 135);  // val2str
                snprintf(rv->s, aw, "%d", rv->val);     // val2str
#endif
	} else if (!rv->rtyp)
	{	rv->s = "";
	}
	rv->rtyp = STR;
}

typedef struct R_S R_S;
struct R_S {
	char *s;
	R_S *nxt;
};

#define MAXSTR	4096
#define MINSTR	  16

R_S *r_str[MAXSTR];
R_S *r_free;

#ifdef RECYCLE_STR
static void
recycle_str(char *s, const int ix)
{	int n = strlen(s)+1;
	R_S *r;

	if (n >= MINSTR && n < MAXSTR)
	{	if (r_free != NULL)
		{	r = r_free;
			r_free = r_free->nxt;
		} else
		{	r = (R_S *) hmalloc(sizeof(R_S), ix, 150);
		}
		r->s = s;
		r->nxt = r_str[n];
		r_str[n] = r;
	}
}
#endif

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
	{	val2str(rv, ix);
	}

	Assert("plus1", tmp.rtyp == rv->rtyp, q->lft);
	Assert("plus2", tmp.rtyp == VAL || tmp.rtyp == STR, q->rgt);

	if (tmp.rtyp == VAL)
	{	rv->val = tmp.val + rv->val;
	} else if (tmp.rtyp == STR)
	{	int n = strlen(tmp.s) + strlen(rv->s) + 1;
		char *s;
		if (n < MAXSTR && r_str[n] != NULL)
		{	R_S *r_p = r_str[n];
			s = r_str[n]->s;
			assert(strlen(s) == n-1);
			r_str[n] = r_str[n]->nxt;

			r_p->nxt = r_free;
			r_free = r_p;
		} else
		{	s = (char *) hmalloc(n, ix, 134);	// plus
		}
		snprintf(s, n, "%s%s", tmp.s, rv->s);		// plus
#ifdef RECYCLE_STR
		// noticed that some of these strings pointed to
		// are not always constant
		if (strlen(rv->s) >= MINSTR)
		{	recycle_str(rv->s, ix);
		}
		if (strlen(tmp.s) >= MINSTR)
		{	recycle_str(tmp.s, ix);
		}
#endif
		rv->s = s;
	} else
	{	fprintf(stderr, "line %d: error: invalid addition attempt\n", q->lnr);
		rv->rtyp = STP;
		sep[ix].T_stop++;
	}
}

static void
Binexp(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)
{	Rtype tmp;
				
	eval_prog(ref_p, q->lft, &tmp, ix);
	if (tmp.rtyp == STP) { return; }
	str2val(&tmp);
	if (tmp.rtyp != VAL)
	{	printf("%d Binexp='	lft: ", q->lnr);
		tok2txt(q->lft, stdout);
		printf(" 	'**'	rgt: ");
		tok2txt(q->rgt, stdout);
	}
	Assert("Binexp", tmp.rtyp == VAL, q->lft);
	eval_prog(ref_p, q->rgt, rv, ix);
	if (rv->rtyp == STP) { return; }
	str2val(rv);
	Assert("Binexp", rv->rtyp == VAL, q->rgt);
	rv->val = (C_TYP) pow( (double) tmp.val, (double) rv->val);
}

// for | & << >> ^
#define fbinop(op)				\
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
	rv->val = ((int)tmp.val) op ((int)rv->val)

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

// for %
#define rfbinop(op)				\
	eval_prog(ref_p, q->lft, &tmp, ix);	\
	if (tmp.rtyp == STP) { break; }		\
	if (tmp.rtyp != VAL) { printf("rbinop='%s'\t", #op); } \
	Assert("rbinop1", tmp.rtyp == VAL, q->lft);	\
	eval_prog(ref_p, q->rgt, rv, ix);	\
	if (rv->rtyp == STP) { break; }		\
	Assert("rbinop2", rv->rtyp == VAL && rv->val != 0, q->rgt); \
	rv->val = ((int)tmp.val) op ((int)rv->val)

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
		rv->val = ~((int)(rv->val));
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

	if (LHS->typ != '.'
	|| !LHS->rgt
	|| LHS->rgt->typ != MARK
	|| RHS)
	{	if (LHS->typ == NAME)
		{	Var_nm *n;
			n = get_var(ref_p, LHS, rv, ix);
			if (n->rtyp == 0)	// first use
			{	n->rtyp = VAL;
			}
			if (n->rtyp == STR && isdigit((uchar) n->s[0]))
			{	n->rtyp = VAL;
#ifdef C_FLOAT
				n->v = atof(n->s);
#else
				n->v = atoi(n->s);
#endif
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
			fprintf(stderr, "line %d: error: invalid use of ++ or --, with lhs: ",
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
	{	int oval = rv->val;	// value to be assigned
		Var_nm *n = get_var(ref_p, LHS->lft, rv, ix);	// overwrites rv->val

		if (!n)
		{	rv->rtyp = STP;
			return;
		}

		if (!n->rtyp)
		{	rv->rtyp = n->rtyp = PTR; // from context
		}

		if (rv->rtyp == VAL)	// stores a value
		{	n->v = oval;	// was missing before, 8/12/24 was rv->val
			return;		// was missing before
		}

		if (rv->rtyp == PTR)
		{	Prim *z = n->pm;
			if (z)
			{	z->mark = oval;	// 8/12/24 was rv->val
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
	Prim *pp;
	Lextok *qq;
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

		if (sep[ix].Verbose>2)
		{
#ifdef C_FLOAT
			fprintf(stderr, "type %d val %f %s[%s]\n",
				rv->rtyp, rv->val, LHS->lft->s, tmp.s);
#else
			fprintf(stderr, "type %d val %d %s[%s]\n",
				rv->rtyp, rv->val, LHS->lft->s, tmp.s);
#endif
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
	{	if (LHS->rgt->typ == TXT
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

		pp = p;
		qq = LHS->rgt;

		if (LHS->typ == '.'
		&&  LHS->lft
		&&  LHS->lft->typ == NAME)	// use the token pointed to by LHS->lft
		{	Var_nm *vn;
			vn = get_var(ref_p, LHS->lft, rv, ix);
			if (!vn)
			{	fprintf(stderr, "error: no such variable\n");
				goto bad;
			}
			if (vn->rtyp != PTR)
			{	fprintf(stderr, "error: wrong variable type of %s (saw %d, expecting token ref %d)\n",
					vn->nm, vn->rtyp, PTR);
				goto bad;
			}
			pp = (Prim *) vn->pm;
		}

more:
		if (qq->typ == '.' && pp)
		{	if (qq->lft)
			{	switch (qq->lft->typ) {
				case JMP:	pp = pp->jmp; break;
				case BOUND:	pp = pp->bound; break;
				case NXT:	pp = pp->nxt; break;
				case PRV:	pp = pp->prv; break;
				default:
bad:					fprintf(stderr, "error: bad dot chain on lhs of asgn\n");
					show_error(stderr, qq->lnr);
					rv->rtyp = STP;
					return;
			}	}

			if (!pp)
			{	fprintf(stderr, "error: pp\n");
				goto bad;
			}

			if (qq->rgt->typ == '.')
			{	qq = qq->rgt;
				goto more;
			}

			eval_prog(ref_p, RHS, rv, ix);

			switch (qq->rgt->typ) {
			case FNM: case TYP: case TXT:
				if (rv->rtyp != STR)
				{	fprintf(stderr, "error: type of rhs should be string\n");
					goto bad;
				}
				break;

			case LNR:   case SEQ:   case MARK: case LEN:
			case CURLY: case ROUND: case BRACKET:
				if (rv->rtyp != VAL)
				{	fprintf(stderr, "error: type of rhs should be integer\n");
					goto bad;
				}
				break;

			case JMP: case BOUND: case NXT: case PRV:
				if (rv->rtyp != PTR)
				{	fprintf(stderr, "error: type of rhs should be token ref\n");
					goto bad;
				}
				break;
			}

			switch (qq->rgt->typ) {
			case FNM:	pp->fnm = rv->s; break;
			case TYP:	pp->typ = rv->s; break;
			case TXT:	pp->txt = rv->s; break;

			case LNR:	pp->lnr = rv->val; break;
			case SEQ:	pp->seq = rv->val; break;
			case MARK:	pp->mark = rv->val; break;
			case CURLY:	pp->curly = rv->val; break;
			case ROUND:	pp->round = rv->val; break;
			case BRACKET:	pp->bracket = rv->val; break;
			case LEN:	pp->len = rv->val; break;

			case JMP:	pp->jmp = rv->ptr; break;
			case BOUND:	pp->bound = rv->ptr; break;
			case NXT:	pp->nxt = rv->ptr; break;
			case PRV:	pp->prv = rv->ptr; break;

			default:	fprintf(stderr, "error: unexpected type %d\n", qq->rgt->typ);
					goto bad;
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
#ifdef C_FLOAT
			rv->val = atof(rv->s);
#else
			rv->val = atoi(rv->s);
#endif
		} else if (rv->rtyp != VAL)
		{	printf("token: '%s'\n", rv->s);
		}
		Assert("rhs = (2)", rv->rtyp == VAL, RHS);
		// rhs is VAL

		if (LHS->lft)	// get the q in q.mark = ...
		{	Var_nm *n = get_var(ref_p, LHS->lft, &tmp, ix);

			if (!n)
			{	rv->rtyp = STP;
				return;
			}
			if (!n->rtyp)			// new variable
			{	n->rtyp = PTR;		// its the q in q.mark
			}
			if (!n->pm)
			{	n->pm = (Prim *) hmalloc(sizeof(Prim), ix, 135); // do_assignment
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

void
get_field(Prim *p, Lextok *q, Rtype *rv, int ix)
{
	assert(q != NULL);

	if (q->typ == '.'
	&&  q->lft == NULL
	&&  q->rgt != NULL)
	{	q = q->rgt;
	}

	switch (q->typ) {
	case    FNM: rv->rtyp = STR; rv->s   = p?p->fnm:"";  break;
	case    TYP: rv->rtyp = STR; rv->s   = p?p->typ:"";  break;
	case    TXT: rv->rtyp = STR; rv->s   = p?p->txt:"";  break;

	case	 LNR: rv->rtyp = VAL; rv->val = p?p->lnr:0; break;
	case     SEQ: rv->rtyp = VAL; rv->val = p?p->seq:0; break;
	case    MARK: rv->rtyp = VAL; rv->val = p?p->mark:0; break;

	case   CURLY: rv->rtyp = VAL; rv->val = p?p->curly:0; break;
	case   ROUND: rv->rtyp = VAL; rv->val = p?p->round:0; break;
	case BRACKET: rv->rtyp = VAL; rv->val = p?p->bracket:0; break;
	case     LEN: rv->rtyp = VAL; rv->val = p?p->len:0; break;

	case    FCT: rv->rtyp = STR; rv->s   = fct_which(p); break;

	case    JMP: rv->rtyp = PTR; rv->ptr = p?p->jmp:p;  break;
	case  BOUND: rv->rtyp = PTR; rv->ptr = p?p->bound:p; break;
	case MBND_D: rv->rtyp = PTR; rv->ptr = p?p->mbnd[0]:p; break;
	case MBND_R: rv->rtyp = PTR; rv->ptr = p?p->mbnd[1]:p; break;
	case    NXT: rv->rtyp = PTR; rv->ptr = p?p->nxt:p;   break;
	case    PRV: rv->rtyp = PTR; rv->ptr = p?p->prv:p;   break;

	default:
		     rv->rtyp = STP;
		     fprintf(stderr, "error: bad dot chain, saw type %d (%p)\n", q->typ, (void *) q->lft);
		     show_error(stderr, q->lnr);
		     break;
	}
}

static void
do_dot(Prim **ref_p, Lextok *q, Rtype *rv, const int ix, Prim *p)
{
	if (!q->lft && !q->rgt)
	{	rv->rtyp = PTR;
		rv->ptr = p;
	} else if (!q->lft)
	{	eval_prog(ref_p, q->rgt, rv, ix);
	} else // q->lft	e.g.:	q.txt
	{	Var_nm *n = 0, dummy;
		assert(ix >= 0 && ix < Ncore);

		if (q->rgt->typ == '.') // chain
		{	if (q->lft->typ != NAME)
			{	fprintf(stderr, "error: bad dot-chain, saw type %d\n", q->lft->typ);
				rv->rtyp = STP;
				return;
			}
			n = get_var(ref_p, q->lft, rv, ix);
			if (n->rtyp != PTR)
			{	if (0)
				{	fprintf(stderr, "%s returns non-ptr, type %d\n",
						q->lft->s, n->rtyp);
				}
fallback:			rv->rtyp = VAL;
				rv->val = 0;
				return;
			}
			p = (Prim *) n->pm;

			// evaluates to a field of p, find which field
			q = q->rgt;	// the '.'
			if (!q->rgt)
			{	fprintf(stderr, "error: cannot happen: no field spec\n");
				rv->rtyp = STP;
				return;
			}

again:			if (q->typ == '.')
			{	if (!q->lft)
				{	get_field(p, q->rgt, rv, ix);
					return;
				}
				get_field(p, q->lft, rv, ix);
				if (rv->rtyp != PTR)
				{	if (0)
					{	fprintf(stderr, "%s returns non-ptr (typ %d)\n",
							q->lft->s, rv->rtyp);
					}
					goto fallback;
				}
				p = rv->ptr;
				// check which field of p, given by p->rgt
				q = q->rgt;
				if (q->typ != '.')
				{	get_field(p, q, rv, ix);
					return;
				}
				goto again;
			}
			get_field(p, q, rv, ix);
			return;
		} else
		{	switch (q->lft->typ) {
			case END:
			case BEGIN:
			case FIRST_T:
			case LAST_T:
				eval_prog(ref_p, q->lft, rv, ix);
				Assert("dodot", rv->rtyp == PTR, q->lft);
				n = &dummy;
				n->pm = rv->ptr;
				break;
			case '.':
				eval_prog(ref_p, q->lft, rv, ix);
				if (rv->rtyp == PTR)
				{	n = &dummy;
					n->pm = rv->ptr;
				}
				break;
			default:
				n = get_var(ref_p, q->lft, rv, ix);
				break;
		}	}

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

static void
convert2string(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)
{
	assert(q->typ != ',');
	eval_prog(ref_p, q, rv, ix);

	switch (rv->rtyp) {
	case STR:
		return;
	case PTR:
		rv->val = rv->ptr->seq;	// was ->mark before
		rv->rtyp = VAL;
		// fall thru
	case VAL:
		val2str(rv, ix);
		return;

	case STP:
	case PRCD:
	default:
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

static int ncommas;

static void
count_commas(Lextok *p)
{
	if (!p)
	{	return;
	}
	if (p->typ == ',')
	{	ncommas++;
	}
	count_commas(p->rgt);
	count_commas(p->lft);
}

static int
bad_param_counts(const char *s, Lextok *f, Lextok *a, int ix)
{	int nf, na;

	lock_print(ix);
	 ncommas = 0;
	 count_commas(f);
	 nf = ncommas;
	 ncommas = 0;
	 count_commas(a);
	 na = ncommas;
	unlock_print(ix);

	if (na != nf)
	{	fprintf(stderr, "error: too %s parameters in fct call of %s()\n",
			(na>nf)?"many":"few", s);
		show_error(stderr, a->lnr);
#ifdef DEBUG
		fprintf(stderr, "%d---f\n", nf);
		dump_tree(f, 0); // -DDEBUG
		fprintf(stderr, "%d---a\n", na);
		dump_tree(a, 0);
		exit(0);
#endif
	}
	return (na != nf);
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
giveup:		sep[ix].T_stop++;
		return;
	}

	if (c_free[ix])
	{	cframe = c_free[ix];
		c_free[ix] = c_free[ix]->nxt;
	} else
	{	cframe = (Cstack *) hmalloc(sizeof(Cstack), ix, 139);
	}

	if (bad_param_counts(s, formals, actuals, ix))
	{	goto giveup;
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

	do_lock(cid);	// re_matches (cobra_prog.y)

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
					{
#ifdef C_FLOAT
						j = atof(n->s);
#else
						j = atoi(n->s);
#endif
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
	{	fprintf(stderr, "error: '%s' bad token ref, type %d\n",
			from->s, rv->rtyp);
	}
}

static Prim *
handle_arg(Prim **ref_p, Lextok *from, Rtype *rv, const int ix)
{	Var_nm *n;

	if (!from)
	{	fprintf(stderr, "error: add/del_pattern (internal error)\n");
		return NULL;
	}

	if (from->typ != NAME)	// new 3/20/2024
	{	eval_prog(ref_p, from, rv, ix);
		if (rv->rtyp == PTR)
		{	return rv->ptr;
		}
		fprintf(stderr, "error: bad add/del_pattern call\n");
		return NULL;
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

static int
get_cumulative(Prim **ref_p, Lextok *q, Rtype *rv, int with_start, const int ix)
{	int cntr, nb = 0;
	ulong cumulative;
	int na, sv = 0;	// default start value

	// 0: retrieve array size and the optional start index

	na = array_sz(q->lft->s, ix);
	if (na <= 0)
	{	return 1;	// not an array
	}

	if (with_start)
	{	eval_prog(ref_p, q->rgt, rv, ix);
		if (rv->rtyp == STR && isdigit((int) (rv->s[0])))
		{	rv->val = atoi(rv->s); // must be int
		} else if (rv->rtyp != VAL)
		{	return 2;	// not start value given
		}
		sv = rv->val;

		// 1: find the starting index in the array
#if 1
		nb = na-sv-1;
		rv->s = array_ix(q->lft->s, nb, ix);
		rv->rtyp = STR;
#else
		for (cntr = 0, nb = na-1; cntr < na; cntr++, nb--)
		{	rv->s = array_ix(q->lft->s, nb, ix);
			if (atoi(rv->s) == sv)	// must be int
			{	break;	// found
		}	}
		if (cntr == na)	// not found
		{	printf("error: index %d not found in array %s\n", sv, q->lft->s);
			return 3;
		}
#endif
		assert(nb >= 0 && nb < na);
	}

	// printf("'%s' is an array of size: %d -- start @ %d found at %d\n", q->lft->s, na, sv, nb);

	// 2: compute the cumulative hash over the array elements
	//    backwards from nb, modulo na

	eval_aname(ref_p, q, rv, ix); 		// retrieve string stored
	assert(rv->rtyp == STR);
	cumulative = hash2_cum(rv->s, 0);	// initialize and first call

	for (cntr = 1; cntr < na; cntr++)
	{	nb = (nb == 0)? na-1 : nb-1;
		rv->s = array_ix(q->lft->s, nb, ix);
		rv->rtyp = STR;
		eval_aname(ref_p, q, rv, ix); // retrieve string stored
		assert(rv->rtyp == STR);
		cumulative = hash2_cum(rv->s, cntr);
	}
	rv->val = cumulative;
	rv->rtyp = VAL;

	return 0;
}

static void
name_to_set(Prim **ref_p, Lextok *q, Rtype *rv, int ix)
{	Var_nm *v;
	Lextok *n = q->lft;

	switch (n->typ) {
	case STRING:
		rv->ptr = cp_pset(n->s, q->rgt, ix);
		rv->rtyp = PTR;
		break;
	case NAME:
		rv->ptr = cp_pset(n->s, q->rgt, ix);
		if (rv->ptr == NULL)	// could be a var
		{	v = get_var(ref_p, n, rv, ix);
			if (rv->rtyp == STR)
			{	rv->ptr = cp_pset(v->s, q->rgt, ix);
			}
			if (verbose && !rv->ptr)
			{	fprintf(stderr, "warning: no such set '%s'\n", (v && v->s)?v->s:n->s);
		}	}
		rv->rtyp = PTR;
		break;
	default:
		fprintf(stderr, "error: pset arg is not a name, variable, or string'\n");
		rv->ptr = NULL;
		rv->rtyp = STP;

		show_error(stderr, q->lnr);
		unwind_stack(ix);
		sep[ix].T_stop++; 
	}
}

void
set_string_type(Lextok *c, char *s)
{
	if (c)
	{	c->typ = STRING;
		c->s = s;
	}
}

extern int evaluate(const Prim *, const Lextok *);

enum tok_eval {		// cobra_eval.c
	eSIZE = 258,
	eNR = 259,
	eNAME = 260,
	eOR = 263,
	eAND = 264,
	eEQ = 265,
	eNE = 266,
	eGT = 267,
	eLT = 268,
	eGE = 269,
	eLE = 270,
	eUMIN = 271
};

int
tok_prog2eval(int t)
{
	switch (t) {
	case SIZE:	t = eSIZE; break;
	case NR:	t = eNR; break;
	case NAME:	t = eNAME; break;
	case OR:	t = eOR; break;
	case AND:	t = eAND; break;
	case EQ:	t = eEQ; break;
	case NE:	t = eNE; break;
	case GT:	t = eGT; break;
	case LT:	t = eLT; break;
	case GE:	t = eGE; break;
	case LE:	t = eLE; break;
	}
	return t;
}

static void
convert_prog2eval(Lextok *p)
{
	if (!p)
	{	return;
	}
	p->typ = tok_prog2eval(p->typ);
	convert_prog2eval(p->lft);
	convert_prog2eval(p->rgt);
}

void
slice_set(Named *x, Lextok *constraint, int ix, int commandline)
{	Prim *p, *last_p = (Prim *) 0;

	if (!constraint
	||  !x
	||  !x->cloned)
	{	return;
	}

	for (p = x->cloned; p; p = p->nxt)
	{	Rtype irv;

		if (constraint->typ == STRING)
		{	Prim *s = p->jmp;
			Prim *e = p->bound;
			irv.rtyp = VAL;
			irv.val = 0;
			while (s->seq <= e->seq)
			{	if (strcmp(s->txt, constraint->s) == 0)
				{	irv.val = 1;	// keep
					break;
				}
				s = s->nxt;
			}
		} else
		{	if (commandline)
			{	convert_prog2eval(constraint);		// ps = A with (constraint)
				irv.val = evaluate(p, constraint);
				irv.rtyp = VAL;
			} else	// inline program
			{	eval_prog(&p, constraint, &irv, ix);	// pset(A) with (constraint)
		}	}

		if (irv.rtyp == STP
		||  irv.rtyp != VAL)
		{	fprintf(stderr, "pset %s: bad constraint\n", x->nm);
			break;
		}
		if (irv.val == 0)	// delete from list
		{	if (!last_p)
			{	x->cloned = x->cloned->nxt;
				if (x->cloned)
				{	x->cloned->prv = (Prim *) 0;
				}
			} else
			{	last_p->nxt = p->nxt;
				if (p->nxt)
				{	p->nxt->prv = last_p;
			}	}
			continue; // dont change last_p
		}
		last_p = p;
	}
}

void
convert_list2set(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)
{	Var_nm *lst;
	Prim *nm;

	if (!q
	||  !q->lft
	||  !q->rgt
	||  q->rgt->typ != STRING
	||  q->lft->typ != NAME)
	{	fprintf(stderr, "error: usage list2set listname \"setname\"\n");
		goto out;
	}

	lst = get_var(ref_p, q->lft, rv, ix);

	if (!lst
	||  !lst->pm
	||  lst->rtyp != PTR)
	{	fprintf(stderr, "error: no list '%s'\n", q->lft->s);
out:		if (q)
		{	show_error(stderr, q->lnr);
		}
		rv->rtyp = STOP;
		return;
	}

	for (nm = lst->pm; nm; nm = nm->nxt)
	{	add_pattern(q->rgt->s, "list2set", nm->jmp, nm->bound, ix);
	}
}

void
dot_chain(Prim *p, Lextok *q, Rtype *rv, int ix)
{	Prim *pp = (Prim *) 0;

	assert(q->typ == '.' && q->rgt);

	if (q->lft)
	{	switch (q->lft->typ) {	// must be JMP, BOUND, NXT, or PRV
		case    JMP: pp = p?p->jmp:p;  break;
		case  BOUND: pp = p?p->bound:p; break;
		case    NXT: pp = p?p->nxt:p;   break;
		case    PRV: pp = p?p->prv:p;   break;
		default:
			fprintf(stderr, "error: bad dot chain\n");
			show_error(stderr, q->lnr);
			unwind_stack(ix);
			sep[ix].T_stop++; 
			rv->rtyp = STP;
			return;
		}
	
		p = pp;
		if (p && q->rgt->typ == '.')
		{	dot_chain(p, q->rgt, rv, ix);
			return;
		}
	}
	get_field(p, q->rgt, rv, ix);
}

void
eval_prog(Prim **ref_p, Lextok *q, Rtype *rv, const int ix)
{	Prim  *p;
	Rtype tmp;

	memset(&tmp, 0, sizeof(Rtype));
	assert(ix >= 0 && ix < Ncore);
	assert(sep != NULL);
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
	{	lock_other(ix);
		fprintf(stderr, "%d: [seq %d ln %d] EVAL_PROG typ %3d ",
			ix, p->seq, p->lnr, q->typ);
		tok2txt(q, stdout);
		fflush(stdout);
		unlock_other(ix);
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
		rv->val = nr_marks_int((int) rv->val, ix);
		break;

	case INT:
		eval_prog(ref_p, q->lft, rv, ix);	
		Assert("int", rv->rtyp == STR, q->lft);
		sprintf(rv->s, "%d", (int) round(atof(rv->s)));
		break;
#ifdef C_FLOAT
	case FLOAT:
		eval_prog(ref_p, q->lft, rv, ix);	
		Assert("tofloat", rv->rtyp == STR, q->lft);
		sprintf(rv->s, "%f", atof(rv->s));
		break;
#endif

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
		rv->s = array_ix(q->rgt->s, (int) rv->val, ix);
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

	case LIST2SET:
		convert_list2set(ref_p, q, rv, ix);
		break;

	case SPLIT:
		split(ref_p, q, rv, ix);
		break;

	case GSUB:
		gsub(ref_p, q, rv, ix);
		break;

	case STRSTR:
		do_strstr(ref_p, q, rv, ix);
		break;

	case STRRSTR:
		do_strrstr(ref_p, q, rv, ix);
		break;

	case STRCMP:
		do_strcmp(ref_p, q, rv, ix);
		break;

	case STRLEN:
		eval_prog(ref_p, q->lft, rv, ix);
		Assert("strlen", rv->rtyp == STR, q->lft);
		rv->rtyp = VAL;
		rv->val  = strlen(rv->s);
		break;

	case OPEN:	// fp = open "fnm" [rwa]
		{ char *str;
		  if (!q->lft || !q->rgt)
		  {	fprintf(stderr, "error: missing arg to open\n");
			goto error_case;
		  }
		  if (q->rgt->typ != STRING)
		  {	eval_prog(ref_p, q->rgt, rv, ix);
			if (rv->rtyp != STR)
			{	fprintf(stderr, "error: args to 'open' should be strings\n");
				goto error_case;
			}
			str = rv->s;
		  } else
		  {	str = q->rgt->s;
		  }
		  if ((q->lft->typ != NAME
		  &&  q->lft->typ != STRING)
		  || (strcmp(q->lft->s, "r") != 0
		  &&  strcmp(q->lft->s, "w") != 0
		  &&  strcmp(q->lft->s, "a") != 0))
		  {	fprintf(stderr, "error: the second arg to 'open' should be r, w, or a\n");
			goto error_case;
		  }
		  rv->rtyp = PTR;
		  rv->ptr = (Prim *) fopen((const char *) str, (const char *) q->lft->s);
		  if (rv->ptr == NULL)
		  {	fprintf(stderr, "error: 'open %s %s' failed\n", str, q->lft->s);
			goto error_case;
		  }
		}
		break;

	case CLOSE:
		if (!q->rgt
		||   q->rgt->typ != NAME)
		{	fprintf(stderr, "error: arg to 'close' must be a varname\n");
			goto error_case;
		}
		{ Var_nm *vn = get_var(ref_p, q->rgt, rv, ix);
		  Assert("close", vn != NULL, q->rgt);
		  rv->rtyp = VAL;
		  rv->val = fclose((FILE *) vn->pm);
		  if (rv->val != 0)
		  {	fprintf(stderr, "error: close %s failed\n", q->rgt->s);
			goto error_case;
		}
		}
		break;

	case UNLINK:
		if (!q->lft)
		{	fprintf(stderr, "error: arg to 'unlink' missing\n");
			goto error_case;
		}
		eval_prog(ref_p, q->lft, rv, ix);
		if (rv->rtyp != STR)
		{	fprintf(stderr, "error: arg to 'unlink' must be a string\n");
			goto error_case;
		}
		rv->rtyp = VAL;
		rv->val = unlink((const char *) rv->s);
		break;

	case EXEC:
		if (!q->lft)
		{	fprintf(stderr, "error: arg to 'exec' missing\n");
			goto error_case;
		}
		eval_prog(ref_p, q->lft, rv, ix);
		if (rv->rtyp != STR)
		{	fprintf(stderr, "error: arg to 'exec' must be a string\n");
			goto error_case;
		}
		rv->rtyp = VAL;
		rv->val = system((const char *) rv->s);
		if (rv->val < 0)
		{	fprintf(stderr, "error: exec '%s' failed\n", rv->s);
			goto error_case;
		}
		break;

	case PUTS:
		if (!q->rgt
		||  !q->lft
		||   q->rgt->typ != NAME)
		{	fprintf(stderr, "error: bad syntax 'puts' command\n");
			goto error_case;
		}
		{ Var_nm *vn = get_var(ref_p, q->rgt, rv, ix);
		  if (vn == NULL || vn->rtyp != PTR)
		  {	fprintf(stderr, "error: puts bad file descriptor\n");
			goto error_case;
		  }
		  Assert("puts", has_lock[ix] == 0, q);
		  lock_print(ix);
		  has_lock[ix] = PUTS;
		   FILE *ofd;
		   ofd = track_fd;
		   track_fd = (FILE *) vn->pm;
		   if (format_string(q->lft))
		   {	print_format(ref_p, q->lft, rv, ix);
		   } else
		   {	print_args(ref_p, q->lft, rv, ix);
		   }
		   track_fd = ofd;
		  has_lock[ix] = 0;
		  unlock_print(ix);
		}
		break;

	case GETS:
		if (!q->rgt
		||  !q->lft
		||   q->rgt->typ != NAME
		||   q->lft->typ != NAME)
		{	fprintf(stderr, "error: bad syntax 'gets' command\n");
			goto error_case;
		}
		{ Var_nm *vn1 = get_var(ref_p, q->rgt, rv, ix);
		  Assert("gets", vn1 != NULL, q->rgt);
		  Var_nm *vn2 = get_var(ref_p, q->lft, rv, ix);
		  Assert("gets", vn2 != NULL, q->lft);
		  if (strlen(vn2->s) < 512)
		  {	vn2->s = (char *) hmalloc(512, ix, 144);
		  }
		  vn2->rtyp = STR;
		  char *tptr = fgets(vn2->s, 512, (FILE *) vn1->pm);
		  if (tptr == NULL)
		  {	strcpy(vn2->s, "EOF");		
		  } else
		  {	int nlen = strlen(vn2->s);
		  	if (vn2->s[nlen-1] == '\n'
			||  vn2->s[nlen-1] == '\r')
			{	vn2->s[nlen-1] = '\0';
		  }	}
		  rv->rtyp = VAL;
		  rv->val = 0;
		}
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

	case ITOSTR:
		convert2string(ref_p, q->lft, rv, ix);
		break;

	case ATOI:
		eval_prog(ref_p, q->lft, rv, ix);
		if (rv->rtyp != VAL)
		{	Assert("atoi", rv->rtyp == STR, q->lft);
			rv->rtyp = VAL;
#ifdef C_FLOAT
			rv->val = atof(rv->s);
#else
			rv->val = atoi(rv->s);
#endif
		}
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
#ifdef C_FLOAT
 #define Binop		fbinop
 #define rBinop		rfbinop
#else
 #define Binop		binop
 #define rBinop		rbinop
#endif
	case  B_OR: Binop(|); break;
	case B_AND: Binop(&); break;

	case EXP: Binexp(ref_p, q, rv, ix); break;

	case LSH: Binop(<<); break;
	case RSH: Binop(>>); break;

	case B_XOR: Binop(^); break;

	case  GT: binop(>);  break;
	case  LT: binop(<);  break;
	case  GE: binop(>=); break;
	case  LE: binop(<=); break;
	case '+': plus(ref_p, q, rv, ix); break;
	case '-': binop(-); break;
	case '*': binop(*);  break;
	case '/': rbinop(/);  break;
	case '%': rBinop(%);  break;
	case UMIN: unop(-);  break;
	case '!':  unop(!);  break;

	case '~': match_anywhere(ref_p, q, rv, ix, p); break;
	case '^': match_at_start(ref_p, q, rv, ix, p); break;

	case	RANGE:
		  if (p
		  &&  p->jmp
		  &&  p->bound
		  &&  p->bound->seq >= p->jmp->seq
		  &&  p->jmp->seq > 0
		  &&  strcmp(p->jmp->fnm, p->bound->fnm) == 0
		  &&  strcmp(p->txt, "Pattern") == 0) // start of pattern
		  {	rv->val = p->bound->lnr - p->jmp->lnr;
			break;
		  }
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
			if (q->typ == '.') // chain on the current token p
			{	dot_chain(p, q, rv, ix); // Prim *p, dotexpr q, sets rv
				break;
			}
			goto next; // avoid recursion
		 }
		 do_dot(ref_p, q, rv, ix, p);
		 break;

	case PRINT:
		Assert("puts", has_lock[ix] != PRINT, q);
		if (has_lock[ix] != PUTS) // we dont already own the lock w puts
		{	lock_print(ix);	  // dont call lock twice in same proc
			has_lock[ix] = PRINT;
		}	// puts calls print_args, but print_args doesnt call puts

		if (format_string(q->lft))
		{	print_format(ref_p, q->lft, rv, ix);
		} else
		{	print_args(ref_p, q->lft, rv, ix);
		}
		fflush(stdout);
		if (has_lock[ix] != PUTS)
		{	has_lock[ix] = 0;
			unlock_print(ix);
		}
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

	case    FCT: case     FNM: case     TYP:
	case    TXT: case     JMP: case   BOUND:
	case MBND_D: case  MBND_R: case     NXT:
	case    PRV: case   ROUND: case BRACKET:
	case   CURLY: case    LEN: case     LNR:
	case    MARK: case    SEQ:
		get_field(p, q, rv, ix);
		break;

	case STRING: rv->rtyp = STR; rv->s   = q->s; break;

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
			if (q->rgt && q->rgt->typ == GOTO
			&&  q->a && q->a->typ == NAME)
			{
				q->a = find_label(q->a->s); // 3/15/2024
			}
			q = q->a;
			goto next;
		} else if (q->b)	// else part
		{	if (sep[ix].P_debug == 2)
			{	doindent();
				printf("Else\n");
			}
			if (q->rgt && q->rgt->typ == GOTO
			&&  q->b && q->b->typ == NAME)
			{
				q->b = find_label(q->b->s); // 3/15/2024
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
			{	char *tmp_nm = "unknown";
				if (q->lft->typ == STRING)
				{	tmp_nm = q->lft->s;
				} else if (q->lft->typ == NAME)
				{	tmp_nm = q->lft->s;
					if (!is_pset(q->lft->s))
					{	Var_nm *n = get_var(ref_p, q->lft, rv, ix);
						if (rv->rtyp == STR)
						{	tmp_nm = n->s;
						}
					}
				} else
				{	goto error_case;
				}
				if (verbose)
				{	printf("%s_pattern to set '%s' from %s:%d upto %s:%d\n",
						(q->typ == ADD_PATTERN) ? "add" : "del",
						tmp_nm, f->fnm, f->lnr, u->fnm, u->lnr);
				}
				if (q->typ == ADD_PATTERN)
				{	add_pattern(tmp_nm, 0, f, u, ix);
				} else
				{	del_pattern(tmp_nm, f, u, ix);
				}
				break;
			}
		}
	error_case:
		show_error(stderr, q->lnr);
		unwind_stack(ix);
		sep[ix].T_stop++; 
		rv->rtyp = STP;
		break;
	case CP_PSET:
		name_to_set(ref_p, q, rv, ix);	// sets rv->ptr and rv->rtyp
		break;

	case HASHARRAY:
		if (q->lft->typ == NAME)
		{	// associative array of na elements
			// return incremental hash over all string elements stored
			// starting at index rv->val

			if (get_cumulative(ref_p, q, rv, 1, ix) == 0)
			{	break;
		}	} // else, an error code was returned and reported

		fprintf(stderr, "error: hash arg %s is not an array of strings\n", q->lft->s);
		show_error(stderr, q->lnr);
		unwind_stack(ix);
		sep[ix].T_stop++; 
		rv->rtyp = STP;	
		break;

	case HASH:	// V 4.5
		eval_prog(ref_p, q->lft, rv, ix);
		if (rv->rtyp == STR)
		{	ulong h1 = hash3(rv->s);
			rv->val = h1;
			rv->rtyp = VAL;
			break;
		}
		if (q->lft->typ == NAME)
		{	Var_nm *n = get_var(ref_p, q->lft, rv, ix);
			if (rv->rtyp == STR)
			{	ulong h1 = hash3(n->s);
				rv->val = h1;
				rv->rtyp = VAL;
				break;
			} // else, it is not a scalar var
			if (get_cumulative(ref_p, q, rv, 0, ix) == 0)
			{	break; // default start value 0
		}	}

		fprintf(stderr, "error: hash arg %s does not evaluate to a string'\n", q->lft->s);
		show_error(stderr, q->lnr);
		unwind_stack(ix);
		sep[ix].T_stop++; 
		rv->rtyp = STP;	
		break;

	case IS_PATTERN:	// Kenneth McLarney
		rv->rtyp = VAL;
		if (q->lft->typ == STRING)
		{	rv->val = setexists(q->lft->s);
		} else if (q->lft->typ == NAME)
		{	rv->val = setexists(q->lft->s);
			if (rv->val == 0)	// check if var
			{	Var_nm *n = get_var(ref_p, q->lft, rv, ix);
				if (rv->rtyp == STR)
				{	rv->val = setexists(n->s);
				} else
				{	rv->val = 0;
				}
				rv->rtyp = VAL; // restore
			}
		} else
		{	fprintf(stderr, "error: arg is not a var, name, or string'\n");
			show_error(stderr, q->lnr);
			unwind_stack(ix);
			sep[ix].T_stop++; 
			rv->rtyp = STP;
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
//			dump_tree(q->lft, 0);
			if (0) fprintf(stderr, "%d: [seq %d ln %d] EVAL_PROG typ %3d ",
				ix, p->seq, p->lnr, q->typ);

			show_error(stderr, q->lnr);
			unwind_stack(ix);
			sep[ix].T_stop++; 
			rv->rtyp = STP;
			break;
		}
		break;

	case SRC_NM:
	case SRC_LN:	// V 4.5
		{	int from_nr, upto_nr;
			char *from_fnm;
			extern int unnumbered; // cobra_json.c

			Assert("src_ln0", q->lft && q->lft->lft && q->lft->rgt && q->rgt, q);

			eval_prog(ref_p, q->rgt, rv, ix);	// upto
			Assert("src_ln3", rv->rtyp == VAL, q->rgt);
			upto_nr = rv->val;

			eval_prog(ref_p, q->lft->rgt, rv, ix);	// from
			Assert("src_ln2", rv->rtyp == VAL, q->lft->rgt);
			from_nr = rv->val;
			Assert("src_ln2", from_nr <= upto_nr, q->lft->rgt);

			eval_prog(ref_p, q->lft->lft, rv, ix);	// fnm
			Assert("src_ln1", rv->rtyp == STR, q->lft->lft);
			from_fnm = rv->s;
			if (q->typ == SRC_NM) // numbered
			{	show_line(stdout, from_fnm, -1, from_nr, upto_nr, 0);
			} else
			{	unnumbered = 1;
				show_line(stdout, from_fnm, -1, from_nr, upto_nr, 0);
				unnumbered = 0;
			}
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
			if (sep[ix].T_stop != 0)
			{	rv->rtyp = STP;
			} else
			{	q = q->a;
				goto next;
		}	}
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
		do_lock(ix);	// lock()
		break;
	case UNLOCK:
		do_unlock(ix);
		break;

	case A_UNIFY:
		{ Lextok zz, yy;
		  eval_prog(ref_p, q->lft, rv, ix);
		  Assert("a_unify", rv->rtyp == VAL, q->lft);
		  zz.lft = &yy;
		  zz.rgt = q->rgt;
		  yy.val = rv->val;
		  yy.typ = NR;
		  array_unify(&zz, ix);
		}
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
		case OPEN:
		case CLOSE:
		case GETS:
		case PUTS:
		case UNLINK:
		case EXEC:
		case PRINT:
		case BREAK:
		case CONTINUE:
		case GLOBAL:
		case LIST2SET:
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
		case SRC_LN:
		case SRC_NM:

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
#ifdef C_FLOAT
		printf("' val %f --> str: %s\n",
			rv->val, (rv->rtyp == STR)?rv->s:"");
#else
		printf("' val %d --> str: %s\n",
			rv->val, (rv->rtyp == STR)?rv->s:"");
#endif
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
	static int nmax = 0;
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
	if (!has_lock)
	{	has_lock = (int *) emalloc(NCORE * sizeof(int), 84);
	}

	if (!t_stop)
	{	t_stop = (int *) emalloc(NCORE * sizeof(int), 84);
	}
	if (!sep || nmax < NCORE)
	{	nmax = NCORE;
		my_tree = (Lextok **) emalloc(NCORE * sizeof(Lextok *), 84);
		sep = (Separate *) emalloc(NCORE * sizeof(Separate), 84);
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
static Lextok *
dup_tree(Lextok *p, int ix)
{	Lextok *n;

	// tbd: doesnt work, suspect jumps
	// arent correctly preserved
	if (!p)
	{	return NULL;
	}
	if (p->visit & 32)
	{	return p;
	}
	p->visit |= 32;

	n = (Lextok *) hmalloc(sizeof(Lextok), ix, 88);
	memcpy(n, p, sizeof(Lextok));

	if (p->s)
	{	n->s = (char *) hmalloc(strlen(p->s)+1, ix, 89);
		strcpy(n->s, p->s);
	}

	n->a = dup_tree(p->a, ix);
	n->b = dup_tree(p->b, ix);
	n->c = dup_tree(p->c, ix);
	n->core = dup_tree(p->core, ix);

	// leave n->f as is

	n->lft = dup_tree(p->lft, ix);
	n->rgt = dup_tree(p->rgt, ix);

	return n;
}
#endif

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

// externally visible functions:

int
exec_prog(Prim **q, int ix)
{	Rtype rv;

	assert(ix >= 0 && ix < Ncore);
	sep[ix].T_stop = 0;
	memset(&rv, 0, sizeof(Rtype));

	if (showprog == 1 && ix == 0)
	{	FILE *fd = fopen(CobraDot, "a");

		if (!fd)
		{	fprintf(stderr, "cannot create '%s'\n", CobraDot);
		} else
		{	fprintf(fd, "digraph prog {\n");
			fprintf(fd, "node [margin=0 fontsize=8];\n");
			fprintf(fd, "start [shape=plaintext];\n");
			fprintf(fd, "start -> N%p;\n", (void *) p_tree);
			clr_marks(p_tree);
			dump_graph(fd, p_tree, "U");
			fprintf(fd, "}\n");
			fclose(fd);
			if (!json_format)
			{	printf("wrote '%s'\n", CobraDot);
			}
			if (system(ShowDot) < 0)
			{	perror(ShowDot);
			}
 #if 0
			sleep(1); // before the dot file is removed
 #else
			exit(0);
 #endif
		}
 #ifdef DEBUG
		clr_marks(p_tree);
		dump_tree(p_tree, 0);
 #endif
	}

	assert(my_tree != NULL && my_tree[ix] != NULL);
	eval_prog(q, my_tree[ix], &rv, ix);

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
	int ix;

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

	if (!showprog && (preserve || p_debug == 3))
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

	mk_fsm(p_tree, 0);				// visit |= 2
	opt_fsm(p_tree);				// visit |= 8

	// dump_tree(p_tree, 0);
#ifdef DUP_TREES
	// reduce cache misses
	my_tree[0] = p_tree;

	for (ix = 1; ix < Ncore; ix++)
	{	my_tree[ix] = dup_tree(p_tree, ix);
		clr_marks(p_tree);
	}
#else
	for (ix = 0; ix < Ncore; ix++)
	{	my_tree[ix] = p_tree;
	}

#endif

	if (stream == 1
	&& !streamable(p_tree))
	{	return 0;
	}

	if (!showprog && (preserve || p_debug == 4))
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
			sleep(1);	// in case there's more than one
			exit(0);	// version 5.0
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
