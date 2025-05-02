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
static	char	rcs_id[] = "@(#) 102.1 $Id: ichiran.c,v 7.18 1996/12/02 02:25:22 kon Exp $";
#endif /* lint */

#include	<errno.h>
#include	"canna.h"

#ifdef luna88k
extern int errno;
#endif

extern int TanNextKouho();

static void clearIchiranContext();
static int IchiranKakutei();
static void getIchiranPreviousKouhoretsu();
static void getIchiranNextKouhoretsu();


#define ICHISIZE 9
static char *sbango = 
  "\243\261\241\241\243\262\241\241\243\263\241\241\243\264\241\241\243\265"
  "\241\241\243\266\241\241\243\267\241\241\243\270\241\241\243\271\241\241"
  "\243\341\241\241\243\342\241\241\243\343\241\241\243\344\241\241\243\345"
  "\241\241\243\346";
     /* １　２　３　４　５　６　７　８　９　ａ　ｂ　ｃ　ｄ　ｅ　ｆ */
                                                /* 候補行の候補番号の文字列 */
static wchar_t *bango;

/*  "1.","　2.","　3.","　4.","　5.","　6.","　7.","　8.","　9.",*/
static char  *sbango2[] = {
  "1","\241\2412","\241\2413","\241\2414","\241\2415",
  "\241\2416","\241\2417","\241\2418","\241\2419",
  };

static wchar_t *bango2[ICHISIZE];

static char *skuuhaku = "\241\241"; 
			/* 　 */
static wchar_t *kuuhaku;

initIchiran()
{
  int i, retval = 0;
  char buf[16];

  retval = setWStrings(&bango, &sbango, 1);
  if (retval != NG) {
    for(i = 0; i < ICHISIZE; i++) {

      /* セパレーターの処理 */
      if (cannaconf.indexSeparator &&
	  0x20 <= cannaconf.indexSeparator &&
	  0x80 > cannaconf.indexSeparator)
        sprintf(buf, "%s%c", sbango2[i], (char)cannaconf.indexSeparator);
      else
        sprintf(buf, "%s%c", sbango2[i], (char)DEFAULTINDEXSEPARATOR);
      
      bango2[i] = WString(buf);
    }

    retval = setWStrings(&kuuhaku, &skuuhaku, 1);
  }
  return retval;
}


/*
 * 一覧行表示中のカレント文節の候補を更新する
 *
 * ・カレント候補を変える。
 * ・これにともない kugiri も更新される
 *
 * 引き数	uiContext
 *              yomiContext
 */
static void
makeIchiranEchoStrCurChange(yc)
yomiContext yc;
{
  RkwXfer(yc->context, yc->curIkouho);
}

/*
 * かな漢字変換用の構造体の内容を更新する(その場のみ)
 *
 * ・一覧を呼び出す前の状態についての表示文字列を作る
 *
 * 引き数	uiContext
 *              yomiContext
 */
static void
makeIchiranKanjiStatusReturn(d, env, yc)
uiContext	d;
mode_context env;
yomiContext yc;
{
  mode_context sv;

  sv = d->modec;
  d->modec = env;
  makeKanjiStatusReturn(d, yc);
  d->modec = sv;
}

#define DEC_COLUMNS(n) ((n) < 10 ? 1 : (n) < 100 ? 2 : (n) < 1000 ? 3 : 4)

/*
 * 候補行に関する構造体の内容を更新する
 *
 * ・glineinfo と kouhoinfo から候補行を作成し、カレント候補番号を反転する
 *
 * 引き数	uiContext
 * 戻り値	なし
 */
void
makeGlineStatus(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;
  wchar_t *p;
  char str[16];
  int i, cur;

  if (cannaconf.kCount) {
    cur = *(ic->curIkouho) + 1;
  }

  d->kanji_status_return->info |= KanjiGLineInfo;
  d->kanji_status_return->gline.line =
    ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].gldata;
  d->kanji_status_return->gline.length = 
    ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].gllen;
  d->kanji_status_return->gline.revPos = 
    ic->kouhoifp[*(ic->curIkouho)].khpoint;
  if (cannaconf.ReverseWord && ic->inhibit & NUMBERING) {
    p = ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].gldata +
      ic->kouhoifp[*(ic->curIkouho)].khpoint;
    for (i = 0;
         *p != *kuuhaku && *p != ((wchar_t)' ') && *p != ((wchar_t)0)
	 && i < ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].gllen;
	 i++) {
      p++;
    }
    d->kanji_status_return->gline.revLen = i;
  } else
    d->kanji_status_return->gline.revLen = 1;

  if (cannaconf.kCount && d->kanji_status_return->gline.length) {
    register int a = ic->nIkouho, b = DEC_COLUMNS(cur) + DEC_COLUMNS(a) + 2;
    sprintf(str, " %d/%d", cur, a);
    MBstowcs(d->kanji_status_return->gline.line + 
	     d->kanji_status_return->gline.length - b, str, b + 1);
    /* 以下はいらないのでは？ */
    d->kanji_status_return->gline.length
      = WStrlen(d->kanji_status_return->gline.line);
  }
}

static ichiranEveryTimeCatch pro((uiContext, int, mode_context));

static
ichiranEveryTimeCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
{
  yomiContext yc;

  yc = (yomiContext)env;

  makeIchiranEchoStrCurChange(yc);
  makeIchiranKanjiStatusReturn(d, env, yc);

  return(retval);
}

static ichiranExitCatch pro((uiContext, int, mode_context));

