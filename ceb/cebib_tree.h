/*
 * Compact Elastic Binary Trees - exported functions operating on indirect blocks
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

#ifndef _CEBIB_TREE_H
#define _CEBIB_TREE_H

#include "cebtree.h"

/* simpler version */
struct ceb_node *cebib_insert(struct ceb_root **root, struct ceb_node *node, size_t len);
struct ceb_node *cebib_first(struct ceb_root *const *root, size_t len);
struct ceb_node *cebib_last(struct ceb_root *const *root, size_t len);
struct ceb_node *cebib_lookup(struct ceb_root *const *root, const void *key, size_t len);
struct ceb_node *cebib_lookup_le(struct ceb_root *const *root, const void *key, size_t len);
struct ceb_node *cebib_lookup_lt(struct ceb_root *const *root, const void *key, size_t len);
struct ceb_node *cebib_lookup_ge(struct ceb_root *const *root, const void *key, size_t len);
struct ceb_node *cebib_lookup_gt(struct ceb_root *const *root, const void *key, size_t len);
struct ceb_node *cebib_next_unique(struct ceb_root *const *root, struct ceb_node *node, size_t len);
struct ceb_node *cebib_prev_unique(struct ceb_root *const *root, struct ceb_node *node, size_t len);
struct ceb_node *cebib_next_dup(struct ceb_root *const *root, struct ceb_node *node, size_t len);
struct ceb_node *cebib_prev_dup(struct ceb_root *const *root, struct ceb_node *node, size_t len);
struct ceb_node *cebib_next(struct ceb_root *const *root, struct ceb_node *node, size_t len);
struct ceb_node *cebib_prev(struct ceb_root *const *root, struct ceb_node *node, size_t len);
struct ceb_node *cebib_delete(struct ceb_root **root, struct ceb_node *node, size_t len);
struct ceb_node *cebib_pick(struct ceb_root **root, const void *key, size_t len);

struct ceb_node *cebuib_insert(struct ceb_root **root, struct ceb_node *node, size_t len);
struct ceb_node *cebuib_first(struct ceb_root *const *root, size_t len);
struct ceb_node *cebuib_last(struct ceb_root *const *root, size_t len);
struct ceb_node *cebuib_lookup(struct ceb_root *const *root, const void *key, size_t len);
struct ceb_node *cebuib_lookup_le(struct ceb_root *const *root, const void *key, size_t len);
struct ceb_node *cebuib_lookup_lt(struct ceb_root *const *root, const void *key, size_t len);
struct ceb_node *cebuib_lookup_ge(struct ceb_root *const *root, const void *key, size_t len);
struct ceb_node *cebuib_lookup_gt(struct ceb_root *const *root, const void *key, size_t len);
struct ceb_node *cebuib_next(struct ceb_root *const *root, struct ceb_node *node, size_t len);
struct ceb_node *cebuib_prev(struct ceb_root *const *root, struct ceb_node *node, size_t len);
struct ceb_node *cebuib_delete(struct ceb_root **root, struct ceb_node *node, size_t len);
struct ceb_node *cebuib_pick(struct ceb_root **root, const void *key, size_t len);

/* returns the pointer to the indirect void* key, not the pointer itself,
 * or NULL if node is NULL (note that the indirect pointer cannot be NULL
 * if node is non-NULL).
 */
static inline void *cebib_key(const struct ceb_node *node)
{
	return node ? *(void **)_ceb_key_ptr(node, sizeof(struct ceb_node)) : NULL;
}

