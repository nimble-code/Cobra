/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "cobra_pre.h"

static const struct {
	char	*str;
	char	*typ;
	unsigned int	lang;
} c_name[] = {
	{ "auto",	"storage",   c_t },		// C
	{ "extern",	"storage",   c_t },
	{ "register",	"storage",   c_t },
	{ "static",	"storage",   c_t },
	{ "signed",	"modifier",  c_t },
	{ "unsigned",	"modifier",  c_t },
	{ "long",	"modifier",  c_t },
	{ "short",	"modifier",  c_t },
	{ "const",	"qualifier", c_t },
	{ "volatile",	"qualifier", c_t },
	{ "FILE",	"type",	c_t },
	{ "char",	"type",	c_t },
	{ "double",	"type",	c_t },
	{ "float",	"type",	c_t },
	{ "int",	"type",	c_t },
	{ "void",	"type",	c_t },
	{ "enum",	"key",	c_t },
	{ "struct",	"key",	c_t },
	{ "union",	"key",	c_t },
	{ "typedef",	"key",	c_t },
	{ "break",	"key",	c_t },
	{ "case",	"key",	c_t },
	{ "continue",	"key",	c_t },
	{ "default",	"key",	c_t },
	{ "do",		"key",	c_t },
	{ "else",	"key",	c_t },
	{ "for",	"key",	c_t },
	{ "goto",	"key",	c_t },
	{ "if",		"key",	c_t },
	{ "return",	"key",	c_t },
	{ "sizeof",	"key",	c_t },
	{ "switch",	"key",	c_t },
	{ "while",	"key",	c_t },
	{ "alignas",	"key",	cpp_t },		// C++
	{ "alignof",	"key",	cpp_t },
	{ "and_eq",	"key",	cpp_t },
	{ "asm",	"key",	cpp_t },
	{ "bitand",	"key",	cpp_t },
	{ "bitor",	"key",	cpp_t },
	{ "bool",	"key",	cpp_t },
	{ "char16_t",	"key",	cpp_t },
	{ "char32_t",	"key",	cpp_t },
	{ "compl",	"key",	cpp_t },
	{ "concept",	"key",	cpp_t },
	{ "constexpr",	"key",	cpp_t },
	{ "const_cast",	"key",	cpp_t },
	{ "decltype",	"key",	cpp_t },
	{ "delete",	"key",	cpp_t },
	{ "dynamic_cast", "key",	cpp_t },
	{ "explicit",	"key",	cpp_t },
	{ "export",	"key",	cpp_t },
	{ "false",	"key",	cpp_t },
	{ "friend",	"key",	cpp_t },
	{ "inline",	"key",	cpp_t },
	{ "mutable",	"key",	cpp_t },
	{ "namespace",	"key",	cpp_t },
	{ "noexcept",	"key",	cpp_t },
	{ "not_eq",	"key",	cpp_t },
	{ "nullptr",	"key",	cpp_t },
	{ "operator",	"key",	cpp_t },
	{ "or_eq",	"key",	cpp_t },
	{ "reinterpret_cast", 	"key",	cpp_t },
	{ "requires",	"key",	cpp_t },
	{ "static_assert",	"key",	cpp_t },
	{ "static_cast",	"key",	cpp_t },
	{ "template",	"key",	cpp_t },
	{ "thread_local",	"key",	cpp_t },
	{ "true",	"key",	cpp_t },
	{ "typeid",	"key",	cpp_t },
	{ "typename",	"key",	cpp_t },
	{ "using",	"key",	cpp_t },
	{ "virtual",	"key",	cpp_t },
	{ "wchar_t",	"key",	cpp_t },
	{ "xor_eq",	"key",	cpp_t },
	{ "abort",	"key",	ada_t },		// Ada
	{ "abs",	"key",	ada_t },
	{ "accept",	"key",	ada_t },
	{ "access",	"key",	ada_t },
	{ "aliased",	"key",	ada_t },
	{ "all",	"key",	ada_t },
	{ "array",	"key",	ada_t },
	{ "at",		"key",	ada_t },
	{ "begin",	"key",	ada_t },
	{ "body",	"key",	ada_t },
	{ "constant",	"key",	ada_t },
	{ "declare",	"key",	ada_t },
	{ "def",	"key",	python_t },
	{ "del",	"key",	python_t },
	{ "except",	"key",	python_t },
	{ "exec",	"key",	python_t },
	{ "from",	"key",	python_t },
	{ "global",	"key",	python_t },
	{ "lambda",	"key",	python_t },
	{ "pass",	"key",	python_t },
	{ "print",	"key",	python_t },
	{ "elif",	"key",	python_t },
	{ "delay",	"key",	ada_t },
	{ "delta",	"key",	ada_t },
	{ "digits",	"key",	ada_t },
	{ "elsif",	"key",	ada_t },
	{ "end",	"key",	ada_t },
	{ "entry",	"key",	ada_t },
	{ "exception",	"key",	ada_t },
	{ "exit",	"key",	ada_t },
	{ "function",	"key",	ada_t },
	{ "generic",	"key",	ada_t },
	{ "limited",	"key",	ada_t },
	{ "loop",	"key",	ada_t },
	{ "mod",	"key",	ada_t },
	{ "null",	"key",	ada_t },
	{ "of",		"key",	ada_t },
	{ "others",	"key",	ada_t },
	{ "out",	"key",	ada_t },
	{ "overriding",	"key",	ada_t },
	{ "procedure",	"key",	ada_t },
	{ "range",	"key",	ada_t },
	{ "record",	"key",	ada_t },
	{ "rem"	,	"key",	ada_t },
	{ "renames",	"key",	ada_t },
	{ "requeue",	"key",	ada_t },
	{ "reverse",	"key",	ada_t },
	{ "select",	"key",	ada_t },
	{ "separate",	"key",	ada_t },
	{ "some",	"key",	ada_t },
	{ "subtype",	"key",	ada_t },
	{ "tagged",	"key",	ada_t },
	{ "task",	"key",	ada_t },
	{ "terminate",	"key",	ada_t },
	{ "then",	"key",	ada_t },
	{ "type",	"key",	ada_t },
	{ "until",	"key",	ada_t },
	{ "use"	,	"key",	ada_t },
	{ "with",	"key",	ada_t },
	{ "when",	"key",	ada_t },
	{ "raise",	"key",	ada_t | python_t },		// ada, python
	{ "is",		"key",	ada_t | python_t },
	{ "in",		"key",	ada_t | python_t },
	{ "and",	"key",	cpp_t | ada_t | python_t },	// C++, ada, python
	{ "not",	"key",	cpp_t | ada_t | python_t },
	{ "xor",	"key",	cpp_t | ada_t | python_t },
	{ "or",		"key",	cpp_t | ada_t | python_t },
	{ "new",	"key",	cpp_t | ada_t | java_t },	// C++, ada, java
	{ "protected",	"key",	cpp_t | ada_t | java_t },
	{ "private",	"key",	cpp_t | ada_t | java_t },
	{ "catch",	"key",  cpp_t | java_t },		// C++, java
	{ "public",	"key",  cpp_t | java_t },
	{ "this",	"key",  cpp_t | java_t },
	{ "throw",	"key",  cpp_t | java_t },
	{ "class",	"key",  cpp_t | java_t | python_t },	// C++, python, java
	{ "try",	"key",  cpp_t | java_t | python_t },
	{ "abstract",	"key",  ada_t | java_t },		// ada, java
	{ "interface",	"key",  ada_t | java_t },
	{ "synchronized",	"key",  ada_t | java_t },
	{ "package",	"key",  ada_t | java_t },
	{ "boolean",	"key",  java_t },			// java
	{ "byte",	"key",  java_t },
	{ "extends",	"key",  java_t },
	{ "final",	"key",  java_t },
	{ "implements",	"key",  java_t },
	{ "instanceof",	"key",  java_t },
	{ "native",	"key",  java_t },
	{ "strictfp",	"key",  java_t },
	{ "super",	"key",  java_t },
	{ "throws",	"key",  java_t },
	{ "transient",	"key",  java_t },
	{ "import",	"key",  java_t | python_t },		// python, java
	{ "finally",	"key",  java_t | python_t },
	{ "assert",	"key",  java_t | python_t },
	{ 0, 0, 0 }
};

