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

/* $Id: ccompat.h,v 1.10 2003/09/25 07:38:11 aida_s Exp $ */

#ifndef CCOMPAT_H
#define CCOMPAT_H
#include "cannaconf.h"

#if defined(__STDC__) || defined(__cplusplus)
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
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
# include <memory.h>
#endif
#include <errno.h>
#ifdef luna68k
extern int  errno;
#endif

#include "canna/sysdep.h"

#if defined(HAVE_STRCHR) && !defined(HAVE_INDEX) && !defined(index)
# define index(s, c) strchr(s, c)
# define rindex(s, c) strrchr(s, c)
#elif !defined(HAVE_STRCHR) && defined(HAVE_INDEX) && !defined(strchr)
# define strchr(s, c) index(s, c)
# define strrchr(s, c) rindex(s, c)
#endif

#if defined(HAVE_MEMSET) && !defined(HAVE_BZERO) && !defined(bzero)
# define bzero(buf, size) ((void)memset((char *)(buf), 0x00, (size)))
#endif
#if defined(HAVE_MEMCPY) && !defined(HAVE_BCOPY) && !defined(bcopy)
# define bcopy(src, dst, size) ((void)memmove((char *)(dst), (char *)(src), (size)))
#elif !defined(HAVE_MEMCPY) && defined(HAVE_BCOPY) && !defined(memcpy)
/* Don't use return value; bcopy() returns void */
# define memcpy(dst, src, size) bcopy((char *)(src), (char *)(dst), (size))
# define memmove(dst, src, size) bcopy((char *)(src), (char *)(dst), (size))
#endif

#include "RKindep/cfuncs.h"

#ifdef __GNUC__
# define UNUSED_SYMBOL __attribute__((__unused__))
# ifdef __ELF__
#  ifdef __STDC__
#   define	WARN_REFERENCES(sym,msg)	\
	__asm__(".section .gnu.warning." #sym);	\
	__asm__(".asciz \"" msg "\"");	\
	__asm__(".previous")
#  else
#   define	WARN_REFERENCES(sym,msg)	\
	__asm__(".section .gnu.warning.sym"); \
	__asm__(".asciz \"msg\"");	\
	__asm__(".previous")
#  endif	/* __STDC__ */
# endif	/* __ELF__ */
#endif	/* __GNUC__ */

#ifndef WARN_REFERENCES
# define WARN_REFERENCES(sym, msg) struct cannahack
#endif
#ifndef UNUSED_SYMBOL
# define UNUSED_SYMBOL
#endif

#if !defined(lint) && !defined(__CODECENTER__)
# define RCSID(id) static const char rcsid[] UNUSED_SYMBOL = id
#else
# define RCSID(id) struct cannahack
#endif

#endif /* CCOMPAT_H */
