/*
 * Compact Binary Trees - exported functions for operations on string keys
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
#include <string.h>
#include "cbatree.h"
#include "cbatree-prv.h"

// to be redone: compare the 3 strings as long as they match, then
// fall back to different labels depending on the non-matching one.
//static cmp3str(const unsigned char *k, const unsigned char *l, const unsigned char *r,
//	       int *ret_llen, int *ret_rlen, int *ret_xlen)
//{
//	unsigned char kl, kr, lr;
//
//	int beg = 0;
//	int llen = 0, rlen = 0, xlen = 0;
//
//	/* skip known and identical bits. We stop at the first different byte
//	 * or at the first zero we encounter on either side.
//	 */
//	while (1) {
//		unsigned char cl, cr, ck;
//		cl = *l;
//		cr = *r;
//		ck = *k;
//
//		kl = cl ^ ck;
//		kr = cr ^ ck;
//		lr = cl ^ cr;
//
//		c = a[beg];
//		d = b[beg];
//		beg++;
//
//		c ^= d;
//		if (c)
//			break;
//		if (!d)
//			return -1;
//	}
//	/* OK now we know that a and b differ at byte <beg>, or that both are zero.
//	 * We have to find what bit is differing and report it as the number of
//	 * identical bits. Note that low bit numbers are assigned to high positions
//	 * in the byte, as we compare them as strings.
//	 */
//	return (beg << 3) - flsnz8(c);
//
//}

/* Inserts node <node> into unique tree <tree> based on its key that
 * immediately follows the node. Returns the inserted node or the one
 * that already contains the same key.
 */
struct cba_node *cba_insert_st(struct cba_node **root, struct cba_node *node)
{
	const void *key = &container_of(node, struct cba_st, node)->key;

	return _cbau_insert(root, node, CB_KT_ST, key);
}

/* return the first node or NULL if not found. */
struct cba_node *cba_first_st(struct cba_node **root)
{
	return _cbau_first(root, CB_KT_ST);
}

/* return the last node or NULL if not found. */
struct cba_node *cba_last_st(struct cba_node **root)
{
	return _cbau_last(root, CB_KT_ST);
}

/* look up the specified key, and returns either the node containing it, or
 * NULL if not found.
 */
struct cba_node *cba_lookup_st(struct cba_node **root, const void *key)
{
	return _cbau_lookup(root, CB_KT_ST, key);
}

/* search for the next node after the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a left turn was made, and returning the first node along the right
 * branch at that fork.
 */
struct cba_node *cba_next_st(struct cba_node **root, struct cba_node *node)
{
	const void *key = &container_of(node, struct cba_st, node)->key;

	return _cbau_next(root, CB_KT_ST, key);
}

/* search for the prev node before the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a right turn was made, and returning the last node along the left
 * branch at that fork.
 */
struct cba_node *cba_prev_st(struct cba_node **root, struct cba_node *node)
{
	const void *key = &container_of(node, struct cba_st, node)->key;

	return _cbau_prev(root, CB_KT_ST, key);
}

/* look up the specified node with its key and deletes it if found, and in any
 * case, returns the node.
 */
struct cba_node *cba_delete_st(struct cba_node **root, struct cba_node *node)
{
	const void *key = &container_of(node, struct cba_st, node)->key;

	return _cbau_delete(root, node, CB_KT_ST, key);
}

/* look up the specified key, and detaches it and returns it if found, or NULL
 * if not found.
 */
struct cba_node *cba_pick_st(struct cba_node **root, const void *key)
{
	return _cbau_delete(root, NULL, CB_KT_ST, key);
}
