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
static char rcs_id[] = "$Id: rkc.c,v 1.12 2003/09/24 15:01:07 aida_s Exp $";
#endif

/*
 * MODIFICATION HISTORY
 *	S000	funahasi@oa2	Fri Oct  2 20:14:13 JST 1992
 *		- debug用関数 RkThrough()のために protocolを追加
 *	S001	funahasi@oa2	Tue Oct 13 15:40:08 JST 1992
 *		- version2.x以前のサーバに接続している時 RkQueryDic()が -1
 *		  を返すよう仕様を変更
 *	S002	funahasi@oa2	Thu Nov  5 13:22:49 JST 1992
 *		- RkQueryDic()の引き数に user名を指定出来るよう仕様を変更
 *		- fixed bug RkBgnBun()で旧サーバに接続している場合，modeから
 *		  コード変換部分だけを取り出す処理を修正
 *	S003	funahasi@oa2	Fri Nov 13 01:19:52 JST 1992
 *		- fixed bug version1.1のサーバに接続出来ない
 *		  rkc_get_yomi()を行っているのが原因
 *		- RkwSetAppName()が無かったので追加
 *		- fixed bug version2.1のサーバに接続出来ない
 *		  rkc_set_app_name()を行っているのが原因
 *	S004	funahasi@oa2	Fri Dec  4 02:04:38 JST 1992
 *		- cannastat, cshosのために rkcの内部関数を呼び出す
 *		  インタフェースを追加
 *	S005	funahasi@oa2	Thu Feb 18 12:18:41 JST 1993
 *		- fixed bug Rk[w]GetKanjiList()が仕様と異なっている．
 */

/* LINTLIBRARY */

#include    "sglobal.h" 
#include    "rkcw.h"
#include    "canna/RK.h"
#include    "rkc.h"
#include "conf.h"
#include "RKindep/ecfuncs.h"

#include    <sys/types.h>
#include    <pwd.h>
#include    <grp.h>
#include    <signal.h>
#include    <unistd.h>
	
/* CX:	コンテクストテーブル
 *	必要なレコードだけをmallocで作成する。
 *	^^^^^^^^^^^^^^^^^^
 */

/*********************************************************************
 *                      wchar_t replace begin                        *
 *********************************************************************/
#ifdef wchar_t
# error "wchar_t is already defined"
#endif
#define wchar_t cannawc

static RkcContext *RkcCX[MAX_CX] ;

#define RkcFree free
#define BUSY	1

#define AUTO_YOMI_SIZE	512

static short
rkc_call_flag = 0x00   ; /* RkInitializeが呼ばれてRkFinalizeが呼ばれるまで */
			 /* BUSYビットが立っている			   */

static short ProtocolMinor = 0 ;
static short ProtocolMajor = 0;
static int ServerFD = 0;					/* S004 */

extern struct rkcproto wideproto;
#ifdef USE_EUC_PROTOCOL
extern struct rkcproto eucproto;
static struct rkcproto *RKCP = &eucproto;
#else /* !USE_EUC_PROTOCOL */
static struct rkcproto *RKCP = &wideproto;
#endif /* !USE_EUC_PROTOCOL */

static short PROTOCOL = 0 ;

static char ConnectIrohaServerName[ MAX_HOSTNAME + 1 ];
static char *ServerNameSpecified;
static RkcConfigErrorProc config_error_handler;

/*
 * サポートするプロトコルのリスト
 * この順にプロトコルを使用し，サーバに接続しようとする
 */
static char *ProtoVerTbl[] = {
    W_VERSION,/* サーバと同じメジャー番号の時は、サーバの
                                    バージョンのプロトコルを使っている。*/
    "2.1",    /* サーバよりメジャー番号が小さい時は、
              小さい方のプロトコルを使う。(ここはマクロにした方がよいね)*/ 
#ifdef USE_EUC_PROTOCOL
    E_VERSION,	/* ver1.2 */
    "1.0",
#endif
    "",
};

static struct {
  Ushort cbuf[CBUFSIZE];
  char buffer[BUFSIZE];
  wchar_t wbuf[CBUFSIZE];
} rkc; /* general buffer。ただし、RkwXXX のなかだけで使うことにしよう。*/

/*
 * クライアント・コンテクストユーティリティ関数
 */


/*
 * 新しいコンテクストを作成する。
 */
static
RkcContext  *
newCC()
{
    register RkcContext *cx ;
    register int i ;

    for( i = 0; i < MAX_CX; i++) {
	if( !RkcCX[ i ] ) {
	    cx = (RkcContext *)malloc( sizeof( RkcContext ) ) ;
	    if( cx ) {
		cx->client = i ;
		cx->bun = (RkcBun *)NULL ;
		cx->Fkouho = (Ushort *)NULL ;
		cx->lastyomi = (Ushort *)NULL ;
		cx->curbun = cx->maxbun = cx->bgnflag = cx->maxyomi = 0 ;
		RkcCX[ i ] = cx ;
	    }
	    return( cx ) ;
	}
    }
    return( (RkcContext *)NULL ) ;
}

/*
 * 指定された文節から最終文節までの先頭候補または、候補列の領域を解放する
 */
static
void
freeBUN(cx, from)
register RkcContext *cx ;
register int	    from ;
{
    register RkcBun *bun ;

    for( ; from < cx->maxbun; from++ ) {
	bun = &cx->bun[ from ] ;
	if( bun->flags == NUMBER_KOUHO ) {
	    /* 第一候補しか入っていない文節は、実際にはmallocしたわけではなく
	     * サーバから通知された各文節の第一候補列の中へのポインタを
	     * 設定しているだけだからフリーしない。
	     */
	    RkcFree( (char *)bun->kanji ) ;
	    bun->kanji = (Ushort *)NULL ;
	    bun->curcand = bun->maxcand = 0 ;
	    bun->flags = NOTHING_KOUHO ;
	}
    }
}

/*
 * 指定されたコンテクストを解放する。
 */
static
void
freeCC( clientcx )
int clientcx ;
{
    register RkcContext     *cx ;

    if( (0 <= clientcx) && (clientcx < MAX_CX) ) {
	cx = RkcCX[ clientcx ] ;
	if( cx->bun ) {
	    freeBUN( cx, 0 ) ;
	    RkcFree( (char *)cx->bun ) ;
	    cx->bun = (RkcBun *)NULL ;
	}
	RkcFree( (char *)cx->Fkouho ) ;
	cx->Fkouho = (Ushort *)NULL ;
	RkcFree( (char *)cx->lastyomi );
	cx->lastyomi = (Ushort *)NULL;
	cx->curbun = cx->maxbun = 0 ;
	cx->bgnflag = 0 ;
	RkcFree( (char *)cx ) ;
	cx = (RkcContext *)NULL ;
	RkcCX[ clientcx ] = (RkcContext *)NULL ;
    }
}

/*
 * コンテクスト番号に対応したコンテクストを取得する。
 * (クライアント側)
 */
#define NOCHECK    0
#define CHECK	   1
static
RkcContext *
getCC( clientcx, type )
int	clientcx, type ;
{
    register RkcContext     *cx = (RkcContext *)NULL ;

    if( (0 <= clientcx) && (clientcx < MAX_CX) ) {
        cx = RkcCX[clientcx];
	if (cx)
	    if( (type == CHECK) && (cx->bgnflag != BUSY) )
		/* 変換中の時,maxbunは最低１である */
		cx = (RkcContext *)NULL ;
    }
    return( cx ) ;
}

int RkwSetUserInfo pro((char *, char *, char *));

static RkUserInfo *uinfo;

int
RkwSetUserInfo(user, group, topdir)
char *user, *group, *topdir;
{
  if (user && group && topdir) {
    uinfo = (RkUserInfo *)malloc(sizeof(RkUserInfo));
    if (uinfo) {
      uinfo->uname = user;
      uinfo->gname = group;
      uinfo->topdir = topdir;
      return 1;
    }
  }
  return 0;
}

static char *
FindLogname()
{
  if (uinfo)
    return uinfo->uname;
  else {
    char *username = NULL, *getenv(), *getlogin();
  
    struct passwd *pass = getpwuid(getuid());
    if( pass )
    username = pass->pw_name;
    if (username == NULL){
      if ( (username = getlogin()) == NULL ) {	
        if( (username = getenv( "LOGNAME" )) == NULL ) {
          username = getenv( "USER" );
        }
      }
    }
    return( username );
  }
  return (char *)NULL;
}

static char *
FindGroupname()
{
  if (uinfo)
    return uinfo->gname;
  else {
    struct group *gr = getgrgid(getgid()) ;
    if (gr && gr->gr_name) {
      return gr->gr_name;
    } 
    else{
      return (char *)NULL;
    }
  }
  return (char *)NULL;
}
/*
 *  RkwInitialize ()
 *
 *  Description:
 *  -----------
 *  かな漢字変換の初期化
 *
 *  Returns:
 *  -------
 *  0 or -1
 */
