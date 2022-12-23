#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
//#include <malloc.h>

#define Version	"Version 1.4 - 23 December 2022"

// convert from Cobra JSON format to either
//	JUnit or SARIF format
// initial prototype

#define JUNIT	0
#define SARIF	1

typedef struct Report Report;
typedef struct Reports Reports;

struct Report {
	char	*fnm;
	int	lnr;
	char	*msg;
	Report *nxt;
};

struct Reports {
	char	*nm;
	Report  *r;
	Reports *nxt;
};

static Reports *reports;
static unsigned long total_used;
static char *BaseId;
static int alternate;

static void
sarif_rules(void)
{	Reports *w;
	Report  *m;
	int cnt = 0;
	char *lastnm  = "";
	char *lastmsg = "";

	for (w = reports; w; w = w->nxt)
	for (m = w->r; m; m = m->nxt)
	{
		if (strcmp(w->nm, lastnm) == 0
		&&  strcmp(m->msg, lastmsg) == 0)
		{	continue;
		}
		lastnm  = w->nm;
		lastmsg = m->msg;

		printf("            {\n");
		if (alternate)
		{	printf("              \"id\": \"%s\",\n", w->nm); cnt++;
		} else
		{	printf("              \"id\": \"R%d\",\n", cnt++); // ruleId
		}
		printf("              \"fullDescription\": {\n");
		if (alternate)
		{	printf("                \"text\": \"%s\"\n", m->msg);
		} else
		{	printf("                \"text\": \"%s\"\n", w->nm);
		}
		printf("              }\n");
		printf("            }%s\n", w->nxt || m->nxt?",":"");
	}
}

static void
sarif_results(void)
{	Reports *w;
	Report  *m;
	int cnt = 0;
	char *lastnm  = "";
	char *lastmsg = "";

	if (alternate)
	{	cnt--;
	}
	for (w = reports; w; w = w->nxt)
	for (m = w->r; m; m = m->nxt /* , ++cnt */)
	{
		if (alternate)
		{	if (strcmp(w->nm, lastnm) != 0
			||  strcmp(m->msg, lastmsg) != 0)
			{	cnt++;
			}
			lastnm  = w->nm;
			lastmsg = m->msg;
		}

		printf("        {\n");
		if (alternate)
		{	printf("          \"ruleId\": \"%s\",\n", w->nm);
		} else
		{	printf("          \"ruleId\": \"R%d\",\n", cnt);
		}
		printf("          \"ruleIndex\": %d,\n", cnt);
		printf("          \"level\": \"warning\",\n");
		printf("          \"message\": {\n");

		if (alternate)
		{	printf("            \"text\": \"%s\"\n", m->msg);
		} else
		{	printf("            \"text\": \"%s\"\n", w->nm);
		}

		printf("          },\n");
		printf("          \"locations\": [\n");
		printf("            {\n");
		printf("              \"physicalLocation\": {\n");
		printf("                \"artifactLocation\": {\n");
		printf("                  \"uri\": \"%s\"\n", m->fnm);
		if (BaseId)
		{	printf("                  \"uriBaseId\": \"%s\",\n", BaseId);
		}
		printf("                },\n");
		printf("                \"region\": {\n");
		printf("                  \"startLine\": %d", m->lnr);
		if (strncmp(m->msg, "lines ", 6) == 0)
		{	int n, x, y;
			n = sscanf(m->msg, "lines %d..%d", &x, &y);
			if (n == 2 && x == m->lnr && y > x)
			{	printf(",\n                  \"endLine\": %d\n", y);
			} else
			{	printf("\n");
			}
		} else
		{	printf("\n");
		}
		printf("                }\n");
		printf("              }\n");	// }, if more fields follow
		printf("            }\n");
		printf("          ]\n");
		printf("        }%s\n", w->nxt || m->nxt?",":"");

		if (!alternate)
		{	cnt++;
		}
	}
}

