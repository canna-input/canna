/* Copyright (c) 2003 Canna Project. All rights reserved.
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

/* $Id: cfuncs.h,v 1.1.2.1 2003/12/27 17:15:24 aida_s Exp $ */

#ifndef	RKINDEP_CFUNCS_H
#define RKINDEP_CFUNCS_H

#ifdef NEED_RKINDEP_SUBST
# include "RKindep/cfuncs.sub"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAVE_MALLOC
# define HAVE_MALLOC 0
#endif
#if !HAVE_MALLOC
extern void *RkiMalloc pro((size_t size));
extern void *RkiCalloc pro((size_t num, size_t size));
# define malloc(size) RkiMalloc(size)
# define calloc(num, size) RkiCalloc(num, size)
#endif

#ifndef HAVE_MEMSET
extern void *RkiMemset pro((void *ptr, int ch, size_t size));
# define memset(buf, ch, size) RkiMemset(buf, ch, size)
#endif

#ifndef HAVE_STRDUP
extern char *RkiStrdup pro((const char *str));
# define strdup(s) RkiStrdup(s)
#endif

#ifdef __cplusplus
}
#endif

#endif	/* RKINDEP_CFUNCS_H */
