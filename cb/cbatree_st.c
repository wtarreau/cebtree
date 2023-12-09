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


/*
 * These trees are optimized for adding the minimalest overhead to the stored
 * data. This version uses the node's pointer as the key, for the purpose of
 * quickly finding its neighbours.
 *
 * A few properties :
 * - the xor between two branches of a node cannot be zero since unless the two
 *   branches are duplicate keys
 * - the xor between two nodes has *at least* the split bit set, possibly more
 * - the split bit is always strictly smaller for a node than for its parent,
 *   which implies that the xor between the keys of the lowest level node is
 *   always smaller than the xor between a higher level node. Hence the xor
 *   between the branches of a regular leaf is always strictly larger than the
 *   xor of its parent node's branches if this node is different, since the
 *   leaf is associated with a higher level node which has at least one higher
 *   level branch. The first leaf doesn't validate this but is handled by the
 *   rules below.
 * - during the descent, the node corresponding to a leaf is always visited
 *   before the leaf, unless it's the first inserted, nodeless leaf.
 * - the first key is the only one without any node, and it has both its
 *   branches pointing to itself during insertion to detect it (i.e. xor==0).
 * - a leaf is always present as a node on the path from the root, except for
 *   the inserted first key which has no node, and is recognizable by its two
 *   branches pointing to itself.
 * - a consequence of the rules above is that a non-first leaf appearing below
 *   a node will necessarily have an associated node with a split bit equal to
 *   or greater than the node's split bit.
 * - another consequence is that below a node, the split bits are different for
 *   each branches since both of them are already present above the node, thus
 *   at different levels, so their respective XOR values will be different.
 * - since all nodes in a given path have a different split bit, if a leaf has
 *   the same split bit as its parent node, it is necessary its associated leaf
 *
 * When descending along the tree, it is possible to know that a search key is
 * not present, because its XOR with both of the branches is stricly higher
 * than the inter-branch XOR. The reason is simple : the inter-branch XOR will
 * have its highest bit set indicating the split bit. Since it's the bit that
 * differs between the two branches, the key cannot have it both set and
 * cleared when comparing to the branch values. So xoring the key with both
 * branches will emit a higher bit only when the key's bit differs from both
 * branches' similar bit. Thus, the following equation :
 *      (XOR(key, L) > XOR(L, R)) && (XOR(key, R) > XOR(L, R))
 * is only true when the key should be placed above that node. Since the key
 * has a higher bit which differs from the node, either it has it set and the
 * node has it clear (same for both branches), or it has it clear and the node
 * has it set for both branches. For this reason it's enough to compare the key
 * with any node when the equation above is true, to know if it ought to be
 * present on the left or on the right side. This is useful for insertion and
 * for range lookups.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cbatree.h"

/* this structure is aliased to the common cba node during st operations */
struct cba_st {
	struct cba_node node;
	unsigned char key[0];
};

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

/* Generic tree descent function. It must absolutely be inlined so that the
 * compiler can eliminate the tests related to the various return pointers,
 * which must either point to a local variable in the caller, or be NULL.
 * It must not be called with an empty tree, it's the caller business to
 * deal with this special case. It returns in ret_root the location of the
 * pointer to the leaf (i.e. where we have to insert ourselves). The integer
 * pointed to by ret_nside will contain the side the leaf should occupy at
 * its own node, with the sibling being *ret_root.
 */
