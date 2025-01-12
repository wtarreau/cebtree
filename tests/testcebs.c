#include <sys/time.h>

#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cebs_tree.h"

struct ceb_node *ceb_root = NULL;

struct key {
	struct ceb_node node;
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

void usage(const char *name)
{
	fprintf(stderr, "Usage: %s [-dD] entries lookups loops [first entries...]\n", name);
	exit(1);
}

int main(int argc, char **argv)
{
	int entries, lookups, loops, found, i;
	const struct ceb_node *old;
	struct ceb_node *prev, *ret;
	char *orig_argv;
	struct key *key;
	int debug = 0;
	int dump = 0;

	while (argc > 1 && argv[1][0] == '-') {
		if (argv[1][1] == 'd')
			debug++;
		else if (argv[1][1] == 'D')
			dump = 1;
		else
			usage(argv[0]);
		argv[1] = argv[0];
		argv++;
		argc--;
	}

	orig_argv = *argv;
	if (argc < 4)
		usage(argv[0]);

	entries = atoi(argv[1]);
	lookups = atoi(argv[2]);
	loops   = atoi(argv[3]);
	found   = 0;

	key = calloc(1, sizeof(*key));

	if (debug)
		fprintf(stderr, "inserting %d entries\n", entries);
	for (i = 0; i < entries; i++) {
		if (i < argc - 4) {
			strncpy(key->key, argv[i + 4], sizeof(key->key) - 1);
			key->key[sizeof(key->key) - 1] = 0;
		}
		else
			rnd64_to_str(key->key);
		old = cebs_lookup(&ceb_root, &key->key);
		if (old)
			fprintf(stderr, "Note: value %s already present at %p\n", key->key, old);

	try_again:
		prev = cebs_insert(&ceb_root, &key->node);
		if (prev != &key->node) {
			fprintf(stderr, "Note: failed to insert %p('%s'), previous was at %p('%s')\n", &key->node, key->key, prev, ((const struct key*)prev)->key);

			ret = cebs_delete(&ceb_root, prev);
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

	if (debug)
		fprintf(stderr, "Now looking up\n");

	while (loops-- > 0) {
		rnd32seed = RND32SEED;
		found = 0;
		for (i = 0; i < lookups; i++) {
			const void *kptr;

			if (i < argc - 4)
				kptr = argv[i + 4];
			else {
				rnd64_to_str(key->key);
				kptr = key->key;
			}
			old = cebs_lookup(&ceb_root, kptr);
			if (old)
				found++;
		}
	}

	if (debug)
		fprintf(stderr, "found=%d\n", found);

	/* now count elements */
	found = 0;
	ret = cebs_first(&ceb_root);
	if (debug > 1)
		fprintf(stderr, "%d: ret=%p\n", __LINE__, ret);

	while (ret) {
		prev = cebs_next(&ceb_root, ret);
		if (debug)
			fprintf(stderr, "   %4d: <%s>\n", found, ((const struct key *)ret)->key);
		found++;
		ret = prev;
	}

	printf("# Dump of all nodes using first() + next()\n");
	for (i = 0, old = cebs_first(&ceb_root); old; i++, old = cebs_next(&ceb_root, (struct ceb_node*)old))
		printf("# node[%d]=%p key=%s\n", i, old, container_of(old, struct key, node)->key);

	printf("# Dump of all nodes using last() + prev()\n");
	for (i = 0, old = cebs_last(&ceb_root); old; i++, old = cebs_prev(&ceb_root, (struct ceb_node*)old))
		printf("# node[%d]=%p key=%s\n", i, old, container_of(old, struct key, node)->key);

	if (!debug && dump)
		cebs_default_dump(&ceb_root, orig_argv, 0, 0);

	printf("# Removing all keys one at a time\n");
	while ((old = cebs_first(&ceb_root))) {
		cebs_delete(&ceb_root, (struct ceb_node*)old);
	}

	if (debug)
		fprintf(stderr, "counted %d elements\n", found);

	return 0;
}
