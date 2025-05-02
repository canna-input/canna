/* Copyright 1994 NEC Corporation, Tokyo, Japan.
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
static char rcsid[]="@(#)$Id: util.c,v 1.8 2003/09/17 08:50:52 aida_s Exp $ $Author: aida_s $ $Revision: 1.8 $ $Data$";
#endif

#include "RKintern.h"
#include <stdio.h>
#ifdef __STDC__
#include <stdarg.h>
#endif

#define	isEndTag(s)	(s[0] == 0 && s[1] == 0 && s[2] == 0 && s[3] == 0)

#define HD_TAG_MAG	"MAG#"
#define HD_TAG_CURV	"CURV"
#define HD_TAG_CMPV	"CMPV"
#define HD_TAG_SIZ	"#SIZ"
#define HD_TAG_HSZ	"#HSZ"
#define HD_TAG_VER	"VER#"
#define HD_TAG_TIME	"TIME"
#define HD_TAG_REC	"#REC"
#define HD_TAG_CAN	"#CAN"
#define HD_TAG_L2P	"L2P#"
#define HD_TAG_L2C	"L2C#"
#define HD_TAG_PAG	"#PAG"
#define HD_TAG_LND	"#LND"
#define HD_TAG_SND	"#SND"
#define HD_TAG_DROF	"DROF"
#define HD_TAG_PGOF	"PGOF"
#define HD_TAG_DMNM	"DMNM"
#define HD_TAG_CODM	"CODM"
#define HD_TAG_LANG	"LANG"
#define HD_TAG_WWID	"WWID"
#define HD_TAG_WTYP	"WTYP"
#define HD_TAG_COPY	"(C) "
#define HD_TAG_NOTE	"NOTE"
#define HD_TAG_TYPE	"TYPE"
#define HD_TAG_CRC	"#CRC"
#define HD_TAG_GRAM	"GRAM"
#define HD_TAG_GRSZ	"GRSZ"

static char	*Hdrtag[] = {
  HD_TAG_MAG,
  HD_TAG_SIZ,
  HD_TAG_HSZ,
  HD_TAG_CURV,
  HD_TAG_CMPV,
  HD_TAG_VER,
  HD_TAG_TIME,
  HD_TAG_REC,
  HD_TAG_CAN,
  HD_TAG_L2P,
  HD_TAG_L2C,
  HD_TAG_PAG,
  HD_TAG_LND,
  HD_TAG_SND,
  HD_TAG_DROF,
  HD_TAG_PGOF,
  HD_TAG_DMNM,
  HD_TAG_CODM,
  HD_TAG_LANG,
  HD_TAG_WWID,
  HD_TAG_WTYP,
  HD_TAG_COPY,
  HD_TAG_NOTE,
  HD_TAG_TYPE,
  HD_TAG_CRC,
  HD_TAG_GRAM,
  HD_TAG_GRSZ,
  0,
};

int
uslen(us)
     Wchar	*us;
{
  Wchar *ous = us;
  
  if (!us)
    return 0;
  while (*us & RK_WMASK)
    us++;
  return (us - ous);
}

void
usncopy(dst, src, len)
     Wchar	*dst, *src;
     int	len;
{
  while (len-- > 0 && (*dst++ = *src++)) /* EMPTY */;
}

unsigned char *
ustoeuc(src, srclen, dest, destlen)
     Wchar		*src;
     unsigned char	*dest;
     int		srclen, destlen;
{
    if (!src || !dest || !srclen || !destlen)
	return dest;
    while (*src && --srclen >= 0 && --destlen >= 0) {
	if (us_iscodeG0(*src)) {
	    *dest++ = (unsigned char)*src++;
	} else if (us_iscodeG2(*src)) {
	    *dest++ = RK_SS2;
	    *dest++ = (unsigned char)*src++;
	    destlen--;
	} else if (destlen > 2) {
	  if (us_iscodeG3(*src)) {
	    *dest++ = RK_SS3;
	  }
	  *dest++ = (unsigned char)(*src >> 8);
	  *dest++ = (unsigned char)(*src++ | 0x80);
	  destlen--;
	};
    };
    *dest = (unsigned char)0;
    return dest;
}

