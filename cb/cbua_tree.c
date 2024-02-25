/*
 * Compact Binary Trees - exported functions for operations on addr keys
 *
 * Copyright (C) 2014-2024 Willy Tarreau - w@1wt.eu
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

/* Inserts node <node> into unique tree <tree> based on its own address
 * Returns the inserted node or the one that has the same address.
 */
struct cb_node *cbua_insert(struct cb_node **root, struct cb_node *node)
{
	return _cbu_insert(root, node, CB_KT_ADDR, 0, 0, node);
}

/* return the first node or NULL if not found. */
struct cb_node *cbua_first(struct cb_node **root)
{
	return _cbu_first(root, CB_KT_ADDR);
}

/* return the last node or NULL if not found. */
struct cb_node *cbua_last(struct cb_node **root)
{
	return _cbu_last(root, CB_KT_ADDR);
}

/* look up the specified key, and returns either the node containing it, or
 * NULL if not found.
 */
struct cb_node *cbua_lookup(struct cb_node **root, const void *key)
{
	return _cbu_lookup(root, CB_KT_ADDR, 0, 0, key);
}

/* look up the specified key or the highest below it, and returns either the
 * node containing it, or NULL if not found.
 */
struct cb_node *cbua_lookup_le(struct cb_node **root, const void *key)
{
	return _cbu_lookup_le(root, CB_KT_ADDR, 0, 0, key);
}

/* look up highest key below the specified one, and returns either the
 * node containing it, or NULL if not found.
 */
struct cb_node *cbua_lookup_lt(struct cb_node **root, const void *key)
{
	return _cbu_lookup_lt(root, CB_KT_ADDR, 0, 0, key);
}

/* look up the specified key or the smallest above it, and returns either the
 * node containing it, or NULL if not found.
 */
struct cb_node *cbua_lookup_ge(struct cb_node **root, const void *key)
{
	return _cbu_lookup_ge(root, CB_KT_ADDR, 0, 0, key);
}

/* look up the smallest key above the specified one, and returns either the
 * node containing it, or NULL if not found.
 */
struct cb_node *cbua_lookup_gt(struct cb_node **root, const void *key)
{
	return _cbu_lookup_gt(root, CB_KT_ADDR, 0, 0, key);
}

/* search for the next node after the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a left turn was made, and returning the first node along the right
 * branch at that fork.
 */
struct cb_node *cbua_next(struct cb_node **root, struct cb_node *node)
{
	return _cbu_next(root, CB_KT_ADDR, 0, 0, node);
}

/* search for the prev node before the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a right turn was made, and returning the last node along the left
 * branch at that fork.
 */
struct cb_node *cbua_prev(struct cb_node **root, struct cb_node *node)
{
	return _cbu_prev(root, CB_KT_ADDR, 0, 0, node);
}

/* look up the specified node with its key and deletes it if found, and in any
 * case, returns the node.
 */
struct cb_node *cbua_delete(struct cb_node **root, struct cb_node *node)
{
	return _cbu_delete(root, node, CB_KT_ADDR, 0, 0, node);
}

/* look up the specified key, and detaches it and returns it if found, or NULL
 * if not found.
 */
struct cb_node *cbua_pick(struct cb_node **root, const void *key)
{
	return _cbu_delete(root, NULL, CB_KT_ADDR, 0, 0, key);
}

/* dumps a cb_node tree using the default functions above. If a node matches
 * <ctx>, this one will be highlighted in red.
 */
void cbua_default_dump(struct cb_node **cb_root, const char *label, const void *ctx)
{
	printf("\ndigraph cbua_tree {\n"
	       "  fontname=\"fixed\";\n"
	       "  fontsize=8\n"
	       "  label=\"%s\"\n"
	       "", label);

	printf("  node [fontname=\"fixed\" fontsize=8 shape=\"box\" style=\"filled\" color=\"black\" fillcolor=\"white\"];\n"
	       "  edge [fontname=\"fixed\" fontsize=8 style=\"solid\" color=\"magenta\" dir=\"forward\"];\n");

	cbu_default_dump_tree(CB_KT_ADDR, cb_root, 0, NULL, 0, ctx, NULL, NULL, NULL);

	printf("}\n");
}
