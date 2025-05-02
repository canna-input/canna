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

/* $Id: strops.h,v 1.2 2003/09/06 13:59:33 aida_s Exp $ */

#ifndef	RKINDEP_STROPS_H
#define RKINDEP_STROPS_H

#ifdef NEED_RKINDEP_SUBST
# include "RKindep/strops.sub"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  /*
   * They are "public" members.
   * If and only if sb_buf == NULL, sb_curr and sb_end are NULL.
   * Otherwise sb_buf and sb_curr must be valid pointers.
   */
  char *sb_buf;
  char *sb_curr;
  char *sb_end;
} RkiStrbuf;

extern void RkiStrbuf_init pro((RkiStrbuf *sb));
extern void RkiStrbuf_destroy pro((RkiStrbuf *sb));
extern void RkiStrbuf_clear pro((RkiStrbuf *sb));
#define RKI_STRBUF_RESERVE(sb, size) \
  (((sb)->sb_curr + (size) < (sb)->sb_end) \
    ? 0 : RkiStrbuf_reserve(sb, size))
extern int RkiStrbuf_reserve pro((RkiStrbuf *sb, size_t size));
extern int RkiStrbuf_term pro((RkiStrbuf *sb));
extern void RkiStrbuf_pack pro((RkiStrbuf *sb));
extern int RkiStrbuf_add pro((RkiStrbuf *sb, const char *src));
extern int RkiStrbuf_addmem pro((RkiStrbuf *sb, const void *src, size_t size));
#define RKI_STRBUF_ADDCH(sb, ch) \
  (RKI_STRBUF_RESERVE(sb, 1) || (*(sb)->sb_curr++ = (char)(ch), 0))
extern int RkiStrbuf_addch pro((RkiStrbuf *sb, int ch));

#ifdef __cplusplus
}
#endif

#endif	/* RKINDEP_STROPS_H */
/* vim: set sw=2: */
