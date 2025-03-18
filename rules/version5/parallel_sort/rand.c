#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void
main(int argc, char *argv[])
{	int i, nr, mod;

	if (argc != 3)
	{	fprintf(stderr, "usage: rand nr mod\n");
		exit(1);
	}
	nr = atoi(argv[1]);
	mod = atoi(argv[2]);

	srand((unsigned int) time(0));
	for (i = 0; i < nr; i++)
	{	printf("%d\n", rand()%mod);
	}
}