/* version taking a key offset */
struct ceb_node *cebib_ofs_insert(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node, size_t len);
struct ceb_node *cebib_ofs_first(struct ceb_root *const *root, ptrdiff_t kofs, size_t len);
struct ceb_node *cebib_ofs_last(struct ceb_root *const *root, ptrdiff_t kofs, size_t len);
struct ceb_node *cebib_ofs_lookup(struct ceb_root *const *root, ptrdiff_t kofs, const void *key, size_t len);
struct ceb_node *cebib_ofs_lookup_le(struct ceb_root *const *root, ptrdiff_t kofs, const void *key, size_t len);
struct ceb_node *cebib_ofs_lookup_lt(struct ceb_root *const *root, ptrdiff_t kofs, const void *key, size_t len);
struct ceb_node *cebib_ofs_lookup_ge(struct ceb_root *const *root, ptrdiff_t kofs, const void *key, size_t len);
struct ceb_node *cebib_ofs_lookup_gt(struct ceb_root *const *root, ptrdiff_t kofs, const void *key, size_t len);
struct ceb_node *cebib_ofs_next_unique(struct ceb_root *const *root, ptrdiff_t kofs, struct ceb_node *node, size_t len);
struct ceb_node *cebib_ofs_prev_unique(struct ceb_root *const *root, ptrdiff_t kofs, struct ceb_node *node, size_t len);
struct ceb_node *cebib_ofs_next_dup(struct ceb_root *const *root, ptrdiff_t kofs, struct ceb_node *node, size_t len);
struct ceb_node *cebib_ofs_prev_dup(struct ceb_root *const *root, ptrdiff_t kofs, struct ceb_node *node, size_t len);
struct ceb_node *cebib_ofs_next(struct ceb_root *const *root, ptrdiff_t kofs, struct ceb_node *node, size_t len);
struct ceb_node *cebib_ofs_prev(struct ceb_root *const *root, ptrdiff_t kofs, struct ceb_node *node, size_t len);
struct ceb_node *cebib_ofs_delete(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node, size_t len);
struct ceb_node *cebib_ofs_pick(struct ceb_root **root, ptrdiff_t kofs, const void *key, size_t len);

struct ceb_node *cebuib_ofs_insert(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node, size_t len);
struct ceb_node *cebuib_ofs_first(struct ceb_root *const *root, ptrdiff_t kofs, size_t len);
struct ceb_node *cebuib_ofs_last(struct ceb_root *const *root, ptrdiff_t kofs, size_t len);
struct ceb_node *cebuib_ofs_lookup(struct ceb_root *const *root, ptrdiff_t kofs, const void *key, size_t len);
struct ceb_node *cebuib_ofs_lookup_le(struct ceb_root *const *root, ptrdiff_t kofs, const void *key, size_t len);
struct ceb_node *cebuib_ofs_lookup_lt(struct ceb_root *const *root, ptrdiff_t kofs, const void *key, size_t len);
struct ceb_node *cebuib_ofs_lookup_ge(struct ceb_root *const *root, ptrdiff_t kofs, const void *key, size_t len);
struct ceb_node *cebuib_ofs_lookup_gt(struct ceb_root *const *root, ptrdiff_t kofs, const void *key, size_t len);
struct ceb_node *cebuib_ofs_next(struct ceb_root *const *root, ptrdiff_t kofs, struct ceb_node *node, size_t len);
struct ceb_node *cebuib_ofs_prev(struct ceb_root *const *root, ptrdiff_t kofs, struct ceb_node *node, size_t len);
struct ceb_node *cebuib_ofs_delete(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node, size_t len);
struct ceb_node *cebuib_ofs_pick(struct ceb_root **root, ptrdiff_t kofs, const void *key, size_t len);

/* returns the pointer to the indirect void* key, not the pointer itself,
 * or NULL if node is NULL (note that the indirect pointer cannot be NULL
 * if node is non-NULL).
 */
static inline void *cebib_ofs_key(const struct ceb_node *node, ptrdiff_t kofs)
{
	return node ? *(void **)_ceb_key_ptr(node, kofs) : NULL;
}

/* insert at root <root>, the item <item_ptr> using node member <nname> and key
 * member <kname>, and length <len> and returns either the inserted item, or
 * the one that was found there.
 */
#define cebib_item_insert(root, nname, kname, len, item_ptr) ({        			\
	ptrdiff_t _nofs = offsetof(typeof(*(item_ptr)), nname);				\
	ptrdiff_t _kofs = offsetof(typeof(*(item_ptr)), kname) - _nofs;			\
	struct ceb_node *_node = cebib_ofs_insert(root, _kofs, &(item_ptr)->nname, len);\
	(typeof(item_ptr))((char *)(_node) - _nofs);					\
})

/* descend root <root>, and return the first item of type <type>, using node
 * member <nname> and key member <kname>, or NULL if the tree is empty.
 */
#define cebib_item_first(root, nname, kname, len, type) ({				\
	ptrdiff_t _nofs = offsetof(type, nname);					\
	ptrdiff_t _kofs = offsetof(type, kname) - _nofs;				\
	struct ceb_node *_node = cebib_ofs_first(root, _kofs, len);			\
	_node ? (type *)((char *)(_node) - _nofs) : NULL;				\
})