Wchar *
euctous(src, srclen, dest, destlen)
     unsigned char	*src;
     Wchar		*dest;
     int		srclen, destlen;
{
  Wchar	*a = dest;
    
  if (!src || !dest || !srclen || !destlen)
    return(a);
  while (*src && (srclen-- > 0) && (destlen-- > 0)) {
    if (!(*src & 0x80) ) {
      *dest++ = (Wchar)*src++;
    } else if (srclen-- > 0) {
      if (*src == RK_SS2) {
	src++;
	*dest++ = (Wchar)(0x0080 | (*src++ & 0x7f));
      } else if ((*src == RK_SS3) && (srclen-- > 0)) {
	src++;
	*dest++ = (Wchar)(0x8000 | ((src[0] & 0x7f) << 8) | (src[1] & (0x7f)));
	src += 2;
      } else {
	*dest++ = (Wchar)(0x8080 | ((src[0] & 0x7f) << 8) | (src[1] & 0x7f));
	src += 2;
      }
    } else {
      break;
    }
  }
  if (destlen-- > 0)
    *dest = (Wchar)0;
  return dest;
}

static FILE	*log = (FILE *)0;

void
_Rkpanic(
#ifdef __STDC__
    const char *fmt, ...
#else
    fmt, p, q, r
#endif
    )
#ifndef __STDC__
     const char	*fmt;
/* VARARGS2 */
#endif
{
  FILE *target = log ? log : stderr;
#ifdef __STDC__
  va_list va;

  va_start(va, fmt);
  vfprintf(target, fmt, va);
  va_end(va);
#else
  fprintf(target, fmt, p, q, r);
#endif
  fputc('\n', target);
  fflush(target);
  if (log)
    fclose(log);
  abort();
}

void
RkAssertFail(file, line, expr)
     const char *file;
     int line;
     const char *expr;
{
  _Rkpanic("RK assertion failed: %s:%d %s", file, line, expr);
  /* NOTREACHED */
}

int
_RkCalcUnlog2(x)
     int	x;
{
  return((1 << x) - 1);
}

int 
_RkCalcLog2(n)
     int n;
{
  int	lg2;
  
  n--;
  for (lg2 = 0; n > 0; lg2++)
    n >>= 1;
  return(lg2);
}

Wchar
uniqAlnum(c)
     Wchar c;
{
  return((0xa3a0 < c && c < 0xa3ff) ? (Wchar)(c & 0x7f) : c);
}

void
_RkClearHeader(hd)
     struct HD	*hd;
{
  int	i;
    
  if (hd) {
    for (i = 0; i < HD_MAXTAG; i++) {
      if (hd->flag[i] > 0) {
	(void)free(hd->data[i].ptr);
      }
    }
  }
}

static int
read_tags(hd, srctop, srcend, pass)
     struct HD	*hd;
     const unsigned char *srctop;
     const unsigned char *srcend;
     int	pass;
{
  unsigned long	len, off;
  const unsigned char *src = srctop;
  unsigned int i;

  while (src + HD_TAGSIZ <= srcend) {
    if (isEndTag(src))
      return 0;
    if (src + HD_MIN_TAGSIZ > srcend)
      break;
    for (i = 0; i < HD_MAXTAG; i++) {
      if (!strncmp((const char *)src, Hdrtag[i],  HD_TAGSIZ))
	break;
    }
    if (i == HD_MAXTAG) {
      src += HD_MIN_TAGSIZ; /* simply skip */
      continue;
    }
    src += HD_TAGSIZ;
    len = bst4_to_l(src);
    src += HD_TAGSIZ;
    off = bst4_to_l(src);
    src += HD_TAGSIZ;
    if (hd->flag[i] != 0)
      return -1;
    if (len == 0) {
      hd->flag[i] = -1;
      hd->data[i].var = off;
    } else {
      if (pass == 2) {
	if (srctop + off + len > srcend)
	  return -1;
	hd->flag[i] = len;
	if (!(hd->data[i].ptr = (unsigned char *)malloc((size_t) (len + 1))))
	  return -1;
	(void)memcpy(hd->data[i].ptr, srctop + off, (size_t) len);
	hd->data[i].ptr[len] = 0;
      }
    }
  }
  return (pass == 2);
}

