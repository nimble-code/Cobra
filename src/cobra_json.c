/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

#include "cobra_pre.h"

int	 json_format;
int	 json_plus;
int	 nr_json;
int	 p_matched;
int	 stream;
char	 bvars[128];
char	 json_msg[128];
char	*glob_te = ""; // optional type field in json output
char	*scrub_caption = "";
char	*cobra_texpr;		// -e xxx
Match	*matches;
Match	*old_matches;
Match	*free_match;
Named	*namedset;
Bound	*free_bound;

extern Prim	*prim;

static void
cleaned_up(const char *tp)
{	const char *p = tp;

	while (p && *p != '\0')
	{	switch (*p) {
		case '\\':	// strip, \ or " characters
			printf(" "); // replace with space
			break;
		case '"':
			printf("\\");	// insert escape
			// fall thru
		default:
			printf("%c", *p);
			break;
		}
		p++;
	}
}

static void
check_bvar(Prim *c)
{
	if ((c->mark & 4)	// bound variable
	&&  strlen(c->txt) + strlen(bvars) + 3 < sizeof(bvars))
	{	if (bvars[0] != '\0')
		{	strcat(bvars, ", ");
		}
		strcat(bvars, c->txt);
	}
}

void
clr_matches(int which)
{	Match *m, *nm;
	Bound *b, *nb;

	m = (which == NEW_M) ? matches : old_matches;

	for (; m; m = nm)
	{	nm = m->nxt;
		m->from = (Prim *) 0;
		m->upto = (Prim *) 0;
		m->nxt = free_match;
		free_match = m;
		for (b = m->bind; b; b = nb)
		{	nb = b->nxt;
			b->ref = (Prim *) 0;
			b->nxt = free_bound;
			free_bound = b;
	}	}

	if (which == NEW_M)
	{	matches     = (Match *) 0;
	} else
	{	old_matches = (Match *) 0;
	}
}

void
clear_matches(void)
{
	clr_matches(NEW_M);
}

Match *
findset(const char *s, int complain, int who)
{	Named *x;
	char *p;
	int n = 0;

	if ((p = strchr(s, ':')) != NULL)
	{	*p = '\0';
	}

//	printf("find set '%s' (caller: %d)\n", s, who);
	for (x = namedset; x; x = x->nxt)
	{	n++;
		if (strcmp(x->nm, s) == 0)
		{	return x->m;
	}	}

	if (complain && !json_format && !no_display && !no_match)
	{	printf("named set '%s' not found\n", s);
		printf("there are %d stored sets: ", n);
		for (x = namedset; x; x = x->nxt)
		{	printf("'%s', ", x->nm);
		}
		printf("\n");
	}

	return (Match *) 0;
}

void
add_match(Prim *f, Prim *t, Store *bd)
{	Match *m;
	Store *b;
	Bound *n;

	if (free_match)
	{	m = free_match;
		free_match = m->nxt;
		m->bind = (Bound *) 0;
	} else
	{	m = (Match *) emalloc(sizeof(Match), 105);
	}
	m->from = f;
	m->upto = t;

	for (b = bd; b; b = b->nxt)
	{	if (free_bound)
		{	n = free_bound;
			free_bound = n->nxt;
		} else
		{	n = (Bound *) emalloc(sizeof(Bound), 106);
		}
		n->ref = b->ref;
		n->nxt = m->bind;
		m->bind = n;
	}
	m->nxt = matches;
	matches = m;
	p_matched++;

	if (stream == 1)	// when streaming, print matches when found
	{	Prim *c, *r;
		if (json_format)
		{	printf("%s {\n", (nr_json>0)?",":"[");
			sprintf(json_msg, "lines %d..%d",
				f?f->lnr:0, t?t->lnr:0);
			memset(bvars, 0, sizeof(bvars));
			for (b = bd; b; b = b->nxt)
			{	if (b->ref
				&&  strlen(b->ref->txt) + strlen(bvars) + 3 < sizeof(bvars))
				{	if (bvars[0] != '\0')
					{	strcat(bvars, ", ");
					}
					strcat(bvars, b->ref->txt);
			}	}
			json_match(glob_te, json_msg, f?f->fnm:"", f?f->lnr:0); // add_match, streaming
			printf("}");
			nr_json++;
		} else
		{	printf("stdin:%d: ", f->lnr);
			for (c = r = f; c; c = c->nxt)
			{	printf("%s ", c->txt);
				if (c->lnr != r->lnr)
				{	printf("\nstdin:%d: ", c->lnr);
					r = c;
				}
				if (c == t)
				{	break;
			}	}
			printf("\n");
		}
		if (verbose && bd && bd->ref)
		{	printf("bound variables matched: ");
			while (bd && bd->ref)
			{	printf("%s ", bd->ref->txt);
				bd = bd->nxt;
			}
			printf("\n");
	}	}
}

