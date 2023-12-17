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

struct cba_node *cba_insert_mb(struct cba_node **root, struct cba_node *node, size_t len);
struct cba_node *cba_lookup_mb(struct cba_node **root, const unsigned char *key, size_t len);
struct cba_node *cba_delete_mb(struct cba_node **root, struct cba_node *node, size_t len);

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
		prev = cba_insert_mb(root, &key->node, sizeof(uint32_t));
		if (prev == &key->node)
			return prev; // was properly inserted
		/* otherwise was already there, let's try to remove it */
		printf("Insert failed, removing node %p before inserting again.\n", prev);
		ret = cba_delete_mb(root, prev, sizeof(uint32_t));
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
	const struct cba_node *old;
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
		old = cba_lookup_mb(&cba_root, &v, sizeof(uint32_t));
		if (old)
			fprintf(stderr, "Note: value %u already present at %p\n", v, old);
		old = add_value(&cba_root, v);

		if (debug) {
			static int round;
			char cmd[100];
			int len;

			len = snprintf(cmd, sizeof(cmd), "%s [%d] +%d", orig_argv, round, v);
			//cbu32_default_dump(&cba_root, len < sizeof(cmd) ? cmd : orig_argv, old);
			round++;
		}

		argv++;
		argc--;
	}

	/* rebuild non-debug args as a single string */
	for (p = orig_argv; p < larg; *p++ = ' ')
		p += strlen(p);

	//if (!debug)
	//	cbu32_default_dump(&cba_root, orig_argv, 0);

	return 0;
}