/* descend root <root>, and return the last item of type <type>, using node
 * member <nname> and key member <kname>, or NULL if the tree is empty.
 */
#define cebib_item_last(root, nname, kname, len, type) ({				\
	ptrdiff_t _nofs = offsetof(type, nname);					\
	ptrdiff_t _kofs = offsetof(type, kname) - _nofs;				\
	struct ceb_node *_node = cebib_ofs_last(root, _kofs, len);			\
	_node ? (type *)((char *)(_node) - _nofs) : NULL;				\
})

/* lookup from root <root>, the first item of type <type> which contains the
 * key <key>, using node member <nname> and key member <kname>. Returns the
 * pointer to the item, or NULL if not found.
 */
#define cebib_item_lookup(root, nname, kname, key, len, type) ({			\
	ptrdiff_t _nofs = offsetof(type, nname);					\
	ptrdiff_t _kofs = offsetof(type, kname) - _nofs;				\
	struct ceb_node *_node = cebib_ofs_lookup(root, _kofs, key, len);		\
	_node ? (type *)((char *)(_node) - _nofs) : NULL;				\
})

/* lookup from root <root>, the item of type <type> which contains the last of
 * the greatest keys that are lower than or equal to <key>, using node member
 * <nname> and key member <kname>. Returns the pointer to the item, or NULL if
 * not found.
 */
#define cebib_item_lookup_le(root, nname, kname, key, len, type) ({			\
	ptrdiff_t _nofs = offsetof(type, nname);					\
	ptrdiff_t _kofs = offsetof(type, kname) - _nofs;				\
	struct ceb_node *_node = cebib_ofs_lookup_le(root, _kofs, key, len);		\
	_node ? (type *)((char *)(_node) - _nofs) : NULL;				\
})

/* lookup from root <root>, the item of type <type> which contains the last of
 * the greatest keys that are strictly lower than <key>, using node member
 * <nname> and key member <kname>. Returns the pointer to the item, or NULL if
 * not found.
 */
#define cebib_item_lookup_lt(root, nname, kname, key, len, type) ({			\
	ptrdiff_t _nofs = offsetof(type, nname);					\
	ptrdiff_t _kofs = offsetof(type, kname) - _nofs;				\
	struct ceb_node *_node = cebib_ofs_lookup_lt(root, _kofs, key, len);		\
	_node ? (type *)((char *)(_node) - _nofs) : NULL;				\
})

/* lookup from root <root>, the item of type <type> which contains the first of
 * the lowest keys key that are greater than or equal to <key>, using node
 * member <nname> and key member <kname>. Returns the pointer to the item, or
 * NULL if not found.
 */
#define cebib_item_lookup_ge(root, nname, kname, key, len, type) ({			\
	ptrdiff_t _nofs = offsetof(type, nname);					\
	ptrdiff_t _kofs = offsetof(type, kname) - _nofs;				\
	struct ceb_node *_node = cebib_ofs_lookup_ge(root, _kofs, key, len);		\
	_node ? (type *)((char *)(_node) - _nofs) : NULL;				\
})

/* lookup from root <root>, the item of type <type> which contains the first of
 * the lowest keys that are strictly greater than <key>, using node member
 * <nname> and key member <kname>. Returns the pointer to the item, or NULL if
 * not found.
 */
#define cebib_item_lookup_gt(root, nname, kname, key, len, type) ({			\
	ptrdiff_t _nofs = offsetof(type, nname);					\
	ptrdiff_t _kofs = offsetof(type, kname) - _nofs;				\
	struct ceb_node *_node = cebib_ofs_lookup_gt(root, _kofs, key, len);		\
	_node ? (type *)((char *)(_node) - _nofs) : NULL;				\
})

/* lookup from root <root>, the first item of type <type> after item <item_ptr>
 * that has a strictly greater key, using node member <nname> and key member
 * <kname>, and returns either the found item, or NULL if none exists.
 */
