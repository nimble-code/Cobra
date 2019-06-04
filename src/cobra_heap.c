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

extern int Ncore;
extern int no_display;
extern int runtimes;
extern int verbose;
extern char *progname;

extern void lock_print(int);
extern void unlock_print(int);

#ifdef __APPLE__
	#include <malloc.h>
	#define sbrk	malloc
#else
	extern void *sbrk(int);
#endif

void
memusage(void)	// used in cobra_lib.c
{
	zap_sem();
	if (verbose == 1)
	{	printf("Memory used     : %4lu MB\n", total_used/(1024*1024));
		printf("Memory allocated: %4lu MB\n", total_allocated/(1024*1024));
	}
}

void
efree(void *ptr)
{
//	printf("nope\n");
}

void *
e_malloc(size_t size)	// bytes
{	static size_t *have_mem = (void *) 0;
	static size_t  have_cnt = 0;	// bytes
	void *n;

	if (have_cnt < size)
	{	if (Chunk < size)
		{	Chunk = size;
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
emalloc(size_t size)	// size in bytes
{	void *n;
	int p;

	if ((p = size % sizeof(size_t)) > 0)
	{	size += sizeof(size_t) - p;
	}
	n = e_malloc(size * sizeof(char));

	if (!n)
	{	fprintf(stderr, "out of memory (%lu bytes - wanted %lu more)\n",
			total_used, (ulong) size);
		verbose++;
		memusage();
		exit(1);
	}
	memset(n, 0, size);
	total_used += (unsigned long) size;

	return n;
}

void
ini_heap(void)
{	static int maxn = 0;
	int i;

	if (!heap || Ncore > maxn)
	{	heap    = (size_t **) emalloc(Ncore * sizeof(size_t *));
		heapsz  =     (int *) emalloc(Ncore * sizeof(int));
		for (i = 0; i < Ncore; i++)
		{	heap[i] = (size_t *) emalloc(HeapSz * sizeof(char));
			heapsz[i] = HeapSz / sizeof(size_t);
		}
		if (Ncore > maxn)
		{	maxn = Ncore;
	}	}
}

size_t *
hmalloc(size_t size, const int ix)	// size in bytes
{	size_t *m;

	size += sizeof(size_t) - 1;
	size /= sizeof(size_t);		// size in words

	assert(heap != NULL);
	assert(ix >= 0 && ix < Ncore);

	if (!heap[ix] || heapsz[ix] <= size)	// words
	{
//fprintf(stderr, "%d in\n", ix);
		lock_print(ix);
		if (HeapSz < size * sizeof(size_t))
		{
// printf("Adjust %lu -> %lu\n", HeapSz, (unsigned long) (size * sizeof(size_t)));
			HeapSz = (int) size * sizeof(size_t);
		}
// printf("Add %lu bytes\n", HeapSz);
		heap[ix]   = (size_t *) emalloc(HeapSz * sizeof(char));
		heapsz[ix] = HeapSz / sizeof(size_t);	// words
		unlock_print(ix);
//fprintf(stderr, "%d out\n", ix);
	}

	m = heap[ix];

	heap[ix]   += size;	// words
	heapsz[ix] -= size;	// words

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
	{	printf("error: lock violation: held by %d, unlock by %d\n",
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
//fprintf(stderr, "Locked by %d\n", who);
}

void
unlock_print(int who)
{
	if (Ncore <= 1)
	{	return;
	}
//fprintf(stderr, "Unlocked by %d\n", who);
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
	{	start_time = (clock_t *) emalloc(2 * Ncore * sizeof(clock_t));
		stop_time  = (clock_t *) emalloc(2 * Ncore * sizeof(clock_t));
		delta_time = (double *)  emalloc(2 * Ncore * sizeof(double));
		if (Ncore > maxn)
		{	maxn = Ncore;
	}	}
#else
	if (!start_time || Ncore > maxn)
	{	start_time = (struct timeval *) emalloc(2 * (Ncore+1) * sizeof(struct timeval));
		stop_time  = (struct timeval *) emalloc(2 * (Ncore+1) * sizeof(struct timeval));
		delta_time = (double *)  emalloc(2 * (Ncore+1) * sizeof(double));
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
#if 0
		stop_time[cid]  = clock();
		delta_time[cid] = ((double) (stop_time[cid] - start_time[cid])) / ((double) CLOCKS_PER_SEC);
#else
		gettimeofday(&(stop_time[cid]), NULL);
		delta_time[cid] =  (double) (stop_time[cid].tv_sec  - start_time[cid].tv_sec);		// seconds
		delta_time[cid] += ((double) (stop_time[cid].tv_usec - start_time[cid].tv_usec))/1000000.0; // microseconds
#endif
#if 0
		if (always
		||  delta_time[cid] > 0.1)
#endif
		{	if (Ncore > 1)
			{	printf("%d: ", cid);
			}
#if 1
			printf("(%.3g sec)\n", delta_time[cid]);
#else
			printf("(%.3g sec) <<%s>>\n", delta_time[cid], s);
#endif
		}
		do_unlock(cid);
	}
}
