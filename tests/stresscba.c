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

void *cba_dump_tree_u32(struct cba_node *node, u32 pxor, void *last,
			int level,
			void (*node_dump)(struct cba_node *node, int level, const void *ctx),
			void (*leaf_dump)(struct cba_node *node, int level, const void *ctx),
			const void *ctx);

struct cba_node *cba_insert_u32(struct cba_node **root, struct cba_node *node);
struct cba_node *cba_lookup_u32(struct cba_node **root, u32 key);
struct cba_node *cba_delete_u32(struct cba_node **root, struct cba_node *node);
struct cba_node *cba_pick_u32(struct cba_node **root, u32 key);

struct cba_node *cba_root = NULL;

struct key {
	struct cba_node node;
	uint32_t key;
};

static void dump_node(struct cba_node *node, int level, const void *ctx)
{
	struct key *key = container_of(node, struct key, node);
	u32 pxor, lxor, rxor;

	/* xor of the keys of the two lower branches */
	pxor = container_of(__cba_clrtag(node->b[0]), struct key, node)->key ^
		container_of(__cba_clrtag(node->b[1]), struct key, node)->key;

	printf("  \"%lx_n\" [label=\"%lx\\nlev=%d\\nkey=%u\" fillcolor=\"lightskyblue1\"];\n",
	       (long)node, (long)node, level, key->key);

	/* xor of the keys of the left branch's lower branches */
	lxor = container_of(__cba_clrtag(((struct cba_node*)__cba_clrtag(node->b[0]))->b[0]), struct key, node)->key ^
		container_of(__cba_clrtag(((struct cba_node*)__cba_clrtag(node->b[0]))->b[1]), struct key, node)->key;

	printf("  \"%lx_n\" -> \"%lx_%c\" [taillabel=\"L\"];\n",
	       (long)node, (long)__cba_clrtag(node->b[0]),
	       (((long)node->b[0] & 1) || (lxor < pxor && ((struct cba_node*)node->b[0])->b[0] != ((struct cba_node*)node->b[0])->b[1])) ? 'n' : 'l');

	/* xor of the keys of the right branch's lower branches */
	rxor = container_of(__cba_clrtag(((struct cba_node*)__cba_clrtag(node->b[1]))->b[0]), struct key, node)->key ^
		container_of(__cba_clrtag(((struct cba_node*)__cba_clrtag(node->b[1]))->b[1]), struct key, node)->key;

	printf("  \"%lx_n\" -> \"%lx_%c\" [taillabel=\"R\"];\n",
	       (long)node, (long)__cba_clrtag(node->b[1]),
	       (((long)node->b[1] & 1) || (rxor < pxor && ((struct cba_node*)node->b[1])->b[0] != ((struct cba_node*)node->b[1])->b[1])) ? 'n' : 'l');
}

static void dump_leaf(struct cba_node *node, int level, const void *ctx)
{
	struct key *key = container_of(node, struct key, node);

	if (node->b[0] == node->b[1])
		printf("  \"%lx_l\" [label=\"%lx\\nlev=%d\\nkey=%u\\n\" fillcolor=\"green\"];\n",
		       (long)node, (long)node, level, key->key);
	else
		printf("  \"%lx_l\" [label=\"%lx\\nlev=%d\\nkey=%u\\n\" fillcolor=\"yellow\"];\n",
		       (long)node, (long)node, level, key->key);
}

struct cba_node *add_value(struct cba_node **root, uint32_t value)
{
	struct key *key;
	struct cba_node *prev, *ret;

	key = calloc(1, sizeof(*key));
	key->key = value;
	do {
		prev = cba_insert_u32(root, &key->node);
		if (prev == &key->node)
			return prev; // was properly inserted
		/* otherwise was already there, let's try to remove it */
		ret = cba_delete_u32(root, prev);
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

void dump(struct cba_node **cba_root, const char *label, const void *ctx)
{
	struct cba_node *node;

	printf("#########################\n");
	printf("digraph cba_tree_u32 {\n"
	       "  fontname=\"fixed\";\n"
	       "  fontsize=8\n"
	       "  label=\"%s\"\n"
	       "", label);

	printf("  node [fontname=\"fixed\" fontsize=8 shape=\"box\" style=\"filled\" color=\"black\" fillcolor=\"white\"];\n"
	       "  edge [fontname=\"fixed\" fontsize=8 style=\"solid\" color=\"magenta\" dir=\"forward\"];\n"
	       "  \"%lx_n\" [label=\"root\\n%lx\"]\n", (long)cba_root, (long)cba_root);

	node = *cba_root;
	if (node) {
		/* under the root we've either a node or the first leaf */
		printf("  \"%lx_n\" -> \"%lx_%c\" [taillabel=\"B\"];\n",
		       (long)cba_root, (long)node,
		       (node->b[0] == node->b[1]) ? 'l' : 'n');
	}

	cba_dump_tree_u32(*cba_root, 0, NULL, 0, dump_node, dump_leaf, ctx);

	printf("}\n");
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
			old = cba_lookup_u32(&cba_root, v);
			if (old) {
				if (cba_delete_u32(&cba_root, old) != old)
					abort();
				free(container_of(old, struct key, node));
			}
			else {
				key = calloc(1, sizeof(*key));
				key->key = v;
				old = cba_insert_u32(&cba_root, &key->node);
				if (old != &key->node)
					abort();
			}
		}
	} else if (test == 1) {
		while (count--) {
			v = rnd32() & mask;
			old = cba_lookup_u32(&cba_root, v);
			if (old) {
				if (cba_delete_u32(&cba_root, old) != old)
					abort();
				free(container_of(old, struct key, node));
			}

			key = calloc(1, sizeof(*key));
			key->key = v;
			old = cba_insert_u32(&cba_root, &key->node);
			if (old != &key->node)
				abort();

			if (debug > 1) {
				static int round;
				char cmd[100];
				int len;

				len = snprintf(cmd, sizeof(cmd), "%s %d/%d : %p %d\n", orig_argv, round, round+count, old, v);
				dump(&cba_root, len < sizeof(cmd) ? cmd : orig_argv, 0);
				round++;
			}
		}
	} else if (test == 2) {
		while (count--) {
			v = rnd32() & mask;
			if (!count && debug > 2)
				dump(&cba_root, "step1", 0);
			old = cba_pick_u32(&cba_root, v);
			if (!count && debug > 2)
				dump(&cba_root, "step2", 0);
			back = old;
			while (old) {
				if (old && !count && debug > 2)
					dump(&cba_root, "step3", 0);
				old = cba_pick_u32(&cba_root, v);
				//if (old)
				//	printf("count=%d v=%u back=%p old=%p\n", count, v, back, old);
			}

			if (!count && debug > 2)
				dump(&cba_root, "step4", 0);

			//abort();
			//memset(old, 0, sizeof(*key));
			//if (old)
			//	free(container_of(old, struct key, node));

			key = calloc(1, sizeof(*key));
			key->key = v;
			old = cba_insert_u32(&cba_root, &key->node);
			if (old != &key->node)
				abort();

			if (!count && debug > 2)
				dump(&cba_root, "step5", 0);
			else if (debug > 1) {
				static int round;
				char cmd[100];
				int len;

				len = snprintf(cmd, sizeof(cmd), "%s %d/%d : %p %d\n", orig_argv, round, round+count, old, v);
				dump(&cba_root, len < sizeof(cmd) ? cmd : orig_argv, 0);
				round++;
			}
		}
	}

	if (debug == 1)
		dump(&cba_root, orig_argv, 0);
	return 0;
}
