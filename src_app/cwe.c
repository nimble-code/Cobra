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

#define A	1
#define B	2
#define C	4

typedef struct Options Options;
struct Options {
	char	*pattern;
	int	*target;
	int	 orcode;
} options[] = {
	{ "cwe_119 ",	&cwe119,	A|B|C },
	{ "cwe_119_1 ",	&cwe119,	A     },
	{ "cwe_119_2 ",	&cwe119,	  B   },
	{ "cwe_119_3 ",	&cwe119,	    C },
	{ "cwe_120 ",	&cwe120,	A|B|C },
	{ "cwe_120_1 ",	&cwe120,	A     },
	{ "cwe_120_2 ",	&cwe120,	  B   },
	{ "cwe_120_3 ",	&cwe120,	    C },
	{ "cwe_131 ",	&cwe131,	A     },
	{ "cwe_134 ",	&cwe134,	A     },
	{ "cwe_170 ",	&cwe170,	A     },
	{ "cwe_197 ",	&cwe197,	A     },
	{ "cwe_468 ",	&cwe468,	A     },
	{ "cwe_805 ",	&cwe805,	A     },
	{ "cwe_416 ",	&cwe416,	A     },
	{ "cwe_457 ",	&cwe457,	A     },
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
	{	printf("cwe_%s\n", s);
	}
	fflush(stdout);
}

void
cobra_main(void)
{	int i, found = 0;

	if (!prim)
	{	return;
	}

	set_multi();

	if (!cwe_args)
	{	cwe119 = A|B|C;
		cwe120 = A|B|C;
		cwe131 = A;
		cwe134 = A;
		cwe170 = A;
		cwe197 = A;
		cwe468 = A;
		cwe805 = A;
		cwe416 = A;
		cwe457 = A;
	} else
	{	for (i = 0; options[i].pattern; i++)
		{	if (strstr(cwe_args, options[i].pattern))
			{	*(options[i].target) |= options[i].orcode;
				found++;
		}	}

		if (!found)
		{	fprintf(stderr, "saw: '%s'\n", cwe_args);
			fprintf(stderr, "usage: cwe [cwe_119[_[123]]] [cwe_120[_[123]]] [cwe_131] [cwe_134]\n");
			fprintf(stderr, "	    [cwe_170] [cwe_197] [cwe_468] [cwe_805] [cwe_416] [cwe_457]\n");
			fprintf(stderr, "	if no args are given, all checks are enabled\n");
			exit(1);
	}	}
	if (runtimes)
	{	ini_timers();
		start_timer(0);
		start_timer(1);
	}
#ifdef MEASURE_STARTUP_TIME
	exit(0);
#endif
	
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