void
new_named_set(const char *setname)
{	Named *q;

	for (q = namedset; q; q = q->nxt)
	{	if (strcmp(q->nm, setname) == 0)
		{	printf("warning: set name %s already exists\n", q->nm);
			printf("         this new copy hides the earlier one\n");
			break;
	}	}


	q = (Named *) emalloc(sizeof(Named), 100);
	q->nm = (char *) emalloc(strlen(setname)+1, 100);	// new_named_set
	strcpy(q->nm, setname);
	q->m = matches;
	q->nxt = namedset;
	namedset = q;
}

void
add_pattern(const char *s, Prim *from, Prim *upto)
{	Named *x;
	Match *om = matches;		// save old value

	matches = NULL;
	for (x = namedset; x; x = x->nxt)
	{	if (strcmp(x->nm, s) == 0)
		{	break;
	}	}
	if (!x)
	{	// strcpy(SetName, s);
		new_named_set((const char *) s);
		x = namedset;
	}
	add_match(from, upto, NULL);	// new pattern now only one in matches
	matches->nxt = x->m;		// if we're adding to the set
	x->m = matches;			// there's only one
	x->cloned = NULL;
	matches = om;			// restore
	p_matched++;
}

void
del_pattern(const char *s, Prim *from, Prim *upto)
{	Named *x;
	Match *m, *om;

	for (x = namedset; x; x = x->nxt)
	{	if (strcmp(x->nm, s) == 0)
		{	break;
	}	}
	if (!x)
	{	if (verbose)
		{	fprintf(stderr, "warning: del_pattern: no such set '%s'\n", s);
		}
		return;
	}
	om = NULL;
	for (m = x->m; m; om = m, m = m->nxt)
	{	if (m->from == from
		&&  m->upto == upto)
		{	if (om)
			{	om->nxt = m->nxt;
			} else
			{	x->m = m->nxt;
			}
			x->cloned = NULL;
			return;
	}	}
	if (verbose)
	{	fprintf(stderr, "warning: del_pattern: pattern not found in set %s\n", s);
	}
}

void
json_match(const char *te, const char *msg, const char *f, int ln)
{
	if (!te
	||  !msg
	||  !f)
	{	return;
	}
	printf("  { \"type\"\t:\t\"");
	 cleaned_up(te);
	printf("\",\n");

	printf("    \"message\"\t:\t\"");
	 cleaned_up(msg);
	printf("\",\n");

	if (json_plus)
	{	if (strlen(bvars) > 0)
		{	printf("    \"bindings\"\t:\t\"%s\",\n", bvars);
		}
		printf("    \"source\"\t:\t\"");
		show_line(stdout, f, 0, ln, ln, -1);
		printf("\",\n");
	}

	printf("    \"file\"\t:\t\"%s\",\n", f);
	printf("    \"line\"\t:\t%d\n", ln);
	printf("  }\n");
}

int
do_markups(const char *setname)
{	Match *m;
	Bound *b;
	int seq = 1;
	int w = 0;
	int p = 0;

	for (m = matches; m; seq++, m = m->nxt)
	{	p++;	// nr of patterns
		if (m->bind && !json_format)
		{	if (verbose && w++ == 0)
			{	printf("bound variables matched:\n");
			}
			for (b = m->bind; b; b = b->nxt)
			{	if (!b->ref)
				{	continue;
				}
				w++;	// nr of bound vars
				if (strlen(setname) == 0)
				{	b->ref->mark |= 4;	// the bound variable
				}
				if (verbose)
				{	printf("\t%d:%d %s:%d: %s\n", seq, w,
						b->ref->fnm,
						b->ref->lnr,
						b->ref->txt);
	}	}	}	}

	if (!json_format && !no_match && !no_display)
	{	if (strlen(setname) == 0)
		{	printf("%d patterns\n", p);
		}
		if (w > 0)
		{	printf("%d bound variable%s\n",
				w, (w==1)?"":"s");
	}	}

	return p;
}

int
matches2marks(int doclear)
{	Match	*m;
	Prim	*p;
	int	loc_cnt;

	loc_cnt = 0;
	for (m = matches; m; m = m->nxt)
	{	p = m->from;
		p->bound = m->upto;
		if (p)
		{	if (!p->mark)
			{	loc_cnt++;
			}
			p->mark |= 2;	// start of pattern
			for (; p; p = p->nxt)
			{	if (!p->mark)
				{	loc_cnt++;
				}
				p->mark |= 1;
				if (p == m->upto)
				{	p->mark |= 8;
					break;
	}	}	}	}

	if (doclear)
	{	clear_matches();
	}
	if (!json_format && !no_match && !no_display)
	{	printf("%d token%s marked\n", loc_cnt, (loc_cnt==1)?"":"s");
	}

	return loc_cnt;
}