static inline __attribute__((always_inline))
struct cba_node *cbau_descend_st(/*const*/ struct cba_node **root,
				 /*const*/ struct cba_node *node,
				 int *ret_nside,
				 struct cba_node ***ret_root,
				 struct cba_node **ret_lparent,
				 int *ret_lpside,
				 struct cba_node **ret_nparent,
				 int *ret_npside,
				 struct cba_node **ret_gparent,
				 int *ret_gpside)
{
	struct cba_st *p, *l, *r;
	const unsigned char *key = container_of(node, struct cba_st, node)->key;
	struct cba_node *gparent = NULL;
	struct cba_node *nparent = NULL;
	struct cba_node *lparent;
	int gpside = 0; // side on the grand parent
	int npside = 0;  // side on the node's parent
	long lpside = 0;  // side on the leaf's parent
	long brside = 0;  // branch side when descending
	int llen = 0, rlen = 0, xlen, plen = 0;
	int found = 0;

	/* the parent will be the (possibly virtual) node so that
	 * &lparent->l == root.
	 */
	lparent = container_of(root, struct cba_node, b[0]);
	gparent = nparent = lparent;

	/* the previous xor is initialized to the largest possible inter-branch
	 * value so that it can never match on the first test as we want to use
	 * it to detect a leaf vs node.
	 */
	while (1) {
		//if (llen < 0 || rlen < 0)
		//	printf("at %d llen=%d rlen=%d\n", __LINE__,  llen, rlen);

		p = container_of(*root, struct cba_st, node);

		/* let's prefetch the lower nodes for the keys */
		__builtin_prefetch(p->node.b[0], 0);
		__builtin_prefetch(p->node.b[1], 0);

		/* neither pointer is tagged */
		l = container_of(p->node.b[0], struct cba_st, node);
		r = container_of(p->node.b[1], struct cba_st, node);

		/* two equal pointers identifies the nodeless leaf. */
		if (l == r) {
			//fprintf(stderr, "key %s break at %d llen=%d rlen=%d\n", key, __LINE__, llen, rlen);
			break;
		}

		//lkey = l->key;
		//rkey = r->key;

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
		//brside = (key ^ l->key) >= (key ^ r->key);
		//if (llen < 0 || rlen < 0)
		//	printf("at %d llen=%d rlen=%d\n", __LINE__,  llen, rlen);

		llen = string_equal_bits(key, l->key, 0);
		rlen = string_equal_bits(key, r->key, 0);
		brside = (unsigned)llen <= (unsigned)rlen;
		if (llen < 0 || rlen < 0)
			found = 1;

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
		xlen = string_equal_bits(l->key, r->key, 0);
		if (xlen < plen) { // test using 2 4 6 4
			/* this is a leaf */
			//fprintf(stderr, "key %s break at %d llen=%d rlen=%d xlen=%d plen=%d\n", key, __LINE__, llen, rlen, xlen, plen);
			break;
		}

		plen = xlen;

		/* check the split bit */
		if ((unsigned)llen < (unsigned)plen && (unsigned)rlen < (unsigned)plen) {
			/* can't go lower, the node must be inserted above p
			 * (which is then necessarily a node). We also know
			 * that (key != p->key) because p->key differs from at
			 * least one of its subkeys by a higher bit than the
			 * split bit, so lookups must fail here.
			 */
			//fprintf(stderr, "key %s break at %d llen=%d rlen=%d plen=%d\n", key, __LINE__, llen, rlen, plen);
			break;
		}

		/* here we're guaranteed to be above a node. If this is the
		 * same node as the one we're looking for, let's store the
		 * parent as the node's parent.
		 */
		if (ret_npside || ret_nparent) {
			int mlen = llen > rlen ? llen : rlen;

			if (mlen > plen)
				mlen = plen;

			//if (node == &p->node) { // seems to be OK, but not sure
			//if (llen < 0 || rlen < 0) { // fails with 2 4 6 4
			if (strcmp((const char *)key + mlen / 8, (const char *)p->key + mlen / 8) == 0) {
				/* strcmp() still needed. E.g. 1 2 3 4 10 11 4 3 2 1 10 11 fails otherwise */
				//fprintf(stderr, "key %s found at %d llen=%d rlen=%d plen=%d\n", key, __LINE__, llen, rlen, plen);
				//printf("key=<%s> +p=<%s>, pkey=<%s> +p=<%s>\n",
				//       (const char *)key, (const char *)key + plen / 8,
				//       (const char *)p->key, (const char *)p->key + plen / 8);
				nparent = lparent;
				npside  = lpside;
				/* we've found a match, so we know the node is there but
				 * we still need to walk down to spot all parents.
				 */
				found = 1;
			}
		}

		/* shift all copies by one */
		gparent = lparent;
		gpside = lpside;
		lparent = &p->node;
		lpside = brside;
		//root = &p->node.b[brside];  // significantly slower on x86
		if (brside)
			root = &p->node.b[1];
		else
			root = &p->node.b[0];

		if (p == container_of(*root, struct cba_st, node)) {
			//fprintf(stderr, "key %s break at %d llen=%d rlen=%d plen=%d\n", key, __LINE__, llen, rlen, plen);
			/* loops over itself, it's a leaf */
			break;
		}
		//if (llen < 0 || rlen < 0)
		//	printf("at %d llen=%d rlen=%d\n", __LINE__,  llen, rlen);
	}

	/* if we've exited on an exact match after visiting a regular node
	 * (i.e. not the nodeless leaf), avoid checking the string again.
	 * However if it doesn't match, we must make sure to compare from
	 * within the key (which can be shorter than the ones already there),
	 * so we restart the check with the longest of the two lengths, which
	 * guarantees these bits exist. Test with "100", "10", "1" to see where
	 * this is needed.
	 */
	if (found)
		plen = -1;
	else
		plen = (llen > rlen) ? llen : rlen;

	/* we may have plen==-1 if we've got an exact match over the whole key length above */

	//printf("llen=seb(%s,%s)=%d rlen=seb(%s,%s)=%d plen=%d /8=%d k[p/8]=%d\n", key, l->key, llen, key, r->key, rlen, plen, plen/8, key[plen/8]);

	/* update the pointers needed for modifications (insert, delete) */
	if (ret_nside)
		*ret_nside = (plen < 0) || strcmp((const char *)key + plen / 8, (const char *)p->key + plen / 8) >= 0;

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

	/* For lookups, an equal value means an instant return. For insertions,
	 * it is the same, we want to return the previously existing value so
	 * that the caller can decide what to do. For deletion, we also want to
	 * return the pointer that's about to be deleted.
	 */
	if (plen < 0 || strcmp((const char *)key + plen / 8, (const char *)p->key + plen / 8) == 0)
		return &p->node;

//	/* We're going to insert <node> above leaf <p> and below <root>. It's
//	 * possible that <p> is the first inserted node, or that it's any other
//	 * regular node or leaf. Therefore we don't care about pointer tagging.
//	 * We need to know two things :
//	 *  - whether <p> is left or right on <root>, to unlink it properly and
//	 *    to take its place
//	 *  - whether we attach <p> left or right below us
//	 */
//	if (!ret_root && !ret_lparent && key == p->key) {
//		/* for lookups this is sufficient. For insert the caller can
//		 * verify that the result is not node hence a conflicting value
//		 * already existed. We do not make more efforts for now towards
//		 * duplicates.
//		 */
//		return *root;
//	}

	/* lookups and deletes fail here */
	/* plain lookups just stop here */
	if (!ret_root)
		return NULL;

	/* inserts return the node we expect to insert */
	//printf("descent insert: node=%p key=%d root=%p *root=%p p->node=%p\n", node, key, root, *root, &p->node);
	return node;
	//return &p->node;
}

