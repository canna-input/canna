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


#ifndef	COMM_H
#define COMM_H

/*
 * 特に書いていないものは,成功時に0または有効なポインタ,メモリ不足
 * などによる失敗時に-1またはNULLを返す。
 */
typedef int sock_type;
#define INVALID_SOCK -1
typedef int (*GetConnectionInfoProc)(void *obj,
      sock_type connfd, Address *addr, char **hostname);

typedef struct {
  /* public */
  ClientBuf *it_val;
  /* private */
  void *entry;
} EventMgrIterator;

extern EventMgr *global_event_mgr;

int ClientBuf_store_reply(ClientBuf *obj,
      const BYTE *data, size_t len);
int ClientBuf_get_connection_info(ClientBuf *obj,
      Address *addr, char **hostname);
sock_type ClientBuf_getfd(ClientBuf *obj);
ClientPtr ClientBuf_getclient(ClientBuf *obj);

EventMgr *EventMgr_new(void);
void EventMgr_delete(EventMgr *obj);
int EventMgr_add_listener_sock(EventMgr *obj,
      sock_type listenerfd, GetConnectionInfoProc info_proc, void *info_obj);
void EventMgr_quit_later(EventMgr *obj, int status);
void EventMgr_finalize_notify(EventMgr *obj, const ClientBuf *clibuf);
int EventMgr_run(EventMgr *obj);
void EventMgr_clibuf_first(EventMgr *obj, EventMgrIterator *it);
void EventMgr_clibuf_end(EventMgr *obj, EventMgrIterator *it);
void EventMgrIterator_next(EventMgrIterator *obj);

SockHolder *SockHolder_new(void);
void SockHolder_delete(SockHolder *obj);
int SockHolder_tie(SockHolder *obj, EventMgr *event_mgr);

#endif	/* COMM_H */
/* vim: set sw=2: */
