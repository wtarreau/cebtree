/*
 * Compact Binary Trees - exported functions for operations on ulong keys
 *
 * Copyright (C) 2014-2023 Willy Tarreau - w@1wt.eu
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "cbtree.h"
#include "cbtree-prv.h"

/* Inserts node <node> into unique tree <tree> based on its key that
 * immediately follows the node. Returns the inserted node or the one
 * that already contains the same key.
 */
struct cb_node *cbul_insert(struct cb_node **root, struct cb_node *node)
{
	unsigned long key = container_of(node, struct cb_node_key, node)->key.ul;

	if (sizeof(long) <= 4)
		return _cbu_insert(root, node, CB_KT_U32, key, 0, NULL);
	else
		return _cbu_insert(root, node, CB_KT_U64, 0, key, NULL);
}

/* return the first node or NULL if not found. */
struct cb_node *cbul_first(struct cb_node **root)
{
	if (sizeof(long) <= 4)
		return _cbu_first(root, CB_KT_U32);
	else
		return _cbu_first(root, CB_KT_U64);
}

/* return the last node or NULL if not found. */
struct cb_node *cbul_last(struct cb_node **root)
{
	if (sizeof(long) <= 4)
		return _cbu_last(root, CB_KT_U32);
	else
		return _cbu_last(root, CB_KT_U64);
}

/* look up the specified key, and returns either the node containing it, or
 * NULL if not found.
 */
struct cb_node *cbul_lookup(struct cb_node **root, unsigned long key)
{
	if (sizeof(long) <= 4)
		return _cbu_lookup(root, CB_KT_U32, key, 0, NULL);
	else
		return _cbu_lookup(root, CB_KT_U64, 0, key, NULL);
}

/* look up the specified key or the highest below it, and returns either the
 * node containing it, or NULL if not found.
 */
struct cb_node *cbul_lookup_le(struct cb_node **root, unsigned long key)
{
	if (sizeof(long) <= 4)
		return _cbu_lookup_le(root, CB_KT_U32, key, 0, NULL);
	else
		return _cbu_lookup_le(root, CB_KT_U64, 0, key, NULL);
}

/* look up the specified key or the smallest above it, and returns either the
 * node containing it, or NULL if not found.
 */
struct cb_node *cbul_lookup_ge(struct cb_node **root, unsigned long key)
{
	if (sizeof(long) <= 4)
		return _cbu_lookup_ge(root, CB_KT_U32, key, 0, NULL);
	else
		return _cbu_lookup_ge(root, CB_KT_U64, 0, key, NULL);
}

/* search for the next node after the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a left turn was made, and returning the first node along the right
 * branch at that fork.
 */
struct cb_node *cbul_next(struct cb_node **root, struct cb_node *node)
{
	unsigned long key = container_of(node, struct cb_node_key, node)->key.ul;

	if (sizeof(long) <= 4)
		return _cbu_next(root, CB_KT_U32, key, 0, NULL);
	else
		return _cbu_next(root, CB_KT_U64, 0, key, NULL);
}

/* search for the prev node before the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a right turn was made, and returning the last node along the left
 * branch at that fork.
 */
struct cb_node *cbul_prev(struct cb_node **root, struct cb_node *node)
{
	unsigned long key = container_of(node, struct cb_node_key, node)->key.ul;

	if (sizeof(long) <= 4)
		return _cbu_prev(root, CB_KT_U32, key, 0, NULL);
	else
		return _cbu_prev(root, CB_KT_U64, 0, key, NULL);
}

/* look up the specified node with its key and deletes it if found, and in any
 * case, returns the node.
 */
struct cb_node *cbul_delete(struct cb_node **root, struct cb_node *node)
{
	unsigned long key = container_of(node, struct cb_node_key, node)->key.ul;

	if (sizeof(long) <= 4)
		return _cbu_delete(root, node, CB_KT_U32, 0, key, NULL);
	else
		return _cbu_delete(root, node, CB_KT_U64, 0, key, NULL);
}

