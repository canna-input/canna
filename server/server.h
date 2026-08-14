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


#ifndef	SERVER_H
#define SERVER_H

#include "ccompat.h"
#include <stdio.h>
#include <sys/types.h>

#include <sys/time.h>		
#ifdef TIME_WITH_SYS_TIME
# include <time.h>
#endif
#include <sys/times.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include "net.h"
#include <unistd.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#include <assert.h>
#include "RKindep/ecfuncs.h"

/* 自動判別支援コメント: これはEUC-JPだぞ。幅という字があれば大丈夫。 */

typedef struct tagEventMgr EventMgr;
typedef struct tagClientBuf ClientBuf;
typedef struct tagSockHolder SockHolder;
typedef struct tagUserTable UserTable;
typedef struct _Address Address;
typedef struct _Client *ClientPtr;
typedef struct _ClientStat *ClientStatPtr;

#define DEBUG

#include "IR.h"
#include "comm.h"

#if !CANNA_LIGHT
#define USE_EUC_PROTOCOL
#endif /* !CANNA_LIGHT */

#define DDPATH              "canna"
#define DDUSER              "user"
#define DDGROUP             "group"
#define DDPATHLEN           (sizeof(DDPATH) - 1)
#define DDUSERLEN           (sizeof(DDUSER) - 1)
#define DDGROUPLEN          (sizeof(DDGROUP) - 1)

#define DATE_LENGH	    29
#define GETDATE 	    1
#define CONNECT 	    2
#define SETTIME 	    3
#define GETTIME 	    4

#define LOCAL_BUFSIZE		2048

#ifdef DEBUG
#define ir_debug( cannadebug )	 cannadebug
#else
#define ir_debug( cannadebug )	
#endif

/* subset of struct addrinfo */
struct _Address {
    int family;
    size_t len;
#ifdef INET6
    struct sockaddr_storage saddr; /* XXX huge padding */
#else
    struct sockaddr_in saddr;
#endif
};

#define IR_ADDR_INSA(x) ((struct sockaddr_in *)&(x)->saddr)
#define IR_ADDR_IN(x) (&IR_ADDR_INSA(x)->sin_addr)
#ifdef INET6
# define IR_ADDR_IN6SA(x) ((struct sockaddr_in6 *)&(x)->saddr)
# define IR_ADDR_IN6(x) (&IR_ADDR_IN6SA(x)->sin6_addr)
# define IR_ADDR_IN6SCOPE(x) (IR_ADDR_IN6SA(x)->sin6_scope_id)
# ifdef IPV6_V6ONLY
#  define IR_V6ONLY_BIND
# endif /* IPV6_V6ONLY */
# if defined(IR_V6ONLY_BIND) || defined(sun)
#  define IR_V4MAPPED_AVOIDABLE
# endif
# ifndef IR_V4MAPPED_AVOIDABLE
#  error "You need newer IPv6 stack."
# endif
#endif /* INET6 */


/* クライアント毎に作られる、レイヤ5の情報を持つ構造体 */
typedef struct _Client {
    struct tagClientBuf *client_buf ;        /* バッファ */
    int 	usr_no ;		     /* ユーザ管理番号 */
    short 	version_hi ;		     /* protocol major version */
    short 	version_lo ;		     /* protocol miner version */
    ir_time_t	used_time ;		     /* ユーザ消費時間 */
    ir_time_t	idle_date ;		     /* アイドル時間 */
    ir_time_t	connect_date ;		     /* コネクトした時間 */
    char	*username ;		     /* ユーザ名  */
    char	*groupname;		     /* グループ名  */
    char	*hostname ;		     /* ホスト名  */
    Address	hostaddr;		     /* ホストアドレス */
    int 	pcount[ W_MAXREQUESTNO ] ;   /* プロトコルカウント */
    int		*context_flag;               /* コンテクスト管理フラグ */
    int		cfsize, ncon;		     /* 上のテーブルの大きさ管理 */
    char	*clientname ;		     /* クライアント名  */
} ClientRec ;			

