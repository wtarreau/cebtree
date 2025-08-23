#include <sys/time.h>

#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cebub_tree.h"

#ifndef container_of
#define container_of(ptr, type, name) ((type *)(((char *)(ptr)) - ((long)&((type *)0)->name)))
#endif

struct ceb_root *ceb_root = NULL;

struct key {
	struct ceb_node node;
	uint64_t key;
};

struct ceb_node *add_value(struct ceb_root **root, uint64_t value)
{
	struct key *key;
	struct ceb_node *prev, *ret;

	key = calloc(1, sizeof(*key));
	key->key = value;
	do {
		prev = cebub_imm_insert(root, &key->node, sizeof(key->key));
		if (prev == &key->node)
			return prev; // was properly inserted
		/* otherwise was already there, let's try to remove it */
		ret = cebub_imm_delete(root, prev, sizeof(key->key));
		if (ret != prev) {
			/* was not properly removed either: THIS IS A BUG! */
			printf("failed to insert %p(%llx) because %p has the same key and could not be removed because returns %p\n",
			       &key->node, (long long)key->key, prev, ret);
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
	int entries, lookups, loops, found, i;
	const struct ceb_node *old;
	uint64_t v;

	if (argc != 4) {
		printf("Usage: %s entries lookups loops\n", argv[0]);
		exit(1);
	}

	entries = atoi(argv[1]);
	lookups = atoi(argv[2]);
	loops   = atoi(argv[3]);
	found   = 0;

	for (i = 0; i < entries; i++) {
		v = rnd64();
		old = cebub_imm_lookup(&ceb_root, (const void*)&v, sizeof(v));
		if (old)
			fprintf(stderr, "Note: value %llx already present at %p\n", (long long)v, old);
		old = add_value(&ceb_root, v);
	}

	while (loops-- > 0) {
		rnd32seed = RND32SEED;
		found = 0;
		for (i = 0; i < lookups; i++) {
			v = rnd64();
			old = cebub_imm_lookup(&ceb_root, (const void*)&v, sizeof(v));
			if (old)
				found++;
		}
	}

	printf("found=%d\n", found);
	return 0;
}
