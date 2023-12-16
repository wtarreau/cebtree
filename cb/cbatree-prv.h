/*
 * Compact Binary Trees - internal functions and types
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

/* This file MUST NOT be included by public code, it contains macros, enums
 * with short names and function definitions that may clash with user code.
 * It may only be included by the respective types' C files.
 */

#ifndef _CBATREE_PRV_H
#define _CBATREE_PRV_H


/* If DEBUG is set, we'll print additional debugging info during the descent */
#ifdef DEBUG
#define CBADBG(x, ...) fprintf(stderr, x, ##__VA_ARGS__)
#else
#define CBADBG(x, ...) do { } while (0)
#endif


/* tree walk method: key, left, right */
enum cba_walk_meth {
	CB_WM_KEY,     /* look up the node's key */
	CB_WM_FST,     /* look up "first" (walk left only) */
	CB_WM_NXT,     /* look up "next" (walk right once then left) */
	CB_WM_PRV,     /* look up "prev" (walk left once then right) */
	CB_WM_LST,     /* look up "last" (walk right only) */
};

enum cba_key_type {
	CB_KT_NONE,    /* no key */
	CB_KT_ST,      /* NUL-terminated string in key_ptr */
};

/* this structure is aliased to the common cba node during st operations */
struct cba_st {
	struct cba_node node;
	unsigned char key[0];
};

/* Generic tree descent function. It must absolutely be inlined so that the
 * compiler can eliminate the tests related to the various return pointers,
 * which must either point to a local variable in the caller, or be NULL.
 * It must not be called with an empty tree, it's the caller business to
 * deal with this special case. It returns in ret_root the location of the
 * pointer to the leaf (i.e. where we have to insert ourselves). The integer
 * pointed to by ret_nside will contain the side the leaf should occupy at
 * its own node, with the sibling being *ret_root. The node is only needed for
 * inserts.
 */
static inline __attribute__((always_inline))
struct cba_node *_cbau_descend(struct cba_node **root,
			       enum cba_walk_meth meth,
			       struct cba_node *node,
			       enum cba_key_type key_type,
			       const void *key_ptr,
			       int *ret_nside,
			       struct cba_node ***ret_root,
			       struct cba_node **ret_lparent,
			       int *ret_lpside,
			       struct cba_node **ret_nparent,
			       int *ret_npside,
			       struct cba_node **ret_gparent,
			       int *ret_gpside,
			       struct cba_node ***ret_alt_l,
			       struct cba_node ***ret_alt_r)
{
	struct cba_st *p, *l, *r;
	struct cba_node *gparent = NULL;
	struct cba_node *nparent = NULL;
	struct cba_node **alt_l = NULL;
	struct cba_node **alt_r = NULL;
	struct cba_node *lparent;
	int gpside = 0;   // side on the grand parent
	int npside = 0;   // side on the node's parent
	long lpside = 0;  // side on the leaf's parent
	long brside = 0;  // branch side when descending
	int llen = 0;     // left vs key matching length
	int rlen = 0;     // right vs key matching length
	int xlen = 0;     // left vs right matching length
	int plen = 0;     // previous xlen
	int found = 0;    // key was found

	switch (key_type) {
	case CB_KT_ST:
		CBADBG(">>> [%04d] meth=%d plen=%d key=str('%s')\n", __LINE__, meth, plen, (meth == CB_WM_KEY) ? (const char*)key_ptr : "");
		break;
	default:
		CBADBG(">>> [%04d] meth=%d plen=%d\n", __LINE__, meth, plen);
		break;
	}

	/* the parent will be the (possibly virtual) node so that
	 * &lparent->l == root.
	 */
	lparent = container_of(root, struct cba_node, b[0]);
	gparent = nparent = lparent;

	/* for key-less descents we need to set the initial branch to take */
	switch (meth) {
	case CB_WM_NXT:
	case CB_WM_LST:
		brside = 1; // start right for next/last
		break;
	case CB_WM_FST:
	case CB_WM_PRV:
	default:
		brside = 0; // start left for first/prev
		break;
	}