static
ichiranExitCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
{
  yomiContext yc;

  yc = (yomiContext)env;
  yc->kouhoCount = 0;
  /* d->curIkouhoをカレント候補とする */
  if ((retval = RkwXfer(yc->context, yc->curIkouho)) == NG) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
    jrKanjiError = "\245\253\245\354\245\363\245\310\270\365\312\344\244\362"
	"\274\350\244\352\275\320\244\273\244\336\244\273\244\363\244\307"
	"\244\267\244\277";             
      /* カレント候補を取り出せませんでした */
    /* カレント候補が取り出せないくらいでは何ともないぞ */
  }
  else {
    retval = d->nbytes = 0;
  }

  makeIchiranEchoStrCurChange(yc);
  makeIchiranKanjiStatusReturn(d, env, yc);

  freeGetIchiranList(yc->allkouho);
  
  popCallback(d);

  if (!cannaconf.stayAfterValidate && !d->more.todo) {
    d->more.todo = 1;
    d->more.ch = 0;
    d->more.fnum = CANNA_FN_Forward;
  }
  currentModeInfo(d);

  return(retval);
}

static ichiranQuitCatch pro((uiContext, int, mode_context));

static
ichiranQuitCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
{
  yomiContext yc;

  yc = (yomiContext)env;
  yc->kouhoCount = 0;

  if ((retval = RkwXfer(yc->context, yc->curIkouho)) == NG) {
    if(errno == EPIPE) {
      jrKanjiPipeError();
    }
    jrKanjiError = "\245\253\245\354\245\363\245\310\270\365\312\344\244\362"
	"\274\350\244\352\275\320\244\273\244\336\244\273\244\363\244\307"
	"\244\267\244\277";               
           /* カレント候補を取り出せませんでした */
    /* カレント候補が取り出せないくらいでは何ともないぞ */
  }
  else {
    retval = d->nbytes = 0;
  }

  makeIchiranEchoStrCurChange(yc);
  makeIchiranKanjiStatusReturn(d, env, yc);

  freeGetIchiranList(yc->allkouho);

  popCallback(d);
  currentModeInfo(d);
  return(retval);
}

void
freeIchiranBuf(ic)
ichiranContext ic;
{
  if(ic->glinebufp)
    free(ic->glinebufp);
  if(ic->kouhoifp)
    free(ic->kouhoifp);
  if(ic->glineifp)
    free(ic->glineifp);
}

void
freeGetIchiranList(buf)
wchar_t **buf;
{
  /* 候補一覧表示行用のエリアをフリーする */
  if(buf) {
    if(*buf) {
      free(*buf);
    }
    free(buf);
  }
}

static void
popIchiranMode(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;

  d->modec = ic->next;
  d->current_mode = ic->prevMode;
  freeIchiranContext(ic);
}

/*
 * すべての候補を取り出して、配列にする
 */

static int makeKouhoIchiran();

wchar_t **
getIchiranList(context, nelem, currentkouho)
int context;
int *nelem, *currentkouho;
{
  wchar_t *work, *wptr, **bptr, **buf;
  RkStat st;
  int i;

  /* RkwGetKanjiList で得る、すべての候補のための領域を得る */
  if ((work = (wchar_t *)malloc(ROMEBUFSIZE * sizeof(wchar_t)))
                                               == (wchar_t *)NULL) {
#ifndef WIN
    jrKanjiError = "malloc (getIchiranList) できませんでした";
#else
    jrKanjiError = "malloc (getIchiranList) \244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277";
#endif
    return (wchar_t **)NULL;
  }

  /* すべての候補を得る。
     例: けいかん → 警官@景観@掛冠@@ (@はNULL) */
  if((*nelem = RkwGetKanjiList(context, work, ROMEBUFSIZE)) < 0) {
    jrKanjiError = "\244\271\244\331\244\306\244\316\270\365\312\344\244\316"
	"\274\350\244\352\275\320\244\267\244\313\274\272\307\324\244\267"
	"\244\336\244\267\244\277";
                   /* すべての候補の取り出しに失敗しました */
    free(work);
    return (wchar_t **)NULL;
  }

#ifdef	INHIBIT_DUPLICATION
  if (*nelem == 3) {
    wchar_t *w1, *w2, *w3;

    w1 = work;
    w2 = w1 + WStrlen(w1);
    w3 = w2 + WStrlen(w2);
    if (!WStrcmp(w1, ++w3)) {
      if (!WStrcmp(w1, ++w2))
	*nelem = 1;
      else
	*nelem = 2;
    }
  }
#endif	/* INHIBIT_DUPLICATION */

  /* makeKouhoIchiran()に渡すデータ */
  if((buf = (wchar_t **)calloc
      (*nelem + 1, sizeof(wchar_t *))) == (wchar_t **)NULL) {
    jrKanjiError = "malloc (getIchiranList) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
                                            /* できませんでした */
    free(work);
    return (wchar_t **)NULL;
  }
  for(wptr = work, bptr = buf, i = 0; *wptr && i++ < *nelem; bptr++) {
    *bptr = wptr;
    while(*wptr++)
      /* EMPTY */
      ;
  }
  *bptr = (wchar_t *)0;

  if(RkwGetStat(context, &st) == -1) {
    jrKanjiError = "\245\271\245\306\245\244\245\277\245\271\244\362\274\350"
	"\244\352\275\320\244\273\244\336\244\273\244\363\244\307\244\267"
	"\244\277";
                   /* ステイタスを取り出せませんでした */
    free(work);
    free(buf);
    return (wchar_t **)NULL;
  }
  *currentkouho = st.candnum; /* カレント候補は何番目？ */

  return(buf);
}

/* cfunc ichiranContext
 *
 * ichiranContext 候補一覧用の構造体を作り初期化する
 *
 */
ichiranContext
newIchiranContext()
{
  ichiranContext icxt;

  if ((icxt = (ichiranContext)malloc(sizeof(ichiranContextRec)))
                                          == (ichiranContext)NULL) {
#ifndef WIN
    jrKanjiError = "malloc (newIchiranContext) できませんでした";
#else
    jrKanjiError = "malloc (newIchiranContext) \244\307\244\255\244\336"
	"\244\273\244\363\244\307\244\267\244\277";
#endif
    return (ichiranContext)NULL;
  }
  clearIchiranContext(icxt);

  return icxt;
}