void
reformat(const int mode)
{	Reports *r;
	Report  *t;

	if (mode == JUNIT)
	{	// https://help.catchsoftware.com/display/ET/JUnit+Format

		printf("<testsuites>\n");
		for (r = reports; r; r = r->nxt)
		{	printf("<testsuite name=\"%s\">\n", r->nm);
			printf(" <properties>\n");	// name/value pairs
			printf("  <property name=\"cobra\" value=\"warning\">\n");
			printf(" </properties>\n");
			for (t = r->r; t; t = t->nxt)
			{	printf(" <testcase name=\"match\">\n");
				printf(" <failure message=\"%s\">\n", t->msg);
				printf(" file=%s line=%d\n", t->fnm, t->lnr);
				printf(" </failure>\n");
				printf(" </testcase>\n");
			}
			printf("</testsuite>\n");
		}
		printf("<testsuites>\n");
	} else if (mode == SARIF)
	{	// https://docs.oasis-open.org/sarif/sarif/v2.1.0/csprd01/sarif-v2.1.0-csprd01.html

		printf("{\n");
		printf("  \"version\": \"2.1.0\",\n");
		printf("  \"$schema\": \"http://json.schemastore.org/sarif-2.1.0-rtm.5\",\n");
		printf("  \"runs\": [\n");
		printf("   {\n");
		printf("    \"tool\": {\n");
		printf("      \"driver\": {\n");
		printf("        \"name\": \"Cobra\",\n");
		printf("        \"version\": \"4.2\",\n");	// should come from input file...
		printf("        \"informationUri\": \"https://github.com/nimble-code/Cobra\",\n");
		printf("        \"rules\": [\n");
			sarif_rules();
		printf("        ]\n");
		printf("      }\n");
		printf("    },\n");
		printf("    \"results\": [\n");
			sarif_results();
		printf("    ]\n");
		printf("   }\n");
		printf("  ]\n");
		printf("}\n");
	} else
	{	fprintf(stderr, "mode %d not recognized\n", mode);
	}
}

static void *
emalloc(size_t size)	// size in bytes
{	void *n;
	int p;

	if ((p = size % sizeof(size_t)) > 0)
	{	size += sizeof(size_t) - p;
	}
	n = malloc(size * sizeof(char));

	if (!n)
	{	fprintf(stderr, "out of memory (%lu bytes - wanted %lu more)\n",
			total_used, (unsigned long) size);
		exit(1);
	}
	memset(n, 0, size);
	total_used += (unsigned long) size;

	return n;
}

static Reports *
new_named_set(const char *s)
{	Reports *q;

	for (q = reports; q; q = q->nxt)
	{	if (strcmp(q->nm, s) == 0)
		{	fprintf(stderr, "warning: set name %s already exists\n", q->nm);
			fprintf(stderr, "         this new copy hides the earlier one\n");
			break;
	}	}

	q = (Reports *) emalloc(sizeof(Reports));
	q->nm = (char *) emalloc(strlen(s)+1);	// new_named_set
	strcpy(q->nm, s);
	q->nxt = reports;
	reports = q;
	return q;
}

static void
add_report(const char *Type, const char *msg, const char *fnm, const int lnr)
{	Reports *x;
	Report  *r;

	for (x = reports; x; x = x->nxt)
	{	if (strcmp(x->nm, Type) == 0)
		{	break;
	}	}
	if (!x)
	{	x = new_named_set((const char *) Type);
	}

	r = (Report *) emalloc(sizeof(Report));
	r->fnm  = (char *) emalloc(strlen(fnm)+1);
	strcpy(r->fnm, fnm);
	r->lnr  = lnr;

	if (msg)
	{	r->msg = (char *) emalloc(strlen(msg)+1);
		strcpy(r->msg, msg);
	}

	r->nxt = x->r;
	x->r = r;
}