int
RkwInitialize( hostname ) /* とりあえずrkcの場合は、引き数を無視する */
char *hostname ;
{
    register int    i;
    register long  server ;
    register RkcContext *cx ;
    char *username, *data ;

    if( rkc_call_flag == BUSY )
		return( 0 );	

    rkc_configure();
    if (config_error_handler)
	(*config_error_handler)(RkcErrorBuf_get(&rkc_errors));
    if (ServerNameSpecified) {
        free(ServerNameSpecified);
	ServerNameSpecified = (char *)0;
    }
    ConnectIrohaServerName[0] = '\0';
    if( hostname && (unsigned)strlen(hostname) > 0 &&
#ifdef __EMX__
	    !_fnisabs( hostname ) &&
#else
	    hostname[0] != '/' &&
#endif
       (ServerNameSpecified = malloc(strlen(hostname) + 1))) {
        strcpy(ServerNameSpecified, hostname);
    }

    if( (ServerFD = rkc_Connect_Iroha_Server( ConnectIrohaServerName )) < 0 ) { /* S004 */
	errno = EPIPE ;
        goto init_err;
    }

    /* ユーザ名を取得する */
    username = FindLogname() ;

    if( !username || !(data = malloc( strlen(username) + strlen(W_VERSION)+2 )))
	    goto init_err;

    /* コンテクストを初期化する */
    for( i=0; i < MAX_CX; i++)
	RkcCX[ i ] = (RkcContext *)NULL ;

    /* コンテクストを作成する */
    if( (cx = newCC()) == (RkcContext *)NULL ) {
        RkcFree(data);
        goto init_err;
    }

    /* 最初はワイドキャラベースのプロトコルを使用する */
    for( i = 0; *ProtoVerTbl[i]; i++ ){
	strcpy( data, ProtoVerTbl[i] );
	strcat( data, ":" );
	strcat( data, username );
	ProtocolMajor = *ProtoVerTbl[i] - '0'; /* Majorが1桁たけ対応 */
        PROTOCOL = (ProtocolMajor > 1);

#ifdef USE_EUC_PROTOCOL
	RKCP = PROTOCOL > 0 ? &wideproto : &eucproto;
#endif

        /* サーバに初期化を要求し、サーバのコンテクストを取得する */
	if ((server = (*RKCP->initialize)( data )) < 0) {
	    /* 既にコンテクストを確保しているので、それを解放する */
	    if( (ServerFD = rkc_Connect_Iroha_Server( ConnectIrohaServerName )) < 0 ) { /* S004 */
		freeCC( cx->client ) ;
		RkcFree(data);
		errno = EPIPE ;
		goto init_err;
	    }
	    continue;
	}
	break;
    }
    RkcFree(data);

    if (!*ProtoVerTbl[i]) {
      freeCC(cx->client);
      errno = EPIPE;
      (void)close(ServerFD);
      goto init_err;
    }

    /* サーバのマイナーバージョンを得る */
    ProtocolMinor = 
      (short)((unsigned long)(server & 0xffff0000) >> (unsigned)0x10);

    /* サーバから取得したコンテクスト番号を入れる */
    cx->server = server & 0x0000ffff ;
    rkc_call_flag = BUSY ;

    /* プロトコルバージョンが 3.2 以上だったらグループ名を通知する */
    if (canna_version(ProtocolMajor, ProtocolMinor) > canna_version(3, 1)) {
      char *gname = FindGroupname();

      if (gname) {
	(*RKCP->notice_group_name)(cx, gname);
      }
    }
    return( cx->client ) ;
 init_err:
    rkc_config_fin();
    return -1;
}

/*
 *  RkwFinalize ()
 *
 *  Description:
 *  -----------
 *  かな漢字変換の終了
 */
void
RkwFinalize()
{
    register int i ;

    if( rkc_call_flag != BUSY )
		return;

    /* 全コンテクストを解放する
     *	    変換中のコンテクストはどうするのか ?
     */
    for( i = 0; i < MAX_CX; i++ ){
	if( RkcCX[ i ] ) {
	    freeCC( i ) ;
	}
    }	

    (*RKCP->finalize)();

    ProtocolMinor = 0 ;
    rkc_call_flag = 0 ;
    ProtocolMajor = 0;
    if (ServerNameSpecified) {
	free(ServerNameSpecified);
	ServerNameSpecified = (char *)0;
    }
    ConnectIrohaServerName[0] = '\0';

    if (uinfo) {
      free((char *)uinfo);
    }
    rkc_config_fin();
}

/*
 *  RkwCloseContext ()
 *
 *  Description:
 *  -----------
 *  コンテクストの開放
 *
 *  Input:
 *  -----
 *  cxnum
 *
 *  Returns:
 *  -------
 *  0 or -1
 */
int
RkwCloseContext(cxnum)
int cxnum ;
{
    register RkcContext *cx = getCC( cxnum, NOCHECK ) ;

    if( !cx || (rkc_call_flag != BUSY) )
	return( -1 ) ;

    if ((*RKCP->close_context)(cx) == -1)
	return( -1 ) ;

    freeCC( cxnum ) ;
    return( 0 );
}
/* 
 * KillServer 
 *
 * Version 3.3 以前のサーバには送らない 戻り値 OLDSRV = -110
 * Version 3.3 のサーバには送る 
 *
 */
int
RkwKillServer()
{
/*
  Protocol Version 3.3 からサポート。それ以前のサーバへは送ってはいけない。
*/

  if (canna_version(ProtocolMajor, ProtocolMinor) > canna_version(3, 2)) {
    return (*RKCP->killserver)();
  }
  return OLDSRV;

/* Protocol Version 3.3 */

}

/*
 *  RkwCreateContext ()
 *
 *  Description:
 *  -----------
 *  新しいコンテクストの作成
 *
 *  Returns:
 *  -------
 *  コンテクスト番号 or -1
 */
int
RkwCreateContext()
{
    register int    server ;
    register RkcContext *cx ;

    if( rkc_call_flag != BUSY )
	return( -1 ) ;

    /* コンテクストを作成する */
    if( (cx = newCC()) == (RkcContext *)NULL )
	return( -1 ) ;

    if ((server = (*RKCP->create_context)()) == -1) {
	/* 既にコンテクストを確保しているので、それを解放する */
	freeCC( cx->client ) ;
	return( -1 ) ;
    }

    cx->server = server ;
    return( cx->client ) ;
}

/*
 *  RkwCreateContext ()
 *
 *  Description:
 *  -----------
 *  新しいコンテクストの作成
 *
 *  Input:
 *  -----
 *  src_cx
 *
 *  Returns:
 *  -------
 *  コンテクスト番号 or -1
 */
int
RkwDuplicateContext( src_cx )
int src_cx ;
{
    register RkcContext *cx_dest, *cx_src = getCC( src_cx, NOCHECK ) ;
    register int	dest_cx ;

    if( !cx_src || (rkc_call_flag != BUSY) )
	return( -1 ) ;

    /* コンテクストを作成する */
    if( (cx_dest = newCC()) == (RkcContext *)NULL )
	return( -1 ) ;

    if ((dest_cx = (*RKCP->duplicate_context)(cx_src)) == -1) {
	/* 既にコンテクストを確保しているので、それを解放する */
	freeCC( cx_dest->client ) ;
	return( -1 ) ;
    }

    cx_dest->server = dest_cx ;
    return( cx_dest->client ) ;
}

/*
 *  RkwGetDicList ()
 *
 *  Description:
 *  -----------
 *  辞書リストに追加できる辞書名の取得
 *
 *  Input:
 *  -----
 *  cxnum
 *
 *  Returns:
 *  -------
 *  辞書名の個数 or -1
 */
int
RkwGetDicList(cxnum, dicnames, max)
int cxnum ;
char *dicnames ;
int max ;
{
    register RkcContext *cx = getCC( cxnum, NOCHECK ) ;

    if( !cx )
	return( -1 ) ;

    if( max <= 0 ) 
	return( 0 ) ;
    else if( !dicnames ) {
      return (*RKCP->dictionary_list)(cx, rkc.buffer, BUFSIZE);
    }
    return (*RKCP->dictionary_list)(cx, dicnames, max);
}

static int
_RkwDefineDic( cxnum, dicname, wordrec )	/* 単語登録 */
int cxnum ;
char *dicname ;
Ushort *wordrec ;
{
    register RkcContext *cx = getCC( cxnum, NOCHECK ) ;

    if( !cx )
	return( -1 ) ;

    return (*RKCP->define_dic)(cx, dicname, wordrec);
}

/*
 *  RkwDefineDic ()
 *
 *  Description:
 *  -----------
 *  単語登録
 *
 *  Input:
 *  -----
 *  cxnum
 *
 *  Returns:
 *  -------
 *  コンテクスト番号 or -1
 */
int
RkwDefineDic(cxnum, dicname, wordrec)
int cxnum;
char *dicname;
wchar_t *wordrec;
{
    if( !dicname || !wordrec )
	return( -1 ) ;

    wchar2ushort(wordrec, wcharstrlen(wordrec), rkc.cbuf, CBUFSIZE);
    return _RkwDefineDic(cxnum, dicname, rkc.cbuf);
}

static int
_RkwDeleteDic( cxnum, dicname, wordrec )	  /* 単語削除 */
int cxnum ;
char *dicname ;
Ushort *wordrec ;
{
    register RkcContext *cx = getCC( cxnum, NOCHECK ) ;

    if( !cx )
	return( -1 ) ;

    return (*RKCP->delete_dic)(cx, dicname, wordrec);
}

int
RkwDeleteDic(cxnum, dicname, wordrec)
int cxnum;
char *dicname;
wchar_t *wordrec;
{
  if( !dicname || !wordrec ) return -1;

  wchar2ushort(wordrec, wcharstrlen(wordrec), rkc.cbuf, CBUFSIZE);
  return _RkwDeleteDic(cxnum, dicname, rkc.cbuf);
}

int
RkwMountDic(cxnum, dicname, mode)
int cxnum, mode ;
char *dicname ;
{
    register RkcContext *cx = getCC( cxnum, NOCHECK ) ;

    if( !dicname || !cx )
	return( -1 ) ;

    return (*RKCP->mount_dictionary)(cx, dicname, mode);
}

int
RkwRemountDic(cxnum, dicname, where)
int cxnum, where  ;
char *dicname ;
{
    register RkcContext *cx = getCC( cxnum, NOCHECK ) ;

    if( !dicname || !cx )
	return( -1 ) ;

    return (*RKCP->remount_dictionary)(cx, dicname, where);
}

int
RkwUnmountDic(cxnum, dicname)
int cxnum ;
char *dicname ;
{
    register RkcContext *cx = getCC( cxnum, NOCHECK ) ;

    if( !dicname || !cx )
	return( -1 ) ;

    return (*RKCP->umount_dictionary)(cx, dicname);
}

int
RkwGetMountList(cxnum, dicnames_return, max)
int cxnum, max ;
char *dicnames_return ;
{
    register RkcContext *cx = getCC( cxnum, NOCHECK ) ;

    if( !cx )
	return( -1 ) ;

    if( !dicnames_return ) {
        return (*RKCP->mount_list)(cx, rkc.buffer, BUFSIZE);
    } else if( max <= 0 )
	return( 0 ) ;

    return (*RKCP->mount_list)(cx, dicnames_return, max);
}

int
RkwSetDicPath( cxnum, path ) /* サーチパスを設定 */
int cxnum ;
char *path ;
/* ARGSUSED */
{
    return( 0 ) ;
}

/*
 * StoreFirstKouho
 *	カレント文節から最終文節までの先頭候補を求め、格納する
 */
