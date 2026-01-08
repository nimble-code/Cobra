#include "find_taint.h"

static ConfigMap configmap[] = {
	{ "Taint Sources",	TAINTSOURCES },
	{ "Importers",		IMPORTERS },
	{ "Propagators",	PROPAGATORS },
	{ "Ignore",		IGNORE },
	{ "Targets",		TARGETS },
	{ 0, 0 }
};

ConfigDefs *configged[NONE];

static int
flag_problems(void)
{	int i;
	ConfigDefs *x, *y;

	for (i = 0; i < NONE; i++)
	for (x = configged[i]; x; x = x->nxt)
	for (y = x->nxt; y; y = y->nxt)
	{	if (strcmp(x->nm, y->nm) == 0)
		{	fprintf(stderr, "duplicate entry '%s' under %s\n",
				x->nm,
				configmap[i].heading);
			// only the first one in the list will count
			return 0;
	}	}
	return 1;	// all good
}

static void
show_configs(FILE *fd)
{	ConfigDefs *x;
	int i;

	fprintf(fd, "taint_config: configuration data read:\n");
	fprintf(fd, "#  propagators format:\n");
	fprintf(fd, "#	fct_name\tsource\tdest\n");
	fprintf(fd, "#  importers format:\n");
	fprintf(fd, "#	fct_name\tsource of tainted data\n");
	fprintf(fd, "#  targets format:\n");
	fprintf(fd, "#	fct_name\tvulnerable arg\n");
	for (i = 0; i < NONE; i++)
	{	fprintf(fd, "%s\n", configmap[i].heading);
		for (x = configged[i]; x; x = x->nxt)
		{	fprintf(fd, "\t%s", x->nm);
			if (x->from)
			{	fprintf(fd, "\t%d", x->from);
				if (x->into)
				{	fprintf(fd, "\t%d", x->into);
			}	}
			fprintf(fd, "\n");
	}	}
}

static ConfigDefs *
new_config(const char *nm, int curtype, int from, int into)
{	ConfigDefs *x;

	x = (ConfigDefs *) emalloc(sizeof(ConfigDefs), 14);
	x->nm = (char *) emalloc(strlen(nm)+1, 15);
	strcpy(x->nm, nm);
	x->from = from;
	x->into = into;
	x->nxt = configged[curtype];
	configged[curtype] = x;

	if (verbose)
	{	fprintf(stderr, "'%s' add '%s'\n", configmap[curtype].heading, nm);
	}

	return x;
}

static void
markit(Prim *q, int m)
{
	q->mark |= ExternalSource;
	if (verbose > 1)
	{	fprintf(stderr, "%s:%d: marked gets arg '%s' (+%d)\n",
			q->fnm, q->lnr, q->txt, m);
	}
}

static int
mark_str_args(Prim *q, Prim *r, int m)	// q is the format string, r is bound on args
{	char *s = q->txt;	// to locate %... specifiers in q->txt
	int cnt = 0;

	do {	q = q->nxt;		// first arg after format string
	} while (q && strcmp(q->typ, "ident") != 0 && q->seq < r->seq);

	while (q && q->seq < r->seq && (s = strchr(s, '%')) != 0)
	{	s++;
		switch (*(++s)) {
		case '%':
			s++;
			break;
		case 's':
			q->mark |= m;
			cnt++;
			if (verbose > 1)
			{	fprintf(stderr, "%s:%d: marked scanf arg '%s' (+%d)\n",
					q->fnm, q->lnr, q->txt, m);
			}
			// fall thru
		default:
			q = next_arg(q, r);
			break;
	}	}

	return cnt;
}

static int
in_context(Prim *p, const char *f)
{
	if (!f)
	{	return 1;
	}

	if (p->curly > 0)
	{	while (p && p->curly > 0)
		{	p = p->prv;	// start of fct body
		}
		while (p && p->txt[0] != ')')
		{	p = p->prv;	// end of param list
		}
		if (p && p->jmp)
		{	p = p->jmp;	// start of param list
			p = p->prv;	// fct name
			if (strcmp(p->txt, f) == 0)
			{	return 1;
	}	}	}

	return 0;
}

static char *
get_base(void)
{	char *p, *q;

	q = (char *) emalloc(
		 strlen(C_BASE)
		-strlen("/rules")
		+strlen("/configs/taint_cfg.txt")+1, 16);

	strcpy(q, C_BASE);
	p = strstr(q, "/rules");
	if (!p)
	{	fprintf(stderr, "could not locate rules directory\n");
		return NULL;
	}
	*p = '\0';

	return q;
}

int
handle_taintsources(Prim *p)
{	ConfigDefs *x;
	int cnt = 0;

	if (!p)
	{	return 0;
	}

	for (x = configged[TAINTSOURCES]; x; x = x->nxt)
	{	if (strcmp(p->txt, x->nm) == 0	// eg argv in any context
		&&  in_context(p, x->fct))
		{	p->mark |= ExternalSource;
			cnt++;
			if (verbose > 1)
			{	fprintf(stderr, "%s:%d: marked '%s' (+%d)\n",
					p->fnm, p->lnr, p->txt, ExternalSource);
			}
			continue;
	}	}

	return cnt;
}

