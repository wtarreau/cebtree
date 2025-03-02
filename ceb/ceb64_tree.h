/*
 * Compact Elastic Binary Trees - exported functions for operations on u64 keys
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

#ifndef _CEB64_TREE_H
#define _CEB64_TREE_H

#include "cebtree.h"
#include <inttypes.h>

/* simpler version */
struct ceb_node *ceb64_insert(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *ceb64_first(struct ceb_root **root);
struct ceb_node *ceb64_last(struct ceb_root **root);
struct ceb_node *ceb64_lookup(struct ceb_root **root, uint64_t key);
struct ceb_node *ceb64_lookup_le(struct ceb_root **root, uint64_t key);
struct ceb_node *ceb64_lookup_lt(struct ceb_root **root, uint64_t key);
struct ceb_node *ceb64_lookup_ge(struct ceb_root **root, uint64_t key);
struct ceb_node *ceb64_lookup_gt(struct ceb_root **root, uint64_t key);
struct ceb_node *ceb64_next_unique(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *ceb64_prev_unique(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *ceb64_next_dup(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *ceb64_prev_dup(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *ceb64_next(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *ceb64_prev(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *ceb64_delete(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *ceb64_pick(struct ceb_root **root, uint64_t key);

struct ceb_node *cebu64_insert(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebu64_first(struct ceb_root **root);
struct ceb_node *cebu64_last(struct ceb_root **root);
struct ceb_node *cebu64_lookup(struct ceb_root **root, uint64_t key);
struct ceb_node *cebu64_lookup_le(struct ceb_root **root, uint64_t key);
struct ceb_node *cebu64_lookup_lt(struct ceb_root **root, uint64_t key);
struct ceb_node *cebu64_lookup_ge(struct ceb_root **root, uint64_t key);
struct ceb_node *cebu64_lookup_gt(struct ceb_root **root, uint64_t key);
struct ceb_node *cebu64_next(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebu64_prev(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebu64_delete(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebu64_pick(struct ceb_root **root, uint64_t key);

/* generic dump function */
void ceb64_default_dump(struct ceb_root **ceb_root, const char *label, const void *ctx, int sub);

/* version taking a key offset */
struct ceb_node *ceb64_ofs_insert(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *ceb64_ofs_first(struct ceb_root **root, ptrdiff_t kofs);
struct ceb_node *ceb64_ofs_last(struct ceb_root **root, ptrdiff_t kofs);
struct ceb_node *ceb64_ofs_lookup(struct ceb_root **root, ptrdiff_t kofs, uint64_t key);
struct ceb_node *ceb64_ofs_lookup_le(struct ceb_root **root, ptrdiff_t kofs, uint64_t key);
struct ceb_node *ceb64_ofs_lookup_lt(struct ceb_root **root, ptrdiff_t kofs, uint64_t key);
struct ceb_node *ceb64_ofs_lookup_ge(struct ceb_root **root, ptrdiff_t kofs, uint64_t key);
struct ceb_node *ceb64_ofs_lookup_gt(struct ceb_root **root, ptrdiff_t kofs, uint64_t key);
struct ceb_node *ceb64_ofs_next_unique(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *ceb64_ofs_prev_unique(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *ceb64_ofs_next_dup(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *ceb64_ofs_prev_dup(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *ceb64_ofs_next(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *ceb64_ofs_prev(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *ceb64_ofs_delete(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *ceb64_ofs_pick(struct ceb_root **root, ptrdiff_t kofs, uint64_t key);

struct ceb_node *cebu64_ofs_insert(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebu64_ofs_first(struct ceb_root **root, ptrdiff_t kofs);
struct ceb_node *cebu64_ofs_last(struct ceb_root **root, ptrdiff_t kofs);
struct ceb_node *cebu64_ofs_lookup(struct ceb_root **root, ptrdiff_t kofs, uint64_t key);
struct ceb_node *cebu64_ofs_lookup_le(struct ceb_root **root, ptrdiff_t kofs, uint64_t key);
struct ceb_node *cebu64_ofs_lookup_lt(struct ceb_root **root, ptrdiff_t kofs, uint64_t key);
struct ceb_node *cebu64_ofs_lookup_ge(struct ceb_root **root, ptrdiff_t kofs, uint64_t key);
struct ceb_node *cebu64_ofs_lookup_gt(struct ceb_root **root, ptrdiff_t kofs, uint64_t key);
struct ceb_node *cebu64_ofs_next(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebu64_ofs_prev(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebu64_ofs_delete(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebu64_ofs_pick(struct ceb_root **root, ptrdiff_t kofs, uint64_t key);

/* generic dump function taking a key offset */
void ceb64_ofs_default_dump(struct ceb_root **root, ptrdiff_t kofs, const char *label, const void *ctx, int sub);

#endif /* _CEB64_TREE_H */
