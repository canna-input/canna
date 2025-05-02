/* Copyright 1992 NEC Corporation, Tokyo, Japan.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of NEC
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  NEC Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * NEC CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN 
 * NO EVENT SHALL NEC CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
 * OTHER TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE. 
 */

#if !defined(lint) && !defined(__CODECENTER__)
static char rcs_id[] = "$Id: wutil.c,v 1.7 2003/09/17 15:13:27 aida_s Exp $";
#endif

#include "sglobal.h"
#include "rkcw.h"

/*********************************************************************
 *                      wchar_t replace begin                        *
 *********************************************************************/
#ifdef wchar_t
# error "wchar_t is already defined"
#endif
#define wchar_t cannawc

int
ushort2eucsize(src, srclen)
Ushort *src;
int srclen;
{
  register int i, j;
  register Ushort wc;

  for (i = 0, j = 0 ; i < srclen ; i++) {
    wc = src[i];
    switch (wc & 0x8080) {
    case 0:
      /* ASCII */
      j++;
      break;
    case 0x0080:
      /* 半角カナ */
      j += 2;
      break;
    case 0x8000:
      /* 外字 */
      j += 3;
      break;
    case 0x8080:
      /* 漢字 */
      j += 2;
      break;
    }
  }
  return j;
}

int
ushort2euc(src, srclen, dest, destlen)
Ushort *src;
char *dest;
int srclen, destlen;
{
  register int i, j;
  register Ushort wc;

  for (i = 0, j = 0 ; i < srclen && j + 2 < destlen ; i++) {
    wc = src[i];
    switch (wc & 0x8080) {
    case 0:
      /* ASCII */
      dest[j++] = (unsigned char)((unsigned)wc & 0x7f);
      break;
    case 0x0080:
      /* 半角カナ */
      dest[j++] = (char)0x8e; /* SS2 */
      dest[j++] = (unsigned char)(((unsigned)wc & 0x7f) | 0x80);
      break;
    case 0x8000:
      /* 外字 */
      dest[j++] = (char)0x8f; /* SS3 */
      dest[j++] = (unsigned char)((((unsigned)wc & 0x7f00) >> 8) | 0x80);
      dest[j++] = (unsigned char)(((unsigned)wc & 0x7f) | 0x80);
      break;
    case 0x8080:
      /* 漢字 */
      dest[j++] = (unsigned char)((((unsigned)wc & 0x7f00) >> 8) | 0x80);
      dest[j++] = (unsigned char)(((unsigned)wc & 0x7f) | 0x80);
      break;
    }
  }
  dest[j] = (unsigned char)0;
  return j;
}

int
eucchars(src, srclen)
unsigned char *src;
int srclen;
{
  register int i, j;
  register unsigned char ec;

  for (i = 0, j = 0 ; i < srclen ; j++) {
    ec = src[i++];
    if (ec & 0x80) {
      i++;
      if (ec == 0x8f) i++; /* SS3 */
    }
  }
  return j;
}

int
euc2ushort(src, srclen, dest, destlen)
char *src;
Ushort *dest;
int srclen, destlen;
{
  register int i, j;
  register unsigned ec;

  for (i = 0, j = 0 ; i < srclen && j + 1 < destlen ; i++) {
    ec = (unsigned)(unsigned char)src[i];
    if (ec & 0x80) {
      switch (ec) {
      case 0x8e: /* SS2 */
	dest[j++] = (Ushort)(0x80 | ((unsigned)src[++i] & 0x7f));
	break;
      case 0x8f: /* SS3 */
	dest[j++] = (Ushort)(0x8000
			      | (((unsigned)src[i + 1] & 0x7f) << 8)
			      | ((unsigned)src[i + 2] & 0x7f));
	i += 2;
	break;
      default:
	dest[j++] = (Ushort)(0x8080 | (((unsigned)src[i] & 0x7f) << 8)
			      | ((unsigned)src[i + 1] & 0x7f));
	i++;
	break;
      }
    }
    else {
      dest[j++] = (Ushort)ec;
    }
  }
  dest[j] = (wchar_t)0;
  return j;
}

exp(int)
Wineuc2ushort(src, srclen, dest, destlen)
char *src;
Ushort *dest;
int srclen, destlen;
{
  return euc2ushort(src, srclen, dest, destlen);
}