int
handle_importers(Prim *p)
{	ConfigDefs *x;
	Prim *q, *r;
	int cnt = 0;

	if (!p)
	{	return 0;
	}

	for (x = configged[IMPORTERS]; x; x = x->nxt)
	{
		if (strcmp(p->txt, x->nm) == 0)	// eg gets, fgets, getline, getdelim, scanf, fscanf
		{	int n = x->from;	// just 1 arg
			q = p->nxt;

			if (q->txt[0] != '('
			||  !q->jmp)
			{	continue;
			}
			r = q->jmp;	// bounds search for args
			q = q->nxt;	// first arg
			while (q && n > 1)
			{	q = next_arg(q, r);
				n--;
			}
			if (!q
			||  strcmp(q->typ, "ident") != 0)
			{	continue;
			}
			if (strstr(p->txt, "scanf") != NULL)	// predefined
			{	// check for unbounded %s in format string
				q = p->nxt;
				while (q
				&&     q->seq < r->seq
				&&     strcmp(q->typ, "str") != 0)	// find format string
				{	q = q->nxt;
				}
				if (!q
				||  strcmp(q->typ, "str") != 0
				||  strstr(q->txt, "%s") == NULL)
				{	continue;	// no explicit format string, or no %s
				}
				// find which arg this corresponds to
				cnt += mark_str_args(q, r, ExternalSource);
			} else
			{	markit(q, ExternalSource);
				cnt++;
				if (strcmp(p->txt, "gets") == 0)
				{	ngets++;
					if (verbose
					|| !no_match
					|| !no_display) // ie no -terse argument
					{	fprintf(stderr, " %3d: %s:%d: warning: unbounded call to gets()\n",
							++warnings, p->fnm, p->lnr);
	}	}	}	}	}

	return cnt;
}

Prim *
start_args(Prim *p)
{
	if (!p)
	{	return NULL;
	}
	p = p->nxt;
	if (p->txt[0] != '('
	||  !p->jmp)
	{	return NULL;
	}
	return p;
}

Prim *
pick_arg(Prim *p, Prim *r, int n)
{
	if (p)
	{	p = p->nxt;	// first arg
		while (p
		&&  p->seq < r->seq
		&&  strcmp(p->typ, "ident") != 0)
		{	p = p->nxt;
		}
		while (p && n-- > 1)
		{	p = next_arg(p, r);
	}	}

	return p;
}

int
cfg_ignore(const char *s)
{	ConfigDefs *x;
	int h;

	for (h = 0; h < NONE; h++)
	for (x = configged[h]; x; x = x->nxt)
	{	if (strcmp(x->nm, s) == 0)
		{	return 1;	// handled separately
	}	}
	return 0;
}

Prim *
next_arg(Prim *p, const Prim *upto)
{
	while (p
	&& p->txt[0] != ','
	&& p->seq < upto->seq)
	{	p = p->nxt;
	}

	while (p
	&& strcmp(p->typ, "ident") != 0
	&& p->seq < upto->seq)
	{	p = p->nxt;	// skip over &, *, etc
	}

	if (!p
	||  p->seq >= upto->seq)
	{	return NULL;
	}

	return p;	// point at arg
}

int
is_propagator(const char *s)
{	ConfigDefs *x;

	if (s)
	for (x = configged[PROPAGATORS]; x; x = x->nxt)
	{	if (strcmp(s, x->nm) == 0)
		{	return 1;
	}	}

	return 0;
}

int
taint_configs(void)
{	FILE *fd;
	char buf[512];
	char nm[512];
	char opt_fct[512];
	int curtype = 0;
	int i, from, into;
	char *p, *q;
	ConfigDefs *x;

	if (!Cfg)	// no user defined configuration file
	{	q = get_base();
		if (!q)
		{	return 0;
		}
		strcat(q, "/configs/taint_cfg.txt");
	} else if (strchr(Cfg, '/'))	// full pathname
	{	q = Cfg;
	} else				// basename
	{	q = get_base();
		if (!q)
		{	return 0;
		}
		strcat(q, "/");
		strcat(q, Cfg);		// located in configs dir
	}

	if ((fd = fopen(q, "r")) == NULL)
	{	fprintf(stderr, "taint_configs: no file '%s' found\n", q);
		return 0;
	}

	while (fgets(buf, sizeof(buf), fd))
	{	if ((p = strchr(buf, '\n')) != NULL)
		{	*p = '\0';
		}
		for (i = 0; configmap[i].heading; i++)
		{	if (strstr(buf, configmap[i].heading) != NULL)
			{	curtype = configmap[i].type;
				if (curtype < 0
				||  curtype >= NONE)
				{	fprintf(stderr, "config: bad header type %d\n",
						curtype);
					return 0;
				}
				break;
		}	}
		if (buf[0] == '#') // ignored commented out entries
		{	continue;
		}
		if (curtype == TAINTSOURCES)	// format:  varname [fctname]
		{	i = sscanf(buf, "%s %s", nm, opt_fct);
			from = into = 0;
			if (i < 1 || i > 2)
			{	continue;
			}
			x = new_config(nm, curtype, from, into);
			if (i == 2)
			{	x->fct = (char *) emalloc(strlen(opt_fct)+1, 17);
				strcpy(x->fct, opt_fct);
			}
		} else				// format:  fctname [from] into
		{	i = sscanf(buf, "%s %d %d", nm, &from, &into);
			switch (i) {
			case 1:
				from = 0;
				// fall thru
			case 2:
				into = 0;
				// fall thru;
			case 3:
				break;
			default:
				continue; // while
			}
			x = new_config(nm, curtype, from, into);
	}	}
	fclose(fd);

	taint_init();	// initialize thread data structures

	if (verbose)
	{	show_configs(stderr);
	}

	return flag_problems();
}
