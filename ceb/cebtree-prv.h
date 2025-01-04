/*
 * Compact Elastic Binary Trees - internal functions and types
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

#ifndef _CEBTREE_PRV_H
#define _CEBTREE_PRV_H

#include <inttypes.h>
#include <string.h>
#include "cebtree.h"

/* If DEBUG is set, we'll print additional debugging info during the descent */
#ifdef DEBUG
#define CEBDBG(x, ...) fprintf(stderr, x, ##__VA_ARGS__)
#else
#define CEBDBG(x, ...) do { } while (0)
#endif

/* These macros are used by upper level files to create two variants of their
 * exported functions:
 *   - one which uses sizeof(struct ceb_node) as the key offset, for nodes with
 *     adjacent keys ; these ones are named <pfx><sfx>(root, ...)
 *   - one with an explicit key offset passed by the caller right after the
 *     root.
 * Both rely on a forced inline version with a body that immediately follows
 * the declaration, so that the declaration looks like a single decorated
 * function while 2 are built in practice. There are variants for the basic one
 * with 0, 1 and 2 extra arguments after the root. The root and the key offset
 * are always the first two arguments, and the key offset never appears in the
 * first variant, it's always replaced by sizeof(struct ceb_node) in the calls
 * to the inline version.
 */
#define CEB_FDECL2(type, pfx, sfx, type1, arg1, type2, arg2)		\
	static inline __attribute__((always_inline))			\
	type _##pfx##sfx(type1 arg1, type2 arg2);			\
	type pfx##sfx(type1 arg1) {					\
		return _##pfx##sfx(arg1, sizeof(struct ceb_node));	\
	}								\
	type pfx##_ofs##sfx(type1 arg1, type2 arg2) {			\
		return _##pfx##sfx(arg1, arg2);				\
	}								\
	static inline __attribute__((always_inline))			\
	type _##pfx##sfx(type1 arg1, type2 arg2)
	/* function body follows */

#define CEB_FDECL3(type, pfx, sfx, type1, arg1, type2, arg2, type3, arg3) \
	static inline __attribute__((always_inline))			\
	type _##pfx##sfx(type1 arg1, type2 arg2, type3 arg3);		\
	type pfx##sfx(type1 arg1, type3 arg3) {				\
		return _##pfx##sfx(arg1, sizeof(struct ceb_node), arg3); \
	}								\
	type pfx##_ofs##sfx(type1 arg1, type2 arg2, type3 arg3) {	\
		return _##pfx##sfx(arg1, arg2, arg3);			\
	}								\
	static inline __attribute__((always_inline))			\
	type _##pfx##sfx(type1 arg1, type2 arg2, type3 arg3)
	/* function body follows */

#define CEB_FDECL4(type, pfx, sfx, type1, arg1, type2, arg2, type3, arg3, type4, arg4) \
	static inline __attribute__((always_inline))			\
	type _##pfx##sfx(type1 arg1, type2 arg2, type3 arg3, type4 arg4); \
	type pfx##sfx(type1 arg1, type3 arg3, type4 arg4) {		\
		return _##pfx##sfx(arg1, sizeof(struct ceb_node), arg3, arg4); \
	}								\
	type pfx##_ofs##sfx(type1 arg1, type2 arg2, type3 arg3, type4 arg4) { \
		return _##pfx##sfx(arg1, arg2, arg3, arg4);		\
	}								\
	static inline __attribute__((always_inline))			\
	type _##pfx##sfx(type1 arg1, type2 arg2, type3 arg3, type4 arg4)
	/* function body follows */