static int
json_import(const char *f)
{	FILE *fd;
	char *q, buf[1024];
	char fnm[512];
	char Type[512];
	char Msg[512];
	int a, b, c, lnr = -1;
	int ntp = 0;
	int nln = 0;
	int nfl = 0;
	int ncb = 0;
	int phase = 0;

	if ((fd = fopen(f, "r")) == NULL)
	{	fprintf(stderr, "cobra: cannot find file '%s'\n", f);
		return 0;
	}
	// we assume that every json record contains at least
	// the three fields: "type", "file", and "line"
	// and optionally also "cobra" which must be the last field
	// the rhs details for all except line must be enclosed in ""
again:
	strcpy(fnm, "");
	strcpy(Type, "");
	strcpy(Msg, "");
	while (fgets(buf, sizeof(buf), fd) != NULL)
	{	if ((q = strstr(buf, "\"type\"")) != NULL)
		{	q += strlen("\"type\"");
			while (*q != '\0' && *q != '"')
			{	q++;
			}
			if (*q == '"')
			{	char *r = q+1;
				while (*r != '\0' && *r != '"'
#ifdef TRUNCATE
				&& !isspace((int) *r)	// truncates at first space in "type" field
#endif
				)
				{	r++;
				}
				*r = '\0';
				strncpy(Type, q+1, sizeof(Type)-1);
				ntp++;
				if (phase == 1
				&&  strlen(fnm) > 0 && lnr >= 0)
				{	add_report(Type, Msg, fnm, lnr);
					strcpy(Type, "");
					lnr = -1;
			}	}
			continue;
		}
		if ((q = strstr(buf, "\"line\"")) != NULL)
		{	q += strlen("\"line\"");
			while (*q != '\0' && !isdigit((int) *q))
			{	q++;
			}
			if (isdigit((int) *q))
			{	lnr = atoi(q);
				nln++;
				if (phase == 1
				&& strlen(Type) > 0 && strlen(fnm) > 0)
				{	add_report(Type, Msg, fnm, lnr);
					strcpy(fnm, "");
					lnr = -1;
			}	}
			continue;
		}
		if ((q = strstr(buf, "\"file\"")) != NULL)
		{	q += strlen("\"file\"");
			while (*q != '\0' && *q != '"')
			{	q++;
			}
			if (*q == '"')
			{	char *r = q+1;
				while (*r != '\0' && *r != '"')
				{	r++;
				}
				*r = '\0';
				strncpy(fnm, q+1, sizeof(fnm)-1);
				nfl++;
				if (phase == 1
				&& strlen(Type) > 0 && lnr >= 0)
				{	add_report(Type, Msg, fnm, lnr);
					strcpy(fnm, "");
					lnr = -1;
			}	}
			continue;
		}
		if ((q = strstr(buf, "\"message\"")) != NULL)
		{	q += strlen("\"message\"");
			while (*q != '\0' && *q != '"')
			{	q++;
			}
			if (*q == '"')
			{	char *r = q+1;
				while (*r != '\0' && *r != '"')
				{	r++;
				}
				*r = '\0';
				strncpy(Msg, q+1, sizeof(Msg)-1);
			}
			continue;
		}
		q = buf;
		if (phase == 0
		&&  (q = strstr(q, "\"cobra\"")) != NULL)
		{	q += strlen("\"cobra\"");
			q = strchr(q, '"');
			if (q
			&&  strlen(Type) > 0
			&&  strlen(fnm) > 0
			&&  lnr >= 0
			&&  sscanf(q, "\"%d %d %d\"", &a, &b, &c) == 3)
			{	add_report(Type, Msg, fnm, lnr);
				ncb++;
				strcpy(fnm, "");
				lnr = -1;
	}	}	}
	if (phase == 1
	&&  strlen(Type) > 0
	&&  strlen(fnm) > 0
	&&  lnr >= 0)
	{	add_report(Type, Msg, fnm, lnr); // final report
	}
	fclose(fd);

	if (phase == 0 && ncb == 0)
	{	phase = 1;
		if (ntp == 0 || nfl == 0 || nln == 0)
		{	fprintf(stderr, "warning: no useable records found (file, line, or cobra fields missing)\n");
		} else
		{	if (nfl != nln || ntp != nfl)
			{	fprintf(stderr, "warning: not all records have type, file and line fields\n");
			}
			if ((fd = fopen(f, "r")) != NULL)	// reopen
			{	goto again;
	}	}	}

	return ntp;
}

static void
usage(void)
{
	// convert a json file format into JUnit format
	fprintf(stderr, "usage: json_convert [options] -f filename.json\n");
	fprintf(stderr, "  -V -- print version number and exit\n");
	fprintf(stderr, "  -v -- prints the nr of valid records read on standard output\n");
	fprintf(stderr, "  -a -- use an alternate output format for SARIF\n");
	fprintf(stderr, "  -junit -- produce output in JUNIT format (default)\n");
	fprintf(stderr, "  -sarif -- produce output in SARIF format\n");
	fprintf(stderr, "  -base=pathname -- add a uriBaseId field set to pathname (SARIF format)\n");
	fprintf(stderr, "   pathname must contain at least one / and no \" characters\n");
	fprintf(stderr, "  -f filename -- read JSON input from filename\n");
	exit(1);
}

int
main(int argc, char *argv[])
{	int ntp  = 0;
	int mode = JUNIT;	// default
	int verbose = 0;

	while (argc > 1 && argv[1][0] == '-')
	{	switch (argv[1][1]) {
		case 'a':
			alternate = 1 - alternate; // alternate Sarif format, BlueOrigin
			break;
		case 's':
			if (strcmp(argv[1], "-sarif") == 0)
			{	mode = SARIF;
			} else
			{	usage();
			}
			break;
		case 'b':
			if (strncmp(argv[1], "-base=", 6) == 0
			&&  strlen(argv[1]) > 6
			&&  strchr(argv[1], '/') != NULL
			&&  strchr(argv[1], '\"') == NULL)
			{	BaseId = &argv[1][6];
			} else
			{	usage();
			}
			break;
		case 'j':
			if (strcmp(argv[1], "-junit") == 0)
			{	mode = JUNIT;
			} else
			{	usage();
			}
			break;
		case 'f':
			argc--; argv++;
			ntp = json_import(argv[1]);
			break;
		case 'v':
			verbose++;
			break;
		case 'V':
			printf("%s %s\n", argv[0], Version);
			exit(0);
		default:
			break;
		}
		argc--; argv++;
	}

	if (ntp > 0)
	{	if (verbose)
		{	fprintf(stderr, "%d records\n", ntp);
		}
		reformat(mode);
	} else
	{	usage();
	}
	return 0;
}
