#include <sys/time.h>

#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cbatree.h"

struct cb_node *cbub_insert(struct cb_node **root, struct cb_node *node, size_t len);
struct cb_node *cbub_lookup(struct cb_node **root, const unsigned char *key, size_t len);
struct cb_node *cbub_delete(struct cb_node **root, struct cb_node *node, size_t len);

struct cb_node *cb_root = NULL;

struct key {
	struct cb_node node;
	uint64_t key;
};

struct cb_node *add_value(struct cb_node **root, uint64_t value)
{
	struct key *key;
	struct cb_node *prev, *ret;

	key = calloc(1, sizeof(*key));
	key->key = value;
	do {
		prev = cbub_insert(root, &key->node, sizeof(key->key));
		if (prev == &key->node)
			return prev; // was properly inserted
		/* otherwise was already there, let's try to remove it */
		ret = cbub_delete(root, prev, sizeof(key->key));
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
	const struct cb_node *old;
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
		old = cbub_lookup(&cb_root, (const void*)&v, sizeof(v));
		if (old)
			fprintf(stderr, "Note: value %llx already present at %p\n", (long long)v, old);
		old = add_value(&cb_root, v);
	}

	while (loops-- > 0) {
		rnd32seed = RND32SEED;
		found = 0;
		for (i = 0; i < lookups; i++) {
			v = rnd64();
			old = cbub_lookup(&cb_root, (const void*)&v, sizeof(v));
			if (old)
				found++;
		}
	}

	printf("found=%d\n", found);
	return 0;
}