/* look up the specified key, and detaches it and returns it if found, or NULL
 * if not found.
 */
struct cb_node *cbul_pick(struct cb_node **root, unsigned long key)
{
	if (sizeof(long) <= 4)
		return _cbu_delete(root, NULL, CB_KT_U32, key, 0, NULL);
	else
		return _cbu_delete(root, NULL, CB_KT_U64, 0, key, NULL);
}

/* default node dump function */
static void cbul_default_dump_node(struct cb_node *node, int level, const void *ctx)
{
	struct cb_node_key *key = container_of(node, struct cb_node_key, node);
	unsigned long pxor, lxor, rxor;

	/* xor of the keys of the two lower branches */
	pxor = container_of(__cb_clrtag(node->b[0]), struct cb_node_key, node)->key.ul ^
		container_of(__cb_clrtag(node->b[1]), struct cb_node_key, node)->key.ul;

	printf("  \"%lx_n\" [label=\"%lx\\nlev=%d bit=%d\\nkey=%lu\" fillcolor=\"lightskyblue1\"%s];\n",
	       (long)node, (long)node, level, flsnz(pxor) - 1, key->key.ul, (ctx == node) ? " color=red" : "");

	/* xor of the keys of the left branch's lower branches */
	lxor = container_of(__cb_clrtag(((struct cb_node*)__cb_clrtag(node->b[0]))->b[0]), struct cb_node_key, node)->key.ul ^
		container_of(__cb_clrtag(((struct cb_node*)__cb_clrtag(node->b[0]))->b[1]), struct cb_node_key, node)->key.ul;

	printf("  \"%lx_n\" -> \"%lx_%c\" [label=\"L\" arrowsize=0.66 %s];\n",
	       (long)node, (long)__cb_clrtag(node->b[0]),
	       (((long)node->b[0] & 1) || (lxor < pxor && ((struct cb_node*)node->b[0])->b[0] != ((struct cb_node*)node->b[0])->b[1])) ? 'n' : 'l',
	       (node == __cb_clrtag(node->b[0])) ? " dir=both" : "");

	/* xor of the keys of the right branch's lower branches */
	rxor = container_of(__cb_clrtag(((struct cb_node*)__cb_clrtag(node->b[1]))->b[0]), struct cb_node_key, node)->key.ul ^
		container_of(__cb_clrtag(((struct cb_node*)__cb_clrtag(node->b[1]))->b[1]), struct cb_node_key, node)->key.ul;

	printf("  \"%lx_n\" -> \"%lx_%c\" [label=\"R\" arrowsize=0.66 %s];\n",
	       (long)node, (long)__cb_clrtag(node->b[1]),
	       (((long)node->b[1] & 1) || (rxor < pxor && ((struct cb_node*)node->b[1])->b[0] != ((struct cb_node*)node->b[1])->b[1])) ? 'n' : 'l',
	       (node == __cb_clrtag(node->b[1])) ? " dir=both" : "");
}

/* default leaf dump function */
static void cbul_default_dump_leaf(struct cb_node *node, int level, const void *ctx)
{
	struct cb_node_key *key = container_of(node, struct cb_node_key, node);
	unsigned long pxor;

	/* xor of the keys of the two lower branches */
	pxor = container_of(__cb_clrtag(node->b[0]), struct cb_node_key, node)->key.ul ^
		container_of(__cb_clrtag(node->b[1]), struct cb_node_key, node)->key.ul;

	if (node->b[0] == node->b[1])
		printf("  \"%lx_l\" [label=\"%lx\\nlev=%d\\nkey=%lu\\n\" fillcolor=\"green\"%s];\n",
		       (long)node, (long)node, level, key->key.ul, (ctx == node) ? " color=red" : "");
	else
		printf("  \"%lx_l\" [label=\"%lx\\nlev=%d bit=%d\\nkey=%lu\\n\" fillcolor=\"yellow\"%s];\n",
		       (long)node, (long)node, level, flsnz(pxor) - 1, key->key.ul, (ctx == node) ? " color=red" : "");
}