/*
 * 候補一覧行を作る
 */

#ifdef __STDC__
int
selectOne(uiContext d, wchar_t **buf, int *ck, int nelem, int bangomax,
	  unsigned inhibit, int currentkouho, int allowcallback,
	  canna_callback_t everyTimeCallback, canna_callback_t exitCallback,
	  canna_callback_t quitCallback, canna_callback_t auxCallback)
#else
selectOne(d, buf, ck, nelem, bangomax, inhibit, currentkouho, allowcallback,
	  everyTimeCallback, exitCallback, quitCallback, auxCallback)
uiContext d;
wchar_t **buf;
int *ck;
int nelem, bangomax;
unsigned inhibit;
int currentkouho;
int allowcallback;
canna_callback_t everyTimeCallback, exitCallback, quitCallback, auxCallback;
#endif
{
  extern KanjiModeRec ichiran_mode;
  ichiranContext ic;

  if (allowcallback != WITHOUT_LIST_CALLBACK &&
      d->list_func == (int (*) pro((char *, int, wchar_t **, int, int *)))0) {
    allowcallback = WITHOUT_LIST_CALLBACK;
  }

  if(pushCallback(d, d->modec,
	everyTimeCallback, exitCallback, quitCallback, auxCallback) == 0) {
    jrKanjiError = "malloc (pushCallback) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
                                          /* できませんでした */
    return(NG);
  }
  
  if((ic = newIchiranContext()) == (ichiranContext)NULL) {
    popCallback(d);
    return(NG);
  }
  ic->majorMode = d->majorMode;
  ic->next = d->modec;
  d->modec = (mode_context)ic;

  ic->prevMode = d->current_mode;
  d->current_mode = &ichiran_mode;
  d->flags &= ~(PLEASE_CLEAR_GLINE | PCG_RECOGNIZED);
  /* ここにくる直前に C-t とかが Gline に表示されている場合上の１行を
     やる必要が出てくる。 */

  ic->allkouho = buf;
  ic->curIkouho = ck;
  ic->inhibit = inhibit;
  ic->nIkouho = nelem;

  if (allowcallback != WITHOUT_LIST_CALLBACK) {
    ic->flags |= ICHIRAN_ALLOW_CALLBACK;
    /* listcallback では番号はつけない */
    ic->inhibit |= NUMBERING;
  }

  if (allowcallback == WITHOUT_LIST_CALLBACK) {
    if (makeKouhoIchiran(d, nelem, bangomax, inhibit, currentkouho)   == NG) {
      popIchiranMode(d);
      popCallback(d);
      return(NG);
    }
  }
  else {
    if (cannaconf.kCount) {
      *(ic->curIkouho) += currentkouho;
      if (*(ic->curIkouho) >= ic->nIkouho)
        ic->svIkouho = *(ic->curIkouho) = 0;
    }
    d->list_func(d->client_data, CANNA_LIST_Start, buf, nelem, ic->curIkouho);
  }

  return(0);
}

/*
 * IchiranContext の初期化
 */
static void
clearIchiranContext(p)
ichiranContext p;
{
  p->id = ICHIRAN_CONTEXT;
  p->svIkouho = 0;
  p->curIkouho = 0;
  p->nIkouho = 0;
  p->tooSmall = 0;
  p->curIchar = 0;
  p->allkouho = 0;
  p->glinebufp = 0;
  p->kouhoifp = (kouhoinfo *)0;
  p->glineifp = (glineinfo *)0;
  p->flags = (unsigned char)0;
}
  
/*
 * 候補一覧のデータ構造体を作るための領域を確保する
 */
allocIchiranBuf(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;
  int size;

  /* サイズの分と番号の分の領域を得る*/
  size = ic->nIkouho * (d->ncolumns + 1) * WCHARSIZE; /* えいやっ */
  if((ic->glinebufp = (wchar_t *)malloc(size)) ==  (wchar_t *)NULL) {
    jrKanjiError = "malloc (allocIchiranBuf) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
                                             /* できませんでした */
    return(NG);
  }

  /* kouhoinfoの領域を得る */
  size = (ic->nIkouho + 1) * sizeof(kouhoinfo);
  if((ic->kouhoifp = (kouhoinfo *)malloc(size)) == (kouhoinfo *)NULL) {
    jrKanjiError = "malloc (allocIchiranBuf) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
                                             /* できませんでした */
    free(ic->glinebufp);
    return(NG);
  }

  /* glineinfoの領域を得る */
  size = (ic->nIkouho + 1) * sizeof(glineinfo);
  if((ic->glineifp = (glineinfo *)malloc(size)) == (glineinfo *)NULL) {
    jrKanjiError = "malloc (allocIchiranBuf) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
                                             /* できませんでした */
    free(ic->glinebufp);
    free(ic->kouhoifp);
    return(NG);
  }
  return(0);
}