static const struct {
	char	*str;
} directive[] = {
	{ "define" },
	{ "elif" },
	{ "else" },
	{ "endif" },
	{ "error" },
	{ "ident" },
	{ "if" },
	{ "ifdef" },
	{ "ifndef" },
	{ "include" },
	{ "pragma" },
	{ "undef" },
	{ 0 }
};

static int
isdirective(int cid)
{	int i;

	assert(cid >= 0 && cid < Ncore);
	for (i = 0; directive[i].str; i++)
	{	if (strcmp(directive[i].str, Px.lex_yytext) == 0)
		{	return 1;
	}	}
	return 0;
}

static int
append_char(int n, int m, int cid)
{
	if (n != EOF
	&&  m < MAXYYTEXT-2
	&&  m >= 0)
	{	assert(cid >= 0 && cid < Ncore);
		Px.lex_yytext[m++] = n;
	}
	return m;
}

static int
nextchar(int cid)
{	int n, m;

	assert(cid >= 0 && cid < Ncore);
	if (Px.lex_pcnt > 0)
	{	n = Px.lex_pback[ --(Px.lex_pcnt) ];
	} else
	{	if (Px.lex_put >= Px.lex_got)
		{	Px.lex_put = 0;
			Px.lex_got = (int)
				read(Px.lex_yyin,
				     &Px.lex_in,
				     sizeof(Px.lex_in));

			if (Px.lex_got <= 0)
			{	return EOF;
		}	}
		m = Px.lex_put++;
		assert(m < NIN);
		n = Px.lex_in[m];
	}

	if (n == '\n')
	{	Px.lex_lineno++;
	}

	return n;
}

