/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at https://codescrub.com/cobra
 */

#include "cobra_pre.h"
typedef unsigned int	uint;
#include "cobra_te.h"

#define Emalloc(a, b)	(void *) ((Ncore==1)?emalloc(a,b):hmalloc(a,ix,b)) // also in: cobra.h

int	 json_format;
int	 json_plus;
int	 nr_json;
int	 p_matched;
int	 stream;
char	 bvars[1024];
char	 json_msg[1024];
char	*glob_te = ""; // optional type field in json output
char	*scrub_caption = "";
char	*cobra_texpr;		// -e xxx
Match	*matches;
Match	*old_matches;
Named	*namedset;

static Bound	*free_bound;

static void
cleaned_up(FILE *fd, const char *tp)
{	const char *p = tp;

	// intercept especially " and things like \n
	// the argument tp is always quoted

	while (p && *p != '\0')
	{	switch (*p) {
		case '\\':
			fprintf(fd, " ");
			break;
		case '\"':
		case '\'':
#ifdef ESCAPE_COLON
		// GitHub issue PR #48
		case ':':
#endif
			fprintf(fd, "\\");
			// fall thru
		default:
			fprintf(fd, "%c", *p);
			break;
		}
		p++;
	}
}

static void
check_bvar(Prim *c)
{	char buf[512];

	if ((c->mark & (4|16)) == 0
	||   strlen(c->txt) + 16 >= sizeof(buf)-1)
	{	return;
	}

	if (c->mark & 4)	// matched
	{	sprintf(buf, "->%d", c->lnr);
	} else 			// defined
	{	snprintf(buf, sizeof(buf), "%s %d", c->txt, c->lnr);
	}
	if (strstr(bvars, buf))
	{	return;
	}
	if (strlen(c->txt) + strlen(bvars) + 3 < sizeof(bvars))
	{	if (bvars[0] != '\0'
		&&  (c->mark&16))
		{	strcat(bvars, ", ");	// check_bvar, from markings
		}
		strcat(bvars, buf);
	}
}

void
put_bound(Bound *n)
{
	n->bdef = (Prim *) 0;
	n->ref  = (Prim *) 0;
	n->aref = (Prim *) 0;
	n->nxt = free_bound;
	free_bound = n;
}

void
clear_bounds(Prim *b)
{
	if (b)
	{	clear_bounds(b->bound);
		b->bound = (Prim *) 0;
	}
}

Named *
findset(const char *s, int complain)
{	Named *x;
	char *p;
	int n = 0;

	if ((p = strchr(s, ':')) != NULL)
	{	*p = '\0';
	}

	for (x = namedset; x; x = x->nxt)
	{	n++;
		if (strcmp(x->nm, s) == 0
		&&  strlen(x->nm) == strlen(s))
		{	return x;
	}	}

	if (complain && !json_format && !no_display && !no_match)
	{	fprintf(stderr, "named set '%s' not found\n", s);
		fprintf(stderr, "there are %d stored sets: ", n);
		for (x = namedset; x; x = x->nxt)
		{	fprintf(stderr, "'%s', ", x->nm);
		}
		fprintf(stderr, "\n");
	}

	return (Named *) 0;
}

int
setexists(const char *s)
{	Named *x;

	for (x = namedset; x; x = x->nxt)
	{	if (strcmp(x->nm , s) == 0)
		{	return (x->m != NULL)?1:0;
			break;
	}	}

	return 0;
}

Bound *
get_bound(void)
{	Bound *n;

	if (free_bound)
	{	n = free_bound;
		free_bound = n->nxt;
	} else
	{	n = (Bound *) emalloc(sizeof(Bound), 106);
	}

	return n;
}

