/* original parser id follows */
/* yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93" */
/* (use YYMAJOR/YYMINOR for ifdefs dependent on parser version) */

#define YYBYACC 1
#define YYMAJOR 2
#define YYMINOR 0
#define YYPATCH 20220114

#define YYEMPTY        (-1)
#define yyclearin      (yychar = YYEMPTY)
#define yyerrok        (yyerrflag = 0)
#define YYRECOVERING() (yyerrflag != 0)
#define YYENOMEM       (-2)
#define YYEOF          0
#undef YYBTYACC
#define YYBTYACC 0
#define YYDEBUGSTR YYPREFIX "debug"
#define YYPREFIX "yy"

#define YYPURE 0

#line 8 "cobra_eval.y"
#include "cobra.h"

/* parser for boolean and*/
/* arithmetic expressions*/

#define YYSTYPE	Lexptr
#define YYDEBUG 0

static Lextok	*p_tree;
static int	 last_tok;
static void	 yyerror(const char *);
static int	 yylex(void);
static int	 parse_error;

extern YYSTYPE	 yylval;
extern char	*b_cmd;
extern int	 evaluate(const Prim *, const Lextok *);
extern int	 yyparse(void);
#line 43 "cobra_eval.c"

#if ! defined(YYSTYPE) && ! defined(YYSTYPE_IS_DECLARED)
/* Default: YYSTYPE is the semantic value type. */
typedef int YYSTYPE;
# define YYSTYPE_IS_DECLARED 1
#endif

/* compatibility with bison */
#ifdef YYPARSE_PARAM
/* compatibility with FreeBSD */
# ifdef YYPARSE_PARAM_TYPE
#  define YYPARSE_DECL() yyparse(YYPARSE_PARAM_TYPE YYPARSE_PARAM)
# else
#  define YYPARSE_DECL() yyparse(void *YYPARSE_PARAM)
# endif
#else
# define YYPARSE_DECL() yyparse(void)
#endif

/* Parameters sent to lex. */
#ifdef YYLEX_PARAM
# define YYLEX_DECL() yylex(void *YYLEX_PARAM)
# define YYLEX yylex(YYLEX_PARAM)
#else
# define YYLEX_DECL() yylex(void)
# define YYLEX yylex()
#endif

#if !(defined(yylex) || defined(YYSTATE))
int YYLEX_DECL();
#endif

/* Parameters sent to yyerror. */
#ifndef YYERROR_DECL
#define YYERROR_DECL() yyerror(const char *s)
#endif
#ifndef YYERROR_CALL
#define YYERROR_CALL(msg) yyerror(msg)
#endif

extern int YYPARSE_DECL();

