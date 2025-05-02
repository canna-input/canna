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
static char rcs_id[]="@(#) $Id: misc.c,v 1.4.2.1 2002/12/18 08:29:09 aida_s Exp $";
#endif

/* LINTLIBRARY */

#include <stdio.h>
#include <errno.h>
#ifndef __EMX__
#include <syslog.h>
#endif

#ifdef USE_VARARGS
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#endif

#ifdef pcux
#include <sys/fcntl.h>
#else
#include <fcntl.h>
#endif
#ifdef __EMX__
#include <unistd.h>
#endif
#include <signal.h>
#include <pwd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include "IR.h"
#include "net.h"

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

extern void CheckConnections();

void
FatalError();

extern int  errno;

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
#endif

#define MAX_PREMOUNTS 20

char *PreMountTabl[MAX_PREMOUNTS];
int npremounts = 0;
static char *MyName ;
static unsigned long MyAddr = 0;

ACLPtr ACLHead = (ACLPtr)NULL;
static int caught_signal = 0;

static void Reset();
static void parQUIT();

#define USAGE "Usage: cannaserver [-p num] [-l num] [-u userid] [-syslog] [-inet] [-d] [dichome]"
static void
Usage()
{
  FatalError(USAGE);
}

extern void getserver_version pro((void));

int
BecomeDaemon ( argc, argv )
int argc;
char *argv[];	
{
    char *ddname = (char *)NULL;
    char buf[ MAXDATA ];
    int     parent, parentid, i;
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
#endif
#ifdef RK_MMOUNT
	else if( !strcmp( argv[i], "-m") ) {
	  MMountFlag = RK_MMOUNT;
	}
#endif
#ifndef __EMX__
 	else if (!strcmp( argv[i], "-syslog")) {
	  Syslog = 1;
	}
    }

    if (Syslog) {
      openlog("cannaserver", LOG_PID, LOG_DAEMON);
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
	        FatalError("cannaserver: couldn't set userid to %s user\n", userID);
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
	    /* ログファイル作成 */
	    if( (Fp = fopen( LOGFILE, "w" ) ) != NULL ){
		LogLevel = atoi(argv[i]);
		if( LogLevel <= 0 )
		    LogLevel = 1 ;
		ServerLogFp = Fp ;
	    } else {
		perror("Can't Create Log File!!\n");
	    }
	  }
	  else {
	    Usage();
	    /* NOTREACHED */
	  }
	}
    }

#endif /* DEBUG */

    getserver_version() ;

   ir_debug( Dmsg(5, "辞書ホームディレクトリィ = %s\n", ddname ); )

    if ((context = RkwInitialize( (char *)ddname )) < 0)
	FatalError("cannaserver:Initialize failed\n") ;
    free( (char *)ddname ) ;
    RkwCloseContext( context ) ;

    if (gethostname( buf, MAXDATA ) == 0) {
      MyName = malloc(strlen(buf) + 1);
      if (MyName) {
	strcpy(MyName, buf);
      }
    }

   ir_debug( Dmsg(5, "My name is %s\n", MyName ); )

#ifdef DEBUG
    if( DebugMode ) {
	signal(SIGPIPE,  SIG_IGN) ;
	bzero(PreMountTabl, MAX_PREMOUNTS * sizeof(unsigned char *));
	CreateAccessControlList() ;

	return 0; /* デーモンにならない */
    }
#endif
    /*
     * FORK a CHILD
     */

    parentid = getpid() ;

    bzero(PreMountTabl, MAX_PREMOUNTS * sizeof(unsigned char *));

    CreateAccessControlList() ;

    signal(SIGTERM, parQUIT);
#ifndef __EMX__
    if ((parent = fork()) == -1) {
	PrintMsg( "Fork faild\n" );
	exit( 1 ) ;
    }
    if ( parent ) {
	pause() ;
	exit( 0 ) ;
	/* wait( (int *)0 ) ;	*/
    } else
	signal(SIGTERM, SIG_DFL);
    return parentid;
#else
    return 0;
#endif
}

void
CloseServer()
{
#ifndef __EMX__
    if (Syslog) {
      closelog();
    }
#endif
    RkwFinalize() ;
    AllCloseDownClients() ;
}

