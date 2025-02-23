/*
 * Compact Elastic Binary Trees - internal functions and types
 *
 * Copyright (C) 2014-2025 Willy Tarreau - w@1wt.eu
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

#include <sys/types.h>
#include <inttypes.h>
#include <stddef.h>
#include <string.h>
#include "cebtree.h"

/* If DEBUG is set, we'll print additional debugging info during the descent */
#ifdef DEBUG
#define CEBDBG(x, ...) fprintf(stderr, x, ##__VA_ARGS__)
#else
#define CEBDBG(x, ...) do { } while (0)
#endif

/* A few utility functions and macros that we need below */

/* Define the missing __builtin_prefetch() for tcc. */
#if defined(__TINYC__) && !defined(__builtin_prefetch)
#define __builtin_prefetch(addr, ...) do { } while (0)
#endif

/* Vector-based string comparison can be disabled by setting CEB_NO_VECTOR. We
 * can automatically detect -fsanitize=address on gcc and clang so we disable
 * it as well in this case.
 */
#if !defined(CEB_NO_VECTOR)
# if defined(__SANITIZE_ADDRESS__) // gcc
#  define CEB_NO_VECTOR 1
# elif defined(__has_feature)
#  if __has_feature(address_sanitizer) // clang
#   define CEB_NO_VECTOR 1
#  endif
# endif
#endif

/* FLSNZ: find last set bit for non-zero value. "Last" here means the highest
 * one. It returns a value from 1 to 32 for 1<<0 to 1<<31.
 */

#if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 2)))
/* gcc >= 4.2 brings __builtin_clz() and __builtin_clzl(), also usable for
 * non-x86. However on x86 gcc does bad stuff if not properly handled. It xors
 * the bsr return with 31 and since it doesn't know how to deal with a xor
 * followed by a negation, it adds two instructions when using 32-clz(). Thus
 * instead we first cancel the xor using another one then add one. Even on ARM
 * that provides a clz instruction, it saves one register to proceed like this.
 */

#define flsnz8(x) flsnz32((unsigned char)x)

static inline __attribute__((always_inline)) unsigned int flsnz32(unsigned int x)
{
	return (__builtin_clz(x) ^ 31) + 1;
}

static inline __attribute__((always_inline)) unsigned int flsnz64(unsigned long long x)
{
	return (__builtin_clzll(x) ^ 63) + 1;
}

#elif (defined(__i386__) || defined(__x86_64__)) && !defined(__atom__) /* Not gcc >= 4.2 */
/* DO NOT USE ON ATOM! The instruction is emulated and is several times slower
 * than doing the math by hand.
 */
#define flsnz8(x) flsnz32((unsigned char)x)

static inline __attribute__((always_inline)) unsigned int flsnz32(unsigned int x)
{
	unsigned int r;
	__asm__("bsrl %1,%0\n"
	        : "=r" (r) : "rm" (x));
	return r + 1;
}

#if defined(__x86_64__)
static inline __attribute__((always_inline)) unsigned int flsnz64(unsigned long long x)
{
	unsigned long long r;
	__asm__("bsrq %1,%0\n"
	        : "=r" (r) : "rm" (x));
	return r + 1;
}
#endif

#else /* Neither gcc >= 4.2 nor x86, use generic code */

static inline __attribute__((always_inline)) unsigned int flsnz8(unsigned int x)
{
	unsigned int ret = 0;
	if (x >> 4) { x >>= 4; ret += 4; }
	return ret + ((0xFFFFAA50U >> (x << 1)) & 3) + 1;
}

#define flsnz32(___a) ({ \
	register unsigned int ___x, ___bits = 0; \
	___x = (___a); \
	if (___x & 0xffff0000) { ___x &= 0xffff0000; ___bits += 16;} \
	if (___x & 0xff00ff00) { ___x &= 0xff00ff00; ___bits +=  8;} \
	if (___x & 0xf0f0f0f0) { ___x &= 0xf0f0f0f0; ___bits +=  4;} \
	if (___x & 0xcccccccc) { ___x &= 0xcccccccc; ___bits +=  2;} \
	if (___x & 0xaaaaaaaa) { ___x &= 0xaaaaaaaa; ___bits +=  1;} \
	___bits + 1; \
	})

static inline __attribute__((always_inline)) unsigned int flsnz64(unsigned long long x)
{
	unsigned int h;
	unsigned int bits = 32;

	h = x >> 32;
	if (!h) {
		h = x;
		bits = 0;
	}
	return flsnz32(h) + bits;
}

#endif

#define flsnz_long(x) ((sizeof(long) > 4) ? flsnz64(x) : flsnz32(x))
#define flsnz(x) ((sizeof(x) > 4) ? flsnz64(x) : (sizeof(x) > 1) ? flsnz32(x) : flsnz8(x))

/* Compare blocks <a> and <b> byte-to-byte, from bit <ignore> to bit <len-1>.
 * Return the number of equal bits between strings, assuming that the first
 * <ignore> bits are already identical. It is possible to return slightly more
 * than <len> bits if <len> does not stop on a byte boundary and we find exact
 * bytes. Note that parts or all of <ignore> bits may be rechecked. It is only
 * passed here as a hint to speed up the check.
 */
static inline __attribute__((always_inline))
size_t equal_bits(const unsigned char *a,
                  const unsigned char *b,
                  size_t ignore, size_t len)
{
	for (ignore >>= 3, a += ignore, b += ignore, ignore <<= 3;
	     ignore < len; ) {
		unsigned char c;

		a++; b++;
		ignore += 8;
		c = b[-1] ^ a[-1];

		if (c) {
			/* OK now we know that old and new differ at byte <ptr> and that <c> holds
			 * the bit differences. We have to find what bit is differing and report
			 * it as the number of identical bits. Note that low bit numbers are
			 * assigned to high positions in the byte, as we compare them as strings.
			 */
			ignore -= flsnz_long(c);
			break;
		}
	}
	return ignore;
}

/* Compare strings <a> and <b> byte-to-byte, from byte <ofs> to the last 0.
 * Return the number of equal bits between strings, assuming that the first
 * <ofs> byts are already identical. The caller is responsible for not passing
 * an <ofs> value larger than any of the two strings. However, referencing the
 * trailing zero is permitted. Equal strings are reported as a negative number
 * of bits, which indicates the end was reached.
 */
static inline __attribute__((always_inline))
size_t _string_equal_bits_by1(const unsigned char *a,
                              const unsigned char *b,
                              size_t ofs)
{
	unsigned char c, d;

	/* skip known and identical bytes. We stop at the first different byte
	 * or at the first zero we encounter on either side.
	 */
	while (1) {
		c = a[ofs];
		d = b[ofs];
		ofs++;

		c ^= d;
		if (c)
			break;
		if (!d)
			return (size_t)-1;
	}

	/* OK now we know that a and b differ at byte <ofs>, or that both are zero.
	 * We have to find what bit is differing and report it as the number of
	 * identical bits. Note that low bit numbers are assigned to high positions
	 * in the byte, as we compare them as strings.
	 */
	return (ofs << 3) - flsnz8(c);
}

/* Compare strings <a> and <b>, from bit <ignore> to the last 0. Depending on
 * build options and optimizations, it may possibly read past the last zero.
 * Return the number of equal bits between strings, assuming that the first
 * <ignore> bits are already identical. Note that parts or all of <ignore> bits
 * may be rechecked. It is only passed here as a hint to speed up the check.
 * The caller is responsible for not passing an <ignore> value larger than any
 * of the two strings. However, referencing any bit from the trailing zero is
 * permitted. Equal strings are reported as a negative number of bits, which
 * indicates the end was reached.
 */
static inline //__attribute__((always_inline))
size_t string_equal_bits(const unsigned char *a,
                         const unsigned char *b,
                         size_t ignore)
{
	size_t ofs = ignore >> 3;

	/* we support a speedup for unaligned accesses on little endian. This
	 * involves reading vectors and implies we will often read past the
	 * trailing zero. There's no problem doing this before the end of a
	 * page, but ASAN and Valgrind won't like it, reason why we disable
	 * this when using ASAN and we can explicitly disable it by defining
	 * CEB_NO_VECTOR. In order to measure the bit lengths, we'll need
	 * __builtin_bswap which requires gcc >= 4.3.
	 */
#if !defined(CEB_NO_VECTOR) &&						\
	((defined(__clang__)) ||                                        \
	 (defined(__GNUC__) &&						\
	  ((__GNUC__ >= 5) ||						\
	   ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 3))))) &&		\
	(defined(__x86_64__) ||                                         \
	 (defined(__ARM_FEATURE_UNALIGNED) &&				\
	  defined(__ARMEL__) && defined(__ARM_ARCH_7A__)) ||		\
	 (defined(__AARCH64EL__) &&					\
	  (defined (__aarch64__) || defined(__ARM_ARCH_8A))))

	uintptr_t ofsa, ofsb, max_words;
	unsigned long l, r;
	unsigned int xbit;

	/* This block reads one unaligned word at a time till the next page
	 * boundary. Then it goes on one byte at a time with the fallback code.
	 * Calculating the exact number is expensive, because the number of
	 * readable words (for a 64-bit machine) is defined by:
	 *    0x1000 - ((((addr + ofs) & 0xfff) + 7) & 0x1ff8)
	 * However this calculation is expensive, and this much cheaper
	 * simplification only sacrifices the last 8 bytes of a page:
	 *   (addr + ofs) & 0xff8) ^ 0xff8
	 * The difference is about 5% of the code size for the strings code,
	 * and the shorter form is actually significantly faster since on a
	 * critical path for small strings.
	 */
