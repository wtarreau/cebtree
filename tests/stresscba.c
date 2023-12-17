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

void cbu32_default_dump(struct cba_node **cba_root, const char *label, const void *ctx);
struct cba_node *cbu32_insert(struct cba_node **root, struct cba_node *node);
struct cba_node *cbu32_lookup(struct cba_node **root, u32 key);
struct cba_node *cbu32_delete(struct cba_node **root, struct cba_node *node);
struct cba_node *cbu32_pick(struct cba_node **root, u32 key);

struct cba_node *cba_root = NULL;

struct key {
	struct cba_node node;
	uint32_t key;
};

struct cba_node *add_value(struct cba_node **root, uint32_t value)
{
	struct key *key;
	struct cba_node *prev, *ret;

	key = calloc(1, sizeof(*key));
	key->key = value;
	do {
		prev = cbu32_insert(root, &key->node);
		if (prev == &key->node)
			return prev; // was properly inserted
		/* otherwise was already there, let's try to remove it */
		ret = cbu32_delete(root, prev);
		if (ret != prev) {
			/* was not properly removed either: THIS IS A BUG! */
			printf("failed to insert %p(%u) because %p has the same key and could not be removed because returns %p\n",
			       &key->node, key->key, prev, ret);
			free(key);
			return NULL;
		}
		free(container_of(ret, struct key, node));
	} while (1);
}

static uint32_t rnd32seed = 2463534242U;
static uint32_t rnd32()
{
	rnd32seed ^= rnd32seed << 13;
	rnd32seed ^= rnd32seed >> 17;
	rnd32seed ^= rnd32seed << 5;
	return rnd32seed;
}

int main(int argc, char **argv)
{
	struct cba_node *old, *back;
	char *orig_argv, *argv0 = *argv, *larg;
	struct key *key;
	char *p;
	uint32_t v;
	int test = 0;
	uint32_t mask = 0xffffffff;
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
			v = rnd32() & mask;
			old = cbu32_lookup(&cba_root, v);
			if (old) {
				if (cbu32_delete(&cba_root, old) != old)
					abort();
				free(container_of(old, struct key, node));
			}
			else {
				key = calloc(1, sizeof(*key));
				key->key = v;
				old = cbu32_insert(&cba_root, &key->node);
				if (old != &key->node)
					abort();
			}
		}
	} else if (test == 1) {
		while (count--) {
			v = rnd32() & mask;
			old = cbu32_lookup(&cba_root, v);
			if (old) {
				if (cbu32_delete(&cba_root, old) != old)
					abort();
				free(container_of(old, struct key, node));
			}

			key = calloc(1, sizeof(*key));
			key->key = v;
			old = cbu32_insert(&cba_root, &key->node);
			if (old != &key->node)
				abort();

			if (debug > 1) {
				static int round;
				char cmd[100];
				int len;

				len = snprintf(cmd, sizeof(cmd), "%s %d/%d : %p %d\n", orig_argv, round, round+count, old, v);
				cbu32_default_dump(&cba_root, len < sizeof(cmd) ? cmd : orig_argv, old);
				round++;
			}
		}
	} else if (test == 2) {
		while (count--) {
			v = rnd32() & mask;
			if (!count && debug > 2)
				cbu32_default_dump(&cba_root, "step1", 0);
			old = cbu32_pick(&cba_root, v);
			if (!count && debug > 2)
				cbu32_default_dump(&cba_root, "step2", 0);
			back = old;
			while (old) {
				if (old && !count && debug > 2)
					cbu32_default_dump(&cba_root, "step3", 0);
				old = cbu32_pick(&cba_root, v);
				//if (old)
				//	printf("count=%d v=%u back=%p old=%p\n", count, v, back, old);
			}

			if (!count && debug > 2)
				cbu32_default_dump(&cba_root, "step4", 0);

			//abort();
			//memset(old, 0, sizeof(*key));
			//if (old)
			//	free(container_of(old, struct key, node));

			key = calloc(1, sizeof(*key));
			key->key = v;
			old = cbu32_insert(&cba_root, &key->node);
			if (old != &key->node)
				abort();

			if (!count && debug > 2)
				cbu32_default_dump(&cba_root, "step5", 0);
			else if (debug > 1) {
				static int round;
				char cmd[100];
				int len;

				len = snprintf(cmd, sizeof(cmd), "%s %d/%d : %p %d\n", orig_argv, round, round+count, old, v);
				cbu32_default_dump(&cba_root, len < sizeof(cmd) ? cmd : orig_argv, old);
				round++;
			}
		}
	}

	if (debug == 1)
		cbu32_default_dump(&cba_root, orig_argv, 0);
	return 0;
}
