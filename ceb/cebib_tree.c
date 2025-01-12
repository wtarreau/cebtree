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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cebtree-prv.h"
#include "cebib_tree.h"

/*
 *  Below are the functions that support duplicate keys (_ceb_*)
 */

/*****************************************************************************\
 * The declarations below always cause two functions to be declared, one     *
 * starting with "cebs_*" and one with "cebs_ofs_*" which takes a key offset *
 * just after the root. The one without kofs just has this argument omitted  *
 * from its declaration and replaced with sizeof(struct ceb_node) in the     *
 * call to the underlying functions.                                         *
\*****************************************************************************/

/* Inserts node <node> into tree <tree> based on its key whose pointer
 * immediately follows the node and for <len> bytes. Returns the inserted node
 * or the one that already contains the same key.
 */
CEB_FDECL4(struct ceb_node *, cebib, _insert, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node, size_t, len)
{
	const void *key = NODEK(node, kofs)->ptr;
	int is_dup = 0;

	return _ceb_insert(root, node, kofs, CEB_KT_IM, 0, len, key, &is_dup);
}

/* return the first node or NULL if not found. */
CEB_FDECL3(struct ceb_node *, cebib, _first, struct ceb_node **, root, ptrdiff_t, kofs, size_t, len)
{
	int is_dup = 0;

	return _ceb_first(root, kofs, CEB_KT_IM, len, &is_dup);
}

/* return the last node or NULL if not found. */
CEB_FDECL3(struct ceb_node *, cebib, _last, struct ceb_node **, root, ptrdiff_t, kofs, size_t, len)
{
	return _ceb_last(root, kofs, CEB_KT_IM, len);
}

/* look up the specified key <key> of length <len>, and returns either the node
 * containing it, or NULL if not found.
 */
CEB_FDECL4(struct ceb_node *, cebib, _lookup, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key, size_t, len)
{
	return _ceb_lookup(root, kofs, CEB_KT_IM, 0, len, key);
}

/* look up the specified key or the highest below it, and returns either the
 * node containing it, or NULL if not found.
 */
CEB_FDECL4(struct ceb_node *, cebib, _lookup_le, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key, size_t, len)
{
	return _ceb_lookup_le(root, kofs, CEB_KT_IM, 0, len, key);
}

/* look up highest key below the specified one, and returns either the
 * node containing it, or NULL if not found.
 */
CEB_FDECL4(struct ceb_node *, cebib, _lookup_lt, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key, size_t, len)
{
	return _ceb_lookup_lt(root, kofs, CEB_KT_IM, 0, len, key);
}

/* look up the specified key or the smallest above it, and returns either the
 * node containing it, or NULL if not found.
 */
CEB_FDECL4(struct ceb_node *, cebib, _lookup_ge, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key, size_t, len)
{
	return _ceb_lookup_ge(root, kofs, CEB_KT_IM, 0, len, key);
}

/* look up the smallest key above the specified one, and returns either the
 * node containing it, or NULL if not found.
 */
CEB_FDECL4(struct ceb_node *, cebib, _lookup_gt, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key, size_t, len)
{
	return _ceb_lookup_gt(root, kofs, CEB_KT_IM, 0, len, key);
}

/* search for the next node after the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a left turn was made, and returning the first node along the right
 * branch at that fork. The <len> field must correspond to the key length in
 * bytes.
 */
CEB_FDECL4(struct ceb_node *, cebib, _next_unique, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node, size_t, len)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_next_unique(root, kofs, CEB_KT_IM, 0, len, key);
}

/* search for the prev node before the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a right turn was made, and returning the last node along the left
 * branch at that fork. The <len> field must correspond to the key length in
 * bytes.
 */
CEB_FDECL4(struct ceb_node *, cebib, _prev_unique, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node, size_t, len)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_prev_unique(root, kofs, CEB_KT_IM, 0, len, key);
}

/* search for the next node after the specified one containing the same value,
 * and return it, or NULL if not found.
 */
CEB_FDECL4(struct ceb_node *, cebib, _next_dup, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node, size_t, len)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_next_dup(root, kofs, CEB_KT_IM, 0, len, key, node);
}

/* search for the prev node before the specified one containing the same value,
 * and return it, or NULL if not found.
 */
CEB_FDECL4(struct ceb_node *, cebib, _prev_dup, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node, size_t, len)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_prev_dup(root, kofs, CEB_KT_IM, 0, len, key, node);
}

/* search for the next node after the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a left turn was made, and returning the first node along the right
 * branch at that fork. The <len> field must correspond to the key length in
 * bytes.
 */
CEB_FDECL4(struct ceb_node *, cebib, _next, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node, size_t, len)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_next(root, kofs, CEB_KT_IM, 0, len, key, node);
}

/* search for the prev node before the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a right turn was made, and returning the last node along the left
 * branch at that fork. The <len> field must correspond to the key length in
 * bytes.
 */
