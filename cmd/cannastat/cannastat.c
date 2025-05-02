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


#ifdef ENGINE_SWITCH
#include "RKrename.h"
#endif
#include "ccompat.h"
RCSID("$Id: cannastat.c,v 1.5.2.2 2003/12/27 17:15:21 aida_s Exp $");

#include    <stdio.h>
#include    <time.h>
#include    <sys/types.h>
#include    <errno.h>
#include    <assert.h>

#include    "net.h"
#include    "IR.h"
#include    "rkcapi.h"

#define PROTO	1
#define ALL	2
#define TOTAL	3

#define EATFORMAT "%-10s%4s%4s%5s%16s%9s%7s %-10s %-10s\n"
#define EADFORMAT "%-10s%4d%4d%5d%16s%9s%7s %-10s %-10s\n"

#define ATFORMAT "%-10s%5s%5s%5s%16s%9s%9s  %-s\n"
#define ADFORMAT "%-10s%5d%5d%5d%16s%9s%9s  %-s\n"

#define BTFORMAT "%7s %7s %7s %11s %11s\n"
#define BDFORMAT "%7d %7d %7d %11s %11s\n\n"

#define EXTPROTO 1

#define SIZEOFCHAR 1
#define SIZEOFSHORT 2
#define SIZEOFINT 4
#define SIZEOFLONG 4

#define HEADER_SIZE (SIZEOFCHAR * 2 + SIZEOFSHORT)
#define E_OTHER -1
#define E_PROTOCOL -2
#define E_MEMORY -3
#define E_SEND -4
#define E_RECV -5

/*#define DEBUG*/
#ifdef DEBUG
# define TRACE(x) x
#else
# define TRACE(x)
#endif

typedef struct _Client {
    int 	id ;			     /* ソケット番号 */
    int 	usr_no ;		     /* ユーザ管理番号 */
    ir_time_t	used_time ;		     /* ユーザ消費時間 */
    ir_time_t	idle_date ;		     /* アイドル時間 */
    ir_time_t	connect_date ;		     /* コネクトした時間 */
    char	*username ;		     /* ユーザ名  */
    char	*groupname;		     /* グループ名  */
    char	*hostname ;		     /* ホスト名  */
    char	*clientname ;		     /* クライアント名  */
} ClientRec, *ClientPtr;

typedef unsigned int Uint;

static int ServerVersion ;
static ir_time_t cur_time ;	

static int *TotalReqCount = NULL;
size_t ProtoCount, ListSize, ContextNum ;

static char
*ProtoList = NULL, *ContextFlag = NULL ;
char		major_version, minor_version;

static void DispInfo pro((ClientPtr client, int flag));
static void DispProto pro((ClientPtr client));
static int CreateData pro((const BYTE *readbuf,
	    ClientPtr who, size_t cinfolen));
static void usage pro((void));
static int process_wide_reply pro((const BYTE *reply, size_t len,
	    int argflag, int flag));
static int get_check_str pro((char **dst, const BYTE *src, size_t len));
static int get_check_str_adv pro((char **dst, size_t recvlen,
	    const BYTE **receivep, size_t *requiredsize));