/* tree walk method: key, left, right */
enum ceb_walk_meth {
	CEB_WM_FST,     /* look up "first" (walk left only) */
	CEB_WM_NXT,     /* look up "next" (walk right once then left) */
	CEB_WM_PRV,     /* look up "prev" (walk left once then right) */
	CEB_WM_LST,     /* look up "last" (walk right only) */
	/* all methods from CEB_WM_KEQ and above do have a key */
	CEB_WM_KEQ,     /* look up the node equal to the key  */
	CEB_WM_KGE,     /* look up the node greater than or equal to the key */
	CEB_WM_KGT,     /* look up the node greater than the key */
	CEB_WM_KLE,     /* look up the node lower than or equal to the key */
	CEB_WM_KLT,     /* look up the node lower than the key */
	CEB_WM_KNX,     /* look up the node's key first, then find the next */
	CEB_WM_KPR,     /* look up the node's key first, then find the prev */
};

enum ceb_key_type {
	CEB_KT_ADDR,    /* the key is the node's address */
	CEB_KT_U32,     /* 32-bit unsigned word in key_u32 */
	CEB_KT_U64,     /* 64-bit unsigned word in key_u64 */
	CEB_KT_MB,      /* fixed size memory block in (key_u64,key_ptr), direct storage */
	CEB_KT_IM,      /* fixed size memory block in (key_u64,key_ptr), indirect storage */
	CEB_KT_ST,      /* NUL-terminated string in key_ptr, direct storage */
	CEB_KT_IS,      /* NUL-terminated string in key_ptr, indirect storage */
};

union ceb_key_storage {
	uint32_t u32;
	uint64_t u64;
	unsigned long ul;
	unsigned char mb[0];
	unsigned char str[0];
	unsigned char *ptr; /* for CEB_KT_IS */
};

/* returns the ceb_key_storage pointer for node <n> and offset <o> */
#define NODEK(n, o) ((union ceb_key_storage*)(((char *)(n)) + (o)))

/* Returns the xor (or common length) between the two sides <l> and <r> if both
 * are non-null, otherwise between the first non-null one and the value in the
 * associate key. As a reminder, memory blocks place their length in key_u64.
 * This is only intended for internal use, essentially for debugging.
 *
 * <kofs> contains the offset between the key and the node's base. When simply
 * adjacent, this would just be sizeof(ceb_node).
 */
__attribute__((unused))
static inline uint64_t _xor_branches(ptrdiff_t kofs, enum ceb_key_type key_type, uint32_t key_u32,
                                     uint64_t key_u64, const void *key_ptr,
                                     const struct ceb_node *l,
                                     const struct ceb_node *r)
{
	if (l && r) {
		if (key_type == CEB_KT_MB)
			return equal_bits(NODEK(l, kofs)->mb, NODEK(r, kofs)->mb, 0, key_u64 << 3);
		else if (key_type == CEB_KT_IM)
			return equal_bits(NODEK(l, kofs)->mb, NODEK(r, kofs)->ptr, 0, key_u64 << 3);
		else if (key_type == CEB_KT_ST)
			return string_equal_bits(NODEK(l, kofs)->str, NODEK(r, kofs)->str, 0);
		else if (key_type == CEB_KT_IS)
			return string_equal_bits(NODEK(l, kofs)->ptr, NODEK(r, kofs)->ptr, 0);
		else if (key_type == CEB_KT_U64)
			return NODEK(l, kofs)->u64 ^ NODEK(r, kofs)->u64;
		else if (key_type == CEB_KT_U32)
			return NODEK(l, kofs)->u32 ^ NODEK(r, kofs)->u32;
		else if (key_type == CEB_KT_ADDR)
			return ((uintptr_t)l ^ (uintptr_t)r);
		else
			return 0;
	}

	if (!l)
		l = r;

	if (key_type == CEB_KT_MB)
		return equal_bits(key_ptr, NODEK(l, kofs)->mb, 0, key_u64 << 3);
	else if (key_type == CEB_KT_IM)
		return equal_bits(key_ptr, NODEK(l, kofs)->ptr, 0, key_u64 << 3);
	else if (key_type == CEB_KT_ST)
		return string_equal_bits(key_ptr, NODEK(l, kofs)->str, 0);
	else if (key_type == CEB_KT_IS)
		return string_equal_bits(key_ptr, NODEK(l, kofs)->ptr, 0);
	else if (key_type == CEB_KT_U64)
		return key_u64 ^ NODEK(l, kofs)->u64;
	else if (key_type == CEB_KT_U32)
		return key_u32 ^ NODEK(l, kofs)->u32;
	else if (key_type == CEB_KT_ADDR)
		return ((uintptr_t)key_ptr ^ (uintptr_t)r);
	else
		return 0;
}