/* Dumps a tree through the specified callbacks. */
void *cbul_dump_tree(struct cb_node *node, unsigned long pxor, void *last,
		      int level,
		      void (*node_dump)(struct cb_node *node, int level, const void *ctx),
		      void (*leaf_dump)(struct cb_node *node, int level, const void *ctx),
		      const void *ctx)
{
	unsigned long xor;

	if (!node) /* empty tree */
		return node;

	if (level < 0) {
		/* we're inside a dup tree. Tagged pointers indicate nodes,
		 * untagged ones leaves.
		 */
		level--;
		if (__cb_tagged(node->b[0])) {
		  last = cbul_dump_tree(__cb_untag(node->b[0]), 0, last, level, node_dump, leaf_dump, ctx);
			if (node_dump)
			  node_dump(__cb_untag(node->b[0]), level, ctx);
		} else if (leaf_dump)
			leaf_dump(node->b[0], level, ctx);

		if (__cb_tagged(node->b[1])) {
			last = cbul_dump_tree(__cb_untag(node->b[1]), 0, last, level, node_dump, leaf_dump, ctx);
			if (node_dump)
				node_dump(__cb_untag(node->b[1]), level, ctx);
		} else if (leaf_dump)
			leaf_dump(node->b[1], level, ctx);
		return node;
	}

	/* regular nodes, all branches are canonical */

	if (node->b[0] == node->b[1]) {
		/* first inserted leaf */
		if (leaf_dump)
			leaf_dump(node, level, ctx);
		return node;
	}

	if (0/*__cb_is_dup(node)*/) {
		if (node_dump)
			node_dump(node, -1, ctx);
		return cbul_dump_tree(node, 0, last, -1, node_dump, leaf_dump, ctx);
	}

	xor = ((struct cb_node_key*)node->b[0])->key.ul ^ ((struct cb_node_key*)node->b[1])->key.ul;
	if (pxor && xor >= pxor) {
		/* that's a leaf */
		if (leaf_dump)
			leaf_dump(node, level, ctx);
		return node;
	}

	if (!xor) {
		/* start of a dup */
		if (node_dump)
			node_dump(node, -1, ctx);
		return cbul_dump_tree(node, 0, last, -1, node_dump, leaf_dump, ctx);
	}

	/* that's a regular node */
	if (node_dump)
		node_dump(node, level, ctx);

	last = cbul_dump_tree(node->b[0], xor, last, level + 1, node_dump, leaf_dump, ctx);
	return cbul_dump_tree(node->b[1], xor, last, level + 1, node_dump, leaf_dump, ctx);
}

/* dumps a cb_node_key tree using the default functions above. If a node matches
 * <ctx>, this one will be highlighted in red.
 */
void cbul_default_dump(struct cb_node **cb_root, const char *label, const void *ctx)
{
	struct cb_node *node;

	printf("\ndigraph cbul_tree {\n"
	       "  fontname=\"fixed\";\n"
	       "  fontsize=8\n"
	       "  label=\"%s\"\n"
	       "", label);

	printf("  node [fontname=\"fixed\" fontsize=8 shape=\"box\" style=\"filled\" color=\"black\" fillcolor=\"white\"];\n"
	       "  edge [fontname=\"fixed\" fontsize=8 style=\"solid\" color=\"magenta\" dir=\"forward\"];\n"
	       "  \"%lx_n\" [label=\"root\\n%lx\"]\n", (long)cb_root, (long)cb_root);

	node = *cb_root;
	if (node) {
		/* under the root we've either a node or the first leaf */
		printf("  \"%lx_n\" -> \"%lx_%c\" [label=\"B\" arrowsize=0.66];\n",
		       (long)cb_root, (long)node,
		       (node->b[0] == node->b[1]) ? 'l' : 'n');
	}

	cbul_dump_tree(*cb_root, 0, NULL, 0, cbul_default_dump_node, cbul_default_dump_leaf, ctx);

	printf("}\n");
}
