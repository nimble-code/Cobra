#include "c_api.h"

// JPL_21  Do not define macros inside functions
// JPL_22  Do not undefine or redefine macros
// JPL_31  No code or declarations before an #include

void
cobra_main(void)
{	int sawcode = 0;

	if (!no_cpp)
	{	fprintf(stderr, "usage: ./nomacros -n file.c\n");
		exit(1);
	}
	# define catchme
	# undef foobar
	for ( ; cur; NEXT)
	{	if (TYPE("cpp"))
		{	char *ptr = cobra_txt();
			while (isblank((uchar)*ptr) || *ptr == '#')
			{	ptr++;
			}
			if (strncmp(ptr, "define", strlen("define")) == 0
			&& cur->curly > 0)
			{	printf("%s:%d: macro definition inside function: %s\n",
				cobra_bfnm(), cobra_lnr(), cobra_txt());
			}
			if (strncmp(ptr, "undef", strlen("undef")) == 0)
			{	printf("%s:%d: use of undef: %s\n",
				cobra_bfnm(), cobra_lnr(), cobra_txt());
			}
			if (strncmp(ptr, "include", strlen("include")) == 0
			&& sawcode)
			{	printf("%s:%d: saw code before include: %s\n",
				cobra_bfnm(), cobra_lnr(), cobra_txt());
		}	}
		if (MATCH(";"))
		{	sawcode++;
		}
	}
	exit(0);
}