	/* the previous xor is initialized to the largest possible inter-branch
	 * value so that it can never match on the first test as we want to use
	 * it to detect a leaf vs node.
	 */
	while (1) {
		p = container_of(*root, struct cba_st, node);

		switch (key_type) {
		case CB_KT_ST:
			CBADBG("    [%04d] meth=%d plen=%d llen=%d rlen=%d xlen=%d p=%p pkey=str('%s') key=str('%s')\n", __LINE__, meth, plen, llen, rlen, xlen, p, (const char*)p->key, (meth == CB_WM_KEY) ? (const char*)key_ptr : "");
			break;
		default:
			CBADBG("    [%04d] meth=%d plen=%d llen=%d rlen=%d xlen=%d p=%p\n", __LINE__, meth, plen, llen, rlen, xlen, p);
			break;
		}

		/* let's prefetch the lower nodes for the keys */
		__builtin_prefetch(p->node.b[0], 0);
		__builtin_prefetch(p->node.b[1], 0);

		/* neither pointer is tagged */
		l = container_of(p->node.b[0], struct cba_st, node);
		r = container_of(p->node.b[1], struct cba_st, node);

		/* two equal pointers identifies the nodeless leaf. */
		if (l == r) {
			switch (key_type) {
			case CB_KT_ST:
				CBADBG(" 1! [%04d] meth=%d plen=%d llen=%d rlen=%d xlen=%d p=%p pkey=str('%s') key=str('%s')\n", __LINE__, meth, plen, llen, rlen, xlen, p, (const char*)p->key, (meth == CB_WM_KEY) ? (const char*)key_ptr : "");
				break;
			default:
				CBADBG(" 1! [%04d] meth=%d plen=%d llen=%d rlen=%d xlen=%d p=%p\n", __LINE__, meth, plen, llen, rlen, xlen, p);
				break;
			}
			break;
		}

		/* we can compute this here for scalar types, it allows the
		 * CPU to predict next branches. We can also xor lkey/rkey
		 * with key and use it everywhere later but it doesn't save
		 * much. Alternately, like here, it's possible to measure
		 * the length of identical bits. This is the solution that
		 * will be needed on strings. Note that a negative length
		 * indicates an equal value with the final zero reached, but it
		 * is still needed to descend to find the leaf. We take that
		 * negative length for an infinite one, hence the uint cast.
		 */

		/* next branch is calculated here when having a key */
		if (meth == CB_WM_KEY) {
			if (key_type == CB_KT_ST) {
				llen = string_equal_bits(key_ptr, l->key, 0);
				rlen = string_equal_bits(key_ptr, r->key, 0);
				brside = (unsigned)llen <= (unsigned)rlen;
				if (llen < 0 || rlen < 0)
					found = 1;
			}
		}

		/* so that's either a node or a leaf. Each leaf we visit had
		 * its node part already visited. The only way to distinguish
		 * them is that the inter-branch xor of the leaf will be the
		 * node's one, and will necessarily be larger than the previous
		 * node's xor if the node is above (we've already checked for
		 * direct descendent below). Said differently, if an inter-
		 * branch xor is strictly larger than the previous one, it
		 * necessarily is the one of an upper node, so what we're
		 * seeing cannot be the node, hence it's the leaf. The case
		 * where they're equal was already dealt with by the test at
		 * the end of the loop (node points to self).
		 */
		switch (key_type) {
		case CB_KT_ST:
			xlen = string_equal_bits(l->key, r->key, 0);
			break;
		default:
			break;
		}

		if (xlen < plen) { // triggered using 2 4 6 4
			/* this is a leaf */
			switch (key_type) {
			case CB_KT_ST:
				CBADBG(" L! [%04d] meth=%d plen=%d llen=%d rlen=%d xlen=%d p=%p pkey=str('%s') key=str('%s')\n", __LINE__, meth, plen, llen, rlen, xlen, p, (const char*)p->key, (meth == CB_WM_KEY) ? (const char*)key_ptr : "");
				break;
			default:
				CBADBG(" L! [%04d] meth=%d plen=%d llen=%d rlen=%d xlen=%d p=%p\n", __LINE__, meth, plen, llen, rlen, xlen, p);
				break;
			}
			break;
		}

		if (meth == CB_WM_KEY) {
			/* check the split bit */
			if ((unsigned)llen < (unsigned)xlen && (unsigned)rlen < (unsigned)xlen) {
				/* can't go lower, the node must be inserted above p
				 * (which is then necessarily a node). We also know
				 * that (key != p->key) because p->key differs from at
				 * least one of its subkeys by a higher bit than the
				 * split bit, so lookups must fail here.
				 */
				switch (key_type) {
				case CB_KT_ST:
					CBADBG(" B! [%04d] meth=%d plen=%d llen=%d rlen=%d xlen=%d p=%p pkey=str('%s') key=str('%s')\n", __LINE__, meth, plen, llen, rlen, xlen, p, (const char*)p->key, (const char*)key_ptr);
					break;
				default:
					CBADBG(" B! [%04d] meth=%d plen=%d llen=%d rlen=%d xlen=%d p=%p\n", __LINE__, meth, plen, llen, rlen, xlen, p);
					break;
				}
				break;
			}

			/* here we're guaranteed to be above a node. If this is the
			 * same node as the one we're looking for, let's store the
			 * parent as the node's parent.
			 */
			if (ret_npside || ret_nparent) {
				int mlen = llen > rlen ? llen : rlen;

				if (mlen > xlen)
					mlen = xlen;

				if (key_type == CB_KT_ST &&
				    strcmp(key_ptr + mlen / 8, (const void *)p->key + mlen / 8) == 0) {
					/* strcmp() still needed. E.g. 1 2 3 4 10 11 4 3 2 1 10 11 fails otherwise */
					switch (key_type) {
					case CB_KT_ST:
						CBADBG(" F! [%04d] meth=%d plen=%d llen=%d rlen=%d xlen=%d p=%p pkey=str('%s') key=str('%s')\n", __LINE__, meth, plen, llen, rlen, xlen, p, (const char*)p->key, (const char*)key_ptr);
						break;
					default:
						CBADBG(" F! [%04d] meth=%d plen=%d llen=%d rlen=%d xlen=%d p=%p\n", __LINE__, meth, plen, llen, rlen, xlen, p);
						break;
					}

					nparent = lparent;
					npside  = lpside;
					/* we've found a match, so we know the node is there but
					 * we still need to walk down to spot all parents.
					 */
					found = 1;
				}
			}
		}

		/* shift all copies by one */
		gparent = lparent;
		gpside = lpside;
		lparent = &p->node;
		lpside = brside;
		if (brside) {
			if (ret_alt_l)
				alt_l = root;
			root = &p->node.b[1];

			/* change branch for key-less walks */
			if (meth == CB_WM_NXT)
				brside = 0;

			switch (key_type) {
			case CB_KT_ST:
				CBADBG(" -> [%04d] meth=%d plen=%d lft=%p,str('%s') rgt=%p,str('%s')\n", __LINE__, meth, plen,
				       p->node.b[0], container_of(p->node.b[0], struct cba_st, node)->key,
				       p->node.b[1], container_of(p->node.b[1], struct cba_st, node)->key);
				break;
			default:
				CBADBG(" -> [%04d] meth=%d plen=%d lft=%p rgt=%p\n", __LINE__, meth, plen, p->node.b[0], p->node.b[1]);
				break;
			}
		}
		else {
			if (ret_alt_r)
				alt_r = root;
			root = &p->node.b[0];

			/* change branch for key-less walks */
			if (meth == CB_WM_PRV)
				brside = 1;

			switch (key_type) {
			case CB_KT_ST:
				CBADBG(" <- [%04d] meth=%d plen=%d lft=%p,str('%s') rgt=%p,str('%s')\n", __LINE__, meth, plen,
				       p->node.b[0], container_of(p->node.b[0], struct cba_st, node)->key,
				       p->node.b[1], container_of(p->node.b[1], struct cba_st, node)->key);
				break;
			default:
				CBADBG(" <- [%04d] meth=%d plen=%d lft=%p rgt=%p\n", __LINE__, meth, plen, p->node.b[0], p->node.b[1]);
				break;
			}
		}

		if (p == container_of(*root, struct cba_st, node)) {
			/* loops over itself, it's a leaf */
			switch (key_type) {
			case CB_KT_ST:
				CBADBG(" ^! [%04d] meth=%d plen=%d llen=%d rlen=%d xlen=%d p=%p pkey=str('%s') key=str('%s')\n", __LINE__, meth, plen, llen, rlen, xlen, p, (const char*)p->key, (meth == CB_WM_KEY) ? (const char*)key_ptr : "");
				break;
			default:
				CBADBG(" ^! [%04d] meth=%d plen=%d llen=%d rlen=%d xlen=%d p=%p\n", __LINE__, meth, plen, llen, rlen, xlen, p);
				break;
			}
			break;
		}
		plen = xlen;
	}