#ifdef DEBUG
__attribute__((unused))
static void dbg(int line,
                const char *pfx,
                enum ceb_walk_meth meth,
                ptrdiff_t kofs,
                enum ceb_key_type key_type,
                struct ceb_node * const *root,
                const struct ceb_node *p,
                uint32_t key_u32,
                uint64_t key_u64,
                const void *key_ptr,
                uint32_t px32,
                uint64_t px64,
                size_t plen)
{
	const char *meths[] = {
		[CEB_WM_FST] = "FST",
		[CEB_WM_NXT] = "NXT",
		[CEB_WM_PRV] = "PRV",
		[CEB_WM_LST] = "LST",
		[CEB_WM_KEQ] = "KEQ",
		[CEB_WM_KGE] = "KGE",
		[CEB_WM_KGT] = "KGT",
		[CEB_WM_KLE] = "KLE",
		[CEB_WM_KLT] = "KLT",
		[CEB_WM_KNX] = "KNX",
		[CEB_WM_KPR] = "KPR",
	};
	const char *ktypes[] = {
		[CEB_KT_ADDR] = "ADDR",
		[CEB_KT_U32]  = "U32",
		[CEB_KT_U64]  = "U64",
		[CEB_KT_MB]   = "MB",
		[CEB_KT_IM]   = "IM",
		[CEB_KT_ST]   = "ST",
		[CEB_KT_IS]   = "IS",
	};
	const char *kstr __attribute__((unused)) = ktypes[key_type];
	const char *mstr __attribute__((unused)) = meths[meth];
	long long nlen __attribute__((unused)) = 0;
	long long llen __attribute__((unused)) = 0;
	long long rlen __attribute__((unused)) = 0;
	long long xlen __attribute__((unused)) = 0;

	if (p)
		nlen = _xor_branches(kofs, key_type, key_u32, key_u64, key_ptr, p, NULL);

	if (p && p->b[0])
		llen = _xor_branches(kofs, key_type, key_u32, key_u64, key_ptr, p->b[0], NULL);

	if (p && p->b[1])
		rlen = _xor_branches(kofs, key_type, key_u32, key_u64, key_ptr, NULL, p->b[1]);

	if (p && p->b[0] && p->b[1])
		xlen = _xor_branches(kofs, key_type, key_u32, key_u64, key_ptr, p->b[0], p->b[1]);

	switch (key_type) {
	case CEB_KT_U32:
		CEBDBG("%04d (%8s) m=%s.%s key=%#x root=%p pxor=%#x p=%p,%#x(^%#llx) l=%p,%#x(^%#llx) r=%p,%#x(^%#llx) l^r=%#llx\n",
		      line, pfx, kstr, mstr, key_u32, root, px32,
		      p, p ? NODEK(p, kofs)->u32 : 0, nlen,
		      p ? p->b[0] : NULL, p ? NODEK(p->b[0], kofs)->u32 : 0, llen,
		      p ? p->b[1] : NULL, p ? NODEK(p->b[1], kofs)->u32 : 0, rlen,
		      xlen);
		break;
	case CEB_KT_U64:
		CEBDBG("%04d (%8s) m=%s.%s key=%#llx root=%p pxor=%#llx p=%p,%#llx(^%#llx) l=%p,%#llx(^%#llx) r=%p,%#llx(^%#llx) l^r=%#llx\n",
		      line, pfx, kstr, mstr, (long long)key_u64, root, (long long)px64,
		      p, (long long)(p ? NODEK(p, kofs)->u64 : 0), nlen,
		      p ? p->b[0] : NULL, (long long)(p ? NODEK(p->b[0], kofs)->u64 : 0), llen,
		      p ? p->b[1] : NULL, (long long)(p ? NODEK(p->b[1], kofs)->u64 : 0), rlen,
		      xlen);
		break;
	case CEB_KT_MB:
		CEBDBG("%04d (%8s) m=%s.%s key=%p root=%p plen=%ld p=%p,%p(^%llu) l=%p,%p(^%llu) r=%p,%p(^%llu) l^r=%llu\n",
		      line, pfx, kstr, mstr, key_ptr, root, (long)plen,
		      p, p ? NODEK(p, kofs)->mb : 0, nlen,
		      p ? p->b[0] : NULL, p ? NODEK(p->b[0], kofs)->mb : 0, llen,
		      p ? p->b[1] : NULL, p ? NODEK(p->b[1], kofs)->mb : 0, rlen,
		      xlen);
		break;
	case CEB_KT_IM:
		CEBDBG("%04d (%8s) m=%s.%s key=%p root=%p plen=%ld p=%p,%p(^%llu) l=%p,%p(^%llu) r=%p,%p(^%llu) l^r=%llu\n",
		      line, pfx, kstr, mstr, key_ptr, root, (long)plen,
		      p, p ? NODEK(p, kofs)->ptr : 0, nlen,
		      p ? p->b[0] : NULL, p ? NODEK(p->b[0], kofs)->ptr : 0, llen,
		      p ? p->b[1] : NULL, p ? NODEK(p->b[1], kofs)->ptr : 0, rlen,
		      xlen);
		break;
	case CEB_KT_ST:
		CEBDBG("%04d (%8s) m=%s.%s key='%s' root=%p plen=%ld p=%p,%s(^%llu) l=%p,%s(^%llu) r=%p,%s(^%llu) l^r=%llu\n",
		      line, pfx, kstr, mstr, key_ptr ? (const char *)key_ptr : "", root, (long)plen,
		      p, p ? (const char *)NODEK(p, kofs)->str : "-", nlen,
		      p ? p->b[0] : NULL, p ? (const char *)NODEK(p->b[0], kofs)->str : "-", llen,
		      p ? p->b[1] : NULL, p ? (const char *)NODEK(p->b[1], kofs)->str : "-", rlen,
		      xlen);
		break;
	case CEB_KT_IS:
		CEBDBG("%04d (%8s) m=%s.%s key='%s' root=%p plen=%ld p=%p,%s(^%llu) l=%p,%s(^%llu) r=%p,%s(^%llu) l^r=%llu\n",
		      line, pfx, kstr, mstr, key_ptr ? (const char *)key_ptr : "", root, (long)plen,
		      p, p ? (const char *)NODEK(p, kofs)->ptr : "-", nlen,
		      p ? p->b[0] : NULL, p ? (const char *)NODEK(p->b[0], kofs)->ptr : "-", llen,
		      p ? p->b[1] : NULL, p ? (const char *)NODEK(p->b[1], kofs)->ptr : "-", rlen,
		      xlen);
		break;
	case CEB_KT_ADDR:
		/* key type is the node's address */
		CEBDBG("%04d (%8s) m=%s.%s key=%#llx root=%p pxor=%#llx p=%p,%#llx(^%#llx) l=%p,%#llx(^%#llx) r=%p,%#llx(^%#llx) l^r=%#llx\n",
		      line, pfx, kstr, mstr, (long long)(uintptr_t)key_ptr, root, (long long)px64,
		      p, (long long)(uintptr_t)p, nlen,
		      p ? p->b[0] : NULL, p ? (long long)(uintptr_t)p->b[0] : 0, llen,
		      p ? p->b[1] : NULL, p ? (long long)(uintptr_t)p->b[1] : 0, rlen,
		      xlen);
	}
}
#else
#define dbg(...) do { } while (0)
#endif

#endif /* _CEBTREE_PRV_H */