static void
StoreFirstKouho( cx, bun_max )
register RkcContext *cx ;
int bun_max ;
{
    /* ここは、よく考えないと構造まで変えた意味が無くなるし
     *	 後でえらい目に遭うかも知れない
     */
    register	int		i ;
    register	RkcBun		*bun ;
    register	Ushort	*kouhobuf ;

    /* カレント文節から最終文節までの候補を解放する */	
    freeBUN( cx, cx->curbun ) ; 

    /* ここには、rkc_*(rkcConvert.c)で第一候補列が格納されている */
    kouhobuf = cx->Fkouho ;

    /* ゼロ文節から最終文節までの第一候補のポインタを設定する */
    for( i = 0; i < bun_max; i++ ) {
	bun = &cx->bun[ i ] ;
	/* カレント文節までの文節で候補一覧を既に取得している文節は、
	 * ポインタの再設定はしない。
	 */
	if( bun->flags != NUMBER_KOUHO ) {
	    bun->kanji = kouhobuf ;
	    bun->curcand = 0 ;		/*  文節0文節1文節2文節3文節4@@ */
	    bun->maxcand = 1 ;		/*  ↑	 ↑   ↑   ↑	↑	*/
	    bun->flags = FIRST_KOUHO ;	/*	bun->kaji		*/
	}
	kouhobuf += ushortstrlen( (Ushort *)kouhobuf)+1 ;
    }
    cx->maxbun = bun_max ;
}

/*
 *    連文節変換開始
 */
static int	
_RkwBgnBun(cxnum, yomi, maxyomi, mode)
int cxnum, maxyomi, mode ; 
Ushort *yomi ;
{
    register RkcContext *cx = getCC( cxnum, NOCHECK ) ;
    int nbun, mask;					/* S002 */

    if( (maxyomi <= 0) || !cx )
	return( -1 ) ;

    /* RkBgnBunだけは、BUSYフラグが立っていると入ってはいけない */
    if( cx->bgnflag == BUSY )
	return( -1 ) ;

    /* 旧サーバに接続している場合，modeからコード変換部分だけを取り出す */
    if( ProtocolMajor < 3 ){
        int code;

	for( code = mode, mask = 0L; code ; code >>= RK_XFERBITS ){
	    if( (code & RK_XFERMASK) == RK_CTRLHENKAN ){
		break;
	    }
	    mask = (mask << RK_XFERBITS) | RK_XFERMASK;
	}
	mode &= mask;					/* S002 */
    }

    /* maxyomiの文節数だけアロケートしておく */
    /* これ以上の文節数は存在しないはず */

    cx->curbun = cx->maxbun = 0 ;
    if( yomi != NULL ){
	if( !(cx->bun = (RkcBun *)calloc( maxyomi, sizeof(RkcBun) )) )
	    return( -1 ) ;
	cx->lastyomi = (Ushort *)NULL;
	nbun = (*RKCP->convert)(cx, yomi, maxyomi, mode);
	if( nbun <= 0 ) {				
	    RkcFree( (char *)cx->bun ) ;
	    cx->bun = (RkcBun *)NULL ;
	    return( -1 ) ;
	}
	StoreFirstKouho( cx, nbun ) ;
    } else {
	if( !(cx->bun = (RkcBun *)calloc( AUTO_YOMI_SIZE, sizeof(RkcBun) )) )
	    return( -1 ) ;
	if( !(cx->lastyomi = (Ushort *)malloc( CBUFSIZE )) ) {
	    RkcFree( (char *)cx->bun );
	    cx->bun = (RkcBun *)NULL ;
	    return( -1 ) ;
	}

	nbun = (*RKCP->autoconv)(cx, maxyomi, mode);
	if( nbun < 0 ) {				
	    RkcFree( (char *)cx->bun ) ;
	    cx->bun = (RkcBun *)NULL ;
	    RkcFree( (char *)cx->lastyomi );
	    cx->lastyomi = (Ushort *)NULL;
	    return( -1 ) ;
	}
	*(cx->lastyomi) = ( Ushort )0;
    }
    cx->bgnflag = BUSY ;
    return( nbun ) ;
}

int
RkwBgnBun(cxnum, yomi, maxyomi, mode)
int cxnum;
wchar_t *yomi;
int maxyomi;
int mode;
{
    int len;

    if (yomi) {
      len = wchar2ushort(yomi, maxyomi, rkc.cbuf, CBUFSIZE);
      return _RkwBgnBun(cxnum, rkc.cbuf, len, mode);
    }
    else {  /* 自動変換開始 */
      return _RkwBgnBun(cxnum, (Ushort *)NULL, maxyomi, mode);
    }
}

int
RkwEndBun( cxnum, mode )
int cxnum, mode ;
{
    register RkcContext  *cx = getCC( cxnum, CHECK ) ;
    int ret ;

    if( cx ) {
	/*
	 * rkc_convert_endで学習すべき候補をサーバに知らせる
	 */
	if ((ret = (*RKCP->convert_end)(cx, mode )) >= 0) {
	    freeBUN( cx, 0 ) ;
	    RkcFree( (char *)cx->bun ) ;
	    RkcFree( (char *)cx->Fkouho ) ;
	    cx->bun = (RkcBun *)NULL ;
	    cx->Fkouho = (Ushort *)NULL ;
	    cx->curbun = cx->maxbun = 0 ;
	    cx->bgnflag = 0 ;

	    RkcFree( (char *)cx->lastyomi );
	    cx->lastyomi = (Ushort *)NULL;
	    cx->maxyomi = 0;
	}
	return( ret ) ;
    }

    return( 0 ) ;
}

/* LoadKouho
 *	必要に応じて全候補を読み出す
 */
static
int
LoadKouho( cx )
register RkcContext	 *cx ;
{
    register RkcBun	*bun = &cx->bun[ cx->curbun ] ;
    int 		ret ;

    if( bun->flags == FIRST_KOUHO ) {
	/*	候補を全て読み出す。
	 *	読み出しに失敗したら、先頭候補しかないふりをする
	 */
	if ((ret = (*RKCP->get_kanji_list)(cx)) >= 0) {
	    /* 読みだし成功 */
	    bun->curcand = 0 ;
	    bun->maxcand = ret ;
	} else if( errno == EPIPE )
	    return( -1 ) ;

	bun->flags = NUMBER_KOUHO ;
    }
    return( 0 ) ;
}

int
RkwXfer(cxnum, knum)
int cxnum, knum ;
{
    register RkcContext  *cx = getCC( cxnum, CHECK ) ;
    register RkcBun	 *bun ;

    if( cx ) {
	bun = &cx->bun[cx->curbun];
	if( LoadKouho( cx ) < 0 )
	    return( -1 ) ;
	if ( 0 <= knum && knum < bun->maxcand ) 
	    bun->curcand = knum;
	return( bun->curcand );
    }
    return( 0 );
}

int
RkwNfer(cxnum)
int cxnum ;
{
    register RkcContext  *cx = getCC( cxnum, CHECK ) ;
    register RkcBun	 *bun ;

    if( cx ) {
	bun = &cx->bun[ cx->curbun ];	
	if( LoadKouho( cx ) < 0 )
	    return( -1 ) ;
	bun->curcand = bun->maxcand - 1; /* 読みは、最後にある(0オリジン) */
	return( bun->curcand ) ;
    }
    return( 0 );
}

int
RkwNext(cxnum)
int cxnum ;
{
    register RkcContext  *cx = getCC( cxnum, CHECK ) ;
    register RkcBun	 *bun ;

    if( cx ) {
	bun = &cx->bun[ cx->curbun ];	
	if( LoadKouho( cx ) < 0 )
	    return( -1 ) ;
	if ( ++bun->curcand > bun->maxcand-1 )
	    bun->curcand = 0;
	return( bun->curcand ) ;
    }
    return( 0 ) ;
}

int
RkwPrev(cxnum)
int cxnum ;
{
    register RkcContext  *cx = getCC( cxnum, CHECK ) ;
    register RkcBun	 *bun ;

    if( cx ) {
	bun = &cx->bun[ cx->curbun ];
	if( LoadKouho( cx ) < 0 )
	    return( -1 ) ;
	if ( --bun->curcand < 0 )
	    bun->curcand = bun->maxcand-1 ; /* ０オリジン */
	return( bun->curcand ) ;
    }
    return( 0 );
}

static
Ushort *
SeekKouho( bun, to )
register RkcBun     *bun ;
register int	    to ;
{
    register int    i ;
    Ushort   *src_yomi ;

    src_yomi = bun->kanji ;
    for( i = 0; i < to; i++ )
	src_yomi += ushortstrlen( (Ushort *)src_yomi ) + 1 ;

    return( src_yomi ) ;
}

static int
_RkwGetKanji( cxnum, kanji, maxkanji )
int cxnum, maxkanji ;
Ushort *kanji ;
{
    RkcContext		    *cx = getCC( cxnum, CHECK ) ;
    RkcBun		    *bun ;
    register Ushort  *src_kouho ;

    if( cx ){
	bun = &cx->bun[ cx->curbun ] ;
			     /* 読みしかない場合は読みを返す */
	src_kouho = SeekKouho( bun, (bun->maxcand ? bun->curcand : 0) ) ;
	if( ushortstrlen( (Ushort *)src_kouho ) > maxkanji )
	    return( 0 ) ;
	ushortstrcpy( kanji, src_kouho );
	return( ushortstrlen( (Ushort *)src_kouho ) ) ;
    }
    return( -1 ) ;
}

int
RkwGetKanji(cxnum, kanji, maxkanji)
int cxnum;
wchar_t *kanji;
int maxkanji;
{
  int len;

    len = _RkwGetKanji(cxnum, rkc.cbuf, CBUFSIZE);
    if (len < 0) {
	return( len );
    }
    else {
	if( !kanji ) {
	  return ushort2wchar(rkc.cbuf, len, rkc.wbuf, CBUFSIZE);
	}
	else if (maxkanji <= 0) {
	    return( 0 );
	}
	else {
	  return ushort2wchar(rkc.cbuf, len, kanji, maxkanji);
	}
    }
}

static int
_RkwGetKanjiList(cxnum, kouho, max)
int cxnum, max ;
Ushort *kouho ;
{
    RkcContext	*cx = getCC( cxnum, CHECK ) ;
    RkcBun	*bun ;
    register Ushort  *dest_kouho, *src_kouho ;
    register int i, len ;
    int total ;

    if( cx ) {
	bun = &cx->bun[ cx->curbun ];
	if( LoadKouho( cx ) < 0 )
	    return( -1 ) ;
	if( !bun->kanji )
	    return( 0 ) ;
	if( !kouho )
	    return( bun->maxcand ? bun->maxcand : 1 ) ;
	/* 候補をコピーする */
	src_kouho = bun->kanji ;
	dest_kouho = kouho ;
	for( total = ushortstrlen( src_kouho ) + 1, i = 0;
	    (i < bun->maxcand) && (total < max) ; i++, total += len ) {
	    len = ushortstrcpy( dest_kouho, src_kouho ) + 1 ;
	    src_kouho += len ;
	    dest_kouho += len ;
	}
	*(dest_kouho++) = (Ushort)0 ;
	*(dest_kouho) = (Ushort)0 ;
	return( i ) ;
    }
    return( -1 ) ;
}

