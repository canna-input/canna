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
static char rcs_id[]="@(#) $Id: misc.c,v 1.16.2.2 2003/10/12 11:58:17 aida_s Exp $";
#endif

/* LINTLIBRARY */

#include "server.h"
#ifdef HAVE_SYSLOG /* !__EMX__ */
# include <syslog.h>
#endif

#ifdef USE_VARARGS
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#endif

#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <sys/ioctl.h>

#ifndef DICHOME
#define DICHOME     "/usr/lib/canna/dic"
#endif

#ifndef ERRDIR
#define ERRDIR      "/usr/spool/canna"
#endif

#define ERRFILE     "CANNA"
#define ERRFILE2    "msgs"
#define ERRSIZE     64
#ifndef ACCESS_FILE
#define ACCESS_FILE "/etc/hosts.canna"
#endif

static void FatalError pro((const char *f));
static int CreateAccessControlList pro((void));
static void FreeAccessControlList pro((void));


#ifdef DEBUG
#define LOGFILE "/tmp/canna.log"
static FILE *ServerLogFp = (FILE *)0;
static FILE *Fp;
static int DebugMode = 0;
static int LogLevel = 0;
#endif
static int Syslog = 0; /* syslog を通すかどうかのフラグ */

int PortNumberPlus = 0;
int MMountFlag = 0; /* メモリに辞書をロードするかしないかのフラグ */
static char Name[64];

static char *userID=NULL; /* canna server's user id */

#ifdef USE_INET_SOCKET
/* flag for using INET Domain Socket */
#ifdef USE_UNIX_SOCKET
/* Not to use INET domain socket, if can use Unix Domain Socket */
int UseInet = 0;
#else
/* if can use Unix Domain Socket, Use INET domain socket */
int UseInet = 1;
#endif
#ifdef INET6
int UseInet6 = 0;
#endif /* INET6 */
#endif

#define MAX_PREMOUNTS 20

char *PreMountTabl[MAX_PREMOUNTS];
int npremounts = 0;
static char *MyName ;

ACLPtr ACLHead = (ACLPtr)NULL;
static int caught_signal = 0;
static int openlog_done = 0;
static int rkw_initialize_done = 0;

static void Reset();

#ifdef INET6
#define USAGE "Usage: cannaserver [-p num] [-l num] [-u userid] [-syslog] [-inet] [-inet6] [-d] [dichome]"
#else
#define USAGE "Usage: cannaserver [-p num] [-l num] [-u userid] [-syslog] [-inet] [-d] [dichome]"
#endif
static void
Usage()
{
  FatalError(USAGE);
}

extern void getserver_version pro((void));

void
EarlyInit ( argc, argv )
int argc;
char *argv[];	
{
    char *ddname = (char *)NULL;
    char buf[ MAXDATA ];
    int     i;
    int     context;
    struct  passwd *pwent;

    strcpy( Name, argv[ 0 ] );

    for( i = 1; i < argc; i++ ) {
	if( argv[i][0] == '/' ) {
	    ddname = malloc(strlen(argv[i]) + 1);
	    if( ddname )
		strcpy( (char *)ddname, argv[ i ] );
	}

	if( !strcmp( argv[i], "-p") ) {
	  if (++i < argc) {
	    PortNumberPlus = atoi( argv[i] ) ;
	    if (PortNumberPlus < 0 || PortNumberPlus >= 100) {
		fprintf(stderr, "valid port number range is 0 <= num < 100\n");
		exit(2);
	    }
	  }
	  else {
	    fprintf(stderr, "%s\n", USAGE);
	    exit(2);
	    /* NOTREACHED */
	  }
	}
	else if( !strcmp( argv[i], "-u")) {
	  if (++i < argc) {
	    userID = argv[i];
	  }
	  else {
	    fprintf(stderr, "%s\n", USAGE);
	    exit(2);
	    /* NOTREACHED */
	  }
	}
#ifdef USE_INET_SOCKET
	else if( !strcmp( argv[i], "-inet")) {
	  UseInet = 1;
	}
#ifdef INET6
	else if( !strcmp( argv[i], "-inet6") ) {
	  UseInet6 = 1;
	}
#endif /* INET6 */
#endif
#ifdef RK_MMOUNT
	else if( !strcmp( argv[i], "-m") ) {
	  MMountFlag = RK_MMOUNT;
	}
#endif
#ifdef HAVE_SYSLOG
 	else if (!strcmp( argv[i], "-syslog")) {
	  Syslog = 1;
	}
    }

    if (Syslog) {
      openlog("cannaserver", LOG_PID, LOG_DAEMON);
      openlog_done = 1;
    } /* -syslog だったら、ログファイルを初期化する */
#else
    }