CEB_FDECL4(struct ceb_node *, cebib, _prev, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node, size_t, len)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_prev(root, kofs, CEB_KT_IM, 0, len, key, node);
}

/* look up the specified node with its key and deletes it if found, and in any
 * case, returns the node. The <len> field must correspond to the key length in
 * bytes.
 */
CEB_FDECL4(struct ceb_node *, cebib, _delete, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node, size_t, len)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_delete(root, node, kofs, CEB_KT_IM, 0, len, key);
}

/* look up the specified key, and detaches it and returns it if found, or NULL
 * if not found. The <len> field must correspond to the key length in bytes.
 */
CEB_FDECL4(struct ceb_node *, cebib, _pick, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key, size_t, len)
{
	return _ceb_delete(root, NULL, kofs, CEB_KT_IM, 0, len, key);
}

/*
 *  Below are the functions that only support unique keys (_cebu_*)
 */

/*****************************************************************************\
 * The declarations below always cause two functions to be declared, one     *
 * starting with "cebuib_*" and one with "cebuib_ofs_*" which takes a key    *
 * offset just after the root. The one without kofs just has this argument   *
 * omitted from its declaration and replaced with sizeof(struct ceb_node) in *
 * the call to the underlying functions.                                     *
\*****************************************************************************/

/* Inserts node <node> into unique tree <tree> based on its key whose pointer
 * immediately follows the node and for <len> bytes. Returns the inserted node
 * or the one that already contains the same key.
 */
CEB_FDECL4(struct ceb_node *, cebuib, _insert, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node, size_t, len)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_insert(root, node, kofs, CEB_KT_IM, 0, len, key, NULL);
}

/* return the first node or NULL if not found. */
CEB_FDECL3(struct ceb_node *, cebuib, _first, struct ceb_node **, root, ptrdiff_t, kofs, size_t, len)
{
	return _ceb_first(root, kofs, CEB_KT_IM, len, NULL);
}

/* return the last node or NULL if not found. */
CEB_FDECL3(struct ceb_node *, cebuib, _last, struct ceb_node **, root, ptrdiff_t, kofs, size_t, len)
{
	return _ceb_last(root, kofs, CEB_KT_IM, len);
}

/* look up the specified key <key> of length <len>, and returns either the node
 * containing it, or NULL if not found.
 */
CEB_FDECL4(struct ceb_node *, cebuib, _lookup, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key, size_t, len)
{
	return _cebu_lookup(root, kofs, CEB_KT_IM, 0, len, key);
}

/* look up the specified key or the highest below it, and returns either the
 * node containing it, or NULL if not found.
 */
CEB_FDECL4(struct ceb_node *, cebuib, _lookup_le, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key, size_t, len)
{
	return _cebu_lookup_le(root, kofs, CEB_KT_IM, 0, len, key);
}

/* look up highest key below the specified one, and returns either the
 * node containing it, or NULL if not found.
 */
CEB_FDECL4(struct ceb_node *, cebuib, _lookup_lt, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key, size_t, len)
{
	return _cebu_lookup_lt(root, kofs, CEB_KT_IM, 0, len, key);
}

/* look up the specified key or the smallest above it, and returns either the
 * node containing it, or NULL if not found.
 */
CEB_FDECL4(struct ceb_node *, cebuib, _lookup_ge, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key, size_t, len)
{
	return _cebu_lookup_ge(root, kofs, CEB_KT_IM, 0, len, key);
}

/* look up the smallest key above the specified one, and returns either the
 * node containing it, or NULL if not found.
 */
CEB_FDECL4(struct ceb_node *, cebuib, _lookup_gt, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key, size_t, len)
{
	return _cebu_lookup_gt(root, kofs, CEB_KT_IM, 0, len, key);
}

/* search for the next node after the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a left turn was made, and returning the first node along the right
 * branch at that fork. The <len> field must correspond to the key length in
 * bytes.
 */
CEB_FDECL4(struct ceb_node *, cebuib, _next, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node, size_t, len)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_next_unique(root, kofs, CEB_KT_IM, 0, len, key);
}

/* search for the prev node before the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a right turn was made, and returning the last node along the left
 * branch at that fork. The <len> field must correspond to the key length in
 * bytes.
 */
CEB_FDECL4(struct ceb_node *, cebuib, _prev, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node, size_t, len)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_prev_unique(root, kofs, CEB_KT_IM, 0, len, key);
}

/* look up the specified node with its key and deletes it if found, and in any
 * case, returns the node. The <len> field must correspond to the key length in
 * bytes.
 */
CEB_FDECL4(struct ceb_node *, cebuib, _delete, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node, size_t, len)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _cebu_delete(root, node, kofs, CEB_KT_IM, 0, len, key);
}

/* look up the specified key, and detaches it and returns it if found, or NULL
 * if not found. The <len> field must correspond to the key length in bytes.
 */
CEB_FDECL4(struct ceb_node *, cebuib, _pick, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key, size_t, len)
{
	return _cebu_delete(root, NULL, kofs, CEB_KT_IM, 0, len, key);
}