int
RkwGetKanjiList(cxnum, kanjis, maxkanjis)
int cxnum;
wchar_t *kanjis;
int maxkanjis;
{
  int nkanji, len, i, j = 0, k = 0;
  int retval;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
  Ushort cbuf[CBIGBUFSIZE];
#else
  Ushort *cbuf = GlobalAlloc(GMEM_FIXED, sizeof(Ushort) * CBIGBUFSIZE);
  if (!cbuf) {
    return 0;
  }
#endif

  if( !kanjis ) {
    retval = _RkwGetKanjiList(cxnum, (Ushort *)0, 0);
  }
  else if (maxkanjis <= 0) {
    retval = 0;
  }
  else {
  nkanji = _RkwGetKanjiList(cxnum, cbuf, CBIGBUFSIZE);

  for (i = 0 ; i < nkanji ; i++) {
    len = ushortstrlen(cbuf + j);
    if (k + len > maxkanjis - 2)				/* S005 */
      break;							/* S005 */
    k += ushort2wchar(cbuf + j, len, kanjis + k, maxkanjis);	/* S005 */
    kanjis[k++] = (wchar_t)0;
    j += len + 1;
  }
  kanjis[k] = (wchar_t)0;
  retval = i;
  }
#ifdef USE_MALLOC_FOR_BIG_ARRAY
  GlobalFree((HGLOBAL)cbuf);
#endif
  return retval;
}

int
RkwGoTo(cxnum, bnum)
int cxnum, bnum ;
{
    register RkcContext  *cx = getCC( cxnum, CHECK ) ;

    if( cx ){
	if ( 0 <= bnum && bnum < cx->maxbun )
	     cx->curbun = bnum;
	return(cx->curbun);
    }
    return( 0 );
}

int
RkwLeft(cxnum)
int cxnum;
{
    register RkcContext  *cx = getCC( cxnum, CHECK ) ;

    if( cx ){
	if ( --cx->curbun < 0 )
	     cx->curbun = cx->maxbun-1 ;
	return( cx->curbun );
    }
    return( 0 );
}

int
RkwRight(cxnum)
int cxnum;
{
    register RkcContext  *cx = getCC( cxnum, CHECK ) ;

    if( cx ){
	if ( ++cx->curbun > cx->maxbun-1 )
	     cx->curbun = 0;
	return( cx->curbun );
    }
    return( 0 );
}

#define ENLARGE     -1
#define SHORTEN     -2
#define MIN_YOMI     1

#ifdef __STDC__
int _RkwGetYomi(RkcContext *, Ushort *, int);
#else
int _RkwGetYomi();
#endif

static int
RKReSize( cxnum, len )
int cxnum, len ;
{
    register RkcContext  *cx = getCC( cxnum, CHECK ) ;
    int ret;		/* 総文節数 */

    if( cx ) {	
	/* 文節長が変更できるかどうかチェックする */
	register RkcBun *bun = &cx->bun[ cx->curbun ] ;

	/* カレント文節が候補列を既に読み込んでいる場合の処理 */
	if( bun->flags == NUMBER_KOUHO ) {
	     /* 文節縮めでカレント読みの長さがMIN_YOMIの場合は何もせずに */
	     /* カレント文節数を返す */
	    int yomilen = ushortstrlen((Ushort *)SeekKouho( bun,
				       (bun->maxcand ? (bun->maxcand-1) : 0)));

	    if( (len == SHORTEN) || (len == MIN_YOMI) ) {
		if( yomilen == MIN_YOMI )
		    return( cx->maxbun ) ;
	    } else {
		int curbun_save = cx->curbun ;
		int yomi_zan ;

		for( yomi_zan = 0; cx->curbun < cx->maxbun; cx->curbun++ ) {
		    int ylen, retval = 0;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
		    Ushort tmp_yomi[CBUFSIZE];
#else
		    Ushort *tmp_yomi = GlobalAlloc(GMEM_FIXED,
						   sizeof(Ushort) * CBUFSIZE);
		    if (!tmp_yomi) {
		      return -1;
		    }
#endif
		    ylen = _RkwGetYomi(cx, tmp_yomi, CBUFSIZE);
		    if (ylen < 0) {
		      retval = -1;
		    }
		    else {
		      yomi_zan += ylen ;
		    }
#ifdef USE_MALLOC_FOR_BIG_ARRAY
		    GlobalFree((HGLOBAL)tmp_yomi);
#endif
		    if (retval < 0) {
		      return retval;
		    }
		}
		cx->curbun = curbun_save ;
		yomi_zan += cx->maxyomi;
		if( ((len == ENLARGE) && (yomilen + 1 > yomi_zan))
						      || (yomi_zan < len) )
		    return( cx->maxbun ) ;
	    }
	}
	/* 〇文節から最終文節まで格納される */	
	if ((ret = (*RKCP->resize)(cx, len)) <= 0) {
	    return( -1 ) ;
	}
	StoreFirstKouho( cx, ret ) ;
	if( cx->lastyomi != (Ushort *)NULL ){
	    if ((len = (*RKCP->get_last_yomi)(cx, cx->lastyomi, CBUFSIZE))
		< 0) return -1;
	    cx->maxyomi = len;
	}
	return( ret ) ;
    }
    return( 0 ) ;
}

int
RkwResize( cxnum, len )
int cxnum, len ;
{
    if( len <= 0 ) {
	register RkcContext  *cx = getCC( cxnum, CHECK ) ;

	if( cx )
	    return( cx->maxbun ) ;
	else
	    return( 0 ) ;
    }

    return( RKReSize( cxnum, len ) ) ;
}

int
RkwEnlarge( cxnum )  /* 文節伸ばし */
int cxnum ;
{
    return( RKReSize( cxnum, ENLARGE  ) ) ;
}

int
RkwShorten(cxnum)    /* 文節縮め */
int cxnum ;
{
    return( RKReSize( cxnum, SHORTEN ) ) ;
}

static int
_RkwStoreYomi(cxnum, yomi, max)
int cxnum, max ;
Ushort *yomi ;
{
    register RkcContext  *cx = getCC( cxnum, CHECK ) ;
    int ret, len ;

    ret = 0 ;
    if( cx ) {
	if ((ret =  (*RKCP->store_yomi)(cx, yomi, max)) < 0) {
	    return( -1 ) ;
	}
	StoreFirstKouho( cx, ret ) ;
	if (!max && cx->curbun && cx->curbun == cx->maxbun)
	  cx->curbun--;
	if( cx->lastyomi != (Ushort *)NULL ){
	    if ((len = (*RKCP->get_last_yomi)(cx, cx->lastyomi, CBUFSIZE))
		< 0 ) return -1;
	    cx->maxyomi = len;
	}
    }
    return( ret ) ;
}

int
RkwStoreYomi(cxnum, yomi, maxyomi)
int cxnum;
wchar_t *yomi;
int maxyomi;
{
  int len;

  if (yomi && maxyomi >= 0) {
    len = RKI_MIN(wcharstrlen(yomi),maxyomi); 
    len = wchar2ushort(yomi, len, rkc.cbuf, CBUFSIZE) + 1;
  } else {
    rkc.cbuf[0] = 0;
    len = 0;
  }
  return _RkwStoreYomi(cxnum, rkc.cbuf, len);
}

int								/* S003 */
_RkwGetYomi(cx, yomi, maxyomi)
RkcContext *cx;
int maxyomi;
Ushort	*yomi;
{
    register RkcBun	*bun ;
    Ushort	*src_yomi;
    int len, retval = -1;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
    Ushort	tmp_yomi[CBUFSIZE];
#else
    Ushort *tmp_yomi = GlobalAlloc(GMEM_FIXED, sizeof(Ushort) * CBUFSIZE);
    if (!tmp_yomi) {
      return -1;
    }
#endif

    if( cx ){
	bun = &cx->bun[ cx->curbun ] ;		
	if( !PROTOCOL && (ProtocolMinor == 0) ) {
	    /* Ver 1.0 では，取りあえず候補一覧を取ってくる */
	    if( LoadKouho( cx ) < 0 )
	      retval = -1;
	}
	if( bun->flags == NUMBER_KOUHO ) {
	    src_yomi = SeekKouho( bun, (bun->maxcand ? (bun->maxcand-1) : 0) );
	} else {
	    if ((*RKCP->get_yomi)(cx, tmp_yomi) < 0)
		retval = -1;
	    src_yomi = tmp_yomi;
	}

	if( (len = ushortstrlen( (Ushort *)src_yomi )) > maxyomi )
	    retval = 0;

	bcopy( src_yomi, yomi, (len + 1) * sizeof( short ) );
	retval = len;
    }
#ifdef USE_MALLOC_FOR_BIG_ARRAY
    GlobalFree((HGLOBAL)tmp_yomi);
#endif
    return retval;
}

int
RkwGetYomi(cxnum, yomi, maxyomi)
int cxnum;
wchar_t *yomi;
int maxyomi;
{
  int len;

  if ((len = _RkwGetYomi(getCC( cxnum, CHECK ), rkc.cbuf, CBUFSIZE)) < 0) {
    return( len );
  }
  else {
    if( !yomi ) {
      return ushort2wchar(rkc.cbuf, len, rkc.wbuf, CBUFSIZE);
    }
    else if( maxyomi <= 0 )
      return( 0 );

    return ushort2wchar(rkc.cbuf, len, yomi, maxyomi);
  }
}

int
RkwGetLex(cxnum, lex, maxlex)
int cxnum, maxlex ;
RkLex *lex ;
{
    register RkcContext  *cx = getCC( cxnum, CHECK ) ;
    int ret = -1 ;

    if( cx ){
      if (lex == (RkLex *)NULL) {
#ifndef USE_MALLOC_FOR_BIG_ARRAY
	RkLex buf[1024];
#else
	RkLex *buf = (RkLex *)GlobalAlloc(GMEM_FIXED, sizeof(RkLex) * 1024);
	if (!buf) {
	  return 0;
	}
#endif
	ret = (*RKCP->get_lex)(cx, 1024, buf); 
#ifdef USE_MALLOC_FOR_BIG_ARRAY
	GlobalFree((HGLOBAL)buf);
#endif
      }
      else if( maxlex <= 0 )
	return( 0 );

      ret = (*RKCP->get_lex)(cx, maxlex, lex);
    }
    return( ret ) ;
}

