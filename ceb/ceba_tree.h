/*
 * Compact Elastic Binary Trees - exported functions operating on addr keys
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

#ifndef _CEBA_TREE_H
#define _CEBA_TREE_H

#include "cebtree.h"

/* simpler version */
struct ceb_node *cebua_insert(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebua_first(struct ceb_root **root);
struct ceb_node *cebua_last(struct ceb_root **root);
struct ceb_node *cebua_lookup(struct ceb_root **root, const void *key);
struct ceb_node *cebua_lookup_le(struct ceb_root **root, const void *key);
struct ceb_node *cebua_lookup_lt(struct ceb_root **root, const void *key);
struct ceb_node *cebua_lookup_ge(struct ceb_root **root, const void *key);
struct ceb_node *cebua_lookup_gt(struct ceb_root **root, const void *key);
struct ceb_node *cebua_next(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebua_prev(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebua_delete(struct ceb_root **root, struct ceb_node *node);
struct ceb_node *cebua_pick(struct ceb_root **root, const void *key);

/* generic dump function */
void ceba_default_dump(struct ceb_root **root, const char *label, const void *ctx, int sub);

#endif /* _CEBA_TREE_H */