typedef struct _ClientStat {
    int 	id ;			     /* ソケット番号 */
    int 	usr_no ;		     /* ユーザ管理番号 */
    ir_time_t	used_time ;		     /* ユーザ消費時間 */
    ir_time_t	idle_date ;		     /* アイドル時間 */
    ir_time_t	connect_date ;		     /* コネクトした時間 */
    int 	pcount[ OLD_MAXREQUESTNO ] ; /* プロトコルカウント */
    char	username[ NAME_LENGTH+1] ;   /* ユーザ名  */
    char	hostname[ HOST_NAME ] ;      /* ホスト名  */
    char	context_flag[ OLD_MAX_CX ] ;	 /* コンテクスト管理フラグ */
} ClientStatRec ;		

typedef struct _AddrList {
    Address addr;
    struct _AddrList *next;
} AddrList;

typedef struct _AccessControlList {
    struct _AccessControlList  *prev ;
    struct _AccessControlList  *next ;
    char *hostname ;
    char *usernames ;
    int  usercnt ;
    AddrList *hostaddrs;
} ACLRec ;

typedef struct _AccessControlList *ACLPtr ;

#ifdef USE_INET_SOCKET
/* flag for using INET Domain Socket */
extern int UseInet;
#ifdef INET6
extern int UseInet6;
#endif /* INET6 */
#endif

extern int PortNumberPlus;
extern UserTable *global_user_table;
#ifdef DEBUG
extern const char *DebugProc[][2];
extern const char *DebugProcWide[][2];
#endif
extern const char *CallFuncName;
extern int (*CallFunc)(ClientPtr *clientp);
extern ACLPtr ACLHead ;
extern int canna_server_hi;
extern int canna_server_lo;

/* misc.c */
#ifdef __STDC__
# define USE_VARARGS
#endif

#ifdef DEBUG
void Dmsg(int Pri, const char *f, ...);
#endif /* DEBUG */
void PrintMsg(const char *f, ...);
void nomem_msg(const char *);

void EarlyInit(int, char **);
int BecomeDaemon(void);
void CloseServer(void);
int CheckSignal(void);
AddrList *GetAddrListFromName(const char *hostname);
AddrList *SearchAddrList(const AddrList *list, const Address *addrp);
void FreeAddrList(AddrList *list);
int NumberAccessControlList(void);
int CheckAccessControlList(Address *hostaddrp, const char *username);
int SetDicHome(ClientPtr client, int cxnum);
ClientPtr *get_all_other_clients(ClientPtr self, size_t *count);
void AllSync(void);
void DetachTTY(void);

/* convert.c */
int ir_nosession(ClientPtr *clientp, ClientBuf *client_buf);
int ir_error(ClientPtr *clientp);
#ifdef DEBUG
void DebugDispKanji(int cxnum, int num);
void DebugDump(int level, const char *buf, int size);
#endif

/* wconvert.c */
int parse_wide_request(int *request, BYTE *data, size_t len,
      const char *username, const char *hostname);
int parse_euc_request(int *request, BYTE *data, size_t len,
      const char *username, const char *hostname);
char *insertUserSla(char *, int);
int checkPermissionToRead(ClientPtr client,
      char *dirname, char *dicname);

/* main.c */
int process_request(
      ClientPtr *clientp, ClientBuf *client_buf,
      BYTE *data, size_t len);

/* session.c */
UserTable *UserTable_new(void);
void UserTable_delete(UserTable *obj);
void close_session(ClientPtr *clientp, int notify);
int open_session(ClientPtr *clientp, char *name, ClientBuf *client_buf);
void ClientStat(ClientPtr client, int type,
      int request_Type, char *return_date);

/* util.c */
size_t ushort2euc(const Ushort *src, size_t srclen,
      char *dest, size_t destlen);
size_t euc2ushort(const char *src, size_t srclen,
      Ushort *dest, size_t destlen);
size_t ushortstrlen(const Ushort *ws);
Ushort *ushortmemchr(const Ushort *ws, int ch, size_t len);
size_t ushortstrcpy(Ushort *wd, const Ushort *ws);
size_t ushortstrncpy(Ushort *wd, const Ushort *ws, size_t len);
int WidenClientContext(ClientPtr cli, size_t n);
int set_cxt(ClientPtr cl, int n);
void off_cxt(ClientPtr cl, int cn);
int chk_cxt(ClientPtr cl, int cn);

#endif	/* SERVER_H */
/* vim: set sw=2: */