static void
pushback(int n, int cid)
{
	assert(cid >= 0 && cid < Ncore);
	Px.lex_pback[ Px.lex_pcnt++ ] = n;
	assert(Px.lex_pcnt < 8);

	if (n == '\n')
	{	Px.lex_lineno--;
	}
}

static void
show1(char *type, int cid)		// item (stands for itself)
{
	assert(cid >= 0 && cid < Ncore);
	snprintf(Px.lex_out, sizeof(Px.lex_out), "%s", type);
	process_line(Px.lex_out, cid);
}

static void
show2(const char *type, const char *raw, int cid)	// item, count, raw text
{
	assert(cid >= 0 && cid < Ncore);
	snprintf(Px.lex_out, sizeof(Px.lex_out), "%s\t%d\t%s",
		type, (int) strlen(raw), raw);
	process_line(Px.lex_out, cid);
}

static void
check_recall(int cid)
{
	assert(cid >= 0 && cid < Ncore);
	if (Px.lex_cpp[2]
	&&  (all_headers
	||   strncmp(Px.lex_fname, HEADER, strlen(HEADER)) != 0))
	{	// printf(">>> done with '%s'\n", Px.lex_fname);
		remember(Px.lex_fname, sanitycheck(cid), cid);
	}

	// 1	start of a new file
	// 2	returning to a file (after having included another file)
	// 3	the following (fragment of) text comes from a system header file
	// 4	the following (fragment of) text should be treated
	//	as being wrapped in an implicit extern "C" block
	//
	// ->	when we see a 2 this means that the last file can be added to
	//	the fully processed list, to avoid duplicates (unless the name
	//	starts with /usr, then we are already excluding it from the token
	//	stream)
	// ->	dont filter on 3/4 since these can span fragments of text
	//	which can make the remainder syntactically incomplete
}

