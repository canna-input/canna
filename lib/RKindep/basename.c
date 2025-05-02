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
#include "RKindep/file.h"

RCSID("$Id: basename.c,v 1.2 2003/03/24 04:04:25 aida_s Exp $");

/*
 * basename() has at least 3 versions.
 *
 * 1. traditional
 * (glibc default, libiberty, Tru64 without _XOPEN_SOURCE_EXTENDED)
 * They simply returns the pointer to just after the last slash. If input
 * is terminated by '/' output is empty string. Typically input == NULL
 * results segfault.
 *
 * 2.  SUSv2
 * (glibc+libgen.h, Tru64 + _XOPEN_SOURCE_EXTENDED, Sun and others)
 * If input is terminated by '/' it is replaced to '\0'.  If nput == NULL
 * then output is ".". It is important that input string may be modified.
 * So it is declared like this:
 * char *basename(char *);
 *
 * 3. recent BSD (including MacOS X)
 * Basically same to SUSv2 but input string is not modified. Alternatively
 * it is stored to static memory. If input path is longer than MAXPATHLEN
 * output is NULL and errno is ENAMETOOLONG. Additionally, FreeBSD uses
 * malloc() to store output string, ENOMEM may happen. I'm not familiar
 * to UNIX standards but the claim in BSD manpages that thier basename()
 * are XPG4.2 conformant might be wrong. XPG4.2 allows and SUSV2 disallows?
 *
 * I implement 1. because it is most simple, safe and does everything what
 * is needed in canna.
 */

char *
RkiBasename(path)
const char *path;
{
  char *p = strrchr(path, '/');
  return p ? p + 1 : (char *)path;
}

/* vim: set sw=2: */