struct cba_node *cba_insert_st(struct cba_node **root, struct cba_node *node)
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

	ret = cbau_descend_st(root, node, &nside, &parent, NULL, NULL, NULL, NULL, NULL, NULL);

	if (ret == node) {
		node->b[nside] = node;
		node->b[!nside] = *parent;
		*parent = ret;
	}
	return ret;
}

/* look up the specified key, and returns either the node containing it, or
 * NULL if not found.
 */
struct cba_node *cba_lookup_st(struct cba_node **root, const unsigned char *key)
{
	/*const*/ struct cba_node *node = &container_of(key, struct cba_st, key)->node;

	if (!*root)
		return NULL;

	return cbau_descend_st(root, node, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* look up the specified node with its key and deletes it if found, and in any
 * case, returns the node.
 */
struct cba_node *cba_delete_st(struct cba_node **root, struct cba_node *node)
{
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

	ret = cbau_descend_st(root, node, NULL, NULL, &lparent, &lpside, &nparent, &npside, &gparent, &gpside);
	if (ret == node) {
		//fprintf(stderr, "root=%p ret=%p l=%p[%d] n=%p[%d] g=%p[%d]\n", root, ret, lparent, lpside, nparent, npside, gparent, gpside);

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
struct cba_node *cba_pick_st(struct cba_node **root, const unsigned char *key)
{
	/*const*/ struct cba_node *node = &container_of(key, struct cba_st, key)->node;
	struct cba_node *lparent, *nparent, *gparent/*, *sibling*/;
	int lpside, npside, gpside;
	struct cba_node *ret;

	if (!*root)
		return NULL;

	//if (key == 425144) printf("%d: k=%u\n", __LINE__, key);

	ret = cbau_descend_st(root, node, NULL, NULL, &lparent, &lpside, &nparent, &npside, &gparent, &gpside);

	//if (key == 425144) printf("%d: k=%u ret=%p\n", __LINE__, key, ret);

	if (ret) {
		struct cba_st *p = container_of(ret, struct cba_st, node);

		if (ret == node)
			abort();

		if (p->key != key)
			abort();

		//fprintf(stderr, "root=%p ret=%p l=%p[%d] n=%p[%d] g=%p[%d]\n", root, ret, lparent, lpside, nparent, npside, gparent, gpside);

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

#if 0
/* default node dump function */
static void cbast_default_dump_node(struct cba_node *node, int level, const void *ctx)
{
	struct cba_st *key = container_of(node, struct cba_st, node);
	mb pxor, lxor, rxor;

	/* xor of the keys of the two lower branches */
	pxor = container_of(__cba_clrtag(node->b[0]), struct cba_st, node)->key ^
		container_of(__cba_clrtag(node->b[1]), struct cba_st, node)->key;

	printf("  \"%lx_n\" [label=\"%lx\\nlev=%d\\nkey=%u\" fillcolor=\"lightskyblue1\"%s];\n",
	       (long)node, (long)node, level, key->key, (ctx == node) ? " color=red" : "");

	/* xor of the keys of the left branch's lower branches */
	lxor = container_of(__cba_clrtag(((struct cba_node*)__cba_clrtag(node->b[0]))->b[0]), struct cba_st, node)->key ^
		container_of(__cba_clrtag(((struct cba_node*)__cba_clrtag(node->b[0]))->b[1]), struct cba_st, node)->key;

	printf("  \"%lx_n\" -> \"%lx_%c\" [label=\"L\" arrowsize=0.66 %s];\n",
	       (long)node, (long)__cba_clrtag(node->b[0]),
	       (((long)node->b[0] & 1) || (lxor < pxor && ((struct cba_node*)node->b[0])->b[0] != ((struct cba_node*)node->b[0])->b[1])) ? 'n' : 'l',
	       (node == __cba_clrtag(node->b[0])) ? " dir=both" : "");

	/* xor of the keys of the right branch's lower branches */
	rxor = container_of(__cba_clrtag(((struct cba_node*)__cba_clrtag(node->b[1]))->b[0]), struct cba_st, node)->key ^
		container_of(__cba_clrtag(((struct cba_node*)__cba_clrtag(node->b[1]))->b[1]), struct cba_st, node)->key;

	printf("  \"%lx_n\" -> \"%lx_%c\" [label=\"R\" arrowsize=0.66 %s];\n",
	       (long)node, (long)__cba_clrtag(node->b[1]),
	       (((long)node->b[1] & 1) || (rxor < pxor && ((struct cba_node*)node->b[1])->b[0] != ((struct cba_node*)node->b[1])->b[1])) ? 'n' : 'l',
	       (node == __cba_clrtag(node->b[1])) ? " dir=both" : "");
}

/* default leaf dump function */
static void cbast_default_dump_leaf(struct cba_node *node, int level, const void *ctx)
{
	struct cba_st *key = container_of(node, struct cba_st, node);

	if (node->b[0] == node->b[1])
		printf("  \"%lx_l\" [label=\"%lx\\nlev=%d\\nkey=%u\\n\" fillcolor=\"green\"%s];\n",
		       (long)node, (long)node, level, key->key, (ctx == node) ? " color=red" : "");
	else
		printf("  \"%lx_l\" [label=\"%lx\\nlev=%d\\nkey=%u\\n\" fillcolor=\"yellow\"%s];\n",
		       (long)node, (long)node, level, key->key, (ctx == node) ? " color=red" : "");
}

/* Dumps a tree through the specified callbacks. */
void *cba_dump_tree_st(struct cba_node *node, mb pxor, void *last,
			int level,
			void (*node_dump)(struct cba_node *node, int level, const void *ctx),
			void (*leaf_dump)(struct cba_node *node, int level, const void *ctx),
			const void *ctx)
{
	mb xor;

	if (!node) /* empty tree */
		return node;

	//fprintf(stderr, "node=%p level=%d key=%u l=%p r=%p\n", node, level, *(unsigned *)((char*)(node)+16), node->b[0], node->b[1]);

	if (level < 0) {
		/* we're inside a dup tree. Tagged pointers indicate nodes,
		 * untagged ones leaves.
		 */
		level--;
		if (__cba_tagged(node->b[0])) {
		  last = cba_dump_tree_st(__cba_untag(node->b[0]), 0, last, level, node_dump, leaf_dump, ctx);
			if (node_dump)
			  node_dump(__cba_untag(node->b[0]), level, ctx);
		} else if (leaf_dump)
			leaf_dump(node->b[0], level, ctx);

		if (__cba_tagged(node->b[1])) {
			last = cba_dump_tree_st(__cba_untag(node->b[1]), 0, last, level, node_dump, leaf_dump, ctx);
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
		return cba_dump_tree_st(node, 0, last, -1, node_dump, leaf_dump, ctx);
	}

	xor = ((struct cba_st*)node->b[0])->key ^ ((struct cba_st*)node->b[1])->key;
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
		return cba_dump_tree_st(node, 0, last, -1, node_dump, leaf_dump, ctx);
	}

	/* that's a regular node */
	if (node_dump)
		node_dump(node, level, ctx);

	last = cba_dump_tree_st(node->b[0], xor, last, level + 1, node_dump, leaf_dump, ctx);
	return cba_dump_tree_st(node->b[1], xor, last, level + 1, node_dump, leaf_dump, ctx);
}

/* dumps a cba_st tree using the default functions above. If a node matches
 * <ctx>, this one will be highlighted in red.
 */
void cbast_default_dump(struct cba_node **cba_root, const char *label, const void *ctx)
{
	struct cba_node *node;

	printf("\ndigraph cba_tree_st {\n"
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

	cba_dump_tree_st(*cba_root, 0, NULL, 0, cbast_default_dump_node, cbast_default_dump_leaf, ctx);

	printf("}\n");
}
#endif
