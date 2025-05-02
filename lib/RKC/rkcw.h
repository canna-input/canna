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

/* sccs_id[]="@(#) NEC UNIX( PC-UX/EWS-UX ) rkw.h 2.3 91/11/11 12:01:34"; */
/* $Id: rkcw.h,v 1.4 2002/10/20 14:29:59 aida_s Exp $ */

#if (defined(_WINDOWS) || defined(WIN32)) && !defined(WIN)
#define WIN
#endif

#include "cannaconf.h"

#ifdef WIN 
#include <windows.h>
#include <windowsx.h>
#include <winsock.h>
#include <io.h>
#include <process.h>
#define WCHAR_T
#define ENGINE_SWITCH
#ifndef USE_MALLOC_FOR_BIG_ARRAY
#define USE_MALLOC_FOR_BIG_ARRAY
#endif
#ifndef OMIT_EUC_FUNCS
#define OMIT_EUC_FUNCS
#endif
#endif

#if defined(WIN) && !defined(WIN32)
#define exp(x)	x __export CALLBACK
#elif defined(WIN32)
#define exp(x)	__declspec(dllexport) x
#else
#define exp(x)	x
#endif

#if !CANNA_LIGHT && !defined(WIN)
#define USE_EUC_PROTOCOL
#endif

#define EXTENSION
#define DEBUG
#if !defined(WIN)
#define UNIXCONN
#endif

#include "widedef.h"
#include "ccompat.h"

#ifndef NULL
#define NULL 0
#endif

#include "protodefs.h"

#ifdef HAVE_WCHAR_OPERATION
#ifndef JAPANESE_LOCALE
#define JAPANESE_LOCALE "japan"
#endif
#endif

/* function prototypes .. */

extern rkcWCinit pro((void));
extern rkcw_get_server_info pro((int *, int *));
extern ushortstrncpy pro((Ushort *, Ushort *, int));
