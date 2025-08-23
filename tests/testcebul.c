#include <sys/time.h>

#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cebul_tree.h"

#ifndef container_of
#define container_of(ptr, type, name) ((type *)(((char *)(ptr)) - ((long)&((type *)0)->name)))
#endif

struct ceb_root *ceb_root = NULL;

struct key {
	struct ceb_node node;
	unsigned long key;
};

struct ceb_node *add_value(struct ceb_root **root, unsigned long value)
{
	struct key *key;
	struct ceb_node *prev, *ret;

	key = calloc(1, sizeof(*key));
	key->key = value;
	do {
		prev = cebul_imm_insert(root, &key->node);
		if (prev == &key->node)
			return prev; // was properly inserted
		/* otherwise was already there, let's try to remove it */
		fprintf(stderr, "Insert failed, removing node %p before inserting again.\n", prev);
		ret = cebul_imm_delete(root, prev);
		if (ret != prev) {
			/* was not properly removed either: THIS IS A BUG! */
			fprintf(stderr, "failed to insert %p(%lu) because %p has the same key and could not be removed because returns %p\n",
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
	struct ceb_node *ret, *next;
	char *argv0 = *argv, *larg;
	char *orig_argv;
	char *p;
	unsigned long v;
	int debug = 0;
	int found;
	int lookup_mode = 0; // 0 = EQ; -1 = LE; 1 = GE
	int do_count = 0;
	int i;

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
			fprintf(stderr, "Usage: %s [-dLlgGc]* [value]*\n", argv0);
			exit(1);
		}
		argc--; argv++;
	}

	orig_argv = larg = (argc > 0) ? *argv : argv0;

	if (debug)
		cebul_imm_default_dump(0, orig_argv, 0, 0); // prologue

	while (argc > 0) {
		v = atoll(argv[0]);
		if (lookup_mode == 0)
			old = cebul_imm_lookup(&ceb_root, v);
		else if (lookup_mode == -2)
			old = cebul_imm_lookup_lt(&ceb_root, v);
		else if (lookup_mode == -1)
			old = cebul_imm_lookup_le(&ceb_root, v);
		else if (lookup_mode == 1)
			old = cebul_imm_lookup_ge(&ceb_root, v);
		else if (lookup_mode == 2)
			old = cebul_imm_lookup_gt(&ceb_root, v);
		else
			old = NULL;

		if (old)
			fprintf(stderr, "Note: lookup of value %lu found at %p: %lu\n", v, old, container_of(old, struct key, node)->key);
		old = add_value(&ceb_root, v);

		if (debug) {
			char cmd[100];
			size_t len;

			len = snprintf(cmd, sizeof(cmd), "%s [%d] +%lu", orig_argv, debug, v);
			cebul_imm_default_dump(&ceb_root, len < sizeof(cmd) ? cmd : orig_argv, old, debug);
			debug++;
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
		ret = cebul_imm_first(&ceb_root);
		if (debug)
			fprintf(stderr, "%d: ret=%p\n", __LINE__, ret);

		while (ret) {
			next = cebul_imm_next(&ceb_root, ret);
			if (debug)
				fprintf(stderr, "   %4d: @%p: <%#lx> next=%p\n", found, ret, ((const struct key *)ret)->key, next);
			found++;
			ret = next;
		}
		fprintf(stderr, "counted %d elements\n", found);
	}

	printf("# Dump of all nodes using first() + next()\n");
	for (i = 0, old = NULL, node = cebul_imm_first(&ceb_root); node; i++, node = cebul_imm_next(&ceb_root, (struct ceb_node*)(old = node))) {
		if (node == old) {
			printf("# BUG! prev(%p) = %p!\n", old, node);
			exit(1);
		}
		printf("# node[%d]=%p key=%lu\n", i, node, container_of(node, struct key, node)->key);
	}

	printf("# Dump of all nodes using last() + prev()\n");
	for (i = 0, old = NULL, node = cebul_imm_last(&ceb_root); node; i++, node = cebul_imm_prev(&ceb_root, (struct ceb_node*)(old = node))) {
		if (node == old) {
			printf("# BUG! prev(%p) = %p!\n", old, node);
			exit(1);
		}
		printf("# node[%d]=%p key=%lu\n", i, node, container_of(node, struct key, node)->key);
	}

	printf("# Removing all keys one at a time\n");
	for (old = NULL; (node = cebul_imm_first(&ceb_root)); old = node) {
		if (node == old) {
			printf("# BUG! first() after delete(%p) = %p!\n", old, node);
			exit(1);
		}
		cebul_imm_delete(&ceb_root, (struct ceb_node*)node);
		if (debug) {
			char cmd[100];
			size_t len;

			len = snprintf(cmd, sizeof(cmd), "delete(%p:%lu)", node, container_of(node, struct key, node)->key);
			cebul_imm_default_dump(&ceb_root, len < sizeof(cmd) ? cmd : orig_argv, node, debug);
			debug++;
		}
	}

	if (debug)
		cebul_imm_default_dump(0, 0, 0, 0); // epilogue
	else
		cebul_imm_default_dump(&ceb_root, orig_argv, 0, 0);

	return 0;
}