void
FatalError(f)
    char *f;
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
Dmsg( Pri, f, s0, s1, s2, s3, s4, s5, s6, s7, s8 )
int Pri ;
char *f;
char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8 ;
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
Dmsg(Pri, f, ...)
int Pri;
char *f;
#else
Dmsg(va_alist)
va_dcl
#endif
{
  va_list ap;
  char *args[MAXARGS];
  int argno = 0;

#ifndef __STDC__
  int Pri;
  char *f;
#endif

  va_start(ap);

#ifndef __STDC__
  Pri = va_arg(ap, int);
  f = va_arg(ap, char *);
#endif

  while (++argno < MAXARGS && (args[argno] = va_arg(ap, char *)))
    ;
  args[MAXARGS - 1] = (char *)0;
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
#endif /* USE_VARARGS */
#endif

#ifndef USE_VARARGS
PrintMsg( f, s0, s1, s2, s3, s4, s5, s6, s7, s8 )
char *f;
char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8 ;
{
    long    Time ;
    char    *date ;

#ifndef __EMX__
    if (Syslog) {
      syslog(LOG_WARNING, f, s0, s1, s2, s3, s4, s5, s6, s7, s8);
    } else {
#else
    {
#endif
      Time = time( (long *)0 ) ;
      date = (char *)ctime( &Time ) ;
      date[24] = '\0' ;
      fprintf( stderr, "%s :", date ) ;
      fprintf( stderr, f, s0, s1, s2, s3, s4, s5, s6, s7, s8 );
      fflush( stderr ) ;
    }
}
#else /* USE_VARARGS */
#ifdef __STDC__
PrintMsg(f, ...)
char *f;
#else
PrintMsg(va_alist)
va_dcl
#endif
{
  va_list ap;
  char *args[MAXARGS];
  int argno = 0;
  long    Time;
  char    *date;
#ifndef __STDC__
  char *f;
#endif

  va_start(ap);

#ifndef __STDC__
  f = va_arg(ap, char *);
#endif

  while (++argno < MAXARGS && (args[argno] = va_arg(ap, char *)))
    ;
  args[MAXARGS - 1] = (char *)0;
  va_end(ap);

#ifndef __EMX__
  if (Syslog) {
    syslog(LOG_WARNING, f, args[0], args[1], args[2], args[3], args[4],
	   args[5], args[6], args[7], args[8]);
  } else {
#else
  {
#endif
    Time = time((long *)0);
    date = (char *)ctime(&Time);
    date[24] = '\0';
    fprintf(stderr, "%s :", date);
    fprintf(stderr, f, args[0], args[1], args[2], args[3], args[4],
	    args[5], args[6], args[7], args[8]);
    fflush( stderr ) ;
  }
}
#endif /* USE_VARARGS */

static void
Reset(sig)
int	sig;
{
    caught_signal = sig;
}

void
CheckSignal()
{
#ifdef USE_UNIX_SOCKET
  extern struct sockaddr_un unsock;
#endif
    if( caught_signal == SIGTERM ) {
	PrintMsg( "Cannaserver Terminated\n" ) ;
	CloseServer() ;
    } else if(caught_signal) {
	PrintMsg( "Caught a signal(%d)\n", caught_signal ) ;
    }
#ifdef USE_UNIX_SOCKET
  PrintMsg("remove [%s]\n" ,unsock.sun_path);
  unlink(unsock.sun_path);   /* UNIXドメインで作ったファイルを消す。*/
#endif
  exit(2);
}

static void
parQUIT(sig)
int    sig;
/* ARGSUSED */
{
    exit( 0 ) ;
  /* 何もしない */
}

static
ACLCheckHostName( currentptr )
ACLPtr	currentptr ;
{
    char *hostname = currentptr->hostname ;
    ACLPtr  wp ;

    for( wp = ACLHead; wp != (ACLPtr)NULL; wp = wp->next ) {
	if( (!strcmp( (char *)wp->hostname, (char *)hostname )) ||
	   (wp->hostaddr == currentptr->hostaddr) ) {
	    return( -1 ) ;
	}
    }
    return( 0 ) ;
}

extern void FreeAccessControlList pro((void));

CreateAccessControlList()
{
    char   buf[BUFSIZE];
    char   *wp, *p ;
    ACLPtr  current;
    ACLPtr  prev = (ACLPtr)NULL ;
    FILE    *fp ;
    struct hostent *hp;
    char *hostname, *name;
    int namelen;

    hp = gethostbyname(MyName);
    if (hp) {
      MyAddr = *(unsigned long *)(hp->h_addr);
    }

    if( (fp = fopen( ACCESS_FILE, "r" )) == (FILE *)NULL )
	return( -1 ) ;

    if (ACLHead) {
      FreeAccessControlList();
    }

    while( fgets( (char *)buf, BUFSIZE, fp ) != (char *)NULL ) {
	buf[ strlen( (char *)buf )-1 ] = '\0' ;
	wp = buf ;
	if( !strtok( (char *)wp, ":" ) )
	    continue ;

	if( !(current = (ACLPtr)malloc( sizeof( ACLRec ) )) ) {
	    PrintMsg("Can't create access control list!!" ) ;	
	    fclose( fp ) ;
	    FreeAccessControlList() ;
	    return( -1 ) ;
	}

	bzero( current, sizeof( ACLRec ) ) ;

	if (!strcmp(wp, (char *)MyName)) {
	  name = "unix";
	  namelen = sizeof("unix") - 1;
	}
	else {
	  name = wp;
	  namelen = strlen(wp);
	}
	current->hostname = malloc(namelen + 1);
	if (current->hostname) {
	  strcpy(current->hostname, name);
	}

	/* AccessControlListをインターネットアドレスで管理する */
	/* hosts.cannaからホスト名を求める */
        if (strcmp((char *)current->hostname, "unix")) {
	    hostname = (char *)current->hostname;
	}
	else {
	    hostname = (char *)MyName;
	}
	/* ホスト名からインターネットアドレスを求めて ACLRecに登録する  */
	if ((hp = gethostbyname(hostname)) == (struct hostent *)NULL) {
	    /* インターネットアドレス表記が間違っているので無視する */
	    /* hostsにエントリが無いことをメッセージにだした方が良いか */
	    /* も知れない */
	    if (current->hostname)
		free((char *)current->hostname);
	    free((char *)current);
	    continue;
	}
	current->hostaddr = *(unsigned long *)(hp->h_addr);
	/* 複数のアドレスが入っていることに対応していないなあ */

	if (ACLCheckHostName(current) < 0) {
	  free((char *)current->hostname);
	  free((char *)current);
	  continue;
	}

	wp += ( strlen( (char *)wp )+1 );
	
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

void
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
	    tailp = wp ;
    }

    for( wp = tailp; wp != (ACLPtr)NULL; wp = wp->prev ) {
	if( wp->next )
	    free( wp->next ) ;
    }
    ACLHead = (ACLPtr)NULL ;
}

CheckAccessControlList(hostaddr, username)
unsigned long hostaddr;
char *username;
{
  int i;
  char *userp;
  ACLPtr wp;

  if (!ACLHead) return 0;

  ir_debug(Dmsg(5, "My name is %s\n", MyName));

  if (!hostaddr) { /* つまり、UNIX ドメインだったれば */
    hostaddr = MyAddr;
  }

  for (wp = ACLHead ; wp ; wp = wp->next) {
    /* AccessControlListで持っているインタネットアドレスと一致する
       ものをサーチする */
    if (wp->hostaddr == hostaddr) {
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

NumberAccessControlList()
{
  ACLPtr wp;
  int n;

  for (wp = ACLHead, n = 0; wp ; wp = wp->next) {
    n++;
  }
  return n;
}

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

ConnectClientCount( client, buf, new_socks )
ClientPtr   client ;
ClientRec   *buf[] ;
unsigned long new_socks ;
{
    extern ClientPtr	   *ConnectionTranslation ;
    register ClientPtr	    who ;
    int 		    i, count ;

    bzero((char *)buf, sizeof(ClientPtr) * new_socks);
    for (i = 0, count = 0 ; i < new_socks ; i++) {
	if( ((who = ConnectionTranslation[ i ]) != (ClientPtr)NULL)
						&& ( who != client ) ) {
	    *buf = who ;
	    buf ++ ;
	    count ++ ;
	}
    }
    return( count ) ;
}

AllSync()
{
  extern ClientPtr *ConnectionTranslation;
  extern unsigned long connow_socks;
  ClientPtr client;
  int i, j, *a;
  
  for (i = 0 ; i < connow_socks ; i++) {
    client = ConnectionTranslation[ i ];
    if( client != (ClientPtr)NULL) {
      a = client->context_flag;
      for (j = 0 ; j < client->ncon ; j++) {
	RkwSync(*a++, NULL);
      }
    }
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
#if defined(SVR4) || defined(__convex__) || defined(__BSD_NET2__) || defined(__BSD44__)
    (void)setsid();
#else
#ifdef __EMX__
    (void)_setsid();
#else
#if defined(SYSV) || defined(linux) || defined(__OSF__)
    setpgrp();
#else
    setpgrp(0, getpid());
#endif
#endif
#endif
    
#ifdef TIOCNOTTY
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
