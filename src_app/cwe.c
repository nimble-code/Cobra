#include "cwe.h"

int cwe119;
int cwe120;
int cwe131;
int cwe134;
int cwe170;
int cwe197;
int cwe468;
int cwe805;
int cwe416;
int cwe457;

// end of redundancies

#define A	1
#define B	2
#define C	4

typedef struct Options Options;
struct Options {
	char	*pattern;
	int	*target;
	int	 orcode;
} options[] = {
	{ "119 ",	&cwe119,	A|B|C },
	{ "119_1 ",	&cwe119,	A     },
	{ "119_2 ",	&cwe119,	  B   },
	{ "119_3 ",	&cwe119,	    C },
	{ "120 ",	&cwe120,	A|B|C },
	{ "120_1 ",	&cwe120,	A     },
	{ "120_2 ",	&cwe120,	  B   },
	{ "120_3 ",	&cwe120,	    C },
	{ "131 ",	&cwe131,	A     },
	{ "134 ",	&cwe134,	A     },
	{ "170 ",	&cwe170,	A     },
	{ "197 ",	&cwe197,	A     },
	{ "468 ",	&cwe468,	A     },
	{ "805 ",	&cwe805,	A     },
	{ "416 ",	&cwe416,	A     },
	{ "457 ",	&cwe457,	A     },
	{ 0, 0, 0 }
};

void
note(const char *s)
{
	if (runtimes)
	{	stop_timer(0, 1, "CWE");
		start_timer(0);
	}

	if (verbose)
	{	fprintf(stderr, "cwe %s\n", s);
	}
	fflush(stdout);
	fflush(stderr);
}

static void
set_default(void)
{
	cwe119 = A|B|C;
	cwe120 = A|B|C;
	cwe131 = A;
	cwe134 = A;
	cwe170 = A;
	cwe197 = A;
	cwe468 = A;
	cwe805 = A;
	cwe416 = A;
	cwe457 = A;
}

static void
usage(void)
{
	fprintf(stderr, "cwe: unrecognized -- option(s): '%s'\n", backend);
	fprintf(stderr, "cwe: valid options include:\n");
	fprintf(stderr, "           --json  produce basic output in JSON format\n");
	fprintf(stderr, "           --json+ produce more verbose output in JSON format\n");
	fprintf(stderr, "cwe: options to enable specific cwe checks are:\n");
	fprintf(stderr, "	    --cwe119	(enables 119_1, 119_2, and 119_3)\n");
	fprintf(stderr, "	    --cwe119_1\n");
	fprintf(stderr, "	    --cwe119_2\n");
	fprintf(stderr, "	    --cwe119_3\n");
	fprintf(stderr, "	    --cwe120	(enables 120_1, 120_2, and 120_3)\n");
	fprintf(stderr, "	    --cwe120_1\n");
	fprintf(stderr, "	    --cwe120_2\n");
	fprintf(stderr, "	    --cwe120_3\n");
	fprintf(stderr, "	    --cwe131\n");
	fprintf(stderr, "	    --cwe134\n");
	fprintf(stderr, "	    --cwe170\n");
	fprintf(stderr, "	    --cwe197\n");
	fprintf(stderr, "	    --cwe416\n");
	fprintf(stderr, "	    --cwe457\n");
	fprintf(stderr, "	    --cwe468\n");
	fprintf(stderr, "	    --cwe805\n");
	fprintf(stderr, "cwe: if no -- options are specified, all checks are enabled\n");
	fprintf(stderr, "note: warnings and error output is always written to stderr\n");
	exit(1);
}

static void
handle_args(void)
{	int i, n;
	int found, subset = 0;
	char *s = backend;

	while (strlen(s) > 0)
	{	if (strncmp(s, "json", 4) == 0)
		{	json_format = 1;
			if (s[4] == '+')
			{	json_plus = 1;
				s++;
			}
			s += 5;		// +1 for trailing space
		} else if (strncmp(s, "cwe", 3) != 0)
		{	usage();
		} else
		{	s += 3;		// skip "cwe" prefix
			if (s[0] == '_')	// allow also "cwe_"
			{	s++;
			}
			found = 0;	// patterns include trailing space
			for (i = 0; options[i].pattern; i++)
			{	n = strlen(options[i].pattern);
				if (strncmp(s, options[i].pattern, n) == 0)
				{	*(options[i].target) |= options[i].orcode;
					found++;
					s += n;
					subset++;
					break;
			}	}
			if (!found)
			{	usage(); // no return
	}	}	}

	if (!subset)
	{	set_default();
	}
}

void
cobra_main(void)
{
	handle_args();	// eg --help

	if (!prim)
	{	return;
	}

	set_multi();

	if (runtimes)
	{	ini_timers();
		start_timer(0);
		start_timer(1);
	}
#ifdef MEASURE_STARTUP_TIME
	exit(0);
#endif
	if (json_format && no_display)
	{	fprintf(stderr, "cwe: note: json format disabled by -terse option\n");
	}
	if (cwe119 & A) { note("119_1"); cwe119_1(); }
	if (cwe119 & B) { note("119_2"); cwe119_2(); }
	if (cwe119 & C) { note("119_3"); cwe119_3(); }
	if (cwe120 & A) { note("120_1"); cwe120_1(); }
	if (cwe120 & B) { note("120_2"); cwe120_2(); }
	if (cwe120 & C) { note("120_3"); cwe120_3(); }
	if (cwe131 & A) { note("131"); cwe131_0(); }
	if (cwe134 & A) { note("134"); cwe134_0(); }
	if (cwe170 & A) { note("170"); cwe170_0(); }
	if (cwe197 & A) { note("197"); cwe197_0(); }
	if (cwe468 & A) { note("468"); cwe468_0(); }
	if (cwe416 & A) { note("416"); cwe416_0(); }
	if (cwe457 & A) { note("457"); cwe457_0(); }
	if (cwe805 & A) { note("805"); cwe805_0(); }

	note("done");
	if (runtimes)
	{	stop_timer(1, 1, "CWE");
	}
	exit(0);
}
