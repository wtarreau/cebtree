#include <sys/time.h>

#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ceb32_tree.h"

#ifndef container_of
#define container_of(ptr, type, name) ((type *)(((char *)(ptr)) - ((long)&((type *)0)->name)))
#endif

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
		prev = ceb32_insert(root, &key->node);
		if (prev == &key->node)
			return prev; // was properly inserted
		/* otherwise was already there, let's try to remove it */
		fprintf(stderr, "Insert failed, removing node %p before inserting again.\n", prev);
		ret = ceb32_delete(root, prev);
		if (ret != prev) {
			/* was not properly removed either: THIS IS A BUG! */
			fprintf(stderr, "failed to insert %p(%u) because %p has the same key and could not be removed because returns %p\n",
			       &key->node, key->key, prev, ret);
			free(key);
			return NULL;
		}
		free(container_of(ret, struct key, node));
	} while (1);
}

int main(int argc, char **argv)
{
	const struct ceb_node *old, *node;
	char *argv0 = *argv, *larg;
	char *orig_argv;
	char *p;
	uint32_t v;
	int debug = 0;
	int i;

	argv++; argc--;

	while (argc && **argv == '-') {
		if (strcmp(*argv, "-d") == 0)
			debug++;
		else {
			fprintf(stderr, "Usage: %s [-d]* [value]*\n", argv0);
			exit(1);
		}
		argc--; argv++;
	}

	orig_argv = larg = (argc > 0) ? *argv : argv0;

	if (debug)
		ceb32_default_dump(0, orig_argv, 0, 0); // prologue

	while (argc > 0) {
		v = atoi(argv[0]);
		old = ceb32_lookup(&ceb_root, v);
		if (old)
			fprintf(stderr, "Note: value %u already present at %p\n", v, old);
		old = add_value(&ceb_root, v);

		if (debug) {
			char cmd[100];
			size_t len;

			len = snprintf(cmd, sizeof(cmd), "%s [%d] +%d", orig_argv, debug, v);
			ceb32_default_dump(&ceb_root, len < sizeof(cmd) ? cmd : orig_argv, old, debug);
			debug++;
		}

		argv++;
		argc--;
	}

	/* rebuild non-debug args as a single string */
	for (p = orig_argv; p < larg; *p++ = ' ')
		p += strlen(p);

	printf("# Dump of all nodes using first() + next()\n");
	for (i = 0, old = NULL, node = ceb32_first(&ceb_root); node; i++, node = ceb32_next(&ceb_root, (struct ceb_node*)(old = node))) {
		if (node == old) {
			printf("# BUG! prev(%p) = %p!\n", old, node);
			exit(1);
		}
		printf("# node[%d]=%p key=%u\n", i, node, container_of(node, struct key, node)->key);
	}

	printf("# Dump of all nodes using last() + prev()\n");
	for (i = 0, old = NULL, node = ceb32_last(&ceb_root); node; i++, node = ceb32_prev(&ceb_root, (struct ceb_node*)(old = node))) {
		if (node == old) {
			printf("# BUG! prev(%p) = %p!\n", old, node);
			exit(1);
		}
		printf("# node[%d]=%p key=%u\n", i, node, container_of(node, struct key, node)->key);
	}

	printf("# Removing all keys one at a time\n");
	for (old = NULL; (node = ceb32_first(&ceb_root)); old = node) {
		if (node == old) {
			printf("# BUG! first() after delete(%p) = %p!\n", old, node);
			exit(1);
		}
		ceb32_delete(&ceb_root, (struct ceb_node*)node);
		if (debug) {
			char cmd[100];
			size_t len;

			len = snprintf(cmd, sizeof(cmd), "delete(%p:%d)", node, container_of(node, struct key, node)->key);
			ceb32_default_dump(&ceb_root, len < sizeof(cmd) ? cmd : orig_argv, node, debug);
			debug++;
		}
	}

	if (debug)
		ceb32_default_dump(0, 0, 0, 0); // epilogue
	else
		ceb32_default_dump(&ceb_root, orig_argv, 0, 0);

	return 0;
}
