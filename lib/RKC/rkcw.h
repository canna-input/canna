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
/* $Id: rkcw.h,v 1.9 2003/09/17 08:50:53 aida_s Exp $ */

#include "cannaconf.h"

#define exp(x) x

#if !defined(CANNA_LIGHT)
#define USE_EUC_PROTOCOL
#endif

#define EXTENSION
#define DEBUG
#define UNIXCONN

#include "ccompat.h"
#ifndef CANNAWC_DEFINED
# define CANNAWC_DEFINED
# ifdef CANNA_WCHAR16
typedef canna_uint16_t cannawc;
# else
typedef canna_uint32_t cannawc;
# endif
#endif


#ifndef NULL
#define NULL 0
#endif

#include "protodefs.h"

/* function prototypes .. */

extern rkcWCinit pro((void));
extern rkcw_get_server_info pro((int *, int *));
extern ushortstrncpy pro((Ushort *, Ushort *, int));