int
RkwGetStat(cxnum, stat)
int cxnum ;
RkStat *stat ;
{
    register RkcContext  *cx = getCC( cxnum, CHECK ) ;
    int ret ;

    if( cx && stat ){
	ret = (*RKCP->get_stat)(cx, stat);
	if( !PROTOCOL && (ProtocolMinor == 0) ) {
	    int p[ 7 ] ;
	    register int tmp1, tmp2 ;
	    int i ;

	    bcopy( stat, p, sizeof( RkStat ) ) ;
	    tmp1 = p[ 5 ];
	    tmp2 = p[ 6 ] ;
	    for( i = 4; i > 1; i-- )
		p[ i + 2 ] = p[ i ] ;
	    p[ 2 ] = tmp1 ;
	    p[ 3 ] = tmp2 ;
	    bcopy( p, stat, sizeof( RkStat ) ) ;
	}
    } else {
	ret = -1 ;
    }

    return( ret ) ;
}

char *
RkwGetServerName()
{
  if (ConnectIrohaServerName[0]) {
    return( ConnectIrohaServerName ) ;
  }
  else {
    return ServerNameSpecified;
  }
}

int
RkwGetProtocolVersion(majorp, minorp)
int *majorp, *minorp;
{
    *majorp = ProtocolMajor;
    *minorp = ProtocolMinor;
    return 0;
}

exp(int)
RkwGetServerVersion(majorp, minorp)
int *majorp, *minorp;
{
    if( !PROTOCOL )
	return( RkwGetProtocolVersion(majorp, minorp) );

    return( rkcw_get_server_info(majorp, minorp) );
}

							/* begin:S004 */
int
RkcGetServerFD()
{
    return( ServerFD );
}

int
G070_RkcGetServerFD()
{
    return( ServerFD );
}

int
RkcConnectIrohaServer( servername )
char* servername;
{
    /* XXX:
     * RkcDisconnectIrohaServerに相当するインターフェースが無いので、
     * これはメモリリークを引き起こす。今のところこのAPIは古いcannastatが
     * ver 1.xのサーバと通信する場合にだけ使うので、この問題は無視する
     * ことにする。
     */
    rkc_configure();
    return( rkc_Connect_Iroha_Server( servername ) );
}
							/* end:S004 */
int
G069_RkcConnectIrohaServer( servername )
char* servername;
{
    return RkcConnectIrohaServer(servername);
}

void
RkcListenConfigErrors( handler )
RkcConfigErrorProc handler;
{
    config_error_handler = handler;
}

#ifdef EXTENSION
static
CheckRemoteToolProtoVersion(mode)
{
  if (!PROTOCOL && ProtocolMinor < 2) /* protocol version 1.2 */
    return -1;
  else if (canna_version(ProtocolMajor, ProtocolMinor) < canna_version(3, 1) &&
	   (mode & (RK_GRP_DIC | RK_SYS_DIC | RK_GRP_DIR | RK_SYS_DIR))) {
    return -1;
  }
  return 0;
}

int
RkwListDic( cxnum, dirname, dicnames_return, size )
int cxnum, size ;
char *dirname, *dicnames_return ;
{
    register RkcContext *cx = getCC( cxnum, NOCHECK ) ;

    if (!cx) {
      return -1;
    }
    if (CheckRemoteToolProtoVersion(0)) {
      return ACCES;
    }

    if( !dirname )
	if( !(dirname = FindLogname()) )
	    return( -1 ) ;

    if( !dicnames_return ) {
#ifndef USE_MALLOC_FOR_BIG_ARRAY
	char buffer[ MAXDATA ] ;

	return (*RKCP->list_dictionary)(cx, dirname, buffer, MAXDATA);
#else
	char *buffer = GlobalAlloc(GMEM_FIXED, MAXDATA);
	if (buffer) {
	  int retval =
	    (*RKCP->list_dictionary)(cx, dirname, buffer, MAXDATA);
	  GlobalFree((HGLOBAL)buffer);
	  return retval;
	}
	else {
	  return 0;
	}
#endif
    }
    else if( size <= 0 )
	return( 0 ) ;

    return (*RKCP->list_dictionary)(cx, dirname, dicnames_return, size);
}

int
RkwCreateDic( cxnum, dicname, mode )
int cxnum, mode ;
char *dicname ;
{
    register RkcContext *cx = getCC( cxnum, NOCHECK ) ;

    if (!cx || !dicname) {
      return -1;
    }
    if (CheckRemoteToolProtoVersion(mode)) {
      return ACCES;
    }

    return (*RKCP->create_dictionary)(cx, (char *)dicname, mode);
}

exp(int)
RkwRemoveDic( cxnum, dicname, mode )
int cxnum ;
char *dicname ;
{
    register RkcContext *cx = getCC( cxnum, NOCHECK ) ;

    if (!cx || !dicname) {
      return -1;
    }
    if (CheckRemoteToolProtoVersion(mode)) {
      return ACCES;
    }

    return (*RKCP->remove_dictionary)(cx, dicname, mode);
}

exp(int)
RkwRenameDic( cxnum, dicname, newdicname, mode )
int cxnum, mode ;
char *dicname, *newdicname;
{
    register RkcContext *cx = getCC( cxnum, NOCHECK ) ;

    if (!cx || !dicname || !newdicname) {
      return -1;
    }
    if (CheckRemoteToolProtoVersion(mode)) {
      return ACCES;
    }

    return (*RKCP->rename_dictionary)(cx, dicname, newdicname, mode);
}

/* CopyDic を 入れる。 */

exp(int)
RkwCopyDic(cxnum, dirname, dicname, newdicname, mode )
int cxnum, mode ;
char *dirname,*dicname, *newdicname ;
{
    register RkcContext *cx = getCC( cxnum, NOCHECK ) ;

    if (!cx || !dirname || !dicname) {
      return -1;
    }
/*
  Protocol Version 3.2 からサポート。それ以前のサーバへは送ってはいけない。
*/

  if (canna_version(ProtocolMajor, ProtocolMinor) > canna_version(3, 1)) {
    return (*RKCP->copy_dictionary)(cx, dirname, dicname, newdicname, mode);
  }
  return -1;

/* Protocol Version 3.2 */

}

/* ここまで */

static
_RkwGetWordTextDic( cxnum, dirname, dicname, info, infolen )
int cxnum, infolen ;
unsigned char *dirname, *dicname;
Ushort *info;
{
    register RkcContext *cx = getCC( cxnum, NOCHECK ) ;

    if (!cx || !dirname || !dicname) {
      return -1;
    }

    if (CheckRemoteToolProtoVersion(0)) {
      return ACCES;
    }

    return (*RKCP->get_text_dictionary)
      (cx, (char *)dirname, (char *)dicname, info, infolen);
}

exp(int)
RkwGetWordTextDic(cxnum, dirname, dicname, info, infolen)
int cxnum, infolen ;
unsigned char *dirname, *dicname;
wchar_t *info;
{
  int len;

  len = _RkwGetWordTextDic(cxnum, dirname, dicname, rkc.cbuf, CBUFSIZE);
  if (len < 0){
    return len;
  }
  else {
    if( !info ) {
      return ushort2wchar(rkc.cbuf, len, rkc.buffer, BUFSIZE);
    }
    else if( infolen <= 0 )
      return 0;

    return ushort2wchar(rkc.cbuf, len, info, infolen);
  }
}

#else
RkwListDic(){}
RkwCreateDic(){}
RkwRemoveDic(){}
RkwRenameDic(){}
RkwGetWordTextDic(){}
#endif /* EXTENSION */

/* 逐次自動変換機能関数				*/
/*						*/
/* 逐次自動変換機能導入によって追加される関数	*/

static int
_RkwSubstYomi( cxnum, ys, ye, yomi, nyomi )
int cxnum, ys, ye, nyomi;
Ushort	*yomi;
{
    register RkcContext *cx = getCC( cxnum, CHECK );
    int len, curbun, nbun = -1, pbun, retval = -1;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
    Ushort cbuf[CBUFSIZE];
#else
    Ushort *cbuf = GlobalAlloc(GMEM_FIXED, sizeof(Ushort) * CBUFSIZE);
    if (!cbuf) {
      return -1;
    }
#endif

    if( cx ){
	len = cx->maxyomi;
	if( ys < 0 || ye < 0 || ye < ys || len < ys || len < ye ) {
	  retval = -1;
	  goto done;
	}

	nyomi = RKI_MIN( ushortstrlen( yomi ), nyomi);
	curbun = cx->curbun;
	cx->curbun = 0;
	if ((nbun = (*RKCP->subst_yomi)(cx, cx->maxbun, ys, ye, yomi, nyomi))
	    < 0) {
	    cx->curbun = curbun;
	    retval = -1;
	    goto done;
	}

	pbun = cx->maxbun;
	cx->maxbun = 0; /* StoreFirstKouho であんまりものを捨てないように */
	StoreFirstKouho(cx, nbun);

	if (nbun != pbun) {
	  len = (*RKCP->get_last_yomi)(cx, cx->lastyomi, CBUFSIZE);
	  if (len < 0) {
	    retval = -1;
	    goto done;
	  }
	} else {
	    len = ys;
	    ushortstrncpy( cbuf, &(cx->lastyomi[ye]), (cx->maxyomi - ye) );
	    len += ushortstrcpy( &(cx->lastyomi[ys]), yomi );
	    len += ushortstrcpy( &(cx->lastyomi[ys + nyomi]), cbuf );
	}
	cx->maxyomi = len;
    }
    retval = nbun;
  done:
#ifdef USE_MALLOC_FOR_BIG_ARRAY
    GlobalFree((HGLOBAL)cbuf);
#endif
    return retval;
}

int
RkwSubstYomi( cxnum, ys, ye, yomi, nyomi )
int cxnum, ys, ye, nyomi;
wchar_t *yomi;
{
  RkcContext *cx = getCC( cxnum, CHECK );
  int len;

  if(cx) {
    len = wchar2ushort(yomi, nyomi, rkc.cbuf, CBUFSIZE);
    return _RkwSubstYomi(cxnum, ys, ye, rkc.cbuf, len);
  }
  return( -1 ) ;
}

