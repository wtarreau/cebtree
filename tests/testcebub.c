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

struct ceb_node *ceb_root = NULL;

struct key {
	struct ceb_node node;
	uint32_t key;
};

struct ceb_node *add_value(struct ceb_node **root, uint32_t value)
{
	struct key *key;
	struct ceb_node *prev, *ret;

	key = calloc(1, sizeof(*key));
	key->key = value;
	do {
		prev = cebub_insert(root, &key->node, sizeof(uint32_t));
		if (prev == &key->node)
			return prev; // was properly inserted
		/* otherwise was already there, let's try to remove it */
		printf("Insert failed, removing node %p before inserting again.\n", prev);
		ret = cebub_delete(root, prev, sizeof(uint32_t));
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

int main(int argc, char **argv)
{
	const struct ceb_node *old;
	char *argv0 = *argv, *larg;
	char *orig_argv;
	char *p;
	uint32_t v;
	int debug = 0;

	argv++; argc--;

	while (argc && **argv == '-') {
		if (strcmp(*argv, "-d") == 0)
			debug++;
		else {
			printf("Usage: %s [-d]* [value]*\n", argv0);
			exit(1);
		}
		argc--; argv++;
	}

	orig_argv = larg = *argv;
	while (argc > 0) {
		v = atoi(argv[0]);
		old = cebub_lookup(&ceb_root, &v, sizeof(uint32_t));
		if (old)
			fprintf(stderr, "Note: value %u already present at %p\n", v, old);
		old = add_value(&ceb_root, v);

		if (debug) {
			static int round;
			char cmd[100];
			int len __attribute__((unused));

			len = snprintf(cmd, sizeof(cmd), "%s [%d] +%d", orig_argv, round, v);
			//cebu32_default_dump(&ceb_root, len < sizeof(cmd) ? cmd : orig_argv, old);
			round++;
		}

		argv++;
		argc--;
	}

	/* rebuild non-debug args as a single string */
	for (p = orig_argv; p < larg; *p++ = ' ')
		p += strlen(p);

	//if (!debug)
	//	cebu32_default_dump(&ceb_root, orig_argv, 0);

	return 0;
}
