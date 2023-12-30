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

#define RND32SEED 2463534242U
static uint32_t rnd32seed = RND32SEED;
static uint32_t rnd32()
{
	rnd32seed ^= rnd32seed << 13;
	rnd32seed ^= rnd32seed >> 17;
	rnd32seed ^= rnd32seed << 5;
	return rnd32seed;
}

static unsigned long rndl()
{
	if (sizeof(long) > 4)
		return ((uint64_t)rnd32() << 32) + rnd32();
	else
		return rnd32();
}

int main(int argc, char **argv)
{
	int entries, lookups, loops, found, i;
	const struct cb_node *old;
	struct cb_node *prev, *ret;
	struct key *key;

	if (argc != 4) {
		printf("Usage: %s entries lookups loops\n", argv[0]);
		exit(1);
	}

	entries = atoi(argv[1]);
	lookups = atoi(argv[2]);
	loops   = atoi(argv[3]);
	found   = 0;

	key = calloc(1, sizeof(*key));

	for (i = 0; i < entries; i++) {
		key->key = rndl();
		old = cbul_lookup(&cb_root, key->key);
		if (old)
			fprintf(stderr, "Note: value %#lx already present at %p\n", key->key, old);

	try_again:
		prev = cbul_insert(&cb_root, &key->node);
		if (prev != &key->node) {
			fprintf(stderr, "Note: failed to insert %#lx, previous was at %p\n", key->key, old);
			ret = cbul_delete(&cb_root, prev);
			if (ret != prev) {
				/* was not properly removed either: THIS IS A BUG! */
				fprintf(stderr, "failed to remove %p (returned %p)\n", prev, ret);
				abort();
			}
			free(ret);
			goto try_again;
		}
		/* key was inserted, we need a new one */
		key = calloc(1, sizeof(*key));
	}

	printf("Now looking up\n");

	while (loops-- > 0) {
		rnd32seed = RND32SEED;
		found = 0;
		for (i = 0; i < lookups; i++) {
			key->key = rndl();
			old = cbul_lookup(&cb_root, key->key);
			if (old)
				found++;
		}
	}

	printf("found=%d\n", found);
	return 0;
}
