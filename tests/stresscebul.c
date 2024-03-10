/*
 * cebtree stress testing tool
 *
 * It runs several parallel threads each with their own test.
 * The test consists in picking random values, applying them a mask so that
 * only 64k values are possible (with extremities present) and which are
 * stored into a 32k size table. The table contains both used and unused
 * items. It's sufficient to memset(0) the table to reset it. Random numbers
 * first pick an index and depending on whether the designated entry is
 * supposed to be present or absent, the entry will be looked up or randomly
 * picked and inserted.
 */
#include <sys/time.h>

#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cebul_tree.h"

/* settings for the test */

#define RNG32MASK       0xc35a6987u           // 16 bits
#define RNG64MASK       0xc018050a06044813ull // 16 bits

#define TBLSIZE         32678
#define MAXTHREADS      256


/* Some utility functions */

#define RND32SEED 2463534242U
static __thread uint32_t rnd32seed = RND32SEED;
static inline uint32_t rnd32()
{
	rnd32seed ^= rnd32seed << 13;
	rnd32seed ^= rnd32seed >> 17;
	rnd32seed ^= rnd32seed << 5;
	return rnd32seed;
}

#define RND64SEED 0x9876543210abcdefull
static __thread uint64_t rnd64seed = RND64SEED;
static inline uint64_t rnd64()
{
	rnd64seed ^= rnd64seed << 13;
	rnd64seed ^= rnd64seed >>  7;
	rnd64seed ^= rnd64seed << 17;
	return rnd64seed;
}

static inline unsigned long rndl()
{
	return (sizeof(long) < sizeof(uint64_t)) ? rnd32() : rnd64();
}

/* long random with no more than 2^16 possible combinations */
static inline unsigned long rndl16()
{
	return (sizeof(long) < sizeof(uint64_t)) ?
		rnd32() & RNG32MASK :
		rnd64() & RNG64MASK;
}

/* produce a random between 0 and range+1 */
static inline unsigned int rnd32range(unsigned int range)
{
        unsigned long long res = rnd32();

        res *= (range + 1);
        return res >> 32;
}

static inline struct timeval *tv_now(struct timeval *tv)
{
        gettimeofday(tv, NULL);
        return tv;
}

static inline unsigned long tv_ms_elapsed(const struct timeval *tv1, const struct timeval *tv2)
{
        unsigned long ret;

        ret  = ((signed long)(tv2->tv_sec  - tv1->tv_sec))  * 1000;
        ret += ((signed long)(tv2->tv_usec - tv1->tv_usec)) / 1000;
        return ret;
}

/* display the message and exit with the code */
__attribute__((noreturn)) void die(int code, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	exit(code);
}

#define BUG_ON(x) do {							\
		if (x) {						\
			fprintf(stderr, "BUG at %s:%d after %lu loops: %s\n", \
				__func__, __LINE__, ctx->loops, #x);	\
			__builtin_trap();				\
		}							\
	} while (0)

/* flags for item->flags */
#define IN_TREE         0x00000001

/* one item */
struct item {
	struct ceb_node node;
	unsigned long key;
	unsigned long flags;
};

/* thread context */
struct ctx {
	struct item table[TBLSIZE];
	struct ceb_node *ceb_root;
	unsigned long min, max;
	pthread_t thr;
	unsigned long loops;
} __attribute__((aligned(64)));


struct ctx th_ctx[MAXTHREADS] = { };
static volatile unsigned int actthreads;
static volatile unsigned int step;
unsigned int nbthreads = 1;