#define SIZE 257
#define NR 258
#define NAME 259
#define EOE 260
#define REGEX 261
#define OR 262
#define AND 263
#define EQ 264
#define NE 265
#define GT 266
#define LT 267
#define GE 268
#define LE 269
#define UMIN 270
#define YYERRCODE 256
typedef int YYINT;
static const YYINT yylhs[] = {                           -1,
    0,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,
};
static const YYINT yylen[] = {                            2,
    2,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    1,    1,    1,    2,    4,
    2,    2,    2,    2,    2,
};
static const YYINT yydefred[] = {                         0,
    0,   18,   16,   17,    0,    0,    0,    0,    0,    0,
    0,    0,   19,    0,   22,   24,   23,   21,   25,    0,
    1,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    2,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    5,    6,    7,   20,
};
#if defined(YYDESTRUCT_CALL) || defined(YYSTYPE_TOSTRING)
static const YYINT yystos[] = {                           0,
  257,  258,  259,  261,   45,   46,  126,   33,   58,   40,
  272,  273,  258,   40,  273,  259,  273,  273,  259,  273,
  260,  262,  263,  264,  265,  266,  267,  268,  269,   43,
   45,   42,   47,   37,  258,   41,  273,  273,  273,  273,
  273,  273,  273,  273,  273,  273,  273,  273,  273,   41,
};
#endif /* YYDESTRUCT_CALL || YYSTYPE_TOSTRING */
static const YYINT yydgoto[] = {                         11,
   12,
};
static const YYINT yysindex[] = {                        26,
   -1,    0,    0,    0,   26, -256,   26,   26, -255,   26,
    0,  -37,    0, -247,    0,    0,    0,    0,    0,  -28,
    0,   26,   26,   26,   26,   26,   26,   26,   26,   26,
   26,   26,   26,   26,  -23,    0,  -21,  -14,   -7,   -7,
    8,    8,    8,    8,  -35,  -35,    0,    0,    0,    0,
};
static const YYINT yyrindex[] = {                         0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  -40,   80,   68,   74,
   28,   38,   48,   58,    3,   13,    0,    0,    0,    0,
};
#if YYBTYACC
static const YYINT yycindex[] = {                         0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
#endif
static const YYINT yygindex[] = {                         0,
  100,
};
#define YYTABLESIZE 343
static const YYINT yytable[] = {                         34,
   14,   34,   16,   19,   32,   30,   32,   31,   34,   33,
   35,   33,   36,   32,   30,   34,   31,   50,   33,    0,
   32,   30,   34,   31,    0,   33,    0,   32,   30,   34,
   31,    0,   33,    0,   32,   30,    0,   31,   14,   33,
    0,    0,    0,    3,   34,    3,    0,    3,    0,   32,
   30,    0,   31,    4,   33,    4,    0,    4,    8,    0,
    0,    0,    0,    0,    0,   10,    0,    0,    8,    0,
    5,    6,    0,    0,    0,    0,    0,    0,   10,    0,
    0,    0,    0,    9,    0,    0,    0,    0,    9,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   11,    0,
    0,    0,    0,    0,   15,    0,   17,   18,   12,   20,
    0,    0,    0,    0,   13,    0,    0,    0,    0,    0,
   15,   37,   38,   39,   40,   41,   42,   43,   44,   45,
   46,   47,   48,   49,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    7,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   14,
    0,   14,   21,    0,   22,   23,   24,   25,   26,   27,
   28,   29,    0,   22,   23,   24,   25,   26,   27,   28,
   29,   23,   24,   25,   26,   27,   28,   29,    0,   24,
   25,   26,   27,   28,   29,    0,   13,    0,   26,   27,
   28,   29,    3,    0,    3,    3,    3,    3,    3,    3,
    3,    3,    4,    0,    4,    4,    4,    4,    4,    4,
    4,    4,    1,    2,    3,    0,    4,    8,    0,    8,
    8,    8,    8,    8,    8,    8,    8,   10,    0,   10,
   10,   10,   10,   10,   10,   10,   10,    9,    0,    9,
    9,    9,    9,    9,    9,    9,    9,   11,    0,   11,
   11,   11,   11,   11,   11,   11,   11,   12,    0,   12,
   12,   12,   12,   13,    0,   13,   13,   13,   13,   15,
    0,   15,   15,
};
static const YYINT yycheck[] = {                         37,
   41,   37,  259,  259,   42,   43,   42,   45,   37,   47,
  258,   47,   41,   42,   43,   37,   45,   41,   47,   -1,
   42,   43,   37,   45,   -1,   47,   -1,   42,   43,   37,
   45,   -1,   47,   -1,   42,   43,   -1,   45,   40,   47,
   -1,   -1,   -1,   41,   37,   43,   -1,   45,   -1,   42,
   43,   -1,   45,   41,   47,   43,   -1,   45,   33,   -1,
   -1,   -1,   -1,   -1,   -1,   40,   -1,   -1,   41,   -1,
   45,   46,   -1,   -1,   -1,   -1,   -1,   -1,   41,   -1,
   -1,   -1,   -1,   58,   -1,   -1,   -1,   -1,   41,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   41,   -1,
   -1,   -1,   -1,   -1,    5,   -1,    7,    8,   41,   10,
   -1,   -1,   -1,   -1,   41,   -1,   -1,   -1,   -1,   -1,
   41,   22,   23,   24,   25,   26,   27,   28,   29,   30,
   31,   32,   33,   34,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  126,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  260,
   -1,  262,  260,   -1,  262,  263,  264,  265,  266,  267,
  268,  269,   -1,  262,  263,  264,  265,  266,  267,  268,
  269,  263,  264,  265,  266,  267,  268,  269,   -1,  264,
  265,  266,  267,  268,  269,   -1,  258,   -1,  266,  267,
  268,  269,  260,   -1,  262,  263,  264,  265,  266,  267,
  268,  269,  260,   -1,  262,  263,  264,  265,  266,  267,
  268,  269,  257,  258,  259,   -1,  261,  260,   -1,  262,
  263,  264,  265,  266,  267,  268,  269,  260,   -1,  262,
  263,  264,  265,  266,  267,  268,  269,  260,   -1,  262,
  263,  264,  265,  266,  267,  268,  269,  260,   -1,  262,
  263,  264,  265,  266,  267,  268,  269,  260,   -1,  262,
  263,  264,  265,  260,   -1,  262,  263,  264,  265,  260,
   -1,  262,  263,
};
#if YYBTYACC
static const YYINT yyctable[] = {                        -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,
};
#endif
#define YYFINAL 11
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 270
#define YYUNDFTOKEN 274
#define YYTRANSLATE(a) ((a) > YYMAXTOKEN ? YYUNDFTOKEN : (a))
#if YYDEBUG
static const char *const yyname[] = {

"$end",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'!'",0,
0,0,"'%'",0,0,"'('","')'","'*'","'+'",0,"'-'","'.'","'/'",0,0,0,0,0,0,0,0,0,0,
"':'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'~'",0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,"error","SIZE","NR","NAME","EOE","REGEX","OR","AND","EQ","NE","GT","LT",
"GE","LE","UMIN","$accept","form","expr","illegal-symbol",
};
static const char *const yyrule[] = {
"$accept : form",
"form : expr EOE",
"expr : '(' expr ')'",
"expr : expr '+' expr",
"expr : expr '-' expr",
"expr : expr '*' expr",
"expr : expr '/' expr",
"expr : expr '%' expr",
"expr : expr GT expr",
"expr : expr GE expr",
"expr : expr LT expr",
"expr : expr LE expr",
"expr : expr EQ expr",
"expr : expr NE expr",
"expr : expr OR expr",
"expr : expr AND expr",
"expr : NAME",
"expr : REGEX",
"expr : NR",
"expr : SIZE NR",
"expr : SIZE '(' NR ')'",
"expr : '!' expr",
"expr : '-' expr",
"expr : '~' expr",
"expr : '.' NAME",
"expr : ':' NAME",

};
#endif

#if YYDEBUG
int      yydebug;
#endif

int      yyerrflag;
int      yychar;
YYSTYPE  yyval;
YYSTYPE  yylval;
int      yynerrs;

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
YYLTYPE  yyloc; /* position returned by actions */
YYLTYPE  yylloc; /* position from the lexer */
#endif

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
#ifndef YYLLOC_DEFAULT
#define YYLLOC_DEFAULT(loc, rhs, n) \
do \
{ \
    if (n == 0) \
    { \
        (loc).first_line   = YYRHSLOC(rhs, 0).last_line; \
        (loc).first_column = YYRHSLOC(rhs, 0).last_column; \
        (loc).last_line    = YYRHSLOC(rhs, 0).last_line; \
        (loc).last_column  = YYRHSLOC(rhs, 0).last_column; \
    } \
    else \
    { \
        (loc).first_line   = YYRHSLOC(rhs, 1).first_line; \
        (loc).first_column = YYRHSLOC(rhs, 1).first_column; \
        (loc).last_line    = YYRHSLOC(rhs, n).last_line; \
        (loc).last_column  = YYRHSLOC(rhs, n).last_column; \
    } \
} while (0)
#endif /* YYLLOC_DEFAULT */
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
#if YYBTYACC

#ifndef YYLVQUEUEGROWTH
#define YYLVQUEUEGROWTH 32
#endif
#endif /* YYBTYACC */

/* define the initial stack-sizes */
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH  YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH  10000
#endif
#endif

#ifndef YYINITSTACKSIZE
#define YYINITSTACKSIZE 200
#endif

typedef struct {
    unsigned stacksize;
    YYINT    *s_base;
    YYINT    *s_mark;
    YYINT    *s_last;
    YYSTYPE  *l_base;
    YYSTYPE  *l_mark;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    YYLTYPE  *p_base;
    YYLTYPE  *p_mark;
#endif
} YYSTACKDATA;
#if YYBTYACC

struct YYParseState_s
{
    struct YYParseState_s *save;    /* Previously saved parser state */
    YYSTACKDATA            yystack; /* saved parser stack */
    int                    state;   /* saved parser state */
    int                    errflag; /* saved error recovery status */
    int                    lexeme;  /* saved index of the conflict lexeme in the lexical queue */
    YYINT                  ctry;    /* saved index in yyctable[] for this conflict */
};
typedef struct YYParseState_s YYParseState;
#endif /* YYBTYACC */
/* variables for the parser stack */
static YYSTACKDATA yystack;
#if YYBTYACC

/* Current parser state */
static YYParseState *yyps = 0;

/* yypath != NULL: do the full parse, starting at *yypath parser state. */
static YYParseState *yypath = 0;

/* Base of the lexical value queue */
static YYSTYPE *yylvals = 0;

/* Current position at lexical value queue */
static YYSTYPE *yylvp = 0;

/* End position of lexical value queue */
static YYSTYPE *yylve = 0;

/* The last allocated position at the lexical value queue */
static YYSTYPE *yylvlim = 0;

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
/* Base of the lexical position queue */
static YYLTYPE *yylpsns = 0;

/* Current position at lexical position queue */
static YYLTYPE *yylpp = 0;

/* End position of lexical position queue */
static YYLTYPE *yylpe = 0;

/* The last allocated position at the lexical position queue */
static YYLTYPE *yylplim = 0;
#endif

/* Current position at lexical token queue */
static YYINT  *yylexp = 0;

static YYINT  *yylexemes = 0;
#endif /* YYBTYACC */
#line 68 "cobra_eval.y"
static int iscan;

#define Expect(x, y, z)			\
	if (b_cmd[iscan+1] == x)	\
	{	iscan += 2;		\
		yylval->typ = y;	\
		return y;		\
	} else				\
	{	iscan++;		\
		yylval->typ = z;	\
		return z;		\
	}

static int
isregexp(const char *s)
{	const char *t = s;

	while (*s != '\0'
	&& !isspace((uchar) *s))
	{	s++;
	}
	return (int) (s-t);
}

static int
eval_lex(void)
{	int p;

	yylval = (Lextok *) emalloc(sizeof(Lextok), 10);

	while (isspace((uchar) b_cmd[iscan]))
	{	iscan++;
	}
	yylval->s = "";
	yylval->typ = b_cmd[iscan];
	switch (b_cmd[iscan]) {
	case '/':
		if ((last_tok == EQ || last_tok == NE)
		&&  (p = isregexp(&b_cmd[iscan])) > 0)
		{	yylval->s = emalloc((p+1)*sizeof(char), 11);
			strncpy(yylval->s, &b_cmd[iscan], p+1);
			yylval->typ = REGEX;
			iscan += p;
			return REGEX;
		}
		// else fall thru
	case '*': 
	case '+': case '-':
	case '%': case '.':
	case '(': case ')':
	case ':':	// possible pe expression
		return b_cmd[iscan++];
	case '\\':
		iscan++;
		if (b_cmd[iscan] == '|')	// watch for \|\| from cobra_te.c
		{	if (b_cmd[iscan+1] == '\\'
			&&  b_cmd[iscan+2] == '|')
			{	iscan += 3;
				yylval->typ = OR;
				return OR;
		}	}
		// else fall through
	case '{': case '}':
	case '[': case ']':
		// special case of strings
		// since these have no special
		// meaning in eval exprs
		yylval->s = emalloc(2*sizeof(char), 12);
		yylval->s[0] = b_cmd[iscan++];
		yylval->typ = NAME;
		return NAME;
	case '"':
		p = ++iscan;
		while (b_cmd[iscan] != '"'
		  &&   b_cmd[iscan] != '\0'
		  &&   b_cmd[iscan] != '\n')
		{	if (b_cmd[iscan] == '\\')
			{	iscan++;
			}
			iscan++;
		}
		if (b_cmd[iscan] == '\0'
		||  b_cmd[iscan] == '\n')
		{	yylval->typ = EOE;
			return EOE;
		}
		b_cmd[iscan] = '\0';
		yylval->s = emalloc(strlen(&b_cmd[p])+1, 13);
		strcpy(yylval->s, &b_cmd[p]);	// safe
		b_cmd[iscan++] = '"';
		yylval->typ = NAME;
		return NAME;
	case '\n':
	case '\0':
		yylval->typ = EOE;
		return EOE;
	case '|':
		Expect('|', OR, '|');
		break;
	case '&':
		Expect('&', AND, '&');
		break;
	case '=':
		Expect('=', EQ, '=');
		break;
	case '!':
		Expect('=', NE, '!');
		break;
	case '>':
		Expect('=', GE, GT);
		break;
	case '<':
		Expect('=', LE, LT);
		break;
	default:
		break;
	}

	if (isdigit((uchar) b_cmd[iscan]))
	{	while (isdigit((uchar) b_cmd[iscan]))
		{	yylval->val *= 10;
			yylval->val += b_cmd[iscan++] - '0';
		}
		yylval->typ = NR;
		return NR;
	}

	if (strncmp(&b_cmd[iscan], "size", strlen("size")) == 0)
	{	iscan += (int) strlen("size");
		yylval->typ = SIZE;
		return SIZE;
	}
	if (isalpha((uchar) b_cmd[iscan])
	||  b_cmd[iscan] == '_')
	{	p = iscan;
		while ((isalpha((uchar) b_cmd[iscan])
		&&    !isspace((uchar) b_cmd[iscan]))
		||      isdigit((uchar) b_cmd[iscan])
		||	b_cmd[iscan] == '_'
		||      b_cmd[iscan] == '.')
		{	iscan++;
		}
		yylval->s = emalloc((iscan-p+1)*sizeof(char), 14);
		strncpy(yylval->s, &b_cmd[p], iscan-p);
		yylval->typ = NAME;
		return NAME;
	}

	printf("expr: cannot parse: '%s'\n", &b_cmd[iscan]);
	return '?';
}

static void
yyerror(const char *s)
{	int i;
	printf("expr: %s\n%s\n", s, yytext);
	for (i = 0; i < iscan; i++)
	{	printf(" ");
	}
	printf("^\n<%d>", yytext[iscan]);
	iscan = 0;
	parse_error++;
}

static int
lookup(const Prim *q, const char *s)
{	// .txt and .fnm handled elsewhere
	assert(q && s);
	if (strcmp(s, "lnr") == 0)
	{	return q->lnr;
	}
	if (strcmp(s, "curly") == 0)
	{	return q->curly;
	}
	if (strcmp(s, "round") == 0)
	{	return q->round;
	}
	if (strcmp(s, "bracket") == 0)
	{	return q->bracket;
	}
	if (strcmp(s, "len") == 0)
	{	return q->len;
	}
	if (strcmp(s, "seq") == 0)
	{	return q->seq;
	}
	if (strcmp(s, "mark") == 0)
	{	return q->mark;
	}
	if (strcmp(s, "range") == 0)
	{	if (q->bound || q->jmp)
		{	Prim *dest = q->bound?q->bound:q->jmp;
			if (strcmp(dest->fnm, q->fnm) == 0)
			{	return dest->lnr - q->lnr;
		}	}
		return 0;
	}
	printf("expr: unknown symbol .%s\n", s);
	return 0;
}

static const char *
tokenname(int n)
{	static char c_tmp[8];

	switch (n) {
	case SIZE: return "SIZE";
	case   NR: return "NR";
	case NAME: return "NAME";
	case  EOE: return "EOE";
	case   OR: return "||";
	case  AND: return "&&";
	case   NE: return "!=";
	case   EQ: return "==";
	case   LE: return "<=";
	case   GE: return ">=";
	case   LT: return "<";
	case   GT: return ">";
	case UMIN: return "-";
	case REGEX: return "RE";
	}
	snprintf(c_tmp, sizeof(c_tmp), "%c", n);
	return c_tmp;
}

enum {
	fnm_t     = 1,
	typ_t     = 2,
	txt_t     = 3,
	fct_t     = 4,

	lnr_t     = 10,
	seq_t     = 11,
	mark_t    = 12,
	curly_t   = 13,
	round_t   = 14,
	bracket_t = 15,
	len_t     = 16,

	nxt_t     = 20,
	prv_t     = 21,
	bound_t   = 22,
	jmp_t     = 23
};

static char *
dot_derive(const Prim *q, int e)
{
	switch (e) {
	case fnm_t: return q->fnm;
	case txt_t: return q->txt;
	case typ_t: return q->typ;
	case fct_t: return fct_which(q);
	default:    break;
	}
	return q->txt;
}

#define numeric_cmp(op) \
	switch (e) {	\
	case lnr_t:	return (q->lnr     op r->lnr)?0:1;	\
	case seq_t:	return (q->seq     op r->seq)?0:1;	\
	case mark_t:	return (q->mark    op r->mark)?0:1;	\
	case curly_t:	return (q->curly   op r->curly)?0:1;	\
	case round_t:	return (q->round   op r->round)?0:1;	\
	case bracket_t:	return (q->bracket op r->bracket)?0:1;	\
	case len_t:	return (q->len     op r->len)?0:1;	\
	default:	\
		fprintf(stderr, "error: invalid use of %s in constraint\n", #op);	\
		break;	\
	}


static int
field_cmp(const int op, const Prim *q, const Prim *r, int e)
{	// like strcmp, return 0 when equal, non-zero otherwise

	if (!q || !r)
	{	return 1;
	}

	switch (op) {
	case EQ:
	case NE:
		switch (e) {
		case fnm_t:	return strcmp(q->fnm, r->fnm);
		case typ_t:	return strcmp(q->typ, r->typ);
		case txt_t:	return strcmp(q->txt, r->txt);
		case fct_t:	return strcmp(fct_which(q), fct_which(r));

		case nxt_t:	return (q->nxt   == r)?0:1;
		case prv_t:	return (q->prv   == r)?0:1;
		case bound_t:	return (q->bound == r)?0:1;
		case jmp_t:	return (q->jmp   == r)?0:1;

		case lnr_t:
		case seq_t:
		case mark_t:
		case curly_t:
		case round_t:
		case bracket_t:
		case len_t:
			if (op == EQ)
			{	numeric_cmp(==);
			} else
			{	numeric_cmp(!=);
			}
			break;
		default:
			break;
		}
		break;

	case GT: numeric_cmp(>);  break; // if not numeric, complain
	case LT: numeric_cmp(<);  break;
	case GE: numeric_cmp(>=); break;
	case LE: numeric_cmp(<=); break;
	default:
		break;
	}
	fprintf(stderr, "error: invalid operator (%d) in constraint\n", op);
	return 1;
}

static int
field_type(const char *s)
{
	switch (s[0]) {
	case 'b':
		if (strcmp(s, "bound") == 0)
		{	return bound_t;
		}
		return (strcmp(s, "bracket") == 0)?bracket_t:0;
	case 'c':
		return (strcmp(s, "curly")   == 0)?curly_t:0;
	case 'f':	
		if (strcmp(s, "fct") == 0)
		{	return fct_t;
		}
		return (strcmp(s, "fnm")     == 0)?fnm_t:0;
	case 'j':
		return (strcmp(s, "jmp")     == 0)?jmp_t:0;
	case 'l':
		if (strcmp(s, "len") == 0)
		{	return len_t;
		}
		return (strcmp(s, "lnr")  == 0)?lnr_t:0;
	case 'm':
		return (strcmp(s, "mark")  == 0)?mark_t:0;
	case 'n':
		return (strcmp(s, "nxt")   == 0)?nxt_t:0;
	case 'p':
		return (strcmp(s, "prv")   == 0)?prv_t:0;
	case 'r':
		return (strcmp(s, "round") == 0)?round_t:0;
	case 's':
		return (strcmp(s, "seq")   == 0)?seq_t:0;
	case 't':
		if (strcmp(s, "txt") == 0)
		{	return txt_t;
		}
		return (strncmp(s, "typ", 3) == 0)?typ_t:0;
	default:
		break;
	}
	return 0;
}

// dot_match can be called from cobra_te.c to evaluate a
// constraint in a pattern match if there are either
// bound variables, names, or regular expressions

#if 0
static void
dt(Lextok *t, int i)
{

	if (t->s) { printf("'%s' ", t->s); }
	if (t->lft)
	{	printf("L: "); dt(t->lft, i+1);
	}
	if (t->rgt)
	{	printf("R: "); dt(t->rgt, i+1);
	}
}
#endif

static int
dot_match(const Prim *q, const int op, Lextok *lft, Lextok *rgt)
{	char *a = (char *) 0;
	char *b = (char *) 0;
	Prim *r = (Prim *) 0;
	int e = 0, le = 0, re = 0;

	if (lft->typ == '.')
	{	le = field_type(lft->s);
		if (!le)
		{	fprintf(stderr, "bad field type '%s'\n", lft->s);
			return -1;
	}	}
	if (rgt->typ == '.')
	{	re = field_type(rgt->s);
		if (!re)
		{	fprintf(stderr, "bad field type '%s'\n", rgt->s);
			return -1;
	}	}

	if (lft->typ == ':')			// lhs is bound var
	{	if (rgt->typ == ':')		// rhs as well
		{	e = 0;
		} else if (rgt->typ == '.')
		{	r = bound_prim(lft->rgt->s); // origin
			e = re;			// cmp bound fields below
		}
		if (lft->rgt && (a = strchr(lft->rgt->s, '.')))
		{	// lhs bound var contains a field like :x.len
			*a = '\0';
			r = bound_prim(lft->rgt->s);
			if (r) { e = field_type(a+1); }
		}

		else if (lft->rgt && lft->rgt->typ == NAME)
		{	r = bound_prim(lft->rgt->s);	// get the ref
		}
	} else  if (rgt->typ == ':')	// rhs is bound var
	{	if (lft->typ == '.')	// rhs refers to token field
		{	r = bound_prim(rgt->rgt->s);	// origin
			e = le;				// lhs ref
		}

		else if (rgt->rgt && rgt->rgt->typ == NAME)
		{	r = bound_prim(rgt->rgt->s);	// get the ref
	}	}


	if (rgt->typ == ':'
	&&  rgt->rgt
	&& (a = strchr(rgt->rgt->s, '.')))
	{	// rhs bound var contains a field like :x.len
		*a = '\0';
		r = bound_prim(rgt->rgt->s);
		if (r) { e = field_type(a+1); }
	}

	// q is the current token that a constraint applies to
	// r is a ref to the location where a bound var is defined
	// e is the field type to use in the comparison
	// e is zero if both lhs and rhs are bound vars or one is a regex
	if (e)
	{	return field_cmp(op, q, r, e);
		// now supports also >,<,>=,<= if field types are numeric
	}

	// two dot fields, bound vars, or one is a regex
	switch (lft->typ) {
	case '.':	a = dot_derive(q, le); break;
	case ':':	a = bound_text(lft->rgt->s); break;
	case REGEX:	a = rgt->s; // ignored: there should be just one regex
	default:	a = lft->s; break;
	}

	switch (rgt->typ) {
	case '.':	b = dot_derive(q, re); break;
	case ':':	b = bound_text(rgt->rgt->s); break;
	case REGEX:	return regex_match(0, a);
	default:	b = rgt->s; break;	// 4.0: was: a = rgt->s
	}
	if (lft->typ == REGEX)
	{	return regex_match(0, b);
	}

	if (!a || !b)
	{	return -1;
	}

	return strcmp(a, b);
}

static int
yylex(void)
{	int n = eval_lex();

	if (0)
	{	printf("yylex: %s <%d>", tokenname(n), n);
		if (n == NAME
		||  n == REGEX)
		{	printf(" = '%s'", yylval->s);
		}
		printf("\n");
	}
	last_tok = n;
	return n;
}

// externally visible function:

#define binop(op)	(evaluate(q, n->lft) op evaluate(q, n->rgt))

#define is_bound(n)	(n->rgt->typ == ':' || n->lft->typ == ':')

static int
do_bound(const Prim *q, const Lextok *n, const int op)
{
	// there is at least one side is a bound variable
	// because is_bound() is true
	// provided bindings were defined

	if (e_bindings)
	{	switch (op) {
		case LT:
		case LE:
		case GT:
		case GE: return dot_match(q, op, n->lft, n->rgt);
		case EQ: 
			return (dot_match(q, EQ, n->lft, n->rgt) == 0);
		case NE: return (dot_match(q, NE, n->lft, n->rgt) != 0);
		default:
			// cannot happen
			break;
	}	}

	return 0; // no bound vars defined
}

int	// also called in cobra_te.c
evaluate(const Prim *q, const Lextok *n)
{	int rval = 0;

	if (n)
	{	switch (n->typ) {
		case '+':  rval = binop(+); break;
		case '!':  rval = !evaluate(q, n->rgt); break;
		case '-':  if (n->lft)
			   {	rval = binop(-);
			   } else
			   {	rval = -evaluate(q, n->rgt);
			   }
			   break;
		case '*':  rval = binop(*); break;
		case '/':
			   rval = evaluate(q, n->rgt);
			   if (rval == 0)
			   {	// division by zero
				fprintf(stderr, "error: divsion by zero\n");
			   } else
			   {	rval = (evaluate(q, n->lft) / rval);
			   }
			   break;
		case '%':
			   rval = evaluate(q, n->rgt);
			   if (rval == 0)
			   {	// division by zero
				fprintf(stderr, "error: modulo of zero\n");
			   } else
			   {	rval = (evaluate(q, n->lft) % rval);
			   }
			   break;
		case  OR:  rval = binop(||); break;
		case AND:  rval = binop(&&); break;
		case  EQ:
			   if (is_bound(n))
			   {	rval = do_bound(q, n, EQ);
				break;
			   }
			   if (n->rgt->typ == NAME
			   ||  n->rgt->typ == REGEX
			   ||  n->lft->typ == NAME
			   ||  n->lft->typ == REGEX)
			   {	rval = (dot_match(q, EQ, n->lft, n->rgt) == 0);
			   } else
			   {	rval = binop(==);
			   }
			   break;
		case  NE:
			   if (is_bound(n))
			   {	rval = do_bound(q, n, NE);
				break;
			   }
			   if (n->rgt->typ == NAME
			   ||  n->rgt->typ == REGEX
			   ||  n->lft->typ == NAME
			   ||  n->lft->typ == REGEX)
			   {	rval = (dot_match(q, NE, n->lft, n->rgt) != 0);
			   } else
			   {	rval = binop(!=);
			   }
			   break;
		case  GT:
			   if (is_bound(n))
			   {	rval = (dot_match(q, n->typ, n->lft, n->rgt) == 0);
			   } else
			   {	rval = binop(>);
			   }
			   break;
		case  LT:
			   if (is_bound(n))
			   {	rval = (dot_match(q, n->typ, n->lft, n->rgt) == 0);
			   } else
			   {	rval = binop(<);
			   }
			   break;
		case  GE:
			   if (is_bound(n))
			   {	rval = (dot_match(q, n->typ, n->lft, n->rgt) == 0);
			   } else
			   {	rval = binop(>=);
			   }
			   break;
		case  LE:
			   if (is_bound(n))
			   {	rval = (dot_match(q, n->typ, n->lft, n->rgt) == 0);
			   } else
			   {	rval = binop(<=);
			   }
			   break;

		case '.':  rval = lookup(q, n->s); break;
		case NR:   rval = n->val; break;
		case SIZE: rval = nr_marks(n->rgt->val); break;
		case ':':  fprintf(stderr, "%s:%d invalid use of ':', expect int, saw string (%s %s)\n",
				q->fnm, q->lnr, n->lft?n->lft->s:"", n->rgt?n->rgt->s:"");
			   // a bound variable reference defaults to a string result
			   // but we expect an integer value here
			   break;
		default:   fprintf(stderr, "expr: unknown type %d\n", n->typ);
			   break;
	}	}
	return rval;
}

Lextok *
prep_eval(void)
{	Lextok *rval = NULL;

	parse_error = 0;
	iscan = 0;
	if (yyparse())
	{	rval = p_tree;
	}
	iscan = 0;
	return rval;
}

int
do_eval(const Prim *q)
{
	if (parse_error)
	{	return 0;
	}
	return evaluate(q, p_tree);
}
#line 1101 "cobra_eval.c"

/* For use in generated program */
#define yydepth (int)(yystack.s_mark - yystack.s_base)
#if YYBTYACC
#define yytrial (yyps->save)
#endif /* YYBTYACC */

#if YYDEBUG
#include <stdio.h>	/* needed for printf */
#endif

#include <stdlib.h>	/* needed for malloc, etc */
#include <string.h>	/* needed for memset */

/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack(YYSTACKDATA *data)
{
    int i;
    unsigned newsize;
    YYINT *newss;
    YYSTYPE *newvs;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    YYLTYPE *newps;
#endif

    if ((newsize = data->stacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return YYENOMEM;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;

    i = (int) (data->s_mark - data->s_base);
    newss = (YYINT *)realloc(data->s_base, newsize * sizeof(*newss));
    if (newss == 0)
        return YYENOMEM;

    data->s_base = newss;
    data->s_mark = newss + i;

    newvs = (YYSTYPE *)realloc(data->l_base, newsize * sizeof(*newvs));
    if (newvs == 0)
        return YYENOMEM;

    data->l_base = newvs;
    data->l_mark = newvs + i;

#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    newps = (YYLTYPE *)realloc(data->p_base, newsize * sizeof(*newps));
    if (newps == 0)
        return YYENOMEM;

    data->p_base = newps;
    data->p_mark = newps + i;
#endif

    data->stacksize = newsize;
    data->s_last = data->s_base + newsize - 1;

#if YYDEBUG
    if (yydebug)
        fprintf(stderr, "%sdebug: stack size increased to %d\n", YYPREFIX, newsize);
#endif
    return 0;
}

#if YYPURE || defined(YY_NO_LEAKS)
static void yyfreestack(YYSTACKDATA *data)
{
    free(data->s_base);
    free(data->l_base);
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    free(data->p_base);
#endif
    memset(data, 0, sizeof(*data));
}
#else
#define yyfreestack(data) /* nothing */
#endif /* YYPURE || defined(YY_NO_LEAKS) */
#if YYBTYACC

static YYParseState *
yyNewState(unsigned size)
{
    YYParseState *p = (YYParseState *) malloc(sizeof(YYParseState));
    if (p == NULL) return NULL;

    p->yystack.stacksize = size;
    if (size == 0)
    {
        p->yystack.s_base = NULL;
        p->yystack.l_base = NULL;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        p->yystack.p_base = NULL;
#endif
        return p;
    }
    p->yystack.s_base    = (YYINT *) malloc(size * sizeof(YYINT));
    if (p->yystack.s_base == NULL) return NULL;
    p->yystack.l_base    = (YYSTYPE *) malloc(size * sizeof(YYSTYPE));
    if (p->yystack.l_base == NULL) return NULL;
    memset(p->yystack.l_base, 0, size * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    p->yystack.p_base    = (YYLTYPE *) malloc(size * sizeof(YYLTYPE));
    if (p->yystack.p_base == NULL) return NULL;
    memset(p->yystack.p_base, 0, size * sizeof(YYLTYPE));
#endif

    return p;
}

static void
yyFreeState(YYParseState *p)
{
    yyfreestack(&p->yystack);
    free(p);
}
#endif /* YYBTYACC */

#define YYABORT  goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR  goto yyerrlab
#if YYBTYACC
#define YYVALID        do { if (yyps->save)            goto yyvalid; } while(0)
#define YYVALID_NESTED do { if (yyps->save && \
                                yyps->save->save == 0) goto yyvalid; } while(0)
#endif /* YYBTYACC */

int
YYPARSE_DECL()
{
    int yym, yyn, yystate, yyresult;
#if YYBTYACC
    int yynewerrflag;
    YYParseState *yyerrctx = NULL;
#endif /* YYBTYACC */
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    YYLTYPE  yyerror_loc_range[3]; /* position of error start/end (0 unused) */
#endif
#if YYDEBUG
    const char *yys;

    if ((yys = getenv("YYDEBUG")) != 0)
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
    if (yydebug)
        fprintf(stderr, "%sdebug[<# of symbols on state stack>]\n", YYPREFIX);
#endif
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    memset(yyerror_loc_range, 0, sizeof(yyerror_loc_range));
#endif

#if YYBTYACC
    yyps = yyNewState(0); if (yyps == 0) goto yyenomem;
    yyps->save = 0;
#endif /* YYBTYACC */
    yym = 0;
    /* yyn is set below */
    yynerrs = 0;
    yyerrflag = 0;
    yychar = YYEMPTY;
    yystate = 0;

#if YYPURE
    memset(&yystack, 0, sizeof(yystack));
#endif

    if (yystack.s_base == NULL && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
    yystack.s_mark = yystack.s_base;
    yystack.l_mark = yystack.l_base;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark = yystack.p_base;
#endif
    yystate = 0;
    *yystack.s_mark = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
#if YYBTYACC
        do {
        if (yylvp < yylve)
        {
            /* we're currently re-reading tokens */
            yylval = *yylvp++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yylloc = *yylpp++;
#endif
            yychar = *yylexp++;
            break;
        }
        if (yyps->save)
        {
            /* in trial mode; save scanner results for future parse attempts */
            if (yylvp == yylvlim)
            {   /* Enlarge lexical value queue */
                size_t p = (size_t) (yylvp - yylvals);
                size_t s = (size_t) (yylvlim - yylvals);

                s += YYLVQUEUEGROWTH;
                if ((yylexemes = (YYINT *)realloc(yylexemes, s * sizeof(YYINT))) == NULL) goto yyenomem;
                if ((yylvals   = (YYSTYPE *)realloc(yylvals, s * sizeof(YYSTYPE))) == NULL) goto yyenomem;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                if ((yylpsns   = (YYLTYPE *)realloc(yylpsns, s * sizeof(YYLTYPE))) == NULL) goto yyenomem;
#endif
                yylvp   = yylve = yylvals + p;
                yylvlim = yylvals + s;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylpp   = yylpe = yylpsns + p;
                yylplim = yylpsns + s;
#endif
                yylexp  = yylexemes + p;
            }
            *yylexp = (YYINT) YYLEX;
            *yylvp++ = yylval;
            yylve++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            *yylpp++ = yylloc;
            yylpe++;
#endif
            yychar = *yylexp++;
            break;
        }
        /* normal operation, no conflict encountered */
#endif /* YYBTYACC */
        yychar = YYLEX;
#if YYBTYACC
        } while (0);
#endif /* YYBTYACC */
        if (yychar < 0) yychar = YYEOF;
#if YYDEBUG
        if (yydebug)
        {
            if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
            fprintf(stderr, "%s[%d]: state %d, reading token %d (%s)",
                            YYDEBUGSTR, yydepth, yystate, yychar, yys);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
            if (!yytrial)
#endif /* YYBTYACC */
                fprintf(stderr, " <%s>", YYSTYPE_TOSTRING(yychar, yylval));
#endif
            fputc('\n', stderr);
        }
#endif
    }
#if YYBTYACC

    /* Do we have a conflict? */
    if (((yyn = yycindex[yystate]) != 0) && (yyn += yychar) >= 0 &&
        yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yychar)
    {
        YYINT ctry;

        if (yypath)
        {
            YYParseState *save;
#if YYDEBUG
            if (yydebug)
                fprintf(stderr, "%s[%d]: CONFLICT in state %d: following successful trial parse\n",
                                YYDEBUGSTR, yydepth, yystate);
#endif
            /* Switch to the next conflict context */
            save = yypath;
            yypath = save->save;
            save->save = NULL;
            ctry = save->ctry;
            if (save->state != yystate) YYABORT;
            yyFreeState(save);

        }
        else
        {

            /* Unresolved conflict - start/continue trial parse */
            YYParseState *save;
#if YYDEBUG
            if (yydebug)
            {
                fprintf(stderr, "%s[%d]: CONFLICT in state %d. ", YYDEBUGSTR, yydepth, yystate);
                if (yyps->save)
                    fputs("ALREADY in conflict, continuing trial parse.\n", stderr);
                else
                    fputs("Starting trial parse.\n", stderr);
            }
#endif
            save                  = yyNewState((unsigned)(yystack.s_mark - yystack.s_base + 1));
            if (save == NULL) goto yyenomem;
            save->save            = yyps->save;
            save->state           = yystate;
            save->errflag         = yyerrflag;
            save->yystack.s_mark  = save->yystack.s_base + (yystack.s_mark - yystack.s_base);
            memcpy (save->yystack.s_base, yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
            save->yystack.l_mark  = save->yystack.l_base + (yystack.l_mark - yystack.l_base);
            memcpy (save->yystack.l_base, yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            save->yystack.p_mark  = save->yystack.p_base + (yystack.p_mark - yystack.p_base);
            memcpy (save->yystack.p_base, yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
            ctry                  = yytable[yyn];
            if (yyctable[ctry] == -1)
            {
#if YYDEBUG
                if (yydebug && yychar >= YYEOF)
                    fprintf(stderr, "%s[%d]: backtracking 1 token\n", YYDEBUGSTR, yydepth);
#endif
                ctry++;
            }
            save->ctry = ctry;
            if (yyps->save == NULL)
            {
                /* If this is a first conflict in the stack, start saving lexemes */
                if (!yylexemes)
                {
                    yylexemes = (YYINT *) malloc((YYLVQUEUEGROWTH) * sizeof(YYINT));
                    if (yylexemes == NULL) goto yyenomem;
                    yylvals   = (YYSTYPE *) malloc((YYLVQUEUEGROWTH) * sizeof(YYSTYPE));
                    if (yylvals == NULL) goto yyenomem;
                    yylvlim   = yylvals + YYLVQUEUEGROWTH;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    yylpsns   = (YYLTYPE *) malloc((YYLVQUEUEGROWTH) * sizeof(YYLTYPE));
                    if (yylpsns == NULL) goto yyenomem;
                    yylplim   = yylpsns + YYLVQUEUEGROWTH;
#endif
                }
                if (yylvp == yylve)
                {
                    yylvp  = yylve = yylvals;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    yylpp  = yylpe = yylpsns;
#endif
                    yylexp = yylexemes;
                    if (yychar >= YYEOF)
                    {
                        *yylve++ = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                        *yylpe++ = yylloc;
#endif
                        *yylexp  = (YYINT) yychar;
                        yychar   = YYEMPTY;
                    }
                }
            }
            if (yychar >= YYEOF)
            {
                yylvp--;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylpp--;
#endif
                yylexp--;
                yychar = YYEMPTY;
            }
            save->lexeme = (int) (yylvp - yylvals);
            yyps->save   = save;
        }
        if (yytable[yyn] == ctry)
        {
#if YYDEBUG
            if (yydebug)
                fprintf(stderr, "%s[%d]: state %d, shifting to state %d\n",
                                YYDEBUGSTR, yydepth, yystate, yyctable[ctry]);
#endif
            if (yychar < 0)
            {
                yylvp++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylpp++;
#endif
                yylexp++;
            }
            if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM)
                goto yyoverflow;
            yystate = yyctable[ctry];
            *++yystack.s_mark = (YYINT) yystate;
            *++yystack.l_mark = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            *++yystack.p_mark = yylloc;
#endif
            yychar  = YYEMPTY;
            if (yyerrflag > 0) --yyerrflag;
            goto yyloop;
        }
        else
        {
            yyn = yyctable[ctry];
            goto yyreduce;
        }
    } /* End of code dealing with conflicts */
#endif /* YYBTYACC */
    if (((yyn = yysindex[yystate]) != 0) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yychar)
    {
#if YYDEBUG
        if (yydebug)
            fprintf(stderr, "%s[%d]: state %d, shifting to state %d\n",
                            YYDEBUGSTR, yydepth, yystate, yytable[yyn]);
#endif
        if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
        yystate = yytable[yyn];
        *++yystack.s_mark = yytable[yyn];
        *++yystack.l_mark = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        *++yystack.p_mark = yylloc;
#endif
        yychar = YYEMPTY;
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if (((yyn = yyrindex[yystate]) != 0) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag != 0) goto yyinrecovery;
#if YYBTYACC

    yynewerrflag = 1;
    goto yyerrhandler;
    goto yyerrlab; /* redundant goto avoids 'unused label' warning */

yyerrlab:
    /* explicit YYERROR from an action -- pop the rhs of the rule reduced
     * before looking for error recovery */
    yystack.s_mark -= yym;
    yystate = *yystack.s_mark;
    yystack.l_mark -= yym;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark -= yym;
#endif

    yynewerrflag = 0;
yyerrhandler:
    while (yyps->save)
    {
        int ctry;
        YYParseState *save = yyps->save;
#if YYDEBUG
        if (yydebug)
            fprintf(stderr, "%s[%d]: ERROR in state %d, CONFLICT BACKTRACKING to state %d, %d tokens\n",
                            YYDEBUGSTR, yydepth, yystate, yyps->save->state,
                    (int)(yylvp - yylvals - yyps->save->lexeme));
#endif
        /* Memorize most forward-looking error state in case it's really an error. */
        if (yyerrctx == NULL || yyerrctx->lexeme < yylvp - yylvals)
        {
            /* Free old saved error context state */
            if (yyerrctx) yyFreeState(yyerrctx);
            /* Create and fill out new saved error context state */
            yyerrctx                 = yyNewState((unsigned)(yystack.s_mark - yystack.s_base + 1));
            if (yyerrctx == NULL) goto yyenomem;
            yyerrctx->save           = yyps->save;
            yyerrctx->state          = yystate;
            yyerrctx->errflag        = yyerrflag;
            yyerrctx->yystack.s_mark = yyerrctx->yystack.s_base + (yystack.s_mark - yystack.s_base);
            memcpy (yyerrctx->yystack.s_base, yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
            yyerrctx->yystack.l_mark = yyerrctx->yystack.l_base + (yystack.l_mark - yystack.l_base);
            memcpy (yyerrctx->yystack.l_base, yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yyerrctx->yystack.p_mark = yyerrctx->yystack.p_base + (yystack.p_mark - yystack.p_base);
            memcpy (yyerrctx->yystack.p_base, yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
            yyerrctx->lexeme         = (int) (yylvp - yylvals);
        }
        yylvp          = yylvals   + save->lexeme;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        yylpp          = yylpsns   + save->lexeme;
#endif
        yylexp         = yylexemes + save->lexeme;
        yychar         = YYEMPTY;
        yystack.s_mark = yystack.s_base + (save->yystack.s_mark - save->yystack.s_base);
        memcpy (yystack.s_base, save->yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
        yystack.l_mark = yystack.l_base + (save->yystack.l_mark - save->yystack.l_base);
        memcpy (yystack.l_base, save->yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        yystack.p_mark = yystack.p_base + (save->yystack.p_mark - save->yystack.p_base);
        memcpy (yystack.p_base, save->yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
        ctry           = ++save->ctry;
        yystate        = save->state;
        /* We tried shift, try reduce now */
        if ((yyn = yyctable[ctry]) >= 0) goto yyreduce;
        yyps->save     = save->save;
        save->save     = NULL;
        yyFreeState(save);

        /* Nothing left on the stack -- error */
        if (!yyps->save)
        {
#if YYDEBUG
            if (yydebug)
                fprintf(stderr, "%sdebug[%d,trial]: trial parse FAILED, entering ERROR mode\n",
                                YYPREFIX, yydepth);
#endif
            /* Restore state as it was in the most forward-advanced error */
            yylvp          = yylvals   + yyerrctx->lexeme;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yylpp          = yylpsns   + yyerrctx->lexeme;
#endif
            yylexp         = yylexemes + yyerrctx->lexeme;
            yychar         = yylexp[-1];
            yylval         = yylvp[-1];
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yylloc         = yylpp[-1];
#endif
            yystack.s_mark = yystack.s_base + (yyerrctx->yystack.s_mark - yyerrctx->yystack.s_base);
            memcpy (yystack.s_base, yyerrctx->yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
            yystack.l_mark = yystack.l_base + (yyerrctx->yystack.l_mark - yyerrctx->yystack.l_base);
            memcpy (yystack.l_base, yyerrctx->yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            yystack.p_mark = yystack.p_base + (yyerrctx->yystack.p_mark - yyerrctx->yystack.p_base);
            memcpy (yystack.p_base, yyerrctx->yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
            yystate        = yyerrctx->state;
            yyFreeState(yyerrctx);
            yyerrctx       = NULL;
        }
        yynewerrflag = 1;
    }
    if (yynewerrflag == 0) goto yyinrecovery;
#endif /* YYBTYACC */

    YYERROR_CALL("syntax error");
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yyerror_loc_range[1] = yylloc; /* lookahead position is error start position */
#endif

#if !YYBTYACC
    goto yyerrlab; /* redundant goto avoids 'unused label' warning */
yyerrlab:
#endif
    ++yynerrs;

yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if (((yyn = yysindex[*yystack.s_mark]) != 0) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    fprintf(stderr, "%s[%d]: state %d, error recovery shifting to state %d\n",
                                    YYDEBUGSTR, yydepth, *yystack.s_mark, yytable[yyn]);
#endif
                if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
                yystate = yytable[yyn];
                *++yystack.s_mark = yytable[yyn];
                *++yystack.l_mark = yylval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                /* lookahead position is error end position */
                yyerror_loc_range[2] = yylloc;
                YYLLOC_DEFAULT(yyloc, yyerror_loc_range, 2); /* position of error span */
                *++yystack.p_mark = yyloc;
#endif
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    fprintf(stderr, "%s[%d]: error recovery discarding state %d\n",
                                    YYDEBUGSTR, yydepth, *yystack.s_mark);
#endif
                if (yystack.s_mark <= yystack.s_base) goto yyabort;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                /* the current TOS position is the error start position */
                yyerror_loc_range[1] = *yystack.p_mark;
#endif
#if defined(YYDESTRUCT_CALL)
#if YYBTYACC
                if (!yytrial)
#endif /* YYBTYACC */
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    YYDESTRUCT_CALL("error: discarding state",
                                    yystos[*yystack.s_mark], yystack.l_mark, yystack.p_mark);
#else
                    YYDESTRUCT_CALL("error: discarding state",
                                    yystos[*yystack.s_mark], yystack.l_mark);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
#endif /* defined(YYDESTRUCT_CALL) */
                --yystack.s_mark;
                --yystack.l_mark;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                --yystack.p_mark;
#endif
            }
        }
    }
    else
    {
        if (yychar == YYEOF) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
            fprintf(stderr, "%s[%d]: state %d, error recovery discarding token %d (%s)\n",
                            YYDEBUGSTR, yydepth, yystate, yychar, yys);
        }
#endif
#if defined(YYDESTRUCT_CALL)
#if YYBTYACC
        if (!yytrial)
#endif /* YYBTYACC */
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
            YYDESTRUCT_CALL("error: discarding token", yychar, &yylval, &yylloc);
#else
            YYDESTRUCT_CALL("error: discarding token", yychar, &yylval);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
#endif /* defined(YYDESTRUCT_CALL) */
        yychar = YYEMPTY;
        goto yyloop;
    }

yyreduce:
    yym = yylen[yyn];
#if YYDEBUG
    if (yydebug)
    {
        fprintf(stderr, "%s[%d]: state %d, reducing by rule %d (%s)",
                        YYDEBUGSTR, yydepth, yystate, yyn, yyrule[yyn]);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
        if (!yytrial)
#endif /* YYBTYACC */
            if (yym > 0)
            {
                int i;
                fputc('<', stderr);
                for (i = yym; i > 0; i--)
                {
                    if (i != yym) fputs(", ", stderr);
                    fputs(YYSTYPE_TOSTRING(yystos[yystack.s_mark[1-i]],
                                           yystack.l_mark[1-i]), stderr);
                }
                fputc('>', stderr);
            }
#endif
        fputc('\n', stderr);
    }
#endif
    if (yym > 0)
        yyval = yystack.l_mark[1-yym];
    else
        memset(&yyval, 0, sizeof yyval);
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)

    /* Perform position reduction */
    memset(&yyloc, 0, sizeof(yyloc));
#if YYBTYACC
    if (!yytrial)
#endif /* YYBTYACC */
    {
        YYLLOC_DEFAULT(yyloc, &yystack.p_mark[-yym], yym);
        /* just in case YYERROR is invoked within the action, save
           the start of the rhs as the error start position */
        yyerror_loc_range[1] = yystack.p_mark[1-yym];
    }
#endif

    switch (yyn)
    {
case 1:
#line 40 "cobra_eval.y"
	{ p_tree = yystack.l_mark[-1]; return 1; }
#line 1774 "cobra_eval.c"
break;
case 2:
#line 42 "cobra_eval.y"
	{ yyval = yystack.l_mark[-1]; }
#line 1779 "cobra_eval.c"
break;
case 3:
#line 43 "cobra_eval.y"
	{ yystack.l_mark[-1]->lft = yystack.l_mark[-2]; yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; }
#line 1784 "cobra_eval.c"
break;
case 4:
#line 44 "cobra_eval.y"
	{ yystack.l_mark[-1]->lft = yystack.l_mark[-2]; yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; }
#line 1789 "cobra_eval.c"
break;
case 5:
#line 45 "cobra_eval.y"
	{ yystack.l_mark[-1]->lft = yystack.l_mark[-2]; yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; }
#line 1794 "cobra_eval.c"
break;
case 6:
#line 46 "cobra_eval.y"
	{ yystack.l_mark[-1]->lft = yystack.l_mark[-2]; yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; }
#line 1799 "cobra_eval.c"
break;
case 7:
#line 47 "cobra_eval.y"
	{ yystack.l_mark[-1]->lft = yystack.l_mark[-2]; yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; }
#line 1804 "cobra_eval.c"
break;
case 8:
#line 48 "cobra_eval.y"
	{ yystack.l_mark[-1]->lft = yystack.l_mark[-2]; yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; }
#line 1809 "cobra_eval.c"
break;
case 9:
#line 49 "cobra_eval.y"
	{ yystack.l_mark[-1]->lft = yystack.l_mark[-2]; yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; }
#line 1814 "cobra_eval.c"
break;
case 10:
#line 50 "cobra_eval.y"
	{ yystack.l_mark[-1]->lft = yystack.l_mark[-2]; yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; }
#line 1819 "cobra_eval.c"
break;
case 11:
#line 51 "cobra_eval.y"
	{ yystack.l_mark[-1]->lft = yystack.l_mark[-2]; yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; }
#line 1824 "cobra_eval.c"
break;
case 12:
#line 52 "cobra_eval.y"
	{ yystack.l_mark[-1]->lft = yystack.l_mark[-2]; yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; }
#line 1829 "cobra_eval.c"
break;
case 13:
#line 53 "cobra_eval.y"
	{ yystack.l_mark[-1]->lft = yystack.l_mark[-2]; yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; }
#line 1834 "cobra_eval.c"
break;
case 14:
#line 54 "cobra_eval.y"
	{ yystack.l_mark[-1]->lft = yystack.l_mark[-2]; yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; }
#line 1839 "cobra_eval.c"
break;
case 15:
#line 55 "cobra_eval.y"
	{ yystack.l_mark[-1]->lft = yystack.l_mark[-2]; yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; }
#line 1844 "cobra_eval.c"
break;
case 16:
#line 56 "cobra_eval.y"
	{ yyval = yystack.l_mark[0]; }
#line 1849 "cobra_eval.c"
break;
case 17:
#line 57 "cobra_eval.y"
	{ yyval = yystack.l_mark[0]; set_regex(yystack.l_mark[0]->s+1); }
#line 1854 "cobra_eval.c"
break;
case 18:
#line 58 "cobra_eval.y"
	{ yyval = yystack.l_mark[0]; }
#line 1859 "cobra_eval.c"
break;
case 19:
#line 59 "cobra_eval.y"
	{ yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; }
#line 1864 "cobra_eval.c"
break;
case 20:
#line 60 "cobra_eval.y"
	{ yystack.l_mark[-3]->rgt = yystack.l_mark[-1]; yyval = yystack.l_mark[-3]; }
#line 1869 "cobra_eval.c"
break;
case 21:
#line 61 "cobra_eval.y"
	{ yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; }
#line 1874 "cobra_eval.c"
break;
case 22:
#line 62 "cobra_eval.y"
	{ yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; }
#line 1879 "cobra_eval.c"
break;
case 23:
#line 63 "cobra_eval.y"
	{ yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; }
#line 1884 "cobra_eval.c"
break;
case 24:
#line 64 "cobra_eval.y"
	{ yystack.l_mark[-1]->s = yystack.l_mark[0]->s; yyval = yystack.l_mark[-1]; }
#line 1889 "cobra_eval.c"
break;
case 25:
#line 65 "cobra_eval.y"
	{ yystack.l_mark[-1]->rgt = yystack.l_mark[0]; yyval = yystack.l_mark[-1]; /* pe expression */ }
#line 1894 "cobra_eval.c"
break;
#line 1896 "cobra_eval.c"
    default:
        break;
    }
    yystack.s_mark -= yym;
    yystate = *yystack.s_mark;
    yystack.l_mark -= yym;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark -= yym;
#endif
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
        {
            fprintf(stderr, "%s[%d]: after reduction, ", YYDEBUGSTR, yydepth);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
            if (!yytrial)
#endif /* YYBTYACC */
                fprintf(stderr, "result is <%s>, ", YYSTYPE_TOSTRING(yystos[YYFINAL], yyval));
#endif
            fprintf(stderr, "shifting from state 0 to final state %d\n", YYFINAL);
        }
#endif
        yystate = YYFINAL;
        *++yystack.s_mark = YYFINAL;
        *++yystack.l_mark = yyval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        *++yystack.p_mark = yyloc;
#endif
        if (yychar < 0)
        {
#if YYBTYACC
            do {
            if (yylvp < yylve)
            {
                /* we're currently re-reading tokens */
                yylval = *yylvp++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                yylloc = *yylpp++;
#endif
                yychar = *yylexp++;
                break;
            }
            if (yyps->save)
            {
                /* in trial mode; save scanner results for future parse attempts */
                if (yylvp == yylvlim)
                {   /* Enlarge lexical value queue */
                    size_t p = (size_t) (yylvp - yylvals);
                    size_t s = (size_t) (yylvlim - yylvals);

                    s += YYLVQUEUEGROWTH;
                    if ((yylexemes = (YYINT *)realloc(yylexemes, s * sizeof(YYINT))) == NULL)
                        goto yyenomem;
                    if ((yylvals   = (YYSTYPE *)realloc(yylvals, s * sizeof(YYSTYPE))) == NULL)
                        goto yyenomem;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    if ((yylpsns   = (YYLTYPE *)realloc(yylpsns, s * sizeof(YYLTYPE))) == NULL)
                        goto yyenomem;
#endif
                    yylvp   = yylve = yylvals + p;
                    yylvlim = yylvals + s;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                    yylpp   = yylpe = yylpsns + p;
                    yylplim = yylpsns + s;
#endif
                    yylexp  = yylexemes + p;
                }
                *yylexp = (YYINT) YYLEX;
                *yylvp++ = yylval;
                yylve++;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
                *yylpp++ = yylloc;
                yylpe++;
#endif
                yychar = *yylexp++;
                break;
            }
            /* normal operation, no conflict encountered */
#endif /* YYBTYACC */
            yychar = YYLEX;
#if YYBTYACC
            } while (0);
#endif /* YYBTYACC */
            if (yychar < 0) yychar = YYEOF;
#if YYDEBUG
            if (yydebug)
            {
                if ((yys = yyname[YYTRANSLATE(yychar)]) == NULL) yys = yyname[YYUNDFTOKEN];
                fprintf(stderr, "%s[%d]: state %d, reading token %d (%s)\n",
                                YYDEBUGSTR, yydepth, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == YYEOF) goto yyaccept;
        goto yyloop;
    }
    if (((yyn = yygindex[yym]) != 0) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == (YYINT) yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
    {
        fprintf(stderr, "%s[%d]: after reduction, ", YYDEBUGSTR, yydepth);
#ifdef YYSTYPE_TOSTRING
#if YYBTYACC
        if (!yytrial)
#endif /* YYBTYACC */
            fprintf(stderr, "result is <%s>, ", YYSTYPE_TOSTRING(yystos[yystate], yyval));
#endif
        fprintf(stderr, "shifting from state %d to state %d\n", *yystack.s_mark, yystate);
    }
#endif
    if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
    *++yystack.s_mark = (YYINT) yystate;
    *++yystack.l_mark = yyval;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    *++yystack.p_mark = yyloc;
#endif
    goto yyloop;
#if YYBTYACC

    /* Reduction declares that this path is valid. Set yypath and do a full parse */
yyvalid:
    if (yypath) YYABORT;
    while (yyps->save)
    {
        YYParseState *save = yyps->save;
        yyps->save = save->save;
        save->save = yypath;
        yypath = save;
    }
#if YYDEBUG
    if (yydebug)
        fprintf(stderr, "%s[%d]: state %d, CONFLICT trial successful, backtracking to state %d, %d tokens\n",
                        YYDEBUGSTR, yydepth, yystate, yypath->state, (int)(yylvp - yylvals - yypath->lexeme));
#endif
    if (yyerrctx)
    {
        yyFreeState(yyerrctx);
        yyerrctx = NULL;
    }
    yylvp          = yylvals + yypath->lexeme;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yylpp          = yylpsns + yypath->lexeme;
#endif
    yylexp         = yylexemes + yypath->lexeme;
    yychar         = YYEMPTY;
    yystack.s_mark = yystack.s_base + (yypath->yystack.s_mark - yypath->yystack.s_base);
    memcpy (yystack.s_base, yypath->yystack.s_base, (size_t) (yystack.s_mark - yystack.s_base + 1) * sizeof(YYINT));
    yystack.l_mark = yystack.l_base + (yypath->yystack.l_mark - yypath->yystack.l_base);
    memcpy (yystack.l_base, yypath->yystack.l_base, (size_t) (yystack.l_mark - yystack.l_base + 1) * sizeof(YYSTYPE));
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
    yystack.p_mark = yystack.p_base + (yypath->yystack.p_mark - yypath->yystack.p_base);
    memcpy (yystack.p_base, yypath->yystack.p_base, (size_t) (yystack.p_mark - yystack.p_base + 1) * sizeof(YYLTYPE));
#endif
    yystate        = yypath->state;
    goto yyloop;
#endif /* YYBTYACC */

yyoverflow:
    YYERROR_CALL("yacc stack overflow");
#if YYBTYACC
    goto yyabort_nomem;
yyenomem:
    YYERROR_CALL("memory exhausted");
yyabort_nomem:
#endif /* YYBTYACC */
    yyresult = 2;
    goto yyreturn;

yyabort:
    yyresult = 1;
    goto yyreturn;

yyaccept:
#if YYBTYACC
    if (yyps->save) goto yyvalid;
#endif /* YYBTYACC */
    yyresult = 0;

yyreturn:
#if defined(YYDESTRUCT_CALL)
    if (yychar != YYEOF && yychar != YYEMPTY)
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        YYDESTRUCT_CALL("cleanup: discarding token", yychar, &yylval, &yylloc);
#else
        YYDESTRUCT_CALL("cleanup: discarding token", yychar, &yylval);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */

    {
        YYSTYPE *pv;
#if defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED)
        YYLTYPE *pp;

        for (pv = yystack.l_base, pp = yystack.p_base; pv <= yystack.l_mark; ++pv, ++pp)
             YYDESTRUCT_CALL("cleanup: discarding state",
                             yystos[*(yystack.s_base + (pv - yystack.l_base))], pv, pp);
#else
        for (pv = yystack.l_base; pv <= yystack.l_mark; ++pv)
             YYDESTRUCT_CALL("cleanup: discarding state",
                             yystos[*(yystack.s_base + (pv - yystack.l_base))], pv);
#endif /* defined(YYLTYPE) || defined(YYLTYPE_IS_DECLARED) */
    }
#endif /* defined(YYDESTRUCT_CALL) */

#if YYBTYACC
    if (yyerrctx)
    {
        yyFreeState(yyerrctx);
        yyerrctx = NULL;
    }
    while (yyps)
    {
        YYParseState *save = yyps;
        yyps = save->save;
        save->save = NULL;
        yyFreeState(save);
    }
    while (yypath)
    {
        YYParseState *save = yypath;
        yypath = save->save;
        save->save = NULL;
        yyFreeState(save);
    }
#endif /* YYBTYACC */
    yyfreestack(&yystack);
    return (yyresult);
}
