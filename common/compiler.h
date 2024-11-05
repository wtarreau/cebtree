/*
 * ebtree/compiler.h
 * This files contains some compiler-specific settings.
 *
 * Copyright (C) 2000-2015 Willy Tarreau - w@1wt.eu
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

#ifndef _EBTREE_COMPILER_H
#define _EBTREE_COMPILER_H


/*
 * Gcc before 3.0 needs [0] to declare a variable-size array
 */
#ifndef VAR_ARRAY
#if  __GNUC__  < 3
#define VAR_ARRAY	0
#else
#define VAR_ARRAY
#endif
#endif


/* Support passing function parameters in registers. For this, the
 * CONFIG_REGPARM macro has to be set to the maximal number of registers
 * allowed. Some functions have intentionally received a regparm lower than
 * their parameter count, it is in order to avoid register clobbering where
 * they are called.
 */
#ifndef REGPRM1
#if CONFIG_REGPARM >= 1 && __GNUC__ >= 3
#define REGPRM1	__attribute__((regparm(1)))
#else
#define REGPRM1
#endif
#endif

#ifndef REGPRM2
#if CONFIG_REGPARM >= 2 && __GNUC__ >= 3
#define REGPRM2	__attribute__((regparm(2)))
#else
#define REGPRM2 REGPRM1
#endif
#endif

#ifndef REGPRM3
#if CONFIG_REGPARM >= 3 && __GNUC__ >= 3
#define REGPRM3	__attribute__((regparm(3)))
#else
#define REGPRM3 REGPRM2
#endif
#endif


/* By default, gcc does not inline large chunks of code, but we want it to
 * respect our choices.
 */
#if !defined(forceinline)
#if __GNUC__ < 3
#define forceinline inline
#else
#define forceinline inline __attribute__((always_inline))
#endif
#endif


/*
 * Gcc >= 3 provides the ability for the programme to give hints to the
 * compiler about what branch of an if is most likely to be taken. This
 * helps the compiler produce the most compact critical paths, which is
 * generally better for the cache and to reduce the number of jumps.
 */
#if !defined(likely)
#if __GNUC__ < 3
#define __builtin_expect(x,y) (x)
#define likely(x) (x)
#define unlikely(x) (x)
#elif __GNUC__ < 4
/* gcc 3.x does the best job at this */
#define likely(x) (__builtin_expect((x) != 0, 1))
#define unlikely(x) (__builtin_expect((x) != 0, 0))
#else
/* GCC 4.x is stupid, it performs the comparison then compares it to 1,
 * so we cheat in a dirty way to prevent it from doing this. This will
 * only work with ints and booleans though.
 */
#define likely(x) (x)
#define unlikely(x) (__builtin_expect((unsigned long)(x), 0))
#endif
#endif

/* Define the missing __builtin_prefetch() for tcc. */
#if defined(__TINYC__)
#define __builtin_prefetch(addr, ...) do { } while (0)
#endif


#endif /* _EBTREE_COMPILER_H */