#define cebib_item_next_unique(root, nname, kname, len, item_ptr) ({			\
	ptrdiff_t _nofs = offsetof(typeof(*(item_ptr)), nname);				\
	ptrdiff_t _kofs = offsetof(typeof(*(item_ptr)), kname) - _nofs;			\
	struct ceb_node *_node = cebib_ofs_next_unique(root, _kofs, &(item_ptr)->nname, len);	\
	_node ? (typeof(item_ptr))((char *)(_node) - _nofs) : NULL;			\
})

/* lookup from root <root>, the last item of type <type> before item <item_ptr>
 * that has a strictly lower key, using node member <nname> and key member
 * <kname>, and returns either the found item, or NULL if none exists.
 */
#define cebib_item_prev_unique(root, nname, kname, len, item_ptr) ({			\
	ptrdiff_t _nofs = offsetof(typeof(*(item_ptr)), nname);				\
	ptrdiff_t _kofs = offsetof(typeof(*(item_ptr)), kname) - _nofs;			\
	struct ceb_node *_node = cebib_ofs_prev_unique(root, _kofs, &(item_ptr)->nname, len);	\
	_node ? (typeof(item_ptr))((char *)(_node) - _nofs) : NULL;			\
})

/* lookup from root <root>, the first item of type <type> after item <item_ptr>
 * that has exactly the same key, using node member <nname> and key member
 * <kname>, and returns either the found item, or NULL if none exists.
 */
#define cebib_item_next_dup(root, nname, kname, len, item_ptr) ({			\
	ptrdiff_t _nofs = offsetof(typeof(*(item_ptr)), nname);				\
	ptrdiff_t _kofs = offsetof(typeof(*(item_ptr)), kname) - _nofs;			\
	struct ceb_node *_node = cebib_ofs_next_dup(root, _kofs, &(item_ptr)->nname, len);	\
	_node ? (typeof(item_ptr))((char *)(_node) - _nofs) : NULL;			\
})

/* lookup from root <root>, the last item of type <type> before item <item_ptr>
 * that has exactly the same key, using node member <nname> and key member
 * <kname>, and returns either the found item, or NULL if none exists.
 */
#define cebib_item_prev_dup(root, nname, kname, len, item_ptr) ({			\
	ptrdiff_t _nofs = offsetof(typeof(*(item_ptr)), nname);				\
	ptrdiff_t _kofs = offsetof(typeof(*(item_ptr)), kname) - _nofs;			\
	struct ceb_node *_node = cebib_ofs_prev_dup(root, _kofs, &(item_ptr)->nname, len);	\
	_node ? (typeof(item_ptr))((char *)(_node) - _nofs) : NULL;			\
})

/* lookup from root <root>, the first item of type <type> immediately after
 * item <item_ptr>, using node member <nname> and key member <kname>, and
 * returns either the found item, or NULL if none exists.
 */
#define cebib_item_next(root, nname, kname, len, item_ptr) ({				\
	ptrdiff_t _nofs = offsetof(typeof(*(item_ptr)), nname);				\
	ptrdiff_t _kofs = offsetof(typeof(*(item_ptr)), kname) - _nofs;			\
	struct ceb_node *_node = cebib_ofs_next(root, _kofs, &(item_ptr)->nname, len);	\
	_node ? (typeof(item_ptr))((char *)(_node) - _nofs) : NULL;			\
})

/* lookup from root <root>, the last item of type <type> immediately before
 * item <item_ptr>, using node member <nname> and key member <kname>, and
 * returns either the found item, or NULL if none exists.
 */
#define cebib_item_prev(root, nname, kname, len, item_ptr) ({				\
	ptrdiff_t _nofs = offsetof(typeof(*(item_ptr)), nname);				\
	ptrdiff_t _kofs = offsetof(typeof(*(item_ptr)), kname) - _nofs;			\
	struct ceb_node *_node = cebib_ofs_prev(root, _kofs, &(item_ptr)->nname, len);	\
	_node ? (typeof(item_ptr))((char *)(_node) - _nofs) : NULL;			\
})

/* lookup from root <root>, the item <item_ptr> using node member <nname> and
 * key member <kname>, deletes it and returns it. If the item is NULL or absent
 * from the tree, NULL is returned.
 */
