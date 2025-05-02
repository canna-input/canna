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

/* sccs_id[]="%Z% NEC UNIX( PC-UX/EWS-UX ) %M% %R%.%L% %E% %U%"; */
/* $Id: IR.h,v 1.11 2003/09/23 07:11:31 aida_s Exp $ */

/* 自動判別支援コメント: これはEUC-JPだぞ。幅という字があれば大丈夫。 */

#include "cannaconf.h"
#include "ccompat.h"
#include <sys/types.h>


#ifdef HAVE_TIME_H
# include <time.h>
#endif
#ifdef HAVE_TIME_T
typedef time_t ir_time_t;
#else
typedef long ir_time_t;
#endif

#define EXTENSION

#include    "protodefs.h"
#define CANNAWC_DEFINED
typedef Ushort cannawc;
#include    "canna/RK.h"
#include    "IRproto.h"
#include    "IRwproto.h"
#include    "net.h"

#define LENTODATA(len, data) ((void)(*(canna_uint32_t *)(data) = htonl(len)))
#define DATATOLEN(data, len) ((void)((len) = ntohl(*(canna_uint32_t *)(data))))