//	ofsa  = (uintptr_t)(a + ofs) & (0x1000 - sizeof(long));
//	ofsb  = (uintptr_t)(b + ofs) & (0x1000 - sizeof(long));
//	ofsa ^= (0x1000 - sizeof(long));
//	ofsb ^= (0x1000 - sizeof(long));
//	max_words = (ofsa < ofsb ? ofsa : ofsb) / sizeof(long);
//
//	max_words += ofs;

	/* count how many bytes exceed a page */
	ofsa  = (uintptr_t)(a + ofs + 0x1000 - sizeof(long));
	ofsb  = (uintptr_t)(b + ofs + 0x1000 - sizeof(long));
	ofsa &= 0xfff; // how many bytes too much
	ofsb &= 0xfff; // how many bytes too much

	//max_words  = (ofsa > ofsb ? ofsa : ofsb);

	//max_words  = (ofsa > ofsb ? ofsa : ofsb) ^ 0xfff;

	ofsa ^= 0xfff; // how many bytes too much
	ofsb ^= 0xfff; // how many bytes too much
	max_words = (ofsa < ofsb ? ofsa : ofsb);

	max_words += ofs;
	while (1) {
		if (ofs >= max_words/*max_words < 8*/)
			goto by_one;

		l = *(unsigned long *)(a + ofs);
		r = *(unsigned long *)(b + ofs);
		//max_words -= 8;
		ofs += sizeof(long);

		r = l ^ r;

		/* check for the presence of a zero byte in one of the strings */
		l = ((l - (unsigned long)0x0101010101010101ULL) &
		     ~l & (unsigned long)0x8080808080808080ULL);

		/* stop if there is one zero or if some bits differ */
		if (__builtin_expect(l || r, 0))
			break;
		/* OK, all 64 bits are equal, continue */
	}

	/* let's figure the first different bit (highest) */
	r = (sizeof(long) > 4) ? __builtin_bswap64(r) : __builtin_bswap32(r);
	xbit = r ? flsnz(r) : 0;

	if (l) {
		/* there's at least a zero, let's find the first zero byte. It
		 * was replaced above with a 0x80 while all other ones are zero.
		 */
		l = (sizeof(long) > 4) ? __builtin_bswap64(l) : __builtin_bswap32(l);

		/* map it to the lowest bit of the byte, and verify if it's
		 * before the first difference, in which case the strings are
		 * equal.
		 */
		if (flsnz(l) - 7 > xbit)
			return -1;
	}

	/* return position of first difference */
	return (ofs << 3) - xbit;

	/* We know there is at least one zero or a difference so we'll stop.
	 * For the zero, there will be 0x80 instead of the NULL bytes. For
	 * the difference, we have at least one non-zero bit on the
	 * differences. If the zero byte is strictly before the difference,
	 * it means both words have the same zero byte, hence there is no
	 * diference. Otherwise there's a difference at the position indicated
	 * by the closest bit set in x from the beginning. In both cases, the
	 * positions closest to the first bytes count, so we'll turn the words
	 * to big endian. Note that comparing pure integer values does not work
	 * due to possible zeroes past the first NUL that would affect the
	 * comparison.
	 */
by_one:
#endif
	return _string_equal_bits_by1(a, b, ofs);
}


/* These macros are used by upper level files to create two variants of their
 * exported functions:
 *   - one which uses sizeof(struct ceb_node) as the key offset, for nodes with
 *     adjacent keys ; these ones are named <pfx><sfx>(root, ...). This is
 *     defined when CEB_USE_BASE is defined.
 *   - one with an explicit key offset passed by the caller right after the
 *     root. This is defined when CEB_USE_OFST is defined.
 * Both rely on a forced inline version with a body that immediately follows
 * the declaration, so that the declaration looks like a single decorated
 * function while 2 are built in practice. There are variants for the basic one
 * with 0, 1 and 2 extra arguments after the root. The root and the key offset
 * are always the first two arguments, and the key offset never appears in the
 * first variant, it's always replaced by sizeof(struct ceb_node) in the calls
 * to the inline version.
 */
#if defined(CEB_USE_BASE)
# define _CEB_DEF_BASE(x) x
#else
# define _CEB_DEF_BASE(x)
#endif

#if defined(CEB_USE_OFST)
# define _CEB_DEF_OFST(x) x
#else
# define _CEB_DEF_OFST(x)
#endif

#define CEB_FDECL2(type, pfx, sfx, type1, arg1, type2, arg2) \
	_CEB_FDECL2(type, pfx, sfx, type1, arg1, type2, arg2)

#define _CEB_FDECL2(type, pfx, sfx, type1, arg1, type2, arg2)		\
	static inline __attribute__((always_inline))			\
	type _##pfx##sfx(type1 arg1, type2 arg2);			\
	_CEB_DEF_BASE(type pfx##sfx(type1 arg1) {			\
		return _##pfx##sfx(arg1, sizeof(struct ceb_node));	\
	})								\
	_CEB_DEF_OFST(type pfx##_ofs##sfx(type1 arg1, type2 arg2) {	\
		return _##pfx##sfx(arg1, arg2);				\
	})								\
	static inline __attribute__((always_inline))			\
	type _##pfx##sfx(type1 arg1, type2 arg2)
	/* function body follows */

#define CEB_FDECL3(type, pfx, sfx, type1, arg1, type2, arg2, type3, arg3) \
	_CEB_FDECL3(type, pfx, sfx, type1, arg1, type2, arg2, type3, arg3)

#define _CEB_FDECL3(type, pfx, sfx, type1, arg1, type2, arg2, type3, arg3) \
	static inline __attribute__((always_inline))			\
	type _##pfx##sfx(type1 arg1, type2 arg2, type3 arg3);		\
	_CEB_DEF_BASE(type pfx##sfx(type1 arg1, type3 arg3) {		\
		return _##pfx##sfx(arg1, sizeof(struct ceb_node), arg3); \
	})								\
	_CEB_DEF_OFST(type pfx##_ofs##sfx(type1 arg1, type2 arg2, type3 arg3) {	\
		return _##pfx##sfx(arg1, arg2, arg3);			\
	})								\
	static inline __attribute__((always_inline))			\
	type _##pfx##sfx(type1 arg1, type2 arg2, type3 arg3)
	/* function body follows */

#define CEB_FDECL4(type, pfx, sfx, type1, arg1, type2, arg2, type3, arg3, type4, arg4) \
	_CEB_FDECL4(type, pfx, sfx, type1, arg1, type2, arg2, type3, arg3, type4, arg4)

#define _CEB_FDECL4(type, pfx, sfx, type1, arg1, type2, arg2, type3, arg3, type4, arg4) \
	static inline __attribute__((always_inline))			\
	type _##pfx##sfx(type1 arg1, type2 arg2, type3 arg3, type4 arg4); \
	_CEB_DEF_BASE(type pfx##sfx(type1 arg1, type3 arg3, type4 arg4) {	\
		return _##pfx##sfx(arg1, sizeof(struct ceb_node), arg3, arg4); \
	})								\
	_CEB_DEF_OFST(type pfx##_ofs##sfx(type1 arg1, type2 arg2, type3 arg3, type4 arg4) { \
		return _##pfx##sfx(arg1, arg2, arg3, arg4);		\
	})								\
	static inline __attribute__((always_inline))			\
	type _##pfx##sfx(type1 arg1, type2 arg2, type3 arg3, type4 arg4)
	/* function body follows */

#define CEB_FDECL5(type, pfx, sfx, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5) \
	_CEB_FDECL5(type, pfx, sfx, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5)

#define _CEB_FDECL5(type, pfx, sfx, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5) \
	static inline __attribute__((always_inline))			\
	type _##pfx##sfx(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5); \
	_CEB_DEF_BASE(type pfx##sfx(type1 arg1, type3 arg3, type4 arg4, type5 arg5) {	\
		return _##pfx##sfx(arg1, sizeof(struct ceb_node), arg3, arg4, arg5); \
	})										\
	_CEB_DEF_OFST(type pfx##_ofs##sfx(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5) { \
		return _##pfx##sfx(arg1, arg2, arg3, arg4, arg5);	\
	})								\
	static inline __attribute__((always_inline))			\
	type _##pfx##sfx(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5)
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

