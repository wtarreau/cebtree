/*
 * Compact Binary Trees - internal functions and types
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

/* This file MUST NOT be included by public code, it contains macros, enums
 * with short names and function definitions that may clash with user code.
 * It may only be included by the respective types' C files.
 */

/*
 * These trees are optimized for adding the minimalest overhead to the stored
 * data. This version uses the node's pointer as the key, for the purpose of
 * quickly finding its neighbours.
 *
 * A few properties :
 * - the xor between two branches of a node cannot be zero unless the two
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

#ifndef _CBTREE_PRV_H
#define _CBTREE_PRV_H

#include <inttypes.h>
#include <string.h>

/* If DEBUG is set, we'll print additional debugging info during the descent */
#ifdef DEBUG
#define CBDBG(x, ...) fprintf(stderr, x, ##__VA_ARGS__)
#else
#define CBDBG(x, ...) do { } while (0)
#endif


/* tree walk method: key, left, right */
enum cb_walk_meth {
	CB_WM_FST,     /* look up "first" (walk left only) */
	CB_WM_NXT,     /* look up "next" (walk right once then left) */
	CB_WM_PRV,     /* look up "prev" (walk left once then right) */
	CB_WM_LST,     /* look up "last" (walk right only) */
	/* all methods from CB_WM_KEQ and above do have a key */
	CB_WM_KEQ,     /* look up the node equal to the key  */
	CB_WM_KGE,     /* look up the node greater than or equal to the key */
	CB_WM_KGT,     /* look up the node greater than the key */
	CB_WM_KLE,     /* look up the node lower than or equal to the key */
	CB_WM_KLT,     /* look up the node lower than the key */
	CB_WM_KNX,     /* look up the node's key first, then find the next */
	CB_WM_KPR,     /* look up the node's key first, then find the prev */
};

enum cb_key_type {
	CB_KT_ADDR,    /* the key is the node's address */
	CB_KT_U32,     /* 32-bit unsigned word in key_u32 */
	CB_KT_U64,     /* 64-bit unsigned word in key_u64 */
	CB_KT_MB,      /* fixed size memory block in (key_u64,key_ptr), direct storage */
	CB_KT_IM,      /* fixed size memory block in (key_u64,key_ptr), indirect storage */
	CB_KT_ST,      /* NUL-terminated string in key_ptr, direct storage */
	CB_KT_IS,      /* NUL-terminated string in key_ptr, indirect storage */
};

union cb_key_storage {
	uint32_t u32;
	uint64_t u64;
	unsigned long ul;
	unsigned char mb[0];
	unsigned char str[0];
	unsigned char *ptr; /* for CB_KT_IS */
};

/* this structure is aliased to the common cba node during st operations */
struct cb_node_key {
	struct cb_node node;
	union cb_key_storage key;
};

/* Returns the xor (or common length) between the two sides <l> and <r> if both
 * are non-null, otherwise between the first non-null one and the value in the
 * associate key. As a reminder, memory blocks place their length in key_u64.
 * This is only intended for internal use, essentially for debugging.
 */
__attribute__((unused))
static inline uint64_t _xor_branches(enum cb_key_type key_type, uint32_t key_u32,
				     uint64_t key_u64, const void *key_ptr,
				     const struct cb_node_key *l,
				     const struct cb_node_key *r)
{
	if (l && r) {
		if (key_type == CB_KT_MB)
			return equal_bits(l->key.mb, r->key.mb, 0, key_u64 << 3);
		else if (key_type == CB_KT_IM)
			return equal_bits(l->key.mb, r->key.ptr, 0, key_u64 << 3);
		else if (key_type == CB_KT_ST)
			return string_equal_bits(l->key.str, r->key.str, 0);
		else if (key_type == CB_KT_IS)
			return string_equal_bits(l->key.ptr, r->key.ptr, 0);
		else if (key_type == CB_KT_U64)
			return l->key.u64 ^ r->key.u64;
		else if (key_type == CB_KT_U32)
			return l->key.u32 ^ r->key.u32;
		else if (key_type == CB_KT_ADDR)
			return ((uintptr_t)l ^ (uintptr_t)r);
		else
			return 0;
	}

	if (!l)
		l = r;

	if (key_type == CB_KT_MB)
		return equal_bits(key_ptr, l->key.mb, 0, key_u64 << 3);
	else if (key_type == CB_KT_IM)
		return equal_bits(key_ptr, l->key.ptr, 0, key_u64 << 3);
	else if (key_type == CB_KT_ST)
		return string_equal_bits(key_ptr, l->key.str, 0);
	else if (key_type == CB_KT_IS)
		return string_equal_bits(key_ptr, l->key.ptr, 0);
	else if (key_type == CB_KT_U64)
		return key_u64 ^ l->key.u64;
	else if (key_type == CB_KT_U32)
		return key_u32 ^ l->key.u32;
	else if (key_type == CB_KT_ADDR)
		return ((uintptr_t)key_ptr ^ (uintptr_t)r);
	else
		return 0;
}

