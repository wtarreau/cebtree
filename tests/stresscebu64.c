#include <sys/time.h>

#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cebu64_tree.h"

#ifndef container_of
#define container_of(ptr, type, name) ((type *)(((char *)(ptr)) - ((long)&((type *)0)->name)))
#endif

struct ceb_node *ceb_root = NULL;

struct key {
	struct ceb_node node;
	uint64_t key;
};

struct ceb_node *add_value(struct ceb_node **root, uint64_t value)
{
	struct key *key;
	struct ceb_node *prev, *ret;

	key = calloc(1, sizeof(*key));
	key->key = value;
	do {
		prev = cebu64_insert(root, &key->node);
		if (prev == &key->node)
			return prev; // was properly inserted
		/* otherwise was already there, let's try to remove it */
		ret = cebu64_delete(root, prev);
		if (ret != prev) {
			/* was not properly removed either: THIS IS A BUG! */
			printf("failed to insert %p(%llu) because %p has the same key and could not be removed because returns %p\n",
			       &key->node, (unsigned long long)key->key, prev, ret);
			free(key);
			return NULL;
		}
		free(container_of(ret, struct key, node));
	} while (1);
}

#define RND32SEED 2463534242U
static uint32_t rnd32seed = RND32SEED;
static uint32_t rnd32()
{
	rnd32seed ^= rnd32seed << 13;
	rnd32seed ^= rnd32seed >> 17;
	rnd32seed ^= rnd32seed << 5;
	return rnd32seed;
}

static uint64_t rnd64()
{
	return ((uint64_t)rnd32() << 32) + rnd32();
}

int main(int argc, char **argv)
{
	struct ceb_node *old, *back __attribute__((unused));
	char *orig_argv, *argv0 = *argv, *larg;
	struct key *key;
	char *p;
	uint64_t v;
	int test = 0;
	uint64_t mask = ~0ULL;
	int count = 10;
	int debug = 0;

	argv++; argc--;

	while (argc && **argv == '-') {
		if (strcmp(*argv, "-d") == 0)
			debug++;
		else {
			printf("Usage: %s [-d]* [test [cnt [mask [seed]]]]\n", argv0);
			exit(1);
		}
		argc--; argv++;
	}

	orig_argv = larg = (argc > 0) ? *argv : argv0;

	if (argc > 0)
		test = atoi(larg = *(argv++));

	if (argc > 1)
		count = atoi(larg = *(argv++));

	if (argc > 2)
		mask = atol(larg = *(argv++));

	if (argc > 3)
		rnd32seed = atol(larg = *(argv++));

	/* rebuild non-debug args as a single string */
	for (p = orig_argv; p < larg; *p++ = ' ')
		p += strlen(p);

	if (debug > 1)
		cebu64_default_dump(0, orig_argv, 0, 0); // prologue

	if (test == 0) {
		while (count--) {
			v = rnd64() & mask;
			old = cebu64_lookup(&ceb_root, v);
			if (old) {
				if (cebu64_delete(&ceb_root, old) != old)
					abort();
				free(container_of(old, struct key, node));
			}
			else {
				key = calloc(1, sizeof(*key));
				key->key = v;
				old = cebu64_insert(&ceb_root, &key->node);
				if (old != &key->node)
					abort();
			}
		}
	} else if (test == 1) {
		while (count--) {
			v = rnd64() & mask;
			old = cebu64_lookup(&ceb_root, v);
			if (old) {
				if (cebu64_delete(&ceb_root, old) != old)
					abort();
				free(container_of(old, struct key, node));
			}

			key = calloc(1, sizeof(*key));
			key->key = v;
			old = cebu64_insert(&ceb_root, &key->node);
			if (old != &key->node)
				abort();

			if (debug > 1) {
				static int round;
				char cmd[100];
				size_t len;

				len = snprintf(cmd, sizeof(cmd), "%s %d/%d : %p %llu\n", orig_argv, round, round+count, old, (unsigned long long)v);
				cebu64_default_dump(&ceb_root, len < sizeof(cmd) ? cmd : orig_argv, old, round + 1);
				round++;
			}
		}
	} else if (test == 2) {
		while (count--) {
			v = rnd64() & mask;
			if (!count && debug > 2)
				cebu64_default_dump(&ceb_root, "step1", 0, (count + 1) * 6 + 1);
			old = cebu64_pick(&ceb_root, v);
			if (!count && debug > 2)
				cebu64_default_dump(&ceb_root, "step2", 0, (count + 1) * 6 + 2);
			back = old;
			while (old) {
				if (old && !count && debug > 2)
					cebu64_default_dump(&ceb_root, "step3", 0, (count + 1) * 6 + 3);
				old = cebu64_pick(&ceb_root, v);
				//if (old)
				//	printf("count=%d v=%u back=%p old=%p\n", count, v, back, old);
			}

			if (!count && debug > 2)
				cebu64_default_dump(&ceb_root, "step4", 0, (count + 1) * 6 + 4);

			//abort();
			//memset(old, 0, sizeof(*key));
			//if (old)
			//	free(container_of(old, struct key, node));

			key = calloc(1, sizeof(*key));
			key->key = v;
			old = cebu64_insert(&ceb_root, &key->node);
			if (old != &key->node)
				abort();

			if (!count && debug > 2)
				cebu64_default_dump(&ceb_root, "step5", 0, (count + 1) * 6 + 5);
			else if (debug > 1) {
				static int round;
				char cmd[100];
				size_t len;

				len = snprintf(cmd, sizeof(cmd), "%s %d/%d : %p %llu\n", orig_argv, round, round+count, old, (unsigned long long)v);
				cebu64_default_dump(&ceb_root, len < sizeof(cmd) ? cmd : orig_argv, old, (count + 1) * 6 + 6);
				round++;
			}
		}
	}

	if (debug > 1)
		cebu64_default_dump(0, 0, 0, 0); // epilogue

	if (debug == 1)
		cebu64_default_dump(&ceb_root, orig_argv, 0, 0);
	return 0;
}
