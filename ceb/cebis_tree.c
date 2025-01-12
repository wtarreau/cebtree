/*
 * Compact Elastic Binary Trees - exported functions operating on indirect strings
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

#ifdef CEBTREE_DEV
/* just to verify API compatibility */
#include "cebis_tree.h"
#endif

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
 * immediately follows the node. Returns the inserted node or the one that
 * already contains the same key.
 */
CEB_FDECL3(struct ceb_node *, cebis, _insert, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node)
{
	const void *key = NODEK(node, kofs)->ptr;
	int is_dup = 0;

	return _ceb_insert(root, node, kofs, CEB_KT_IS, 0, 0, key, &is_dup);
}

/* return the first node or NULL if not found. */
CEB_FDECL2(struct ceb_node *, cebis, _first, struct ceb_node **, root, ptrdiff_t, kofs)
{
	return _ceb_first(root, kofs, CEB_KT_IS, 0);
}

/* return the last node or NULL if not found. */
CEB_FDECL2(struct ceb_node *, cebis, _last, struct ceb_node **, root, ptrdiff_t, kofs)
{
	return _ceb_last(root, kofs, CEB_KT_IS, 0);
}

/* look up the specified key, and returns either the node containing it, or
 * NULL if not found.
 */
CEB_FDECL3(struct ceb_node *, cebis, _lookup, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key)
{
	return _ceb_lookup(root, kofs, CEB_KT_IS, 0, 0, key);
}

/* look up the specified key or the highest below it, and returns either the
 * node containing it, or NULL if not found.
 */
CEB_FDECL3(struct ceb_node *, cebis, _lookup_le, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key)
{
	return _ceb_lookup_le(root, kofs, CEB_KT_IS, 0, 0, key);
}

/* look up highest key below the specified one, and returns either the
 * node containing it, or NULL if not found.
 */
CEB_FDECL3(struct ceb_node *, cebis, _lookup_lt, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key)
{
	return _ceb_lookup_lt(root, kofs, CEB_KT_IS, 0, 0, key);
}

/* look up the specified key or the smallest above it, and returns either the
 * node containing it, or NULL if not found.
 */
CEB_FDECL3(struct ceb_node *, cebis, _lookup_ge, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key)
{
	return _ceb_lookup_ge(root, kofs, CEB_KT_IS, 0, 0, key);
}

/* look up the smallest key above the specified one, and returns either the
 * node containing it, or NULL if not found.
 */
CEB_FDECL3(struct ceb_node *, cebis, _lookup_gt, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key)
{
	return _ceb_lookup_gt(root, kofs, CEB_KT_IS, 0, 0, key);
}

/* search for the next node after the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a left turn was made, and returning the first node along the right
 * branch at that fork.
 */
CEB_FDECL3(struct ceb_node *, cebis, _next_unique, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_next_unique(root, kofs, CEB_KT_IS, 0, 0, key);
}

/* search for the prev node before the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a right turn was made, and returning the last node along the left
 * branch at that fork.
 */
CEB_FDECL3(struct ceb_node *, cebis, _prev_unique, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_prev_unique(root, kofs, CEB_KT_IS, 0, 0, key);
}

/* search for the next node after the specified one containing the same value,
 * and return it, or NULL if not found.
 */
CEB_FDECL3(struct ceb_node *, cebis, _next_dup, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_next_dup(root, kofs, CEB_KT_IS, 0, 0, key, node);
}

/* search for the prev node before the specified one containing the same value,
 * and return it, or NULL if not found.
 */
CEB_FDECL3(struct ceb_node *, cebis, _prev_dup, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_prev_dup(root, kofs, CEB_KT_IS, 0, 0, key, node);
}

/* search for the next node after the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a left turn was made, and returning the first node along the right
 * branch at that fork.
 */
CEB_FDECL3(struct ceb_node *, cebis, _next, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_next(root, kofs, CEB_KT_IS, 0, 0, key, node);
}

/* search for the prev node before the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a right turn was made, and returning the last node along the left
 * branch at that fork.
 */
CEB_FDECL3(struct ceb_node *, cebis, _prev, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_prev(root, kofs, CEB_KT_IS, 0, 0, key, node);
}

/* look up the specified node with its key and deletes it if found, and in any
 * case, returns the node.
 */
CEB_FDECL3(struct ceb_node *, cebis, _delete, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_delete(root, node, kofs, CEB_KT_IS, 0, 0, key);
}

/* look up the specified key, and detaches it and returns it if found, or NULL
 * if not found.
 */
CEB_FDECL3(struct ceb_node *, cebis, _pick, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key)
{
	return _ceb_delete(root, NULL, kofs, CEB_KT_IS, 0, 0, key);
}

/*
 *  Below are the functions that only support unique keys (_cebu_*)
 */

/*****************************************************************************\
 * The declarations below always cause two functions to be declared, one     *
 * starting with "cebuis_*" and one with "cebuis_ofs_*" which takes a key    *
 * offset just after the root. The one without kofs just has this argument   *
 * omitted from its declaration and replaced with sizeof(struct ceb_node) in *
 * the call to the underlying functions.                                     *
\*****************************************************************************/

/* Inserts node <node> into unique tree <tree> based on its key whose pointer
 * immediately follows the node. Returns the inserted node or the one that
 * already contains the same key.
 */