/* TCP/IP スタックが利用可能でない時は終了する */
    if (gethostname( buf, MAXDATA ) != 0) {
	fprintf(stderr,"TCP/IP stack is not working\n") ;
	exit( 1 );
    }
#endif

    if( !ddname ) {
	ddname = malloc(strlen(DICHOME) + 1);
	if( !ddname )
	    FatalError("cannaserver:Initialize failed\n");
	strcpy( (char *)ddname, DICHOME );
    }

    if (userID != NULL) {
        pwent = getpwnam(userID);
	if (pwent) {
	    if(setgid(pwent->pw_gid)) {
	        FatalError("cannaserver:couldn't set groupid to canna user's group\n");	  
	    }
	    if (initgroups(userID, pwent->pw_gid)) {
	        FatalError("cannserver: couldn't init supplementary groups\n");
	    }
	    if (setuid(pwent->pw_uid)) {
	        FatalError("cannaserver: couldn't set userid\n");
	    }
	} else if (userID != NULL) {
	    FatalError("cannaserver: -u flag specified, but canna not run as root\n");
	}
    }

#ifdef DEBUG
    DebugMode = 0 ;
    ServerLogFp = stderr ;
		
    for( i = 1; i < argc; i++ ) {
	if( !strcmp( argv[ i ], "-d" )) {
	    DebugMode = 1 ;
	    LogLevel = 5 ;
	}
	
	if( !strcmp( argv[ i ], "-l" ) ) {
	  if (++i < argc) {
	    LogLevel = atoi(argv[ i ]);
	    if( LogLevel <= 0 )
		LogLevel = 1 ;
	  }
	  else {
	    Usage();
	    /* NOTREACHED */
	  }
	}
    }
    
    if (LogLevel && !DebugMode) {
	/* ログファイル作成 */
	if( (Fp = fopen( LOGFILE, "w" ) ) != NULL ){
	    ServerLogFp = Fp ;
	} else {
	    LogLevel = 0;
	    perror("Can't Create Log File!!\n");
	}
    }

#endif /* DEBUG */

    getserver_version() ;

   ir_debug( Dmsg(5, "辞書ホームディレクトリィ = %s\n", ddname ); )

    if ((context = RkwInitialize( (char *)ddname )) < 0)
	FatalError("cannaserver:Initialize failed\n") ;
    rkw_initialize_done = 1;
    free( (char *)ddname ) ;
    RkwCloseContext( context ) ;

    if (gethostname( buf, MAXDATA ) == 0) {
      MyName = malloc(strlen(buf) + 1);
      if (MyName) {
	strcpy(MyName, buf);
      }
    }

   ir_debug( Dmsg(5, "My name is %s\n", MyName ); )

    bzero(PreMountTabl, MAX_PREMOUNTS * sizeof(unsigned char *));

    CreateAccessControlList() ;
}

int
BecomeDaemon ()
{
    int     parent, parentid;

    if (DebugMode) {
	signal(SIGPIPE,  SIG_IGN) ;
	return 0; /* デーモンにならない */
    }

    parentid = getpid() ;

#ifndef __EMX__
    if ((parent = fork()) == -1) {
	PrintMsg( "Fork faild\n" );
	exit( 1 ) ;
    }
    if ( parent ) {
	_exit( 0 ) ;
    }
    return parentid;
#else
    return 0;
#endif
}

