/*
 * Compact Binary Trees - exported functions for operations on mb keys
 *
 * Copyright (C) 2014-2021 Willy Tarreau - w@1wt.eu
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
#include <string.h>
#include "cbtree.h"
#include "cbtree-prv.h"

/* Inserts node <node> into unique tree <tree> based on its key that
 * immediately follows the node and for <len> bytes. Returns the
 * inserted node or the one that already contains the same key.
 */
struct cb_node *cbub_insert(struct cb_node **root, struct cb_node *node, size_t len)
{
	const void *key = &container_of(node, struct cb_node_key, node)->key.mb;

	return _cbu_insert(root, node, CB_KT_MB, 0, len, key);
}

/* return the first node or NULL if not found. */
struct cb_node *cbub_first(struct cb_node **root)
{
	return _cbu_first(root, CB_KT_MB);
}

/* return the last node or NULL if not found. */
struct cb_node *cbub_last(struct cb_node **root)
{
	return _cbu_last(root, CB_KT_MB);
}

/* look up the specified key <key> of length <len>, and returns either the node
 * containing it, or NULL if not found.
 */
struct cb_node *cbub_lookup(struct cb_node **root, const void *key, size_t len)
{
	return _cbu_lookup(root, CB_KT_MB, 0, len, key);
}

/* search for the next node after the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a left turn was made, and returning the first node along the right
 * branch at that fork. The <len> field must correspond to the key length in
 * bytes.
 */
struct cb_node *cbub_next(struct cb_node **root, struct cb_node *node, size_t len)
{
	const void *key = &container_of(node, struct cb_node_key, node)->key.mb;

	return _cbu_next(root, CB_KT_MB, 0, len, key);
}

/* search for the prev node before the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a right turn was made, and returning the last node along the left
 * branch at that fork. The <len> field must correspond to the key length in
 * bytes.
 */
struct cb_node *cbub_prev(struct cb_node **root, struct cb_node *node, size_t len)
{
	const void *key = &container_of(node, struct cb_node_key, node)->key.mb;

	return _cbu_prev(root, CB_KT_MB, 0, len, key);
}

/* look up the specified node with its key and deletes it if found, and in any
 * case, returns the node. The <len> field must correspond to the key length in
 * bytes.
 */
struct cb_node *cbub_delete(struct cb_node **root, struct cb_node *node, size_t len)
{
	const void *key = &container_of(node, struct cb_node_key, node)->key.mb;

	return _cbu_delete(root, node, CB_KT_MB, 0, len, key);
}

/* look up the specified key, and detaches it and returns it if found, or NULL
 * if not found. The <len> field must correspond to the key length in bytes.
 */
struct cb_node *cbub_pick(struct cb_node **root, const void *key, size_t len)
{
	return _cbu_delete(root, NULL, CB_KT_MB, 0, len, key);
}