void
new_named_set(const char *setname)
{	Named *q;

	q = findset(setname, 0);
	if (q)
	{	if (verbose)
		{ fprintf(stderr, "warning: set name %s already exists\n", q->nm);
		  fprintf(stderr, "         the earlier set is now renamed _%s\n", q->nm);
		}
		q->nm = (char *) emalloc(strlen(setname)+2, 100);
		sprintf(q->nm, "_%s", setname);
	}

	q = (Named *) emalloc(sizeof(Named), 100);
	q->nm = (char *) emalloc(strlen(setname)+1, 100);	// new_named_set
	strcpy(q->nm, setname);
	q->m = matches;			// new_named_set
	q->nxt = namedset;
	namedset = q;
}

void
del_pattern(const char *s, Prim *from, Prim *upto, int cid)
{	Named *x;
	Match *m, *om;

	do_lock(cid, 4);	// del_pattern

	x = findset(s, 0);
	if (!x)
	{	if (verbose)
		{	fprintf(stderr, "warning: del_pattern: no such set '%s'\n", s);
		}
		do_unlock(cid, 4);
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
			}	// could recycle m
			x->cloned = NULL;
			break;
	}	}
	if (!m && verbose)
	{	fprintf(stderr, "warning: del_pattern: pattern not found in set %s\n", s);
	}
	do_unlock(cid, 4);
}

static void
cobra_export(const Prim *from, const Prim *upto)
{	int n = 0, m = 0;
	Prim *q = (Prim *) from;
	FILE *fd = (track_fd) ? track_fd : stdout;

	if (!q)
	{	return;
	}

	while (q && q->lnr == from->lnr)
	{	n++;
		q = q->prv;
	}
	if (upto && upto->seq > from->seq)
	{	q = (Prim *) from;
		while (q && q->seq < upto->seq)
		{	m++;
			q = q->nxt;
	}	}
	// last field of json record
	fprintf(fd, "    \"cobra\"\t:\t\"%d %d %d\"\n", no_cpp, n, m);
}

static int
json_add(const char *tp, const char *msg, const char *fnm, const int lnr, int pos, int len, int ix)
{	Files *f;
	Prim *p, *q;
	int cnt = 0;
	static char lastname[516];
#if 0
	int opos = pos;
#endif

	f = findfile(fnm);

	if (!f)
	{	if (strcmp(lastname, fnm) != 0)
		{	fprintf(stderr, "warning: cannot find '%s'\n", fnm);
		}
		strncpy(lastname, fnm, sizeof(lastname)-1);
		return 0;
	}
	p = f->first_token;
	while (p && p != f->last_token)
	{	if (p->lnr == lnr)
		{	while (p && pos > 1)
			{	p = p->nxt;
				pos--;
			}
			if (!p)
			{	break;
			}

			p->mark++;
			cnt++;
			q = p;
			while (q && len > 0)
			{	q = q->nxt;
				q->mark++;
				cnt++;
				len--;
			}
			if (tp && strlen(tp) > 0)
			{	add_pattern(tp, msg, p, q, ix);
			}
			break; // marked token position(s)
		}
		p = p->nxt;
	}
	return cnt;
}

static int
json_add2(const char *tp, const char *msg, const char *fnm, const int lnr, int ix)	// if we have no pos and len
{	Files *f;
	Prim *p;
	int cnt = 0;
	static char lastname[516];

	f = findfile(fnm);
	if (!f)
	{	if (strcmp(lastname, fnm) != 0)
		{	fprintf(stderr, "warning: cannot find '%s'\n", fnm);
		}
		strncpy(lastname, fnm, sizeof(lastname)-1);
		return 0;
	}
	p = f->first_token;
	while (p && p != f->last_token)
	{	if (p->lnr == lnr)
		{	while (p && p->lnr == lnr)
			{	if (tp && strlen(tp) > 0)
				{	add_pattern(tp, msg, p, p, ix);
				}
				p->mark++;
				p = p->nxt;
				cnt++;
			}
			break; // marked token position(s)
		}
		p = p->nxt;
	}
	return cnt;
}