/* run the test for a thread */
void run(void *arg)
{
	int tid = (long)arg;
	struct ctx *ctx = &th_ctx[tid];
	unsigned int idx;
	struct item *itm;
	struct ceb_node *node1, *node2, *node3;
	unsigned long v;

	rnd32seed += tid + 1;
	rnd64seed += tid + 1;

	/* step 0: create all threads */
	while (__atomic_load_n(&step, __ATOMIC_ACQUIRE) == 0) {
		/* don't disturb pthread_create() */
		usleep(10000);
	}

	/* step 1 : wait for signal to start */
	__atomic_fetch_add(&actthreads, 1, __ATOMIC_SEQ_CST);

	while (__atomic_load_n(&step, __ATOMIC_ACQUIRE) == 1)
		;

	/* step 2 : run */
	for (; __atomic_load_n(&step, __ATOMIC_ACQUIRE) == 2; ctx->loops++) {
		/* pick a random value and an index number */
		v = rndl16();

		idx = rnd32range(TBLSIZE - 1);
		BUG_ON(idx >= TBLSIZE);
		itm = &ctx->table[idx];

		if (itm->flags & IN_TREE) {
			/* the item is expected to already be in the tree, so
			 * let's verify a few things.
			 */
			BUG_ON(!ceb_intree(&itm->node));

			node1 = cebul_lookup(&ctx->ceb_root, itm->key);
			BUG_ON(!node1);
			BUG_ON(node1 != &itm->node);

			node1 = cebul_lookup_ge(&ctx->ceb_root, itm->key);
			BUG_ON(!node1);
			BUG_ON(node1 != &itm->node);

			node1 = cebul_lookup_le(&ctx->ceb_root, itm->key);
			BUG_ON(!node1);
			BUG_ON(node1 != &itm->node);

			node3 = cebul_lookup_lt(&ctx->ceb_root, itm->key);
			BUG_ON(node3 == node1);

			node2 = cebul_prev(&ctx->ceb_root, node1);
			BUG_ON(node2 != node3); // prev() of an existing node is lt

			if (!node2) {
				/* this must be the first */
				node3 = cebul_first(&ctx->ceb_root);
				BUG_ON(node3 != node1);
			} else {
				node3 = cebul_next(&ctx->ceb_root, node2);
				BUG_ON(node3 != node1);
			}

			node2 = node3; // lt
			node3 = cebul_lookup_gt(&ctx->ceb_root, itm->key);
			BUG_ON(node3 == node1);
			BUG_ON(node3 && node3 == node2);

			node2 = cebul_next(&ctx->ceb_root, node1);
			BUG_ON(node2 != node3); // next() of an existing node is gt

			if (!node2) {
				/* this must be the last */
				node3 = cebul_last(&ctx->ceb_root);
				BUG_ON(node3 != node1);
			} else {
				node3 = cebul_prev(&ctx->ceb_root, node2);
				BUG_ON(node3 != node1);
			}

			/* If the new picked value matches the existing
			 * one, it is just removed. If it does not match,
			 * it is removed and we try to enter the new one.
			 */
			node2 = cebul_delete(&ctx->ceb_root, node1);
			BUG_ON(node2 != node1);

			itm->flags &= ~IN_TREE;
			BUG_ON(ceb_intree(node1));

			if (v != itm->key) {
				itm->key = v;
				node2 = cebul_insert(&ctx->ceb_root, &itm->node);
				if (node2 == &itm->node) {
					BUG_ON(!ceb_intree(&itm->node));
					itm->flags |= IN_TREE;
				}
				else {
					BUG_ON(ceb_intree(&itm->node));
				}
			}
		} else {
			/* this item is not in the tree, let's invent a new
			 * value.
			 */
			do {
				itm->key = v;
				node2 = cebul_lookup_le(&ctx->ceb_root, itm->key);
				if (node2)
					BUG_ON(container_of(node2, struct item, node)->key > itm->key);

				node3 = cebul_lookup_ge(&ctx->ceb_root, itm->key);
				if (node3)
					BUG_ON(container_of(node3, struct item, node)->key < itm->key);

				node1 = cebul_insert(&ctx->ceb_root, &itm->node);

				if (node2 && container_of(node2, struct item, node)->key == itm->key)
					BUG_ON(node1 != node2);

				if (node3 && container_of(node3, struct item, node)->key == itm->key)
					BUG_ON(node1 != node3);

				/* if there was a collision, it must be with at least one of
				 * these nodes.
				 */
				if (node1 != &itm->node)
					BUG_ON(node1 != node2 && node1 != node3);
			} while (node1 != &itm->node && ((v = rndl16()), 1));

			BUG_ON(!ceb_intree(&itm->node));
			itm->flags |= IN_TREE;

			/* perform a few post-insert checks */
			node1 = cebul_lookup(&ctx->ceb_root, itm->key);
			BUG_ON(!node1);
			BUG_ON(node1 != &itm->node);

			node1 = cebul_lookup_ge(&ctx->ceb_root, itm->key);
			BUG_ON(!node1);
			BUG_ON(node1 != &itm->node);

			node1 = cebul_lookup_le(&ctx->ceb_root, itm->key);
			BUG_ON(!node1);
			BUG_ON(node1 != &itm->node);

			node3 = cebul_lookup_lt(&ctx->ceb_root, itm->key);
			BUG_ON(node3 == node1);

			node2 = cebul_prev(&ctx->ceb_root, node1);
			BUG_ON(node2 != node3); // prev() of an existing node is lt

			if (!node2) {
				/* this must be the first */
				node3 = cebul_first(&ctx->ceb_root);
				BUG_ON(node3 != node1);
			} else {
				node3 = cebul_next(&ctx->ceb_root, node2);
				BUG_ON(node3 != node1);
			}

			node2 = node3; // lt
			node3 = cebul_lookup_gt(&ctx->ceb_root, itm->key);
			BUG_ON(node3 == node1);
			BUG_ON(node3 && node3 == node2);

			node2 = cebul_next(&ctx->ceb_root, node1);
			BUG_ON(node2 != node3); // next() of an existing node is gt

			if (!node2) {
				/* this must be the last */
				node3 = cebul_last(&ctx->ceb_root);
				BUG_ON(node3 != node1);
			} else {
				node3 = cebul_prev(&ctx->ceb_root, node2);
				BUG_ON(node3 != node1);
			}
		}
	}

	/* step 3 : stop */
	__atomic_fetch_sub(&actthreads, 1, __ATOMIC_SEQ_CST);

	fprintf(stderr, "thread %d quitting\n", tid);

	pthread_exit(0);
}

