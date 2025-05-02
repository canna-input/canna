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

/*
 * @(#) 102.1 $Id: widedef.h,v 1.7.2.2 2003/12/27 17:15:20 aida_s Exp $
 */

#ifndef _WIDEDEF_H_
#define _WIDEDEF_H_

#ifdef __FreeBSD__
# include <osreldate.h>
#endif

#if (defined(__FreeBSD__) && __FreeBSD_version < 500000) \
    || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
# include <machine/ansi.h>
#endif

#if (defined(__FreeBSD__) && __FreeBSD_version < 500000) \
    || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
# ifdef _BSD_WCHAR_T_
#  undef _BSD_WCHAR_T_
#  ifdef WCHAR16
#   define _BSD_WCHAR_T_ unsigned short
#  else
#   define _BSD_WCHAR_T_ unsigned long
#  endif
#  if defined(__APPLE__) && defined(__WCHAR_TYPE__)
#   undef __WCHAR_TYPE__
#   define __WCHAR_TYPE__ _BSD_WCHAR_T_
#  endif
#  include <stddef.h>
#  define _WCHAR_T
# endif
#elif defined(__FreeBSD__) && __FreeBSD_version >= 500000
# ifdef WCHAR16
typedef unsigned short wchar_t;
#  define _WCHAR_T_DECLARED
# endif
# include <stddef.h>
# define _WCHAR_T
#else
#if !defined(WCHAR_T) && !defined(_WCHAR_T) && !defined(_WCHAR_T_) \
 && !defined(__WCHAR_T) && !defined(_GCC_WCHAR_T) && !defined(_WCHAR_T_DEFINED)
# ifdef WCHAR16
typedef unsigned short wchar_t;
# else
/* replace this with #include or typedef appropriate for your system */
typedef unsigned long wchar_t;
# endif
# define WCHAR_T
# define _WCHAR_T
# define _WCHAR_T_
# define __WCHAR_T
# define _GCC_WCHAR_T
#define _WCHAR_T_DEFINED
#endif
#endif

#endif /* _WIDEDEF_H_ */
