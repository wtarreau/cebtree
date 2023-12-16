/*
 * Compact Binary Trees - internal functions and types
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

/* This file MUST NOT be included by public code, it contains macros, enums
 * with short names and function definitions that may clash with user code.
 * It may only be included by the respective types' C files.
 */

#ifndef _CBATREE_PRV_H
#define _CBATREE_PRV_H

/* tree walk method: key, left, right */
enum cba_walk_meth {
	CB_WM_KEY,     /* look up the node's key */
	CB_WM_FST,     /* look up "first" (walk left only) */
	CB_WM_NXT,     /* look up "next" (walk right once then left) */
	CB_WM_PRV,     /* look up "prev" (walk left once then right) */
	CB_WM_LST,     /* look up "last" (walk right only) */
};

#endif /* _CBATREE_PRV_H */
