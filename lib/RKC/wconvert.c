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
/*
  This program contains a part of the X library.  The communication part
  of the X library is modified and built into this program.
 */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/


/* LINTLIBRARY */

#include <stdio.h>

#include "sglobal.h"
#include "rkcw.h"
#include "canna/RK.h"
#include "RKindep/file.h"
#include "rkc.h"
#include "conf.h"

#include <sys/types.h>
#include <signal.h>

#include "net.h"

#ifndef CANNAHOSTFILE
#define CANNAHOSTFILE	    "/usr/lib/canna/cannahost"
#endif

#define ReqType0    0
#define ReqType1    1
#define ReqType2    2
#define ReqType3    3
#define ReqType4    4
#define ReqType5    5
#define ReqType6    6
#define ReqType7    7
#define ReqType8    8
#define ReqType9    9
#define ReqType10   10
#define ReqType11   11
#define ReqType12   12
#define ReqType13   13
#define ReqType14   14
#define ReqType15   15
#define ReqType16   16
#define ReqType17   17
#define ReqType18   18
#define ReqType19   19
#define ReqType20   20
#define ReqType21   21

#define SENDBUFSIZE 1024
#define RECVBUFSIZE 1024

int ServerFD ;
unsigned int ServerTimeout;

#ifdef SIGNALRETURNSINT
typedef int sig_ret_type;
#else
typedef void sig_ret_type;
#endif

static sig_ret_type
DoSomething(int sig)
/* ARGSUSED */
{
    errno = EPIPE;
    signal(SIGPIPE, DoSomething);
}

static int
try_connect(int fd, struct sockaddr *addrp, size_t len)
{
    struct timeval timeout;
    if( !ServerTimeout )
	return connect( fd, addrp, len );
    timeout.tv_sec = ServerTimeout / 1000;
    timeout.tv_usec = (ServerTimeout % 1000) * 1000;
    return RkiConnect( fd, addrp, len, &timeout );
}

#ifdef UNIXCONN
#if !defined(__EMX__)
/* UNIXドメインでお話する */
static int
connect_unix(int number)   
{
    struct sockaddr_un unaddr;	    /* UNIX socket address. */
    struct sockaddr *addr;	    /* address to connect to */
    
    /* いろはサーバと、ＵＮＩＸドメインで接続 */
    unaddr.sun_family = AF_UNIX;
    if( number )
	sprintf( unaddr.sun_path,"%s:%d", IR_UNIX_PATH, number ) ;
    else
	strcpy( unaddr.sun_path, IR_UNIX_PATH ) ;

    addr = (struct sockaddr *)&unaddr;
    /*
     * Open the network connection.
     */
    if ((ServerFD = socket((int) addr->sa_family, SOCK_STREAM, 0)) >= 0){
	if( try_connect( ServerFD, addr, sizeof unaddr ) < 0 ) {
	    close( ServerFD ) ;
	    return( -1 ) ;
	}
    }
    return( ServerFD ) ;
}    
#endif
#endif /* UNIXCONN */

#ifdef STREAMCONN
/* ストリームパイプで いろはサーバとお話する */
static int
connect_stream_pipe(int number)
{
    char namebuf[(sizeof(IR_STREAM_PATH)) + 8];
    char buf[ sizeof(struct file *) ] ; 
    struct strbuf ctrlbuf ;
    int retfd, flags, mfd ;
        
    sprintf( namebuf, "%s%d%s", IR_STREAM_PATH, number, "R" ) ;
    
    if( (mfd = open( namebuf, O_RDWR )) < 0 )
	return( -1 ) ;

    if( (retfd = open( IROHA_STREAM, O_RDWR )) < 0 ) {
	int w = errno ;
	
	close ( mfd ) ;
	errno = w ;
	return( -1 ) ;
    }

    if( write (mfd, &mfd, 1) != 1 ) {
	int w = errno ;
	
	close( retfd ) ;
	close( mfd ) ;
	errno = w ;
	return( -1 ) ;
    }
    
    ctrlbuf.maxlen = sizeof( buf ) ;
    ctrlbuf.buf = buf ;
    flags = 0 ;
    if ( getmsg(mfd, &ctrlbuf, (struct strbuf *)NULL, &flags) < 0 ) {
	int w = errno ;
	
	close( retfd ) ;
	close( mfd ) ;
	errno = w ;
	return( -1 ) ;
    }
    
    if( putmsg(retfd, &ctrlbuf, (struct strbuf *) NULL, 0) < 0 ) {
	int w = errno ;
	
	close( retfd ) ;
	close( mfd ) ;
	errno = w ;
	return ( -1 );
    }
    close( mfd ) ;
    return( retfd ) ;
}
#endif /* STREAMCONN */

#ifdef INET6
static int
connect_inet(char *hostname, int number)
{
    struct addrinfo hints, *infolist, *info;
    struct servent *sp ;
    char portbuf[10];
    canna_uint16_t port;

    sp = getservbyname( IR_SERVICE_NAME, "tcp" );
    port = ( sp ? ntohs(sp->s_port) : IR_DEFAULT_PORT ) + number;
    sprintf( portbuf, "%u", (unsigned int)port );

    bzero( &hints, sizeof(hints) );
    hints.ai_flags = 0;
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    if( getaddrinfo( hostname, portbuf, &hints, &infolist ) ) {
	errno = EINVAL;
	return( -1 );
    }

    for( info = infolist; info; info = info->ai_next ) {
	ServerFD = socket( info->ai_family,
		info->ai_socktype, info->ai_protocol );
	if( ServerFD != -1 ) {
	    if ( !try_connect( ServerFD, info->ai_addr, info->ai_addrlen ) ) {
		freeaddrinfo( infolist );
		return( ServerFD );
	    }
	    close( ServerFD );
	}
    }
    freeaddrinfo( infolist );
    return( -1 );
}
#else /* !INET6 */
static int
connect_inet(char *hostname, int number)
{
    struct sockaddr_in inaddr;	    /* INET socket address. */
    canna_in_addr_t hostinetaddr;   /* result of inet_addr of arpa addr */
    struct hostent *host_ptr, workhostbuf ;
    struct servent *sp ;
    int addrlen ;		 
    char *h_addr_ptr;

    /* インターネットドメインで接続する。 */
    if( (host_ptr = gethostbyname( hostname ) )
	                                 == (struct hostent *)NULL) {
	hostinetaddr = inet_addr( hostname );
	if( hostinetaddr == (canna_in_addr_t)-1 ) {
	    /* インターネットアドレス表記が間違っている */
	    errno = EINVAL;
	    return( -1 );
	}
	
	if( !(host_ptr = gethostbyaddr( (char *)&hostinetaddr,
				 sizeof( hostinetaddr ), AF_INET )) ) {
	    host_ptr = &workhostbuf ;
	    host_ptr->h_addrtype = AF_INET ;
# ifdef HAVE_STRUCT_HOSTENT_H_ADDR_LIST
	    host_ptr->h_addr_list = &h_addr_ptr;
#else
	    host_ptr->h_addr = (char *)&hostinetaddr ;
# endif
	    host_ptr->h_length = sizeof( hostinetaddr ) ;
	}
    } else { 
	/* アドレスタイプをチェックする */
	if (host_ptr->h_addrtype != AF_INET) {
	    /* Not an Internet host! */
	    errno = EPROTOTYPE;
	    return( -1 );
	}
    }

    if( (ServerFD = socket( AF_INET, SOCK_STREAM, 0 )) < 0 ) 
	return( -1 ) ;
  
    errno = 0;
    /* /etc/servicesからポート番号を取得する */ 
    sp = getservbyname( IR_SERVICE_NAME, "tcp");
    /* データセット */
    inaddr.sin_family = host_ptr->h_addrtype;
    inaddr.sin_port = (sp ? ntohs(sp->s_port) : IR_DEFAULT_PORT) + number;
    inaddr.sin_port = htons(inaddr.sin_port);
    bcopy( host_ptr->h_addr, &inaddr.sin_addr, sizeof(inaddr.sin_addr) ) ;
    addrlen = sizeof( struct sockaddr_in ) ;
    errno = 0 ; 
    if ( try_connect( ServerFD, (struct sockaddr *)&inaddr, addrlen ) < 0 ) {
#ifdef nodef
	perror("connect") ;
#endif
	close( ServerFD ) ;
	return( -1 ) ;
    }
    return( ServerFD ) ;
}
#endif /* !INET6 */

#define MAX_LIST	128

static int
increment_counter(int flush)   
{
    static int counter = 0 ;

    if( flush )
	counter = 0 ;
    else
	counter++ ;

    if( counter > MAX_LIST - 1 )
	return( -1 ) ;
    
    return( counter ) ;
}