/* Returns the xor (or the complement of the common length for strings) between
 * the two sides <l> and <r> if both are non-null, otherwise between the first
 * non-null one and the value in the associate key. As a reminder, memory
 * blocks place their length in key_u64. This is only intended for internal
 * use, essentially for debugging. It only returns zero when the keys are
 * identical, and returns a greater value for keys that are more distant.
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
			return (key_u64 << 3) - equal_bits(NODEK(l, kofs)->mb, NODEK(r, kofs)->mb, 0, key_u64 << 3);
		else if (key_type == CEB_KT_IM)
			return (key_u64 << 3) - equal_bits(NODEK(l, kofs)->mb, NODEK(r, kofs)->ptr, 0, key_u64 << 3);
		else if (key_type == CEB_KT_ST)
			return ~string_equal_bits(NODEK(l, kofs)->str, NODEK(r, kofs)->str, 0);
		else if (key_type == CEB_KT_IS)
			return ~string_equal_bits(NODEK(l, kofs)->ptr, NODEK(r, kofs)->ptr, 0);
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
		return (key_u64 << 3) - equal_bits(key_ptr, NODEK(l, kofs)->mb, 0, key_u64 << 3);
	else if (key_type == CEB_KT_IM)
		return (key_u64 << 3) - equal_bits(key_ptr, NODEK(l, kofs)->ptr, 0, key_u64 << 3);
	else if (key_type == CEB_KT_ST)
		return ~string_equal_bits(key_ptr, NODEK(l, kofs)->str, 0);
	else if (key_type == CEB_KT_IS)
		return ~string_equal_bits(key_ptr, NODEK(l, kofs)->ptr, 0);
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

	if (meth >= CEB_WM_KEQ && p) {
		nlen = _xor_branches(kofs, key_type, key_u32, key_u64, key_ptr, p, NULL);

		if (p->b[0])
			llen = _xor_branches(kofs, key_type, key_u32, key_u64, key_ptr, p->b[0], NULL);

		if (p->b[1])
			rlen = _xor_branches(kofs, key_type, key_u32, key_u64, key_ptr, NULL, p->b[1]);

		if (p->b[0] && p->b[1])
			xlen = _xor_branches(kofs, key_type, key_u32, key_u64, key_ptr, p->b[0], p->b[1]);
	}

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
		CEBDBG("%04d (%8s) m=%s.%s key=%p root=%p plen=%ld p=%p,%p(^%lld) l=%p,%p(^%lld) r=%p,%p(^%lld) l^r=%lld\n",
		      line, pfx, kstr, mstr, key_ptr, root, (long)plen,
		      p, p ? NODEK(p, kofs)->mb : 0, (key_u64 << 3) - nlen,
		      p ? p->b[0] : NULL, p ? NODEK(p->b[0], kofs)->mb : 0, (key_u64 << 3) - llen,
		      p ? p->b[1] : NULL, p ? NODEK(p->b[1], kofs)->mb : 0, (key_u64 << 3) - rlen,
		      (key_u64 << 3) - xlen);
		break;
	case CEB_KT_IM:
		CEBDBG("%04d (%8s) m=%s.%s key=%p root=%p plen=%ld p=%p,%p(^%lld) l=%p,%p(^%lld) r=%p,%p(^%lld) l^r=%lld\n",
		      line, pfx, kstr, mstr, key_ptr, root, (long)plen,
		      p, p ? NODEK(p, kofs)->ptr : 0, (key_u64 << 3) - nlen,
		      p ? p->b[0] : NULL, p ? NODEK(p->b[0], kofs)->ptr : 0, (key_u64 << 3) - llen,
		      p ? p->b[1] : NULL, p ? NODEK(p->b[1], kofs)->ptr : 0, (key_u64 << 3) - rlen,
		      (key_u64 << 3) - xlen);
		break;
	case CEB_KT_ST:
		CEBDBG("%04d (%8s) m=%s.%s key='%s' root=%p plen=%ld p=%p,%s(^%lld) l=%p,%s(^%lld) r=%p,%s(^%lld) l^r=%lld\n",
		      line, pfx, kstr, mstr, key_ptr ? (const char *)key_ptr : "", root, (long)plen,
		      p, p ? (const char *)NODEK(p, kofs)->str : "-", ~nlen,
		      p ? p->b[0] : NULL, p ? (const char *)NODEK(p->b[0], kofs)->str : "-", ~llen,
		      p ? p->b[1] : NULL, p ? (const char *)NODEK(p->b[1], kofs)->str : "-", ~rlen,
		      ~xlen);
		break;
	case CEB_KT_IS:
		CEBDBG("%04d (%8s) m=%s.%s key='%s' root=%p plen=%ld p=%p,%s(^%lld) l=%p,%s(^%lld) r=%p,%s(^%lld) l^r=%lld\n",
		      line, pfx, kstr, mstr, key_ptr ? (const char *)key_ptr : "", root, (long)plen,
		      p, p ? (const char *)NODEK(p, kofs)->ptr : "-", ~nlen,
		      p ? p->b[0] : NULL, p ? (const char *)NODEK(p->b[0], kofs)->ptr : "-", ~llen,
		      p ? p->b[1] : NULL, p ? (const char *)NODEK(p->b[1], kofs)->ptr : "-", ~rlen,
		      ~xlen);
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
struct ceb_node *_ceb_descend(struct ceb_node **root,
                              enum ceb_walk_meth meth,
                              ptrdiff_t kofs,
                              enum ceb_key_type key_type,
                              uint32_t key_u32,
                              uint64_t key_u64,
                              const void *key_ptr,
                              int *ret_nside,
                              struct ceb_node ***ret_root,
                              struct ceb_node **ret_lparent,
                              int *ret_lpside,
                              struct ceb_node **ret_nparent,
                              int *ret_npside,
                              struct ceb_node **ret_gparent,
                              int *ret_gpside,
                              struct ceb_node **ret_back,
                              int *ret_is_dup)
{
#if defined(__GNUC__) && (__GNUC__ >= 12) && !defined(__OPTIMIZE__)
/* Avoid a bogus warning with gcc 12 and above: it warns about negative
 * memcmp() length in non-existing code paths at -O0, as reported here:
 *    https://gcc.gnu.org/bugzilla/show_bug.cgi?id=114622
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overread"
#endif
	struct ceb_node *p;
	union ceb_key_storage *l, *r, *k;
	struct ceb_node *gparent = NULL;
	struct ceb_node *nparent = NULL;
	struct ceb_node *bnode = NULL;
	struct ceb_node *lparent;
	uint32_t pxor32 = ~0U;   // previous xor between branches
	uint64_t pxor64 = ~0ULL; // previous xor between branches
	int gpside = 0;   // side on the grand parent
	int npside = 0;   // side on the node's parent
	long lpside = 0;  // side on the leaf's parent
	long brside = 0;  // branch side when descending
	size_t llen = 0;  // left vs key matching length
	size_t rlen = 0;  // right vs key matching length
	size_t plen = 0;  // previous common len between branches
	int is_dup = 0;   // returned key is a duplicate
	int found = 0;    // key was found (saves an extra strcmp for arrays)

	dbg(__LINE__, "_enter__", meth, kofs, key_type, root, NULL, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);

	/* the parent will be the (possibly virtual) node so that
	 * &lparent->l == root, i.e. container_of(root, struct ceb_node, b[0]).
	 */
	lparent = (struct ceb_node *)((char *)root - (long)&((struct ceb_node *)0)->b[0]);
	gparent = nparent = lparent;

	/* for key-less descents we need to set the initial branch to take */
	switch (meth) {
	case CEB_WM_NXT:
	case CEB_WM_LST:
		brside = 1; // start right for next/last
		break;
	case CEB_WM_FST:
	case CEB_WM_PRV:
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
		p = *root;

		/* Tests have shown that for write-intensive workloads (many
		 * insertions/deletion), prefetching for reads is counter
		 * productive (-10% perf) but that prefetching only the next
		 * nodes for writes when deleting can yield around 3% extra
		 * boost.
		 */
		if (ret_lpside) {
			/* this is a deletion, prefetch for writes */
			__builtin_prefetch(p->b[0], 1);
			__builtin_prefetch(p->b[1], 1);
		}

		/* neither pointer is tagged */
		k = NODEK(p, kofs);
		l = NODEK(p->b[0], kofs);
		r = NODEK(p->b[1], kofs);

		dbg(__LINE__, "newp", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);

		/* two equal pointers identifies either the nodeless leaf or
		 * the 2nd dup of a sub-tree. Both are leaves anyway, but we
		 * must not yet stop here for a dup as we may want to report
		 * it.
		 */
		if (l == r && r == k) {
			dbg(__LINE__, "l==r", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
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

		if (key_type == CEB_KT_U32) {
			uint32_t xor32;   // left vs right branch xor
			uint32_t kl, kr;

			kl = l->u32; kr = r->u32;
			xor32 = kl ^ kr;

			if (xor32 > pxor32) { // test using 2 4 6 4
				dbg(__LINE__, "xor>", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				break;
			}

			if (meth >= CEB_WM_KEQ) {
				/* "found" is not used here */
				kl ^= key_u32; kr ^= key_u32;
				brside = kl >= kr;

				/* let's stop if our key is not there */

				if (kl > xor32 && kr > xor32) {
					dbg(__LINE__, "mismatch", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
					break;
				}

				if (ret_npside || ret_nparent) {
					if (key_u32 == k->u32) {
						dbg(__LINE__, "equal", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
						nparent = lparent;
						npside  = lpside;
					}
				}
			}
			pxor32 = xor32;
			if (ret_is_dup && !xor32) {
				/* both sides are equal, that's a duplicate */
				dbg(__LINE__, "dup>", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				is_dup = 1;
				break;
			}
		}
		else if (key_type == CEB_KT_U64) {
			uint64_t xor64;   // left vs right branch xor
			uint64_t kl, kr;

			kl = l->u64; kr = r->u64;
			xor64 = kl ^ kr;

			if (xor64 > pxor64) { // test using 2 4 6 4
				dbg(__LINE__, "xor>", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				break;
			}

			if (meth >= CEB_WM_KEQ) {
				/* "found" is not used here */
				kl ^= key_u64; kr ^= key_u64;
				brside = kl >= kr;

				/* let's stop if our key is not there */

				if (kl > xor64 && kr > xor64) {
					dbg(__LINE__, "mismatch", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
					break;
				}

				if (ret_npside || ret_nparent) {
					if (key_u64 == k->u64) {
						dbg(__LINE__, "equal", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
						nparent = lparent;
						npside  = lpside;
					}
				}
			}
			pxor64 = xor64;
			if (ret_is_dup && !xor64) {
				/* both sides are equal, that's a duplicate */
				dbg(__LINE__, "dup>", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				is_dup = 1;
				break;
			}
		}
		else if (key_type == CEB_KT_MB) {
			size_t xlen = 0; // left vs right matching length

			if (meth >= CEB_WM_KEQ) {
				/* measure identical lengths */
				llen = equal_bits(key_ptr, l->mb, 0, key_u64 << 3);
				rlen = equal_bits(key_ptr, r->mb, 0, key_u64 << 3);
				brside = llen <= rlen;
				if (llen == rlen && (uint64_t)llen == key_u64 << 3)
					found = 1;
			}

			xlen = equal_bits(l->mb, r->mb, 0, key_u64 << 3);
			if (xlen < plen) {
				/* this is a leaf. E.g. triggered using 2 4 6 4 */
				dbg(__LINE__, "xor>", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				break;
			}

			if (meth >= CEB_WM_KEQ) {
				/* let's stop if our key is not there */

				if (llen < xlen && rlen < xlen) {
					dbg(__LINE__, "mismatch", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
					break;
				}

				if (ret_npside || ret_nparent) { // delete ?
					size_t mlen = llen > rlen ? llen : rlen;

					if (mlen > xlen)
						mlen = xlen;

					if ((uint64_t)xlen / 8 == key_u64 || memcmp(key_ptr + mlen / 8, k->mb + mlen / 8, key_u64 - mlen / 8) == 0) {
						dbg(__LINE__, "equal", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
						nparent = lparent;
						npside  = lpside;
						found = 1;
					}
				}
			}
			plen = xlen;
			if (ret_is_dup && (uint64_t)xlen / 8 == key_u64) {
				/* both sides are equal, that's a duplicate */
				dbg(__LINE__, "dup>", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				is_dup = 1;
				break;
			}
		}
		else if (key_type == CEB_KT_IM) {
			size_t xlen = 0; // left vs right matching length

			if (meth >= CEB_WM_KEQ) {
				/* measure identical lengths */
				llen = equal_bits(key_ptr, l->ptr, 0, key_u64 << 3);
				rlen = equal_bits(key_ptr, r->ptr, 0, key_u64 << 3);
				brside = llen <= rlen;
				if (llen == rlen && (uint64_t)llen == key_u64 << 3)
					found = 1;
			}

			xlen = equal_bits(l->ptr, r->ptr, 0, key_u64 << 3);
			if (xlen < plen) {
				/* this is a leaf. E.g. triggered using 2 4 6 4 */
				dbg(__LINE__, "xor>", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				break;
			}

			if (meth >= CEB_WM_KEQ) {
				/* let's stop if our key is not there */

				if (llen < xlen && rlen < xlen) {
					dbg(__LINE__, "mismatch", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
					break;
				}

				if (ret_npside || ret_nparent) { // delete ?
					size_t mlen = llen > rlen ? llen : rlen;

					if (mlen > xlen)
						mlen = xlen;

					if ((uint64_t)xlen / 8 == key_u64 || memcmp(key_ptr + mlen / 8, k->ptr + mlen / 8, key_u64 - mlen / 8) == 0) {
						dbg(__LINE__, "equal", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
						nparent = lparent;
						npside  = lpside;
						found = 1;
					}
				}
			}
			plen = xlen;
			if (ret_is_dup && (uint64_t)xlen / 8 == key_u64) {
				/* both sides are equal, that's a duplicate */
				dbg(__LINE__, "dup>", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				is_dup = 1;
				break;
			}
		}
		else if (key_type == CEB_KT_ST) {
			size_t xlen = 0; // left vs right matching length

			if (meth >= CEB_WM_KEQ) {
				/* Note that a negative length indicates an
				 * equal value with the final zero reached, but
				 * it is still needed to descend to find the
				 * leaf. We take that negative length for an
				 * infinite one, hence the uint cast.
				 */
				llen = string_equal_bits(key_ptr, l->str, 0);
				rlen = string_equal_bits(key_ptr, r->str, 0);
				brside = (size_t)llen <= (size_t)rlen;
				if ((ssize_t)llen < 0 || (ssize_t)rlen < 0)
					found = 1;
			}

			xlen = string_equal_bits(l->str, r->str, 0);
			if (xlen < plen) {
				/* this is a leaf. E.g. triggered using 2 4 6 4 */
				dbg(__LINE__, "xor>", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				break;
			}

			if (meth >= CEB_WM_KEQ) {
				/* let's stop if our key is not there */

				if ((unsigned)llen < (unsigned)xlen && (unsigned)rlen < (unsigned)xlen) {
					dbg(__LINE__, "mismatch", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
					break;
				}

				if (ret_npside || ret_nparent) { // delete ?
					size_t mlen = llen > rlen ? llen : rlen;

					if (mlen > xlen)
						mlen = xlen;

					if ((ssize_t)xlen < 0 || strcmp(key_ptr + mlen / 8, (const void *)k->str + mlen / 8) == 0) {
						/* strcmp() still needed. E.g. 1 2 3 4 10 11 4 3 2 1 10 11 fails otherwise */
						dbg(__LINE__, "equal", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
						nparent = lparent;
						npside  = lpside;
						found = 1;
					}
				}
			}
			plen = xlen;
			if (ret_is_dup && (ssize_t)xlen < 0) {
				/* exact match, that's a duplicate */
				dbg(__LINE__, "dup>", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				is_dup = 1;
				break;
			}
		}
		else if (key_type == CEB_KT_IS) {
			size_t xlen = 0; // left vs right matching length

			if (meth >= CEB_WM_KEQ) {
				/* Note that a negative length indicates an
				 * equal value with the final zero reached, but
				 * it is still needed to descend to find the
				 * leaf. We take that negative length for an
				 * infinite one, hence the uint cast.
				 */
				llen = string_equal_bits(key_ptr, l->ptr, 0);
				rlen = string_equal_bits(key_ptr, r->ptr, 0);
				brside = (size_t)llen <= (size_t)rlen;
				if ((ssize_t)llen < 0 || (ssize_t)rlen < 0)
					found = 1;
			}

			xlen = string_equal_bits(l->ptr, r->ptr, 0);
			if (xlen < plen) {
				/* this is a leaf. E.g. triggered using 2 4 6 4 */
				dbg(__LINE__, "xor>", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				break;
			}

			if (meth >= CEB_WM_KEQ) {
				/* let's stop if our key is not there */

				if ((unsigned)llen < (unsigned)xlen && (unsigned)rlen < (unsigned)xlen) {
					dbg(__LINE__, "mismatch", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
					break;
				}

				if (ret_npside || ret_nparent) { // delete ?
					size_t mlen = llen > rlen ? llen : rlen;

					if (mlen > xlen)
						mlen = xlen;

					if ((ssize_t)xlen < 0 || strcmp(key_ptr + mlen / 8, (const void *)k->ptr + mlen / 8) == 0) {
						/* strcmp() still needed. E.g. 1 2 3 4 10 11 4 3 2 1 10 11 fails otherwise */
						dbg(__LINE__, "equal", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
						nparent = lparent;
						npside  = lpside;
						found = 1;
					}
				}
			}
			plen = xlen;
			if (ret_is_dup && (ssize_t)xlen < 0) {
				/* both sides are equal, that's a duplicate */
				dbg(__LINE__, "dup>", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				is_dup = 1;
				break;
			}
		}
		else if (key_type == CEB_KT_ADDR) {
			uintptr_t xoraddr;   // left vs right branch xor
			uintptr_t kl, kr;

			kl = (uintptr_t)l; kr = (uintptr_t)r;
			xoraddr = kl ^ kr;

			if (xoraddr > (uintptr_t)pxor64) { // test using 2 4 6 4
				dbg(__LINE__, "xor>", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				break;
			}

			if (meth >= CEB_WM_KEQ) {
				/* "found" is not used here */
				kl ^= (uintptr_t)key_ptr; kr ^= (uintptr_t)key_ptr;
				brside = kl >= kr;

				/* let's stop if our key is not there */

				if (kl > xoraddr && kr > xoraddr) {
					dbg(__LINE__, "mismatch", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
					break;
				}

				if (ret_npside || ret_nparent) {
					if ((uintptr_t)key_ptr == (uintptr_t)p) {
						dbg(__LINE__, "equal", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
						nparent = lparent;
						npside  = lpside;
					}
				}
			}
			pxor64 = xoraddr;
			if (ret_is_dup && !xoraddr) {
				/* both sides are equal, that's a duplicate */
				dbg(__LINE__, "dup>", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
				is_dup = 1;
				break;
			}
		}

		/* shift all copies by one */
		gparent = lparent;
		gpside = lpside;
		lparent = p;
		lpside = brside;
		if (brside) {
			if (meth == CEB_WM_KPR || meth == CEB_WM_KLE || meth == CEB_WM_KLT)
				bnode = p;
			root = &p->b[1];

			/* change branch for key-less walks */
			if (meth == CEB_WM_NXT)
				brside = 0;

			dbg(__LINE__, "side1", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
		}
		else {
			if (meth == CEB_WM_KNX || meth == CEB_WM_KGE || meth == CEB_WM_KGT)
				bnode = p;
			root = &p->b[0];

			/* change branch for key-less walks */
			if (meth == CEB_WM_PRV)
				brside = 1;

			dbg(__LINE__, "side0", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
		}

		if (p == *root) {
			/* loops over itself, it's a leaf */
			dbg(__LINE__, "loop", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);
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
	if ((key_type == CEB_KT_ST || key_type == CEB_KT_IS) && meth >= CEB_WM_KEQ && !found)
		plen = (llen > rlen) ? llen : rlen;

	/* update the pointers needed for modifications (insert, delete) */
	if (ret_nside && meth >= CEB_WM_KEQ) {
		switch (key_type) {
		case CEB_KT_U32:
			*ret_nside = key_u32 >= k->u32;
			break;
		case CEB_KT_U64:
			*ret_nside = key_u64 >= k->u64;
			break;
		case CEB_KT_MB:
			*ret_nside = (uint64_t)plen / 8 == key_u64 || memcmp(key_ptr + plen / 8, k->mb + plen / 8, key_u64 - plen / 8) >= 0;
			break;
		case CEB_KT_IM:
			*ret_nside = (uint64_t)plen / 8 == key_u64 || memcmp(key_ptr + plen / 8, k->ptr + plen / 8, key_u64 - plen / 8) >= 0;
			break;
		case CEB_KT_ST:
			*ret_nside = found || strcmp(key_ptr + plen / 8, (const void *)k->str + plen / 8) >= 0;
			break;
		case CEB_KT_IS:
			*ret_nside = found || strcmp(key_ptr + plen / 8, (const void *)k->ptr + plen / 8) >= 0;
			break;
		case CEB_KT_ADDR:
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
		*ret_back = bnode;

	if (ret_is_dup)
		*ret_is_dup = is_dup;

	dbg(__LINE__, "_ret____", meth, kofs, key_type, root, p, key_u32, key_u64, key_ptr, pxor32, pxor64, plen);

	if (meth >= CEB_WM_KEQ) {
		/* For lookups, an equal value means an instant return. For insertions,
		 * it is the same, we want to return the previously existing value so
		 * that the caller can decide what to do. For deletion, we also want to
		 * return the pointer that's about to be deleted.
		 */
		if (key_type == CEB_KT_U32) {
			if ((meth == CEB_WM_KEQ && k->u32 == key_u32) ||
			    (meth == CEB_WM_KNX && k->u32 == key_u32) ||
			    (meth == CEB_WM_KPR && k->u32 == key_u32) ||
			    (meth == CEB_WM_KGE && k->u32 >= key_u32) ||
			    (meth == CEB_WM_KGT && k->u32 >  key_u32) ||
			    (meth == CEB_WM_KLE && k->u32 <= key_u32) ||
			    (meth == CEB_WM_KLT && k->u32 <  key_u32))
				return p;
		}
		else if (key_type == CEB_KT_U64) {
			if ((meth == CEB_WM_KEQ && k->u64 == key_u64) ||
			    (meth == CEB_WM_KNX && k->u64 == key_u64) ||
			    (meth == CEB_WM_KPR && k->u64 == key_u64) ||
			    (meth == CEB_WM_KGE && k->u64 >= key_u64) ||
			    (meth == CEB_WM_KGT && k->u64 >  key_u64) ||
			    (meth == CEB_WM_KLE && k->u64 <= key_u64) ||
			    (meth == CEB_WM_KLT && k->u64 <  key_u64))
				return p;
		}
		else if (key_type == CEB_KT_MB) {
			int diff;

			if ((uint64_t)plen / 8 == key_u64)
				diff = 0;
			else
				diff = memcmp(k->mb + plen / 8, key_ptr + plen / 8, key_u64 - plen / 8);

			if ((meth == CEB_WM_KEQ && diff == 0) ||
			    (meth == CEB_WM_KNX && diff == 0) ||
			    (meth == CEB_WM_KPR && diff == 0) ||
			    (meth == CEB_WM_KGE && diff >= 0) ||
			    (meth == CEB_WM_KGT && diff >  0) ||
			    (meth == CEB_WM_KLE && diff <= 0) ||
			    (meth == CEB_WM_KLT && diff <  0))
				return p;
		}
		else if (key_type == CEB_KT_IM) {
			int diff;

			if ((uint64_t)plen / 8 == key_u64)
				diff = 0;
			else
				diff = memcmp(k->ptr + plen / 8, key_ptr + plen / 8, key_u64 - plen / 8);

			if ((meth == CEB_WM_KEQ && diff == 0) ||
			    (meth == CEB_WM_KNX && diff == 0) ||
			    (meth == CEB_WM_KPR && diff == 0) ||
			    (meth == CEB_WM_KGE && diff >= 0) ||
			    (meth == CEB_WM_KGT && diff >  0) ||
			    (meth == CEB_WM_KLE && diff <= 0) ||
			    (meth == CEB_WM_KLT && diff <  0))
				return p;
		}
		else if (key_type == CEB_KT_ST) {
			int diff;

			if (found)
				diff = 0;
			else
				diff = strcmp((const void *)k->str + plen / 8, key_ptr + plen / 8);

			if ((meth == CEB_WM_KEQ && diff == 0) ||
			    (meth == CEB_WM_KNX && diff == 0) ||
			    (meth == CEB_WM_KPR && diff == 0) ||
			    (meth == CEB_WM_KGE && diff >= 0) ||
			    (meth == CEB_WM_KGT && diff >  0) ||
			    (meth == CEB_WM_KLE && diff <= 0) ||
			    (meth == CEB_WM_KLT && diff <  0))
				return p;
		}
		else if (key_type == CEB_KT_IS) {
			int diff;

			if (found)
				diff = 0;
			else
				diff = strcmp((const void *)k->ptr + plen / 8, key_ptr + plen / 8);

			if ((meth == CEB_WM_KEQ && diff == 0) ||
			    (meth == CEB_WM_KNX && diff == 0) ||
			    (meth == CEB_WM_KPR && diff == 0) ||
			    (meth == CEB_WM_KGE && diff >= 0) ||
			    (meth == CEB_WM_KGT && diff >  0) ||
			    (meth == CEB_WM_KLE && diff <= 0) ||
			    (meth == CEB_WM_KLT && diff <  0))
				return p;
		}
		else if (key_type == CEB_KT_ADDR) {
			if ((meth == CEB_WM_KEQ && (uintptr_t)p == (uintptr_t)key_ptr) ||
			    (meth == CEB_WM_KNX && (uintptr_t)p == (uintptr_t)key_ptr) ||
			    (meth == CEB_WM_KPR && (uintptr_t)p == (uintptr_t)key_ptr) ||
			    (meth == CEB_WM_KGE && (uintptr_t)p >= (uintptr_t)key_ptr) ||
			    (meth == CEB_WM_KGT && (uintptr_t)p >  (uintptr_t)key_ptr) ||
			    (meth == CEB_WM_KLE && (uintptr_t)p <= (uintptr_t)key_ptr) ||
			    (meth == CEB_WM_KLT && (uintptr_t)p <  (uintptr_t)key_ptr))
				return p;
		}
	} else if (meth == CEB_WM_FST || meth == CEB_WM_LST) {
		return p;
	} else if (meth == CEB_WM_PRV || meth == CEB_WM_NXT) {
		return p;
	}

	/* lookups and deletes fail here */

	/* let's return NULL to indicate the key was not found. For a lookup or
	 * a delete, it's a failure. For an insert, it's an invitation to the
	 * caller to proceed since the element is not there.
	 */
	return NULL;
#if defined(__GNUC__) && (__GNUC__ >= 12) && !defined(__OPTIMIZE__)
#pragma GCC diagnostic pop
#endif
}

/*
 *  Below are the functions that support duplicate keys (_ceb_*)
 */

/* Generic tree insertion function for trees with duplicate keys. Inserts node
 * <node> into tree <tree>, with key type <key_type> and key <key_*>.
 * Returns the inserted node or the one that already contains the same key.
 * If <is_dup_ptr> is non-null, then duplicates are permitted and this variable
 * is used to temporarily carry an internal state.
 */
static inline __attribute__((always_inline))
struct ceb_node *_ceb_insert(struct ceb_node **root,
                             struct ceb_node *node,
                             ptrdiff_t kofs,
                             enum ceb_key_type key_type,
                             uint32_t key_u32,
                             uint64_t key_u64,
                             const void *key_ptr,
                             int *is_dup_ptr)
{
	struct ceb_node **parent;
	struct ceb_node *ret;
	int nside;

	if (!*root) {
		/* empty tree, insert a leaf only */
		node->b[0] = node->b[1] = node;
		*root = node;
		return node;
	}

	ret = _ceb_descend(root, CEB_WM_KEQ, kofs, key_type, key_u32, key_u64, key_ptr, &nside, &parent, NULL, NULL, NULL, NULL, NULL, NULL, NULL, is_dup_ptr);

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
	} else if (is_dup_ptr) {
		/* The key was found. We must insert after it as the last
		 * element of the dups list, which means that our left branch
		 * will point to the key, the right one to the first dup
		 * (i.e. previous dup's right if it exists, otherwise ourself)
		 * and the parent must point to us.
		 */
		node->b[0] = *parent;

		if (*is_dup_ptr) {
			node->b[1] = (*parent)->b[1];
			(*parent)->b[1] = node;
		} else {
			node->b[1] = node;
		}
		*parent = node;
		ret = node;
	}
	return ret;
}

/* Returns the first node or NULL if not found, assuming a tree made of keys of
 * type <key_type>, and optionally <key_len> for fixed-size arrays (otherwise 0).
 * If the tree starts with duplicates, the first of them is returned.
 */
static inline __attribute__((always_inline))
struct ceb_node *_ceb_first(struct ceb_node **root,
                            ptrdiff_t kofs,
                            enum ceb_key_type key_type,
                            uint64_t key_len,
                            int *is_dup_ptr)
{
	struct ceb_node *node;

	if (!*root)
		return NULL;

	node = _ceb_descend(root, CEB_WM_FST, kofs, key_type, 0, key_len, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, is_dup_ptr);
	if (node && is_dup_ptr && *is_dup_ptr) {
		/* on a duplicate, the first node is right->left */
		node = node->b[1]->b[0];
	}
	return node;
}

/* Returns the last node or NULL if not found, assuming a tree made of keys of
 * type <key_type>, and optionally <key_len> for fixed-size arrays (otherwise 0).
 * If the tree ends with duplicates, the last of them is returned.
 */
static inline __attribute__((always_inline))
struct ceb_node *_ceb_last(struct ceb_node **root,
                           ptrdiff_t kofs,
                           enum ceb_key_type key_type,
                           uint64_t key_len)
{
	if (!*root)
		return NULL;

	/* note for duplicates: the current scheme always returns the last one by default */
	return _ceb_descend(root, CEB_WM_LST, kofs, key_type, 0, key_len, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* Searches in the tree <root> made of keys of type <key_type>, for the next
 * node after the one containing the key <key_*>. Returns NULL if not found.
 * It's up to the caller to pass the current node's key in <key_*>. The
 * approach consists in looking up that node first, recalling the last time a
 * left turn was made, and returning the first node along the right branch at
 * that fork.
 */
static inline __attribute__((always_inline))
struct ceb_node *_ceb_next_unique(struct ceb_node **root,
                                  ptrdiff_t kofs,
                                  enum ceb_key_type key_type,
                                  uint32_t key_u32,
                                  uint64_t key_u64,
                                  const void *key_ptr)
{
	struct ceb_node *restart;

	if (!*root)
		return NULL;

	if (!_ceb_descend(root, CEB_WM_KNX, kofs, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &restart, NULL))
		return NULL;

	if (!restart)
		return NULL;

	return _ceb_descend(&restart, CEB_WM_NXT, kofs, key_type, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* Searches in the tree <root> made of keys of type <key_type>, for the prev
 * node before the one containing the key <key_*>. Returns NULL if not found.
 * It's up to the caller to pass the current node's key in <key_*>. The
 * approach consists in looking up that node first, recalling the last time a
 * right turn was made, and returning the last node along the left branch at
 * that fork.
 */
static inline __attribute__((always_inline))
struct ceb_node *_ceb_prev_unique(struct ceb_node **root,
                                  ptrdiff_t kofs,
                                  enum ceb_key_type key_type,
                                  uint32_t key_u32,
                                  uint64_t key_u64,
                                  const void *key_ptr)
{
	struct ceb_node *restart;

	if (!*root)
		return NULL;

	if (!_ceb_descend(root, CEB_WM_KPR, kofs, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &restart, NULL))
		return NULL;

	if (!restart)
		return NULL;

	return _ceb_descend(&restart, CEB_WM_PRV, kofs, key_type, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* Searches in the tree <root> made of keys of type <key_type>, for the next
 * node after <from> also containing key <key_*>. Returns NULL if not found.
 * It's up to the caller to pass the current node's key in <key_*>.
 */
static inline __attribute__((always_inline))
struct ceb_node *_ceb_next_dup(struct ceb_node **root,
                               ptrdiff_t kofs,
                               enum ceb_key_type key_type,
                               uint32_t key_u32,
                               uint64_t key_u64,
                               const void *key_ptr,
                               const struct ceb_node *from)
{
	struct ceb_node *node;
	int is_dup;

	if (!*root)
		return NULL;

	node = _ceb_descend(root, CEB_WM_KNX, kofs, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &is_dup);
	if (!node)
		return NULL;

	/* Normally at this point, if node != from, we've found a node that
	 * differs from the one we're starting from, which indicates that
	 * the starting point belongs to a dup list and is not the last one.
	 * We must then visit the other members. We cannot navigate from the
	 * regular leaf node (the first one) but we can easily verify if we're
	 * on that one by checking if it's node->b[1]->b[0], in which case we
	 * jump to node->b[1]. Otherwise we take from->b[1].
	 */
	if (node != from) {
		if (node->b[1]->b[0] == from)
			return node->b[1];
		else
			return from->b[1];
	}

	/* there's no other dup here */
	return NULL;
}

/* Searches in the tree <root> made of keys of type <key_type>, for the prev
 * node before <from> also containing key <key_*>. Returns NULL if not found.
 * It's up to the caller to pass the current node's key in <key_*>.
 */
static inline __attribute__((always_inline))
struct ceb_node *_ceb_prev_dup(struct ceb_node **root,
                               ptrdiff_t kofs,
                               enum ceb_key_type key_type,
                               uint32_t key_u32,
                               uint64_t key_u64,
                               const void *key_ptr,
                               const struct ceb_node *from)
{
	struct ceb_node *node;
	int is_dup;

	if (!*root)
		return NULL;

	node = _ceb_descend(root, CEB_WM_KPR, kofs, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &is_dup);
	if (!node)
		return NULL;

	/* Here we have several possibilities:
	 *   - from == node => we've found our node. It may be a unique node,
	 *     or the last one of a dup series. We'll sort that out thanks to
	 *     is_dup, and if it's a dup, we'll use node->b[0].
	 *   - from is not the first dup, so we haven't visited them all yet,
	 *     hence we visit node->b[0] to switch to the previous dup.
	 *   - from is the first dup so we've visited them all.
	 */
	if (is_dup && (node == from || node->b[1]->b[0] != from))
		return from->b[0];

	/* there's no other dup here */
	return NULL;
}

/* Searches in the tree <root> made of keys of type <key_type>, for the next
 * node after <from> which contains key <key_*>. Returns NULL if not found.
 * It's up to the caller to pass the current node's key in <key_*>. The
 * approach consists in looking up that node first, recalling the last time a
 * left turn was made, and returning the first node along the right branch at
 * that fork. In case the current node belongs to a duplicate list, all dups
 * will be visited in insertion order prior to jumping to different keys.
 */
static inline __attribute__((always_inline))
struct ceb_node *_ceb_next(struct ceb_node **root,
                           ptrdiff_t kofs,
                           enum ceb_key_type key_type,
                           uint32_t key_u32,
                           uint64_t key_u64,
                           const void *key_ptr,
                           const struct ceb_node *from)
{
	struct ceb_node *restart;
	struct ceb_node *node;
	int is_dup;

	if (!*root)
		return NULL;

	node = _ceb_descend(root, CEB_WM_KNX, kofs, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &restart, &is_dup);
	if (!node)
		return NULL;

	/* Normally at this point, if node != from, we've found a node that
	 * differs from the one we're starting from, which indicates that
	 * the starting point belongs to a dup list and is not the last one.
	 * We must then visit the other members. We cannot navigate from the
	 * regular leaf node (the first one) but we can easily verify if we're
	 * on that one by checking if it's node->b[1]->b[0], in which case we
	 * jump to node->b[1]. Otherwise we take from->b[1].
	 */
	if (node != from) {
		if (node->b[1]->b[0] == from)
			return node->b[1];
		else
			return from->b[1];
	}

	/* Here the looked up node was found (node == from) and we can look up
	 * the next unique one if any.
	 */
	if (!restart)
		return NULL;

	/* this look up will stop on the topmost dup in a sub-tree which is
	 * also the last one. Thanks to restart we know that this entry exists.
	 */
	node = _ceb_descend(&restart, CEB_WM_NXT, kofs, key_type, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &is_dup);
	if (node && is_dup) {
		/* on a duplicate, the first node is right->left */
		node = node->b[1]->b[0];
	}
	return node;
}

/* Searches in the tree <root> made of keys of type <key_type>, for the prev
 * node before the one containing the key <key_*>. Returns NULL if not found.
 * It's up to the caller to pass the current node's key in <key_*>. The
 * approach consists in looking up that node first, recalling the last time a
 * right turn was made, and returning the last node along the left branch at
 * that fork. In case the current node belongs to a duplicate list, all dups
 * will be visited in reverse insertion order prior to jumping to different
 * keys.
 */
static inline __attribute__((always_inline))
struct ceb_node *_ceb_prev(struct ceb_node **root,
                           ptrdiff_t kofs,
                           enum ceb_key_type key_type,
                           uint32_t key_u32,
                           uint64_t key_u64,
                           const void *key_ptr,
                           const struct ceb_node *from)
{
	struct ceb_node *restart;
	struct ceb_node *node;
	int is_dup;

	if (!*root)
		return NULL;

	node = _ceb_descend(root, CEB_WM_KPR, kofs, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &restart, &is_dup);
	if (!node)
		return NULL;

	/* Here we have several possibilities:
	 *   - from == node => we've found our node. It may be a unique node,
	 *     or the last one of a dup series. We'll sort that out thanks to
	 *     is_dup, and if it's a dup, we'll use node->b[0].
	 *   - from is not the first dup, so we haven't visited them all yet,
	 *     hence we visit node->b[0] to switch to the previous dup.
	 *   - from is the first dup so we've visited them all, we now need
	 *     to jump to the previous unique value.
	 */
	if (is_dup && (node == from || node->b[1]->b[0] != from))
		return from->b[0];

	/* look up the previous unique entry */
	if (!restart)
		return NULL;

	/* Note that the descent stops on the last dup which is the one we want */
	return _ceb_descend(&restart, CEB_WM_PRV, kofs, key_type, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &is_dup);
}

/* Searches in the tree <root> made of keys of type <key_type>, for the first
 * node containing the key <key_*>. Returns NULL if not found.
 */
static inline __attribute__((always_inline))
struct ceb_node *_ceb_lookup(struct ceb_node **root,
                             ptrdiff_t kofs,
                             enum ceb_key_type key_type,
                             uint32_t key_u32,
                             uint64_t key_u64,
                             const void *key_ptr,
                             int *is_dup_ptr)
{
	struct ceb_node *ret;

	if (!*root)
		return NULL;

	ret = _ceb_descend(root, CEB_WM_KEQ, kofs, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, is_dup_ptr);
	if (ret && is_dup_ptr && *is_dup_ptr) {
		/* on a duplicate, the first node is right->left */
		ret = ret->b[1]->b[0];
	}
	return ret;
}

/* Searches in the tree <root> made of keys of type <key_type>, for the last
 * node containing the key <key_*> or the highest one that's lower than it.
 * Returns NULL if not found.
 */
static inline __attribute__((always_inline))
struct ceb_node *_ceb_lookup_le(struct ceb_node **root,
                                ptrdiff_t kofs,
                                enum ceb_key_type key_type,
                                uint32_t key_u32,
                                uint64_t key_u64,
                                const void *key_ptr)
{
	struct ceb_node *ret = NULL;
	struct ceb_node *restart;

	if (!*root)
		return NULL;

	/* note that for duplicates, we already find the last one */
	ret = _ceb_descend(root, CEB_WM_KLE, kofs, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &restart, NULL);
	if (ret)
		return ret;

	if (!restart)
		return NULL;

	return _ceb_descend(&restart, CEB_WM_PRV, kofs, key_type, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* Searches in the tree <root> made of keys of type <key_type>, for the last
 * node containing the greatest key that is strictly lower than <key_*>.
 * Returns NULL if not found. It's very similar to next() except that the
 * looked up value doesn't need to exist.
 */
static inline __attribute__((always_inline))
struct ceb_node *_ceb_lookup_lt(struct ceb_node **root,
                                ptrdiff_t kofs,
                                enum ceb_key_type key_type,
                                uint32_t key_u32,
                                uint64_t key_u64,
                                const void *key_ptr)
{
	struct ceb_node *ret = NULL;
	struct ceb_node *restart;

	if (!*root)
		return NULL;

	/* note that for duplicates, we already find the last one */
	ret = _ceb_descend(root, CEB_WM_KLT, kofs, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &restart, NULL);
	if (ret)
		return ret;

	if (!restart)
		return NULL;

	return _ceb_descend(&restart, CEB_WM_PRV, kofs, key_type, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/* Searches in the tree <root> made of keys of type <key_type>, for the first
 * node containing the key <key_*> or the smallest one that's greater than it.
 * Returns NULL if not found. If <is_dup_ptr> is non-null, then duplicates are
 * permitted and this variable is used to temporarily carry an internal state.

 */
static inline __attribute__((always_inline))
struct ceb_node *_ceb_lookup_ge(struct ceb_node **root,
                                ptrdiff_t kofs,
                                enum ceb_key_type key_type,
                                uint32_t key_u32,
                                uint64_t key_u64,
                                const void *key_ptr,
                                int *is_dup_ptr)
{
	struct ceb_node *ret = NULL;
	struct ceb_node *restart;

	if (!*root)
		return NULL;

	ret = _ceb_descend(root, CEB_WM_KGE, kofs, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &restart, is_dup_ptr);
	if (!ret) {
		if (!restart)
			return NULL;

		ret = _ceb_descend(&restart, CEB_WM_NXT, kofs, key_type, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, is_dup_ptr);
	}

	if (ret && is_dup_ptr && *is_dup_ptr) {
		/* on a duplicate, the first node is right->left */
		ret = ret->b[1]->b[0];
	}
	return ret;
}

/* Searches in the tree <root> made of keys of type <key_type>, for the first
 * node containing the lowest key that is strictly greater than <key_*>. Returns
 * NULL if not found. It's very similar to prev() except that the looked up
 * value doesn't need to exist. If <is_dup_ptr> is non-null, then duplicates are
 * permitted and this variable is used to temporarily carry an internal state.
 */
static inline __attribute__((always_inline))
struct ceb_node *_ceb_lookup_gt(struct ceb_node **root,
                                ptrdiff_t kofs,
                                enum ceb_key_type key_type,
                                uint32_t key_u32,
                                uint64_t key_u64,
                                const void *key_ptr,
                                int *is_dup_ptr)
{
	struct ceb_node *ret = NULL;
	struct ceb_node *restart;

	if (!*root)
		return NULL;

	ret = _ceb_descend(root, CEB_WM_KGT, kofs, key_type, key_u32, key_u64, key_ptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &restart, is_dup_ptr);
	if (!ret) {
		if (!restart)
			return NULL;

		ret = _ceb_descend(&restart, CEB_WM_NXT, kofs, key_type, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, is_dup_ptr);
	}

	if (ret && is_dup_ptr && *is_dup_ptr) {
		/* on a duplicate, the first node is right->left */
		ret = ret->b[1]->b[0];
	}
	return ret;
}

/* Searches in the tree <root> made of keys of type <key_type>, for the node
 * that contains the key <key_*>, and deletes it. If <node> is non-NULL, a
 * check is performed and the node found is deleted only if it matches. The
 * found node is returned in any case, otherwise NULL if not found. A deleted
 * node is detected since it has b[0]==NULL, which this functions also clears
 * after operation. The function is idempotent, so it's safe to attempt to
 * delete an already deleted node (NULL is returned in this case since the node
 * was not in the tree). If <is_dup_ptr> is non-null, then duplicates are
 * permitted and this variable is used to temporarily carry an internal state.
 */
static inline __attribute__((always_inline))
struct ceb_node *_ceb_delete(struct ceb_node **root,
                             struct ceb_node *node,
                             ptrdiff_t kofs,
                             enum ceb_key_type key_type,
                             uint32_t key_u32,
                             uint64_t key_u64,
                             const void *key_ptr,
                             int *is_dup_ptr)
{
	struct ceb_node *lparent, *nparent, *gparent;
	int lpside, npside, gpside;
	struct ceb_node *ret = NULL;

	if (node && !node->b[0]) {
		/* NULL on a branch means the node is not in the tree */
		return NULL;
	}

	if (!*root) {
		/* empty tree, the node cannot be there */
		goto done;
	}

	ret = _ceb_descend(root, CEB_WM_KEQ, kofs, key_type, key_u32, key_u64, key_ptr, NULL, NULL,
			   &lparent, &lpside, &nparent, &npside, &gparent, &gpside, NULL, is_dup_ptr);

	if (!ret) {
		/* key not found */
		goto done;
	}

	if (is_dup_ptr && *is_dup_ptr) {
		/* the node to be deleted belongs to a dup sub-tree whose ret
		 * is the last. The possibilities here are:
		 *   1) node==NULL => unspecified, we delete the first one,
		 *      which is the tree leaf. The tree node (if it exists)
		 *      is replaced by the first dup. There's nothing else to
		 *      change.
		 *   2) node is the tree leaf. The tree node (if it exists)
		 *      is replaced by the first dup.
		 *   3) node is a dup. We just delete the dup.
		 *      In order to delete a dup, there are 4 cases:
		 *        a) node==last and there's a single dup, it's this one
		 *           -> *parent = node->b[0];
		 *        b) node==last and there's another dup:
		 *           -> *parent = node->b[0];
		 *              node->b[0]->b[1] = node->b[1];
		 *              (or (*parent)->b[1] = node->b[1] covers a and b)
		 *        c) node==first != last:
		 *           -> node->b[1]->b[0] = node->b[0];
		 *              last->b[1] = node->b[1];
		 *              (or (*parent)->b[1] = node->b[1] covers a,b,c)
		 *        d) node!=first && !=last:
		 *           -> node->b[1]->b[0] = node->b[0];
		 *              node->b[0]->b[1] = node->b[1];
		 *      a,b,c,d can be simplified as:
		 *         ((node == first) ? last : node->b[0])->b[1] = node->b[1];
		 *         *((node == last) ? parent : &node->b[1]->b[0]) = node->b[0];
		 */
		struct ceb_node *first, *last;

		last = ret;
		first = last->b[1];

		/* cases 1 and 2 below */
		if (!node || node == first->b[0]) {
			/* node unspecified or the first, remove the first entry (the leaf) */
			ret = first->b[0]; // update return node
			last->b[1] = first->b[1]; // new first (remains OK if last==first)

			if (ret->b[0] != ret || ret->b[1] != ret) {
				/* not the nodeless leaf, a node exists, put it
				 * on the first and update its parent.
				 */
				first->b[0] = ret->b[0];
				first->b[1] = ret->b[1];
				gparent->b[gpside] = first;
			}
			else {
				/* first becomes the nodeless leaf since we only keep its leaf */
				first->b[0] = first->b[1] = first;
			}
			/* done */
		} else {
			/* case 3: the node to delete is a dup, we only have to
			 * manipulate the list.
			 */
			ret = node;
			((node == first) ? last : node->b[0])->b[1] = node->b[1];
			*((node == last) ? &lparent->b[lpside] : &node->b[1]->b[0]) = node->b[0];
			/* done */
		}
		goto mark_and_leave;
	}

	/* ok below the returned value is a real leaf, we have to adjust the tree */

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

/*
 * Functions used to dump trees in Dot format. These are only enabled if
 * CEB_ENABLE_DUMP is defined.
 */

#if defined(CEB_ENABLE_DUMP)

#include <stdio.h>

/* dump the root and its link to the first node or leaf */
__attribute__((unused))
static void ceb_default_dump_root(ptrdiff_t kofs, enum ceb_key_type key_type, struct ceb_node *const *root, const void *ctx, int sub)
{
	const struct ceb_node *node;
	uint64_t pxor;

	if (!sub)
		printf("  \"%lx_n_%d\" [label=\"root\\n%lx\"]\n", (long)root, sub, (long)root);
	else
		printf("  \"%lx_n_%d\" [label=\"root\\n%lx\\ntree #%d\"]\n", (long)root, sub, (long)root, sub);

	node = *root;
	if (node) {
		/* under the root we've either a node or the first leaf */

		/* xor of the keys of the two lower branches */
		pxor = _xor_branches(kofs, key_type, 0, 0, NULL,
				     node->b[0], node->b[1]);

		printf("  \"%lx_n_%d\" -> \"%lx_%c_%d\" [label=\"B\" arrowsize=0.66%s];\n",
		       (long)root, sub, (long)node,
		       (node->b[0] == node->b[1] || !pxor) ? 'l' : 'n', sub,
		       (ctx == node) ? " color=red" : "");
	}
}

/* dump a node */
__attribute__((unused))
static void ceb_default_dump_node(ptrdiff_t kofs, enum ceb_key_type key_type, const struct ceb_node *node, int level, const void *ctx, int sub)
{
	unsigned long long int_key = 0;
	uint64_t pxor, lxor, rxor;
	const char *str_key = NULL;

	switch (key_type) {
	case CEB_KT_ADDR:
		int_key = (uintptr_t)node;
		break;
	case CEB_KT_U32:
		int_key = NODEK(node, kofs)->u32;
		break;
	case CEB_KT_U64:
		int_key = NODEK(node, kofs)->u64;
		break;
	case CEB_KT_ST:
		str_key = (char*)NODEK(node, kofs)->str;
		break;
	case CEB_KT_IS:
		str_key = (char*)NODEK(node, kofs)->ptr;
		break;
	default:
		break;
	}

	/* xor of the keys of the two lower branches */
	pxor = _xor_branches(kofs, key_type, 0, 0, NULL,
			     node->b[0], node->b[1]);

	/* xor of the keys of the left branch's lower branches */
	lxor = _xor_branches(kofs, key_type, 0, 0, NULL,
			     (((struct ceb_node*)node->b[0])->b[0]),
			     (((struct ceb_node*)node->b[0])->b[1]));

	/* xor of the keys of the right branch's lower branches */
	rxor = _xor_branches(kofs, key_type, 0, 0, NULL,
			     (((struct ceb_node*)node->b[1])->b[0]),
			     (((struct ceb_node*)node->b[1])->b[1]));

	switch (key_type) {
	case CEB_KT_ADDR:
	case CEB_KT_U32:
	case CEB_KT_U64:
		printf("  \"%lx_n_%d\" [label=\"%lx\\nlev=%d bit=%d\\nkey=%llu\" fillcolor=\"lightskyblue1\"%s];\n",
		       (long)node, sub, (long)node, level, flsnz(pxor) - 1, int_key, (ctx == node) ? " color=red" : "");

		printf("  \"%lx_n_%d\" -> \"%lx_%c_%d\" [label=\"L\" arrowsize=0.66%s%s];\n",
		       (long)node, sub, (long)node->b[0],
		       (lxor < pxor && ((struct ceb_node*)node->b[0])->b[0] != ((struct ceb_node*)node->b[0])->b[1] && lxor) ? 'n' : 'l',
		       sub, (node == node->b[0]) ? " dir=both" : "", (ctx == node->b[0]) ? " color=red" : "");

		printf("  \"%lx_n_%d\" -> \"%lx_%c_%d\" [label=\"R\" arrowsize=0.66%s%s];\n",
		       (long)node, sub, (long)node->b[1],
		       (rxor < pxor && ((struct ceb_node*)node->b[1])->b[0] != ((struct ceb_node*)node->b[1])->b[1] && rxor) ? 'n' : 'l',
		       sub, (node == node->b[1]) ? " dir=both" : "", (ctx == node->b[1]) ? " color=red" : "");
		break;
	case CEB_KT_MB:
		break;
	case CEB_KT_IM:
		break;
	case CEB_KT_ST:
	case CEB_KT_IS:
		printf("  \"%lx_n_%d\" [label=\"%lx\\nlev=%d bit=%ld\\nkey=\\\"%s\\\"\" fillcolor=\"lightskyblue1\"%s];\n",
		       (long)node, sub, (long)node, level, (long)~pxor, str_key, (ctx == node) ? " color=red" : "");

		printf("  \"%lx_n_%d\" -> \"%lx_%c_%d\" [label=\"L\" arrowsize=0.66%s%s];\n",
		       (long)node, sub, (long)node->b[0],
		       (lxor < pxor && ((struct ceb_node*)node->b[0])->b[0] != ((struct ceb_node*)node->b[0])->b[1] && lxor) ? 'n' : 'l',
		       sub, (node == node->b[0]) ? " dir=both" : "", (ctx == node->b[0]) ? " color=red" : "");

		printf("  \"%lx_n_%d\" -> \"%lx_%c_%d\" [label=\"R\" arrowsize=0.66%s%s];\n",
		       (long)node, sub, (long)node->b[1],
		       (rxor < pxor && ((struct ceb_node*)node->b[1])->b[0] != ((struct ceb_node*)node->b[1])->b[1] && rxor) ? 'n' : 'l',
		       sub, (node == node->b[1]) ? " dir=both" : "", (ctx == node->b[1]) ? " color=red" : "");
		break;
	}
}

/* dump a duplicate entry */
__attribute__((unused))
static void ceb_default_dump_dups(ptrdiff_t kofs, enum ceb_key_type key_type, const struct ceb_node *node, int level, const void *ctx, int sub)
{
	unsigned long long int_key = 0;
	const struct ceb_node *leaf;
	const char *str_key = NULL;
	int is_last;

	switch (key_type) {
	case CEB_KT_ADDR:
		int_key = (uintptr_t)node;
		break;
	case CEB_KT_U32:
		int_key = NODEK(node, kofs)->u32;
		break;
	case CEB_KT_U64:
		int_key = NODEK(node, kofs)->u64;
		break;
	case CEB_KT_ST:
		str_key = (char*)NODEK(node, kofs)->str;
		break;
	case CEB_KT_IS:
		str_key = (char*)NODEK(node, kofs)->ptr;
		break;
	default:
		break;
	}

	/* Let's try to determine which one is the last of the series. The
	 * right node's left node is a tree leaf in this only case. This means
	 * that node either has both sides equal to itself, or a distinct
	 * neighbours.
	 */
	leaf = node->b[1]->b[0];

	is_last = 1;
	if (leaf->b[0] != leaf || leaf->b[1] != leaf)
		is_last = _xor_branches(kofs, key_type, 0, 0, NULL,
					((struct ceb_node*)leaf->b[0]),
					((struct ceb_node*)leaf->b[1])) != 0;

	switch (key_type) {
	case CEB_KT_ADDR:
	case CEB_KT_U32:
	case CEB_KT_U64:
		printf("  \"%lx_l_%d\" [label=\"%lx\\nlev=%d\\nkey=%llu\" fillcolor=\"wheat1\"%s];\n",
		       (long)node, sub, (long)node, level, int_key, (ctx == node) ? " color=red" : "");

		printf("  \"%lx_l_%d\":sw -> \"%lx_l_%d\":n [taillabel=\"L\" arrowsize=0.66%s];\n",
		       (long)node, sub, (long)node->b[0], sub, (ctx == node->b[0]) ? " color=red" : "");

		printf("  \"%lx_l_%d\":%s -> \"%lx_l_%d\":%s [taillabel=\"R\" arrowsize=0.66%s];\n",
		       (long)node, sub, is_last ? "se" : "ne",
		       (long)node->b[1], sub, is_last ? "e" : "s", (ctx == node->b[1]) ? " color=red" : "");
		break;
	case CEB_KT_MB:
		break;
	case CEB_KT_IM:
		break;
	case CEB_KT_ST:
	case CEB_KT_IS:
		printf("  \"%lx_l_%d\" [label=\"%lx\\nlev=%d\\nkey=\\\"%s\\\"\" fillcolor=\"wheat1\"%s];\n",
		       (long)node, sub, (long)node, level, str_key, (ctx == node) ? " color=red" : "");

		printf("  \"%lx_l_%d\":sw -> \"%lx_l_%d\":n [taillabel=\"L\" arrowsize=0.66%s];\n",
		       (long)node, sub, (long)node->b[0], sub, (ctx == node->b[0]) ? " color=red" : "");

		printf("  \"%lx_l_%d\":%s -> \"%lx_l_%d\":%s [taillabel=\"R\" arrowsize=0.66%s];\n",
		       (long)node, sub, is_last ? "se" : "ne",
		       (long)node->b[1], sub, is_last ? "e" : "s", (ctx == node->b[1]) ? " color=red" : "");
		break;
	}
}

/* dump a leaf */
__attribute__((unused))
static void ceb_default_dump_leaf(ptrdiff_t kofs, enum ceb_key_type key_type, const struct ceb_node *node, int level, const void *ctx, int sub)
{
	unsigned long long int_key = 0;
	const char *str_key = NULL;
	uint64_t pxor;

	switch (key_type) {
	case CEB_KT_ADDR:
		int_key = (uintptr_t)node;
		break;
	case CEB_KT_U32:
		int_key = NODEK(node, kofs)->u32;
		break;
	case CEB_KT_U64:
		int_key = NODEK(node, kofs)->u64;
		break;
	case CEB_KT_ST:
		str_key = (char*)NODEK(node, kofs)->str;
		break;
	case CEB_KT_IS:
		str_key = (char*)NODEK(node, kofs)->ptr;
		break;
	default:
		break;
	}

	/* xor of the keys of the two lower branches */
	pxor = _xor_branches(kofs, key_type, 0, 0, NULL,
			     node->b[0], node->b[1]);

	switch (key_type) {
	case CEB_KT_ADDR:
	case CEB_KT_U32:
	case CEB_KT_U64:
		if (node->b[0] == node->b[1])
			printf("  \"%lx_l_%d\" [label=\"%lx\\nlev=%d\\nkey=%llu\\n\" fillcolor=\"green\"%s];\n",
			       (long)node, sub, (long)node, level, int_key, (ctx == node) ? " color=red" : "");
		else
			printf("  \"%lx_l_%d\" [label=\"%lx\\nlev=%d bit=%d\\nkey=%llu\\n\" fillcolor=\"yellow\"%s];\n",
			       (long)node, sub, (long)node, level, flsnz(pxor) - 1, int_key, (ctx == node) ? " color=red" : "");
		break;
	case CEB_KT_MB:
		break;
	case CEB_KT_IM:
		break;
	case CEB_KT_ST:
	case CEB_KT_IS:
		if (node->b[0] == node->b[1])
			printf("  \"%lx_l_%d\" [label=\"%lx\\nlev=%d\\nkey=\\\"%s\\\"\\n\" fillcolor=\"green\"%s];\n",
			       (long)node, sub, (long)node, level, str_key, (ctx == node) ? " color=red" : "");
		else
			printf("  \"%lx_l_%d\" [label=\"%lx\\nlev=%d bit=%ld\\nkey=\\\"%s\\\"\\n\" fillcolor=\"yellow\"%s];\n",
			       (long)node, sub, (long)node, level, (long)~pxor, str_key, (ctx == node) ? " color=red" : "");
		break;
	}
}

/* Dumps a tree through the specified callbacks, falling back to the default
 * callbacks above if left NULL.
 */
__attribute__((unused))
static const struct ceb_node *ceb_default_dump_tree(ptrdiff_t kofs, enum ceb_key_type key_type, struct ceb_node *const *root,
                                                    uint64_t pxor, const void *last, int level, const void *ctx, int sub,
                                                    void (*root_dump)(ptrdiff_t kofs, enum ceb_key_type key_type, struct ceb_node *const *root, const void *ctx, int sub),
                                                    void (*node_dump)(ptrdiff_t kofs, enum ceb_key_type key_type, const struct ceb_node *node, int level, const void *ctx, int sub),
                                                    void (*dups_dump)(ptrdiff_t kofs, enum ceb_key_type key_type, const struct ceb_node *node, int level, const void *ctx, int sub),
                                                    void (*leaf_dump)(ptrdiff_t kofs, enum ceb_key_type key_type, const struct ceb_node *node, int level, const void *ctx, int sub))
{
	const struct ceb_node *node = *root;
	uint64_t xor;

	if (!node) /* empty tree */
		return node;

	if (!root_dump)
		root_dump = ceb_default_dump_root;

	if (!node_dump)
		node_dump = ceb_default_dump_node;

	if (!dups_dump)
		dups_dump = ceb_default_dump_dups;

	if (!leaf_dump)
		leaf_dump = ceb_default_dump_leaf;

	if (!level) {
		/* dump the first arrow */
		root_dump(kofs, key_type, root, ctx, sub);
	}

	/* regular nodes, all branches are canonical */

	while (1) {
		if (node->b[0] == node && node->b[1] == node) {
			/* first inserted leaf */
			leaf_dump(kofs, key_type, node, level, ctx, sub);
			return node;
		}

		xor = _xor_branches(kofs, key_type, 0, 0, NULL,
				    node->b[0], node->b[1]);
		if (xor)
			break;

		/* a zero XOR with different branches indicates a list element,
		 * we dump it and walk to the left until we find the node.
		 */
		dups_dump(kofs, key_type, node, level, ctx, sub);
		node = node->b[0];
	}

	if (pxor && xor >= pxor) {
		/* that's a leaf */
		leaf_dump(kofs, key_type, node, level, ctx, sub);
		return node;
	}

	/* that's a regular node */
	node_dump(kofs, key_type, node, level, ctx, sub);

	last = ceb_default_dump_tree(kofs, key_type, &node->b[0], xor, last, level + 1, ctx, sub, root_dump, node_dump, dups_dump, leaf_dump);
	return ceb_default_dump_tree(kofs, key_type, &node->b[1], xor, last, level + 1, ctx, sub, root_dump, node_dump, dups_dump, leaf_dump);
}

#endif /* CEB_ENABLE_DUMP */

#endif /* _CEBTREE_PRV_H */