CEB_FDECL3(struct ceb_node *, cebuis, _insert, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _ceb_insert(root, node, kofs, CEB_KT_IS, 0, 0, key, NULL);
}

/* return the first node or NULL if not found. */
CEB_FDECL2(struct ceb_node *, cebuis, _first, struct ceb_node **, root, ptrdiff_t, kofs)
{
	return _cebu_first(root, kofs, CEB_KT_IS, 0);
}

/* return the last node or NULL if not found. */
CEB_FDECL2(struct ceb_node *, cebuis, _last, struct ceb_node **, root, ptrdiff_t, kofs)
{
	return _cebu_last(root, kofs, CEB_KT_IS, 0);
}

/* look up the specified key, and returns either the node containing it, or
 * NULL if not found.
 */
CEB_FDECL3(struct ceb_node *, cebuis, _lookup, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key)
{
	return _cebu_lookup(root, kofs, CEB_KT_IS, 0, 0, key);
}

/* look up the specified key or the highest below it, and returns either the
 * node containing it, or NULL if not found.
 */
CEB_FDECL3(struct ceb_node *, cebuis, _lookup_le, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key)
{
	return _cebu_lookup_le(root, kofs, CEB_KT_IS, 0, 0, key);
}

/* look up highest key below the specified one, and returns either the
 * node containing it, or NULL if not found.
 */
CEB_FDECL3(struct ceb_node *, cebuis, _lookup_lt, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key)
{
	return _cebu_lookup_lt(root, kofs, CEB_KT_IS, 0, 0, key);
}

/* look up the specified key or the smallest above it, and returns either the
 * node containing it, or NULL if not found.
 */
CEB_FDECL3(struct ceb_node *, cebuis, _lookup_ge, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key)
{
	return _cebu_lookup_ge(root, kofs, CEB_KT_IS, 0, 0, key);
}

/* look up the smallest key above the specified one, and returns either the
 * node containing it, or NULL if not found.
 */
CEB_FDECL3(struct ceb_node *, cebuis, _lookup_gt, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key)
{
	return _cebu_lookup_gt(root, kofs, CEB_KT_IS, 0, 0, key);
}

/* search for the next node after the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a left turn was made, and returning the first node along the right
 * branch at that fork.
 */
CEB_FDECL3(struct ceb_node *, cebuis, _next, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _cebu_next(root, kofs, CEB_KT_IS, 0, 0, key);
}

/* search for the prev node before the specified one, and return it, or NULL if
 * not found. The approach consists in looking up that node, recalling the last
 * time a right turn was made, and returning the last node along the left
 * branch at that fork.
 */
CEB_FDECL3(struct ceb_node *, cebuis, _prev, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _cebu_prev(root, kofs, CEB_KT_IS, 0, 0, key);
}

/* look up the specified node with its key and deletes it if found, and in any
 * case, returns the node.
 */
CEB_FDECL3(struct ceb_node *, cebuis, _delete, struct ceb_node **, root, ptrdiff_t, kofs, struct ceb_node *, node)
{
	const void *key = NODEK(node, kofs)->ptr;

	return _cebu_delete(root, node, kofs, CEB_KT_IS, 0, 0, key);
}

/* look up the specified key, and detaches it and returns it if found, or NULL
 * if not found.
 */
CEB_FDECL3(struct ceb_node *, cebuis, _pick, struct ceb_node **, root, ptrdiff_t, kofs, const void *, key)
{
	return _cebu_delete(root, NULL, kofs, CEB_KT_IS, 0, 0, key);
}

/* dumps a ceb_node tree using the default functions above. If a node matches
 * <ctx>, this one will be highlighted in red. If the <sub> value is non-null,
 * only a subgraph will be printed. If it's null, and root is non-null, then
 * the tree is dumped at once, otherwise if root is NULL, then a prologue is
 * dumped when label is not NULL, or the epilogue when label is NULL. As a
 * summary:
 *    sub  root label
 *     0   NULL NULL   epilogue only (closing brace and LF)
 *     0   NULL text   prologue with <text> as label
 *     0   tree *      prologue+tree+epilogue at once
 *    N>0  tree *      only the tree, after a prologue and before an epilogue
 */
CEB_FDECL5(void, cebis, _default_dump, struct ceb_node **, root, ptrdiff_t, kofs, const char *, label, const void *, ctx, int, sub)
{
	if (!sub && label) {
		printf("\ndigraph cebis_tree {\n"
		       "  fontname=\"fixed\";\n"
		       "  fontsize=8\n"
		       "  label=\"%s\"\n"
		       "", label);

		printf("  node [fontname=\"fixed\" fontsize=8 shape=\"box\" style=\"filled\" color=\"black\" fillcolor=\"white\"];\n"
		       "  edge [fontname=\"fixed\" fontsize=8 style=\"solid\" color=\"magenta\" dir=\"forward\"];\n");
	} else
		printf("\n### sub %d ###\n\n", sub);

	if (root)
		ceb_default_dump_tree(kofs, CEB_KT_IS, root, 0, NULL, 0, ctx, sub, NULL, NULL, NULL, NULL);

	if (!sub && (root || !label))
		printf("}\n");
}
