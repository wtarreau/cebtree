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

struct cba_node *cba_insert_st(struct cba_node **root, struct cba_node *node);
struct cba_node *cba_lookup_st(struct cba_node **root, const void *key);
struct cba_node *cba_delete_st(struct cba_node **root, struct cba_node *node);

struct cba_node *cba_root = NULL;

struct key {
	struct cba_node node;
	char key[21];
};

/*
 * unsigned long long ASCII representation
 *
 * return the last char '\0'. The destination must have at least 21 bytes of
 * space including the trailing \0.
 */
char *ulltoa(unsigned long long n, char *dst)
{
	int i = 0;
	char *res;

	switch(n) {
		case 1ULL ... 9ULL:
			i = 0;
			break;

		case 10ULL ... 99ULL:
			i = 1;
			break;

		case 100ULL ... 999ULL:
			i = 2;
			break;

		case 1000ULL ... 9999ULL:
			i = 3;
			break;

		case 10000ULL ... 99999ULL:
			i = 4;
			break;

		case 100000ULL ... 999999ULL:
			i = 5;
			break;

		case 1000000ULL ... 9999999ULL:
			i = 6;
			break;

		case 10000000ULL ... 99999999ULL:
			i = 7;
			break;

		case 100000000ULL ... 999999999ULL:
			i = 8;
			break;

		case 1000000000ULL ... 9999999999ULL:
			i = 9;
			break;

		case 10000000000ULL ... 99999999999ULL:
			i = 10;
			break;

		case 100000000000ULL ... 999999999999ULL:
			i = 11;
			break;

		case 1000000000000ULL ... 9999999999999ULL:
			i = 12;
			break;

		case 10000000000000ULL ... 99999999999999ULL:
			i = 13;
			break;

		case 100000000000000ULL ... 999999999999999ULL:
			i = 14;
			break;

		case 1000000000000000ULL ... 9999999999999999ULL:
			i = 15;
			break;

		case 10000000000000000ULL ... 99999999999999999ULL:
			i = 16;
			break;

		case 100000000000000000ULL ... 999999999999999999ULL:
			i = 17;
			break;

		case 1000000000000000000ULL ... 9999999999999999999ULL:
			i = 18;
			break;

		case 10000000000000000000ULL ... ~0ULL:
			i = 19;
			break;
	}

	res = dst + i + 1;
	*res = '\0';
	for (; i >= 0; i--) {
		dst[i] = n % 10ULL + '0';
		n /= 10ULL;
	}
	return res;
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

static void rnd64_to_str(char *dst)
{
	ulltoa(rnd64(), dst);
}

int main(int argc, char **argv)
{
	int entries, lookups, loops, found, i;
	const struct cba_node *old;
	struct cba_node *prev, *ret;
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
		rnd64_to_str(key->key);
		old = cba_lookup_st(&cba_root, &key->key);
		if (old)
			fprintf(stderr, "Note: value %s already present at %p\n", key->key, old);

	try_again:
		prev = cba_insert_st(&cba_root, &key->node);
		if (prev != &key->node) {
			fprintf(stderr, "Note: failed to insert %s, previous was at %p\n", key->key, old);
			ret = cba_delete_st(&cba_root, prev);
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
			rnd64_to_str(key->key);
			old = cba_lookup_st(&cba_root, &key->key);
			if (old)
				found++;
		}
	}

	printf("found=%d\n", found);
	return 0;
}