/*
 * 候補一覧行を表示用のデータをテーブルに作成する
 *
 * ・glineinfo と kouhoinfoを作成する
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
static int
makeKouhoIchiran(d, nelem, bangomax, inhibit, currentkouho)
uiContext d;
int nelem, bangomax;
unsigned char inhibit;
int currentkouho;
{
  ichiranContext ic = (ichiranContext)d->modec;
  wchar_t **kkptr, *kptr, *gptr, *svgptr;
  int           ko, lnko, cn = 0, svcn, line = 0, dn = 0, svdn;
  int netwidth;

  netwidth = d->ncolumns -
    (cannaconf.kCount ? (DEC_COLUMNS(nelem) * 2 + 2/* 2は/とSPの分 */) : 0);

  ic->nIkouho = nelem;	/* 候補の数 */

  /* カレント候補をセットする */
  ic->svIkouho = *(ic->curIkouho);
  *(ic->curIkouho) += currentkouho;
  if(*(ic->curIkouho) >= ic->nIkouho)
    ic->svIkouho = *(ic->curIkouho) = 0;

  if(allocIchiranBuf(d) == NG)
    return(NG);

  if(d->ncolumns < 1) {
    ic->tooSmall = 1;
    return(0);
  }

  /* glineinfoとkouhoinfoを作る */
  /* 
   ＊glineinfo＊
      int glkosu   : int glhead     : int gllen  : wchar_t *gldata
      １行の候補数 : 先頭候補が     : １行の長さ : 候補一覧行の文字列
                   : 何番目の候補か :
   -------------------------------------------------------------------------
   0 | 6           : 0              : 24         : １新２心３進４真５神６信
   1 | 4           : 6              : 16         : １臣２寝３伸４芯

    ＊kouhoinfo＊
      int khretsu  : int khpoint  : wchar_t *khdata
      なん列目に   : 行の先頭から : 候補の文字列
      ある候補か   : 何バイト目か :
   -------------------------------------------------------------------------
   0 | 0           : 0            : 新
   1 | 0           : 4            : 心
             :                :             :
   7 | 1           : 0            : 臣
   8 | 1           : 4            : 寝
  */

  kkptr = ic->allkouho;
  kptr = *(ic->allkouho);
  gptr = ic->glinebufp;

  /* line -- 何列目か
     ko   -- 全体の先頭から何番目の候補か
     lnko -- 列の先頭から何番目の候補か
     cn   -- 列の先頭から何バイト目か */

  for(line=0, ko=0; ko<ic->nIkouho; line++) {
    ic->glineifp[line].gldata = gptr; /* 候補行を表示するための文字列 */
    ic->glineifp[line].glhead = ko;   /* この行の先頭候補は、全体でのko番目 */

    ic->tooSmall = 1;
    for (lnko = cn = dn = 0 ;
	dn < netwidth && lnko < bangomax && ko < ic->nIkouho ; lnko++, ko++) {
      ic->tooSmall = 0;
      kptr = kkptr[ko];
      ic->kouhoifp[ko].khretsu = line; /* 何行目に存在するかを記録 */
      ic->kouhoifp[ko].khpoint = cn + (lnko ? 1 : 0);
      ic->kouhoifp[ko].khdata = kptr;  /* その文字列へのポインタ */
      svgptr = gptr;
      svcn = cn;
      svdn = dn;
      /* ２種類の表示を分ける */
      if(!(inhibit & (unsigned char)NUMBERING)) {
	/* 番号をコピーする */
	if (!cannaconf.indexHankaku) {/* 全角 */
	  if(lnko == 0) {
	    *gptr++ = *bango; cn ++; dn +=2;
	  } else {
	    WStrncpy(gptr, bango + 1 + BANGOSIZE * (lnko - 1), BANGOSIZE);
	    cn += BANGOSIZE; gptr += BANGOSIZE, dn += BANGOSIZE*2;
	  }
	}
	else{ /* 半角 */
	  WStrcpy(gptr, bango2[lnko]);
	  if(lnko == 0) {
	    dn +=2;
	  } else {
	    dn +=4;
	  }
	  cn += WStrlen(bango2[lnko]);
	  gptr += WStrlen(bango2[lnko]);
	}
      } else {
	/* 空白をコピーする */
	if(lnko) {
	  *gptr++ = *kuuhaku; cn ++; dn +=2;
	}
      }
      /* 候補をコピーする */
      for (; *kptr && dn < netwidth ; gptr++, kptr++, cn++) {
	*gptr = *kptr;
	if (WIsG0(*gptr))
	  dn++;
	else if (WIsG1(*gptr))
	  dn += 2;
	else if (WIsG2(*gptr))
	  dn ++;
	else if (WIsG3(*gptr))
	  dn += 2;
      }

      /* カラム数よりはみだしてしまいそうになったので１つ戻す */
      if (dn >= netwidth) {
	if (lnko) {
	  gptr = svgptr;
	  cn = svcn;
	  dn = svdn;
	}
	else {
	  ic->tooSmall = 1;
	}
	break;
      }
    }
    if (ic->tooSmall) {
      return 0;
    }
    if (cannaconf.kCount) {
      for (;dn < d->ncolumns - 1; dn++)
	*gptr++ = ' ';
    }
    /* １行終わり */
    *gptr++ = 0;
    ic->glineifp[line].glkosu = lnko;
    ic->glineifp[line].gllen = WStrlen(ic->glineifp[line].gldata);
  }

  /* 最後にNULLを入れる */
  ic->kouhoifp[ko].khretsu = 0;
  ic->kouhoifp[ko].khpoint = 0;
  ic->kouhoifp[ko].khdata  = (wchar_t *)NULL;
  ic->glineifp[line].glkosu  = 0;
  ic->glineifp[line].glhead  = 0;
  ic->glineifp[line].gllen   = 0;
  ic->glineifp[line].gldata  = (wchar_t *)NULL;

#if defined(DEBUG) && !defined(WIN)
  if (iroha_debug) {
    int i;
    for(i=0; ic->glineifp[i].glkosu; i++)
      printf("%d: %s\n", i, ic->glineifp[i].gldata);
  }
#endif

  return(0);
}