#define cebib_item_delete(root, nname, kname, len, item_ptr) ({				\
	ptrdiff_t _nofs = offsetof(typeof(*(item_ptr)), nname);				\
	ptrdiff_t _kofs = offsetof(typeof(*(item_ptr)), kname) - _nofs;			\
	typeof(item_ptr) _item = (item_ptr);						\
	struct ceb_node *_node = cebib_ofs_delete(root, _kofs, _item ? &_item->nname : NULL, len);	\
	_node ? (typeof(item_ptr))((char *)(_node) - _nofs) : NULL;			\
})

/* lookup from root <root>, the first item of type <type> which contains the
 * key <key>, using node member <nname> and key member <kname>, deletes it and
 * returns it. If the key is not found in the tree, NULL is returned.
 */
#define cebib_item_pick(root, nname, kname, key, len, type) ({				\
	ptrdiff_t _nofs = offsetof(type, nname);					\
	ptrdiff_t _kofs = offsetof(type, kname) - _nofs;				\
	struct ceb_node *_node = cebib_ofs_pick(root, _kofs, key, len);			\
	_node ? (type *)((char *)(_node) - _nofs) : NULL;				\
})

/*** versions using unique keys below ***/

/* insert at root <root>, the item <item_ptr> using node member <nname> and key
 * member <kname>, and returns either the inserted item, or the one that was
 * found there.
 */
#define cebuib_item_insert(root, nname, kname, len, item_ptr) ({			\
	ptrdiff_t _nofs = offsetof(typeof(*(item_ptr)), nname);				\
	ptrdiff_t _kofs = offsetof(typeof(*(item_ptr)), kname) - _nofs;			\
	struct ceb_node *_node = cebuib_ofs_insert(root, _kofs, &(item_ptr)->nname, len);	\
	(typeof(item_ptr))((char *)(_node) - _nofs);					\
})

/* descend root <root>, and return the first item of type <type>, using node
 * member <nname> and key member <kname>, or NULL if the tree is empty.
 */
#define cebuib_item_first(root, nname, kname, len, type) ({				\
	ptrdiff_t _nofs = offsetof(type, nname);					\
	ptrdiff_t _kofs = offsetof(type, kname) - _nofs;				\
	struct ceb_node *_node = cebuib_ofs_first(root, _kofs, len);			\
	_node ? (type *)((char *)(_node) - _nofs) : NULL;				\
})

/* descend root <root>, and return the last item of type <type>, using node
 * member <nname> and key member <kname>, or NULL if the tree is empty.
 */
#define cebuib_item_last(root, nname, kname, len, type) ({				\
	ptrdiff_t _nofs = offsetof(type, nname);					\
	ptrdiff_t _kofs = offsetof(type, kname) - _nofs;				\
	struct ceb_node *_node = cebuib_ofs_last(root, _kofs, len);			\
	_node ? (type *)((char *)(_node) - _nofs) : NULL;				\
})

/* lookup from root <root>, the item of type <type> which contains the key
 * <key>, using node member <nname> and key member <kname>. Returns the pointer
 * to the item, or NULL if not found.
 */
#define cebuib_item_lookup(root, nname, kname, key, len, type) ({			\
	ptrdiff_t _nofs = offsetof(type, nname);					\
	ptrdiff_t _kofs = offsetof(type, kname) - _nofs;				\
	struct ceb_node *_node = cebuib_ofs_lookup(root, _kofs, key, len);		\
	_node ? (type *)((char *)(_node) - _nofs) : NULL;				\
})

/* lookup from root <root>, the item of type <type> which contains the greatest
 * key that is lower than or equal to <key>, using node member <nname> and key
 * member <kname>. Returns the pointer to the item, or NULL if not found.
 */
#define cebuib_item_lookup_le(root, nname, kname, key, len, type) ({			\
	ptrdiff_t _nofs = offsetof(type, nname);					\
	ptrdiff_t _kofs = offsetof(type, kname) - _nofs;				\
	struct ceb_node *_node = cebuib_ofs_lookup_le(root, _kofs, key, len);		\
	_node ? (type *)((char *)(_node) - _nofs) : NULL;				\
})

/* lookup from root <root>, the item of type <type> which contains the greatest
 * key that is strictly lower than <key>, using node member <nname> and key
 * member <kname>. Returns the pointer to the item, or NULL if not found.
 */
#define cebuib_item_lookup_lt(root, nname, kname, key, len, type) ({			\
	ptrdiff_t _nofs = offsetof(type, nname);					\
	ptrdiff_t _kofs = offsetof(type, kname) - _nofs;				\
	struct ceb_node *_node = cebuib_ofs_lookup_lt(root, _kofs, key, len);		\
	_node ? (type *)((char *)(_node) - _nofs) : NULL;				\
})

