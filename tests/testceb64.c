#include <sys/time.h>

#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ceb64_tree.h"

#ifndef container_of
#define container_of(ptr, type, name) ((type *)(((char *)(ptr)) - ((long)&((type *)0)->name)))
#endif

struct ceb_node *ceb_root = NULL;

struct key {
	struct ceb_node node;
	uint64_t key;
};

struct ceb_node *add_value(struct ceb_node **root, uint64_t value)
{
	struct key *key;
	struct ceb_node *prev, *ret;

	key = calloc(1, sizeof(*key));
	key->key = value;
	do {
		prev = ceb64_insert(root, &key->node);
		if (prev == &key->node)
			return prev; // was properly inserted
		/* otherwise was already there, let's try to remove it */
		fprintf(stderr, "Insert failed, removing node %p before inserting again.\n", prev);
		ret = ceb64_delete(root, prev);
		if (ret != prev) {
			/* was not properly removed either: THIS IS A BUG! */
			fprintf(stderr, "failed to insert %p(%llu) because %p has the same key and could not be removed because returns %p\n",
			       &key->node, (unsigned long long)key->key, prev, ret);
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
	uint64_t v;
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
		ceb64_default_dump(0, orig_argv, 0, 0); // prologue

	while (argc > 0) {
		v = atoll(argv[0]);
		old = ceb64_lookup(&ceb_root, v);
		if (old)
			fprintf(stderr, "Note: value %llu already present at %p\n", (unsigned long long)v, old);
		old = add_value(&ceb_root, v);

		if (debug) {
			char cmd[100];
			size_t len;

			len = snprintf(cmd, sizeof(cmd), "%s [%d] +%llu", orig_argv, debug, (unsigned long long)v);
			ceb64_default_dump(&ceb_root, len < sizeof(cmd) ? cmd : orig_argv, old, debug);
			debug++;
		}

		argv++;
		argc--;
	}

	/* rebuild non-debug args as a single string */
	for (p = orig_argv; p < larg; *p++ = ' ')
		p += strlen(p);

	printf("# Dump of all nodes using first() + next()\n");
	for (i = 0, old = ceb64_first(&ceb_root); old; i++, old = ceb64_next(&ceb_root, (struct ceb_node*)old))
		printf("# node[%d]=%p key=%llu\n", i, old, (unsigned long long)container_of(old, struct key, node)->key);

	printf("# Dump of all nodes using last() + prev()\n");
	for (i = 0, old = ceb64_last(&ceb_root); old; i++, old = ceb64_prev(&ceb_root, (struct ceb_node*)old))
		printf("# node[%d]=%p key=%llu\n", i, old, (unsigned long long)container_of(old, struct key, node)->key);

	printf("# Removing all keys one at a time\n");
	while ((old = ceb64_first(&ceb_root))) {
		ceb64_delete(&ceb_root, (struct ceb_node*)old);
		if (debug) {
			char cmd[100];
			size_t len;

			len = snprintf(cmd, sizeof(cmd), "delete(%p:%llu)", old, (unsigned long long)container_of(old, struct key, node)->key);
			ceb64_default_dump(&ceb_root, len < sizeof(cmd) ? cmd : orig_argv, old, debug);
			debug++;
		}
	}

	if (debug)
		ceb64_default_dump(0, 0, 0, 0); // epilogue
	else
		ceb64_default_dump(&ceb_root, orig_argv, 0, 0);

	return 0;
}