int
RkwFlushYomi( cxnum )
int cxnum;
{
    RkcContext *cx = getCC( cxnum, CHECK );
    int curbun, nbun = -1;

    if( cx ){
	curbun = cx->curbun;
	cx->curbun = 0;
	if ((nbun = (*RKCP->flush_yomi)(cx)) < 0) {
	    cx->curbun = curbun;
	    return( -1 ) ;
	}

	if( nbun != cx->maxbun ) {
	    cx->curbun = cx->maxbun;
	    StoreFirstKouho( cx, nbun );
	    cx->curbun = curbun;
	}

	*(cx->lastyomi) = (Ushort)0;
	cx->maxyomi = 0;
    }
    return( nbun ) ;
}

static int
_RkwGetLastYomi( cxnum, yomi, maxyomi )
int	cxnum, maxyomi;
Ushort	*yomi;
{
    register RkcContext *cx = getCC( cxnum, CHECK );
    int	len = -1;

    if( cx ){
	if( !maxyomi || cx->maxyomi > maxyomi )
	    return( 0 );
	len = ushortstrncpy( yomi, cx->lastyomi, cx->maxyomi );
    }
    return( len ) ;
}

int
RkwGetLastYomi( cxnum, yomi, maxyomi )
int cxnum;
wchar_t *yomi;
int maxyomi;
{
  int len;

  len = _RkwGetLastYomi(cxnum, rkc.cbuf, CBUFSIZE);
  if (len < 0) {
    return -1;
  }
  else {
    if( !yomi ) {
      return ushort2wchar(rkc.cbuf, len, rkc.wbuf, CBUFSIZE);
    }
    else if( maxyomi <= 0 )
      return 0;

    return ushort2wchar(rkc.cbuf, len, yomi, maxyomi);
  }
}

/*
 * 先頭文節から指定された文節までの先頭候補または、候補列の領域を解放する
 */
static
void
removeBUN( cx, to )
RkcContext *cx;
register int to;
{
    register RkcBun *bun ;
    register i;

    for( i = 0; i < to; i++ ) {
	bun = &cx->bun[ i ] ;
	if( bun->flags == NUMBER_KOUHO ) {
	    /* 第一候補しか入っていない文節は、実際にはmallocしたわけではなく
	     * サーバから通知された各文節の第一候補列の中へのポインタを
	     * 設定しているだけだからフリーしない。
	     */
	    RkcFree( (char *)bun->kanji ) ;
	    bun->kanji = (Ushort *)NULL ;
	    bun->curcand = bun->maxcand = 0 ;
	    bun->flags = NOTHING_KOUHO ;
	}
    }
}

int
RkwRemoveBun( cx_num, mode )
int cx_num, mode;
{
    register RkcContext  *cx = getCC( cx_num, CHECK );
    register int cnt, i;
    int ret;

    if( cx ) {
	/*
	 * rkcw_remove_bun で学習すべき候補をサーバに知らせる
	 */
	if ((ret = (*RKCP->remove_bun)(cx, mode)) < 0)
	    return( -1 );

	removeBUN( cx, cx->curbun + 1 );
	for( cnt = 0, i = cx->curbun + 1; i < cx->maxbun; cnt++, i++ ) {
	    cx->bun[cnt].kanji = cx->bun[i].kanji;
	    cx->bun[cnt].curcand = cx->bun[i].curcand;
	    cx->bun[cnt].maxcand = cx->bun[i].maxcand;
	    cx->bun[cnt].flags = cx->bun[i].flags;
	    cx->bun[i].kanji = (Ushort *)NULL ;
	    cx->bun[i].curcand = cx->bun[i].maxcand = 0 ;
	    cx->bun[i].flags = NOTHING_KOUHO ;
	}
	cx->curbun = cx->maxbun = 0;
	StoreFirstKouho( cx, ret ) ;
	return( ret ) ;
    }

    return( 0 ) ;
}

static int
_RkwGetSimpleKanji(cxnum, dicname, yomi, maxyomi, kanjis, maxkanjis,
		   hinshis, maxhinshis)
unsigned char *dicname;
int cxnum, maxyomi, maxkanjis, maxhinshis;
Ushort *yomi, *kanjis, *hinshis;
{
    RkcContext *cx = getCC( cxnum, CHECK ) ;

    if( cx ){
	return (*RKCP->get_simple_kanji)
	  (cx, (char *)dicname, yomi, maxyomi, kanjis,
	   maxkanjis, hinshis, maxhinshis);
    }
    return( -1 ) ;
}

int
RkwGetSimpleKanji( cxnum, dicname, yomi, maxyomi, kanjis, maxkanjis, hinshis, maxhinshis )
int cxnum, maxyomi, maxkanjis, maxhinshis ;
wchar_t *yomi, *kanjis, *hinshis;
char *dicname;
{
  Ushort cbuf[CBUFSIZE], cbuf2[CBIGBUFSIZE], cbuf3[CBIGBUFSIZE];
  int nkanji, len, i, j = 0, k = 0, l = 0, m = 0;

  if( !dicname || !yomi || maxyomi <= 0 )
    return( -1 );

  len = wchar2ushort(yomi, maxyomi, cbuf, CBUFSIZE);
  nkanji = _RkwGetSimpleKanji(cxnum, dicname, cbuf, len,
			  cbuf2, CBIGBUFSIZE, cbuf3, CBIGBUFSIZE );

  if( nkanji <= 0 || !kanjis || !hinshis )
    return( nkanji );
  if( maxkanjis <= 0 || maxhinshis <= 0 )
    return( 0 );

  for( i = 0 ; i < nkanji ; i++ ) {
    k += ushort2wchar(cbuf2 + j, ushortstrlen(cbuf2 + j),
		   kanjis + k, maxkanjis - k) + 1;
    j += ushortstrlen(cbuf2 + j) + 1;
    l += ushort2euc(cbuf3 + m, ushortstrlen(cbuf3 + m),
		   hinshis + l, maxhinshis - l) + 1;
    m += ushortstrlen(cbuf3 + m) + 1;
  }
  kanjis[k] = hinshis[l] = (wchar_t)0;
  return ( nkanji );
}

int
RkwQueryDic( cxnum, username, dicname, status )			/* S002 */
int cxnum;
char *username;					/* S002 */
char *dicname;
struct DicInfo *status;
{
    RkcContext *cx = getCC( cxnum, NOCHECK );

    if( !cx || !dicname || ProtocolMajor < 3 )
	return( -1 ) ;

    if( !username )
	if( !(username = FindLogname()) )
	    return( -1 ) ;

    if( !status ) {
      struct DicInfo buffer; /* ダミー */

      return (*RKCP->query_dic)(cx, username, dicname, &buffer);
    }
    else{
	return (*RKCP->query_dic)(cx, username, dicname, status);
    }
}

static int
_RkwGetHinshi( cxnum, dst, maxdst )
int cxnum, maxdst;
Ushort *dst;
{
    RkcContext *cx = getCC( cxnum, CHECK );

    if( cx ){
	return (*RKCP->get_hinshi)(cx, dst, maxdst);
    }
    return( -1 ) ;
}

int
RkwGetHinshi( cxnum, dst, maxdst )
int cxnum, maxdst;
wchar_t *dst;
{
  int len;

  len = _RkwGetHinshi(cxnum, rkc.cbuf, CBUFSIZE);
  if (len < 0) {
    return -1;
  }
  else {
    if( !dst ) {
      return ushort2wchar(rkc.cbuf, len, rkc.wbuf, CBUFSIZE);
    }
    else if( maxdst <= 0 )
      return 0;

    return ushort2wchar(rkc.cbuf, len, dst, maxdst);
  }
}

static int
_RkwStoreRange( cxnum, yomi, maxyomi )
int cxnum, maxyomi;
Ushort *yomi;
{
    RkcContext *cx = getCC( cxnum, CHECK );

    if( cx ){
	return (*RKCP->store_range)(cx, yomi, maxyomi);
    }
    return( -1 ) ;
}

int
RkwStoreRange( cxnum, yomi, maxyomi )
int cxnum, maxyomi;
wchar_t *yomi;
{
  int len;

  if( !yomi || maxyomi <= 0 )
    return -1;

  len = wchar2ushort(yomi, maxyomi, rkc.cbuf, CBUFSIZE);
  return _RkwStoreRange(cxnum, rkc.cbuf, len);
}

int
RkwSetLocale( cxnum, locale )
int cxnum;
unsigned char *locale;
{
    RkcContext *cx = getCC( cxnum, NOCHECK );

    if( cx ){
	return (*RKCP->set_locale)(cx, (char *)locale);
    }
    return( -1 ) ;
}

/*
 *  RkwSync ()
 *
 *  Description:
 *  -----------
 *  辞書ファイルへの同期処理
 *
 *  Input:
 *  -----
 *  dicname: 辞書名@辞書名@...@@
 *
 *  Returns:
 *  -------
 *  0 or -1
 */

int
RkwSync( cxnum, dicname )
int cxnum;
char *dicname;
{
  RkcContext *cx = getCC( cxnum, NOCHECK );

  if (cx) {
    if (canna_version(ProtocolMajor, ProtocolMinor) > canna_version(3, 1)) {
      return (*RKCP->sync)(cx, dicname ? dicname : "");
    }
  }
  return -1;
}

/*
 *  RkwSetAppName ()
 *
 *  Description:
 *  -----------
 *  アプリケーション名の登録
 *
 *  Input:
 *  -----
 *  apname: アプリケーション名
 *
 *  Returns:
 *  -------
 *  0 or -1
 */

int RkwSetAppName pro((int, char *));

int
RkwSetAppName( cxnum, apname )
int cxnum;
char *apname;
{
    RkcContext *cx = getCC( cxnum, NOCHECK );

    if( cx && (ProtocolMajor > 2) && apname){
	return (*RKCP->set_app_name)(cx, apname);
    }
    return( -1 ) ;
}

int RkSetAppName pro((int, char *));

/*
 *  RkwChmodDic ()
 *
 *  Description:
 *  -----------
 *  辞書のアクセス権の変更
 *
 *  Input:
 *  -----
 *  dicname: 辞書の名前
 *
 *  mode: モード
 *
 *  Returns:
 *  -------
 *  0 or -1
 */
exp(int)
RkwChmodDic(cxnum, dicname, mode)
int cxnum;
char *dicname;
int  mode;
{
  RkcContext *cx = getCC( cxnum, NOCHECK );

  if (cx &&
      canna_version(ProtocolMajor, ProtocolMinor) > canna_version(3, 1)) {
    return (*RKCP->chmod_dic)(cx, dicname, mode);
  }
  return -1;
}

/* EUC functions */

#ifndef OMIT_EUC_FUNCS

