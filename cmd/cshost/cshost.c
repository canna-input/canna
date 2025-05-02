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

#ifndef lint
static char sccs_id[]="@(#) NEC UNIX( PC-UX/EWS-UX ) cshost.c 2.1 91/11/11 11:17:51";
static char rcs_id[] = "$Id: cshost.c,v 1.3.2.1 2003/10/09 15:29:10 aida_s Exp $";
#endif

/*
 * MODIFICATION HISTORY
 *	S000	funahasi@oa2	Fri Dec  4 02:44:09 JST 1992
 *		- cshostから rkcの内部関数を呼ぶのを止める
 *		  rkcに interfaceを作ったのでそれを使用する．
 *	S001	funahasi@oa2	Fri Jan  8 16:02:31 JST 1993
 *		- fixed bug rkc_Connect_iroha_Server()を RkInitialize()に
 *		  変えたために cannahostnameが設定されなくなった．
 *	S002	funahasi@oa2	Mon Jan 11 13:04:44 JST 1993
 *		- fixed bug cannahostnameを "unix"で初期化したため環境変数
 *		  CANNAHOSTを使用できなくなった．
 *		- "-cs"optionで hostnameがない時は，usageを出力する．
 */

#ifdef ENGINE_SWITCH
#include "RKrename.h"
#endif

#include    <stdio.h>
#include    <sys/types.h>

#include    "IR.h"
#include    "net.h"
#include    "rkcapi.h"
#include    "RKindep/ecfuncs.h"
#include    <assert.h>

#define EXTPROTO 1

#define SIZEOFCHAR 1
#define SIZEOFSHORT 2
#define SIZEOFINT 4
#define SIZEOFLONG 4

#define HEADER_SIZE (SIZEOFCHAR * 2 + SIZEOFSHORT)

static int CannaDispControlList pro((void));
static void usage pro((void));

int
main(argc, argv)
int argc ;
char **argv ;
{
    char		cannahostname[256];
    int 		i ;
    int proto_major, proto_minor, cx;				/* S000 */
    int status;

    cannahostname[0] = '\0';					/* S002 */
    for( i = 1; i < argc; i++ ) {
	if(!strcmp( argv[ i ], "-cs" )
		    || !strcmp( argv[ i ], "-cannaserver" )) {
	  if (++i < argc) {
	    RkiStrlcpy( cannahostname, argv[i], sizeof cannahostname ) ;
	  } else						/* S002 */
	      usage();						/* S002 */
	} else
	    usage() ;
    }
							/* begin:S000 */
    if( (cx = RkwInitialize( cannahostname )) == -1 ){
	fprintf( stderr,"Error Disconnected %s\n", cannahostname );
	exit(2);
    }
    RkiStrlcpy(cannahostname, RkwGetServerName(), sizeof cannahostname );
							/* end:S000 */
    printf("Connected to %s\n", cannahostname ) ;
							/* begin:S000 */
    RkwGetProtocolVersion(&proto_major, &proto_minor);
    if( proto_major < 2 ) {
	printf("Too old cannaserver\n");
	status = 2;
	goto last;
    }
							/* end:S000 */
    status = CannaDispControlList() ;
last:
    RkwFinalize();						/* S000 */
    return status;
}

static int
CannaDispControlList()
{
    int     HostNum;
    int     i ;
    BYTE reqbuf[HEADER_SIZE], replybuf[128], *replyp = replybuf;
    BYTE *wp, *endp;
    int replylen, status;

    /*	パケット組み立て */
    wp = reqbuf;
    *wp++ = wGetAccessControlList;
    *wp++ = EXTPROTO;
    STOS2(0, wp); wp += SIZEOFSHORT;
    if (RkcSendWRequest(reqbuf, HEADER_SIZE)) {
	fprintf(stderr, "Cannot send request to server\n");
	goto fail;
    }
    if (RkcRecvWReply(replybuf, sizeof replybuf, &replylen, &replyp)) {
	fprintf(stderr, "Cannot receive reply from server\n");
	goto fail;
    }

    wp = replyp;
    if (*wp++ != wGetAccessControlList
	    || *wp++ != EXTPROTO)
	goto protoerr;
    assert(S2TOS(wp) == replylen); wp += SIZEOFSHORT;
    
    endp = wp + replylen;
    if (endp < wp + SIZEOFSHORT)
	goto protoerr;
    HostNum = S2TOS(wp); wp += SIZEOFSHORT;
    printf("access control enabled\n" ) ;
    if (HostNum) {
	if ((endp < wp + 2)
		|| *(endp - 1) != '\0'
		|| *(endp - 2) != '\0')
	    goto protoerr;
    }
    for( i = 0; i < HostNum; i++ ) {
	if (wp == endp)
	    goto protoerr;
	assert(wp + 2 <= endp);
	printf("HOST NAME:%s\n", wp ) ;
	wp += strlen( wp ) + 1 ;
	if( *wp ) {
	    printf("USER NAME:" ) ;
	    while( *wp ) {
		printf("%s ", wp ) ;
		wp += strlen( wp ) + 1 ;
	    }
	} else
	    printf("ALL USER" ) ;
	printf("\n\n") ;
	wp++;
    }
    status = 0;
    goto last;
protoerr:
    fprintf(stderr, "Protocol error\n");
fail:
    status = 2;
last:
    return status;
}

static void
usage()
{
    fprintf( stderr, "usage: cshost [-cs | -cannaserver hostname]\n" ) ;
    fflush( stderr ) ;
    exit( 0 ) ;
}

/* vim: set sw=4: */
