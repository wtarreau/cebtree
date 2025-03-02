/*
 * Compact Elastic Binary Trees - exported functions operating on ulong keys
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

#ifndef _CEBL_TREE_H
#define _CEBL_TREE_H

#include "cebtree.h"

/* simpler version */
struct ceb_node *cebl_insert(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebl_first(struct ceb_root **root);
struct ceb_node *cebl_last(struct ceb_root **root);
struct ceb_node *cebl_lookup(struct ceb_root **root, unsigned long key);
struct ceb_node *cebl_lookup_le(struct ceb_root **root, unsigned long key);
struct ceb_node *cebl_lookup_lt(struct ceb_root **root, unsigned long key);
struct ceb_node *cebl_lookup_ge(struct ceb_root **root, unsigned long key);
struct ceb_node *cebl_lookup_gt(struct ceb_root **root, unsigned long key);
struct ceb_node *cebl_next_unique(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebl_prev_unique(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebl_next_dup(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebl_prev_dup(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebl_next(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebl_prev(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebl_delete(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebl_pick(struct ceb_root **root, unsigned long key);

struct ceb_node *cebul_insert(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebul_first(struct ceb_root **root);
struct ceb_node *cebul_last(struct ceb_root **root);
struct ceb_node *cebul_lookup(struct ceb_root **root, unsigned long key);
struct ceb_node *cebul_lookup_le(struct ceb_root **root, unsigned long key);
struct ceb_node *cebul_lookup_lt(struct ceb_root **root, unsigned long key);
struct ceb_node *cebul_lookup_ge(struct ceb_root **root, unsigned long key);
struct ceb_node *cebul_lookup_gt(struct ceb_root **root, unsigned long key);
struct ceb_node *cebul_next(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebul_prev(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebul_delete(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebul_pick(struct ceb_root **root, unsigned long key);

/* generic dump function */
void cebl_default_dump(struct ceb_root **ceb_root, const char *label, const void *ctx, int sub);

/* version taking a key offset */
struct ceb_node *cebl_ofs_insert(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebl_ofs_first(struct ceb_root **root, ptrdiff_t kofs);
struct ceb_node *cebl_ofs_last(struct ceb_root **root, ptrdiff_t kofs);
struct ceb_node *cebl_ofs_lookup(struct ceb_root **root, ptrdiff_t kofs, unsigned long key);
struct ceb_node *cebl_ofs_lookup_le(struct ceb_root **root, ptrdiff_t kofs, unsigned long key);
struct ceb_node *cebl_ofs_lookup_lt(struct ceb_root **root, ptrdiff_t kofs, unsigned long key);
struct ceb_node *cebl_ofs_lookup_ge(struct ceb_root **root, ptrdiff_t kofs, unsigned long key);
struct ceb_node *cebl_ofs_lookup_gt(struct ceb_root **root, ptrdiff_t kofs, unsigned long key);
struct ceb_node *cebl_ofs_next_unique(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebl_ofs_prev_unique(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebl_ofs_next_dup(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebl_ofs_prev_dup(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebl_ofs_next(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebl_ofs_prev(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebl_ofs_delete(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebl_ofs_pick(struct ceb_root **root, ptrdiff_t kofs, unsigned long key);

struct ceb_node *cebul_ofs_insert(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebul_ofs_first(struct ceb_root **root, ptrdiff_t kofs);
struct ceb_node *cebul_ofs_last(struct ceb_root **root, ptrdiff_t kofs);
struct ceb_node *cebul_ofs_lookup(struct ceb_root **root, ptrdiff_t kofs, unsigned long key);
struct ceb_node *cebul_ofs_lookup_le(struct ceb_root **root, ptrdiff_t kofs, unsigned long key);
struct ceb_node *cebul_ofs_lookup_lt(struct ceb_root **root, ptrdiff_t kofs, unsigned long key);
struct ceb_node *cebul_ofs_lookup_ge(struct ceb_root **root, ptrdiff_t kofs, unsigned long key);
struct ceb_node *cebul_ofs_lookup_gt(struct ceb_root **root, ptrdiff_t kofs, unsigned long key);
struct ceb_node *cebul_ofs_next(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebul_ofs_prev(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebul_ofs_delete(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebul_ofs_pick(struct ceb_root **root, ptrdiff_t kofs, unsigned long key);

/* generic dump function taking a key offset */
void cebl_ofs_default_dump(struct ceb_root **root, ptrdiff_t kofs, const char *label, const void *ctx, int sub);

#endif /* _CEBL_TREE_H */