int
RkInitialize( hostname )
char *hostname ;
{
    return( RkwInitialize( hostname ) ) ;
}

void
RkFinalize()
{
    RkwFinalize() ;
}

RkKillServer()
{
    return RkwKillServer();
}

int
RkCloseContext( cxnum )
int cxnum ;
{
    return( RkwCloseContext( cxnum ) ) ;
}

int
RkCreateContext()
{
    return( RkwCreateContext() ) ;
}

int
RkDuplicateContext( src_cx )
int src_cx ;
{
    return( RkwDuplicateContext( src_cx ) ) ;
}

int
RkGetDicList(cxnum, dicnames, max)
int cxnum ;
char *dicnames ;
int max ;
{
    return( RkwGetDicList( cxnum, dicnames, max ) ) ;
}

int
RkDefineDic(cxnum, dicname, wordrec)
int cxnum;
char *dicname, *wordrec;
{
    if( !dicname || !wordrec )
	return( -1 ) ;

    euc2ushort(wordrec, strlen(wordrec), rkc.cbuf, CBUFSIZE);
    return _RkwDefineDic(cxnum, dicname, rkc.cbuf);
}

int
RkDeleteDic(cxnum, dicname, wordrec)
int cxnum;
char *dicname, *wordrec;
{
    Ushort cbuf[CBUFSIZE];

    if( !dicname || !wordrec )
	return( -1 ) ;

    euc2ushort(wordrec, strlen(wordrec), cbuf, CBUFSIZE);
    return( _RkwDeleteDic(cxnum, dicname, cbuf) );
}

int
RkMountDic( cxnum, dicname, mode )
int cxnum, mode;
char *dicname;
{
    return( RkwMountDic( cxnum, dicname, mode ) );
}

int
RkRemountDic( cxnum, dicname, where )
int cxnum, where;
char *dicname;
{
    return( RkwRemountDic( cxnum, dicname, where ) );
}

int
RkUnmountDic( cxnum, dicname )
int cxnum;
char *dicname;
{
    return( RkwUnmountDic( cxnum, dicname ) );
}

int
RkGetMountList( cxnum, dicnames_return, max )
int cxnum, max;
char *dicnames_return;
{
    return( RkwGetMountList( cxnum, dicnames_return, max ) );
}

int
RkSetDicPath( cxnum, path ) /* サーチパスを設定 */
int cxnum ;
char *path ;
/* ARGSUSED */
{
    /*
     * サーバが立ち上がるときに設定するのでRKCでは何もせずに返す
     */
    return( 0 ) ;
}

int
RkGetDirList( cxnum, ddname, maxddname )   /* 辞書リストを取得 */
int cxnum ;
char *ddname ;
int maxddname ;
/* ARGSUSED */
{
#ifdef USE_EUC_PROTOCOL
    register RkcContext *cx = getCC( cxnum, NOCHECK ) ;

    if( !cx || ProtocolMajor > 1 )
	return( -1 ) ;

    if( !ddname ) {
	char buffer[BUFSIZE] ;

	return (rkc_get_dir_list(cx, buffer, BUFSIZE));
    } else if( maxddname <= 0 )
	return( 0 ) ;

    return( rkc_get_dir_list( cx, ddname, maxddname ) ) ;
#else /* !USE_EUC_PROTOCOL */
    return -1;
#endif /* !USE_EUC_PROTOCOL */
}

int
RkBgnBun(cxnum, yomi, maxyomi, mode)
int cxnum;
char *yomi;
int maxyomi;
int mode;
{
    Ushort cbuf[CBIGBUFSIZE];
    int len;

    if (yomi) {
      len = euc2ushort(yomi, maxyomi, cbuf, CBIGBUFSIZE);
      return( _RkwBgnBun(cxnum, cbuf, len, mode) );
    }
    else {  /* 自動変換開始 */
      return _RkwBgnBun(cxnum, (Ushort *)NULL, maxyomi, mode);
    }
}

int
RkEndBun( cxnum, mode )
int cxnum, mode ;
{
    return( RkwEndBun( cxnum, mode ) );
}

int
RkXfer( cxnum, knum )
int cxnum, knum;
{
    return( RkwXfer( cxnum, knum ) );
}

int
RkNfer( cxnum )
int cxnum ;
{
    return( RkwNfer( cxnum ) );
}

int
RkNext( cxnum )
int cxnum ;
{
    return( RkwNext( cxnum ) );
}

int
RkPrev(cxnum)
int cxnum ;
{
    return( RkwPrev( cxnum ) );
}

int
RkGetKanji(cxnum, kanji, maxkanji)
int cxnum;
unsigned char *kanji;
int maxkanji;
{
    Ushort cbuf[CBUFSIZE];
    int len;

    if( (len = _RkwGetKanji(cxnum, cbuf, CBUFSIZE)) < 0 ){
	return( len );
    }
    else {
	if( !kanji ) {
	    char buffer[CBUFSIZE];

	    return( ushort2euc(cbuf, len, buffer, CBUFSIZE) );
	}
	else if( maxkanji <= 0 )
	    return( 0 );

	return ushort2euc(cbuf, len, (char *)kanji, maxkanji);
    }
}

int
RkGetKanjiList(cxnum, kanjis, maxkanjis)
int cxnum;
unsigned char *kanjis;
int maxkanjis;
{
  Ushort cbuf[CBIGBUFSIZE];
  int nkanji, len, i, j = 0, k = 0;
  char eucbuf[CBUFSIZE*2];				/* S005 */
  int euclen;							/* S005 */

  if( !kanjis ) {
    return( _RkwGetKanjiList(cxnum, (Ushort *)0, 0) );
  }
  else if( maxkanjis <= 0 )
    return( 0 );

  nkanji = _RkwGetKanjiList(cxnum, cbuf, CBIGBUFSIZE);

  for (i = 0 ; i < nkanji ; i++) {
    len = ushortstrlen(cbuf + j);
    euclen = ushort2euc(cbuf + j, len, eucbuf, CBUFSIZE * 2);	/* S005 */
    if (k + euclen > maxkanjis - 2)				/* S005 */
      break;							/* S005 */
    strcpy((char *)kanjis + k, (char *)eucbuf);			/* S005 */
    k += euclen + 1;						/* S005 */
    j += len + 1;
  }
  kanjis[k] = (unsigned char)0;
  return( i );							/* S005 */
}

int
RkGoTo(cxnum, bnum)
int cxnum, bnum ;
{
    return( RkwGoTo( cxnum, bnum ) );
}

int
RkLeft( cxnum )
int cxnum;
{
    return( RkwLeft( cxnum ) );
}

int
RkRight( cxnum )
int cxnum;
{
    return( RkwRight( cxnum ) );
}

int
RkResize(cxnum, len)
int cxnum, len ;
{
    /* この len はバイトやけど，RkwResize には文字数を渡さなあかんのと
       ちゃうやろか? こんなん，どないやって変換したらええんやろ? */
    Ushort cbuf[CBUFSIZE];
    char tmpbuf[BUFSIZE];
    register int euclen, uslen = 0;
    int curbun, ret;
    register RkcContext  *cx = getCC( cxnum, CHECK ) ;

    if( cx ) {
	if( len <= 0 )
	    return( cx->maxbun );

	/* しゃあないから，読みを取ってきて文字数を調べたろ */
	curbun = cx->curbun;
	for( ; (cx->curbun < cx->maxbun) && len; (cx->curbun)++ ) {
	    /* こんなんしとったらめちゃくちゃ効率悪いがな...とほほ */
	    if( (ret = _RkwGetYomi( cx, cbuf, CBUFSIZE )) < 0 ) {
		cx->curbun = curbun;
		return( -1 ) ;
	    }
	    if( (euclen = ushort2euc(cbuf, ret, tmpbuf, BUFSIZE)) > len ) {
		uslen += euc2ushort(tmpbuf, len, cbuf, CBUFSIZE);
		break;
	    } else {
		uslen += ret;
		len -= euclen;
	    }
	}
	cx->curbun = curbun;
    } else {
	if( len <= 0 )
	    return( 0 );
    }
    return( RKReSize( cxnum, uslen ) );
}

int
RkEnlarge( cxnum )  /* 文節伸ばし */
int cxnum ;
{
    return( RKReSize( cxnum, ENLARGE  ) ) ;
}

int
RkShorten(cxnum)    /* 文節縮め */
int cxnum ;
{
    return( RKReSize( cxnum, SHORTEN ) ) ;
}

int
RkStoreYomi(cxnum, yomi, maxyomi)
int cxnum;
char *yomi;
int maxyomi;
{
  Ushort cbuf[CBUFSIZE];
  int len;

  if (yomi && maxyomi >= 0) {
    len = RKI_MIN((int)strlen(yomi),maxyomi); 
    len = euc2ushort(yomi, len, cbuf, CBUFSIZE) + 1;
  } else {
    cbuf[0] = 0;
    len = 0;
  }
  return( _RkwStoreYomi(cxnum, cbuf, len) );
}

int
RkGetYomi(cxnum, yomi, maxyomi)
int cxnum;
unsigned char *yomi;
int maxyomi;
{
  Ushort cbuf[CBUFSIZE];
  int len;

  if ((len = _RkwGetYomi(getCC( cxnum, CHECK ), cbuf, CBUFSIZE)) < 0) {
    return( len );
  }
  else {
    if( !yomi ) {
      char buffer[CBUFSIZE];

      return( ushort2euc(cbuf, len, buffer, CBUFSIZE) );
    }
    else if( maxyomi <= 0 )
      return( 0 );

    return ushort2euc(cbuf, len, (char *)yomi, maxyomi);
  }
}

int
RkGetLex(cxnum, lex, maxlex)
int cxnum, maxlex ;
RkLex *lex ;
{
    RkLex *tango;
    Ushort ybuf[CBUFSIZE], kbuf[CBUFSIZE];
    char cbuf[BUFSIZE];
    int i, len, ylen, klen, ret;

    if( (ret = RkwGetLex( cxnum, lex, maxlex )) <= 0 )
	return( ret );
    if (lex != (RkLex *)NULL) {
      len = _RkwGetYomi(getCC( cxnum, CHECK ), ybuf, CBUFSIZE);
      len = _RkwGetKanji(cxnum, kbuf, CBUFSIZE);
      ylen = klen = 0;
      tango = lex;
      for( i = 0; i < ret; i++ ){
	len = tango->ylen;
	tango->ylen = ushort2euc(ybuf + ylen, len, cbuf, BUFSIZE);
	ylen += len;
	len = tango->klen;
	tango->klen = ushort2euc(kbuf + klen, len, cbuf, BUFSIZE);
	klen += len;
	tango++;
      }
    }
    return( ret );
}

