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

/* $Id: conf.h,v 1.1 2003/08/05 12:03:02 aida_s Exp $ */

#ifndef CONF_H
#define CONF_H

typedef struct tagRkcConfMgr RkcConfMgr;
typedef struct tagRkcErrorBuf RkcErrorBuf;

#define CONF_TYPE(i) ((i)&0xff00)
#define CONF_NONE 0
#define CONF_SPECIAL 0x100
#define CONF_STRING 0x200
#define CONF_NUMBER 0x300
#define CONF_YESNO 0x400
typedef enum {
  /* top config */
  CONF_CANNAHOST = CONF_STRING + 1,

  /* host config */
  CONF_SERVER_TIMEOUT = CONF_NUMBER + 1,

  CONF_DUMMYCODE = 0x7fff
} ConfItem;

extern RkcErrorBuf rkc_errors;
extern RkcConfMgr rkc_config;

extern void rkc_configure pro((void));
extern void rkc_config_fin pro((void));
const char *const *RkcErrorBuf_get pro((RkcErrorBuf *cx));
const char *RkcConfMgr_get_string pro((const RkcConfMgr *cx,
      ConfItem item, const char *hostname));
unsigned int RkcConfMgr_get_number pro((const RkcConfMgr *cx,
      ConfItem item, const char *hostname));
int RkcConfMgr_get_yesno pro((const RkcConfMgr *cx,
      ConfItem item, const char *hostname));

#endif /* CONF_H */
/* vim: set sw=2: */