	/* if we've exited on an exact match after visiting a regular node
	 * (i.e. not the nodeless leaf), avoid checking the string again.
	 * However if it doesn't match, we must make sure to compare from
	 * within the key (which can be shorter than the ones already there),
	 * so we restart the check with the longest of the two lengths, which
	 * guarantees these bits exist. Test with "100", "10", "1" to see where
	 * this is needed.
	 */
	if (found || meth != CB_WM_KEY)
		plen = -1;
	else
		plen = (llen > rlen) ? llen : rlen;

	/* we may have plen==-1 if we've got an exact match over the whole key length above */

	/* update the pointers needed for modifications (insert, delete) */
	if (ret_nside)
		*ret_nside = (plen < 0) || strcmp(key_ptr + plen / 8, (const void *)p->key + plen / 8) >= 0;

	if (ret_root)
		*ret_root = root;

	/* info needed by delete */
	if (ret_lpside)
		*ret_lpside = lpside;

	if (ret_lparent)
		*ret_lparent = lparent;

	if (ret_npside)
		*ret_npside = npside;

	if (ret_nparent)
		*ret_nparent = nparent;

	if (ret_gpside)
		*ret_gpside = gpside;

	if (ret_gparent)
		*ret_gparent = gparent;

	if (ret_alt_l)
		*ret_alt_l = alt_l;