static void
setlineno(const char *s, int cid)
{	char b[1024];
	int n;

	if (no_cpp)
	{	return;
	}
	if (!s
	||  *s == '\n'
	||  *s == '\0'
	||  strstr(s, "<built-in>")
	||  strstr(s, "<command-line>")
	||  strlen(s) > sizeof(b))
	{	return;
	}

	assert(cid >= 0 && cid < Ncore);

	if (strlen(s) > sizeof(b)
	||  sscanf(s, "# %d \"%s\"", &Px.lex_lineno, b) != 2)
	{	printf("%s:%d: error: setlino '%s'\n",
			Px.lex_fname, Px.lex_lineno, s);
		return;
	}

	n = strlen(b);
	if (n > 0 && b[n-1] == '"')
	{	b[n-1] = '\0';
		n--;
	}
	if (strcmp(Px.lex_fname, b) != 0)
	{	check_recall(cid);
		Px.lex_fname = (char *) hmalloc(n+1, cid);	// XXX setlineno
		strcpy(Px.lex_fname, b);
		if (seenbefore(b, 0))
		{	Px.lex_dontcare = 1;
		} else
		{	Px.lex_dontcare = 0;
	}	}
}

static void
c_comment(int cid)	/* c comment */
{	int n;
	int m = 2;

	assert(cid >= 0 && cid < Ncore);
	strcpy(Px.lex_yytext, "/*");
	while ((n = nextchar(cid)) != EOF)
	{	m = append_char(n, m, cid);
		if (n != '*')
		{	continue;
		}
		while ((n = nextchar(cid)) == '*')
		{	m = append_char(n, m, cid);
		}
		m = append_char(n, m, cid);
		if (n == '/')
		{	if (with_comments)
			{	Px.lex_yytext[m] = '\0';
				show2("cmnt", Px.lex_yytext, cid);
			}
			break;
	}	}
	assert(m < MAXYYTEXT);
	Px.lex_yytext[m] = '\0';
}

static void
p_comment(const char *s, int cid)
{	char buf[MAXYYTEXT];
	char *p;
	int n, i;

	assert(cid >= 0 && cid < Ncore);
	strncpy(Px.lex_yytext, s, MAXYYTEXT);
	Px.lex_yytext[MAXYYTEXT-1] = '\0';
#if 0
	if (fgets(buf, sizeof(buf), Px.lex_yyin) == NULL)
	{	pushback(EOF, cid);
		return;
	}
#else
	i = 0;
	do {	n = nextchar(cid);
		assert(i < MAXYYTEXT);
		buf[i++] = n;
	} while (n != '\n' && n != EOF);

	Px.lex_lineno--;

	if (n == EOF)
	{	pushback(EOF, cid);
		return;
	}
	buf[i] = '\0';
#endif
	if ((p = strchr(buf, '\r')) != NULL
	||  (p = strchr(buf, '\n')) != NULL)
	{	*p = '\0';
	}
	strncat(Px.lex_yytext, buf, MAXYYTEXT - 3);
	if (with_comments)
	{	show2("cmnt", Px.lex_yytext, cid);
	}
	if (no_cpp)
	{	Px.lex_lineno++;
	}
	line(cid);
}

static void
char_or_str(int which, int cid)
{	int n, m;

	assert(cid >= 0 && cid < Ncore);
	strcpy(Px.lex_yytext, "");
	m = append_char(which, 0, cid);
	while ((n = nextchar(cid)) != EOF)
	{	m = append_char(n, m, cid);
		if (n == '\\')
		{	n = nextchar(cid);
			m = append_char(n, m, cid);
		} else if (n == which)
		{	break;
		} else if (n == '\n')
		{	// error: strings or chars should not contain newlines
			if (verbose)
			{	printf("%s:%d: error: unterminated string or character constant\n",
					Px.lex_fname, Px.lex_lineno);
			}
			Px.lex_lineno++;
			break;
	}	}
	assert(m < MAXYYTEXT);
	Px.lex_yytext[m] = '\0';
}

