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

/* $Id: sysdep.h,v 1.4 2003/10/02 07:40:29 aida_s Exp $ */

#ifndef CANNA_SYSDEP_H
#define CANNA_SYSDEP_H

#include <canna/pubconf.h>
#if defined(CANNA_HAVE_INTTYPES_H)
# include <inttypes.h>
#elif defined(CANNA_HAVE_STDINT_H)
# include <stdint.h>
#else
# include <sys/types.h>
#endif

#ifndef _WCHAR_T
# if defined(WCHAR_T) || defined(_WCHAR_T_) || defined(__WCHAR_T) \
  || defined(_GCC_WCHAR_T) || defined(_WCHAR_T_DEFINED)
#  define _WCHAR_T
# endif
#endif

#ifdef CANNA_HAVE_INT32_T
typedef int8_t canna_int8_t;
typedef int16_t canna_int16_t;
typedef int32_t canna_int32_t;
typedef intptr_t canna_intptr_t;
typedef uint8_t canna_uint8_t;
typedef uint16_t canna_uint16_t;
typedef uint32_t canna_uint32_t;
typedef uintptr_t canna_uintptr_t;
#else /* CANNA_HAVE_INT32_T */
/* ILP32 and LP64 */
typedef signed char canna_int8_t;
typedef short canna_int16_t;
typedef int canna_int32_t;
typedef unsigned long canna_intptr_t;
typedef unsigned char canna_uint8_t;
typedef unsigned short canna_uint16_t;
typedef unsigned int canna_uint32_t;
typedef unsigned long canna_uintptr_t;
#endif /* !CANNA_HAVE_INT32_T */

#endif /* CANNA_SYSDEP_H */
/* vim: set sw=2: */