int
_RkReadHeader(fd, hd, off_from_top)
     int	fd;
     struct HD	*hd;
     off_t	off_from_top;
{
  off_t tmpres;
  ssize_t pass1size;
  unsigned char pass1buf[RK_OLD_MAX_HDRSIZ];
  unsigned char	*pass2buf = NULL;
  long curr_ver = 0, compat_ver = 0;
  unsigned int i;
  size_t hdrsiz;

  for (i = 0; i < HD_MAXTAG; i++) {
    hd->data[i].var = 0;
    hd->flag[i] = 0;
  }
  tmpres = lseek(fd, off_from_top, 0);
  if (tmpres < 0)
    return -1;
  pass1size = read(fd, (char *)pass1buf, RK_OLD_MAX_HDRSIZ);
  if (pass1size <= 0)
    return -1;

  /* Pass 1 */
  if (read_tags(hd, pass1buf, pass1buf + pass1size, 1))
    goto read_err;

  if (hd->flag[HD_MAG] != -1
      || hd->data[HD_MAG].var != (long)bst4_to_l("CDIC"))
    goto read_err;
  if (hd->flag[HD_CURV] == -1 && hd->flag[HD_CMPV] == -1) {
    curr_ver = hd->data[HD_CURV].var;
    compat_ver = hd->data[HD_CMPV].var;
  } else if (hd->flag[HD_VER] == -1
      && hd->data[HD_VER].var == (long)bst4_to_l("R3.0")) {
    curr_ver = 0x300000L;
    compat_ver = 0x300000L;
  }
  if (curr_ver < compat_ver || compat_ver < 0x300000L || compat_ver > 0x300702L)
    goto read_err;

  if (hd->flag[HD_HSZ] != -1 || hd->flag[HD_SIZ] != -1)
    goto read_err;
  hdrsiz = (size_t)hd->data[HD_HSZ].var;

  /* Pass 2 */
  pass2buf = malloc(hdrsiz);
  if (!pass2buf)
    goto read_err;
  if (pass1size < hdrsiz) {
    size_t rest = hdrsiz - pass1size;
    ssize_t r = read(fd, (char *)pass2buf + pass1size, rest);
    if (r < rest)
      goto read_err;
    memcpy(pass2buf, pass1buf, pass1size);
  } else {
    memcpy(pass2buf, pass1buf, hdrsiz);
  }

  for (i = 0; i < HD_MAXTAG; i++) {
    hd->data[i].var = 0;
    hd->flag[i] = 0;
  }
  if (read_tags(hd, pass2buf, pass2buf + hdrsiz, 2))
    goto read_err;

  return 0;

 read_err:
  for (i = 0; i < HD_MAXTAG; i++) {
    if (hd->flag[i] > 0)
      (void)free(hd->data[i].ptr);
    hd->flag[i] = 0;
    hd->data[i].var = 0;
  }
  free(pass2buf);
  return -1;
}

unsigned char *
_RkCreateHeader(hd, size)
     struct HD	*hd;
     size_t *size;
{
  unsigned char	*tagdst, *datadst, *ptr;
  unsigned int i;
  unsigned long len, off;
  size_t tagsz = 0, datasz = 0;
  long curr_ver = 0, compat_ver = 0;

  if (hd->flag[HD_CURV] == -1 && hd->flag[HD_CMPV] == -1) {
    curr_ver = hd->data[HD_CURV].var;
    compat_ver = hd->data[HD_CMPV].var;
  } else if (hd->flag[HD_VER] == -1
      && hd->data[HD_VER].var == (long)bst4_to_l("R3.0")) {
    curr_ver = 0x300000L;
    compat_ver = 0x300000L;
  }
  if (curr_ver < compat_ver || compat_ver < 0x300000L || compat_ver > 0x300702L)
    return NULL;

  for (i = 0; i < HD_MAXTAG; i++) {
    if (hd->flag[i])
      tagsz += HD_MIN_TAGSIZ;
    if (hd->flag[i] > 0)
      datasz += hd->flag[i];
  }

  if (!(ptr = malloc(tagsz + HD_TAGSIZ + datasz)))
    return NULL;

  tagdst = ptr;
  datadst = ptr + tagsz + HD_TAGSIZ;
  for (i = 0; i < HD_MAXTAG; i++) {
    if (!hd->flag[i])
      continue;

    (void)memcpy(tagdst, Hdrtag[i], HD_TAGSIZ);
    tagdst += HD_TAGSIZ;
    if (hd->flag[i] == -1) {
      len = 0;
      off = hd->data[i].var;
    } else {
      len = hd->flag[i];
      off = datadst - ptr;
      (void)memcpy(datadst, hd->data[i].ptr, (size_t) len);
      datadst += len;
    }
    l_to_bst4(len, tagdst); tagdst += HD_TAGSIZ;
    l_to_bst4(off, tagdst); tagdst += HD_TAGSIZ;
  }
  RK_ASSERT(tagdst == ptr + tagsz);
  RK_ASSERT(datadst == ptr + tagsz + HD_TAGSIZ + datasz);
  *tagdst++ = 0; *tagdst++ = 0; *tagdst++ = 0; *tagdst++ = 0;
  *size = tagsz + HD_TAGSIZ + datasz;
  return ptr;
}