void
CloseServer()
{
#ifdef HAVE_SYSLOG
    if (Syslog && openlog_done) {
      closelog();
    }
#endif
    if (rkw_initialize_done)
	RkwFinalize() ;
}
/* 初期化に失敗した場合に呼ぶ。EventMgr_run()まで来たら呼ばないこと。 */
static void
FatalError(f)
    const char *f;
{
    fprintf(stderr,"%s\n", f);
    CloseServer();
    exit(2);
    /*NOTREACHED*/
}

#define MAXARGS 10

#ifdef DEBUG

#ifndef USE_VARARGS

/* VARARGS */
void
Dmsg( Pri, f, s0, s1, s2, s3, s4, s5, s6, s7, s8 )
int Pri ;
const char *f;
const char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8 ;
{
    if (!ServerLogFp)
	ServerLogFp = stderr;
    if ( LogLevel >= Pri ) {
	fprintf(ServerLogFp , f, s0, s1, s2, s3, s4, s5, s6, s7, s8 );
	fflush( ServerLogFp ) ;
    }
}

#else /* USE_VARARGS */

#ifdef __STDC__
void
Dmsg(int Pri, const char *f, ...)
{
  va_list ap;

  va_start(ap, f);

  if (!ServerLogFp) {
    ServerLogFp = stderr;
  }
  if (LogLevel >= Pri) {
    vfprintf(ServerLogFp, f, ap);
    fflush(ServerLogFp);
  }
  va_end(ap);
}
#else
void
Dmsg(Pri, f, va_alist)
int Pri;
const char *f;
va_dcl
{
  va_list ap;
  const char *args[MAXARGS];
  int argno = 0;

  va_start(ap);

  while (++argno < MAXARGS && (args[argno] = va_arg(ap, const char *)))
    ;
  args[MAXARGS - 1] = (const char *)0;
  va_end(ap);

  if (!ServerLogFp) {
    ServerLogFp = stderr;
  }
  if (LogLevel >= Pri) {
    fprintf(ServerLogFp, f, args[0], args[1], args[2], args[3], args[4],
	    args[5], args[6], args[7], args[8]);
    fflush(ServerLogFp);
  }
}
#endif /* !__STDC__ */
#endif /* USE_VARARGS */
#endif

#ifndef USE_VARARGS
void
PrintMsg( f, s0, s1, s2, s3, s4, s5, s6, s7, s8 )
const char *f;
const char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8 ;
{
    ir_time_t Time ;
    char    *date ;

#ifdef HAVE_SYSLOG
    if (Syslog) {
      syslog(LOG_WARNING, f, s0, s1, s2, s3, s4, s5, s6, s7, s8);
    } else
#endif
    {
      Time = time( NULL ) ;
      date = (char *)ctime( &Time ) ;
      date[24] = '\0' ;
      fprintf( stderr, "%s :", date ) ;
      fprintf( stderr, f, s0, s1, s2, s3, s4, s5, s6, s7, s8 );
      fflush( stderr ) ;
    }
}
#else /* USE_VARARGS */

#if !defined(__STDC__) || (defined(HAVE_SYSLOG) && !defined(HAVE_VSYSLOG))
# define READ_ALL_ARGS
#endif

