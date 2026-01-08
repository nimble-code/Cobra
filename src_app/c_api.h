/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

#ifndef COBRA_H
#define COBRA_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>

#ifdef PC
 #include <io.h>
#else
 #include <unistd.h>
 #include <pthread.h>
 #include <sys/times.h>
#endif

#include "../src/cobra_prim.h"

#define NEXT		cobra_nxt()
#define LAST		cobra_prv()
#define MATCH(x)	(strcmp(cobra_txt(), x) == 0)
#define TYPE(x)		(strcmp(cobra_typ(), x) == 0)
#define FIND(x)		{ if (!MATCH(x)) { continue; } }

#define EXPECT(x) { \
	NEXT; \
	if (!MATCH(x)) \
	{	printf("%s:%d: expected '%s', saw '%s'\n", \
			cobra_bfnm(), cobra_lnr()-1, x, \
			cobra_txt()); \
		rval = 0; \
	} \
}
#endif