tanKouhoIchiran(d, step)
uiContext d;
int step;
{
  yomiContext yc = (yomiContext)d->modec;
  ichiranContext ic;
  int nelem, currentkouho, retval = 0;
  unsigned inhibit = 0;
  unsigned char listcallback = (unsigned char)(d->list_func ? 1 : 0);
  int netwidth;

  netwidth = d->ncolumns -
    (cannaconf.kCount ? (DEC_COLUMNS(9999) * 2 + 2/* 2は / と SP の分 */) : 0);

  /* 候補一覧行が狭くて候補一覧が出せない */
  if (listcallback == 0 && netwidth < 2) {
    /* tooSmall */
    return TanNextKouho(d);
  }

  /* 逐次関連 */
  yc->status |= CHIKUJI_OVERWRAP;

  /* すべての候補を取り出す */
  yc->allkouho = getIchiranList(yc->context, &nelem, &currentkouho);
  if (yc->allkouho == 0) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
    TanMuhenkan(d);
    makeGLineMessageFromString(d, jrKanjiError);
    return 0;
  }

  if (!cannaconf.HexkeySelect) {
    inhibit |= (unsigned char)NUMBERING;
  }

  yc->curIkouho = currentkouho;	/* 現在のカレント候補番号を保存する */
  currentkouho = step;	/* カレント候補から何番目をカレント候補とするか */

  /* 候補一覧に移行する */
  if ((retval = selectOne(d, yc->allkouho, &yc->curIkouho, nelem, BANGOMAX,
			  inhibit, currentkouho, WITH_LIST_CALLBACK,
			  ichiranEveryTimeCatch, ichiranExitCatch,
			  ichiranQuitCatch, NO_CALLBACK)) == NG) {
    freeGetIchiranList(yc->allkouho);
    return GLineNGReturn(d);
  }

  ic = (ichiranContext)d->modec;
  if (ic->tooSmall) {
    freeGetIchiranList(yc->allkouho);
    popIchiranMode(d);
    popCallback(d);
    return TanNextKouho(d);
  }

  ic->minorMode = CANNA_MODE_IchiranMode;
  currentModeInfo(d);

  if ( !(ic->flags & ICHIRAN_ALLOW_CALLBACK) ) {
    makeGlineStatus(d);
  }
  /* d->status = EVERYTIME_CALLBACK; */

  return(retval);
}

/*
 * 候補一覧行の表示を強制終了する
 */
IchiranQuit(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;
  int retval = 0;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    if (ic->flags & ICHIRAN_NEXT_EXIT) {
      d->list_func(d->client_data,
                     CANNA_LIST_Select, (wchar_t **)0, 0, (int *)0);
    }
    else {
      d->list_func(d->client_data,
                     CANNA_LIST_Quit, (wchar_t **)0, 0, (int *)0);
    }
  }
  
  if (ic->flags & ICHIRAN_NEXT_EXIT) {
    ichiranFin(d); 
    d->status = EXIT_CALLBACK;
  }
  else {
    *(ic->curIkouho) = ic->nIkouho - 1; /* ひらがな候補にする */
    ichiranFin(d); 
    d->status = QUIT_CALLBACK;
  }
  return(retval);
}

int
IchiranNop(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;

  if ((ic->flags & ICHIRAN_ALLOW_CALLBACK) && d->list_func) {
    (*d->list_func)
      (d->client_data, CANNA_LIST_Query, (wchar_t **)0, 0, (int *)0);
  }

  /* currentModeInfo でモード情報が必ず返るようにダミーのモードを入れておく */
  d->majorMode = d->minorMode = CANNA_MODE_AlphaMode;
  currentModeInfo(d);

  if (!(ic->flags & ICHIRAN_ALLOW_CALLBACK)) {
    makeGlineStatus(d);
  }
  return 0;
}

/*
   IchiranKakuteiThenDo

     -- Do determine from the candidate list, then do one more function.
 */

static
IchiranKakuteiThenDo(d, func)
uiContext d;
int func;
{
  ichiranContext ic = (ichiranContext)d->modec;
  int retval;
  BYTE ifl = ic->flags;

  if (!ic->prevMode || !ic->prevMode->func ||
      !(*ic->prevMode->func)((uiContext)0/*dummy*/, ic->prevMode, KEY_CHECK,
			     0/*dummy*/, func)) {
    return NothingChangedWithBeep(d);
  }
  retval = IchiranKakutei(d);
  if (ifl & ICHIRAN_STAY_LONG) {
    (void)IchiranQuit(d);
  }
  d->more.todo = 1;
  d->more.ch = d->ch;
  d->more.fnum = func;
  return retval;
}

static
IchiranQuitThenDo(d, func)
uiContext d;
int func;
{
  ichiranContext ic = (ichiranContext)d->modec;
  int retval;

  if (!ic->prevMode || !ic->prevMode->func ||
      !(*ic->prevMode->func)((uiContext)0/*dummy*/, ic->prevMode, KEY_CHECK,
			     0/*dummy*/, func)) {
    return NothingChangedWithBeep(d);
  }
  retval = IchiranQuit(d);
  d->more.todo = 1;
  d->more.ch = d->ch;
  d->more.fnum = func;
  return retval;
}

/*
 * 次候補に移動する
 *
 * ・カレント候補が最終候補だったら先頭候補をカレント候補とする
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
IchiranForwardKouho(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    int res;
    
    res = (*d->list_func)
      (d->client_data, CANNA_LIST_Forward, (wchar_t **)0, 0, (int *)0);
    if (res) {
      return 0;
    }
    else { /* CANNA_LIST_Forward was not prepared at the callback func */
      return IchiranKakuteiThenDo(d, CANNA_FN_Forward);
    }
  }

  /* 次候補にする (単語候補一覧状態で、最後の候補だったら一覧をやめる) */
  *(ic->curIkouho) += 1;
  if(*(ic->curIkouho) >= ic->nIkouho) {
    if (cannaconf.QuitIchiranIfEnd
       && (((coreContext)d->modec)->minorMode == CANNA_MODE_IchiranMode)) {
      return(IchiranQuit(d));
    }
    else if (cannaconf.CursorWrap) {
      *(ic->curIkouho) = 0;
    } else {
      *(ic->curIkouho) -= 1;
      return NothingChangedWithBeep(d);
    }
  }

  if(ic->tooSmall) { /* for bushu */
    d->status = AUX_CALLBACK;
    return 0;
  }

  makeGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return 0;
}

