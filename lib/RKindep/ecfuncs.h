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

/* $Id: ecfuncs.h,v 1.2 2003/09/17 15:13:27 aida_s Exp $ */

#ifndef	RKINDEP_ECFUNCS_H
#define RKINDEP_ECFUNCS_H

#ifdef NEED_RKINDEP_SUBST
# include "RKindep/ecfuncs.sub"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_STRLCPY
# define RkiStrlcpy strlcpy
# define RkiStrlcat strlcat
#else
# define RkiStrlcpy RkiAltStrlcpy
# define RkiStrlcat RkiAltStrlcat
extern size_t RkiAltStrlcpy pro((char *dst, const char *src, size_t len));
extern size_t RkiAltStrlcat pro((char *dst, const char *src, size_t len));
#endif

#define RKI_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define RKI_MAX(x, y) (((x) < (y)) ? (y) : (x))

#ifdef __cplusplus
}
#endif

#endif	/* RKINDEP_ECFUNCS_H */