int
RkGetStat(cxnum, stat)
int cxnum ;
RkStat *stat ;
{
    unsigned char cbuf[BUFSIZE];
    int ret;

    if( (ret = RkwGetStat( cxnum, stat )) < 0 )
	return( ret );
    stat->ylen = RkGetYomi(cxnum, cbuf, BUFSIZE);
    stat->klen = RkGetKanji(cxnum, cbuf, BUFSIZE);
    return( ret );
}

#ifdef EXTENSION
RkListDic( cxnum, dirname, dicnames_return, size )
int cxnum, size;
unsigned char *dirname, *dicnames_return;
{
  return RkwListDic(cxnum, (char *)dirname,(char *) dicnames_return, size);
}

exp(int)
RkCreateDic( cxnum, dicname, mode )
int cxnum, mode;
unsigned char *dicname;
{
  return RkwCreateDic(cxnum, (char *)dicname, mode);
}

RkRemoveDic( cxnum, dicname, mode )
int cxnum;
unsigned char *dicname;
{
  return RkwRemoveDic(cxnum, (char *)dicname, mode);
}

RkRenameDic( cxnum, dicname, newdicname, mode )
int cxnum, mode;
unsigned char *dicname, *newdicname;
{
  return RkwRenameDic(cxnum, (char *)dicname, (char *)newdicname, mode);
}

RkCopyDic(cxnum, dirname, dicname, newdicname, mode)
int cxnum, mode;
unsigned char *dirname, *dicname, *newdicname;
{
  return RkwCopyDic(cxnum, (char *)dirname, (char *)dicname,
		    (char *)newdicname, mode);
}

exp(int)
RkGetWordTextDic(cxnum, dirname, dicname, info, infolen)
int cxnum, infolen ;
unsigned char *dirname, *dicname, *info;
{
  Ushort cbuf[CBUFSIZE];
  int len;

  if ((len = _RkwGetWordTextDic(cxnum, dirname, dicname, cbuf, CBUFSIZE)) < 0){
    return len;
  }
  else {
    if( !info ) {
      char buffer[CBUFSIZE];

      return( ushort2euc(cbuf, len, buffer, CBUFSIZE) );
    }
    else if( infolen <= 0 )
      return 0;

    return ushort2euc(cbuf, len, (char *)info, infolen);
  }
}

#else
RkListDic(){}
RkCreateDic(){}
RkRemoveDic(){}
RkRenameDic(){}
RkGetWordTextDic(){}
#endif /* EXTENSION */

int
RkSubstYomi( cxnum, ys, ye, yomi, nyomi )
int cxnum, ys, ye, nyomi;
char *yomi;
{
  RkcContext *cx = getCC( cxnum, CHECK );
  char cbuf[CBUFSIZE];
  Ushort cbuf2[CBUFSIZE];
  int len;

  if (cx) {
    ushort2euc(cx->lastyomi, cx->maxyomi, cbuf, CBUFSIZE);
    ys = euc2ushort(cbuf, ys, cbuf2, CBUFSIZE);
    ye = euc2ushort(cbuf, ye, cbuf2, CBUFSIZE);
    len = euc2ushort(yomi, nyomi, cbuf2, CBUFSIZE);
    return( _RkwSubstYomi(cxnum, ys, ye, cbuf2, len) );
  }
  return( -1 ) ;
}

int
RkFlushYomi( cxnum )
int cxnum;
{
    return( RkwFlushYomi( cxnum ) );
}

int
RkGetLastYomi( cxnum, yomi, maxyomi )
int cxnum;
char *yomi;
int maxyomi;
{
  Ushort cbuf[CBUFSIZE];
  int len;

  if( (len = _RkwGetLastYomi(cxnum, cbuf, CBUFSIZE)) < 0 ){
    return -1;
  }
  else {
    if( !yomi ) {
      char buffer[CBUFSIZE];

      return( ushort2euc(cbuf, len, buffer, CBUFSIZE) );
    }
    else if( maxyomi <= 0 )
      return 0;

    return ushort2euc(cbuf, len, yomi, maxyomi);
  }
}

int
RkRemoveBun( cxnum, mode )
int cxnum, mode ;
{
    return( RkwRemoveBun( cxnum, mode ) );
}

int
RkGetSimpleKanji( cxnum, dicname, yomi, maxyomi, kanjis, maxkanjis, hinshis, maxhinshis )
int cxnum, maxyomi, maxkanjis, maxhinshis ;
unsigned char *dicname, *yomi, *kanjis, *hinshis ;
{
  Ushort cbuf[CBUFSIZE], cbuf2[CBIGBUFSIZE], cbuf3[CBIGBUFSIZE];
  int nkanji, len, i, j = 0, k = 0, l = 0, m = 0;

  if( !dicname || !yomi || maxyomi <= 0 )
    return( -1 );

  len = euc2ushort((char *)yomi, maxyomi, cbuf, CBUFSIZE);
  nkanji = _RkwGetSimpleKanji(cxnum, dicname, cbuf, len,
			  cbuf2, CBIGBUFSIZE, cbuf3, CBIGBUFSIZE );

  if( nkanji <= 0 || !kanjis || !hinshis )
    return( nkanji );
  if( maxkanjis <= 0 || maxhinshis <= 0 )
    return( 0 );

  for( i = 0 ; i < nkanji ; i++ ) {
    k += ushort2euc(cbuf2 + j, ushortstrlen(cbuf2 + j),
		    (char *)kanjis + k, maxkanjis - k) + 1;
    j += ushortstrlen(cbuf2 + j) + 1;
    l += ushort2euc(cbuf3 + m, ushortstrlen(cbuf3 + m),
		    (char *)hinshis + l, maxhinshis - l) + 1;
    m += ushortstrlen(cbuf3 + m) + 1;
  }
  kanjis[k] = hinshis[l] = (unsigned char)0;
  return ( nkanji );
}

int
RkQueryDic( cxnum, username, dicname, status )			/* S002 */
int cxnum;
char *username;					/* S002 */
char *dicname;
struct DicInfo *status;
{
  return RkwQueryDic(cxnum, username, dicname, status);
}

int
RkGetHinshi( cxnum, dst, maxdst )
int cxnum, maxdst;
unsigned char *dst;
{
  Ushort cbuf[CBUFSIZE];
  int len;

  if( (len = _RkwGetHinshi( cxnum, cbuf, CBUFSIZE )) < 0 ){
    return -1;
  }
  else {
    if( !dst ) {
      char buffer[CBUFSIZE];

      return( ushort2euc(cbuf, len, buffer, CBUFSIZE) );
    }
    else if( maxdst <= 0 )
      return 0;

    return ushort2euc(cbuf, len, (char *)dst, maxdst);
  }
}

int
RkStoreRange( cxnum, yomi, maxyomi )
int cxnum, maxyomi;
unsigned char *yomi;
{
  Ushort cbuf[CBUFSIZE];
  int len;

  if( !yomi || maxyomi <= 0 )
    return -1;

  len = euc2ushort((char *)yomi, maxyomi, cbuf, CBUFSIZE);
  return( _RkwStoreRange( cxnum, cbuf, len ) );
}

int
RkSetLocale( cxnum, locale )
int cxnum;
unsigned char *locale;
{
    return( RkwSetLocale( cxnum, locale ) );
}

int
RkSync( cxnum, dicname )
int cxnum;
char *dicname;
{
    return( RkwSync( cxnum, dicname ) );
}

int
RkSetAppName( cxnum, apname )
int cxnum;
char *apname;
{
    return( RkwSetAppName( cxnum, apname ) );
}

int
RkChmodDic(cxnum, dicname, mode)
int cxnum;
unsigned char *dicname;
int mode;
{
  return RkwChmodDic(cxnum, (char *)dicname, mode);
}
#endif /* OMIT_EUC_FUNCS */

#ifdef ENGINE_SWITCH
exp(struct rkfuncs) RkFuncs = {
  RkwGetProtocolVersion,
  RkwGetServerName,
  RkwGetServerVersion,
  RkwInitialize,
  RkwFinalize,
  RkwCreateContext,
  RkwDuplicateContext,
  RkwCloseContext,
  RkwSetDicPath,
  RkwCreateDic,
  RkwSync,
  RkwGetDicList,
  RkwGetMountList,
  RkwMountDic,
  RkwRemountDic,
  RkwUnmountDic,
  RkwDefineDic,
  RkwDeleteDic,
  RkwGetHinshi,
  RkwGetKanji,
  RkwGetYomi,
  RkwGetLex,
  RkwGetStat,
  RkwGetKanjiList,
  RkwFlushYomi,
  RkwGetLastYomi,
  RkwRemoveBun,
  RkwSubstYomi,
  RkwBgnBun,
  RkwEndBun,
  RkwGoTo,
  RkwLeft,
  RkwRight,
  RkwNext,
  RkwPrev,
  RkwNfer,
  RkwXfer,
  RkwResize,
  RkwEnlarge,
  RkwShorten,
  RkwStoreYomi,
  RkwSetAppName,
  RkwSetUserInfo,
  RkwQueryDic,
  RkwCopyDic,
  RkwListDic,
  RkwRemoveDic,
  RkwRenameDic,
  RkwChmodDic,
  RkwGetWordTextDic,
  RkwGetSimpleKanji,
};
#endif /* ENGINE_SWITCH */
/*
 *  RkThrough ()
 *
 *  Description:
 *  -----------
 *  commandで示される機能を実行する
 *
 *  Input:
 *  -----
 *  command: コマンド番号
 *  buf: データ/結果の先頭ポインタ
 *  content_size: データのサイズ
 *  buffer_size: 結果領域のサイズ
 *
 *  Returns:
 *  -------
 *  bufに格納された大きさ or -1
 */
int
RkThrough( cxnum, command, buf, content_size, buffer_size )
int cxnum;
int command;
unsigned char *buf;
int content_size;
int buffer_size;
{
    RkcContext *cx = getCC( cxnum, NOCHECK );

    if( cx ){
	return (*RKCP->through)
	  (cx, command, (char *)buf, content_size, buffer_size);
    }
    return( -1 ) ;
}							/* S000:end */

#ifndef wchar_t
# error "wchar_t is already undefined"
#endif
#undef wchar_t
/*********************************************************************
 *                       wchar_t replace end                         *
 *********************************************************************/
