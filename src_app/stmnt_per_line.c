#include <stdio.h>
#include <stdlib.h>
// #include <malloc.h>
#include <string.h>
#include <assert.h>

// not a real cobra checker
// a little faster, but no preprocessing

#define NRMAX	256

char buf[4096];
int  counts[NRMAX];
char *location[NRMAX];
int  lno[NRMAX];

int
main(int argc, char *argv[])
{	FILE *fd;
	int i, cnt, lnr;
	char *p;

	for (i = 1; i < argc; i++)
	{	fd = fopen(argv[i], "r");
		if (fd == NULL)	// possibly a cobra arg
		{	continue;
		}
		lnr = 0;
		while (fgets(buf, sizeof(buf), fd))
		{	lnr++;
			p = buf;
			cnt = 0;
			while (*p)
			{	if (*p++ == ';')
				{	cnt++;
			}	}
			assert(cnt < NRMAX);
			counts[cnt]++;
			if (!location[cnt])
			{	location[cnt] = (char *) malloc(strlen(argv[i])+1);
				strcpy(location[cnt], argv[i]);
				lno[cnt] = lnr;
		}	}
		fclose(fd);
	}
	for (i = 0; i < NRMAX; i++)
	{	if (counts[i])
		{	printf("%d\t%d\t%s:%d\n",
				i, counts[i],
				location[i], lno[i]);
	}	}

	return 0;
}