void
#ifdef __STDC__
PrintMsg(const char *f, ...)
#else
PrintMsg(f, va_alist)
const char *f;
va_dcl
#endif
{
  va_list ap;
#ifdef READ_ALL_ARGS
  const char *args[MAXARGS];
  int argno = 0;
#endif
  ir_time_t Time;
  char    *date;

#ifdef __STDC__
  va_start(ap, f);
#else
  va_start(ap);
#endif

#ifdef READ_ALL_ARGS
  while (++argno < MAXARGS && (args[argno] = va_arg(ap, const char *)))
    ;
  args[MAXARGS - 1] = (const char *)0;
#endif

#ifdef HAVE_SYSLOG
  if (Syslog) {
#ifdef HAVE_VSYSLOG
    vsyslog(LOG_WARNING, f, ap);
#else
    syslog(LOG_WARNING, f, args[0], args[1], args[2], args[3], args[4],
	   args[5], args[6], args[7], args[8]);
#endif
  } else
#endif /* HAVE_SYSLOG */
  {
    Time = time(NULL);
    date = (char *)ctime(&Time);
    date[24] = '\0';
    fprintf(stderr, "%s :", date);
#ifdef __STDC__
    vfprintf(stderr, f, ap);
#else
    fprintf(stderr, f, args[0], args[1], args[2], args[3], args[4],
	    args[5], args[6], args[7], args[8]);
#endif
    fflush( stderr ) ;
  }
  va_end(ap);
}
#endif /* USE_VARARGS */

void
nomem_msg(where)
const char *where;
{
  if (where)
    PrintMsg("%s: out of memory\n", where);
  else
    PrintMsg("out of memory\n");
}

static void
Reset(sig)
int	sig;
{
    caught_signal = sig;
}

int
CheckSignal()
{
    if( caught_signal == SIGTERM ) {
	PrintMsg( "Cannaserver Terminated\n" ) ;
	return 1;
    } else if(caught_signal) {
	PrintMsg( "Caught a signal(%d)\n", caught_signal ) ;
	return 1;
    }
    return 0;
}

static int
AddrAreEqual(x, y)
const Address *x, *y;
{
    int res = 0;
    if (x->family != y->family)
      return 0;
    switch (x->family) {
      case AF_UNIX:
	res = 1;
	break;
      case AF_INET:
	res = IR_ADDR_IN(x)->s_addr == IR_ADDR_IN(y)->s_addr;
	break;
#ifdef INET6
      case AF_INET6:
	res = (IR_ADDR_IN6SCOPE(x) == 0 || IR_ADDR_IN6SCOPE(y) == 0
	    || IR_ADDR_IN6SCOPE(x) == IR_ADDR_IN6SCOPE(y))
	  && IN6_ARE_ADDR_EQUAL(IR_ADDR_IN6(x), IR_ADDR_IN6(y));
	break;
#endif
      default:
	abort();
      /* NOTREACHED */
    }
    return res;
}