	if (ret_alt_r)
		*ret_alt_r = alt_r;

	switch (key_type) {
	case CB_KT_ST:
		CBADBG("<<< [%04d] meth=%d plen=%d xlen=%d p=%p pkey=str('%s') key=str('%s')\n", __LINE__, meth, plen, xlen, p, (const char*)p->key, (meth == CB_WM_KEY) ? (const char*)key_ptr : "");
		break;
	default:
		CBADBG("<<< [%04d] meth=%d plen=%d xlen=%d p=%p\n", __LINE__, meth, plen, xlen, p);
		break;
	}

	/* For lookups, an equal value means an instant return. For insertions,
	 * it is the same, we want to return the previously existing value so
	 * that the caller can decide what to do. For deletion, we also want to
	 * return the pointer that's about to be deleted.
	 */
	if (plen < 0 || strcmp(key_ptr + plen / 8, (const void *)p->key + plen / 8) == 0)
		return &p->node;

	/* lookups and deletes fail here */

	/* plain lookups just stop here */
	if (!ret_root)
		return NULL;

	/* inserts return the node we expect to insert */
	return node;
}


/* Generic tree insertion function for trees with unique keys. Inserts node
 * <node> into tree <tree>, with key type <key_type> and key <key_ptr>.
 * Returns the inserted node or the one that already contains the same key.
 */
static inline __attribute__((always_inline))
struct cba_node *_cbau_insert(struct cba_node **root,
			      struct cba_node *node,
			      enum cba_key_type key_type,
			      const void *key_ptr)
{
	struct cba_node **parent;
	struct cba_node *ret;
	int nside;

	if (!*root) {
		/* empty tree, insert a leaf only */
		node->b[0] = node->b[1] = node;
		*root = node;
		return node;
	}

	ret = _cbau_descend(root, CB_WM_KEY, node, key_type, key_ptr, &nside, &parent, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	if (ret == node) {
		/* better use an "if" like this because the inline function
		 * above already has quite identifiable code paths. This
		 * reduces the code and optimizes it a bit.
		 */
		if (nside) {
			node->b[1] = node;
			node->b[0] = *parent;
		} else {
			node->b[0] = node;
			node->b[1] = *parent;
		}
		*parent = ret;
	}
	return ret;
}


#endif /* _CBATREE_PRV_H */