unsigned long
_RkGetTick(mode)
     int	mode;
{
  static unsigned long time = 10000;
  return(mode ? time++ : time);
}

int
set_hdr_var(hd, n, var)
     struct HD		*hd;
     int		n;
     unsigned long	var;
{
    if (!hd)
	return -1;
    hd->data[n].var = var;
    hd->flag[n] = -1;
    return 0;
}

_RkGetLink(dic, pgno, off, lvo, csn)
     struct ND	*dic;
     long	pgno;
     unsigned long	off;
     unsigned long	*lvo;
     unsigned long	*csn;
{
  struct NP	*pg = dic->pgs + pgno;
  unsigned char	*p;
  unsigned	i;

  for (i = 0, p = pg->buf + 14 + 4 * pg->ndsz; i < pg->lnksz; i++, p += 5) {
    if (thisPWO(p) == off) {
      *lvo = pg->lvo + thisLVO(p);
      *csn = pg->csn + thisCSN(p);
      return(0);
    }
  }
  return(-1);
}

unsigned long
_RkGetOffset(dic, pos)
     struct ND		*dic;
     unsigned char	*pos;
{
  struct NP	*pg;
  unsigned char	*p;
  unsigned	i;
  unsigned long	lvo;
#if 0 /* csn is not used */
  unsigned long	csn;
#endif

  for (i = 0; i < dic->ttlpg; i++) {
    if (dic->pgs[i].buf) {
      if (dic->pgs[i].buf < pos && pos < dic->pgs[i].buf + dic->pgsz)
	break;
    }
  }
  if (i == dic->ttlpg) {
    return(0);
  }
  pg = dic->pgs + i;
  for (i = 0, p = pg->buf + 14 + 4 * pg->ndsz; i < pg->lnksz; i++, p += 5) {
    if ((unsigned long) (pos - pg->buf) == thisPWO(p)) {
      lvo = pg->lvo + thisLVO(p);
#if 0 /* csn is not used */
      csn = pg->csn + thisCSN(p);
#endif
      return(lvo);
    }
  }
  _Rkpanic("Cannot get Offset", 0, 0, 0);
}

int
HowManyChars(yomi, len)
     Wchar	*yomi;
     int	len;
{
  int chlen, bytelen;

  for (chlen = 0, bytelen = 0; bytelen < len; chlen++) {
    Wchar ch = yomi[chlen];
    
    if (us_iscodeG0(ch))
      bytelen++;
    else if (us_iscodeG3(ch))
      bytelen += 3;
    else
      bytelen += 2;
  }
  return(chlen);
}

int
HowManyBytes(yomi, len)
     Wchar	*yomi;
     int	len;
{
  int chlen, bytelen;

  for (chlen = 0, bytelen = 0; chlen < len; chlen++) {
    Wchar ch = yomi[chlen];

    if (us_iscodeG0(ch))
      bytelen++;
    else if (us_iscodeG3(ch))
      bytelen += 3;
    else {
      bytelen += 2;
    }
  }
  return(bytelen);
}

#ifdef TEST

printWord(w)
struct nword *w;
{
  printf("[0x%x] Y=%d, K=%d, class=0x%x, flg=0x%x, lit=%d, prio=%d, kanji=",
	 w, w->nw_ylen, w->nw_klen, w->nw_class, w->nw_flags,
	 w->nw_lit, w->nw_prio);
  if (w->nw_kanji) {
    int i, klen = w->nw_left ? w->nw_klen - w->nw_left->nw_klen : w->nw_klen;
    char *p = w->nw_kanji + 2;

    for (i = 0 ; i < klen ; i++) {
      printf("%c%c", p[0], p[1]);
      p += 2;
    }
  }
  printf("\n");
}

showWord(w)
struct nword *w;
{
  struct nword *p, *q;

  printf("next:\n");
  for (p = w ; p ; p = p->nw_next) {
    printWord(p);
    for (q = p->nw_left ; q ; q = q->nw_left) {
      printWord(q);
    }
    printf("\n");
  }
}

#endif /* TEST */
/* vim: set sw=2: */