#ifdef DEBUG
__attribute__((unused))
static void dbg(int line,
		const char *pfx,
		enum cb_walk_meth meth,
		enum cb_key_type key_type,
		struct cb_node * const *root,
		const struct cb_node_key *p,
		uint32_t key_u32,
		uint64_t key_u64,
		const void *key_ptr,
		uint32_t px32,
		uint64_t px64,
		size_t plen)
{
	const char *meths[] = {
		[CB_WM_FST] = "FST",
		[CB_WM_NXT] = "NXT",
		[CB_WM_PRV] = "PRV",
		[CB_WM_LST] = "LST",
		[CB_WM_KEQ] = "KEQ",
		[CB_WM_KGE] = "KGE",
		[CB_WM_KGT] = "KGT",
		[CB_WM_KLE] = "KLE",
		[CB_WM_KLT] = "KLT",
		[CB_WM_KNX] = "KNX",
		[CB_WM_KPR] = "KPR",
	};
	const char *ktypes[] = {
		[CB_KT_ADDR] = "ADDR",
		[CB_KT_U32]  = "U32",
		[CB_KT_U64]  = "U64",
		[CB_KT_MB]   = "MB",
		[CB_KT_IM]   = "IM",
		[CB_KT_ST]   = "ST",
		[CB_KT_IS]   = "IS",
	};
	const struct cb_node_key *l = NULL;
	const struct cb_node_key *r = NULL;
	const char *kstr __attribute__((unused)) = ktypes[key_type];
	const char *mstr __attribute__((unused)) = meths[meth];
	long long nlen __attribute__((unused)) = 0;
	long long llen __attribute__((unused)) = 0;
	long long rlen __attribute__((unused)) = 0;
	long long xlen __attribute__((unused)) = 0;

	if (p) {
		l = container_of(p->node.b[0], struct cb_node_key, node);
		r = container_of(p->node.b[1], struct cb_node_key, node);
		nlen = _xor_branches(key_type, key_u32, key_u64, key_ptr, p, NULL);
	}

	if (l)
		llen = _xor_branches(key_type, key_u32, key_u64, key_ptr, l, NULL);

	if (r)
		rlen = _xor_branches(key_type, key_u32, key_u64, key_ptr, NULL, r);

	if (l && r)
		xlen = _xor_branches(key_type, key_u32, key_u64, key_ptr, l, r);

	switch (key_type) {
	case CB_KT_U32:
		CBDBG("%04d (%8s) m=%s.%s key=%#x root=%p pxor=%#x p=%p,%#x(^%#llx) l=%p,%#x(^%#llx) r=%p,%#x(^%#llx) l^r=%#llx\n",
		      line, pfx, kstr, mstr, key_u32, root, px32,
		      p ? &p->node : NULL, p ? p->key.u32 : 0, nlen,
		      l ? &l->node : NULL, l ? l->key.u32 : 0, llen,
		      r ? &r->node : NULL, r ? r->key.u32 : 0, rlen,
		      xlen);
		break;
	case CB_KT_U64:
		CBDBG("%04d (%8s) m=%s.%s key=%#llx root=%p pxor=%#llx p=%p,%#llx(^%#llx) l=%p,%#llx(^%#llx) r=%p,%#llx(^%#llx) l^r=%#llx\n",
		      line, pfx, kstr, mstr, (long long)key_u64, root, (long long)px64,
		      p ? &p->node : NULL, (long long)(p ? p->key.u64 : 0), nlen,
		      l ? &l->node : NULL, (long long)(l ? l->key.u64 : 0), llen,
		      r ? &r->node : NULL, (long long)(r ? r->key.u64 : 0), rlen,
		      xlen);
		break;
	case CB_KT_MB:
		CBDBG("%04d (%8s) m=%s.%s key=%p root=%p plen=%ld p=%p,%p(^%llu) l=%p,%p(^%llu) r=%p,%p(^%llu) l^r=%llu\n",
		      line, pfx, kstr, mstr, key_ptr, root, (long)plen,
		      p ? &p->node : NULL, p ? p->key.mb : 0, nlen,
		      l ? &l->node : NULL, l ? l->key.mb : 0, llen,
		      r ? &r->node : NULL, r ? r->key.mb : 0, rlen,
		      xlen);
		break;
	case CB_KT_IM:
		CBDBG("%04d (%8s) m=%s.%s key=%p root=%p plen=%ld p=%p,%p(^%llu) l=%p,%p(^%llu) r=%p,%p(^%llu) l^r=%llu\n",
		      line, pfx, kstr, mstr, key_ptr, root, (long)plen,
		      p ? &p->node : NULL, p ? p->key.ptr : 0, nlen,
		      l ? &l->node : NULL, l ? l->key.ptr : 0, llen,
		      r ? &r->node : NULL, r ? r->key.ptr : 0, rlen,
		      xlen);
		break;
	case CB_KT_ST:
		CBDBG("%04d (%8s) m=%s.%s key='%s' root=%p plen=%ld p=%p,%s(^%llu) l=%p,%s(^%llu) r=%p,%s(^%llu) l^r=%llu\n",
		      line, pfx, kstr, mstr, key_ptr ? (const char *)key_ptr : "", root, (long)plen,
		      p ? &p->node : NULL, p ? (const char *)p->key.str : "-", nlen,
		      l ? &l->node : NULL, l ? (const char *)l->key.str : "-", llen,
		      r ? &r->node : NULL, r ? (const char *)r->key.str : "-", rlen,
		      xlen);
		break;
	case CB_KT_IS:
		CBDBG("%04d (%8s) m=%s.%s key='%s' root=%p plen=%ld p=%p,%s(^%llu) l=%p,%s(^%llu) r=%p,%s(^%llu) l^r=%llu\n",
		      line, pfx, kstr, mstr, key_ptr ? (const char *)key_ptr : "", root, (long)plen,
		      p ? &p->node : NULL, p ? (const char *)p->key.ptr : "-", nlen,
		      l ? &l->node : NULL, l ? (const char *)l->key.ptr : "-", llen,
		      r ? &r->node : NULL, r ? (const char *)r->key.ptr : "-", rlen,
		      xlen);
		break;
	case CB_KT_ADDR:
		/* key type is the node's address */
		CBDBG("%04d (%8s) m=%s.%s key=%#llx root=%p pxor=%#llx p=%p,%#llx(^%#llx) l=%p,%#llx(^%#llx) r=%p,%#llx(^%#llx) l^r=%#llx\n",
		      line, pfx, kstr, mstr, (long long)(uintptr_t)key_ptr, root, (long long)px64,
		      p ? &p->node : NULL, (long long)(uintptr_t)p, nlen,
		      l ? &l->node : NULL, (long long)(uintptr_t)l, llen,
		      r ? &r->node : NULL, (long long)(uintptr_t)r, rlen,
		      xlen);
	}
}
#else
#define dbg(...) do { } while (0)
#endif

