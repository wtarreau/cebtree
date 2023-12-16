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

struct cba_node *cba_insert_st(struct cba_node **root, struct cba_node *node)
{
	const typeof(((struct cba_st*)0)->key) *key = &container_of(node, struct cba_st, node)->key;
	struct cba_node **parent;
	struct cba_node *ret;
	int nside;

	if (!*root) {
		/* empty tree, insert a leaf only */
		node->b[0] = node->b[1] = node;
		*root = node;
		return node;
	}

	ret = _cbau_descend(root, CB_WM_KEY, node, CB_KT_ST, key, &nside, &parent, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	if (ret == node) {
		node->b[nside] = node;
		node->b[!nside] = *parent;
		*parent = ret;
	}
	return ret;
}

/* return the first node or NULL if not found. */
struct cba_node *cba_first_st(struct cba_node **root)
{
	if (!*root)
		return NULL;

	return _cbau_descend(root, CB_WM_FST, NULL, CB_KT_ST, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* return the last node or NULL if not found. */
struct cba_node *cba_last_st(struct cba_node **root)
{
	if (!*root)
		return NULL;

	return _cbau_descend(root, CB_WM_LST, NULL, CB_KT_ST, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* look up the specified key, and returns either the node containing it, or
 * NULL if not found.
 */
struct cba_node *cba_lookup_st(struct cba_node **root, const void *key)
{
	if (!*root)
		return NULL;

	return _cbau_descend(root, CB_WM_KEY, NULL, CB_KT_ST, key, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* search for the next node after the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a left turn was made, and returning the first node along the right
 * branch at that fork.
 */
struct cba_node *cba_next_st(struct cba_node **root, struct cba_node *node)
{
	const typeof(((struct cba_st*)0)->key) *key = &container_of(node, struct cba_st, node)->key;
	struct cba_node **right_branch = NULL;

	if (!*root)
		return NULL;

	_cbau_descend(root, CB_WM_KEY, NULL, CB_KT_ST, key, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &right_branch);
	if (!right_branch)
		return NULL;
	return _cbau_descend(right_branch, CB_WM_NXT, NULL, CB_KT_ST, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* search for the prev node before the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a right turn was made, and returning the last node along the left
 * branch at that fork.
 */
struct cba_node *cba_prev_st(struct cba_node **root, struct cba_node *node)
{
	const typeof(((struct cba_st*)0)->key) *key = &container_of(node, struct cba_st, node)->key;
	struct cba_node **left_branch = NULL;

	if (!*root)
		return NULL;

	_cbau_descend(root, CB_WM_KEY, NULL, CB_KT_ST, key, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &left_branch, NULL);
	if (!left_branch)
		return NULL;
	return _cbau_descend(left_branch, CB_WM_PRV, NULL, CB_KT_ST, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* look up the specified node with its key and deletes it if found, and in any
 * case, returns the node.
 */
struct cba_node *cba_delete_st(struct cba_node **root, struct cba_node *node)
{
	const typeof(((struct cba_st*)0)->key) *key = &container_of(node, struct cba_st, node)->key;
	struct cba_node *lparent, *nparent, *gparent, *sibling;
	int lpside, npside, gpside;
	struct cba_node *ret;

	if (!node->b[0]) {
		/* NULL on a branch means the node is not in the tree */
		return node;
	}

	if (!*root) {
		/* empty tree, the node cannot be there */
		return node;
	}

	ret = _cbau_descend(root, CB_WM_KEY, NULL, CB_KT_ST, key, NULL, NULL, &lparent, &lpside, &nparent, &npside, &gparent, &gpside, NULL, NULL);
	if (ret == node) {
		//CBADBG("root=%p ret=%p l=%p[%d] n=%p[%d] g=%p[%d]\n", root, ret, lparent, lpside, nparent, npside, gparent, gpside);

		if (&lparent->b[0] == root) {
			/* there was a single entry, this one */
			*root = NULL;
			goto done;
		}
		//printf("g=%p\n", gparent);

		/* then we necessarily have a gparent */
		sibling = lpside ? lparent->b[0] : lparent->b[1];
		gparent->b[gpside] = sibling;

		if (lparent == node) {
			/* we're removing the leaf and node together, nothing
			 * more to do.
			 */
			goto done;
		}

		if (node->b[0] == node->b[1]) {
			/* we're removing the node-less item, the parent will
			 * take this role.
			 */
			lparent->b[0] = lparent->b[1] = lparent;
			goto done;
		}

		/* more complicated, the node was split from the leaf, we have
		 * to find a spare one to switch it. The parent node is not
		 * needed anymore so we can reuse it.
		 */
		lparent->b[0] = node->b[0];
		lparent->b[1] = node->b[1];
		nparent->b[npside] = lparent;
	}
done:
	return ret;
}

/* look up the specified key, and detaches it and returns it if found, or NULL
 * if not found.
 */
struct cba_node *cba_pick_st(struct cba_node **root, const void *key)
{
	struct cba_node *lparent, *nparent, *gparent/*, *sibling*/;
	int lpside, npside, gpside;
	struct cba_node *ret;

	if (!*root)
		return NULL;

	//if (key == 425144) printf("%d: k=%u\n", __LINE__, key);

	ret = _cbau_descend(root, CB_WM_KEY, NULL, CB_KT_ST, key, NULL, NULL, &lparent, &lpside, &nparent, &npside, &gparent, &gpside, NULL, NULL);

	//if (key == 425144) printf("%d: k=%u ret=%p\n", __LINE__, key, ret);

	if (ret) {
		struct cba_st *p = container_of(ret, struct cba_st, node);

		if (p->key != key)
			abort();

		//CBADBG("root=%p ret=%p l=%p[%d] n=%p[%d] g=%p[%d]\n", root, ret, lparent, lpside, nparent, npside, gparent, gpside);

		if (&lparent->b[0] == root) {
			/* there was a single entry, this one */
			*root = NULL;
			//if (key == 425144) printf("%d: k=%u ret=%p\n", __LINE__, key, ret);
			goto done;
		}
		//printf("g=%p\n", gparent);

		/* then we necessarily have a gparent */
		//sibling = lpside ? lparent->b[0] : lparent->b[1];
		//gparent->b[gpside] = sibling;

		gparent->b[gpside] = lparent->b[!lpside];

		if (lparent == ret) {
			/* we're removing the leaf and node together, nothing
			 * more to do.
			 */
			//if (key == 425144) printf("%d: k=%u ret=%p\n", __LINE__, key, ret);
			goto done;
		}

		if (ret->b[0] == ret->b[1]) {
			/* we're removing the node-less item, the parent will
			 * take this role.
			 */
			lparent->b[0] = lparent->b[1] = lparent;
			//if (key == 425144) printf("%d: k=%u ret=%p\n", __LINE__, key, ret);
			goto done;
		}

		/* more complicated, the node was split from the leaf, we have
		 * to find a spare one to switch it. The parent node is not
		 * needed anymore so we can reuse it.
		 */
		//if (key == 425144) printf("%d: k=%u ret=%p lp=%p np=%p gp=%p\n", __LINE__, key, ret, lparent, nparent, gparent);
		lparent->b[0] = ret->b[0];
		lparent->b[1] = ret->b[1];
		nparent->b[npside] = lparent;
	}
done:
	//if (key == 425144) printf("%d: k=%u ret=%p\n", __LINE__, key, ret);
	return ret;
}