int
main(argc, argv)
int argc ;
char **argv ;
{
    char		cannahostname[ 256 ] ;
    int 		argflag = 0, flag = 0 ;
    int i;
    int proto_major, cx;
    int status, error = E_OTHER;
    int replylen;
    BYTE replybuf[128], reqbuf[HEADER_SIZE];
    BYTE *replyp = replybuf, *p = reqbuf;

    cannahostname[0] = '\0';
    for( i = 1; i < argc; i++ ) {
	if(!strcmp( argv[ i ], "-cs" )
		|| !strcmp( argv[ i ], "-cannaserver" )) {
	  if (++i < argc) {
	    strcpy( cannahostname, argv[i] ) ;
	  } else
	      usage();
	} else if( !strcmp( argv[ i ], "-p" ))	
	    argflag = PROTO ;
	else if( !strcmp( argv[ i ], "-a" ))
	    argflag = ALL ;
	else if( !strcmp( argv[ i ], "-t" ))
	    argflag = TOTAL ;
	else if( !strcmp( argv[ i ], "-v" ))
	    flag = 1 ;
	else
	    usage() ;
    }

    if( (cx = RkwInitialize( cannahostname )) == -1 ){
	fprintf( stderr,"Error Disconnected %s\n", cannahostname );
	exit(2);
    }
    strcpy(cannahostname, RkwGetServerName());

    printf("Connected to %s\n", cannahostname ) ;

    RkwGetProtocolVersion(&proto_major, &ServerVersion);
    if( proto_major < 2 ) {
	printf("Too old cannaserver\n");
	exit(2);
    }

    /* パケット組み立て */
    *p++ = wGetServerInfo; /* major */
    *p++ = EXTPROTO; /* minor */
    STOS2(0, p); p += SIZEOFSHORT; /* datalen */
    if (RkcSendWRequest(reqbuf, HEADER_SIZE)) {
	error = E_SEND;
	goto widelast;
    }

    if (RkcRecvWReply(replybuf, sizeof replybuf, &replylen, &replyp)) {
	error = E_RECV;
	goto widelast;
    }
    error = process_wide_reply(replyp, replylen + HEADER_SIZE, argflag, flag);
widelast:
    if (replyp != replybuf)
	free(replyp);

    RkwFinalize();
    status = 2;
    switch (error) {
	case 0:
	    status = 0;
	    break;
	case E_PROTOCOL:
	    fprintf(stderr, "Protocol error\n");
	    break;
	case E_MEMORY:
	    fprintf(stderr, "Out of memory\n");
	    status = 3;
	    break;
	case E_SEND:
	    fprintf(stderr, "Cannot send request to server\n");
	    break;
	case E_RECV:
	    fprintf(stderr, "Cannot receive reply from server\n");
	    break;
	default:
	    assert(0);
    }
    return status;
}

#ifdef DEBUG
static void
DebugDump( buf, size )
int size ;
const char *buf ;
{
    char buf1[80] ;
    char buf2[17] ;
    char c ;
    int     i, j;
    int     count = 0 ;

    fprintf( stderr, " SIZE = %d\n", size ) ;
    fprintf( stderr, " COUNT  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f      0123456789abcdef\n" ) ;
    for (i = 0; i < size; i += 16) {
	bzero( buf1, 50 ) ;
	bzero( buf2, 17 ) ;
	for (j = 0; j < 16; j++) {
	    if( i+j >= size ) {
		strcat( (char *)buf1, "   " ) ;
		strcat( (char *)buf2, " " ) ;
	    } else {
		sprintf( (char *)buf1,
			"%s%02x ", buf1, (c = buf[i + j]) & 0xFF);
		if((unsigned)(c & 0xff) >= (unsigned)' ' &&
			(unsigned)(c & 0xff) < (unsigned)0x7f )
		    sprintf( (char *)buf2,"%s%c", buf2, c ) ;
		else
		    sprintf( (char *)buf2,"%s.", buf2 ) ;
	    }
	}
	fprintf( stderr," %05x %s     %s\n", count++,  buf1, buf2 );
    }

}
#endif