void
json_import(const char *f, int ix)
{	FILE *fd;
	char *q, buf[1024];
	char fnm[512];
	char Type[512];
	char Msg[512];
	char Src[512];
	int a, b, c, lnr = -1;
	int once = 0;
	int cnt = 0;
	int ntp = 0;
	int nln = 0;
	int nfl = 0;
	int ncb = 0;
	int phase = 0;

	assert(ix == 0);

	if ((fd = fopen(f, "r")) == NULL)
	{	fprintf(stderr, "cobra: cannot find file '%s'\n", f);
		return;
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
				while (*r != '\0'
				&& *r != '"'
				&& *r != '\n'
				&& !isspace((int) *r))	// first word is used as pattern setname
				{	r++;
				}
				*r = '\0';
				strncpy(Type, q+1, sizeof(Type)-1);
				ntp++;		// nr of type fields read
				if (phase == 1
				&&  strlen(fnm) > 0 && lnr >= 0)
				{	cnt += json_add2(Type, Msg, fnm, lnr, ix);
					strcpy(Type, "");
					lnr = -1;
				}
			//	printf("Type: '%s'\n", Type);
			}
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
		if ((q = strstr(buf, "\"source\"")) != NULL)
		{	q += strlen("\"source\"");
			while (*q != '\0' && *q != '"')
			{	q++;
			}
			if (*q == '"')
			{	char *r = q+1;
				while (*r != '\0' && *r != '"')
				{	r++;
				}
				*r = '\0';
				strncpy(Src, q+1, sizeof(Src)-1);	// not yet used
			//	printf("Src: '%s'\n", Src);
			}
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
				{	cnt += json_add2(Type, Msg, fnm, lnr, ix);
					strcpy(fnm, "");
					lnr = -1;
				}
			//	printf("File: '%s'\n", fnm);
			}
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
				{	cnt += json_add2(Type, Msg, fnm, lnr, ix);
					strcpy(fnm, "");
					lnr = -1;
				}
			//	printf("Line: %d\n", lnr);
			}
			continue;
		}
		if (phase == 0
		&&  (q = strstr(buf, "\"cobra\"")) != NULL)
		{	q += strlen("\"cobra\"");
			q = strchr(q, '"');
			if (q
			&&  strlen(Type) > 0
			&&  strlen(fnm) > 0
			&&  lnr >= 0
			&&  sscanf(q, "\"%d %d %d\"", &a, &b, &c) == 3)
			{	cnt += json_add(Type, Msg, fnm, lnr, b, c, ix);
				// a = no_cpp value
				// b = nr of tokens from start of line
				// c = nr of tokens in pattern
				ncb++;	// nr of cobra records read
				if (no_cpp != a && !once++)
				{	fprintf(stderr, "warning: preprocessing is %sabled\n", no_cpp?"dis":"en");
					fprintf(stderr, "warning: but '%s' was written with%s preprocessing\n",
						f, a?"out":"");
					fprintf(stderr, "warning: which can affect location accuray\n");
				}
				strcpy(fnm, "");
				lnr = -1;
			}
		//	printf("Cobra: %d %d %d -- %d\n\n", a, b, c, ntp);
	}	}
	if (phase == 1
	&&  strlen(Type) > 0
	&&  strlen(fnm) > 0
	&&  lnr >= 0)
	{	json_add2(Type, Msg, fnm, lnr, ix);	// final match
	}
	fclose(fd);

	if (phase == 0 && ncb == 0)
	{	phase = 1;
		if (ntp == 0 || nfl == 0 || nln == 0)
		{	if (verbose) // eg be an import from scope_check
			{ fprintf(stderr, "warning: no usable records found (file, line, or cobra fields missing)\n");
			}
		} else
		{	if (nfl != nln || ntp != nfl)
			{	fprintf(stderr, "warning: not all records have type, file and line fields\n");
			}
			if ((fd = fopen(f, "r")) != NULL)	// reopen
			{	goto again;
	}	}	}

	if (!no_display && !no_match)
	{	printf("imported %d records, marked %d tokens\n", ntp, cnt);
	}
}