AddrList *
GetAddrListFromName(hostname)
const char   *hostname;
{
    AddrList *res = NULL;
#ifdef INET6
    struct addrinfo hints, *info;
    struct addrinfo *infolists[2];
    int i;
#else
    const struct hostent *hent;
    const char *const *haddrp;
    struct in_addr numaddr;
#endif

    if (!strcmp(hostname, "unix")) {
      res = calloc(1, sizeof(AddrList));
      if (!res)
	return NULL;
      res->addr.family = AF_UNIX;
      res->addr.len = 0;
      res->next = NULL;
      return res;
    }

#ifdef INET6
    bzero(&hints, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    infolists[0] = infolists[1] = NULL;
    if (UseInet6) {
	hints.ai_family = PF_INET6;
	getaddrinfo(hostname, NULL, &hints, &infolists[0]);
    }
    if (UseInet) {
	hints.ai_family = PF_INET;
	getaddrinfo(hostname, NULL, &hints, &infolists[1]);
    }

    for (i = 0; i < 2; i++) {
      for (info = infolists[i]; info; info = info->ai_next) {
	AddrList *newnode;
	if (info->ai_family == AF_INET6
	    &&IN6_IS_ADDR_V4MAPPED(
	      &((struct sockaddr_in6 *)info->ai_addr)->sin6_addr))
	  continue;
	newnode = calloc(1, sizeof(AddrList));
	if (!newnode) {
	  freeaddrinfo(infolists[i]);
	  goto fail;
	}
	newnode->addr.family = info->ai_family;
	newnode->addr.len = info->ai_addrlen;
	memcpy(&newnode->addr.saddr, info->ai_addr, info->ai_addrlen);
	newnode->next = res;
	res = newnode;
      }
      if (infolists[i])
	  freeaddrinfo(infolists[i]);
    }
#else /* !INET6 */
    if (
#ifdef HAVE_INET_ATON
	inet_aton(hostname, &numaddr)
#else
	((numaddr.s_addr = inet_addr(hostname)) != (canna_in_addr_t)-1)
#endif
       ) {
      res = calloc(1, sizeof(AddrList));
      if (!res)
	goto fail;
      res->addr.family = AF_INET;
      res->addr.len = sizeof(struct sockaddr_in);
      res->addr.saddr.sin_addr = numaddr;
      res->next = 0;
      return res;
    }
    hent = gethostbyname(hostname);
    if (hent == NULL || hent->h_addrtype != AF_INET)
      return NULL;
#ifndef HAVE_STRUCT_HOSTENT_H_ADDR_LIST
    haddrp = &hent->h_addr;
#else
    for (haddrp = hent->h_addr_list; *haddrp; haddrp++)
#endif
    {
      AddrList *newnode = calloc(1, sizeof(AddrList));
      if (!newnode)
	goto fail;
      newnode->addr.family = AF_INET;
      newnode->addr.len = sizeof(struct sockaddr_in);
      newnode->addr.saddr.sin_addr = *(const struct in_addr *)*haddrp;
      newnode->next = res;
      res = newnode;
    }
#endif /* !INET6 */
    return res;
fail:
    while(res) {
      AddrList *next = res->next;
      free(res);
      res = next;
    }
    return NULL;
}

AddrList *
SearchAddrList(list, addrp)
const AddrList *list;
const Address *addrp;
{
    for (; list; list = list->next)
      if (AddrAreEqual(&list->addr, addrp))
	break;
    return (AddrList *)list;
}

void
FreeAddrList(list)
AddrList *list;
{
    while(list) {
      AddrList *next = list->next;
      free(list);
      list = next;
    };
}

static int
CreateAccessControlList()
{
    char   buf[BUFSIZE];
    char   *wp, *p ;
    ACLPtr  current;
    ACLPtr  prev = (ACLPtr)NULL ;
    FILE    *fp ;
    int namelen;

    if( (fp = fopen( ACCESS_FILE, "r" )) == (FILE *)NULL )
	return( -1 ) ;

    if (ACLHead) {
      FreeAccessControlList();
    }

    while( fgets( (char *)buf, BUFSIZE, fp ) != (char *)NULL ) {
	buf[ strlen( (char *)buf )-1 ] = '\0' ;
	wp = buf ;
#ifdef INET6
	if( *wp == '\0' )
	    continue;
	else if( *wp == '[' ) {
	    size_t bodylen;
	    wp++;
	    p = strchr( wp, ']' );
	    if( !p )
		continue;
	    *( p++ ) = '\0';
	    if( *p == ':' )
		p++;
	    else if( *p != '\0' )
		continue;
	    /* ここでの形式チェックは厳密でなくてよい */
	    bodylen = strspn( wp, "0123456789ABCDEFabcdef:." );
	    if( !bodylen || !( wp[bodylen] == '\0' || wp[bodylen] == '%' )
		    || strchr( wp, ':' ) == NULL )
		continue;
	} else {
	    p = strchr( wp, ':' );
	    if( p )
		*( p++ ) = '\0';
	    else
		p = wp + strlen( wp );
	}
#else /* !INET6 */
	if( !strtok( (char *)wp, ":" ) )
	    continue ;
	p = wp + strlen( (char *)wp ) + 1;
#endif /* !INET6 */

	if( !(current = (ACLPtr)malloc( sizeof( ACLRec ) )) ) {
	    PrintMsg("Can't create access control list!!" ) ;	
	    fclose( fp ) ;
	    FreeAccessControlList() ;
	    return( -1 ) ;
	}

	bzero( current, sizeof( ACLRec ) ) ;

	namelen = strlen(wp);
	current->hostname = malloc(namelen + 1);
	if (current->hostname) {
	  strcpy(current->hostname, wp);
	}

	/* AccessControlListをインターネットアドレスで管理する */
	/* hosts.cannaからホスト名を求める */
	/* ホスト名からインターネットアドレスを求めて ACLRecに登録する  */
	current->hostaddrs = GetAddrListFromName(wp);
	if (!current->hostaddrs) {
	  /* アドレスが見つからない場合 */
	  /* インターネットアドレス表記が間違っているので無視する */
	  /* hostsにエントリが無いことをメッセージにだした方が良いか */
	  /* も知れない */
	  if (current->hostname)
	    free((char *)current->hostname);
	  free((char *)current);
	  continue;
	}
	/* 今のところアドレスが重複していてもそのまま覚えておく */

	wp = p;

	if( strlen( (char *)wp ) ) {
	    current->usernames = malloc(strlen(wp) + 1);
	    if (current->usernames) {
	        strcpy((char *)current->usernames, wp);
		for( p = current->usernames; *p != '\0'; p++ ) {
		    if( *p == ',' ) {
			*p = '\0' ;
			current->usercnt ++ ;
		    }
		}
		current->usercnt ++ ;
	    }
	}
	if( ACLHead ) {
	    current->prev = prev ;
	    prev->next = current ;
	} else {
	    ACLHead = current ;
	    current->prev = (ACLPtr)NULL ;
	}
	current->next = (ACLPtr)NULL ;
	prev = current ;
    }

    fclose( fp ) ;
    return 0;
}

static void
FreeAccessControlList() 
{
    ACLPtr  wp, tailp = (ACLPtr)NULL;

    if( !(wp = ACLHead) )
	return ;

    for( ; wp != (ACLPtr)NULL; wp = wp->next ) {
	    if( wp->hostname )
		free( wp->hostname ) ;
	    if( wp->usernames )
		free( wp->usernames ) ;
	    FreeAddrList( wp->hostaddrs ) ;
	    tailp = wp ;
    }

    for( wp = tailp; wp != (ACLPtr)NULL; wp = wp->prev ) {
	if( wp->next )
	    free( wp->next ) ;
    }
    ACLHead = (ACLPtr)NULL ;
}

int
CheckAccessControlList(hostaddrp, username)
Address *hostaddrp;
const char *username;
{
  int i;
  char *userp;
  ACLPtr wp;

  if (!ACLHead) return 0;

  ir_debug(Dmsg(5, "My name is %s\n", MyName));

  for (wp = ACLHead ; wp ; wp = wp->next) {
    /* AccessControlListで持っているインタネットアドレスと一致する
       ものをサーチする */
    if (SearchAddrList(wp->hostaddrs, hostaddrp)) {
      if (wp->usernames) {
	for (i = 0, userp = wp->usernames ; i < wp->usercnt ; i++) {
	  if (!strcmp(userp, username)) {
	    return 0;
	  }
	  userp += strlen(userp) + 1;
	}
	return -1;
      }
      else {
	return 0;
      }
    }
  }
  return -1;
}

int
NumberAccessControlList()
{
  ACLPtr wp;
  int n;

  for (wp = ACLHead, n = 0; wp ; wp = wp->next) {
    n++;
  }
  return n;
}

int
SetDicHome( client, cxnum )
ClientPtr client ;
int cxnum ;
{
    char dichome[ 256 ] ;

    if (cxnum < 0)
	return( -1 ) ;

    if (client->username && client->username[0]) {
      if (client->groupname && client->groupname[0]) {
	if (strlen(DDUSER) + strlen(client->username) +
	    strlen(DDGROUP) + strlen(client->groupname) +
	    strlen(DDPATH) + 4 >= 256)
	  return ( -1 );
	sprintf(dichome, "%s/%s:%s/%s:%s",
		DDUSER, client->username,
		DDGROUP, client->groupname,
		DDPATH);
      }
      else {
	if (strlen(DDUSER) + strlen(client->username) +
	    strlen(DDPATH) + 2 >= 256)
	  return ( -1 );
	sprintf(dichome, "%s/%s:%s",
		DDUSER, client->username,
		DDPATH);
      }
    }
    else {
      strcpy(dichome, DDPATH);
    }

   ir_debug( Dmsg(5,"辞書ホームディレクトリィ：%s\n", dichome ); )
    if( RkwSetDicPath( cxnum, dichome ) == -1 ) {
	return( -1 ) ;
    }
    return( 1 ) ;
}

ClientPtr *
get_all_other_clients(self, count)
ClientPtr self;
size_t *count;
{
    EventMgrIterator curr, end;
    ClientPtr *res, *p;

    *count = 0;
    EventMgr_clibuf_end(global_event_mgr, &end);

    for (EventMgr_clibuf_first(global_event_mgr, &curr);
	    curr.it_val != end.it_val;
	    EventMgrIterator_next(&curr)) {
	ClientPtr who = ClientBuf_getclient(curr.it_val);
	if (who && who != self)
	    ++*count;
    }

    res = p = malloc(*count * sizeof(ClientPtr));
    if (!res) {
	*count = 0;
	return res;
    }

    for (EventMgr_clibuf_first(global_event_mgr, &curr);
	    curr.it_val != end.it_val;
	    EventMgrIterator_next(&curr)) {
	ClientPtr who = ClientBuf_getclient(curr.it_val);
	if (who && who != self)
	    *p++ = who;
    }
    return res;
}

void
AllSync()
{
    EventMgrIterator curr, end;

    EventMgr_clibuf_first(global_event_mgr, &curr);
    EventMgr_clibuf_end(global_event_mgr, &end);

    for (EventMgr_clibuf_first(global_event_mgr, &curr);
	    curr.it_val != end.it_val;
	    EventMgrIterator_next(&curr)) {
	ClientPtr client = ClientBuf_getclient(curr.it_val);
	int i;
	if (!client)
	    continue;
	for (i = 0; i < client->ncon; ++i)
	    RkwSync(client->context_flag[i], NULL);
    }
}

void
DetachTTY()
{
  char    errfile[ERRSIZE];
  int     errfd;
  
#ifdef DEBUG
  if (!DebugMode)
  {
#endif 
    /* 標準エラー出力をエラーファイルに切り替えて、標準入出力をクローズする */

    if(!Syslog) {    
      sprintf(errfile,"%s/%s%d%s", ERRDIR, ERRFILE, PortNumberPlus, ERRFILE2);
    
      if((errfd = open(errfile, O_CREAT | O_RDWR | O_TRUNC, 0644)) < 0) {
	(void)fprintf(stderr, "Warning: %s: %s open faild\n", Name, errfile);
	(void)perror("");
      } else {
	if(dup2( errfd, fileno(stderr)) < 0) {
	  (void)fprintf(stderr, "Warning: %s: %s dup2 faild\n", Name, errfile);
	  (void)perror("");
	  close(fileno(stderr));
	}
      }
      close(fileno(stdin));
      close(fileno(stdout));
      close(errfd);
    }
    /*
     * TTY の切り離し
     */
#if defined(HAVE_SETSID)
    (void)setsid();
#elif defined(__EMX__)
    (void)_setsid();
#elif defined(SETPGRP_VOID)
    /* defined(SYSV) || defined(linux) || defined(__OSF__) */
    setpgrp();
#else
    setpgrp(0, getpid());
#endif
    
#if defined(TIOCNOTTY) && !defined(HAVE_SETSID)
    {
      int fd = open("/dev/tty", O_RDWR, 0);
      if (fd >= 0) {
	(void)ioctl(fd, TIOCNOTTY, (char *)0);
	(void)close(fd);
      }
    }
#endif
    
#ifdef DEBUG
  }
#endif
  
    /*
     * シグナル処理
     */
    signal(SIGHUP,   SIG_IGN);
    signal(SIGINT,   Reset);
    signal(SIGALRM,  SIG_IGN);
    signal(SIGPIPE,  SIG_IGN) ;
    signal(SIGTERM,  Reset); /* for killserver */

    umask( 002 ) ;
}
