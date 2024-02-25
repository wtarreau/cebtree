/*
 * Compact Binary Trees - exported functions for operations on string keys
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

struct cb_node *cbus_insert(struct cb_node **root, struct cb_node *node);
struct cb_node *cbus_first(struct cb_node **root);
struct cb_node *cbus_last(struct cb_node **root);
struct cb_node *cbus_lookup(struct cb_node **root, const void *key);
struct cb_node *cbus_lookup_le(struct cb_node **root, const void *key);
struct cb_node *cbus_lookup_lt(struct cb_node **root, const void *key);
struct cb_node *cbus_lookup_ge(struct cb_node **root, const void *key);
struct cb_node *cbus_lookup_gt(struct cb_node **root, const void *key);
struct cb_node *cbus_next(struct cb_node **root, struct cb_node *node);
struct cb_node *cbus_prev(struct cb_node **root, struct cb_node *node);
struct cb_node *cbus_delete(struct cb_node **root, struct cb_node *node);
struct cb_node *cbus_pick(struct cb_node **root, const void *key);
void cbus_default_dump(struct cb_node **cb_root, const char *label, const void *ctx);
