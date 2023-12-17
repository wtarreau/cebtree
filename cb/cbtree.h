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
static inline struct cb_node *__cb_dotag(const struct cb_node *node)
{
	return (struct cb_node *)((size_t)node + 1);
}

/* untag a tagged pointer */
static inline struct cb_node *__cb_untag(const struct cb_node *node)
{
	return (struct cb_node *)((size_t)node - 1);
}

/* clear a pointer's tag */
static inline struct cb_node *__cb_clrtag(const struct cb_node *node)
{
	return (struct cb_node *)((size_t)node & ~((size_t)1));
}

/* returns whether a pointer is tagged */
static inline int __cb_tagged(const struct cb_node *node)
{
	return !!((size_t)node & 1);
}

/* returns an integer equivalent of the pointer */
static inline size_t __cb_intptr(struct cb_node *tree)
{
	return (size_t)tree;
}

///* returns true if at least one of the branches is a subtree node, indicating
// * that the current node is at the top of a duplicate sub-tree and that all
// * values below it are the same.
// */
//static inline int __cb_is_dup(const struct cb_node *node)
//{
//	return __cb_tagged((struct cb_node *)(__cb_intptr(node->l) | __cb_intptr(node->r)));
//}

#endif /* _CBTREE_H */
