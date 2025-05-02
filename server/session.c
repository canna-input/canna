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

/* LINTLIBRARY */
#include "server.h"

RCSID("$Id: session.c,v 1.1 2003/09/21 12:56:29 aida_s Exp $");

typedef struct {
  char *ue_name;
  int ue_count;
} UserEntry;

struct tagUserTable {
  UserEntry *entries;
  int n_entries;
};

UserTable *global_user_table = NULL;

UserTable *
UserTable_new()
{
  UserTable *obj = malloc(sizeof(UserTable));

  if (!obj)
    return NULL;
  obj->entries = NULL;
  obj->n_entries = 0;
  return obj;
}

void
UserTable_delete(obj)
UserTable *obj;
{
  free(obj->entries);
  free(obj);
}

static int
UserTable_register(obj, username)
UserTable *obj;
const char *username;
{
  int usr_no;
  
  ir_debug( Dmsg( 6,"ユーザ名[%s]\n", username ) );
  for (usr_no = 0; usr_no < obj->n_entries; ++usr_no) {
    if (obj->entries[usr_no].ue_name
	&& !strcmp(obj->entries[usr_no].ue_name, username)) {
      ir_debug( Dmsg( 6,"登録済みユーザ名[%s]\n", username ) );
      ir_debug( Dmsg( 6,"ユーザカウント[%d]\n",
	    obj->entries[usr_no].ue_count ) );
      ++obj->entries[usr_no].ue_count;
      return usr_no;
    }
  }
  for (usr_no = 0; usr_no < obj->n_entries; ++usr_no)
    if (!obj->entries[usr_no].ue_name)
      break;
  if (usr_no == obj->n_entries) {
    int newsize = obj->n_entries * 2 + 4;
    UserEntry *tmp;
    tmp = realloc(obj->entries, newsize * sizeof(UserEntry));
    if (!tmp)
      return -1;
    bzero(tmp + usr_no, (newsize - usr_no) * sizeof(UserEntry));
    obj->entries = tmp;
    obj->n_entries = newsize;
  }
  ir_debug( Dmsg( 6,"ユーザ[%s]を新規登録する\n", username ) );
  ir_debug( Dmsg( 6,"ユーザナンバー[%d]\n", usr_no ) );
  if (!(obj->entries[usr_no].ue_name = strdup(username)))
    return -1;
  obj->entries[usr_no].ue_count = 1;
  return usr_no;
}

static void
UserTable_unregister(obj, usr_no)
UserTable *obj;
int usr_no;
{
  UserEntry *ent = obj->entries + usr_no;
  assert(usr_no >= 0 && usr_no < obj->n_entries);
  ir_debug( Dmsg( 6,"ユーザカウント:[%d]\n", ent->ue_count ) );
  assert(ent->ue_count && ent->ue_name);
  if (--ent->ue_count == 0) {
    ir_debug( Dmsg( 6,"ユーザ[%s]を削除する\n", ent->ue_name ) );
    free(ent->ue_name);
    ent->ue_name = NULL;
  }
}

static int
CheckVersion( data, client )
char *data ;
ClientPtr client ;
{
  char *logname ;
  int clienthi, clientlo ;
  char *buf ;

  if( !(buf = (char *)strtok( (char *)data, "." )) )
    return( -1 ) ;
  clienthi = atoi( buf ) ;

  if( !(buf = (char *)strtok((char *)NULL, ":")) )
    return( -1 ) ;
  clientlo = atoi( buf ) ;

  if( !(logname = strtok( (char *)NULL, ":" )) )
    return( -1 ) ;

  ir_debug( Dmsg( 5,"UserName:[%s]\n", logname ); )
    ir_debug( Dmsg( 5,"client:hi[%d],lo[%d]\n", clienthi, clientlo ); )
    ir_debug( Dmsg( 5,"server:hi[%d],lo[%d]\n", canna_server_hi, canna_server_lo ); )
#ifndef USE_EUC_PROTOCOL
    if (clienthi < 2) {
      return RETURN_VERSION_ERROR_STAT;
    }
#endif /* USE_EUC_PROTOCOL */
  if( canna_server_hi < clienthi )
    return( RETURN_VERSION_ERROR_STAT ) ;

  if (!(client->username = strdup(logname)))
    return -1;
  client->version_hi = (short)clienthi ;
  client->version_lo = (short)clientlo ;

  if( clienthi < canna_server_hi )
    return( clientlo );
  else
    return( canna_server_lo );
}