/* stops all threads upon SIG_ALRM */
void alarm_handler(int sig)
{
	__atomic_store_n(&step, 3, __ATOMIC_RELEASE);
	fprintf(stderr, "received signal %d\n", sig);
}

void usage(const char *name, int ret)
{
	die(ret, "usage: %s [-h] [-d*] [-t threads] [-r run_secs] [-s seed]\n", name);
}

int main(int argc, char **argv)
{
	static struct timeval start, stop;
	unsigned int arg_run = 1;
	unsigned long loops = 0;
	unsigned long seed = 0;
	char *argv0 = *argv;
	unsigned int u;
	int debug = 0;
	int i, err;

	argv++; argc--;

	while (argc && **argv == '-') {
		if (strcmp(*argv, "-d") == 0) {
			debug++;
		}
		else if (!strcmp(*argv, "-t")) {
			if (--argc < 0)
				usage(argv0, 1);
			nbthreads = atol(*++argv);
		}
		else if (!strcmp(*argv, "-s")) {
			if (--argc < 0)
				usage(argv0, 1);
			seed = atol(*++argv);
		}
		else if (!strcmp(*argv, "-r")) {
			if (--argc < 0)
				usage(argv0, 1);
			arg_run = atol(*++argv);
		}
		else if (strcmp(*argv, "-h") == 0)
			usage(argv0, 0);
		else
			usage(argv0, 1);
		argc--; argv++;
	}

	if (nbthreads >= MAXTHREADS)
		nbthreads = MAXTHREADS;

	rnd32seed += seed;
	rnd64seed += seed;

	actthreads = 0;	step = 0;

	printf("Starting %d thread%c\n", nbthreads, (nbthreads > 1)?'s':' ');

	for (u = 0; u < nbthreads; u++) {
		err = pthread_create(&th_ctx[u].thr, NULL, (void *)&run, (void *)(long)u);
		if (err)
			die(1, "pthread_create(): %s\n", strerror(err));
	}

	/* prepare the threads to start */
	__atomic_fetch_add(&step, 1, __ATOMIC_SEQ_CST);

	/* wait for them all to be ready */
	while (__atomic_load_n(&actthreads, __ATOMIC_ACQUIRE) != nbthreads)
		;

	signal(SIGALRM, alarm_handler);
	alarm(arg_run);

	gettimeofday(&start, NULL);

	/* Go! */
	__atomic_fetch_add(&step, 1, __ATOMIC_SEQ_CST);

	/* Threads are now running until the alarm rings */

	/* wait for them all to die */

	for (u = 0; u < nbthreads; u++) {
		pthread_join(th_ctx[u].thr, NULL);
		loops += th_ctx[u].loops;
	}

	gettimeofday(&stop, NULL);

	i = (stop.tv_usec - start.tv_usec);
	while (i < 0) {
		i += 1000000;
		start.tv_sec++;
	}
	i = i / 1000 + (int)(stop.tv_sec - start.tv_sec) * 1000;

	printf("threads: %d loops: %lu time(ms): %u rate(lps): %llu\n",
	       nbthreads, loops, i, loops * 1000ULL / (unsigned)i);

	return 0;
}
