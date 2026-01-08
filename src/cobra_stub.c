typedef unsigned int uint;

#include "cobra_fe.h"
#include "cobra_te.h"

ThreadLocal_te *thrl; // not used in src_app code, but used in c.ar fct add_match()

int
check_run(void)
{	// .cobra file cannot be used
	// for background checkers
	return 0;
}

void
reset_int(const int unused)
{	return;
}

void
set_thread(void)
{
	thrl = (ThreadLocal_te *) emalloc(Ncore * sizeof(ThreadLocal_te), 7);
}
