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

/* $Id: cksum.h,v 1.3 2003/09/25 14:37:24 aida_s Exp $ */

#ifndef	RKINDEP_CKSUM_H
#define RKINDEP_CKSUM_H

#ifdef NEED_RKINDEP_SUBST
# include "RKindep/cksum.sub"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  canna_uint32_t curr;
  size_t len;
} RkiCksumCalc;

/*
 * only ISO/IEC 8802-3:1989 CRC (==POSIX 1003.2 cksum) is supported for now
 */

extern int RkiCksumCRCInit pro((RkiCksumCalc *cx));
extern int RkiCksumAdd pro((RkiCksumCalc *cx, const void *data, size_t len));
extern canna_uint32_t RkiCksumCRCFinish pro((RkiCksumCalc *cx));

#ifdef __cplusplus
}
#endif

#endif	/* RKINDEP_CKSUM_H */
/* vim: set sw=2: */