/*
 * 前候補に移動する
 *
 * ・カレント候補が先頭候補だったら最終候補をカレント候補とする
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
IchiranBackwardKouho(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;
  BYTE mode;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    int res;
    res = (*d->list_func)
      (d->client_data, CANNA_LIST_Backward, (wchar_t **)0, 0, (int *)0);
    if (res) {
      return 0;
    }
    else { /* CANNA_LIST_Backward was not prepared at the callback func */
      return IchiranKakuteiThenDo(d, CANNA_FN_Backward);
    }
  }

  /* 現在のモードを求める */
  if (cannaconf.QuitIchiranIfEnd)
    mode = ((coreContext)d->modec)->minorMode;

  /* 前候補にする (単語候補一覧状態で、最初の候補だったら一覧をやめる) */
  if(*(ic->curIkouho))
    *(ic->curIkouho) -= 1;
  else {
    if (cannaconf.QuitIchiranIfEnd && (mode == CANNA_MODE_IchiranMode)) {
      return(IchiranQuit(d));
    }
    else if (cannaconf.CursorWrap) {
      *(ic->curIkouho) = ic->nIkouho - 1;
    } else {
      *(ic->curIkouho) = 0;
      return NothingChangedWithBeep(d);
    }
  }

  if(ic->tooSmall) { /* for bushu */
    d->status = AUX_CALLBACK;
    return 0;
  }

  makeGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return 0;
}

/*
   IchiranConvert() will be called when `convert' key is pressed
 */

static IchiranConvert pro((uiContext));

static
IchiranConvert(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK && d->list_func) {
    (*d->list_func)
      (d->client_data, CANNA_LIST_Convert, (wchar_t **)0, 0, (int *)0);
    return 0;
  }
  else {
    return IchiranForwardKouho(d);
  }
}

/*
 * 前候補列に移動する
 *
 * ・カレント候補を求めて候補一覧とその場の候補を表示する
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
IchiranPreviousKouhoretsu(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    int res;
    res = (*d->list_func)
      (d->client_data, CANNA_LIST_Prev, (wchar_t **)0, 0, (int *)0);
    if (res) {
      return 0;
    }
    else { /* CANNA_LIST_Backward was not prepared at the callback func */
      return IchiranKakuteiThenDo(d, CANNA_FN_Prev);
    }
  }

  if(ic->tooSmall) { /* for bushu */
    return(IchiranBackwardKouho(d));
  }

  /* 前候補列にする (*(ic->curIkouho)を求める)*/
  getIchiranPreviousKouhoretsu(d);

  makeGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return 0;
}

/*
 * 前候補列のカレント候補を求める
 *
 * ・前候補列中の同じ候補番号のものをカレント候補とする
 * ・候補番号の同じものがない時は、その候補中の最終候補をカレント候補とする
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
static void
getIchiranPreviousKouhoretsu(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;
  int kindex;
  int curretsu, nretsu;

  /* カレント候補行のなかで何番目の候補かなのかを得る */
  kindex = *(ic->curIkouho) - 
    ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].glhead;
  /* 前候補列を得る */
  curretsu = ic->kouhoifp[*(ic->curIkouho)].khretsu;
  nretsu = ic->kouhoifp[ic->nIkouho - 1].khretsu + 1;
  if(curretsu == 0) {
    if (cannaconf.CursorWrap)
      curretsu = nretsu;
    else {
      NothingChangedWithBeep(d);
      return;
    }
  }
  curretsu -= 1;
  /* kindex がカレント候補列の候補数より大きくなってしまったら
     最右候補をカレント候補とする */
  if(ic->glineifp[curretsu].glkosu <= kindex) 
    kindex = ic->glineifp[curretsu].glkosu - 1;
  /* 前候補列の同じ番号に移動する */
  *(ic->curIkouho) = kindex + ic->glineifp[curretsu].glhead;
  return;
}

/*
 * 次候補列に移動する
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
IchiranNextKouhoretsu(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    int res;
    res = (*d->list_func)
      (d->client_data, CANNA_LIST_Next, (wchar_t **)0, 0, (int *)0);
    if (res) {
      return 0;
    }
    else { /* CANNA_LIST_Backward was not prepared at the callback func */
      return IchiranKakuteiThenDo(d, CANNA_FN_Next);
    }
  }

  if(ic->tooSmall) {
    return(IchiranForwardKouho(d));
  }

  /* 次候補列にする (*(ic->curIkouho) を求める) */
  getIchiranNextKouhoretsu(d);

  makeGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return 0;
}

/*
 * 次候補頁に移動する
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */

static IchiranNextPage pro((uiContext));

static
IchiranNextPage(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    int res;
    res = (*d->list_func)
      (d->client_data, CANNA_LIST_PageDown, (wchar_t **)0, 0, (int *)0);
    if (res) {
      return 0;
    }
    else { /* CANNA_LIST_Backward was not prepared at the callback func */
      return IchiranKakuteiThenDo(d, CANNA_FN_PageDown);
    }
  }

  return IchiranNextKouhoretsu(d);
}

/*
 * 前候補頁に移動する
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */

static IchiranPreviousPage pro((uiContext));

static
IchiranPreviousPage(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    int res;
    res = (*d->list_func)
      (d->client_data, CANNA_LIST_PageUp, (wchar_t **)0, 0, (int *)0);
    if (res) {
      return 0;
    }
    else { /* CANNA_LIST_Backward was not prepared at the callback func */
      return IchiranKakuteiThenDo(d, CANNA_FN_PageUp);
    }
  }

  return IchiranPreviousKouhoretsu(d);
}

