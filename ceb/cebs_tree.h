/*
 * Compact Elastic Binary Trees - exported functions operating on string keys
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

#ifndef _CEBS_TREE_H
#define _CEBS_TREE_H

#include "cebtree.h"

/* simpler version */
struct ceb_node *cebs_insert(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebs_first(struct ceb_root **root);
struct ceb_node *cebs_last(struct ceb_root **root);
struct ceb_node *cebs_lookup(struct ceb_root **root, const void *key);
struct ceb_node *cebs_lookup_le(struct ceb_root **root, const void *key);
struct ceb_node *cebs_lookup_lt(struct ceb_root **root, const void *key);
struct ceb_node *cebs_lookup_ge(struct ceb_root **root, const void *key);
struct ceb_node *cebs_lookup_gt(struct ceb_root **root, const void *key);
struct ceb_node *cebs_next_unique(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebs_prev_unique(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebs_next_dup(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebs_prev_dup(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebs_next(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebs_prev(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebs_delete(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebs_pick(struct ceb_root **root, const void *key);

struct ceb_node *cebus_insert(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebus_first(struct ceb_root **root);
struct ceb_node *cebus_last(struct ceb_root **root);
struct ceb_node *cebus_lookup(struct ceb_root **root, const void *key);
struct ceb_node *cebus_lookup_le(struct ceb_root **root, const void *key);
struct ceb_node *cebus_lookup_lt(struct ceb_root **root, const void *key);
struct ceb_node *cebus_lookup_ge(struct ceb_root **root, const void *key);
struct ceb_node *cebus_lookup_gt(struct ceb_root **root, const void *key);
struct ceb_node *cebus_next(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebus_prev(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebus_delete(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebus_pick(struct ceb_root **root, const void *key);

/* generic dump function */
void cebs_default_dump(struct ceb_root **root, const char *label, const void *ctx, int sub);

/* version taking a key offset */
struct ceb_node *cebs_ofs_insert(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebs_ofs_first(struct ceb_root **root, ptrdiff_t kofs);
struct ceb_node *cebs_ofs_last(struct ceb_root **root, ptrdiff_t kofs);
struct ceb_node *cebs_ofs_lookup(struct ceb_root **root, ptrdiff_t kofs, const void *key);
struct ceb_node *cebs_ofs_lookup_le(struct ceb_root **root, ptrdiff_t kofs, const void *key);
struct ceb_node *cebs_ofs_lookup_lt(struct ceb_root **root, ptrdiff_t kofs, const void *key);
struct ceb_node *cebs_ofs_lookup_ge(struct ceb_root **root, ptrdiff_t kofs, const void *key);
struct ceb_node *cebs_ofs_lookup_gt(struct ceb_root **root, ptrdiff_t kofs, const void *key);
struct ceb_node *cebs_ofs_next_unique(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebs_ofs_prev_unique(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebs_ofs_next_dup(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebs_ofs_prev_dup(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebs_ofs_next(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebs_ofs_prev(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebs_ofs_delete(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebs_ofs_pick(struct ceb_root **root, ptrdiff_t kofs, const void *key);

struct ceb_node *cebus_ofs_insert(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebus_ofs_first(struct ceb_root **root, ptrdiff_t kofs);
struct ceb_node *cebus_ofs_last(struct ceb_root **root, ptrdiff_t kofs);
struct ceb_node *cebus_ofs_lookup(struct ceb_root **root, ptrdiff_t kofs, const void *key);
struct ceb_node *cebus_ofs_lookup_le(struct ceb_root **root, ptrdiff_t kofs, const void *key);
struct ceb_node *cebus_ofs_lookup_lt(struct ceb_root **root, ptrdiff_t kofs, const void *key);
struct ceb_node *cebus_ofs_lookup_ge(struct ceb_root **root, ptrdiff_t kofs, const void *key);
struct ceb_node *cebus_ofs_lookup_gt(struct ceb_root **root, ptrdiff_t kofs, const void *key);
struct ceb_node *cebus_ofs_next(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebus_ofs_prev(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebus_ofs_delete(struct ceb_root **root, ptrdiff_t kofs, struct ceb_node *node);
struct ceb_node *cebus_ofs_pick(struct ceb_root **root, ptrdiff_t kofs, const void *key);

/* generic dump function taking a key offset */
void cebs_ofs_default_dump(struct ceb_root **root, ptrdiff_t kofs, const char *label, const void *ctx, int sub);

#endif /* _CEBS_TREE_H */
