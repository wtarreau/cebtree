/*
 * Compact Binary Trees - exported functions for operations on u32 keys
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
#include "cbatree.h"
#include "cbatree-prv.h"

/* Inserts node <node> into unique tree <tree> based on its key that
 * immediately follows the node. Returns the inserted node or the one
 * that already contains the same key.
 */
struct cb_node *cbu32_insert(struct cb_node **root, struct cb_node *node)
{
	uint32_t key = container_of(node, struct cb_node_key, node)->key.u32;

	return _cbu_insert(root, node, CB_KT_U32, key, 0, NULL);
}

/* return the first node or NULL if not found. */
struct cb_node *cbu32_first(struct cb_node **root)
{
	return _cbu_first(root, CB_KT_U32);
}

/* return the last node or NULL if not found. */
struct cb_node *cbu32_last(struct cb_node **root)
{
	return _cbu_last(root, CB_KT_U32);
}

/* look up the specified key, and returns either the node containing it, or
 * NULL if not found.
 */
struct cb_node *cbu32_lookup(struct cb_node **root, uint32_t key)
{
	return _cbu_lookup(root, CB_KT_U32, key, 0, NULL);
}

/* search for the next node after the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a left turn was made, and returning the first node along the right
 * branch at that fork.
 */
struct cb_node *cbu32_next(struct cb_node **root, struct cb_node *node)
{
	uint32_t key = container_of(node, struct cb_node_key, node)->key.u32;

	return _cbu_next(root, CB_KT_U32, key, 0, NULL);
}

/* search for the prev node before the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a right turn was made, and returning the last node along the left
 * branch at that fork.
 */
struct cb_node *cbu32_prev(struct cb_node **root, struct cb_node *node)
{
	uint32_t key = container_of(node, struct cb_node_key, node)->key.u32;

	return _cbu_prev(root, CB_KT_U32, key, 0, NULL);
}

/* look up the specified node with its key and deletes it if found, and in any
 * case, returns the node.
 */
struct cb_node *cbu32_delete(struct cb_node **root, struct cb_node *node)
{
	uint32_t key = container_of(node, struct cb_node_key, node)->key.u32;

	return _cbu_delete(root, node, CB_KT_U32, key, 0, NULL);
}

/* look up the specified key, and detaches it and returns it if found, or NULL
 * if not found.
 */
struct cb_node *cbu32_pick(struct cb_node **root, uint32_t key)
{
	return _cbu_delete(root, NULL, CB_KT_U32, key, 0, NULL);
}