/* 以下の順序で，指定したサーバ名のポインタリストを返す */
static void
rkc_build_cannaserver_list(char **list)
{
    char work[ MAX_HOSTNAME ];
    const char *hostp ;
    char **listp = list;
    FILE *hostfp ;
    
    increment_counter( 1 ) ;

    /* First, check if the server name is specified by API.
       Then make the list of servers from environment variable. */
    if ((hostp = RkwGetServerName()) != NULL ||
	*(hostp = RkcConfMgr_get_string(
		&rkc_config, CONF_CANNAHOST, NULL)) ||
	(hostp = getenv( "CANNAHOST" )) != NULL) {
	char *wp, buf[ MAX_HOSTNAME ] ;

	strncpy( buf, hostp, MAX_HOSTNAME ) ;
	buf[MAX_HOSTNAME - 1] = '\0';				/* S004 */
	for( wp = strtok( buf, "," ); wp && *wp; wp = strtok( NULL, "," ) ){ /* S004 */
	    *listp = (char *)malloc(strlen(wp) + 1);
	    if (*listp) {
	      strcpy(*listp, wp);
	    }
	    if( increment_counter( 0 ) < 0 ) {
		*listp = (char *)NULL ;
		return ;
	    }
	    else {
		listp++ ;
	    }
	}							/* S004 */
    }

    /* CANNAHOSTFILE ファイルからリストを作成する */
#ifdef __EMX__
    if( (hostfp = fopen( CANNAHOSTFILE, "rt" )) != (FILE *)NULL ) {
#else
    if( (hostfp = fopen( CANNAHOSTFILE, "r" )) != (FILE *)NULL ) {
#endif
      while( (hostp = fgets( work, MAX_HOSTNAME, hostfp) ) != NULL ) {
	/* 改行文字をとる */
	work[ strlen( hostp )-1 ] = '\0' ;
	/* リストに格納する */
	*listp = (char *)malloc(strlen(work) + 1);
	if (*listp) {
	  strcpy(*listp, work);
	}
	if( !*listp || ( increment_counter( 0 ) < 0) ) 
	  break ;
	listp++ ;
      }
      fclose( hostfp ) ;
    }
    *listp = (char *)NULL ;
}

/* 引数に NULL ポインタを渡してはいけません。*/
/* それどころか、十分おおきな配列を渡さなければならないのだ */
int
rkc_Connect_Iroha_Server(char *hostname)
{
    char *serverlist[ MAX_LIST ], **listp ;
    int num ;
    char *number ;
#ifdef UNIXCONN
    char *localhost = "unix";
#else
    char *localhost = "localhost";
#endif

    listp = serverlist ;
    if( hostname[ 0 ] == '\0' ) {
	rkc_build_cannaserver_list( listp ) ;
	if( !*listp ) {
	    *listp = (char *)malloc(strlen(localhost) + 1);
	    if (*listp) {
	      strcpy(*listp, localhost);
	    }
	    listp++ ;
	    *listp = (char *)NULL ;
	}	
    }
    else {
	*listp = (char *)malloc(strlen(hostname) + 1);
	if (*listp) {
	  strcpy(*listp, hostname);
	}
	listp++ ;
	*listp = (char *)NULL ;
    }
    
    for( listp = serverlist; *listp; listp++ ) {
	ServerTimeout = RkcConfMgr_get_number(
		&rkc_config, CONF_SERVER_TIMEOUT, *listp );
	/* サーバ起動番号を取得する */
#ifdef INET6
	if( **listp == '[' ) {
	    char *p, *q;
	    p = *listp + 1;
	    q = strchr(p, ']');
	    if( q ) {
		size_t bodylen;
		*( q++ ) = '\0';
		/* ここでの形式チェックは厳密でなくてよい */
		bodylen = strspn( p, "0123456789ABCDEFabcdef:." );
		if( bodylen
			&& ( p[bodylen] == '\0' || p[bodylen] == '%' )
			&& strchr( p, ':' )
			&& ( *q == ':' || *q == '\0' ) ) {
		    if( *q == ':' ) {
			*( q++ ) = '\0';
			num = atoi( q );
			sprintf( hostname,"[%s]:%d", p, num ) ;
		    } else {
			num = 0;
			strcpy( hostname, p );
		    }
		    ServerFD = connect_inet( p, num );
		    if( ServerFD >= 0 )
			break;
		    continue;
		}
	    }
	    ServerFD = -1;
	    errno = EINVAL;
	    continue;
	}
#endif /* INET6 */
	strtok( *listp, ":" ) ;
	number = (char *)strtok( NULL, ":" ) ;
	num = number ? atoi( number ) : 0 ;
	if( num )
	    sprintf( hostname,"%s:%d", *listp, num ) ;
	else
	    strcpy( hostname, *listp ) ;

#if defined(UNIXCONN) || defined(STREAMCONN)
	if ( (strcmp( "unix", *listp ) == 0) ) {
#ifdef UNIXCONN
#ifdef __EMX__
	    ServerFD = -1;
#else
	    ServerFD = connect_unix( num ) ;
#endif
	}
	else { 
#else /* STREAMCONN */
	    /* いろはサーバとストリームパイプで接続 */
	    ServerFD = connect_stream_pipe( num ) ;
	}
        else {
#endif /* UNIXCONN */
#endif /* UNIXCONN || STREAMCONN */
	    ServerFD = connect_inet( *listp, num ) ;
#if defined(UNIXCONN) || defined(STREAMCONN)
        }
#endif
	if( ServerFD >= 0 )
	    break ;
    }
    listp = serverlist ;
    
    while( *listp )
	free( *listp++ ) ;
	
    return( ServerFD ) ;
}

#define HEADER_SIZE ((sizeof(char)) + (sizeof(char)) + SIZEOFSHORT)

/*
 * サーバから返された第一候補列を、第一候補列バッファに格納する。
 */

static int firstKouhoStore(int, BYTE *, int, BYTE *);

static int
firstKouhoStore(int n, BYTE *data, int len, BYTE *dest)
{
    RkcContext *cx = (RkcContext *)dest;
    register Ushort *return_kouho, *wp ;
    register int i, save_len ;
    Ushort *first_kouho = cx->Fkouho ;
    int length ;

    if (n < 0) return n;

    /* コピーすべきバッファの大きさを調べる */
     for( save_len = i = 0; i < cx->curbun; i++ ) {
 	length = ushortstrlen( first_kouho )+1 ;
 	first_kouho += length ;
 	save_len += length ;
     }
 
    if (!(return_kouho = (Ushort *)malloc(save_len * SIZEOFSHORT + len)))
      return( -1 ) ;
 
    wp = return_kouho;
    bcopy(cx->Fkouho, wp, save_len * SIZEOFSHORT);
    wp += save_len ;
    len /= SIZEOFSHORT;
    for (i = 0 ; i < len ; i++) {
      *wp = S2TOS(data); data += SIZEOFSHORT; wp++;
    }
    free((char *)cx->Fkouho);
    cx->Fkouho = return_kouho ;
    return 0;
}

static int firstKouhoStore_2(int, BYTE *, int, BYTE *);

static int
firstKouhoStore_2(int n, BYTE *data, int len, BYTE *dest)
{
    RkcContext *cx = (RkcContext *)dest;
    register Ushort *return_kouho, *wp ;
    register int i ;

    if (n < 0) return n;

    if( !(return_kouho = (Ushort *)malloc( len )) )
	return( -1 );

    wp = return_kouho;
    len /= SIZEOFSHORT;
    for (i = 0 ; i < len ; i++) {
	*wp = S2TOS(data); data += SIZEOFSHORT; wp++;
    }
    free((char *)cx->Fkouho);
    cx->Fkouho = return_kouho ;
    return 0;
}

#define PROTOBUF (16 * 8)

#define TRY_COUNT	    10


/* 
  buf は 4 Byte 以上あり、bufsize >= 4 であることを仮定している
 */

#define READIT(ServerFD, requiredsize, p, bufcnt, rest) /* SUPPRESS622 */\
  do {                                                               \
    int empty_count = 0;                                             \
    while (empty_count < TRY_COUNT && bufcnt < requiredsize) {       \
      struct timeval timeout2 = timeout;                             \
      rki_fd_set rmask2 = rmask;                                     \
      if (ServerTimeout) {                                           \
	int r = select(ServerFD + 1, &rmask2, NULL, NULL, &timeout2); \
	if (r == 0) {                                                \
	  break;                                                     \
	} else if (r == -1) {                                        \
	  if (errno == EINTR)                                        \
	    continue;                                                \
	  else                                                       \
	    break;                                                   \
	}                                                            \
      }                                                              \
      readlen = read(ServerFD, p, rest);                             \
      if (readlen < 0) {                                             \
	if (errno == EINTR) {                                        \
	  continue;                                                  \
	}                                                            \
	else {                                                       \
	  break;                                                     \
	}                                                            \
      }                                                              \
      else if ( readlen == 0 ) {                                     \
	empty_count++;                                               \
      }                                                              \
      else {                                                         \
	empty_count = 0;                                             \
	bufcnt += readlen;                                           \
	p += readlen;                                                \
	rest -= readlen;                                             \
      }                                                              \
    }                                                                \
  } while (0)

#ifdef DEBUGPROTO
static void
printproto(char *p, int n)
{
  int i;

  for (i = 0 ; i < n ; i++) {
    if (i) {
      if ((i %  4) == 0) printf(" ");
      if ((i % 32) == 0) printf("\n");
    }
    printf("%02x", (unsigned)((*p++) & 0xff));
  }
  printf("\n");
}

static void
probe(char *format, int n, int p)
{
  printf(format, n);
  printproto(p, n);
}
#else /* !DEBUGPROTO */
#define probe(a, b, c)
#endif /* !DEBUGPROTO */

/*

  RkcRecvWReply()

   0: Succeed;
  -1: Error;

  RkcRecvWReply はサーバからの reply を read する。とりあえず、RkcRecvWReply
  へはバッファとバッファサイズを渡すが、RkcRecvWReply はバッファが足りないと
  判断すると自分で malloc してそのバッファを使う。RkcRecvWReply がバッファを
  malloc した場合には allocptr にそのバッファへのポインタを返す。
  allocptr に 0 が渡された場合には RkcRecvWReply がバッファサイズが足りないと
  判断した場合は、上記 malloc が行われず RkcRecvWReply はエラーリターンする。

  RkcRecvWReply がエラーリターンした場合は malloc は行われていないと判断
  して良い。

  allocptr: バッファが足りなかった場合、RkcRecvWReply が alloc したバッファ
  len_return: 読んだデータの長さ。NULL を与えれば 格納しない。

 */

#define ReadServer RkcRecvWReply

int
RkcRecvWReply(BYTE *buf, int bufsize, int *len_return, BYTE **allocptr)
{
  BYTE *bufptr = buf, *p = buf, *q;
  int bufcnt = 0, rest = bufsize, readlen;
  int requiredsize = HEADER_SIZE;
  unsigned short len = (unsigned short)0;
  struct timeval timeout;
  rki_fd_set rmask;

  timeout.tv_sec = ServerTimeout / 1000;
  timeout.tv_usec = (ServerTimeout % 1000) * 1000;
  RKI_FD_ZERO(&rmask);
  RKI_FD_SET(ServerFD, &rmask);

  errno = 0;

  READIT(ServerFD, requiredsize, p, bufcnt, rest);
  if (bufcnt < requiredsize) {
    errno = EPIPE;
    close(ServerFD);
    if (allocptr && bufptr != buf) free(bufptr);
    return -1;
  }

  q = buf + 2;
  if (bufsize > 4) len = S2TOS(q);
  if (len_return) *len_return = len;
  requiredsize = len + HEADER_SIZE;
  if (bufsize < requiredsize) {
    if (allocptr && (bufptr = (BYTE *)malloc(requiredsize))) {
      bcopy(buf, bufptr, bufcnt);
      rest = requiredsize - bufcnt;
      p = bufptr + bufcnt;
    }
    else {
      return -1;
    }
  }
  if (bufcnt < requiredsize) {
    READIT(ServerFD, requiredsize, p, bufcnt, rest);
  }
  if (bufcnt < requiredsize) {
    errno = EPIPE;
    close(ServerFD);
    if (allocptr && bufptr != buf) free(bufptr);
    return -1;
  }
  else {
    if (allocptr && bufptr != buf) *allocptr = bufptr;
    probe("Read: %d\n", bufcnt, buf);
    return 0;
  }
}

#define WriteServer RkcSendWRequest

int
RkcSendWRequest(const BYTE *Buffer, int size)
{
    register int todo, retval = 0;
    register int write_stat;
    register const BYTE *bufindex;
    static sig_ret_type (*Sig)(int);
    struct timeval timeout, timeout2;
    rki_fd_set wfds, wfds2;

    timeout.tv_sec = ServerTimeout / 1000;
    timeout.tv_usec = (ServerTimeout % 1000) * 1000;
    RKI_FD_ZERO(&wfds);
    RKI_FD_SET(ServerFD, &wfds);

    errno = 0 ;
    bufindex = Buffer ;
    todo = size ;
    Sig = signal(SIGPIPE, DoSomething);
    while (size) {
	timeout2 = timeout;
	wfds2 = wfds;
	errno = 0;
	probe("Write: %d\n", todo, (char *)bufindex);
	if (ServerTimeout) {
	  int r = select(ServerFD + 1, NULL, &wfds, NULL, &timeout2);
	  if (r == 0) {
	    goto fail;
	  } else if (r == -1) {
	    if (errno == EINTR)
	      continue;
	    else
	      goto fail;
	  }
	}
	write_stat = write(ServerFD, (char *)bufindex, (int) todo);

	if (write_stat >= 0) {
	    size -= write_stat;
	    todo = size;
	    bufindex += write_stat;
	} else if (errno == EWOULDBLOCK) {   /* pc98 */
	    continue ;
	}
	else if (errno == EINTR) {
	    continue;
	}
#ifdef EMSGSIZE
	else if (errno == EMSGSIZE) {
	    if (todo > 1)
		todo >>= 1;
	    else
		continue ;
        }
#endif
	else {
	    goto fail;
	}
    }
    goto last;
fail:
    close( ServerFD ) ;
    retval = -1;
    errno = EPIPE ;
last:
    signal(SIGPIPE, Sig);
    return retval;
}

static int
SendType0Request( /* Initialize */
	long proto,
	long len,
	char *name)
{
  BYTE lbuf[PROTOBUF], *bufp = lbuf, *p;
  long sz = 8 + len;
  int res;

  if (sz <= PROTOBUF || (bufp = (BYTE *)malloc(sz))) {
    p = bufp;
    LTOL4(proto, p); p += SIZEOFLONG;
    LTOL4(len, p);   p += SIZEOFLONG;
    strcpy((char *)p, (char *)name);
    res = WriteServer(bufp, sz);
    if (bufp != lbuf) free((char *)bufp);
    return res;
  }
  else {
    return -1;
  }
}

static int
SendType1Request( /* Finalize , KillServer */
	int majo,
	int mino)
{
  BYTE buf[4];

  buf[0] = (BYTE)majo;
  buf[1] = (BYTE)mino;
  buf[2] = buf[3] = (BYTE)0;

  return WriteServer(buf, sizeof(buf));
}

static int
SendType2Request( /* DuplicateContext */
	int majo,
	int mino,
	int val)
{
  BYTE buf[6], *p = buf;

  *p++ = (BYTE)majo;
  *p++ = (BYTE)mino;
  *p++ = (BYTE)0;
  *p++ = (BYTE)((sizeof(buf)) - HEADER_SIZE);
  STOS2(val, p);

  return WriteServer(buf, sizeof(buf));
}

static int
SendType3Request( /* GetDictionaryList */
	int majo,
	int mino,
	int con,
	int val)
{
  BYTE buf[8], *p = buf;

  *p++ = (BYTE)majo;
  *p++ = (BYTE)mino;
  *p++ = (BYTE)0;
  *p++ = (BYTE)((sizeof(buf)) - HEADER_SIZE);
  STOS2(con, p); p += SIZEOFSHORT;
  STOS2(val, p);

  return WriteServer(buf, sizeof(buf));
}

static int
SendType4Request( /* SubstYomi */
	int majo,
	int mino,
	int con,
	int bgn,
	int end,
	Ushort *wstr,
	int wlen)
{
    int sz = HEADER_SIZE + SIZEOFSHORT * 4 + (SIZEOFSHORT * (wlen + 1));
    int len, i, retval;
    Ushort *wp;
    BYTE lbuf[SENDBUFSIZE], *bufp = lbuf, *p;

    if (sz <= SENDBUFSIZE || (bufp = (BYTE *)malloc(sz))) {
      p = bufp;
      *p++ = majo;
      *p++ = mino;
      len = sz - HEADER_SIZE;
      STOS2(len, p); p += SIZEOFSHORT;
      STOS2(con, p); p += SIZEOFSHORT;
      STOS2(bgn, p); p += SIZEOFSHORT;
      STOS2(end, p); p += SIZEOFSHORT;
      STOS2(wlen, p); p += SIZEOFSHORT;

      for (wp = wstr, i = 0 ; i < wlen ; wp++, i++) {
	STOS2(*wp, p); p += SIZEOFSHORT;
      }
      p[0] = p[1] = (BYTE)0;

      retval = WriteServer(bufp, sz);
      if (bufp != lbuf) free((char *)bufp);
      return retval;
    }
    else {
      return -1;
    }
}

static int
SendType5Request( /* AutoConvert */
	int majo,
	int mino,
	int con,
	int val,
	int mod)
{
  BYTE buf[12], *p = buf;
  long mode = mod;

  *p++ = (BYTE)majo;
  *p++ = (BYTE)mino;
  *p++ = (BYTE)0;
  *p++ = (BYTE)((sizeof(buf)) - HEADER_SIZE);
  STOS2(con, p); p += SIZEOFSHORT;
  STOS2(val, p); p += SIZEOFSHORT;
  LTOL4(mode, p);

  return WriteServer(buf, sizeof(buf));
}

static int
SendType6Request( /* GetYomi */
	int majo,
	int mino,
	int con,
	int bun,
	int val)
{
  BYTE buf[10], *p = buf;

  *p++ = (BYTE)majo;
  *p++ = (BYTE)mino;
  *p++ = (BYTE)0;
  *p++ = (BYTE)((sizeof(buf)) - HEADER_SIZE);
  STOS2(con, p); p += SIZEOFSHORT;
  STOS2(bun, p); p += SIZEOFSHORT;
  STOS2(val, p);

  return WriteServer(buf, sizeof(buf));
}

#define SendType8Request SendType9Request /* GetHinshi */

static int
SendType9Request( /* GetLex */
	int majo,
	int mino,
	int con,
	int bun,
	int cand,
	int val)
{
  BYTE buf[12], *p = buf;

  *p++ = (BYTE)majo;
  *p++ = (BYTE)mino;
  *p++ = (BYTE)0;
  *p++ = (BYTE)((sizeof(buf)) - HEADER_SIZE);
  STOS2(con, p); p += SIZEOFSHORT;
  STOS2(bun, p); p += SIZEOFSHORT;
  STOS2(cand, p); p += SIZEOFSHORT;
  STOS2(val, p);

  return WriteServer(buf, sizeof(buf));
}

static int
SendType10Request( /* EndConvert */
	int majo,
	int mino,
	RkcContext *cx,
	int n,
	int mod)
{
    int sz = HEADER_SIZE + SIZEOFSHORT * 2 + SIZEOFLONG + (SIZEOFSHORT * n);
    int len, i, con = (int)cx->server, retval;
    BYTE lbuf[SENDBUFSIZE], *bufp = lbuf, *p;
    RkcBun *bun;
    long mode = mod;

    if (sz <= SENDBUFSIZE || (bufp = (BYTE *)malloc(sz))) {
      p = bufp;
      *p++ = majo;
      *p++ = mino;
      len = sz - HEADER_SIZE;
      STOS2(len, p); p += SIZEOFSHORT;
      STOS2(con, p); p += SIZEOFSHORT;
      STOS2(n, p); p += SIZEOFSHORT;
      LTOL4(mode, p); p += SIZEOFLONG;

      for (bun = cx->bun, i = 0 ; i < n ; bun++, i++) {
	short cand = (bun->curcand < bun->maxcand) ?  bun->curcand : 0;
	STOS2(cand, p); p += SIZEOFSHORT;
      }
      retval = WriteServer(bufp, sz);
      if (bufp != lbuf) free((char *)bufp);
      return retval;
    }
    else {
      return -1;
    }
}

static int
SendType11Request( /* StoreYomi */
	int majo,
	int mino,
	int con,
	int bun,
	Ushort *wstr,
	int wlen)
{
    int sz = HEADER_SIZE + SIZEOFSHORT * 2 + (SIZEOFSHORT * wlen);
    Ushort *wp;
    int len, i, retval;
    BYTE lbuf[SENDBUFSIZE], *bufp = lbuf, *p;

    if (sz <= SENDBUFSIZE || (bufp = (BYTE *)malloc(sz))) {
      p = bufp;
      *p++ = majo;
      *p++ = mino;
      len = sz - HEADER_SIZE;
      STOS2(len, p); p += SIZEOFSHORT;
      STOS2(con, p); p += SIZEOFSHORT;
      STOS2(bun, p); p += SIZEOFSHORT;

      for (wp = wstr, i = 0 ; i < wlen ; wp++, i++) {
	STOS2(*wp, p); p += SIZEOFSHORT;
      }
      /*
       * このリクエストは実装されて以来、実際には空のwstrを渡すStoreYomi
       * でしか使われていない。また、3.6p1までのサーバにはバグがあり、
       * 3.6まではwstrの領域は空でなくてはならず(ヌル文字も不可)、
       * 3.6p1の場合は常に失敗してしまう。そのため、このリクエストに
       * ついては当面、呼び出し側がヌル終端かwlen=0を保証するものとする。
       * 2003.01.05 aida_s
       */

      retval = WriteServer(bufp, sz);
      if (bufp != lbuf) free((char *)bufp);
      return retval;
    }
    else {
      return -1;
    }
}

static int
SendType12Request( /* DefineWord */
	int majo,
	int mino,
	int con,
	Ushort *wstr,
	char *str)
{
  BYTE lbuf[SENDBUFSIZE], *bufp = lbuf, *p;
  int slen = strlen(str) + 1, wlen = ushortstrlen(wstr) + 1, i;
  int sz = HEADER_SIZE + SIZEOFSHORT + (SIZEOFSHORT * wlen) + slen, len, res;

  if (sz <= SENDBUFSIZE || (bufp = (BYTE *)malloc(sz))) {
    p = bufp;

    *p++ = (BYTE)majo;
    *p++ = (BYTE)mino;
    len = sz - HEADER_SIZE;
    STOS2(len, p); p += SIZEOFSHORT;
    STOS2(con, p); p += SIZEOFSHORT;
    for (i = 0 ; i < wlen ; i++) {
      Ushort wch = wstr[i];

      STOS2(wch, p); p += SIZEOFSHORT;
    }
    bcopy(str, p, slen);

    res = WriteServer(bufp, sz);
    if (bufp != lbuf) free((char *)bufp);
    return res;
  }
  else {
    return -1;
  }
}

static int
SendType13Request(
                                                   /* GetSimpleKanji */
	int majo,
	int mino,
	int con,
	char *str,
	Ushort *wstr,
	int wlen,
	int mxk,
	int mxh)
{
  BYTE lbuf[SENDBUFSIZE], *bufp = lbuf, *p;
  int slen = strlen(str) + 1, i;
  int sz = HEADER_SIZE + SIZEOFSHORT + (SIZEOFSHORT * (wlen + 1)) + slen;
  int len, res;

  if (sz <= SENDBUFSIZE || (bufp = (BYTE *)malloc(sz))) {
    p = bufp;

    *p++ = (BYTE)majo;
    *p++ = (BYTE)mino;
    len = sz - HEADER_SIZE;
    STOS2(len, p); p += SIZEOFSHORT;
    STOS2(con, p); p += SIZEOFSHORT;
    bcopy(str, p, slen); p += slen;
    for (i = 0 ; i < wlen ; i++) {
      Ushort wch = wstr[i];

      STOS2(wch, p); p += SIZEOFSHORT;
    }
    p[0] = p[1] = (BYTE)0; p += SIZEOFSHORT;

    STOS2(wlen, p); p += SIZEOFSHORT;
    STOS2(mxk, p); p += SIZEOFSHORT;
    STOS2(mxh, p);
    res = WriteServer(bufp, sz);
    if (bufp != lbuf) free((char *)bufp);
    return res;
  }
  else {
    return -1;
  }
}

static int
SendType14Request( /* BeginConvert */
	int majo,
	int mino,
	int mod,
	int con,
	Ushort *wstr,
	int wlen)
{
  BYTE lbuf[SENDBUFSIZE], *bufp = lbuf, *p;
  int sz = HEADER_SIZE + SIZEOFLONG + SIZEOFSHORT + (SIZEOFSHORT * (wlen + 1));
  int len, res;
  int i;
  long mode = mod;

  if (sz <= SENDBUFSIZE || (bufp = (BYTE *)malloc(sz))) {
    p = bufp;

    *p++ = (BYTE)majo;
    *p++ = (BYTE)mino;
    len = sz - HEADER_SIZE;
    STOS2(len, p); p += SIZEOFSHORT;
    LTOL4(mode, p); p += SIZEOFLONG;
    STOS2(con, p); p += SIZEOFSHORT;
    for (i = 0 ; i < wlen ; i++) {
      Ushort wch = wstr[i];

      STOS2(wch, p); p += SIZEOFSHORT;
    }
    p[0] = p[1] = (BYTE)0;

    res = WriteServer(bufp, sz);
    if (bufp != lbuf) free((char *)bufp);
    return res;
  }
  else {
    return -1;
  }
}

static int
SendType15Request( /* MountDictionary */
	int majo,
	int mino,
	int mod,
	int con,
	char *str)
{
  BYTE lbuf[SENDBUFSIZE], *bufp = lbuf, *p;
  int slen = strlen(str) + 1;
  int sz = HEADER_SIZE + SIZEOFLONG + SIZEOFSHORT + slen, len, res;
  long mode = mod;

  if (sz <= SENDBUFSIZE || (bufp = (BYTE *)malloc(sz))) {
    p = bufp;

    *p++ = (BYTE)majo;
    *p++ = (BYTE)mino;
    len = sz - HEADER_SIZE;
    STOS2(len, p); p += SIZEOFSHORT;
    LTOL4(mode, p); p += SIZEOFLONG;
    STOS2(con, p); p += SIZEOFSHORT;
    bcopy(str, p, slen);

    res = WriteServer(bufp, sz);
    if (bufp != lbuf) free((char *)bufp);
    return res;
  }
  else {
    return -1;
  }
}

static int
SendType16Request( /* RenameDictionary */
	int majo,
	int mino,
	int mod,
	int con,
	char *ostr,
	char *nstr)
{
  BYTE lbuf[SENDBUFSIZE], *bufp = lbuf, *p;
  int oslen = strlen(ostr) + 1, nslen = strlen(nstr) + 1;
  int sz = HEADER_SIZE + SIZEOFLONG + SIZEOFSHORT + oslen + nslen;
  int len, res;
  long mode = mod;

  if (sz <= SENDBUFSIZE || (bufp = (BYTE *)malloc(sz))) {
    p = bufp;

    *p++ = (BYTE)majo;
    *p++ = (BYTE)mino;
    len = sz - HEADER_SIZE;
    STOS2(len, p); p += SIZEOFSHORT;
    LTOL4(mode, p); p += SIZEOFLONG;
    STOS2(con, p); p += SIZEOFSHORT;
    bcopy(ostr, p, oslen); p += oslen;
    bcopy(nstr, p, nslen);

    res = WriteServer(bufp, sz);
    if (bufp != lbuf) free((char *)bufp);
    return res;
  }
  else {
    return -1;
  }
}

static int
SendType17Request( /* QueryExtension */
	int majo,
	int mino,
	char *str,
	int slen)
{
  BYTE lbuf[SENDBUFSIZE], *bufp = lbuf, *p;
  int sz = HEADER_SIZE + slen, res;

  if (sz <= SENDBUFSIZE || (bufp = (BYTE *)malloc(sz))) {
    p = bufp;

    *p++ = (BYTE)majo;
    *p++ = (BYTE)mino;
    STOS2(slen, p); p += SIZEOFSHORT;
    bcopy(str, p, slen);

    res = WriteServer(bufp, sz);
    if (bufp != lbuf) free((char *)bufp);
    return res;
  }
  else {
    return -1;
  }
}

#ifdef EXTENSION
static int
SendType18Request( /* ListDictionary */
	int majo,
	int mino,
	int con,
	char *str1,
	int s1len,
	char *str2,
	int s2len,
	int val)
{
  BYTE lbuf[SENDBUFSIZE], *bufp = lbuf, *p;
  int sz = HEADER_SIZE + SIZEOFSHORT + s1len + s2len + SIZEOFSHORT;
  int res, len;

  if (sz <= SENDBUFSIZE || (bufp = (BYTE *)malloc(sz))) {
    p = bufp;

    *p++ = (BYTE)majo;
    *p++ = (BYTE)mino;
    len = sz - HEADER_SIZE;
    STOS2(len, p); p += SIZEOFSHORT;
    STOS2(con, p); p += SIZEOFSHORT;
    bcopy(str1, p, s1len); p += s1len;
    if (str2 != (char *)0) {
      bcopy(str2, p, s2len); p += s2len;
    }
    STOS2(val, p);

    res = WriteServer(bufp, sz);
    if (bufp != lbuf) free((char *)bufp);
    return res;
  }
  else {
    return -1;
  }
}
#endif /* EXTENSION */

static int
SendType19Request( /* QueryDictionary */
	int majo,
	int mino,
	int mod,
	int con,
	char *ustr,
	char *dstr)
{
  BYTE lbuf[SENDBUFSIZE], *bufp = lbuf, *p;
  int uslen = strlen(ustr) + 1, dslen = strlen(dstr) + 1;
  int sz = HEADER_SIZE + SIZEOFLONG + SIZEOFSHORT + uslen + dslen;
  int len, res;
  long mode = mod;

  if (sz <= SENDBUFSIZE || (bufp = (BYTE *)malloc(sz))) {
    p = bufp;

    *p++ = (BYTE)majo;
    *p++ = (BYTE)mino;
    len = sz - HEADER_SIZE;
    STOS2(len, p); p += SIZEOFSHORT;
    LTOL4(mode, p); p += SIZEOFLONG;
    STOS2(con, p); p += SIZEOFSHORT;
    bcopy(dstr, p, dslen); p += dslen;
    bcopy(ustr, p, uslen);

    res = WriteServer(bufp, sz);
    if (bufp != lbuf) free((char *)bufp);
    return res;
  }
  else {
    return -1;
  }
}

static int
SendType20Request( /* Through */
	int majo,
	int mino,
	int con,
	int cmd,
	int dsz,
	char *data,
	int bsz)
{
  BYTE lbuf[SENDBUFSIZE], *bufp = lbuf, *p;
  int sz = HEADER_SIZE + SIZEOFSHORT + SIZEOFLONG * 2 + dsz;
  int len, res;
  long cmdl = cmd;
  long bszl = bsz;

  if (sz <= SENDBUFSIZE || (bufp = (BYTE *)malloc(sz))) {
    p = bufp;

    *p++ = (BYTE)majo;
    *p++ = (BYTE)mino;
    len = sz - HEADER_SIZE;
    STOS2(len, p); p += SIZEOFSHORT;
    STOS2(con, p); p += SIZEOFSHORT;
    LTOL4(cmdl, p); p += SIZEOFLONG;
    LTOL4(bszl, p); p += SIZEOFLONG;
    bcopy(data, p, dsz);

    res = WriteServer(bufp, sz);
    if (bufp != lbuf) free((char *)bufp);
    return res;
  }
  else {
    return -1;
  }
}

/* Copy Dic のため */

static int
SendType21Request(
                                                        /* CopyDictionary */
	int majo,
	int mino,
	int mod,
	int con,
	char *dirstr,
	char *ostr,
	char *nstr)
{
  BYTE lbuf[SENDBUFSIZE], *bufp = lbuf, *p;
  int dirlen = strlen(dirstr) + 1; 
  int oslen = strlen(ostr) + 1, nslen = strlen(nstr) + 1;
  int sz = HEADER_SIZE + SIZEOFLONG + SIZEOFSHORT 
                                                +dirlen + oslen + nslen;
  int len, res;
  long mode = mod;

  if (sz <= SENDBUFSIZE || (bufp = (BYTE *)malloc(sz))) {
    p = bufp;

    *p++ = (BYTE)majo;
    *p++ = (BYTE)mino;
    len = sz - HEADER_SIZE;
    STOS2(len, p); p += SIZEOFSHORT;
    LTOL4(mode, p); p += SIZEOFLONG;
    STOS2(con, p); p += SIZEOFSHORT;
    bcopy(dirstr, p, dirlen); p += dirlen;
    bcopy(ostr, p, oslen); p += oslen;
    bcopy(nstr, p, nslen);

    res = WriteServer(bufp, sz);
    if (bufp != lbuf) free((char *)bufp);
    return res;
  }
  else {
    return -1;
  }
}

/* ここまで */

static int
RecvType0Reply( /* Initialize */
	long *rep)
{
  BYTE buf[4];
  long res;

  if (ReadServer(buf, 4, (int *)0, (BYTE **)0) < 0) {
    return -1;
  }
  else {
    res = L4TOL(buf);
    *rep = (long)res;
    return 0;
  }
}

static int
RecvType1Reply( /* GetServerInfo */
	int *n,
	int *vmajp,
	int *vminp)
{
  BYTE lbuf[RECVBUFSIZE], *p, *bufp = lbuf;
  int sz, retval;

  if (ReadServer(lbuf, RECVBUFSIZE, &sz, &bufp) < 0) {
    *n = retval = -1;
  }
  else {
    p = bufp + HEADER_SIZE;
    *n = (int)I8toI32(*p); p++;
    *vmajp = (int)I8toI32(*p); p++;
    *vminp = (int)I8toI32(*p);
    retval = 0;
    if (bufp != lbuf) free((char *)bufp);
  }
  return retval;
}

static int
RecvType2Reply( /* Finalize , KillServer */
	int *rep)
{
  BYTE buf[5];

  if (ReadServer(buf, 5, (int *)0, (BYTE **)0) < 0) {
    return -1;
  }
  else {
    *rep = (int)I8toI32(buf[4]);
    return 0;
  }
}

static int RecvType3Reply(int *, int (*)(int, BYTE *, int, BYTE *), BYTE *);

static int
RecvType3Reply( /* GetHinshi */
	int *n,
	int (*storefunc)(int, BYTE *, int, BYTE *),
	BYTE *extdata)
{
  BYTE lbuf[RECVBUFSIZE], *p, *bufp = lbuf;
  int sz, datalen, retval;

  if (ReadServer(lbuf, RECVBUFSIZE, &sz, &bufp) < 0) {
    retval = -1;
  }
  else {
    p = bufp + HEADER_SIZE;
    retval = (int)I8toI32(*p++);
    datalen = sz - sizeof(char);

    if (storefunc && (*storefunc)(retval, p, datalen, extdata) < 0) {
      *n = retval = -1;
    }
    else {
      *n = retval;
      retval = 0;
    }
    if (bufp != lbuf) free((char *)bufp);
  }
  return retval;
}

static int RecvType4Reply(int *, int (*)(int, BYTE *, BYTE *), BYTE *);

static int
RecvType4Reply( /* GetStatus */
	int *n,
	int (*storefunc)(int, BYTE *, BYTE *),
	BYTE *extdata)
{
  BYTE lbuf[RECVBUFSIZE], *p, *bufp = lbuf;
  short res;
  int sz, retval;

  if (ReadServer(lbuf, RECVBUFSIZE, &sz, &bufp) < 0) {
    retval = -1;
  }
  else {
    p = bufp + HEADER_SIZE;
    res = *p++;
    retval = (int)I8toI32(res);

    if (storefunc && (*storefunc)(retval, p, extdata) < 0) {
      *n = retval = -1;
    }
    else {
      *n = retval;
      retval = 0;
    }
    if (bufp != lbuf) free((char *)bufp);
  }
  return retval;
}

static int
RecvType5Reply( /* CreateContext */
	int *rep)
{
  BYTE buf[6], *p;
  short res;

  if (ReadServer(buf, 6, (int *)0, (BYTE **)0) < 0) {
    return -1;
  }
  else {
    p = buf + 4;
    res = (short)S2TOS(p);
    *rep = (int)I16toI32(res);
    return 0;
  }
}

static int
RecvType6Reply( /* GetDictionaryList */
	BYTE *buf,
	int mxi,
	int *n)
/* ARGSUSED */
{
  BYTE lbuf[RECVBUFSIZE], *p, *bufp = lbuf;
  short res;
  int sz;

  if (ReadServer(lbuf, RECVBUFSIZE, &sz, &bufp) < 0) {
    res = -1;
  }
  else {
    p = bufp + HEADER_SIZE;
    res = (short)S2TOS(p); p += SIZEOFSHORT;
    *n = (int)I16toI32(res);
 /* 次の bcopy はサーバは mxi 以内の長さしか返さないと信じてノーチェック */
    bcopy(p, buf, sz - SIZEOFSHORT);
    res = 0;
    if (bufp != lbuf) free((char *)bufp);
  }
  return res;
}

static int RecvType7Reply(int *, int (*)(int, BYTE *, int, BYTE *), BYTE *);

static int
RecvType7Reply( /* BeginConvert */
	int *n,
	int (*storefunc)(int, BYTE *, int, BYTE *),
	BYTE *extdata)
{
  BYTE lbuf[RECVBUFSIZE], *p, *bufp = lbuf;
  short res;
  int sz, datalen, retval;

  if (ReadServer(lbuf, RECVBUFSIZE, &sz, &bufp) < 0) {
    retval = -1;
  }
  else {
    p = bufp + HEADER_SIZE;
    res = (short)S2TOS(p); p += SIZEOFSHORT;
    datalen = sz - SIZEOFSHORT;
    retval = (int)I16toI32(res);

    if (storefunc && (*storefunc)(retval, p, datalen, extdata) < 0) {
      *n = retval = -1;
    }
    else {
      *n = retval;
      retval = 0;
    }
    if (bufp != lbuf) free((char *)bufp);
  }
  return retval;
}

static int RecvType8Reply(int *, int (*)(int, BYTE *, BYTE *, BYTE *), BYTE *, BYTE *);

static int
RecvType8Reply( /* GetSimpleKanji */
	int *n,
	int (*storefunc)(int, BYTE *, BYTE *, BYTE *),
	BYTE *kdata,
	BYTE *hdata)
{
  BYTE lbuf[RECVBUFSIZE], *p, *bufp = lbuf;
  short res;
  int sz, retval;

  if (ReadServer(lbuf, RECVBUFSIZE, &sz, &bufp) < 0) {
    retval = -1;
  }
  else {
    p = bufp + HEADER_SIZE;
    res = (short)S2TOS(p); p += SIZEOFSHORT;
    retval = (int)I16toI32(res);

    if (storefunc && (*storefunc)(retval, p, kdata, hdata) < 0) {
      *n = retval = -1;
    }
    else {
      *n = retval;
      retval = 0;
    }
    if (bufp != lbuf) free((char *)bufp);
  }
  return retval;
}

#define RecvType9Reply RecvType7Reply /* GetLex */

static int
RecvType10Reply( /* Through */
	int *n,
	char *buf,
	int bsz)
{
  BYTE lbuf[RECVBUFSIZE], *p, *bufp = lbuf;
  int sz, retval, datalen;

  if (ReadServer(lbuf, RECVBUFSIZE, &sz, &bufp) < 0) {
    retval = -1;
  }
  else {
    p = bufp + HEADER_SIZE;
    retval = (int)L4TOL(p); p += SIZEOFLONG;
    datalen = sz - SIZEOFLONG;
    bzero(buf, bsz);
    bcopy(p, buf, datalen);
    *n = retval;
    retval = 0;
    if (bufp != lbuf) free((char *)bufp);
  }
  return retval;
}

static long rkcw_initialize(char *);

static long
rkcw_initialize(char *username)
{
  long reply;
  long len = strlen( (char *)username ) + 1 ;

  if (SendType0Request((long) wInitialize, len, username) == 0 &&
      RecvType0Reply(&reply) == 0) {
    if (reply < 0) {
      close(ServerFD);
    }
    return reply;
  }
  return((long) -1);
}

static int rkcw_finalize(void);

static int
rkcw_finalize(void)
{
  int reply;

  if (SendType1Request(wFinalize, 0) == 0 &&
      RecvType2Reply(&reply) == 0) {
    (void)close( ServerFD ) ;
    return reply;
  }
  return -1;
}

static int rkcw_killserver(void);

static int
rkcw_killserver(void)
{
  int reply;

  if (SendType1Request(wKillServer, 0) == 0 &&
      RecvType2Reply(&reply) == 0) {
    (void)close( ServerFD );
    return reply;
  }
  return -1;
}

static int rkcw_create_context(void);

static int
rkcw_create_context(void)
{
  int context;

  if (SendType1Request(wCreateContext, 0) == 0 &&
      RecvType5Reply(&context) == 0) {
    return context;
  }
  return -1;
}

static int rkcw_duplicate_context(RkcContext *);

static int
rkcw_duplicate_context(register RkcContext *cx)
{
  int context;

  if (SendType2Request(wDuplicateContext, 0, (int)cx->server) == 0 &&
      RecvType5Reply(&context) == 0) {
    return context;
  }
  return -1;
}

static int rkcw_close_context(RkcContext *);

static int
rkcw_close_context(register RkcContext *cx)
{
  int reply;

  if (SendType2Request(wCloseContext, 0, (int)cx->server) == 0 &&
      RecvType2Reply(&reply) == 0) {
    return reply;
  }
  return -1;
}

static int
dictionary_list(int proto, int con, char *dicnames, int mxi)
{
  int res;

  if (SendType3Request(proto, 0, con, mxi) == 0 &&
      RecvType6Reply((BYTE *)dicnames, mxi, &res) == 0) {
    return res;
  }
  return -1;
}

static int rkcw_dictionary_list(RkcContext *, char *, int);

static int
rkcw_dictionary_list(register RkcContext *cx, char *dicnames, int mxi)
{
  return dictionary_list(wGetDictionaryList, (int)cx->server, dicnames, mxi);
}

static int
define_dic(int proto, register RkcContext *cx, char *dicname, Ushort *wordrec)
{
  int reply;

  if (SendType12Request(proto, 0, (int)cx->server, wordrec, dicname) == 0 &&
      RecvType2Reply(&reply) == 0) {
    return reply;
  }
  return -1;
}

static int rkcw_define_dic(RkcContext *, char *, Ushort *);

static int
rkcw_define_dic(register RkcContext *cx, char *dicname, Ushort *wordrec)
{
  return define_dic(wDefineWord, cx, dicname, wordrec);
}

static int rkcw_delete_dic(RkcContext *, char *, Ushort *);

static int
rkcw_delete_dic(register RkcContext *cx, char *dicname, Ushort *wordrec)
{
  return define_dic(wDeleteWord, cx, dicname, wordrec);
}

static int mount_dictionary(int, int, int, char *, int);

static int
mount_dictionary(int majo, int mino, int context, char *data, int mode)
{
  int reply;

  if (SendType15Request(majo, mino, mode, context, data) == 0 &&
      RecvType2Reply(&reply) == 0){
    return reply;
  }
  return -1;
}

static int rkcw_mount_dictionary(RkcContext *, char *, int);

static int
rkcw_mount_dictionary(register RkcContext *cx, char *dicname, int mode)
{
  return mount_dictionary(wMountDictionary, 0, (int)cx->server, dicname, mode);
}

static int rkcw_umount_dictionary(RkcContext *, char *);

static int
rkcw_umount_dictionary(register RkcContext *cx, char *dicname)
{
  return mount_dictionary(wUnmountDictionary, 0, (int)cx->server, dicname, 0);
}

static int rkcw_remount_dictionary(RkcContext *, char *, int);

static int
rkcw_remount_dictionary(register RkcContext *cx, char *dicname, int where)
{
  return mount_dictionary(wRemountDictionary, 0,
			  (int)cx->server, dicname, where);
}

static int rkcw_mount_list(RkcContext *, char *, int);

static int
rkcw_mount_list(register RkcContext *cx, char *dicnames, int mxi)
{
  return dictionary_list(wGetMountDictionaryList,
			 (int)cx->server, dicnames, mxi);
}

#if 0 /* this is not used */
static
rkcw_get_dir_list(register RkcContext *cx, char *ddname, int maxddname)
{
  return dictionary_list(wGetDirectoryList, (int)cx->server,
			 ddname, maxddname);
}
#endif

static int
end_convert(int proto, RkcContext *cx, int n, int mod)
{
  int reply;

  if (SendType10Request(proto, 0, cx, n, mod) == 0 &&
      RecvType2Reply(&reply) == 0) {
    return reply;
  }
  return -1;
}

static int rkcw_convert_end(RkcContext *, int);

static int
rkcw_convert_end(RkcContext *cx, int mode)
{
  return end_convert(wEndConvert, cx, cx->maxbun, mode);
}

static int convStore(int, BYTE *, int, BYTE *);

static int
convStore(int n, BYTE *data, int len, BYTE *dest)
{
  RkcContext *cx = (RkcContext *)dest;

  if (firstKouhoStore(n, data, len, (BYTE *)cx) < 0) {
    rkcw_convert_end(cx, 0); /* サーバ側もRkBgnBunを終了 */
    return -1;
  }
  return n;
}

static int rkcw_convert(RkcContext *, Ushort *, int, int);

static int
rkcw_convert(RkcContext *cx, Ushort *yomi, int length, int mode)
{
  int n;

  if (SendType14Request(wBeginConvert, 0, mode,
			(int)cx->server, yomi, length) == 0&&
      RecvType7Reply(&n, convStore, (BYTE *)cx) == 0) {
    return n;
  }
  return -1;
}

/*
 get_yomi()

 storefunc に 0 をいれたらどうなるか知らないからね。
 */

static int get_yomi(int, int, int, int (*)(int, BYTE *, int, BYTE *), BYTE *);

static int
get_yomi(int proto, int context, int curbun, int (*storefunc)(int, BYTE *, int, BYTE *), BYTE *yomibuf)
{
  int n;

  if (SendType6Request(proto, 0, context, curbun, BUFSIZE) == 0 &&
      RecvType7Reply(&n, storefunc, yomibuf) == 0) {
    return n;
  }
  return -1;
}

static int yomiStore(int, BYTE *, int, BYTE *);

static int
yomiStore(int n, BYTE *data, int len, BYTE *destb)
{
  Ushort *dest = (Ushort *)destb;

  if (!(n < 0)) {
    int i;

    len /= SIZEOFSHORT;
    for (i = 0 ; i < len ; i++) {
      *dest = S2TOS(data); data += SIZEOFSHORT; dest++;
    }
  }
  return 0;
}

static int rkcw_get_yomi(RkcContext *, Ushort *);

static int
rkcw_get_yomi( /* yomip のサイズは無限大かよ？ */
	register RkcContext *cx,
	Ushort *yomip)
{		
  return get_yomi(wGetYomi, (int)cx->server, cx->curbun,
		  yomiStore, (BYTE *)yomip);
}

static int kanjilistStore(int, BYTE *, int, BYTE *);

static int
kanjilistStore(int n, BYTE *data, int len, BYTE *dest)
/* ARGSUSED */
{
  Ushort *p, *wp;

  p = (Ushort *)malloc(len);
  if (p) {
    int i;

    len /= SIZEOFSHORT;
    for (wp = p, i = 0 ; i < len ; i++) {
      *wp = S2TOS(data); data += SIZEOFSHORT; wp++;
    }
    *(Ushort **)dest = p;
    return 0;
  }
  else {
    return -1;
  }
}

static int rkcw_get_kanji_list(RkcContext *);

static int
rkcw_get_kanji_list(register RkcContext *cx)
{		
    RkcBun *bun = &cx->bun[ cx->curbun ] ;

    return get_yomi(wGetCandidacyList, (int)cx->server, cx->curbun,
		    kanjilistStore, (BYTE *)&bun->kanji);
}

static int rkcw_resize(RkcContext *, int);

static int
rkcw_resize(register RkcContext *cx, int yomi_length)
{
  int n;

  if (SendType6Request(wResizePause, 0,
		       (int)cx->server, cx->curbun, yomi_length)
      == 0 &&
      RecvType7Reply(&n, firstKouhoStore, (BYTE *)cx) == 0) {
    return n;
  }
  return -1;
}

static int
store_yomi(int proto, RkcContext *cx, Ushort *yomi, int maxyomi)
{
  int wlen = ushortstrlen(yomi) + 1, n;

  if (maxyomi < wlen) wlen = maxyomi;

  if (SendType11Request(proto, 0, (int)cx->server, cx->curbun, yomi, wlen)
      == 0 && 
      RecvType7Reply(&n, firstKouhoStore, (BYTE *)cx) == 0) {
    return n;
  }
  return -1;
}

static int rkcw_store_yomi(RkcContext *, Ushort *, int);

static int
rkcw_store_yomi(register RkcContext *cx, Ushort *yomi, int maxyomi)
{
  return store_yomi(wStoreYomi, cx, (Ushort *)yomi, maxyomi);
}

static
char *BasicExtension = {
#ifdef EXTENSION
    /* Request Name */
    "GetServerInfo\0GetAccessControlList\0CreateDictioinary\0\
DeleteDictioinary\0RenameDictioinary\0GetWordTextDictioinary\0\
ListDictioinary\0\0"
#else
    "\0"
#endif /* EXTENSION */
} ;

static int
Query_Extension_Ex(char *reqnames)
{
    int datalen = 0, reply;

    while( *(reqnames + datalen) ){
      datalen += strlen(reqnames + datalen) + 1;
    }
    datalen++;

    /* Request Names は '\0' を含む文字列であるためパケットに載せるべき
       大きさが判りにくいので全体の大きさを datalen に指定する． */
    /* 最後に余計な1バイトが付く。とりあえずこのバイトは0にしておく。 */
    if (SendType17Request(wQueryExtensions, 0, reqnames, datalen + 1) == 0 &&
	RecvType2Reply(&reply) == 0) {
      return reply;
    }
    return -1;
}

static int
Query_Extension(void)
{
    return Query_Extension_Ex(BasicExtension);
}

#ifdef EXTENSION
/* ARGSUSED */

static int rkcw_list_dictionary(RkcContext *, char *, char *, int);

static int
rkcw_list_dictionary(register RkcContext *cx, char *dirname, char *dicnames_return, int size)
{
    int extension_base = Query_Extension(), n;
    int slen = strlen((char *)dirname) + 1;

    if( extension_base < 0 )
	return( -1 ) ;

    if (SendType18Request(extension_base + wListDictionary,
			  1, (int)cx->server, 
			  (char *)dirname, slen, (char *)0, 0, size)
	== 0 &&
        RecvType6Reply((BYTE *)dicnames_return, size, &n) == 0) {
      return n;
    }
    return -1;
}

static int rkcw_create_dictionary(RkcContext *, char *, int);

static int
rkcw_create_dictionary(register RkcContext *cx, char *dicname, int mode)
{
    int extension_base = Query_Extension() ;

    if( extension_base < 0 )
	return( -1 ) ;
    return mount_dictionary(extension_base + wCreateDictionary, 1,
			    (int)cx->server, dicname, mode);
}

static int rkcw_delete_dictionary(RkcContext *, char *, int);

static int
rkcw_delete_dictionary(register RkcContext *cx, char *dicname, int mode)
{
    int extension_base = Query_Extension() ;

    if( extension_base < 0 )
	return( -1 ) ;
    return mount_dictionary(extension_base + wDeleteDictionary,
			    1, (int)cx->server,
			    dicname, mode);
}

static int rkcw_rename_dictionary(RkcContext *, char *, char *, int);

static int
rkcw_rename_dictionary(register RkcContext *cx, char *dic, char *newdic, int mode)
{
  int reply;
  int extension_base = Query_Extension() ;

  if( extension_base < 0 )
      return( -1 ) ;

  if (SendType16Request(extension_base + wRenameDictionary,
			1, mode, (int)cx->server, dic,
			newdic) == 0 &&
      RecvType2Reply(&reply) == 0){
    return reply;
  }
  return -1;
}

/* Copy Dic ここから */

/*
  Protocol Version 3.2 からサポート。それ以前のサーバへは送ってはいけない。
  このチェックは rkc.c で行なうこと。
 */

static int rkcw_copy_dictionary(RkcContext *, char *, char *, char *, int);

static int
rkcw_copy_dictionary(register RkcContext *cx, char *dir, char *dic, char *newdic, int mode)
{
  int reply;
  int extension_base = Query_Extension() ;

  if( extension_base < 0 )
      return( -1 ) ;

  if (SendType21Request(extension_base + wCopyDictionary, 1, mode, 
                (int)cx->server, dir, dic, newdic) == 0 &&
                                RecvType2Reply(&reply) == 0){
    return reply;
  }
  return -1;
}
/* ここまで */
/* ARGSUSED */

static int rkcw_get_text_dictionary(RkcContext *, char *, char *, Ushort *, int);

static int
rkcw_get_text_dictionary(register RkcContext *cx, char *dirname, char *dicname, Ushort *info, int infolen)
{
    int extension_base = Query_Extension() ;
    int dirlen = strlen((char *)dirname) + 1;
    int diclen = strlen((char *)dicname) + 1;
    int n, retval = -1;

    if( extension_base < 0 )
	return( -1 ) ;

    if (SendType18Request(extension_base + wGetWordTextDictionary,
			  1, (int)cx->server,
			  dirname, dirlen, dicname, diclen,
			  infolen) == 0&&
	RecvType7Reply(&n, yomiStore, (BYTE *)info) == 0) {
      retval = n;
    }
    return retval;
}

int
rkcw_get_server_info(int *majorp, int *minorp)
{
  int reply, vmajp, vminp;
  int extension_base = Query_Extension() ;

  if( extension_base < 0 )
      return( -1 ) ;

  if (SendType1Request(extension_base + wGetServerInfo, 1) == 0 &&
      RecvType1Reply(&reply, &vmajp, &vminp) == 0) {
    *majorp = vmajp;
    *minorp = vminp;
    return reply;
  }
  return -1;
}
#endif /* EXTENSION */

static int
statusStore(int n, BYTE *data, RkStat *dest)
{
  if (!(n < 0)) {
    dest->bunnum = (int)L4TOL(data);	/* bunsetsu bangou */
    data += SIZEOFLONG;
    dest->candnum = (int)L4TOL(data);	/* kouho bangou */
    data += SIZEOFLONG;
    dest->maxcand = (int)L4TOL(data);	/* sou kouho suu */
    data += SIZEOFLONG;
    dest->diccand = (int)L4TOL(data);	/* jisho ni aru kouho suu */
    data += SIZEOFLONG;
    dest->ylen = (int)L4TOL(data);	/* yomigana no nagasa (in byte) */ 
    data += SIZEOFLONG;
    dest->klen = (int)L4TOL(data);	/* kanji no nagasa (in byte) */
    data += SIZEOFLONG;
    dest->tlen = (int)L4TOL(data);	/* tango no kosuu */
  }
  return 0;
}

static int rkcw_get_stat(RkcContext *, RkStat *);

static int
rkcw_get_stat(register RkcContext *cx, RkStat *stat)
{
    RkcBun *bun = &cx->bun[cx->curbun];
    int n, retval = -1;

    if (SendType6Request(wGetStatus, 0, (int)cx->server,
			 cx->curbun, bun->curcand) == 0 &&
	RecvType4Reply(&n, (int (*)(int, BYTE *, BYTE *))statusStore,
		       (BYTE *)stat) == 0) {
	retval = n;
    }
    return retval;
}

static int
lexStore(int n, BYTE *data, int dlen, RkLex *dest)
/* ARGSUSED */
{
  int i;

  for (i = 0; i < n; i++, dest++) {
    dest->ylen = (int)L4TOL(data);	/* yomigana no nagasa (in byte) */ 
    data += SIZEOFLONG;
    dest->klen = (int)L4TOL(data);	/* kanji no nagasa (in byte) */
    data += SIZEOFLONG;
    dest->rownum = (int)L4TOL(data);	/* row number */
    data += SIZEOFLONG;
    dest->colnum = (int)L4TOL(data);	/* column number */
    data += SIZEOFLONG;
    dest->dicnum = (int)L4TOL(data);	/* dic number */
    data += SIZEOFLONG;
  }
  return 0;
}

static int rkcw_get_lex(RkcContext *, int, RkLex *);

static int
rkcw_get_lex(register RkcContext *cx, int mxi, RkLex *info)
{
    RkcBun *bun = &cx->bun[cx->curbun];
    int n, retval = -1;

    if (SendType9Request(wGetLex, 0, (int)cx->server,
			 cx->curbun, bun->curcand, mxi)	== 0 &&
	RecvType9Reply(&n, (int (*)(int, BYTE *, int, BYTE *))lexStore,
		       (BYTE *)info) == 0) {
	retval = n;
    }
    return retval;
}

/* 逐次変換に必要な関数 */

static int rkcw_autoconv(RkcContext *, int, int);

static int
rkcw_autoconv(RkcContext *cx, int length, int mode)
{
  int reply;

  if (SendType5Request(wAutoConvert, 0, (int)cx->server, length, mode) == 0 &&
      RecvType2Reply(&reply) == 0) {
    return reply;
  }
  return -1;
}

static int rkcw_subst_yomi(RkcContext *, int, int, int, Ushort *, int);

static int
rkcw_subst_yomi(register RkcContext *cx, int nbun, int y_start, int y_end, Ushort *yomi, int y_len)
/* ARGSUSED */
{
  int n;

  if (SendType4Request(wSubstYomi, 0, (int)cx->server, y_start, y_end,
		       yomi, y_len) == 0 &&
      RecvType7Reply(&n, firstKouhoStore_2, (BYTE *)cx) == 0) {
    return n;
  }
  return -1;
}

static int rkcw_flush_yomi(RkcContext *);

static int
rkcw_flush_yomi(register RkcContext *cx)
{		
  int n;

  if (SendType10Request(wFlushYomi, 0, cx, cx->maxbun, 0) == 0 &&
      RecvType7Reply(&n, firstKouhoStore, (BYTE *)cx) == 0) {
    return n;
  }
  return -1;
}

static int rkcw_get_last_yomi(RkcContext *, Ushort *, int);

static int
rkcw_get_last_yomi(register RkcContext *cx, Ushort *yomi, int maxyomi)
{
  int n;

  if (SendType3Request(wGetLastYomi, 0, (int)cx->server, maxyomi) == 0 &&
      RecvType7Reply(&n, yomiStore, (BYTE *)yomi) == 0) {
    return n;
  }
  return -1;
}

static int rkcw_remove_bun(RkcContext *, int);

static int
rkcw_remove_bun(RkcContext *cx, int mode)
{
    register Ushort *return_kouho;
    register int i, len, stat, curbun;
    Ushort *first_kouho = cx->Fkouho;

    stat = end_convert(wRemoveYomi, cx, cx->curbun, mode);

    if( stat < 0 )
	return( -1 );

    /* 削除すべきバッファの大きさを調べる */
    len = 0;
    curbun = cx->curbun + 1;
    for( i = 0; i < curbun; i++ )
	len += ushortstrlen( first_kouho + len ) + 1;
 
    /* コピーすべきバッファの大きさを調べる */
    first_kouho += len;
    len = 0;
    for( i = curbun; i < cx->maxbun; i++ )
	len += ushortstrlen( first_kouho + len ) + 1;

    if( len > 0 ){
	if( !(return_kouho = (Ushort *)malloc( len * SIZEOFSHORT )) )
	    return( -1 );
  
	bcopy( first_kouho, return_kouho, len * SIZEOFSHORT );
	free( (char *)cx->Fkouho );
	cx->Fkouho = return_kouho;
    }
    return( stat );
}

static int
simpleKanjiStore(int n, BYTE *data, Ushort *kdest, Ushort *hdest)
{
  if (!(n < 0)) {
    int i;
    Ushort *wp;

    wp = kdest;
    for (i = 0; i < n; i++) {
      while( *data ){
	*wp = S2TOS(data); data += SIZEOFSHORT; wp++;
      }
      wp[0] = (Ushort)0; data += SIZEOFSHORT; wp++;
    }
    wp[0] = (Ushort)0; data += SIZEOFSHORT; wp++;

    wp = hdest;
    for (i = 0; i < n; i++) {
      while( *data ){
	*wp = S2TOS(data); data += SIZEOFSHORT; wp++;
      }
      wp[0] = (Ushort)0; data += SIZEOFSHORT; wp++;
    }
    wp[0] = (Ushort)0;
  }
  return 0;
}

static int rkcw_get_simple_kanji(RkcContext *, char *, Ushort *, int, Ushort *, int, Ushort *, int);

static int
rkcw_get_simple_kanji(register RkcContext *cx, char *dic, Ushort *yomi, int mxy, Ushort *kan, int mxk, Ushort *hin, int mxh)
{
  int n;

  if (SendType13Request(wGetSimpleKanji, 0, (int)cx->server, dic, yomi,
			mxy, mxk, mxh) == 0 &&
      RecvType8Reply(&n, (int (*)(int, BYTE *, BYTE *, BYTE *))
		     simpleKanjiStore, (BYTE *)kan, (BYTE *)hin) == 0) {
    return n;
  }
  return -1;
}

BYTE *
copyS8(BYTE *src, BYTE *des, int maxlen)
{
  BYTE *p = src, *q = des, *r = des + maxlen - 1; /* 1 for EOS */

  if (!q || !maxlen) r = q; /* don't copy */

  while (*p) {
    if (q < r) {
      *q++ = *p;
    }
    p++;
  }
  if (q) {
    *q = '\0';
  }
  return p + 1;
}

static int
dicinfoStore(int n, BYTE *data, struct DicInfo *dest)
{
  int mlen = dest->di_count;

  if (!(n < 0)) {
    if (dest->di_dic) {
      data = copyS8(data, dest->di_dic, mlen);
    }
    if (dest->di_file) {
      data = copyS8(data, dest->di_file, mlen);
    }
    dest->di_kind = (int)L4TOL(data); data += SIZEOFLONG;
    dest->di_form = (int)L4TOL(data); data += SIZEOFLONG;
    dest->di_count = (unsigned)L4TOL(data); data += SIZEOFLONG;
    dest->di_mode = (int)L4TOL(data); data += SIZEOFLONG;
    dest->di_time = (long)L4TOL(data);
  }
  return 0;
}

static int rkcw_query_dic(RkcContext *, char *, char *, struct DicInfo *);

static int
rkcw_query_dic(RkcContext *cx, char *usrname, char *dicname, struct DicInfo *info)
{
  int reply;

  if (SendType19Request(wQueryDictionary, 0, 0, (int)cx->server,
			usrname, dicname) == 0 &&
      RecvType4Reply(&reply, (int (*)(int, BYTE *, BYTE *))dicinfoStore,
		     (BYTE *)info) == 0) {
    return reply;
  }
  return -1;
}

static int rkcw_get_hinshi(RkcContext *, Ushort *, int);

static int
rkcw_get_hinshi(register RkcContext *cx, Ushort *dst, int mxd)
{
  RkcBun *bun = &cx->bun[cx->curbun];
  int reply;

  if (SendType8Request(wGetHinshi, 0, (int)cx->server, cx->curbun,
		       bun->curcand, mxd)
      == 0 && RecvType3Reply(&reply, yomiStore, (BYTE *)dst) == 0) {
    return reply;
  }
  return -1;
}

static int rkcw_store_range(RkcContext *, Ushort *, int);

static int
rkcw_store_range(register RkcContext *cx, Ushort *yomi, int maxyomi)
{
  return store_yomi(wStoreRange, cx, yomi, maxyomi);
}

static int rkcw_set_locale(RkcContext *, char *);

static int
rkcw_set_locale(register RkcContext *cx, char *locale)
{
  return mount_dictionary(wSetLocale, 0, (int)cx->server, locale, 0);
}

static int rkcw_sync(RkcContext *, char *);

static int
rkcw_sync(register RkcContext *cx, char *dicname)
{
  return mount_dictionary(wSync, 1, (int)cx->server, dicname, 0);
}

static int rkcw_set_app_name(RkcContext *, char *);

static int
rkcw_set_app_name(register RkcContext *cx, char *apname)
{
  return mount_dictionary(wSetApplicationName, 0, (int)cx->server, apname, 0);
}

/*
  Protocol Version 3.2 からサポート。それ以前のサーバへは送ってはいけない。
  このチェックは rkc.c で行なうこと。
 */

static int rkcw_notice_group_name(RkcContext *, char *);

static int
rkcw_notice_group_name(RkcContext *cx, char *groupname)
{
  return mount_dictionary(wNoticeGroupName, 0, (int)cx->server, groupname, 0);
}

/*
  Protocol Version 3.2 からサポート。それ以前のサーバへは送ってはいけない。
  このチェックは rkc.c で行なうこと。
 */

static int rkcw_chmod_dic(RkcContext *, char *, int);

static int
rkcw_chmod_dic(register RkcContext *cx, char *dicname, int mode)
{
  int reply;

  if (SendType15Request(wChmodDictionary, 1, mode, (int)cx->server, dicname)
      == 0 && RecvType5Reply(&reply) == 0) {
    return reply;
  }
  return -1;
}

static int rkcw_through(RkcContext *, int, char *, int, int);

static int
rkcw_through(register RkcContext *cx, int cmd, char *data, int datasz, int bufsz)
{
  int reply;

  if (SendType20Request(wThrough, 0, (int)cx->server, cmd, datasz, data, bufsz)
      == 0 && RecvType10Reply(&reply, data, bufsz) == 0) {
    return reply;
  }
  return -1;
}


struct rkcproto wideproto = {
  rkcw_initialize,
  rkcw_finalize,
  rkcw_close_context,
  rkcw_create_context,
  rkcw_duplicate_context,
  rkcw_dictionary_list,
  rkcw_define_dic,
  rkcw_delete_dic,
  rkcw_mount_dictionary,
  rkcw_remount_dictionary,
  rkcw_umount_dictionary,
  rkcw_mount_list,
  rkcw_convert,
  rkcw_convert_end,
  rkcw_get_kanji_list,
  rkcw_get_stat,
  rkcw_resize,
  rkcw_store_yomi,
  rkcw_get_yomi,
  rkcw_get_lex,
  rkcw_autoconv,
  rkcw_subst_yomi,
  rkcw_flush_yomi,
  rkcw_get_last_yomi,
  rkcw_remove_bun,
  rkcw_get_simple_kanji,
  rkcw_query_dic,
  rkcw_get_hinshi,
  rkcw_store_range,
  rkcw_set_locale,
  rkcw_set_app_name,
  rkcw_notice_group_name,
  rkcw_through,
  rkcw_killserver,
#ifdef EXTENSION
  rkcw_list_dictionary,
  rkcw_create_dictionary,
  rkcw_delete_dictionary,
  rkcw_rename_dictionary,
  rkcw_get_text_dictionary,
  rkcw_sync,
  rkcw_chmod_dic,
  rkcw_copy_dictionary,
#endif /* EXTENSION */
};