static int
process_wide_reply(reply, len, argflag, flag)
const BYTE *reply;
size_t len;
int argflag;
int flag;
{
    size_t requiredsize;
    const BYTE *p = reply;
    int r;
    size_t i, cConnectNum;
    ClientRec client;

    requiredsize = 
	HEADER_SIZE
	+ SIZEOFCHAR            /* 終了状態 */
	+ SIZEOFCHAR            /* メジャーバージョン */
	+ SIZEOFCHAR            /* マイナーバージョン */
	+ SIZEOFLONG            /* 時刻 */
	+ SIZEOFSHORT           /* プロトコル数 */
	+ SIZEOFSHORT           /* プロトコル名の長さ */
	+ 0                     /* プロトコル名(まだわからない) */
	+ 0			/* プロトコル使用頻度(まだわからない) */
	+ SIZEOFSHORT           /* クライアント数 */
	+ SIZEOFSHORT           /* コンテキスト数 */
	+ 0                     /* クライアント情報関連(まだわからない) */
	;
    TRACE(fprintf(stderr, "process_wide_reply: len=%u, requiredsize_0=%u\n",
		(Uint)len, (Uint)requiredsize));
    /*TRACE(DebugDump((const char *)reply, len));*/
    if (len < requiredsize)
	goto protoerr;
    if (*p++ != wGetServerInfo /* majo */
	    || *p++ != EXTPROTO) /* mino */
	goto protoerr;
    assert(S2TOS(p) == len - 4), p+= SIZEOFSHORT; /* size */

    if (*p++ != 0) /* stat */
	goto protoerr;
    /* サーババージョンを取得する */
    major_version = *p++;
    minor_version = *p++;
    printf("Canna Server (Ver. %d.%d)\n", major_version, minor_version ) ;

    /* サーバのカレント時間 */
    cur_time = (ir_time_t)L4TOL(p), p+= SIZEOFLONG;

    /* プロトコル数 */
    ProtoCount = S2TOS(p), p+= SIZEOFSHORT;
	
    /* プロトコル名リスト長 */
    ListSize = S2TOS(p), p+= SIZEOFSHORT;

    requiredsize += ListSize	/* プロトコル名(わかった) */
	+ ProtoCount * SIZEOFLONG /* プロトコル使用頻度(わかった) */
	;
    TRACE(fprintf(stderr, "ListSize=%u, ProtoCount=%u, requiredsize_1=%u\n",
		(Uint)ListSize, (Uint)ProtoCount, (Uint)requiredsize));
    if (len < requiredsize)
	goto protoerr;

    /* プロトコル名リスト取得 */
    ProtoList = (char *)malloc( ListSize ) ;
    if (!ProtoList)
	goto nomem;
    memcpy(ProtoList, p, ListSize);
    p += ListSize;
    TRACE(fprintf(stderr, "after ProtoList: off=%x\n", (Uint)(p - reply)));

    /* プロトコル使用頻度取得 */
    TotalReqCount = (int *)calloc( ProtoCount, sizeof( int ) ) ;
    if (!TotalReqCount)
	goto nomem;
    for (i = 0; i < ProtoCount; ++i)
	TotalReqCount[i] = (int)L4TOL(p), p += SIZEOFLONG;
    TRACE(fprintf(stderr, "after TotalReqCount: off=%x\n", (Uint)(p - reply)));

    cConnectNum = S2TOS(p), p+= SIZEOFSHORT;

    if( cConnectNum )
	printf("Total connecting clients %u\n", (Uint)cConnectNum);
    else {
	printf("No clients\n") ;
    }

    if( argflag == TOTAL ) {
	DispProto( (ClientPtr)NULL ) ;
	putchar( '\n' ) ;
	RkwFinalize();
	exit( 0 ) ;
    } else if( !cConnectNum ){
	RkwFinalize();
	exit( 0 ) ;
    }

    if( !flag && (!argflag || (argflag == ALL)) ) {
	if( major_version > 2 )
	    printf( EATFORMAT, "USER_NAME", "ID", "NO", "U_CX", "C_TIME",
		    "U_TIME", "I_TIME", "HOST_NAME", "CLIENT" ) ;
	else
	    printf( ATFORMAT, "USER_NAME", "ID", "NO", "U_CX", "C_TIME",
		    "U_TIME", "I_TIME", "HOST_NAME" ) ;
    } else {
	printf("\n") ;
    }

    ContextNum = S2TOS(p), p+= SIZEOFSHORT;

    ContextFlag = (char *)malloc( ContextNum ) ;
    if (!ContextFlag)
	goto nomem;

    /* クライアント情報関連 */
    for( i = 0; i < cConnectNum ; i++ ) {
	size_t clientinfolen;

	clientinfolen = S2TOS(p), p+= SIZEOFSHORT;
	/* このクライアントについては分かった */
	requiredsize += SIZEOFSHORT + clientinfolen;
	TRACE(fprintf(stderr, "clientinfolen=%u, requiredsize_x=%u\n",
		    (Uint)clientinfolen, (Uint)requiredsize));
	if (len < requiredsize)
	    goto protoerr;
	
	bzero( &client, sizeof( ClientRec ) ) ;
	bzero( ContextFlag, ContextNum ) ;
	r = CreateData( p, &client, clientinfolen ) ;
	if (r)
	    goto last;

	switch( argflag ) {
	    case PROTO :
		printf("%s\n", client.username ) ;
		DispProto( &client ) ;
		putchar( '\n' ) ;
		break ;
	    case ALL :
		DispInfo( &client, flag ) ;
		DispProto( &client ) ;
		putchar( '\n' ) ;
		break ;
	    default :
		DispInfo( &client, flag ) ;
		break ;
	}
	free( client.username ) ;
	free( client.hostname ) ;
	if (major_version > 2 && client.clientname) {
	  free( client.clientname ) ;
	}
	p += clientinfolen;
    }
    assert(p == reply + requiredsize);
    assert(p <= reply + len);
    if (p < reply + len)
	goto protoerr;
    r = 0;
    goto last;
nomem:
    r = E_MEMORY;
    goto last;
protoerr:
    r = E_PROTOCOL;
    goto last;
last:
    free(ProtoList);
    ProtoList = NULL;
    free(TotalReqCount);
    TotalReqCount = NULL;
    free(ContextFlag);
    ContextFlag = NULL;
    return r;
}