/* Generic tree descent function. It must absolutely be inlined so that the
 * compiler can eliminate the tests related to the various return pointers,
 * which must either point to a local variable in the caller, or be NULL.
 * It must not be called with an empty tree, it's the caller business to
 * deal with this special case. It returns in ret_root the location of the
 * pointer to the leaf (i.e. where we have to insert ourselves). The integer
 * pointed to by ret_nside will contain the side the leaf should occupy at
 * its own node, with the sibling being *ret_root. Note that keys for fixed-
 * size arrays are passed in key_ptr with their length in key_u64. For keyless
 * nodes whose address serves as the key, the pointer needs to be passed in
 * key_ptr, and pxor64 will be used internally.
 */
static inline __attribute__((always_inline))
struct cb_node *_cbu_descend(struct cb_node **root,
			     enum cb_walk_meth meth,
			     enum cb_key_type key_type,
			     uint32_t key_u32,
			     uint64_t key_u64,
			     const void *key_ptr,
			     int *ret_nside,
			     struct cb_node ***ret_root,
			     struct cb_node **ret_lparent,
			     int *ret_lpside,
			     struct cb_node **ret_nparent,
			     int *ret_npside,
			     struct cb_node **ret_gparent,
			     int *ret_gpside,
			     struct cb_node **ret_back)
{
	struct cb_node_key *p, *l, *r;
	struct cb_node *gparent = NULL;
	struct cb_node *nparent = NULL;
	struct cb_node_key *bnode = NULL;
	struct cb_node *lparent;
	uint32_t pxor32 = ~0U;   // previous xor between branches
	uint64_t pxor64 = ~0ULL; // previous xor between branches
	int gpside = 0;   // side on the grand parent
	int npside = 0;   // side on the node's parent
	long lpside = 0;  // side on the leaf's parent
	long brside = 0;  // branch side when descending
	size_t llen = 0;  // left vs key matching length
	size_t rlen = 0;  // right vs key matching length
	size_t plen = 0;  // previous common len between branches
	int found = 0;    // key was found (saves an extra strcmp for arrays)

	dbg(__LINE__, "_enter__", meth, key_type, root, NULL, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);