static char *
number(int c, int cid)
{	int n, m = 0;
	char *s = (c == '.')?"const_flt":"const_int";

	assert(cid >= 0 && cid < Ncore);
	strcpy(Px.lex_yytext, "");

	if (c == '.')
	{	n = c;
		goto F;
	}

	m = append_char(c, 0, cid);

	if (c == '0')	// oct or hex
	{	n = nextchar(cid);
		if (n == '.')
		{	goto F;
		}
		if (n == 'x' || n == 'X')	// hex
		{	m = append_char(n, m, cid);
			n = nextchar(cid);
			while ((n >= '0' && n <= '9')
			   ||  (n >= 'a' && n <= 'f')
			   ||  (n >= 'A' && n <= 'F'))
			{	m = append_char(n, m, cid);
				n = nextchar(cid);
			}
			s = "const_hex";
		} else				// oct
		{	while (n >= '0' && n <= '7')
			{	m = append_char(n, m, cid);
				n = nextchar(cid);
			}
			s = (m == 1) ? "const_int" : "const_oct";
		}
		// optional suffix
		while (n == 'l' || n == 'L' || n == 'u' || n == 'U')
		{	m = append_char(n, m, cid);
			n = nextchar(cid);
		}
		pushback(n, cid);
		Px.lex_yytext[m] = '\0';
		return s;
	}

	while ((n = nextchar(cid)) != EOF)
	{	if (!isdigit((uchar) n))
		{	if (n == '.')
			{
F:				m = append_char(n, m, cid);
				s = "const_flt";
				continue;
			}
			if (n == 'e' || n == 'E')
			{	m = append_char(n, m, cid);
				s = "const_flt";
				n = nextchar(cid);
				if (n == '-' || n == '+')
				{	m = append_char(n, m, cid);
				} else
				{	pushback(n, cid);
				}
				continue;
			}

			while  (n == 'l' || n == 'L'
			   ||   n == 'u' || n == 'U'
			   || ((n == 'f' || n == 'F')
			      && strcmp(s, "const_flt") == 0))
			{	m = append_char(n, m, cid);
				n = nextchar(cid);
			}
			pushback(n, cid);
			break;
		}
		m = append_char(n, m, cid);
	}
	assert(m < MAXYYTEXT);
	Px.lex_yytext[m] = '\0';
	return s;
}

static void
name(int c, int cid)
{	int n, m;

	assert(cid >= 0 && cid < Ncore);
	strcpy(Px.lex_yytext, "");
	m = append_char(c, 0, cid);
	while ((n = nextchar(cid)) != EOF)
	{	if (!isalnum((uchar) n) && n != '_')
		{	pushback(n, cid);
			break;
		}
		m = append_char(n, m, cid);
	}
	assert(m < MAXYYTEXT);
	Px.lex_yytext[m] = '\0';
	if (Px.lex_preprocessing == 1)
	{	if (isdirective(cid))
		{	char buf[MAXYYTEXT+64];
			snprintf(buf, sizeof(buf), "#%s", Px.lex_yytext);
			show2("cpp", buf, cid);
			Px.lex_preprocessing = 2; // need EOP token
		} else	
		{	show2("ident", Px.lex_yytext, cid);
		}	
		return;
	}
	for (n = 0; c_name[n].str; n++)
	{	if (strcmp(c_name[n].str, Px.lex_yytext) == 0)
		{	if (cplusplus + java + python + ada == 0
			&&  !(c_name[n].lang & c_t))
			{	break;
			}
			if ((!(c_name[n].lang & (c_t | cpp_t)) && cplusplus)
			||  (!(c_name[n].lang & java_t)   && java)
			||  (!(c_name[n].lang & python_t) && python)
			||  (!(c_name[n].lang & ada_t)    && ada))
			{	break;
			}
			show2(c_name[n].typ, Px.lex_yytext, cid);
			return; 
	}	}
	show2("ident", Px.lex_yytext, cid);
}

static char *
ifnext(const int c, char *ifyes, char *ifno, int cid)
{	int n;

	assert(cid >= 0 && cid < Ncore);
	n = nextchar(cid);
	if (n == c)
	{	return ifyes;
	}
	pushback(n, cid);
	return ifno;
}

static char *
ifeither(const int a, const int b, char *if_a, char *if_b, char *ifno, int cid)
{	int n;

	n = nextchar(cid);
	if (n == a)
	{	return if_a;
	}
	if (n == b)
	{	return if_b;
	}
	pushback(n, cid);
	return ifno;
}