static void
DispInfo( client, flag )
register ClientPtr client ;
int flag ;
{
    static char *week[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" } ;
    char	ctime[ 15 ], utime[ 10 ], itime[ 10 ] ;
    char	name[ 10 ], host[ 15 ], appname[15];
    char	*ascdate = (char *)0xdeadbeef /* for gcc */ ;
    int 	i, id, user_no, u_cx ;
    ir_time_t	cdate, udate, idate ;
    struct tm	*tt ;

    id = client->id ;
    user_no = client->usr_no ;
    udate = client->used_time ;

    /* コネクト時間の整形 */
    cdate = client->connect_date ;
    tt = localtime( &cdate ) ;
    if( flag ) {
	ascdate = asctime( tt ) ;
    } else {
	sprintf( ctime,"%s %d", week[ tt->tm_wday ], tt->tm_mday ) ;
	if( tt->tm_hour > 12 )
	  sprintf( ctime,"%s %2d:%02dpm", ctime, tt->tm_hour - 12, tt->tm_min ) ;
	else if( tt->tm_hour == 12 )
	  sprintf( ctime,"%s 12:%02dpm", ctime, tt->tm_min ) ;
	else if( tt->tm_hour == 0 )
	  sprintf( ctime,"%s 12:%02dam", ctime, tt->tm_min ) ;
	else
	  sprintf( ctime,"%s %2d:%02dam", ctime, tt->tm_hour, tt->tm_min ) ;
	
	sprintf( utime,"%02u:%02u:%02u", (Uint)udate/3600,
		(Uint)(udate%3600)/60, (Uint)(udate%3600)%60 ) ;
    }

    if (udate < 3600) {
	if (udate < 60) 
	    sprintf( utime, "      %2u", (Uint)udate);
	else
	    sprintf( utime, "   %2u'%02u", (Uint)(udate/60), (Uint)(udate%60));
    } else {
      sprintf( utime,"%02u:%02u'%02u",
		(Uint)(udate/3600), (Uint)((udate%3600)/60),
		(Uint)((udate%3600)%60) ) ;
    }

    idate = cur_time - client->idle_date ;
    /* １分以内は表示しない */
    if( idate < 60 )
	strcpy( itime, "" ) ;
    else if ( idate < 3600)
	sprintf (itime, "   %2u", (Uint)(idate / 60));
    else
	sprintf( itime,"%2u:%02u",
		(Uint)(idate/3600), (Uint)((idate%3600)/60)) ;
				
    for( i = 0, u_cx = 0; i < ContextNum ; i++ )
	if( ContextFlag[ i ] )
	    u_cx ++ ;

    if( flag ) {
	printf( "USER_NAME    : %s\n", client->username ) ;
	printf( "HOST_NAME    : %s\n", client->hostname ) ;
	if( major_version > 2 )
	    printf( "CLIENT_NAME  : %s\n", client->clientname ) ;
	printf( "CONNECT_DATE : %s", ascdate ) ;
	printf( BTFORMAT,
	"USER_ID", "USER_NO", "USED_CX", "USED_TIME","IDLE_TIME" ) ;
	printf( BDFORMAT, id, user_no, u_cx, utime, itime ) ;
    } else {
	bzero( name, 10 ) ;
	bzero( host, 15 ) ;
	bzero( appname, 15 ) ;
	strncpy( name, client->username, 9 ) ;
	if( major_version > 2 ){
	    strncpy( host, client->hostname, 10 ) ;
	    strncpy( appname, client->clientname, 10 ) ;
	    printf( EADFORMAT, name, id,
			user_no, u_cx, ctime, utime, itime, host, appname ) ;
	} else {
	    strncpy( host, client->hostname, 14 ) ;
	    printf( ADFORMAT, name, id,
			user_no, u_cx, ctime, utime, itime, host ) ;
	}
    }	
}

static void
DispProto( client )
register ClientPtr client ;
{
    register int i ;
    char *protoname ;
    int  *dispdata ;

    protoname = ProtoList ;
    dispdata = TotalReqCount ;

    for( i = 0; i < ProtoCount; i++ ) {
	printf("%23s:%6d", protoname, dispdata[ i ] ) ;
	protoname += ( strlen( protoname ) + 1 ) ;
	if( !((i+1)%2) ) putchar('\n') ;
    }
    putchar('\n') ;			
}

static int
CreateData( readbuf, who, cinfolen )
const BYTE *readbuf ;
ClientPtr   who ;
size_t cinfolen ;
{
    const BYTE *receivep = readbuf ;
    size_t j ;
    int r = 0;
    size_t requiredsize;

    requiredsize =
	(5 * SIZEOFLONG)	/* ソケット番号、ユーザ管理番号、各種時間 */
	+ (ProtoCount * SIZEOFLONG) /* プロトコル使用頻度 */
	+ SIZEOFSHORT		/* ユーザ名の長さ */
	+ SIZEOFSHORT		/* ホスト名の長さ */
	+ ((major_version > 2) ? SIZEOFSHORT : 0)
	    /* クライアント名の長さ */
	+ ContextNum		/* コンテキスト管理フラグ */
	;
    /*TRACE(DebugDump((const char *)readbuf, cinfolen));*/
    TRACE(fprintf(stderr, "CreateData: cinfolen=%u, requiredsize_0=%u\n",
		(Uint)cinfolen, (Uint)requiredsize));
    if (cinfolen < requiredsize)
	goto protoerr;

    who->id = (int)L4TOL(receivep); receivep += SIZEOFLONG;
    who->usr_no = (int)L4TOL(receivep); receivep += SIZEOFLONG;
    who->used_time = (ir_time_t)L4TOL(receivep); receivep += SIZEOFLONG;
    who->idle_date = (ir_time_t)L4TOL(receivep); receivep += SIZEOFLONG;
    who->connect_date = (ir_time_t)L4TOL(receivep); receivep += SIZEOFLONG;

    for( j = 0; j < ProtoCount; j++ )
	TotalReqCount[j] = (int)L4TOL(receivep), receivep += SIZEOFLONG;

    TRACE(fprintf(stderr, "try username, off=%x\n",
		(Uint)(receivep - readbuf)));
    if ((r = get_check_str_adv(&who->username, cinfolen,
		    &receivep, &requiredsize)) != 0)
	goto last;
    TRACE(fprintf(stderr, "try hostname, off=%x\n",
		(Uint)(receivep - readbuf)));
    if ((r = get_check_str_adv(&who->hostname, cinfolen,
		    &receivep, &requiredsize)) != 0)
	goto last;

    if( major_version > 2 ){
	TRACE(fprintf(stderr, "try clientname, off=%x\n",
		    (Uint)(receivep - readbuf)));
	if ((r = get_check_str_adv(&who->clientname, cinfolen,
			&receivep, &requiredsize)) != 0)
	    goto last;
    }

    memcpy(ContextFlag, receivep, ContextNum);
    receivep += ContextNum;
    TRACE(fprintf(stderr, "last requiredsize=%u\n", (Uint)requiredsize));
    assert(receivep == readbuf + requiredsize);
    assert(cinfolen >= requiredsize);
    if (cinfolen > requiredsize)
	goto protoerr;
    goto last;
    /*
memerr:
    r = E_MEMORY;
    goto last;
    */
protoerr:
    r = E_PROTOCOL;
    goto last;
last:
    return r;
}

static int
get_check_str(dst, src, len)
char **dst;
const BYTE *src;
size_t len;
{
    size_t body;
    *dst = NULL;
    if (src[len - 1] != '\0')
	return E_PROTOCOL;
    body = strlen((const char *)src);
    if (body != len - 2 && body != len - 1)
	return E_PROTOCOL;
    *dst = strdup((const char *)src);
    return (*dst) ? 0 : E_MEMORY;
}

static int
get_check_str_adv(dst, recvlen, receivep, requiredsize)
char **dst;
size_t recvlen;
const BYTE **receivep;
size_t *requiredsize;
{
    size_t len;
    const BYTE *p = *receivep;

    *dst = NULL;
    len = S2TOS(p), p+= SIZEOFSHORT;
    TRACE(fprintf(stderr, "get_check_str_adv: len=%u, req=%u, recvlen=%u\n",
		(Uint)len, (Uint)*requiredsize, (Uint)recvlen));
    *requiredsize += len;
    if (recvlen < *requiredsize)
	return E_PROTOCOL;
    *receivep = p + len;
    return get_check_str(dst, p, len);
}

static void
usage()
{
    fprintf( stderr, "usage: cannastat [-cs | -cannaserver hostname] [-a|-v]\n" ) ;
    fprintf( stderr, "                 [-cs | -cannaserver hostname] [-t]\n" ) ;
    fprintf( stderr, "                 [-cs | -cannaserver hostname] [-p]\n" ) ;

    fflush( stderr ) ;
    exit( 0 ) ;
}

/* vim: set sw=4: */
