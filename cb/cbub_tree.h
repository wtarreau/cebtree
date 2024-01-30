/*
 * Compact Binary Trees - exported functions for operations on mb keys
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

#include "cbtree.h"

struct cb_node *cbub_insert(struct cb_node **root, struct cb_node *node, size_t len);
struct cb_node *cbub_first(struct cb_node **root);
struct cb_node *cbub_last(struct cb_node **root);
struct cb_node *cbub_lookup(struct cb_node **root, const void *key, size_t len);
struct cb_node *cbub_lookup_le(struct cb_node **root, const void *key, size_t len);
struct cb_node *cbub_lookup_lt(struct cb_node **root, const void *key, size_t len);
struct cb_node *cbub_lookup_ge(struct cb_node **root, const void *key, size_t len);
struct cb_node *cbub_lookup_gt(struct cb_node **root, const void *key, size_t len);
struct cb_node *cbub_next(struct cb_node **root, struct cb_node *node, size_t len);
struct cb_node *cbub_prev(struct cb_node **root, struct cb_node *node, size_t len);
struct cb_node *cbub_delete(struct cb_node **root, struct cb_node *node, size_t len);
struct cb_node *cbub_pick(struct cb_node **root, const void *key, size_t len);
