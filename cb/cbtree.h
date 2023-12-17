/*
 * Compact Binary Trees - exported functions for operations on node's address
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

#ifndef _CBTREE_H
#define _CBTREE_H

#include <stddef.h>
#include "../common/tools.h"

/* Standard node when using absolute pointers */
struct cb_node {
	struct cb_node *b[2]; /* branches: 0=left, 1=right */
};

/* indicates whether a valid node is in a tree or not */
static inline int cb_intree(const struct cb_node *node)
{
	return !!node->b[0];
}

/* tag an untagged pointer */
static inline struct cb_node *__cba_dotag(const struct cb_node *node)
{
	return (struct cb_node *)((size_t)node + 1);
}

/* untag a tagged pointer */
static inline struct cb_node *__cba_untag(const struct cb_node *node)
{
	return (struct cb_node *)((size_t)node - 1);
}

/* clear a pointer's tag */
static inline struct cb_node *__cba_clrtag(const struct cb_node *node)
{
	return (struct cb_node *)((size_t)node & ~((size_t)1));
}

/* returns whether a pointer is tagged */
static inline int __cba_tagged(const struct cb_node *node)
{
	return !!((size_t)node & 1);
}

/* returns an integer equivalent of the pointer */
static inline size_t __cba_intptr(struct cb_node *tree)
{
	return (size_t)tree;
}

///* returns true if at least one of the branches is a subtree node, indicating
// * that the current node is at the top of a duplicate sub-tree and that all
// * values below it are the same.
// */
//static inline int __cba_is_dup(const struct cb_node *node)
//{
//	return __cba_tagged((struct cb_node *)(__cba_intptr(node->l) | __cba_intptr(node->r)));
//}

///* Returns the type of the branch pointed to by <tree> among CB_TYPE_LEAF and
// * CB_TYPE_NODE.
// */
//static inline size_t __cba_get_branch_type(struct cb_node * tree)
//{
//	return (size_t)tree & CB_TYPE_MASK;
//}
//
///* Converts a cb_node pointer to its equivalent tagged value for use in ->l/r.
// * NULL is not preserved. <tag> must be either CB_TYPE_LEAF or CB_TYPE_NODE.
// */
//static inline struct cb_node * __cba_dotag(struct cb_node *node, size_t tag)
//{
//	return (struct cb_node *)((char *)node + tag);
//}
//
///* Converts a struct cb_node * to its equivalent untagged pointer. NULL is preserved.
// * <tag> must be either CB_TYPE_LEAF or CB_TYPE_NODE.
// */
//static inline struct cb_node *__cba_untag(struct cb_node * tree, size_t tag)
//{
//	return (struct cb_node *)((char *)tree - tag);
//}

//
//struct cb_node *cba_insert(struct cb_node **root, struct cb_node *data);
//struct cb_node *cba_lookup(struct cb_node **root, void *data);
//struct cb_node *cba_lookup_le(struct cb_node **root, void *data);
//struct cb_node *cba_lookup_ge(struct cb_node **root, void *data);
//void *cba_dump_tree(struct cb_node *node, unsigned long pxor, void *last,
//                    int level,
//                    void (*node_dump)(struct cb_node *node, int level),
//                    void (*leaf_dump)(struct cb_node *node, int level));
//
#endif /* _CBTREE_H */