static void
free_client_rec(client)
ClientPtr client;
{
  int *contexts;
  int i;

  contexts = client->context_flag;
  for (i = 0 ; i < client->ncon ; i++) {
    RkwCloseContext(contexts[i]);
  }
  client->ncon = 0;

  ir_debug( Dmsg( 6,"ユーザナンバー:[%d]\n", client->usr_no ));
  if (client->usr_no >= 0)
    UserTable_unregister(global_user_table, client->usr_no);

  if( client->hostname )
    free( (char *)client->hostname ) ;

  if( client->username )
    free( (char *)client->username ) ;

  free( (char *)client );
}

int
open_session(clientp, name, client_buf)
ClientPtr *clientp;
char *name;
ClientBuf *client_buf;
{
  int cxnum = -1, eff_lo, stat = -1;

  register ClientPtr client = (ClientPtr)calloc( 1, sizeof( ClientRec ) ) ;

  ir_debug(Dmsg(3, "セッションを開く\n"));

  if (!client)
    goto fail;
  client->usr_no = -1; /* これを最初にやること */
  client->client_buf = client_buf;
  if (ClientBuf_get_connection_info(client_buf,
	&client->hostaddr, &client->hostname))
    goto fail;
  /* プロトコルバージョンのチェック,ユーザ名取得 */
  if( (eff_lo = CheckVersion( name, client )) < 0 )
    goto fail;
  ir_debug(Dmsg(5, "PROTOCOL.Version[%d:%d]\n", client->version_hi, eff_lo));

  /* ユーザ登録 */
  if ((client->usr_no = UserTable_register(global_user_table,
	  client->username)) < 0)
    goto fail;

  /* アクセス制御 */
#if defined(INET6) && !defined(IR_V6ONLY_BIND)
  if( client->hostaddr.family == AF_INET6
      && IN6_IS_ADDR_V4MAPPED( IR_ADDR_IN6( &client->hostaddr) ) ) {
    PrintMsg("[%s@%s] IPv4 mapped address detected\n",
	client->username, client->hostname);
    goto fail;
  }
  if( !UseInet ) {
    if( client->hostaddr.family == AF_INET ) {
      PrintMsg("[%s@%s] IPv4 is disabled\n",
	  client->username, client->hostname);
      goto fail;
    }
  }
#endif /* INET6 && !IR_V6ONLY_BIND */
  if (CheckAccessControlList(&client->hostaddr, client->username) < 0) {
    PrintMsg("[%s@%s] Access denied\n",
	client->username, client->hostname);
    goto fail;
  }

  /* 変換コンテキストの確保 */
  cxnum = RkwCreateContext() ;
  if (cxnum < 0)
    goto fail;
  if (SetDicHome( client, cxnum ) < 0
      || set_cxt(client, cxnum) == 0)
    goto fail;
  stat = ((eff_lo << 0x10) | cxnum);

  ClientStat(client, CONNECT, 0/*dummy*/, 0) ;
  *clientp = client;
  ir_debug(Dmsg(3, "セッションの開始に成功\n"));
  return( stat );

fail:
  ir_debug(Dmsg(3, "セッションを開けなかった\n"));
  if (cxnum >= 0)
    RkwCloseContext(cxnum);
  if (client)
    free_client_rec(client);
  return( -1 );
}

void
close_session(clientp, notify)
ClientPtr *clientp;
int notify;
{
  ClientPtr client = *clientp;

  if (!client)
    return;
  if (notify)
    EventMgr_finalize_notify(global_event_mgr, client->client_buf);

  ir_debug(Dmsg(3, "セッションを閉じる\n"));
  free_client_rec(client);
  *clientp = NULL;
}

void
ClientStat(client, type, request_Type, return_date)
ClientPtr client ;
int request_Type, type;
char *return_date ;
{
  static ir_time_t set_time ; /* サーバを使用した時間を測る基点 */
  ir_time_t  cur_time ;

  cur_time = time( NULL ) ;

  switch( type ) {
#ifdef DEBUG
    case GETDATE :
      {
	struct tm	 *tt ;
	char *date ;
	ir_time_t cdate ;

	cdate = client->connect_date ;
	tt = localtime( &cdate ) ;
	date = asctime( tt ) ;
	date[24] = '\0' ;
	if (return_date) {
	  strcpy(return_date, date);
	  strcat(return_date, " JST");
	}
	break ;
      }
#endif
    case CONNECT :
      client->connect_date = cur_time ;
      client->used_time = 0 ;
      client->idle_date = cur_time ;
      break ;
    case SETTIME :
      if( (request_Type == IR_SER_STAT) || (request_Type == IR_SER_STAT2) )
	return ;
      set_time = cur_time ;
      client->idle_date = 0 ;
      break ;

    case GETTIME :
      if( (request_Type == IR_SER_STAT) || (request_Type == IR_SER_STAT2) )
	return ;
      client->idle_date = cur_time ;
      client->used_time += (cur_time - set_time) ;
      break ;

    default :
      break ;
  }
}