void
json_match(const char *setname, const char *te, const char *msg, const Prim *from, const Prim *upto, int first)
{	const char *f = from?from->fnm:"";
	const int ln  = from?from->lnr:0;
	FILE *fd = (track_fd) ? track_fd : stdout;

	// fields can only contain: alphanumeric _ . $ @
	// unless preceded by an escape \ character
	// cleaned_up inserts escapes where needed
	// unless in double quotes....
	if (!te
	||  !msg)
	{	return;
	}
	if (!first)
	{	fprintf(fd, ",\n");
	}
	fprintf(fd, "  { \"type\"\t:\t\"");
	 if (setname
	 && strlen(setname) > 0)
	 {	cleaned_up(fd, setname);
		fprintf(fd, "\",\n");
		fprintf(fd, "    \"message\"\t:\t\"");
	 	cleaned_up(fd, te);
	 } else
	 {	cleaned_up(fd, te);
		fprintf(fd, "\",\n");
		fprintf(fd, "    \"message\"\t:\t\"");
	 	cleaned_up(fd, msg);
	}
	fprintf(fd, "\",\n");

	if (json_plus)
	{	if (strlen(bvars) > 0)		// json_match
		{	fprintf(fd, "    \"bindings\"\t:\t\"");
			cleaned_up(fd, bvars);
			fprintf(fd, "\",\n");
			strcpy(bvars, "");
		}
		fprintf(fd, "    \"source\"\t:\t\"");
		show_line(fd, f, 0, ln, ln, -1);
		fprintf(fd, "\",\n");
	}

	fprintf(fd, "    \"file\"\t:\t\"%s\",\n", f);
	fprintf(fd, "    \"line\"\t:\t%d,\n", ln);

	cobra_export(from, upto); // 3 numbers
	fprintf(fd, "  }\n");
}

int
do_markups(const char *setname)
{	Match *m;
	Bound *b;
	int seq = 1;
	int w = 0;
	int p = 0;

	for (m = matches; m; seq++, m = m->nxt)		// do_markups, read-only
	{	p++;	// nr of patterns
		if (m->bind)
		{	if (verbose && w++ == 0)
			{	printf("bound variables matched:\n");
			}
			for (b = m->bind; b; b = b->nxt)
			{	w++;	// nr of bound vars
			//	if (strlen(setname) == 0)	// removed July 31, 2025
				{	if (b->ref)
					{	b->ref->mark |= 4;	// match of bound variable
					} else	// eg ref of bound var in constraint
					{	// m->from->mark |= 4;	// mark the start of the pattern
				}	}
				if (b->bdef)
				{	b->bdef->mark |= 16;	// location bound variable defined
				}
				if (verbose)
				{	printf("\t%d:%d %s:%d: %s (line %d)\n", seq, w,
						b->ref?b->ref->fnm:"",
						b->ref?b->ref->lnr:0,
						b->ref?b->ref->txt:"",
						b->bdef?b->bdef->lnr:0);
				}
				if (b->nxt == b)
				{	b->nxt = 0;
					fprintf(stderr, "internal error: bound variables (%s)\n", setname);
					break;
				}
	}	}	}

	if (gui)
	{	if (strlen(setname) == 0)
		{	printf("%d matches\n", p);
		} else
		{	printf("%d matches stored in %s (json)\n", p, setname); // do_markups
		}
	} else if (!json_format && !no_match && !no_display)
	{	if (strlen(setname) == 0)
		{	printf("%d pattern%s matched\n", p, p!=1?"s":"");
		}
		if (w > 0)
		{	printf("%d bound variable%s\n",
				w, (w==1)?"":"s");
	}	}

	return p;
}