#ifndef CANNA_WCHAR16
static int
wchar2ushort32(src, srclen, dest, destlen)
register wchar_t *src;
register Ushort *dest;
int srclen, destlen;
{
  register int i;

  for (i = 0 ; i < srclen && i + 1 < destlen ; i++) {
    switch (((unsigned long)*src & 0xf0000000) >> 28) {
    case 0:
      /* ASCII */
      *dest = (Ushort)((unsigned)*src & 0x7f);
      break;
    case 1:
      /* 半角カナ */
      *dest = (Ushort)(0x80 | ((unsigned)*src & 0x7f));
      break;
    case 2:
      /* 外字 */
      *dest = (Ushort)(0x8000
			     | (((unsigned)*src & 0x3f80) << 1)
			     | ((unsigned)*src & 0x7f));
      break;
    case 3:
      /* 漢字 */
      *dest = (Ushort)(0x8080 
			     | (((unsigned)*src & 0x3f80) << 1)
			     | ((unsigned)*src & 0x7f));
      break;
    }
    src++;
    dest++;
  }
  *dest = (Ushort)0;
  return i;
}

static int
ushort2wchar32(src, srclen, dest, destlen)
register Ushort *src;
register wchar_t *dest;
int srclen, destlen;
{
  register int i;

  for (i = 0 ; i < srclen && i + 1 < destlen ; i++) {
    switch (*src & 0x8080) {
    case 0:
      /* ASCII */
      *dest = (wchar_t)(*src & 0x7f);
      break;
    case 0x0080:
      /* 半角カナ */
     * dest = (wchar_t)((0x1 << 28) | (*src & 0x7f));
      break;
    case 0x8000:
      /* 外字 */
      *dest = (wchar_t)((0x2 << 28)
			| (((unsigned long)*src & 0x7f00) >> 1)
			| ((unsigned long)*src & 0x7f));
      break;
    case 0x8080:
      /* 漢字 */
      *dest = (wchar_t)((0x3 << 28)
			| (((unsigned long)*src & 0x7f00) >> 1)
			| ((unsigned long)*src & 0x7f));
      break;
    }
    src++;
    dest++;
  }
  *dest = (wchar_t)0;
  return i;
}

#else /* CANNA_WCHAR16 */

static int
wchar2ushort16(src, srclen, dest, destlen)
wchar_t *src;
Ushort *dest;
int srclen, destlen;
{
  register int i;

  for (i = 0 ; (i < srclen) && ((i + 1) < destlen) ; i++)
      *dest++ = (Ushort)*src++;

  *dest = (Ushort)0;
  return i;
}

static int
ushort2wchar16(src, srclen, dest, destlen)
Ushort *src;
wchar_t *dest;
int srclen, destlen;
{
  register int i;

  for (i = 0 ; (i < srclen) && ((i + 1) < destlen) ; i++)
      *dest++ = (wchar_t)*src++;

  *dest = (wchar_t)0;
  return i;
}
#endif /* CANNA_WCHAR16 */

/*
 * ワイドキャラクタオペレーション
 *
 */

int
wchar2ushort(src, slen, dst, dlen)
wchar_t *src;
Ushort *dst;
int slen, dlen;
{
#ifdef CANNA_WCHAR16
    return( wchar2ushort16( src, slen, dst, dlen ) );
#else
    return( wchar2ushort32( src, slen, dst, dlen ) );
#endif
}

int
ushort2wchar(src, slen, dst, dlen)
Ushort *src;
wchar_t *dst;
int slen, dlen;
{
#ifdef CANNA_WCHAR16
    return( ushort2wchar16( src, slen, dst, dlen ) );
#else
    return( ushort2wchar32( src, slen, dst, dlen ) );
#endif
}

exp(int)
Winushort2wchar(src, slen, dst, dlen)
Ushort *src;
wchar_t *dst;
int slen, dlen;
{
  return ushort2wchar(src, slen, dst, dlen);
}

int
wcharstrlen(ws)
wchar_t *ws;
{
  register wchar_t *p = ws;
  while (*p)
    p++;
  return p - ws;
}

int
ushortstrlen(ws)
Ushort *ws;
{
  register Ushort *p = ws;
  while (*p)
    p++;
  return p - ws;
}

int
ushortstrcpy(wd, ws)
Ushort *wd, *ws;
{
  register int res = 0;
  while ((*wd++ = *ws++) != (Ushort)0) {
    res++;
  }
  return res;
}

int
ushortstrncpy(wd, ws, n)
Ushort *wd, *ws;
int n;
{
  register int res = 0;
  while (n > res && (*wd = *ws) != (Ushort)0) {
    wd++; ws++; res++;
  }
  *wd = 0;
  return res;
}

#ifndef wchar_t
# error "wchar_t is already undefined"
#endif
#undef wchar_t
/*********************************************************************
 *                       wchar_t replace end                         *
 *********************************************************************/
