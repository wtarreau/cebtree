#include <sys/time.h>

#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cbul_tree.h"

struct cb_node *cb_root = NULL;

struct key {
	struct cb_node node;
	unsigned long key;
};

struct cb_node *add_value(struct cb_node **root, unsigned long value)
{
	struct key *key;
	struct cb_node *prev, *ret;

	key = calloc(1, sizeof(*key));
	key->key = value;
	do {
		prev = cbul_insert(root, &key->node);
		if (prev == &key->node)
			return prev; // was properly inserted
		/* otherwise was already there, let's try to remove it */
		ret = cbul_delete(root, prev);
		if (ret != prev) {
			/* was not properly removed either: THIS IS A BUG! */
			printf("failed to insert %p(%lu) because %p has the same key and could not be removed because returns %p\n",
			       &key->node, key->key, prev, ret);
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

static unsigned long rnd64()
{
	return ((unsigned long)rnd32() << 32) + rnd32();
}

int main(int argc, char **argv)
{
	struct cb_node *old, *back __attribute__((unused));
	char *orig_argv, *argv0 = *argv, *larg;
	struct key *key;
	char *p;
	unsigned long v;
	int test = 0;
	unsigned long mask = ~0ULL;
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

	orig_argv = larg = *argv;

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

	if (test == 0) {
		while (count--) {
			v = rnd64() & mask;
			old = cbul_lookup(&cb_root, v);
			if (old) {
				if (cbul_delete(&cb_root, old) != old)
					abort();
				free(container_of(old, struct key, node));
			}
			else {
				key = calloc(1, sizeof(*key));
				key->key = v;
				old = cbul_insert(&cb_root, &key->node);
				if (old != &key->node)
					abort();
			}
		}
	} else if (test == 1) {
		while (count--) {
			v = rnd64() & mask;
			old = cbul_lookup(&cb_root, v);
			if (old) {
				if (cbul_delete(&cb_root, old) != old)
					abort();
				free(container_of(old, struct key, node));
			}

			key = calloc(1, sizeof(*key));
			key->key = v;
			old = cbul_insert(&cb_root, &key->node);
			if (old != &key->node)
				abort();

			if (debug > 1) {
				static int round;
				char cmd[100];
				size_t len;

				len = snprintf(cmd, sizeof(cmd), "%s %d/%d : %p %lu\n", orig_argv, round, round+count, old, v);
				cbul_default_dump(&cb_root, len < sizeof(cmd) ? cmd : orig_argv, old);
				round++;
			}
		}
	} else if (test == 2) {
		while (count--) {
			v = rnd64() & mask;
			if (!count && debug > 2)
				cbul_default_dump(&cb_root, "step1", 0);
			old = cbul_pick(&cb_root, v);
			if (!count && debug > 2)
				cbul_default_dump(&cb_root, "step2", 0);
			back = old;
			while (old) {
				if (old && !count && debug > 2)
					cbul_default_dump(&cb_root, "step3", 0);
				old = cbul_pick(&cb_root, v);
				//if (old)
				//	printf("count=%d v=%u back=%p old=%p\n", count, v, back, old);
			}

			if (!count && debug > 2)
				cbul_default_dump(&cb_root, "step4", 0);

			//abort();
			//memset(old, 0, sizeof(*key));
			//if (old)
			//	free(container_of(old, struct key, node));

			key = calloc(1, sizeof(*key));
			key->key = v;
			old = cbul_insert(&cb_root, &key->node);
			if (old != &key->node)
				abort();

			if (!count && debug > 2)
				cbul_default_dump(&cb_root, "step5", 0);
			else if (debug > 1) {
				static int round;
				char cmd[100];
				size_t len;

				len = snprintf(cmd, sizeof(cmd), "%s %d/%d : %p %lu\n", orig_argv, round, round+count, old, v);
				cbul_default_dump(&cb_root, len < sizeof(cmd) ? cmd : orig_argv, old);
				round++;
			}
		}
	}

	if (debug == 1)
		cbul_default_dump(&cb_root, orig_argv, 0);
	return 0;
}