///* returns the highest node which is less than or equal to data. This is
// * typically used to know what memory area <data> belongs to.
// */
//struct cb_node *cba_lookup_le(struct cb_node **root, void *data)
//{
//	struct cb_node *p, *last_r;
//	u32 pxor;
//
//	pxor = 0;
//	p = *root;
//
//	if (!p)
//		return p;
//
//	last_r = NULL;
//	while (1) {
//		if (!p->l || (xorptr(p->l, p->r) >= pxor && pxor != 0)) {
//			/* first leaf inserted, or regular leaf. Either
//			 * the entry fits our expectations or we have to
//			 * roll back and go down the opposite direction.
//			 */
//			if ((u32)p > (u32)data)
//				break;
//			return p;
//		}
//
//		pxor = xorptr(p->l, p->r);
//		if (xorptr(data, p->l) > pxor && xorptr(data, p->r) > pxor) {
//			/* The difference between the looked up key and the branches
//			 * is higher than the difference between the branches, which
//			 * means that the key ought to have been found upper in the
//			 * chain. Since we won't find the key below, either we have
//			 * a chance to find the largest inferior one below and we
//			 * walk down, or we need to rewind.
//			 */
//			if ((u32)p->l > (u32)data)
//				break;
//
//			p = p->r;
//			goto walkdown;
//		}
//
//		if (xorptr(data, p->l) < xorptr(data, p->r)) {
//			root = &p->l;
//		}
//		else {
//			last_r = p;
//			root = &p->r;
//		}
//
//		p = *root;
//	}
//
//	/* first roll back to last node where we turned right, and go down left
//	 * or stop at first leaf if any. If we only went down left, we already
//	 * found the smallest key so none will suit us.
//	 */
//	if (!last_r)
//		return NULL;
//
//	pxor = xorptr(last_r->l, last_r->r);
//	p = last_r->l;
//
// walkdown:
//	/* switch to the other branch last time we turned right */
//	while (p->r) {
//		if (xorptr(p->l, p->r) >= pxor)
//			break;
//		pxor = xorptr(p->l, p->r);
//		p = p->r;
//	}
//
//	if ((u32)p > (u32)data)
//		return NULL;
//	return p;
//}
//
///* returns the note which equals <data> or NULL if <data> is not in the tree */
//struct cb_node *cba_lookup(struct cb_node **root, void *data)
//{
//	struct cb_node *p;
//	u32 pxor;
//
//	pxor = 0;
//	p = *root;
//
//	if (!p)
//		return p;
//
//	while (1) {
//		if (!p->l || (xorptr(p->l, p->r) >= pxor && pxor != 0)) {
//			/* first leaf inserted, or regular leaf */
//			if ((u32)p != (u32)data)
//				p = NULL;
//			break;
//		}
//
//		pxor = xorptr(p->l, p->r);
//		if (xorptr(data, p->l) > pxor && xorptr(data, p->r) > pxor) {
//			/* The difference between the looked up key and the branches
//			 * is higher than the difference between the branches, which
//			 * means that the key ought to have been found upper in the
//			 * chain.
//			 */
//			p = NULL;
//			break;
//		}
//
//		if (xorptr(data, p->l) < xorptr(data, p->r))
//			root = &p->l;
//		else
//			root = &p->r;
//
//		p = *root;
//	}
//	return p;
//}
//
///* returns the lowest node which is greater than or equal to data. This is
// * typically used to know the distance between <data> and the next memory
// * area.
// */
//struct cb_node *cba_lookup_ge(struct cb_node **root, void *data)
//{
//	struct cb_node *p, *last_l;
//	u32 pxor;
//
//	pxor = 0;
//	p = *root;
//
//	if (!p)
//		return p;
//
//	last_l = NULL;
//	while (1) {
//		if (!p->l || (xorptr(p->l, p->r) >= pxor && pxor != 0)) {
//			/* first leaf inserted, or regular leaf. Either
//			 * the entry fits our expectations or we have to
//			 * roll back and go down the opposite direction.
//			 */
//			if ((u32)p < (u32)data)
//				break;
//			return p;
//		}
//
//		pxor = xorptr(p->l, p->r);
//		if (xorptr(data, p->l) > pxor && xorptr(data, p->r) > pxor) {
//			/* The difference between the looked up key and the branches
//			 * is higher than the difference between the branches, which
//			 * means that the key ought to have been found upper in the
//			 * chain. Since we won't find the key below, either we have
//			 * a chance to find the smallest superior one below and we
//			 * walk down, or we need to rewind.
//			 */
//			if ((u32)p->l < (u32)data)
//				break;
//
//			p = p->l;
//			goto walkdown;
//		}
//
//		if (xorptr(data, p->l) < xorptr(data, p->r)) {
//			last_l = p;
//			root = &p->l;
//		}
//		else {
//			root = &p->r;
//		}
//
//		p = *root;
//	}
//
//	/* first roll back to last node where we turned right, and go down left
//	 * or stop at first leaf if any. If we only went down left, we already
//	 * found the smallest key so none will suit us.
//	 */
//	if (!last_l)
//		return NULL;
//
//	pxor = xorptr(last_l->l, last_l->r);
//	p = last_l->r;
//
// walkdown:
//	/* switch to the other branch last time we turned left */
//	while (p->l) {
//		if (xorptr(p->l, p->r) >= pxor)
//			break;
//		pxor = xorptr(p->l, p->r);
//		p = p->l;
//	}
//
//	if ((u32)p < (u32)data)
//		return NULL;
//	return p;
//}

/* default node dump function */
static void cbu32_default_dump_node(struct cb_node *node, int level, const void *ctx)
{
	struct cb_node_key *key = container_of(node, struct cb_node_key, node);
	u32 pxor, lxor, rxor;

	/* xor of the keys of the two lower branches */
	pxor = container_of(__cba_clrtag(node->b[0]), struct cb_node_key, node)->key.u32 ^
		container_of(__cba_clrtag(node->b[1]), struct cb_node_key, node)->key.u32;

	printf("  \"%lx_n\" [label=\"%lx\\nlev=%d bit=%d\\nkey=%u\" fillcolor=\"lightskyblue1\"%s];\n",
	       (long)node, (long)node, level, flsnz(pxor) - 1, key->key.u32, (ctx == node) ? " color=red" : "");

	/* xor of the keys of the left branch's lower branches */
	lxor = container_of(__cba_clrtag(((struct cb_node*)__cba_clrtag(node->b[0]))->b[0]), struct cb_node_key, node)->key.u32 ^
		container_of(__cba_clrtag(((struct cb_node*)__cba_clrtag(node->b[0]))->b[1]), struct cb_node_key, node)->key.u32;

	printf("  \"%lx_n\" -> \"%lx_%c\" [label=\"L\" arrowsize=0.66 %s];\n",
	       (long)node, (long)__cba_clrtag(node->b[0]),
	       (((long)node->b[0] & 1) || (lxor < pxor && ((struct cb_node*)node->b[0])->b[0] != ((struct cb_node*)node->b[0])->b[1])) ? 'n' : 'l',
	       (node == __cba_clrtag(node->b[0])) ? " dir=both" : "");

	/* xor of the keys of the right branch's lower branches */
	rxor = container_of(__cba_clrtag(((struct cb_node*)__cba_clrtag(node->b[1]))->b[0]), struct cb_node_key, node)->key.u32 ^
		container_of(__cba_clrtag(((struct cb_node*)__cba_clrtag(node->b[1]))->b[1]), struct cb_node_key, node)->key.u32;

	printf("  \"%lx_n\" -> \"%lx_%c\" [label=\"R\" arrowsize=0.66 %s];\n",
	       (long)node, (long)__cba_clrtag(node->b[1]),
	       (((long)node->b[1] & 1) || (rxor < pxor && ((struct cb_node*)node->b[1])->b[0] != ((struct cb_node*)node->b[1])->b[1])) ? 'n' : 'l',
	       (node == __cba_clrtag(node->b[1])) ? " dir=both" : "");
}