int
json_convert(const char *s)	// convert pattern matches to marks
{	Match *m = matches;
	int n = 0;

	matches = findset(s, 1, 1);
	if (matches)
	{	(void) do_markups(s);
		n = matches2marks(0);
	}
	matches = m;
	return n;
}

void
json(const char *te)
{	Prim *sop = (Prim *) 0;
	Prim *q, *mycur;
	int seen = 0;

	// usage:
	//	json		# prints json format summary of all matches
	//	json string	# puts string the type field

	// tokens that are part of a match are marked         &1
	// the token at start of each pattern match is marked &2
	// tokens that match a bound variable are marked      &4
	// the token at end of each pattern match is marked   &8

	// if the json argument is a number or a single *, then
	// the output is verbose for that match (or all, for a *)

	memset(bvars, 0, sizeof(bvars));
	printf("[\n");
	for (mycur = prim; mycur; mycur = mycur?mycur->nxt:0)
	{	if (mycur->mark)
		{	seen |= 2;
		}
		if (!(mycur->mark & 2))	// find start of match
		{	continue;
		}
		if (sop)
		{	printf(",\n");
		}
		// start of match
		sop = mycur;

		q = mycur;
		if (q->bound)	// assume this wasnt set for other reasons...
		{	sprintf(json_msg, "lines %d..%d", q->lnr, q->bound->lnr);
			while (q && q->mark > 0)
			{	check_bvar(q);
				q = q->nxt;
			}
		} else
		{	while (q && q->mark > 0)
			{	check_bvar(q);
				if (q->nxt)
				{	q = q->nxt;
				} else
				{	break;
			}	}
			sprintf(json_msg, "lines %d..%d", sop->lnr, q?q->lnr:0);
		}
		json_match(te, json_msg, sop?sop->fnm:"", sop?sop->lnr:0);	// json()
		seen |= 4;
	}
	if (seen == 2)	// saw marked tokens, but no patterns, find ranges to report
	{	for (mycur = prim; mycur; mycur = mycur?mycur->nxt:0)
		{	if (!mycur->mark)
			{	continue;
			}
			sop = mycur;
			while (mycur && mycur->mark)
			{	if (mycur->nxt)
				{	mycur = mycur->nxt;
				} else
				{	break;
			}	}
			sprintf(json_msg, "lines %d..%d", sop->lnr, mycur?mycur->lnr:0);
			json_match(te, json_msg, sop?sop->fnm:"", sop?sop->lnr:0); // json()
		}
	}
	printf("\n]\n");
}

void
show_line(FILE *fdo, const char *fnm, int n, int from, int upto, int tag)
{	FILE *fdi;
	char buf[MAXYYTEXT];
	int ln = 0;
	static int wcnt = 1;

	if (scrub && strlen(scrub_caption) > 0)
	{	fprintf(fdo, "cobra%05d <Med> :%s:%d: %s\n",
			wcnt++, fnm, tag, scrub_caption);
		if (upto == from)
		{	return;
	}	}

	if ((fdi = fopen(fnm, "r")) == NULL)
	{	printf("cannot open '%s'\n", fnm);
		return;
	}

	while (fgets(buf, sizeof(buf), fdi) && ++ln <= upto)
	{	if (ln >= from)
		{	if (scrub)
			{	if (buf[0] == '\n' || buf[0] == '\r')
				{	continue;
				}
				if (strlen(scrub_caption) > 0)
				{	fprintf(fdo, "\t");
				} else
				{	fprintf(fdo, "cobra%05d <Med> :%s:%d: ",
						wcnt++, fnm, ln);
				}
				fprintf(fdo, "%c", (ln == tag && upto > from)?'>':' ');
				goto L;
			}
			if (n > 0 && !cobra_texpr)
			{	fprintf(fdo, "%3d: ", n);
			}
			if (gui)
			{	fprintf(fdo, "%c%s:%05d:  ",
					(ln == tag && upto > from)?'>':' ',
					fnm, ln);
			} else if (tag >= 0)
			{	fprintf(fdo, "%c%5d  ",
					(ln == tag && upto > from)?'>':' ',
					ln);
			} else	// tag < 0 -> json output
			{	char *q, *p = buf;
				while (*p == ' ' || *p == '\t')
				{	p++;
				}
				while ((q = strchr(p, '\n'))
				||     (q = strchr(p, '\r')))
				{	*q = '\0';
				}
				// remove escape characters and double quotes
				for (q = p; *q != '\0'; q++)
				{	if (*q == '"')
					{	*q = '\'';
					} else if (*q == '\\')
					{	*q = ' ';
					} else if (*q == '/' && *(q+1) == '/')
					{	*q = '\0';
						break;
				}	}
				fprintf(fdo, "%s", p);
				continue;
			}
L:			fprintf(fdo, "%s", buf);
	}	}

	fclose(fdi);
}
