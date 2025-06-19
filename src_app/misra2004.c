#include "c_api.h"

static int done_cpp;

static void
bad(char *s)
{
	printf("%s:%d:  %s\n",
		cobra_bfnm(), cobra_lnr(), s);
}

static int
s_match(char *t, char *s)	// at least one char from s appears in t
{	char *p;

	for (p = s; *p != '\0'; p++)
	{	if (strchr(t, *p))
		{	return 1;
	}	}
	return 0;
}

void
cobra_main(void)
{	char *s;

	if (!no_cpp)
	{	fprintf(stderr, "usage: ./misra file.c*\n");
		exit(1);
	}

	for ( ; cur; NEXT)
	{	if (TYPE("cmnt"))
		{	s = cobra_txt();
			if (strncmp(s, "//", 2) == 0)
			{	bad("R2.2: C++ style comment");
			}
			if (strncmp(s, "/*", 2) == 0
			&&  strstr(s+2, "/*") != NULL)
			{	bad("R2.3: nested /* comment");
		}	}

		if (TYPE("str") || TYPE("chr"))
		{	s = cobra_txt();
			if (s[0] == '\\'	// incorrect: parsed as two tokens \ and 990
			&&  isdigit((uchar)s[1])
			&&  isdigit((uchar)s[2])
			&&  isdigit((uchar)s[3]))
			{	bad("R7.1: octal escape");
			}

			while ((s = strchr(s, '\\')) != NULL)
			{	if (!s_match(++s, "\\'\"?abfnrtvxX0-9"))
				{	bad("R4.1: undefined escape sequence");
		}	}	}

		if (TYPE("ident"))
		{	done_cpp = 1;
			if (strlen(cobra_txt()) > 30)
			{	bad("R5.1: long identifier");
		}	}

		if (MATCH("goto"))	{ bad("R14.4: uses goto"); }
		if (MATCH("continue"))	{ bad("R14.5: uses continue"); }
		if (MATCH("..."))	{ bad("R16.1: uses elipsis"); }
		if (MATCH("union"))	{ bad("R18.4: uses union"); }

		if (MATCH("const_int"))
		{	s = cobra_txt();
			if (s[0] == '0' && isdigit((uchar)s[1]))
			{	bad("R7.1: octal constant");
		}	}

		if (TYPE("cpp")
		&&  strstr(cobra_txt(), "include"))
		{	if (done_cpp)
			{	bad("R19.1: #include preceded by code");
			}
			if (s_match(cobra_txt(), "'\\"))
			{	bad("R19.2: non-standard character in header name");
		}	}

		s = cobra_txt();
		while ((s = strstr(s, "??")) != NULL)
		{	s += 2;
			if (s_match(s, "=()/'<>!-"))
			{	bad("R4.2: using trigraph");
		}	}
	}
}