void
json(const char *te)
{	Prim *sop = (Prim *) 0;
	Prim *q, *mycur;
	int seen = 0;
	FILE *fd = (track_fd) ? track_fd : stdout;

	// usage:
	//	json		# prints json format summary of all matches
	//	json string	# puts string the type field
	//	if string is a namedset, cobra_lib maps this to "ps json string"

	// tokens that are part of a match are marked         &1
	// the token at start of each pattern match is marked &2
	// tokens that match a bound variable are marked      &4
	// the token at end of each pattern match is marked   &8
	// the token where a bound variable is defined        &16

	// if the json argument is a number or a single *, then
	// the output is verbose for that match (or all, for a *)

	memset(bvars, 0, sizeof(bvars));

	fprintf(fd, "[\n");
	for (mycur = prim; mycur; mycur = mycur?mycur->nxt:0)
	{	if (mycur->mark)
		{	seen |= 2;
		}
		if (!(mycur->mark & 2))	// find start of match
		{	continue;
		}
		if (sop)
		{	fprintf(fd, ",\n");
		}
		// start of match
		sop = mycur;

		q = mycur;
		if (q->bound)	// assume this wasnt set for other reasons...
		{	if (strcmp(q->fnm, q->bound->fnm) == 0)
			{	sprintf(json_msg, "lines %d..%d", q->lnr, q->bound->lnr);
			} else
			{	sprintf(json_msg, "lines %s:%d..%s:%d",
					q->fnm, q->lnr, q->bound->fnm, q->bound->lnr);
			}
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
			if (strcmp(sop->fnm, q?q->fnm:"") == 0)
			{	sprintf(json_msg, "lines %d..%d", sop->lnr, q?q->lnr:0);
			} else
			{	sprintf(json_msg, "lines %s:%d..%s:%d",
					sop->fnm, sop->lnr, q?q->fnm:"", q?q->lnr:0);
		}	}
		json_match("Patterns", te, json_msg, sop, q, 1);	// json()
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
			json_match("Marked", te, json_msg, sop, mycur, 1); // json()
		}
	}
	fprintf(fd, "\n]\n");
}

int unnumbered;	// cobra_prog.y

void
show_line(FILE *fdo, const char *fnm, int n, int from, int upto, int tag)
{	FILE *fdi;
	char buf[MAXYYTEXT];
	int ln = 0;
	static int wcnt = 1;

	if (tgrep && (tcount || lcount))
	{	return;
	}

	if (scrub && strlen(scrub_caption) > 0)
	{	fprintf(fdo, "cobra%05d <Med> :%s:%d: %s\n",
			wcnt++, fnm, tag, scrub_caption);
		if (upto == from)
		{	return;
	}	}

	if ((fdi = fopen(fnm, "r")) == NULL)
	{	if (verbose || n < 0)	// n<- means called from inline program
		{	fprintf(stderr, "show_line, cannot open '%s'\n", fnm);
		}
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
			if (n > 0 && !cobra_texpr && !unnumbered)
			{	fprintf(fdo, "%3d: ", n);
			}
			if (gui)
			{	fprintf(fdo, "%c%s:%05d:  ",
					(ln == tag && upto > from)?'>':' ', fnm, ln);
			} else if (tag >= 0)
			{	if (!unnumbered)
				{	fprintf(fdo, "%c%5d  ",
						(ln == tag && upto > from)?'>':' ', ln);
				}
			//	if (tgrep && !unnumbered)
			//	{	fprintf(fdo, ":");
			//	}
			} else	// tag < 0 -> json output
			{	char *q, *p = buf;
				while (*p == ' ' || *p == '\t')
				{	p++;
				}
				while ((q = strchr(p, '\n'))
				||     (q = strchr(p, '\r')))
				{	*q = '\0';
				}
#if 1
				if ((q = strstr(p, "//")) != NULL)
				{	*q = '\0'; // strip comments
				}
				cleaned_up(fdo, p);
#else
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
#endif
				continue;
			}
L:			fprintf(fdo, "%s", buf);
	}	}

	fclose(fdi);
}
