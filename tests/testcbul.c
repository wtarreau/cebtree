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
		printf("Insert failed, removing node %p before inserting again.\n", prev);
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

int main(int argc, char **argv)
{
	const struct cb_node *old;
	struct cb_node *ret, *next;
	char *argv0 = *argv, *larg;
	char *orig_argv;
	char *p;
	unsigned long v;
	int debug = 0;
	int found;
	int lookup_mode = 0; // 0 = EQ; -1 = LE; 1 = GE
	int do_count = 0;

	argv++; argc--;

	while (argc && **argv == '-') {
		if (strcmp(*argv, "-d") == 0)
			debug++;
		else if (strcmp(*argv, "-g") == 0)
			lookup_mode=1;
		else if (strcmp(*argv, "-l") == 0)
			lookup_mode=-1;
		else if (strcmp(*argv, "-G") == 0)
			lookup_mode=2;
		else if (strcmp(*argv, "-L") == 0)
			lookup_mode=-2;
		else if (strcmp(*argv, "-c") == 0)
			do_count=1;
		else {
			printf("Usage: %s [-dLlgGc]* [value]*\n", argv0);
			exit(1);
		}
		argc--; argv++;
	}

	orig_argv = larg = *argv;
	while (argc > 0) {
		v = atoll(argv[0]);
		if (lookup_mode == 0)
			old = cbul_lookup(&cb_root, v);
		else if (lookup_mode == -2)
			old = cbul_lookup_lt(&cb_root, v);
		else if (lookup_mode == -1)
			old = cbul_lookup_le(&cb_root, v);
		else if (lookup_mode == 1)
			old = cbul_lookup_ge(&cb_root, v);
		else if (lookup_mode == 2)
			old = cbul_lookup_gt(&cb_root, v);
		else
			old = NULL;

		if (old)
			fprintf(stderr, "Note: lookup of value %lu found at %p: %lu\n", v, old, container_of(old, struct key, node)->key);
		old = add_value(&cb_root, v);

		if (debug) {
			static int round;
			char cmd[100];
			size_t len;

			len = snprintf(cmd, sizeof(cmd), "%s [%d] +%lu", orig_argv, round, v);
			cbul_default_dump(&cb_root, len < sizeof(cmd) ? cmd : orig_argv, old);
			round++;
		}

		argv++;
		argc--;
	}

	/* rebuild non-debug args as a single string */
	for (p = orig_argv; p < larg; *p++ = ' ')
		p += strlen(p);

	/* now count elements */
	if (do_count) {
		found = 0;
		ret = cbul_first(&cb_root);
		if (debug)
			fprintf(stderr, "%d: ret=%p\n", __LINE__, ret);

		while (ret) {
			next = cbul_next(&cb_root, ret);
			if (debug)
				fprintf(stderr, "   %4d: @%p: <%#lx> next=%p\n", found, ret, ((const struct key *)ret)->key, next);
			found++;
			ret = next;
		}
		fprintf(stderr, "counted %d elements\n", found);
	}

	if (!debug)
		cbul_default_dump(&cb_root, orig_argv, 0);

	return 0;
}
