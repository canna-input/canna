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

#if !defined(lint) && !defined(__CODECENTER__)
static char rcs_id[] = "$Id: main.c,v 1.10 2003/09/24 14:50:40 aida_s Exp $";
#endif

/* LINTLIBRARY */

#include "server.h"
#include <signal.h>

#ifdef DEBUG
const char *CallFuncName;
#endif
int (*CallFunc) pro((ClientPtr *clientp));


main(argc, argv)
int argc ;			
char *argv[] ;
{
  int parentid;
  SockHolder *sock_holder = NULL;
  int status;

  EarlyInit(argc, argv);
  if (!(global_user_table = UserTable_new())
      || !(global_event_mgr = EventMgr_new()))
    goto genfail;

  if (!(sock_holder = SockHolder_new()))
    goto fail;
  if (SockHolder_tie(sock_holder, global_event_mgr))
    goto genfail;

  /* サーバを子プロセス(デーモン)として起動する */
  parentid = BecomeDaemon();
  
  /* エラー出力の切り替え、TTYの切り離し */
  DetachTTY();

  /* デーモンになる場合はここでのstatusは実際には意味を持たない */
  status = EventMgr_run(global_event_mgr);
  goto last;

genfail:
  fprintf(stderr, "Initialization failed; probably due to lack of memor\n");
fail:
  status = 2;
last:
  SockHolder_delete(sock_holder);
  EventMgr_delete(global_event_mgr);
  UserTable_delete(global_user_table);
  CloseServer();
  return status;
}

int
process_request(clientp, client_buf, data, len)
ClientPtr *clientp;
ClientBuf *client_buf;
BYTE *data;
size_t len;
{
  int request;
  int nwant, r;
  ClientPtr client = *clientp;
  const char *username = client ? client->username : NULL;
  const char *hostname = client ? client->hostname : NULL;

#ifdef DEBUG
  CallFuncName = NULL;
#endif
  if (client && client->version_hi > 1)
    nwant = parse_wide_request(&request, data, len, username, hostname);
  else
    nwant = parse_euc_request(&request, data, len, username, hostname);

  if (nwant)
    return nwant; /* 失敗、またはもっとデータが必要 */

  /* 実際のプロトコルに応じた処理（関数を呼ぶ） */

  if (client) /* initialize等の場合は呼ばない */
      (void)ClientStat(client, SETTIME, request, 0);
  /* プロトコルの種類毎に統計を取る */
  if (client && client->version_hi > 1) {
#ifdef EXTENSION
    if( request < W_MAXREQUESTNO )
#endif
      ++client->pcount[request];
  } else if (client) {
#ifdef EXTENSION
    if( request < MAXREQUESTNO )
#endif
      ++client->pcount[request];
  }

#ifdef DEBUG
  if (CallFuncName)
    Dmsg( 3,"Now Call %s\n", CallFuncName );
#endif
  if (!client)
    r = ir_nosession(clientp, client_buf);
  else
    r = (*CallFunc) (clientp);
  ir_debug(Dmsg(8,"%s returned %d\n", CallFuncName, r));

  /* クライアントの累積サーバ使用時間を設定する */
  if (client && client == *clientp) /* initialize,finalize等のときは呼ばない */
    ClientStat(client, GETTIME, request, 0);

  if (r)
    r = -1; /* どういう失敗でもとりあえず-1を返す */
  return r;
}