/*
 * 次候補列に移動する
 *
 * ・次候補列中の同じ候補番号のものをカレント候補とする
 * ・候補番号の同じものがない時は、その候補中の最終候補をカレント候補とする
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
static void
getIchiranNextKouhoretsu(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;
  int kindex;
  int curretsu, nretsu;

  /* カレント候補行のなかで何番目の候補かなのかを得る */
  kindex = *(ic->curIkouho) - 
    ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].glhead;
  /* 次候補列を得る */
  curretsu = ic->kouhoifp[*(ic->curIkouho)].khretsu;
  nretsu = ic->kouhoifp[ic->nIkouho - 1].khretsu + 1;
  curretsu += 1;
  if(curretsu >= nretsu) {
    if (cannaconf.CursorWrap)
      curretsu = 0;
    else {
      NothingChangedWithBeep(d);
      return;
    }
  }
  /* kindex がカレント候補列の候補数より大きくなってしまったら
     最右候補をカレント候補とする */
  if(ic->glineifp[curretsu].glkosu <= kindex) 
    kindex = ic->glineifp[curretsu].glkosu - 1;
  /* 前候補列の同じ番号に移動する */
  *(ic->curIkouho) = kindex + ic->glineifp[curretsu].glhead;
  return;
}

/*
 * 候補行中の先頭候補に移動する
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
IchiranBeginningOfKouho(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    int res;
    res = (*d->list_func)
      (d->client_data, CANNA_LIST_BeginningOfLine, (wchar_t **)0, 0,(int *)0);
    if (res) {
      return 0;
    }
    else { /* CANNA_LIST_Backward was not prepared at the callback func */
      return IchiranKakuteiThenDo(d, CANNA_FN_BeginningOfLine);
    }
  }

  if(ic->tooSmall) {
    d->status = AUX_CALLBACK;
    return 0;
  }

  /* 候補列の先頭候補をカレント候補にする */
  *(ic->curIkouho) = 
    ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].glhead;

  makeGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return 0;
}

/*
 * 候補行中の最右候補に移動する
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
IchiranEndOfKouho(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;

  if (ic->flags & ICHIRAN_ALLOW_CALLBACK &&
      d->list_func) {
    int res;
    res = (*d->list_func)
      (d->client_data, CANNA_LIST_EndOfLine, (wchar_t **)0, 0, (int *)0);
    if (res) {
      return 0;
    }
    else { /* CANNA_LIST_Backward was not prepared at the callback func */
      return IchiranKakuteiThenDo(d, CANNA_FN_EndOfLine);
    }
  }

  if(ic->tooSmall) {
    d->status = AUX_CALLBACK;
    return 0;
  }

  /* 候補列の最右候補をカレント候補にする */
  *(ic->curIkouho) = 
    ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].glhead
    + ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].glkosu - 1;

  makeGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return 0;
}

/*
 * 候補行中の入力された番号の候補に移動する
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */

static int getIchiranBangoKouho pro((uiContext));
static IchiranBangoKouho pro((uiContext));

static
IchiranBangoKouho(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;
  int zflag, retval = 0;

  if(ic->tooSmall) {
    d->status = AUX_CALLBACK;
    return(retval);
  }

  /* d->status = EVERYTIME_CALLBACK; */

  if (cannaconf.HexkeySelect && !(ic->inhibit & NUMBERING)) {
    /* 入力された番号の候補をカレント候補とする */
    if((zflag = getIchiranBangoKouho(d)) == NG)
      goto insert;

    /* SelectDirect のカスタマイズの処理 */
  do_selection:
    if (cannaconf.SelectDirect) /* ON */ {
      if(zflag) /* ０が入力された */
	retval = IchiranQuit(d);
      else
	retval = IchiranKakutei(d);
    } else {          /* OFF */
      makeGlineStatus(d);
      /* d->status = EVERYTIME_CALLBACK; */
    }
    return(retval);
  }
  else {
#ifdef CANNA_LIST_Insert /* 絶対定義されているんだけどね */
    if (ic->flags & ICHIRAN_ALLOW_CALLBACK && d->list_func) {
      int res = (*d->list_func) /* list_func を呼び出す */
	(d->client_data, CANNA_LIST_Insert, (wchar_t **)0, d->ch, (int *)0);
      if (res) { /* d->ch がアプリケーション側で処理された */
	if (res == CANNA_FN_FunctionalInsert) {
	  zflag = 1; /* 0 じゃなければいい */
	  goto do_selection;
	}
	else if (res != CANNA_FN_Nop) {
	  /* アプリケーション側から要求して来た機能を続けて実行する */
	  d->more.todo = 1;
	  d->more.ch = d->ch;
	  d->more.fnum = CANNA_FN_FunctionalInsert;
	}
	return 0;
      }
      else { /* CANNA_LIST_Insert was not processed at the callback func */
	/* continue to the 'insert:' tag.. */
      }
    }
#endif

  insert:
    if(!(ic->inhibit & CHARINSERT) && cannaconf.allowNextInput) {
      BYTE ifl = ic->flags;
      retval = IchiranKakutei(d);
      if (ifl & ICHIRAN_STAY_LONG) {
	(void)IchiranQuit(d);
      }
      d->more.todo = 1;
      d->more.ch = d->ch;
      d->more.fnum = CANNA_FN_FunctionalInsert;
    } else {
      NothingChangedWithBeep(d);
    }
    return(retval);
  }
}

/*
 * 候補行中の入力された番号の候補に移動する
 *
 *
 * 引き数	uiContext
 * 戻り値	０が入力されたら              １を返す
 * 		１〜９、ａ〜ｆが入力されたら  ０を返す
 * 		エラーだったら              ー１を返す
 */
