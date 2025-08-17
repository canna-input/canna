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

#include "cannaconf.h"
#include "ccompat.h"
#include "RKindep/cksum.h"

RCSID("$Id: cksum.c,v 1.5 2004/05/02 23:08:29 aida_s Exp $");

static void RkiCksumCRCAdd pro((RkiCksumCalc *cx,
      const void *data, size_t len));

int
RkiCksumAdd(cx, data, len)
RkiCksumCalc *cx;
const void *data;
size_t len;
{
  RkiCksumCRCAdd(cx, data, len);
  return 0;
}

/*
 * POSIX 1003.2 cksum (==ISO/IEC 8802-3:1989 CRC)
 */

static const canna_uint32_t crctab[] = {
#include "crctab.c"
};

int
RkiCksumCRCInit(cx)
RkiCksumCalc *cx;
{
  cx->curr = 0;
  cx->len = 0;
  return 0;
}

#define	NEXTVAL(old, ch) ((old) << 8 ^ crctab[(old) >> 24 ^ (ch)])
static void
RkiCksumCRCAdd(cx, data, len)
RkiCksumCalc *cx;
const void *data;
size_t len;
{
  const unsigned char *p = (const unsigned char *)data;
  const unsigned char *endp = p + len;
  canna_uint32_t curr;

  curr = cx->curr;
  for (; p < endp; ++p)
    curr = NEXTVAL(curr, *p);
  cx->curr = curr;
  cx->len += len;
}

canna_uint32_t
RkiCksumCRCFinish(cx)
RkiCksumCalc *cx;
{
  canna_uint32_t curr = cx->curr;
  for (; cx->len != 0; cx->len >>= 8) /* LSB first, variable length */
    curr = NEXTVAL(curr, cx->len & 0xff);
  return ~curr;
}

/* vim: set sw=2: */