static int
operator(int c, int cid)
{	int n;
	char *dst = "";

	switch (c) {
	case '^':
		dst = ifnext('=', "^=", "^", cid);
		break;
	case '~':
		dst = ifnext('=', "~=", "~", cid);
		break;
	case '=':
		dst = ifnext('=', "==", "=", cid);
		break;

	case '<':
		n = nextchar(cid);
		switch (n) {
		case '=':
			dst = "<=";
			break;
		case '<':
			dst = ifnext('=', "<<=", "<<", cid);
			break;
		default:
			pushback(n, cid);
			dst = "<";
			break;
		}
		break;

	case '>':
		n = nextchar(cid);
		switch (n) {
		case '=':
			dst = ">=";
			break;
		case '>':	// seen >> so far, could be >>=, >>>, or >>pushback
			if (java)
			{	n = nextchar(cid);
				switch (n) {
				case '=': dst = ">>="; break;
				case '>': dst = ">>>"; break;
				default : dst = ">>"; pushback(n, cid); break;
				}
			} else
			{	dst = ifnext('=', ">>=", ">>", cid);
			}
			break;
		default:
			pushback(n, cid);
			dst = ">";
			break;
		}
		break;

	case '.':
		n = nextchar(cid);
		if (n != '.')
		{	pushback(n, cid);
			dst = ".";
			break;
		}
		n = nextchar(cid);
		if (n != '.')
		{	pushback(n, cid);
			pushback('.', cid);
			dst = ".";
			break;
		}
		dst = "...";
		break;

	case '|':
		dst = ifeither('=', '|', "|=", "||", "|", cid);
		break;
	case '&':
		dst = ifeither('=', '&', "&=", "&&", "&", cid);
		break;
	case '+':
		dst = ifeither('=', '+', "+=", "++", "+", cid);
		break;

	case '-':
		n = nextchar(cid);
		switch (n) {
		case '=':
			dst = "-=";
			break;
		case '-':
			dst = "--"; // ada: start of comment to \n
			break;
		case '>':
			dst = "->";
			break;
		default:
			pushback(n, cid);
			dst = "-";
			break;
		}
		break;

	case ':':
		dst = ifnext(':', "::", ":", cid);
		break;

	case '!':
		dst = ifnext('=', "!=", "!", cid);
		break;

	case '/':
		dst = ifnext('=', "/=", "/", cid);
		break;

	case '*':
		if (python)
		{	n = nextchar(cid);
			if (n == '*')
			{	dst = ifnext('=', "**=", "**", cid);
				break;
			}
			pushback(n, cid);
		}
		dst = ifnext('=', "*=", "*", cid);
		break;

	case '%':
		dst = ifnext('=', "%=", "%", cid);
		break;

	case '?':
		dst = "?";
		break;

	default:
		return 0;
	}
	show2("oper", dst, cid);
	return 1;
}

static int
skip_white(int cid)
{	int n;
	do {
		n = nextchar(cid);
	} while (n == ' ' || n == '\t');
	return n;
}

static void
dodirective(int cid)
{	int n, i, noname=0;

	// only called for parse macros option, which implies -nocpp

	assert(cid >= 0 && cid < Ncore);
	strcpy(Px.lex_yytext, "# ");
	n = skip_white(cid);

	if (n == EOF)
	{	return;
	}

	if (isdigit((uchar) n))
	{	noname = 1;
	}

	for (i = 2; n != EOF && n != '\n'; i++)
	{	Px.lex_yytext[i] = n;
		n = nextchar(cid);
	}
	Px.lex_yytext[i] = '\0';

	if (noname)
	{	setlineno(Px.lex_yytext, cid);
	}
	show2("cpp", Px.lex_yytext, cid);
	line(cid);
}

static void
token(int c, int cid)
{
	assert(cid >= 0 && cid < Ncore);
	strcpy(Px.lex_yytext, "");
	(void) append_char(c, 0, cid);
	Px.lex_yytext[1] = '\0';
//	show2("token", Px.lex_yytext, cid);
	show1(Px.lex_yytext, cid);
}

void
line(int cid)	// called also in cobra_prep.c
{
	assert(cid >= 0 && cid < Ncore);
	snprintf(Px.lex_out, sizeof(Px.lex_out), "line\t%d\t%s",
		Px.lex_lineno, Px.lex_fname);
	process_line(Px.lex_out, cid);	// where the data structure is build
}

