/* Copyright (c) 2002 Canna Project. All rights reserved.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of the
 * author and contributors not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written
 * prior permission.  The author and contributors no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * THE AUTHOR AND CONTRIBUTORS DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE AUTHOR AND CONTRIBUTORS BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTUOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */

/* $Id: ccompat.h,v 1.1 2002/10/20 14:29:56 aida_s Exp $ */

#ifndef CCOMPAT_H
#define CCOMPAT_H

#if defined(__STDC__) || defined(WIN)
# define HAVE_STRING_H
# define HAVE_STRCHR
# define HAVE_STRRCHR
# define HAVE_MEMSET
# define HAVE_MEMCPY
# define HAVE_STDLIB_H
#elif defined(SYSV) || defined(SVR4)
# define HAVE_STRING_H
# define HAVE_STRCHR
# define HAVE_STRRCHR
# define HAVE_MEMSET
# define HAVE_MEMCPY
# define NEED_MEMORY_H
#else /* others */
# if defined(USG)
#  define HAVE_STRING_H
#  define HAVE_STRCHR
#  define HAVE_STRRCHR
# endif
#endif /* others */

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
# define HAVE_INDEX
# define HAVE_RINDEX
# define HAVE_BZERO
# define HAVE_BCOPY
#elif defined(__EMX__)
# define HAVE_BZERO
# define HAVE_BCOPY
#endif

#if !defined(__STDC__) && !defined(WIN)
# if defined(__GNUC__)
#  define const __const__
# else
#  define const
# endif
#endif

#if defined(__STDC__) || defined(__cplusplus) || defined(WIN)
# define pro(x) x
#else
# define pro(x) ()
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#else
extern char *malloc(), *realloc(), *calloc();
extern void free();
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif
#ifdef NEED_MEMORY_H
# include <memory.h>
#endif

#if defined(HAVE_STRCHR) && !defined(HAVE_INDEX) && !defined(index)
# define index strchr
#elif !defined(HAVE_STRCHR) && defined(HAVE_INDEX) && !defined(strchr)
# define strchr index
#endif
#if defined(HAVE_STRRCHR) && !defined(HAVE_RINDEX) && !defined(rindex)
# define rindex strrchr
#elif !defined(HAVE_STRRCHR) && defined(HAVE_INDEX) && !defined(strrchr)
# define strrchr rindex
#endif

#if defined(HAVE_MEMSET) && !defined(HAVE_BZERO) && !defined(bzero)
# define bzero(buf, size) memset((char *)(buf), 0x00, (size))
#endif
#if defined(HAVE_MEMCPY) && !defined(HAVE_BCOPY) && !defined(bcopy)
# define bcopy(src, dst, size) memcpy((char *)(dst), (char *)(src), (size))
#elif !defined(HAVE_MEMCPY) && defined(HAVE_BCOPY) && !defined(memcpy)
# define memcpy(dst, src, size) bcopy((char *)(src), (char *)(dst), (size))
#endif

#endif /* CCOMPAT_H */
