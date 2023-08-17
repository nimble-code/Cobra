/*
 * This file is part of the public release of Cobra. It is subject to the
 * terms in the License file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com/cobra
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>

#ifndef PC
 #if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  #define PC
  #define WINDOWS
 #endif
#endif

#ifdef DEBUG_MEM
static size_t	 Emu[200];
void report_memory_use(void);
#endif

typedef long unsigned int	ulong;

static sem_t	  *sem;
static sem_t	  *psem;

static char	  sem_name1[16];
static char	  sem_name2[16];
static int	  sem_created;	// parse time only
static int	  lock_held = -1;

static size_t	**heap;		// to avoid emalloc in multi-core runs
static int	 *heapsz;	// size in words

static ulong	  total_used;
static ulong	  total_allocated;

static ulong	  Chunk = 32*1024*1024; 	// bytes, main allocator

#ifdef PC
 static ulong	  HeapSz = 64*1024;		// bytes, per-core heap size
#else
 static ulong	  HeapSz = 32*1024*1024;	// bytes, per-core heap size
#endif

static struct	timeval *start_time;
static struct	timeval *stop_time;
static double	*delta_time;

static void	zap_sem(void);

extern ulong	MaxMemory;

extern int Ncore;
extern int no_display;
extern int runtimes;
extern int verbose;
extern int tfree, tnew;
extern char *progname;

extern void lock_print(int);
extern void unlock_print(int);

#if defined(__APPLE__) || defined(__linux__) || defined(NO_SBRK)
	#include <stdlib.h>
	#define sbrk	malloc
#else
	extern void *sbrk(int);
#endif

void
memusage(void)	// used in cobra_lib.c
{
	zap_sem();
	if (verbose == 1 || MaxMemory != 24000)
	{	printf("Memory used     : %4lu MB\n", total_used/(1024*1024));
		printf("Memory allocated: %4lu MB\n", total_allocated/(1024*1024));
	}
}

void
efree(void *ptr)
{
//	printf("nope\n");
}

static void *
e_malloc(size_t size)	// bytes
{	static size_t *have_mem = (void *) 0;
	static size_t  have_cnt = 0;	// bytes
	void *n;

	if (have_cnt < size)
	{	if (Chunk < size)
		{	Chunk = size;
		}
		if (MaxMemory != 0
		&&  (total_allocated + Chunk)/(1024*1024) > MaxMemory)
		{	fprintf(stderr, "cobra: exceeding max memory of %4lu MB (change limit with -MaxMem N)\n", MaxMemory);
			return 0;
		}
		do {
			have_mem = (size_t *) sbrk(Chunk);
			Chunk /= 2;
		} while (have_mem == (size_t *) -1 && Chunk > 0);
		Chunk *= 2;
		total_allocated += Chunk;
		have_cnt = Chunk;
//		printf("sbrk +%lu\n", (ulong) Chunk);
	}
//	printf("%d <-> %d\n", (int) have_cnt, (int) size);
	if (have_cnt >= size)
	{	n = (void *) have_mem;
		have_cnt -= size;
		size /= sizeof(void *);	// bytes to words
		have_mem += size;	// words
		return n;
	}
	return 0;
}

void *
emalloc(size_t size, const int caller)	// size in bytes
{	void *n;
	int p;

	if ((p = size % sizeof(size_t)) > 0)
	{	size += sizeof(size_t) - p;
	}
	n = e_malloc(size * sizeof(char));

	if (!n)
	{	fprintf(stderr, "%d: out of memory (%lu bytes - wanted %lu more)\n",
			caller, total_used, (ulong) size);
		verbose++;
		memusage();
#ifdef DEBUG_MEM
		report_memory_use();
#endif
		exit(1);
	}
	memset(n, 0, size);
	total_used += (unsigned long) size;

#ifdef DEBUG_MEM
	assert(caller >= 0 && caller < sizeof(Emu)/sizeof(size_t));
	Emu[caller] += size;
#endif

	return n;
}

#ifdef DEBUG_MEM
void
report_memory_use(void)
{	int j;

	for (j = 0; j < sizeof(Emu)/sizeof(size_t); j++)
	{	if (Emu[j] > 0)
		{	printf("Emu[%d]	%lu\n", j, Emu[j]);
	}	}
	printf("tfree: %6d\n", tfree);
	printf("tnew : %6d\n", tnew);
}
#endif

void
ini_heap(void)
{	static int maxn = 0;
	int i;

	if (!heap || Ncore > maxn)
	{	heap    = (size_t **) emalloc(Ncore * sizeof(size_t *), 37);
		heapsz  =     (int *) emalloc(Ncore * sizeof(int), 38);
		for (i = 0; i < Ncore; i++)
		{	heap[i] = (size_t *) emalloc(HeapSz * sizeof(char), 39);
			heapsz[i] = HeapSz / sizeof(size_t);
		}
		if (Ncore > maxn)
		{	maxn = Ncore;
	}	}
}

size_t *
hmalloc(size_t size, const int ix, const int caller)	// size in bytes
{	size_t *m;

	size += sizeof(size_t) - 1;
	size /= sizeof(size_t);		// size in words

	assert(heap != NULL);
	assert(ix >= 0 && ix < Ncore);

	if (!heap[ix] || heapsz[ix] <= size)	// words
	{
		lock_print(ix);
		if (HeapSz < size * sizeof(size_t))
		{
			HeapSz = (int) size * sizeof(size_t);
		}
		heap[ix]   = (size_t *) emalloc(HeapSz * sizeof(char), caller);
		heapsz[ix] = HeapSz / sizeof(size_t);	// words
		unlock_print(ix);
	}

	m = heap[ix];

	heap[ix]   += size;	// words
	heapsz[ix] -= size;	// words

#ifdef DEBUG_MEM
	assert(caller >= 0 && caller < sizeof(Emu)/sizeof(size_t));
	Emu[caller] += (int) size;
#endif

	return m;
}

static void
zap_sem(void)
{
	if (sem_created)
	{	sem_unlink(sem_name1);
		sem_unlink(sem_name2);
	}
}

void
ini_lock(void)
{
	if (Ncore > 1 && !sem_created)
	{	srand((unsigned int) time(0));

		sprintf(sem_name1, "/C1_%d", (unsigned char) rand());
		sem_unlink(sem_name1);	// make sure

		sprintf(sem_name2, "/C2_%d", (unsigned char) rand());
		sem_unlink(sem_name2);

		sem  = sem_open(sem_name1, O_CREAT, 0666, 1);	// omitted O_EXCL
		psem = sem_open(sem_name2, O_CREAT, 0666, 1);	// omitted O_EXCL

		if (sem == SEM_FAILED
		|| psem == SEM_FAILED)
		{	perror("sem_open");
		}
		sem_created = 1;
	}
}

void
do_lock(const int ix)
{
	if (Ncore <= 1)
	{	return;
	}
	if (sem_wait(psem) != 0)
	{	perror("psem_wait");
	}
	lock_held = ix;
}

void
do_unlock(const int ix)
{
	if (Ncore <= 1)
	{	return;
	}
	if (lock_held != ix)
	{	fprintf(stderr, "error: lock violation: held by %d, unlock by %d\n",
			lock_held, ix);
	}
	lock_held = -1;
	if (sem_post(psem) != 0)
	{	perror("psem_post");
	}
}

void
lock_print(int who)
{
	if (Ncore <= 1)
	{	return;
	}
	if (sem_wait(sem) != 0)
	{	perror("sem_wait");
	}
}

void
unlock_print(int who)
{
	if (Ncore <= 1)
	{	return;
	}
	if (sem_post(sem) != 0)
	{	perror("sem_post");
	}
}

void
ini_timers(void)
{	static int maxn = 0;
	// allow for separate timers for cobra_prog.y programs
#if 0
	if (!start_time || Ncore > maxn)
	{	start_time = (clock_t *) emalloc(2 * Ncore * sizeof(clock_t), 41);
		stop_time  = (clock_t *) emalloc(2 * Ncore * sizeof(clock_t), 42);
		delta_time = (double *)  emalloc(2 * Ncore * sizeof(double), 43);
		if (Ncore > maxn)
		{	maxn = Ncore;
	}	}
#else
	if (!start_time || Ncore > maxn)
	{	start_time = (struct timeval *) emalloc(2 * (Ncore+1) * sizeof(struct timeval), 41);
		stop_time  = (struct timeval *) emalloc(2 * (Ncore+1) * sizeof(struct timeval), 42);
		delta_time = (double *)  emalloc(2 * (Ncore+1) * sizeof(double), 43);
		// +1 for cwe timer
		if (Ncore > maxn)
		{	maxn = Ncore;
	}	}
#endif
}

void
start_timer(int ix)
{
	assert(ix >= 0 && ix < 2*(Ncore+1));
#if 0
	start_time[ix] = clock();
#else
	gettimeofday(&(start_time[ix]), NULL);
#endif
}

void
stop_timer(int cid, int always, const char *s)
{
	if (runtimes)
	{	assert(cid >= 0 && cid < 2*(Ncore+1));
		do_lock(cid);
		gettimeofday(&(stop_time[cid]), NULL);
		delta_time[cid] =  (double) (stop_time[cid].tv_sec  - start_time[cid].tv_sec);		// seconds
		delta_time[cid] += ((double) (stop_time[cid].tv_usec - start_time[cid].tv_usec))/1000000.0; // microseconds
		{	if (Ncore > 1)
			{	printf("%d: ", cid);
			}
			printf("(%.3g sec)\n", delta_time[cid]);
		}
		do_unlock(cid);
	}
}

#if 0
void
memtest(void)
{	unsigned int bit=1024*1024*1024;
	double sum=0.0;
	char *x;

	while (bit > 4096)
	{	x = sbrk(bit);
		if (x != (void *) -1)
		{	sum += (double) bit;
//			printf("sbrk +%lu	%lu	%.1f\n",
				(ulong) bit, (ulong) x, sum/1024.0/1024.0);
		} else
		{	bit >>= 1;
		}
	}
	printf("%.1f Mb\n", sum/1024.0/1024.0);
	exit(0);
}
#endif