extern int eol;
int
c_lex(int cid)	// called in cobra_prep.c
{	int n=0;
	char *t;

	assert(cid >= 0 && cid < Ncore);
	while (n != EOF)
	{	strcpy(Px.lex_yytext, "");
		n = nextchar(cid);
// fprintf(stderr, "%d %d\n", cid, n);
		switch (n) {
		case '\n':
			line(cid);
			if (Px.lex_preprocessing == 2 || eol != 0)
			{	show2("cpp", "EOL", cid);
			}
			Px.lex_preprocessing = 0;
			// fall thru
		case '\r':
			continue;
		case '\\':
			n = nextchar(cid);
			if (n == '\n')
			{	continue;
			}
			pushback(n, cid);
			token('\\', cid);
			return '\\';
		case '/':
			n = nextchar(cid);
			switch (n) {
			case '*':
				c_comment(cid);
				break;
			case '/':
				p_comment("//", cid);
				break;
			default:
				pushback(n, cid);
				show2("oper", "/", cid);
				break;
			}
			continue;
		case '-':
			if (ada)
			{	n = nextchar(cid);
				if (n == '-')
				{	p_comment("--", cid);
					continue;
				}
				pushback(n, cid);
				n = '-';
			}
			break;
		case '#':
			if (parse_macros) // means nocpp
			{	dodirective(cid);
				continue;
			}
			if (!Px.lex_preprocessing)
			{	Px.lex_preprocessing = 1;
				continue;
			}
			break;
		case 'L':
			n = nextchar(cid);
			switch (n) {
			case '"':
			case '\'':
				char_or_str(n, cid);
				show2((n=='"')?"str":"chr", Px.lex_yytext, cid);
				continue;
			default:
				pushback(n, cid);
				n = 'L';
				break;	
			}
			break;
		case '"':
			if (Px.lex_preprocessing == 1)
			{	char buf[NOUT+64];
				char_or_str(n, cid);
				snprintf(buf, sizeof(buf), "# %d %s", Px.lex_lineno, Px.lex_yytext);
				memset(Px.lex_cpp, 0, sizeof(Px.lex_cpp));
				if (no_cpp)
				{	do {
						n = nextchar(cid);
					} while (n != EOF && n != '\n');
				} else
				{	do {
						n = nextchar(cid);
						if (isdigit((uchar) n))
						{	// printf(">> %s  [%c]\n", buf, n);
							assert(n > '0' && n < '5');
							Px.lex_cpp[n - '0'] = 1;
						} else if (!isspace((uchar) n) && n != EOF && n != '\n')
						{	do {
							  n = nextchar(cid);
							} while (n != EOF && n != '\n');
						}
					} while (n != EOF && n != '\n');
				}
				setlineno(buf, cid); // XYXYXY possible fnm change, flags set
				line(cid);
				Px.lex_preprocessing = 0;
				memset(Px.lex_cpp, 0, sizeof(Px.lex_cpp));
				continue;
			}
			// fall thru
		case '\'':
			char_or_str(n, cid);
			show2((n=='"')?"str":"chr", Px.lex_yytext, cid);
			continue;
		case ' ':
		case '\t':
			continue;
		default:
			break;
		}
		// process other tokens
		if (n == '.')
		{	n = nextchar(cid);
			if (n == EOF)
			{	break;
			}
			if (isdigit((uchar) n))
			{	pushback(n, cid);
				n = '.';
				goto N;
			} else
			{	pushback(n, cid);
				n = '.';
		}	}

		if (isdigit((uchar) n))
		{
N:			t = number(n, cid);
			if (strcmp(t, "const_int") == 0
			&&  Px.lex_preprocessing == 1
			&&  !no_cpp)
			{	Px.lex_lineno = atoi(Px.lex_yytext);
//				Px.lex_lineno--; // counter nl at end
			} else
			{	show2(t, Px.lex_yytext, cid);
			}
			continue;
		}
		if (isalpha((uchar) n) || n == '_')
		{	name(n, cid);
			continue;
		}
		if (operator(n, cid))
		{	continue;
		}
		if (n == EOF)
		{	break;
		}
		token(n, cid);
	}
	return 0;
}