	/* the parent will be the (possibly virtual) node so that
	 * &lparent->l == root.
	 */
	lparent = container_of(root, struct cb_node, b[0]);
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
	 * it to detect a leaf vs node. That's achieved with plen==0 for arrays
	 * and pxorXX==~0 for scalars.
	 */
	while (1) {
		p = container_of(*root, struct cb_node_key, node);

		/* let's prefetch the lower nodes for the keys */
		__builtin_prefetch(p->node.b[0], 0);
		__builtin_prefetch(p->node.b[1], 0);

		/* neither pointer is tagged */
		l = container_of(p->node.b[0], struct cb_node_key, node);
		r = container_of(p->node.b[1], struct cb_node_key, node);

		dbg(__LINE__, "newp", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);

		/* two equal pointers identifies the nodeless leaf. */
		if (l == r) {
			dbg(__LINE__, "l==r", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
			break;
		}

		/* In the following block, we're dealing with type-specific
		 * operations which follow the same construct for each type:
		 *   1) calculate the new side for key lookups (otherwise keep
		 *      the current side, e.g. for first/last). Doing it early
		 *      allows the CPU to more easily predict next branches and
		 *      is faster by ~10%. For complex bits we keep the length
		 *      of identical bits instead of xor. We can also xor lkey
		 *      and rkey with key and use it everywhere later but it
		 *      doesn't seem to bring anything.
		 *
		 *   2) calculate the xor between the two sides to figure the
		 *      split bit position. If the new split bit is before the
		 *      previous one, we've reached a leaf: each leaf we visit
		 *      had its node part already visited. The only way to
		 *      distinguish them is that the inter-branch xor of the
		 *      leaf will be the node's one, and will necessarily be
		 *      larger than the previous node's xor if the node is
		 *      above (we've already checked for direct descendent
		 *      below). Said differently, if an inter-branch xor is
		 *      strictly larger than the previous one, it necessarily
		 *      is the one of an upper node, so what we're seeing
		 *      cannot be the node, hence it's the leaf. The case where
		 *      they're equal was already dealt with by the test at the
		 *      end of the loop (node points to self). For scalar keys,
		 *      we directly store the last xor value in pxorXX. For
		 *      arrays and strings, instead we store the previous equal
		 *      length.
		 *
		 *   3) for lookups, check if the looked key still has a chance
		 *      to be below: if it has a xor with both branches that is
		 *      larger than the xor between them, it cannot be there,
		 *      since it means that it differs from these branches by
		 *      at least one bit that's higher than the split bit,
		 *      hence not common to these branches. In such cases:
		 *      - if we're just doing a lookup, the key is not found
		 *        and we fail.
		 *      - if we are inserting, we must stop here and we have
		 *        the guarantee to be above a node.
		 *      - if we're deleting, it could be the key we were
		 *        looking for so we have to check for it as long as
		 *        it's still possible to keep a copy of the node's
		 *        parent. <found> is set int this case for expensive
		 *        types.
		 */

		if (key_type == CB_KT_U32) {
			uint32_t xor32;   // left vs right branch xor

			if (meth >= CB_WM_KEQ) {
				/* "found" is not used here */
				brside = (key_u32 ^ l->key.u32) >= (key_u32 ^ r->key.u32);
			}

			xor32 = l->key.u32 ^ r->key.u32;
			if (xor32 > pxor32) { // test using 2 4 6 4
				dbg(__LINE__, "xor>", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				break;
			}

			if (meth >= CB_WM_KEQ) {
				/* let's stop if our key is not there */

				if ((key_u32 ^ l->key.u32) > xor32 && (key_u32 ^ r->key.u32) > xor32) {
					dbg(__LINE__, "mismatch", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
					break;
				}

				if (ret_npside || ret_nparent) {
					if (key_u32 == p->key.u32) {
						dbg(__LINE__, "equal", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
						nparent = lparent;
						npside  = lpside;
					}
				}
			}
			pxor32 = xor32;
		}
		else if (key_type == CB_KT_U64) {
			uint64_t xor64;   // left vs right branch xor

			if (meth >= CB_WM_KEQ) {
				/* "found" is not used here */
				brside = (key_u64 ^ l->key.u64) >= (key_u64 ^ r->key.u64);
			}

			xor64 = l->key.u64 ^ r->key.u64;
			if (xor64 > pxor64) { // test using 2 4 6 4
				dbg(__LINE__, "xor>", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				break;
			}

			if (meth >= CB_WM_KEQ) {
				/* let's stop if our key is not there */

				if ((key_u64 ^ l->key.u64) > xor64 && (key_u64 ^ r->key.u64) > xor64) {
					dbg(__LINE__, "mismatch", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
					break;
				}

				if (ret_npside || ret_nparent) {
					if (key_u64 == p->key.u64) {
						dbg(__LINE__, "equal", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
						nparent = lparent;
						npside  = lpside;
					}
				}
			}
			pxor64 = xor64;
		}
		else if (key_type == CB_KT_MB) {
			size_t xlen = 0; // left vs right matching length

			if (meth >= CB_WM_KEQ) {
				/* measure identical lengths */
				llen = equal_bits(key_ptr, l->key.mb, 0, key_u64 << 3);
				rlen = equal_bits(key_ptr, r->key.mb, 0, key_u64 << 3);
				brside = llen <= rlen;
				if (llen == rlen && (uint64_t)llen == key_u64 << 3)
					found = 1;
			}

			xlen = equal_bits(l->key.mb, r->key.mb, 0, key_u64 << 3);
			if (xlen < plen) {
				/* this is a leaf. E.g. triggered using 2 4 6 4 */
				dbg(__LINE__, "xor>", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				break;
			}

			if (meth >= CB_WM_KEQ) {
				/* let's stop if our key is not there */

				if (llen < xlen && rlen < xlen) {
					dbg(__LINE__, "mismatch", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
					break;
				}

				if (ret_npside || ret_nparent) { // delete ?
					size_t mlen = llen > rlen ? llen : rlen;

					if (mlen > xlen)
						mlen = xlen;

					if ((uint64_t)xlen / 8 == key_u64 || memcmp(key_ptr + mlen / 8, p->key.mb + mlen / 8, key_u64 - mlen / 8) == 0) {
						dbg(__LINE__, "equal", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
						nparent = lparent;
						npside  = lpside;
						found = 1;
					}
				}
			}
			plen = xlen;
		}
		else if (key_type == CB_KT_IM) {
			size_t xlen = 0; // left vs right matching length

			if (meth >= CB_WM_KEQ) {
				/* measure identical lengths */
				llen = equal_bits(key_ptr, l->key.ptr, 0, key_u64 << 3);
				rlen = equal_bits(key_ptr, r->key.ptr, 0, key_u64 << 3);
				brside = llen <= rlen;
				if (llen == rlen && (uint64_t)llen == key_u64 << 3)
					found = 1;
			}

			xlen = equal_bits(l->key.ptr, r->key.ptr, 0, key_u64 << 3);
			if (xlen < plen) {
				/* this is a leaf. E.g. triggered using 2 4 6 4 */
				dbg(__LINE__, "xor>", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				break;
			}

			if (meth >= CB_WM_KEQ) {
				/* let's stop if our key is not there */

				if (llen < xlen && rlen < xlen) {
					dbg(__LINE__, "mismatch", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
					break;
				}

				if (ret_npside || ret_nparent) { // delete ?
					size_t mlen = llen > rlen ? llen : rlen;

					if (mlen > xlen)
						mlen = xlen;

					if ((uint64_t)xlen / 8 == key_u64 || memcmp(key_ptr + mlen / 8, p->key.ptr + mlen / 8, key_u64 - mlen / 8) == 0) {
						dbg(__LINE__, "equal", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
						nparent = lparent;
						npside  = lpside;
						found = 1;
					}
				}
			}
			plen = xlen;
		}
		else if (key_type == CB_KT_ST) {
			size_t xlen = 0; // left vs right matching length

			if (meth >= CB_WM_KEQ) {
				/* Note that a negative length indicates an
				 * equal value with the final zero reached, but
				 * it is still needed to descend to find the
				 * leaf. We take that negative length for an
				 * infinite one, hence the uint cast.
				 */
				llen = string_equal_bits(key_ptr, l->key.str, 0);
				rlen = string_equal_bits(key_ptr, r->key.str, 0);
				brside = (size_t)llen <= (size_t)rlen;
				if ((ssize_t)llen < 0 || (ssize_t)rlen < 0)
					found = 1;
			}

			xlen = string_equal_bits(l->key.str, r->key.str, 0);
			if (xlen < plen) {
				/* this is a leaf. E.g. triggered using 2 4 6 4 */
				dbg(__LINE__, "xor>", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				break;
			}

			if (meth >= CB_WM_KEQ) {
				/* let's stop if our key is not there */

				if ((unsigned)llen < (unsigned)xlen && (unsigned)rlen < (unsigned)xlen) {
					dbg(__LINE__, "mismatch", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
					break;
				}

				if (ret_npside || ret_nparent) { // delete ?
					size_t mlen = llen > rlen ? llen : rlen;

					if (mlen > xlen)
						mlen = xlen;

					if (strcmp(key_ptr + mlen / 8, (const void *)p->key.str + mlen / 8) == 0) {
						/* strcmp() still needed. E.g. 1 2 3 4 10 11 4 3 2 1 10 11 fails otherwise */
						dbg(__LINE__, "equal", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
						nparent = lparent;
						npside  = lpside;
						found = 1;
					}
				}
			}
			plen = xlen;
		}
		else if (key_type == CB_KT_IS) {
			size_t xlen = 0; // left vs right matching length

			if (meth >= CB_WM_KEQ) {
				/* Note that a negative length indicates an
				 * equal value with the final zero reached, but
				 * it is still needed to descend to find the
				 * leaf. We take that negative length for an
				 * infinite one, hence the uint cast.
				 */
				llen = string_equal_bits(key_ptr, l->key.ptr, 0);
				rlen = string_equal_bits(key_ptr, r->key.ptr, 0);
				brside = (size_t)llen <= (size_t)rlen;
				if ((ssize_t)llen < 0 || (ssize_t)rlen < 0)
					found = 1;
			}

			xlen = string_equal_bits(l->key.ptr, r->key.ptr, 0);
			if (xlen < plen) {
				/* this is a leaf. E.g. triggered using 2 4 6 4 */
				dbg(__LINE__, "xor>", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				break;
			}

			if (meth >= CB_WM_KEQ) {
				/* let's stop if our key is not there */

				if ((unsigned)llen < (unsigned)xlen && (unsigned)rlen < (unsigned)xlen) {
					dbg(__LINE__, "mismatch", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
					break;
				}

				if (ret_npside || ret_nparent) { // delete ?
					size_t mlen = llen > rlen ? llen : rlen;

					if (mlen > xlen)
						mlen = xlen;

					if (strcmp(key_ptr + mlen / 8, (const void *)p->key.ptr + mlen / 8) == 0) {
						/* strcmp() still needed. E.g. 1 2 3 4 10 11 4 3 2 1 10 11 fails otherwise */
						dbg(__LINE__, "equal", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
						nparent = lparent;
						npside  = lpside;
						found = 1;
					}
				}
			}
			plen = xlen;
		}
		else if (key_type == CB_KT_ADDR) {
			uintptr_t xoraddr;   // left vs right branch xor

			if (meth >= CB_WM_KEQ) {
				/* "found" is not used here */
				brside = ((uintptr_t)key_ptr ^ (uintptr_t)l) >= ((uintptr_t)key_ptr ^ (uintptr_t)r);
			}

			xoraddr = (uintptr_t)l ^ (uintptr_t)r;
			if (xoraddr > (uintptr_t)pxor64) { // test using 2 4 6 4
				dbg(__LINE__, "xor>", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				break;
			}

			if (meth >= CB_WM_KEQ) {
				/* let's stop if our key is not there */

				if (((uintptr_t)key_ptr ^ (uintptr_t)l) > xoraddr && ((uintptr_t)key_ptr ^ (uintptr_t)r) > xoraddr) {
					dbg(__LINE__, "mismatch", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
					break;
				}

				if (ret_npside || ret_nparent) {
					if ((uintptr_t)key_ptr == (uintptr_t)p) {
						dbg(__LINE__, "equal", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
						nparent = lparent;
						npside  = lpside;
					}
				}
			}
			pxor64 = xoraddr;
		}

		/* shift all copies by one */
		gparent = lparent;
		gpside = lpside;
		lparent = &p->node;
		lpside = brside;
		if (brside) {
			if (meth == CB_WM_KPR || meth == CB_WM_KLE || meth == CB_WM_KLT)
				bnode = p;
			root = &p->node.b[1];

			/* change branch for key-less walks */
			if (meth == CB_WM_NXT)
				brside = 0;

			dbg(__LINE__, "side1", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
		}
		else {
			if (meth == CB_WM_KNX || meth == CB_WM_KGE || meth == CB_WM_KGT)
				bnode = p;
			root = &p->node.b[0];

			/* change branch for key-less walks */
			if (meth == CB_WM_PRV)
				brside = 1;

			dbg(__LINE__, "side0", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
		}

		if (p == container_of(*root, struct cb_node_key, node)) {
			/* loops over itself, it's a leaf */
			dbg(__LINE__, "loop", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
			break;
		}
	}

	/* here we're on the closest node from the requested value. It may be
	 * slightly lower (has a zero where we expected a one) or slightly
	 * larger has a one where we expected a zero). Thus another check is
	 * still deserved, depending on the matching method.
	 */

	/* if we've exited on an exact match after visiting a regular node
	 * (i.e. not the nodeless leaf), we'll avoid checking the string again.
	 * However if it doesn't match, we must make sure to compare from
	 * within the key (which can be shorter than the ones already there),
	 * so we restart the check from the longest of the two lengths, which
	 * guarantees these bits exist. Test with "100", "10", "1" to see where
	 * this is needed.
	 */
	if ((key_type == CB_KT_ST || key_type == CB_KT_IS) && meth >= CB_WM_KEQ && !found)
		plen = (llen > rlen) ? llen : rlen;

	/* update the pointers needed for modifications (insert, delete) */
	if (ret_nside && meth >= CB_WM_KEQ) {
		switch (key_type) {
		case CB_KT_U32:
			*ret_nside = key_u32 >= p->key.u32;
			break;
		case CB_KT_U64:
			*ret_nside = key_u64 >= p->key.u64;
			break;
		case CB_KT_MB:
			*ret_nside = (uint64_t)plen / 8 == key_u64 || memcmp(key_ptr + plen / 8, p->key.mb + plen / 8, key_u64 - plen / 8) >= 0;
			break;
		case CB_KT_IM:
			*ret_nside = (uint64_t)plen / 8 == key_u64 || memcmp(key_ptr + plen / 8, p->key.ptr + plen / 8, key_u64 - plen / 8) >= 0;
			break;
		case CB_KT_ST:
			*ret_nside = found || strcmp(key_ptr + plen / 8, (const void *)p->key.str + plen / 8) >= 0;
			break;
		case CB_KT_IS:
			*ret_nside = found || strcmp(key_ptr + plen / 8, (const void *)p->key.ptr + plen / 8) >= 0;
			break;
		case CB_KT_ADDR:
			*ret_nside = (uintptr_t)key_ptr >= (uintptr_t)p;
			break;
		}
	}

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

	if (ret_back)
		*ret_back = &bnode->node;

	dbg(__LINE__, "_ret____", meth, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);

	if (meth >= CB_WM_KEQ) {
		/* For lookups, an equal value means an instant return. For insertions,
		 * it is the same, we want to return the previously existing value so
		 * that the caller can decide what to do. For deletion, we also want to
		 * return the pointer that's about to be deleted.
		 */
		if (key_type == CB_KT_U32) {
			if ((meth == CB_WM_KEQ && p->key.u32 == key_u32) ||
			    (meth == CB_WM_KNX && p->key.u32 == key_u32) ||
			    (meth == CB_WM_KPR && p->key.u32 == key_u32) ||
			    (meth == CB_WM_KGE && p->key.u32 >= key_u32) ||
			    (meth == CB_WM_KGT && p->key.u32 >  key_u32) ||
			    (meth == CB_WM_KLE && p->key.u32 <= key_u32) ||
			    (meth == CB_WM_KLT && p->key.u32 <  key_u32))
				return &p->node;
		}
		else if (key_type == CB_KT_U64) {
			if ((meth == CB_WM_KEQ && p->key.u64 == key_u64) ||
			    (meth == CB_WM_KNX && p->key.u64 == key_u64) ||
			    (meth == CB_WM_KPR && p->key.u64 == key_u64) ||
			    (meth == CB_WM_KGE && p->key.u64 >= key_u64) ||
			    (meth == CB_WM_KGT && p->key.u64 >  key_u64) ||
			    (meth == CB_WM_KLE && p->key.u64 <= key_u64) ||
			    (meth == CB_WM_KLT && p->key.u64 <  key_u64))
				return &p->node;
		}
		else if (key_type == CB_KT_MB) {
			int diff;

			if ((uint64_t)plen / 8 == key_u64)
				diff = 0;
			else
				diff = memcmp(p->key.mb + plen / 8, key_ptr + plen / 8, key_u64 - plen / 8);

			if ((meth == CB_WM_KEQ && diff == 0) ||
			    (meth == CB_WM_KNX && diff == 0) ||
			    (meth == CB_WM_KPR && diff == 0) ||
			    (meth == CB_WM_KGE && diff >= 0) ||
			    (meth == CB_WM_KGT && diff >  0) ||
			    (meth == CB_WM_KLE && diff <= 0) ||
			    (meth == CB_WM_KLT && diff <  0))
				return &p->node;
		}
		else if (key_type == CB_KT_IM) {
			int diff;

			if ((uint64_t)plen / 8 == key_u64)
				diff = 0;
			else
				diff = memcmp(p->key.ptr + plen / 8, key_ptr + plen / 8, key_u64 - plen / 8);

			if ((meth == CB_WM_KEQ && diff == 0) ||
			    (meth == CB_WM_KNX && diff == 0) ||
			    (meth == CB_WM_KPR && diff == 0) ||
			    (meth == CB_WM_KGE && diff >= 0) ||
			    (meth == CB_WM_KGT && diff >  0) ||
			    (meth == CB_WM_KLE && diff <= 0) ||
			    (meth == CB_WM_KLT && diff <  0))
				return &p->node;
		}
		else if (key_type == CB_KT_ST) {
			int diff;

			if (found)
				diff = 0;
			else
				diff = strcmp((const void *)p->key.str + plen / 8, key_ptr + plen / 8);

			if ((meth == CB_WM_KEQ && diff == 0) ||
			    (meth == CB_WM_KNX && diff == 0) ||
			    (meth == CB_WM_KPR && diff == 0) ||
			    (meth == CB_WM_KGE && diff >= 0) ||
			    (meth == CB_WM_KGT && diff >  0) ||
			    (meth == CB_WM_KLE && diff <= 0) ||
			    (meth == CB_WM_KLT && diff <  0))
				return &p->node;
		}
		else if (key_type == CB_KT_IS) {
			int diff;

			if (found)
				diff = 0;
			else
				diff = strcmp((const void *)p->key.ptr + plen / 8, key_ptr + plen / 8);

			if ((meth == CB_WM_KEQ && diff == 0) ||
			    (meth == CB_WM_KNX && diff == 0) ||
			    (meth == CB_WM_KPR && diff == 0) ||
			    (meth == CB_WM_KGE && diff >= 0) ||
			    (meth == CB_WM_KGT && diff >  0) ||
			    (meth == CB_WM_KLE && diff <= 0) ||
			    (meth == CB_WM_KLT && diff <  0))
				return &p->node;
		}
		else if (key_type == CB_KT_ADDR) {
			if ((meth == CB_WM_KEQ && (uintptr_t)p == (uintptr_t)key_ptr) ||
			    (meth == CB_WM_KNX && (uintptr_t)p == (uintptr_t)key_ptr) ||
			    (meth == CB_WM_KPR && (uintptr_t)p == (uintptr_t)key_ptr) ||
			    (meth == CB_WM_KGE && (uintptr_t)p >= (uintptr_t)key_ptr) ||
			    (meth == CB_WM_KGT && (uintptr_t)p >  (uintptr_t)key_ptr) ||
			    (meth == CB_WM_KLE && (uintptr_t)p <= (uintptr_t)key_ptr) ||
			    (meth == CB_WM_KLT && (uintptr_t)p <  (uintptr_t)key_ptr))
				return &p->node;
		}
	} else if (meth == CB_WM_FST || meth == CB_WM_LST) {
		return &p->node;
	} else if (meth == CB_WM_PRV || meth == CB_WM_NXT) {
		return &p->node;
	}

	/* lookups and deletes fail here */

	/* let's return NULL to indicate the key was not found. For a lookup or
	 * a delete, it's a failure. For an insert, it's an invitation to the
	 * caller to proceed since the element is not there.
	 */
	return NULL;
}


/* Generic tree insertion function for trees with unique keys. Inserts node
 * <node> into tree <tree>, with key type <key_type> and key <key_*>.
 * Returns the inserted node or the one that already contains the same key.
 */
static inline __attribute__((always_inline))
struct cb_node *_cbu_insert(struct cb_node **root,
			    struct cb_node *node,
			    enum cb_key_type key_type,
			    uint32_t key_u32,
			    uint64_t key_u64,
			    const void *key_ptr)
{
	struct cb_node **parent;
	struct cb_node *ret;
	int nside;

	if (!*root) {
		/* empty tree, insert a leaf only */
		node->b[0] = node->b[1] = node;
		*root = node;
		return node;
	}

	ret = _cbu_descend(root, CB_WM_KEQ, key_type, key_u32, key_u64, key_ptr, &nside, &parent, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	if (!ret) {
		/* The key was not in the tree, we can insert it. Better use an
		 * "if" like this because the inline function above already has
		 * quite identifiable code paths. This reduces the code and
		 * optimizes it a bit.
		 */
		if (nside) {
			node->b[1] = node;
			node->b[0] = *parent;
		} else {
			node->b[0] = node;
			node->b[1] = *parent;
		}
		*parent = node;
		ret = node;
	}
	return ret;
}

/* Returns the first node or NULL if not found, assuming a tree made of keys of
 * type <key_type>.
 */
static inline __attribute__((always_inline))
struct cb_node *_cbu_first(struct cb_node **root,
			   enum cb_key_type key_type)
{
	if (!*root)
		return NULL;

	return _cbu_descend(root, CB_WM_FST, key_type, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* Returns the last node or NULL if not found, assuming a tree made of keys of
 * type <key_type>.
 */
static inline __attribute__((always_inline))
struct cb_node *_cbu_last(struct cb_node **root,
			  enum cb_key_type key_type)
{
	if (!*root)
		return NULL;

	return _cbu_descend(root, CB_WM_LST, key_type, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* Searches in the tree <root> made of keys of type <key_type>, for the next
 * node after the one containing the key <key_*>. Returns NULL if not found.
 * It's up to the caller to pass the current node's key in <key_*>. The
 * approach consists in looking up that node first, recalling the last time a
 * left turn was made, and returning the first node along the right branch at
 * that fork.
 */
static inline __attribute__((always_inline))
struct cb_node *_cbu_next(struct cb_node **root,
			  enum cb_key_type key_type,
			  uint32_t key_u32,
			  uint64_t key_u64,
			  const void *key_ptr)
{
	struct cb_node *restart;

	if (!*root)
		return NULL;

	if (!_cbu_descend(root, CB_WM_KNX, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &restart))
		return NULL;

	if (!restart)
		return NULL;

	return _cbu_descend(&restart, CB_WM_NXT, key_type, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* Searches in the tree <root> made of keys of type <key_type>, for the prev
 * node before the one containing the key <key_*>. Returns NULL if not found.
 * It's up to the caller to pass the current node's key in <key_*>. The
 * approach consists in looking up that node first, recalling the last time a
 * right turn was made, and returning the last node along the left branch at
 * that fork.
 */
static inline __attribute__((always_inline))
struct cb_node *_cbu_prev(struct cb_node **root,
			  enum cb_key_type key_type,
			  uint32_t key_u32,
			  uint64_t key_u64,
			  const void *key_ptr)
{
	struct cb_node *restart;

	if (!*root)
		return NULL;

	if (!_cbu_descend(root, CB_WM_KPR, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &restart))
		return NULL;

	if (!restart)
		return NULL;

	return _cbu_descend(&restart, CB_WM_PRV, key_type, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* Searches in the tree <root> made of keys of type <key_type>, for the node
 * containing the key <key_*>. Returns NULL if not found.
 */
static inline __attribute__((always_inline))
struct cb_node *_cbu_lookup(struct cb_node **root,
			    enum cb_key_type key_type,
			    uint32_t key_u32,
			    uint64_t key_u64,
			    const void *key_ptr)
{
	if (!*root)
		return NULL;

	return _cbu_descend(root, CB_WM_KEQ, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* Searches in the tree <root> made of keys of type <key_type>, for the node
 * containing the key <key_*> or the highest one that's lower than it. Returns
 * NULL if not found.
 */
static inline __attribute__((always_inline))
struct cb_node *_cbu_lookup_le(struct cb_node **root,
			       enum cb_key_type key_type,
			       uint32_t key_u32,
			       uint64_t key_u64,
			       const void *key_ptr)
{
	struct cb_node *ret = NULL;
	struct cb_node *restart;

	if (!*root)
		return NULL;

	ret = _cbu_descend(root, CB_WM_KLE, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &restart);
	if (ret)
		return ret;

	if (!restart)
		return NULL;

	return _cbu_descend(&restart, CB_WM_PRV, key_type, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* Searches in the tree <root> made of keys of type <key_type>, for the node
 * containing the greatest key that is strictly lower than <key_*>. Returns
 * NULL if not found. It's very similar to next() except that the looked up
 * value doesn't need to exist.
 */
static inline __attribute__((always_inline))
struct cb_node *_cbu_lookup_lt(struct cb_node **root,
			       enum cb_key_type key_type,
			       uint32_t key_u32,
			       uint64_t key_u64,
			       const void *key_ptr)
{
	struct cb_node *ret = NULL;
	struct cb_node *restart;

	if (!*root)
		return NULL;

	ret = _cbu_descend(root, CB_WM_KLT, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &restart);
	if (ret)
		return ret;

	if (!restart)
		return NULL;

	return _cbu_descend(&restart, CB_WM_PRV, key_type, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* Searches in the tree <root> made of keys of type <key_type>, for the node
 * containing the key <key_*> or the smallest one that's greater than it.
 * Returns NULL if not found.
 */
static inline __attribute__((always_inline))
struct cb_node *_cbu_lookup_ge(struct cb_node **root,
			       enum cb_key_type key_type,
			       uint32_t key_u32,
			       uint64_t key_u64,
			       const void *key_ptr)
{
	struct cb_node *ret = NULL;
	struct cb_node *restart;

	if (!*root)
		return NULL;

	ret = _cbu_descend(root, CB_WM_KGE, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &restart);
	if (ret)
		return ret;

	if (!restart)
		return NULL;

	return _cbu_descend(&restart, CB_WM_NXT, key_type, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* Searches in the tree <root> made of keys of type <key_type>, for the node
 * containing the lowest key that is strictly greater than <key_*>. Returns
 * NULL if not found. It's very similar to prev() except that the looked up
 * value doesn't need to exist.
 */
static inline __attribute__((always_inline))
struct cb_node *_cbu_lookup_gt(struct cb_node **root,
			       enum cb_key_type key_type,
			       uint32_t key_u32,
			       uint64_t key_u64,
			       const void *key_ptr)
{
	struct cb_node *ret = NULL;
	struct cb_node *restart;

	if (!*root)
		return NULL;

	ret = _cbu_descend(root, CB_WM_KGT, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &restart);
	if (ret)
		return ret;

	if (!restart)
		return NULL;

	return _cbu_descend(&restart, CB_WM_NXT, key_type, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* Searches in the tree <root> made of keys of type <key_type>, for the node
 * that contains the key <key_*>, and deletes it. If <node> is non-NULL, a
 * check is performed and the node found is deleted only if it matches. The
 * found node is returned in any case, otherwise NULL if not found. A deleted
 * node is detected since it has b[0]==NULL, which this functions also clears
 * after operation. The function is idempotent, so it's safe to attempt to
 * delete an already deleted node (NULL is returned in this case since the node
 * was not in the tree).
 */
static inline __attribute__((always_inline))
struct cb_node *_cbu_delete(struct cb_node **root,
			    struct cb_node *node,
			    enum cb_key_type key_type,
			    uint32_t key_u32,
			    uint64_t key_u64,
			    const void *key_ptr)
{
	struct cb_node *lparent, *nparent, *gparent;
	int lpside, npside, gpside;
	struct cb_node *ret = NULL;

	if (node && !node->b[0]) {
		/* NULL on a branch means the node is not in the tree */
		return NULL;
	}

	if (!*root) {
		/* empty tree, the node cannot be there */
		goto done;
	}

	ret = _cbu_descend(root, CB_WM_KEQ, key_type, key_u32, key_u64, key_ptr, NULL, NULL,
			    &lparent, &lpside, &nparent, &npside, &gparent, &gpside, NULL);

	if (!ret) {
		/* key not found */
		goto done;
	}

	if (ret == node || !node) {
		if (&lparent->b[0] == root) {
			/* there was a single entry, this one, so we're just
			 * deleting the nodeless leaf.
			 */
			*root = NULL;
			goto mark_and_leave;
		}

		/* then we necessarily have a gparent */
		gparent->b[gpside] = lparent->b[!lpside];

		if (lparent == ret) {
			/* we're removing the leaf and node together, nothing
			 * more to do.
			 */
			goto mark_and_leave;
		}

		if (ret->b[0] == ret->b[1]) {
			/* we're removing the node-less item, the parent will
			 * take this role.
			 */
			lparent->b[0] = lparent->b[1] = lparent;
			goto mark_and_leave;
		}

		/* more complicated, the node was split from the leaf, we have
		 * to find a spare one to switch it. The parent node is not
		 * needed anymore so we can reuse it.
		 */
		lparent->b[0] = ret->b[0];
		lparent->b[1] = ret->b[1];
		nparent->b[npside] = lparent;

	mark_and_leave:
		/* now mark the node as deleted */
		ret->b[0] = NULL;
	}
done:
	return ret;
}


#endif /* _CBTREE_PRV_H */