/* lookup from root <root>, the item of type <type> which contains the lowest
 * key key that is greater than or equal to <key>, using node member <nname>
 * and key member <kname>. Returns the pointer to the item, or NULL if not
 * found.
 */
#define cebuib_item_lookup_ge(root, nname, kname, key, len, type) ({			\
	ptrdiff_t _nofs = offsetof(type, nname);					\
	ptrdiff_t _kofs = offsetof(type, kname) - _nofs;				\
	struct ceb_node *_node = cebuib_ofs_lookup_ge(root, _kofs, key, len);		\
	_node ? (type *)((char *)(_node) - _nofs) : NULL;				\
})

/* lookup from root <root>, the item of type <type> which contains the lowest
 * key that is strictly greater than <key>, using node member <nname> and key
 * member <kname>. Returns the pointer to the item, or NULL if not found.
 */
#define cebuib_item_lookup_gt(root, nname, kname, key, len, type) ({			\
	ptrdiff_t _nofs = offsetof(type, nname);					\
	ptrdiff_t _kofs = offsetof(type, kname) - _nofs;				\
	struct ceb_node *_node = cebuib_ofs_lookup_gt(root, _kofs, key, len);		\
	_node ? (type *)((char *)(_node) - _nofs) : NULL;				\
})

/* lookup from root <root>, the first item of type <type> immediately after
 * item <item_ptr>, using node member <nname> and key member <kname>, and
 * returns either the found item, or NULL if none exists.
 */
#define cebuib_item_next(root, nname, kname, len, item_ptr) ({				\
	ptrdiff_t _nofs = offsetof(typeof(*(item_ptr)), nname);				\
	ptrdiff_t _kofs = offsetof(typeof(*(item_ptr)), kname) - _nofs;			\
	struct ceb_node *_node = cebuib_ofs_next(root, _kofs, &(item_ptr)->nname, len);	\
	_node ? (typeof(item_ptr))((char *)(_node) - _nofs) : NULL;			\
})

/* lookup from root <root>, the last item of type <type> immediately before
 * item <item_ptr>, using node member <nname> and key member <kname>, and
 * returns either the found item, or NULL if none exists.
 */
#define cebuib_item_prev(root, nname, kname, len, item_ptr) ({				\
	ptrdiff_t _nofs = offsetof(typeof(*(item_ptr)), nname);				\
	ptrdiff_t _kofs = offsetof(typeof(*(item_ptr)), kname) - _nofs;			\
	struct ceb_node *_node = cebuib_ofs_prev(root, _kofs, &(item_ptr)->nname, len);	\
	_node ? (typeof(item_ptr))((char *)(_node) - _nofs) : NULL;			\
})

/* lookup from root <root>, the item <item_ptr> using node member <nname> and
 * key member <kname>, deletes it and returns it. If the item is NULL or absent
 * from the tree, NULL is returned.
 */
#define cebuib_item_delete(root, nname, kname, len, item_ptr) ({			\
	ptrdiff_t _nofs = offsetof(typeof(*(item_ptr)), nname);				\
	ptrdiff_t _kofs = offsetof(typeof(*(item_ptr)), kname) - _nofs;			\
	typeof(item_ptr) _item = (item_ptr);						\
	struct ceb_node *_node = cebuib_ofs_delete(root, _kofs, _item ? &_item->nname : NULL, len);	\
	_node ? (typeof(item_ptr))((char *)(_node) - _nofs) : NULL;			\
})

/* lookup from root <root>, the first item of type <type> which contains the
 * key <key>, using node member <nname> and key member <kname>, deletes it and
 * returns it. If the key is not found in the tree, NULL is returned.
 */
#define cebuib_item_pick(root, nname, kname, key, len, type) ({				\
	ptrdiff_t _nofs = offsetof(type, nname);					\
	ptrdiff_t _kofs = offsetof(type, kname) - _nofs;				\
	struct ceb_node *_node = cebuib_ofs_pick(root, _kofs, key, len);		\
	_node ? (type *)((char *)(_node) - _nofs) : NULL;				\
})

#endif /* _CEBIB_TREE_H */
