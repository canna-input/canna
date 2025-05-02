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

/* $Id: comm.h,v 1.1 2003/09/21 12:56:29 aida_s Exp $ */

#ifndef	COMM_H
#define COMM_H

/*
 * 特に書いていないものは,成功時に0または有効なポインタ,メモリ不足
 * などによる失敗時に-1またはNULLを返す。
 */
typedef int sock_type;
#define INVALID_SOCK -1
typedef int (*GetConnectionInfoProc) pro((void *obj,
      sock_type connfd, Address *addr, char **hostname));

typedef struct {
  /* public */
  ClientBuf *it_val;
  /* private */
  void *entry;
} EventMgrIterator;

extern EventMgr *global_event_mgr;

int ClientBuf_store_reply pro((ClientBuf *obj,
      const BYTE *data, size_t len));
int ClientBuf_get_connection_info pro((ClientBuf *obj,
      Address *addr, char **hostname));
sock_type ClientBuf_getfd pro((ClientBuf *obj));
ClientPtr ClientBuf_getclient pro((ClientBuf *obj));

EventMgr *EventMgr_new pro((void));
void EventMgr_delete pro((EventMgr *obj));
int EventMgr_add_listener_sock pro((EventMgr *obj,
      sock_type listenerfd, GetConnectionInfoProc info_proc, void *info_obj));
void EventMgr_quit_later pro((EventMgr *obj, int status));
void EventMgr_finalize_notify pro((EventMgr *obj, const ClientBuf *clibuf));
int EventMgr_run pro((EventMgr *obj));
void EventMgr_clibuf_first pro((EventMgr *obj, EventMgrIterator *it));
void EventMgr_clibuf_end pro((EventMgr *obj, EventMgrIterator *it));
void EventMgrIterator_next pro((EventMgrIterator *obj));

SockHolder *SockHolder_new pro((void));
void SockHolder_delete pro((SockHolder *obj));
int SockHolder_tie pro((SockHolder *obj, EventMgr *event_mgr));

#endif	/* COMM_H */
/* vim: set sw=2: */