static int
getIchiranBangoKouho(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;
  int num, kindex;

  /* 入力データは ０〜９ ａ〜ｆ か？ */
  if(((0x30 <= d->ch) && (d->ch <= 0x39))
     || ((0x61 <= d->ch) && (d->ch <= 0x66))) {
    if((0x30 <= d->ch) && (d->ch <= 0x39))
      num = (int)(d->ch & 0x0f);
    else if((0x61 <= d->ch) && (d->ch <= 0x66))
      num = (int)(d->ch - 0x57);
  } 
  else {
    /* 入力された番号は正しくありません */
    return(NG);
  }
  /* 入力データは 候補行の中に存在する数か？ */
  if(num > ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].glkosu) {
    /* 入力された番号は正しくありません */
    return(NG);
  }

  /* 入力された数が０で SelectDirect が ON なら読みに戻して１を返す */
  if(num == 0) {
    if (cannaconf.SelectDirect)
      return(1);
    else {
      /* 入力された番号は正しくありません */
      return(NG);
    }  
  } else {
    /* 候補列の先頭候補を得る */
    kindex = ic->glineifp[ic->kouhoifp[*(ic->curIkouho)].khretsu].glhead;
    *(ic->curIkouho) = kindex + (num - 1);
  }

  return(0);
}

/*
 * 候補行中から選択された候補をカレント候補とする
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */

static IchiranKakutei pro((uiContext));

static
IchiranKakutei(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;
  int retval = 0;
  wchar_t *kakuteiStrings;

  if ((ic->flags & ICHIRAN_ALLOW_CALLBACK) && d->list_func) {
    if (ic->flags & ICHIRAN_STAY_LONG) {
      d->list_func(d->client_data,
                     CANNA_LIST_Query, (wchar_t **)0, 0, (int *)0);
    }
    else {
      d->list_func(d->client_data,
                     CANNA_LIST_Select, (wchar_t **)0, 0, (int *)0);
    }
  }

  kakuteiStrings = ic->allkouho[*(ic->curIkouho)];
  retval = d->nbytes = WStrlen(kakuteiStrings);
  WStrcpy(d->buffer_return, kakuteiStrings);

  if (ic->flags & ICHIRAN_STAY_LONG) {
    ic->flags |= ICHIRAN_NEXT_EXIT; 
    d->status = EVERYTIME_CALLBACK;
  }
  else {
    ichiranFin(d);

    d->status = EXIT_CALLBACK;
  }

  return(retval);
}

/*
 * 候補行表示モードから抜ける
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
void
ichiranFin(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec; 

  /* 候補一覧表示行用のエリアをフリーする */
  freeIchiranBuf(ic);

  popIchiranMode(d);

  /* gline をクリアする */
  GlineClear(d);
}

static IchiranExtendBunsetsu pro((uiContext));

static
IchiranExtendBunsetsu(d)
uiContext d;
{
  return IchiranQuitThenDo(d, CANNA_FN_Extend);
}

static IchiranShrinkBunsetsu pro((uiContext));

static
IchiranShrinkBunsetsu(d)
uiContext d;
{
  return IchiranQuitThenDo(d, CANNA_FN_Shrink);
}

static IchiranAdjustBunsetsu pro((uiContext));

static
IchiranAdjustBunsetsu(d)
uiContext d;
{
  return IchiranQuitThenDo(d, CANNA_FN_AdjustBunsetsu);
}

static IchiranKillToEndOfLine pro((uiContext));

static
IchiranKillToEndOfLine(d)
uiContext d;
{
  return IchiranKakuteiThenDo(d, CANNA_FN_KillToEndOfLine);
}

static IchiranDeleteNext pro((uiContext));

static
IchiranDeleteNext(d)
uiContext d;
{
  return IchiranKakuteiThenDo(d, CANNA_FN_DeleteNext);
}

static IchiranBubunMuhenkan pro((uiContext));

static
IchiranBubunMuhenkan(d)
uiContext d;
{
  return IchiranQuitThenDo(d, CANNA_FN_BubunMuhenkan);
}

static IchiranHiragana pro((uiContext));

static
IchiranHiragana(d)
uiContext d;
{
  return IchiranQuitThenDo(d, CANNA_FN_Hiragana);
}

static IchiranKatakana pro((uiContext));

static
IchiranKatakana(d)
uiContext d;
{
  return IchiranQuitThenDo(d, CANNA_FN_Katakana);
}

static IchiranZenkaku pro((uiContext));

static
IchiranZenkaku(d)
uiContext d;
{
  return IchiranQuitThenDo(d, CANNA_FN_Zenkaku);
}

static IchiranHankaku pro((uiContext));

static
IchiranHankaku(d)
uiContext d;
{
  return IchiranQuitThenDo(d, CANNA_FN_Hankaku);
}

static IchiranRomaji pro((uiContext));

static
IchiranRomaji(d)
uiContext d;
{
  return IchiranQuitThenDo(d, CANNA_FN_Romaji);
}

static IchiranToUpper pro((uiContext));

static
IchiranToUpper(d)
uiContext d;
{
  return IchiranQuitThenDo(d, CANNA_FN_ToUpper);
}

static IchiranToLower pro((uiContext));

static
IchiranToLower(d)
uiContext d;
{
  return IchiranQuitThenDo(d, CANNA_FN_ToLower);
}

static IchiranCapitalize pro((uiContext));

static
IchiranCapitalize(d)
uiContext d;
{
  return IchiranQuitThenDo(d, CANNA_FN_Capitalize);
}

static IchiranKanaRotate pro((uiContext));

static
IchiranKanaRotate(d)
uiContext d;
{
  return IchiranQuitThenDo(d, CANNA_FN_KanaRotate);
}

static IchiranRomajiRotate pro((uiContext));

static
IchiranRomajiRotate(d)
uiContext d;
{
  return IchiranQuitThenDo(d, CANNA_FN_RomajiRotate);
}

static IchiranCaseRotateForward pro((uiContext));

static
IchiranCaseRotateForward(d)
uiContext d;
{
  return IchiranQuitThenDo(d, CANNA_FN_CaseRotate);
}

#include	"ichiranmap.h"
