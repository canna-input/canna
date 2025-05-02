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
#include "RKindep/ecfuncs.h"

RCSID("$Id: cfuncs.c,v 1.2.2.1 2003/12/27 17:15:24 aida_s Exp $");

#undef malloc
#undef calloc

#if !HAVE_MALLOC
void *
RkiMalloc(size)
size_t size;
{
  return malloc(size ? size : 1);
}

void *
RkiCalloc(num, size)
size_t num;
size_t size;
{
  return calloc(num ? num : 1, size ? size : 1);
}
#endif /* !HAVE_MALLOC */

#ifndef HAVE_MEMSET
void *
RkiMemset(buf, ch, size)
void *buf;
int ch;
size_t size;
{
  char *p = (char *)buf;
  char *endp = p + size;
  while (p < endp)
    *p++ = (char)ch;
  return buf;
}
#endif /* !HAVE_MSEMSET */

#ifndef HAVE_STRDUP
char *
RkiStrdup(str)
const char *str;
{
  size_t len = strlen(str) + 1;
  char *p;
  p = malloc(len);
  if (!p)
    return NULL;
  strcpy(p, str);
  return p;
}
#endif /* !HAVE_MSEMSET */

#ifndef HAVE_STRLCPY
size_t
RkiAltStrlcpy(dst, src, size)
char *dst;
const char *src;
size_t size;
{
  const char *sp = src;
  char *dp = dst;
  char *dstend;
  if (!size) /* should not happen */
    goto last;
  dstend = dst + size - 1;
  for (; dp < dstend && *sp; ++dp, ++sp)
    *dp = *sp;
  *dp = '\0';
last:
  for (; *sp; ++sp)
    ;
  return sp - src;
}

size_t
RkiAltStrlcat(dst, src, size)
char *dst;
const char *src;
size_t size;
{
  const char *sp = src;
  char *dp = dst;
  char *dstend;

  dstend = dst + size; /* first dstend */
  for (; dp < dstend; ++dp)
    if (!*dp)
      goto next;
  /* dp == dstend */
  goto last; /* should not happen */
next:
  --dstend; /* second dstend */
  for (; dp < dstend && *src; ++dp, ++src)
    *dp = *src;
  *dp = '\0';
last:
  for (; *sp; ++sp)
    ;
  return (dp - dst) + (sp - src);
}
#endif /* !HAVE_STRLCPY */

/* vim: set sw=2: */