/* default leaf dump function */
static void cbu32_default_dump_leaf(struct cb_node *node, int level, const void *ctx)
{
	struct cb_node_key *key = container_of(node, struct cb_node_key, node);
	u32 pxor;

	/* xor of the keys of the two lower branches */
	pxor = container_of(__cba_clrtag(node->b[0]), struct cb_node_key, node)->key.u32 ^
		container_of(__cba_clrtag(node->b[1]), struct cb_node_key, node)->key.u32;

	if (node->b[0] == node->b[1])
		printf("  \"%lx_l\" [label=\"%lx\\nlev=%d\\nkey=%u\\n\" fillcolor=\"green\"%s];\n",
		       (long)node, (long)node, level, key->key.u32, (ctx == node) ? " color=red" : "");
	else
		printf("  \"%lx_l\" [label=\"%lx\\nlev=%d bit=%d\\nkey=%u\\n\" fillcolor=\"yellow\"%s];\n",
		       (long)node, (long)node, level, flsnz(pxor) - 1, key->key.u32, (ctx == node) ? " color=red" : "");
}

/* Dumps a tree through the specified callbacks. */
void *cbu32_dump_tree(struct cb_node *node, u32 pxor, void *last,
		      int level,
		      void (*node_dump)(struct cb_node *node, int level, const void *ctx),
		      void (*leaf_dump)(struct cb_node *node, int level, const void *ctx),
		      const void *ctx)
{
	u32 xor;

	if (!node) /* empty tree */
		return node;

	//fprintf(stderr, "node=%p level=%d key=%u l=%p r=%p\n", node, level, *(unsigned *)((char*)(node)+16), node->b[0], node->b[1]);

	if (level < 0) {
		/* we're inside a dup tree. Tagged pointers indicate nodes,
		 * untagged ones leaves.
		 */
		level--;
		if (__cba_tagged(node->b[0])) {
		  last = cbu32_dump_tree(__cba_untag(node->b[0]), 0, last, level, node_dump, leaf_dump, ctx);
			if (node_dump)
			  node_dump(__cba_untag(node->b[0]), level, ctx);
		} else if (leaf_dump)
			leaf_dump(node->b[0], level, ctx);

		if (__cba_tagged(node->b[1])) {
			last = cbu32_dump_tree(__cba_untag(node->b[1]), 0, last, level, node_dump, leaf_dump, ctx);
			if (node_dump)
				node_dump(__cba_untag(node->b[1]), level, ctx);
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

	if (0/*__cba_is_dup(node)*/) {
		if (node_dump)
			node_dump(node, -1, ctx);
		return cbu32_dump_tree(node, 0, last, -1, node_dump, leaf_dump, ctx);
	}

	xor = ((struct cb_node_key*)node->b[0])->key.u32 ^ ((struct cb_node_key*)node->b[1])->key.u32;
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
		return cbu32_dump_tree(node, 0, last, -1, node_dump, leaf_dump, ctx);
	}

	/* that's a regular node */
	if (node_dump)
		node_dump(node, level, ctx);

	last = cbu32_dump_tree(node->b[0], xor, last, level + 1, node_dump, leaf_dump, ctx);
	return cbu32_dump_tree(node->b[1], xor, last, level + 1, node_dump, leaf_dump, ctx);
}

/* dumps a cb_node_key tree using the default functions above. If a node matches
 * <ctx>, this one will be highlighted in red.
 */
void cbu32_default_dump(struct cb_node **cba_root, const char *label, const void *ctx)
{
	struct cb_node *node;

	printf("\ndigraph cbu32_tree {\n"
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
		printf("  \"%lx_n\" -> \"%lx_%c\" [label=\"B\" arrowsize=0.66];\n",
		       (long)cba_root, (long)node,
		       (node->b[0] == node->b[1]) ? 'l' : 'n');
	}

	cbu32_dump_tree(*cba_root, 0, NULL, 0, cbu32_default_dump_node, cbu32_default_dump_leaf, ctx);

	printf("}\n");
}
