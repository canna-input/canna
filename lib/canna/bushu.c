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
static char rcs_id[] = "@(#) 102.1 $Id: bushu.c,v 6.10 1996/11/28 10:25:35 kon Exp $";
#endif /* lint */

#include	<errno.h>
#include "canna.h"

#ifdef luna88k
extern int errno;
#endif

extern wchar_t *WString();

extern int uuslQuitCatch();
extern int uuslIchiranQuitCatch();
static int bushuHenkan(), makeBushuIchiranQuit();
static int vBushuExitCatch(), bushuQuitCatch();


#define	BUSHU_SZ	150

static
char *bushu_schar[] = 
{ 
  /* "一", "丿", "凵", "十", "卩", "刀", */
  "\260\354", "\320\250", "\321\341", "\275\275", "\322\307", "\305\341",
  
  /* "刈（りっとう）", "力", "厂", "勹", "冂匚囗", "亠", */
  "\264\242\241\312\244\352\244\303\244\310\244\246\241\313", "\316\317", "\322\314", "\322\261", "\321\304\322\271\323\370", "\320\265",
  
  /* "冫", "人／仁（にんべん）", "又", "几", "八", "儿", */
  "\321\322", "\277\315\241\277\277\316\241\312\244\313\244\363\244\331\244\363\241\313", "\313\364", "\321\334", "\310\254", "\321\271",
  
  /* "冖", "宀", "廴", "郁（おおざと)", "己", "女", */
  "\321\314", "\325\337", "\327\256", "\260\352\241\312\244\252\244\252\244\266\244\310\51", "\270\312", "\275\367",
  
  /* "彳", "口", "草（くさかんむり)", "独（けものへん）", */
  "\327\306", "\270\375", "\301\360\241\312\244\257\244\265\244\253\244\363\244\340\244\352\51", "\306\310\241\312\244\261\244\342\244\316\244\330\244\363\241\313",

  /* "子", "陏（こざと）", "士", "江（さんずい）", "弋", */
  "\273\322", "\357\372\241\312\244\263\244\266\244\310\241\313", "\273\316", "\271\276\241\312\244\265\244\363\244\272\244\244\241\313", "\327\265",
  
  /* "尸", "小／単（つ）", "辷（しんにょう）", "寸", "大", */
  "\325\371", "\276\256\241\277\303\261\241\312\244\304\241\313", "\355\350\241\312\244\267\244\363\244\313\244\347\244\246\241\313", "\300\243", "\302\347",
  
  /* "土", "手（てへん）", "巾", "广", "山", "夕", */
  "\305\332", "\274\352\241\312\244\306\244\330\244\363\241\313", "\266\322", "\326\370", "\273\263", "\315\274",
  
  /* "弓", "忙（りっしんべん）", "欠", "歹", "犬", */
  "\265\335", "\313\273\241\312\244\352\244\303\244\267\244\363\244\331\244\363\241\313", "\267\347", "\335\306", "\270\244",
  
  /* "牛／牡（うしへん）", "片", "木", "气", "毛", "心", */
  "\265\355\241\277\262\264\241\312\244\246\244\267\244\330\244\363\241\313", "\312\322", "\314\332", "\335\343", "\314\323", "\277\264",
  
  /* "水", "月", "爪", "日", "攵", "火", */
  "\277\345", "\267\356", "\304\336", "\306\374", "\332\276", "\262\320",
  
  /* "方", "戈", "点（れっか）", "殳", "穴", "石", */
  "\312\375", "\330\371", "\305\300\241\312\244\354\244\303\244\253\241\313", "\335\325", "\267\352", "\300\320",

  /* "玉", "皮", "瓦", "皿", "示", "神（しめすへん）", "白", */
  "\266\314", "\310\351", "\264\244", "\273\256", "\274\250", "\277\300\241\312\244\267\244\341\244\271\244\330\244\363\241\313", "\307\362",
  
  /* "田", "立", "禾", "目", "癶", "矢", */
  "\305\304", "\316\251", "\262\323", "\314\334", "\342\242", "\314\360",
  
  /* "疔（やまいだれ）", "四", "糸", "臼", "瓜", "老", */
  "\341\313\241\312\244\344\244\336\244\244\244\300\244\354\241\313", "\273\315", "\273\345", "\261\261", "\261\273", "\317\267",
  
  /* "缶", "衣", "初（ころもへん）", "米", "舌", "耒", */
  "\264\314", "\260\341", "\275\351\241\312\244\263\244\355\244\342\244\330\244\363\241\313", "\312\306", "\300\345", "\346\320",
  
  /* "竹（たけかんむり）", "血", "虎（とらかんむり）", "肉", */
  "\303\335\241\312\244\277\244\261\244\253\244\363\244\340\244\352\241\313", "\267\354", "\270\327\241\312\244\310\244\351\244\253\244\363\244\340\244\352\241\313", "\306\371",
  
  /* "西", "羽", "羊", "聿", "舟", "耳", */
  "\300\276", "\261\251", "\315\323", "\346\346", "\275\256", "\274\252",
  
  /* "虫", "赤", "足／疋", "豕", "臣", */
  "\303\356", "\300\326", "\302\255\241\277\311\245", "\354\265", "\277\303",
  
  /* "貝", "辛", "車", "見", "言", "酉", "走", "谷", */
  "\263\255", "\277\311", "\274\326", "\270\253", "\270\300", "\306\323", "\301\366", "\303\253",
  
  /* "角", "釆", "麦", "豆", "身", "豸", "雨", "非", */
  "\263\321", "\310\320", "\307\376", "\306\246", "\277\310", "\354\270", "\261\253", "\310\363",
  
  /* "金", "門", "隹", "頁", "音", "香", "革", "風", */
  "\266\342", "\314\347", "\360\262", "\312\307", "\262\273", "\271\341", "\263\327", "\311\367",
  
  /* "首", "食", "韋", "面", "馬", "鬼", "髟", "高", */
  "\274\363", "\277\251", "\360\352", "\314\314", "\307\317", "\265\264", "\361\365", "\271\342",
  
  /* "鬥", "骨", "魚", "亀", "鳥", "黒", "鹿", "鼻", */
  "\362\250", "\271\374", "\265\373", "\265\265", "\304\273", "\271\365", "\274\257", "\311\241",

  /* "齒", "記号", "その他" */
  "\363\357", "\265\255\271\346", "\244\275\244\316\302\276"
};

static
char *bushu_skey[] =  
{ 
/* "いち", "の", "うけばこ", "じゅう", "ふし", "かたな", */
"\244\244\244\301", "\244\316", "\244\246\244\261\244\320\244\263", "\244\270\244\345\244\246", "\244\325\244\267", "\244\253\244\277\244\312",

/* "りっとう", "か", "がん", "く", "かまえ", "なべ", "に", */
"\244\352\244\303\244\310\244\246", "\244\253", "\244\254\244\363", "\244\257", "\244\253\244\336\244\250", "\244\312\244\331", "\244\313",

/* "ひと", "ぬ", "つくえ", "はち", "る", "わ", */
"\244\322\244\310", "\244\314", "\244\304\244\257\244\250", "\244\317\244\301", "\244\353", "\244\357",

/* "う", "えん", "おおざと", "おのれ", "おんな", "ぎょう", */
"\244\246", "\244\250\244\363", "\244\252\244\252\244\266\244\310", "\244\252\244\316\244\354", "\244\252\244\363\244\312", "\244\256\244\347\244\246",

/* "ろ", "くさ", "けもの", "こ", "こざと", "さむらい", */
"\244\355", "\244\257\244\265", "\244\261\244\342\244\316", "\244\263", "\244\263\244\266\244\310", "\244\265\244\340\244\351\244\244",

/* "し", "しき", "しゃく", "つ", "しん", "すん", */
"\244\267", "\244\267\244\255", "\244\267\244\343\244\257", "\244\304", "\244\267\244\363", "\244\271\244\363",

/* "だい", "ど", "て", "はば", "ま", "やま", */
"\244\300\244\244", "\244\311", "\244\306", "\244\317\244\320", "\244\336", "\244\344\244\336",

/* "ゆう", "ゆみ", "りっしん", "けつ", "いちた", "いぬ", */
"\244\346\244\246", "\244\346\244\337", "\244\352\244\303\244\267\244\363", "\244\261\244\304", "\244\244\244\301\244\277", "\244\244\244\314",

/* "うし", "かた", "き", "きがまえ", "け", "こころ", */
"\244\246\244\267", "\244\253\244\277", "\244\255", "\244\255\244\254\244\336\244\250", "\244\261", "\244\263\244\263\244\355",

/* "すい", "つき", "つめ", "にち", "のぶん", "ひ", */
"\244\271\244\244", "\244\304\244\255", "\244\304\244\341", "\244\313\244\301", "\244\316\244\326\244\363", "\244\322",

/* "ほう", "ほこ", "よつてん", "るまた", "あな", "いし", */
"\244\333\244\246", "\244\333\244\263", "\244\350\244\304\244\306\244\363", "\244\353\244\336\244\277", "\244\242\244\312", "\244\244\244\267",

/* "おう", "かわ", "かわら", "さら", "しめす", "ね", */
"\244\252\244\246", "\244\253\244\357", "\244\253\244\357\244\351", "\244\265\244\351", "\244\267\244\341\244\271", "\244\315",

/* "しろ", "た", "たつ", "のぎ", "め", "はつ", "や", */
"\244\267\244\355", "\244\277", "\244\277\244\304", "\244\316\244\256", "\244\341", "\244\317\244\304", "\244\344",

/* "やまい", "よん", "いと", "うす", "うり", "おい", */
"\244\344\244\336\244\244", "\244\350\244\363", "\244\244\244\310", "\244\246\244\271", "\244\246\244\352", "\244\252\244\244",

/* "かん", "きぬ", "ころも", "こめ", "した", "すき", */
"\244\253\244\363", "\244\255\244\314", "\244\263\244\355\244\342", "\244\263\244\341", "\244\267\244\277", "\244\271\244\255",

/* "たけ", "ち", "とら", "にく", "にし", "はね", "ひつじ", */
"\244\277\244\261", "\244\301", "\244\310\244\351", "\244\313\244\257", "\244\313\244\267", "\244\317\244\315", "\244\322\244\304\244\270",

/* "ふで", "ふね", "みみ", "むし", "あか", "あし", */
"\244\325\244\307", "\244\325\244\315", "\244\337\244\337", "\244\340\244\267", "\244\242\244\253", "\244\242\244\267",

/* "いのこ", "おみ", "かい", "からい", "くるま", "けん", */
"\244\244\244\316\244\263", "\244\252\244\337", "\244\253\244\244", "\244\253\244\351\244\244", "\244\257\244\353\244\336", "\244\261\244\363",

/* "ごん", "さけ", "そう", "たに", "つの", "のごめ", */
"\244\264\244\363", "\244\265\244\261", "\244\275\244\246", "\244\277\244\313", "\244\304\244\316", "\244\316\244\264\244\341",

/* "ばく", "まめ", "み", "むじな", "あめ", "あらず", */
"\244\320\244\257", "\244\336\244\341", "\244\337", "\244\340\244\270\244\312", "\244\242\244\341", "\244\242\244\351\244\272",

/* "かね", "もん", "ふるとり", "ぺーじ", "おと", "こう", */
"\244\253\244\315", "\244\342\244\363", "\244\325\244\353\244\310\244\352", "\244\332\241\274\244\270", "\244\252\244\310", "\244\263\244\246",

/* "かく", "かぜ", "くび", "しょく", "なめし", "めん", */
"\244\253\244\257", "\244\253\244\274", "\244\257\244\323", "\244\267\244\347\244\257", "\244\312\244\341\244\267", "\244\341\244\363",

/* "うま", "おに", "かみ", "たかい", "とう", "ほね", */
"\244\246\244\336", "\244\252\244\313", "\244\253\244\337", "\244\277\244\253\244\244", "\244\310\244\246", "\244\333\244\315",

/* "うお", "かめ", "とり", "くろ", "しか", "はな", */
"\244\246\244\252", "\244\253\244\341", "\244\310\244\352", "\244\257\244\355", "\244\267\244\253", "\244\317\244\312",

/* "は", "きごう", "そのた" */
"\244\317", "\244\255\244\264\244\246", "\244\275\244\316\244\277"
};


#define	BUSHU_CNT	(sizeof(bushu_schar)/sizeof(char *))

static wchar_t *bushu_char[BUSHU_CNT];
static wchar_t *bushu_key[BUSHU_CNT];

int
initBushuTable()
{
  int retval = 0;

  retval = setWStrings(bushu_char, bushu_schar, BUSHU_CNT);
  if (retval != NG) {
    retval = setWStrings(bushu_key, bushu_skey, BUSHU_CNT);
  }
  return retval;
}


/*
 * 部首候補のエコー用の文字列を作る
 *
 * 引き数	RomeStruct
 * 戻り値	正常終了時 0
 */
static int
makeBushuEchoStr(d)
uiContext d;
{
  ichiranContext ic = (ichiranContext)d->modec;

  d->kanji_status_return->echoStr = ic->allkouho[*(ic->curIkouho)];
  d->kanji_status_return->length = 1;
  d->kanji_status_return->revPos = 0;
  d->kanji_status_return->revLen = 1;

  return(0);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * forichiranContext用関数                                                   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * forichiranContext の初期化
 */
static
clearForIchiranContext(p)
forichiranContext p;
{
  p->id = FORICHIRAN_CONTEXT;
  p->curIkouho = 0;
  p->allkouho = 0;

  return(0);
}
  
static forichiranContext
newForIchiranContext()
{
  forichiranContext fcxt;

  if ((fcxt = (forichiranContext)malloc(sizeof(forichiranContextRec)))
                                             == (forichiranContext)NULL) {
#ifndef WIN
    jrKanjiError = "malloc (newForIchiranContext) できませんでした";
#else
    jrKanjiError = "malloc (newForIchiranContext) \244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277";  /* できませんでした */
#endif
    return (forichiranContext)NULL;
  }
  clearForIchiranContext(fcxt);

  return fcxt;
}

getForIchiranContext(d)
uiContext d;
{
  forichiranContext fc;
  int retval = 0;

  if (pushCallback(d, d->modec, NO_CALLBACK, NO_CALLBACK,
                                  NO_CALLBACK, NO_CALLBACK) == 0) {
#ifndef WIN
    jrKanjiError = "malloc (pushCallback) できませんでした";
#else
    jrKanjiError = "malloc (pushCallback) \244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277"; /* できませんでした */
#endif
    return(NG);
  }
  
  if((fc = newForIchiranContext()) == NULL) {
    popCallback(d);
    return(NG);
  }
  fc->next = d->modec;
  d->modec = (mode_context)fc;

  fc->prevMode = d->current_mode;
  fc->majorMode = d->majorMode;

  return(retval);
}

void
popForIchiranMode(d)
uiContext d;
{
  forichiranContext fc = (forichiranContext)d->modec;

  d->modec = fc->next;
  d->current_mode = fc->prevMode;
  freeForIchiranContext(fc);
}

#ifndef NO_EXTEND_MENU
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 部首モード入力                                                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
vBushuMode(d, major_mode)
uiContext d;
int major_mode;
{
  forichiranContext fc;
  ichiranContext ic;
  unsigned inhibit = 0;
  int retval = 0;

  d->status = 0;

  if((retval = getForIchiranContext(d)) == NG) {
    killmenu(d);
    return(GLineNGReturn(d));
  }

  fc = (forichiranContext)d->modec;

  /* selectOne を呼ぶための準備 */
  fc->allkouho = bushu_char;
  fc->curIkouho = 0;
  if (!cannaconf.HexkeySelect)
    inhibit |= ((unsigned char)NUMBERING | (unsigned char)CHARINSERT);
  else
    inhibit |= (unsigned char)CHARINSERT;

  if((retval = selectOne(d, fc->allkouho, &fc->curIkouho, BUSHU_SZ,
		 BANGOMAX, inhibit, 0, WITH_LIST_CALLBACK,
		 NO_CALLBACK, vBushuExitCatch,
		 bushuQuitCatch, uiUtilIchiranTooSmall)) == NG) {
    killmenu(d);
    return(GLineNGReturnFI(d));
  }

  ic = (ichiranContext)d->modec;
  ic->majorMode = major_mode;
  ic->minorMode = CANNA_MODE_BushuMode;
  currentModeInfo(d);

  *(ic->curIkouho) = d->curbushu;

  /* 候補一覧行が狭くて候補一覧が出せない */
  if(ic->tooSmall) {
    d->status = AUX_CALLBACK;
    killmenu(d);
    return(retval);
  }

  if ( !(ic->flags & ICHIRAN_ALLOW_CALLBACK) ) {
    makeGlineStatus(d);
  }
  /* d->status = ICHIRAN_EVERYTIME; */

  return(retval);
}

static
vBushuIchiranQuitCatch(d, retval, env)
     uiContext d;
     int retval;
     mode_context env;
     /* ARGSUSED */
{
  popCallback(d); /* 一覧をポップ */

  if (((forichiranContext)env)->allkouho != (wchar_t **)bushu_char) {
    /* bushu_char は static の配列だから free してはいけない。
       こう言うのってなんか汚いなあ */
    freeGetIchiranList(((forichiranContext)env)->allkouho);
  }
  popForIchiranMode(d);
  popCallback(d);

  return(vBushuMode(d, CANNA_MODE_BushuMode));
}

static
vBushuExitCatch(d, retval, env)
     uiContext d;
     int retval;
     mode_context env;
     /* ARGSUSED */
{
  forichiranContext fc;
  int cur, res;

  popCallback(d); /* 一覧をポップ */

  fc = (forichiranContext)d->modec;
  cur = fc->curIkouho;

  popForIchiranMode(d);
  popCallback(d);

  res = bushuHenkan(d, 1, 1, cur, vBushuIchiranQuitCatch);
  if (res < 0) {
    makeYomiReturnStruct(d);
    return 0;
  }
  return res;
}

BushuMode(d)
uiContext d;
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    killmenu(d);
    return NothingChangedWithBeep(d);
  }    

  return(vBushuMode(d, CANNA_MODE_BushuMode));
}
#endif /* not NO_EXTEND_MENU */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 部首モード入力の一覧表示                                                  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static bushuEveryTimeCatch pro((uiContext, int, mode_context));

static
bushuEveryTimeCatch(d, retval, env)
     uiContext d;
     int retval;
     mode_context env;
     /* ARGSUSED */
{
  makeBushuEchoStr(d);

  return(retval);
}

static bushuExitCatch pro((uiContext, int, mode_context));

static
bushuExitCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
{
  yomiContext yc;

  popCallback(d); /* 一覧をポップ */

  if (((forichiranContext)env)->allkouho != bushu_char) {
    /* bushu_char は static の配列だから free してはいけない。
       こう言うのってなんか汚いなあ */
    freeGetIchiranList(((forichiranContext)env)->allkouho);
  }
  popForIchiranMode(d);
  popCallback(d);
  yc = (yomiContext)d->modec;
  if (yc->savedFlags & CANNA_YOMI_MODE_SAVED) {
    restoreFlags(yc);
  }
  retval = YomiExit(d, retval);
  killmenu(d);
  currentModeInfo(d);

  return retval;
}

#ifndef NO_EXTEND_MENU
static
bushuQuitCatch(d, retval, env)
     uiContext d;
     int retval;
     mode_context env;
     /* ARGSUSED */
{
  popCallback(d); /* 一覧をポップ */

  if (((forichiranContext)env)->allkouho != (wchar_t **)bushu_char) {
    /* bushu_char は static の配列だから free してはいけない。
       こう言うのってなんか汚いなあ */
    freeGetIchiranList(((forichiranContext)env)->allkouho);
  }
  popForIchiranMode(d);
  popCallback(d);
  currentModeInfo(d);
  GlineClear(d);

  return prevMenuIfExist(d);
}
#endif /* not NO_EXTEND_MENU */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 部首としての変換の一覧表示                                                *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
convBushuQuitCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
{
  popCallback(d); /* 一覧をポップ */

  if (((forichiranContext)env)->allkouho != (wchar_t **)bushu_char) {
    /* bushu_char は static の配列だから free してはいけない。
       こう言うのってなんか汚いなあ */
    freeGetIchiranList(((forichiranContext)env)->allkouho);
  }
  popForIchiranMode(d);
  popCallback(d);

  makeYomiReturnStruct(d);
  currentModeInfo(d);

  return(retval);
}

/*
 * 読みを部首として変換する
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
int ConvertAsBushu pro((uiContext));

ConvertAsBushu(d)
uiContext	d;
{
  yomiContext yc = (yomiContext)d->modec;
  int res;

  d->status = 0; /* clear status */
  
  if (yc->henkanInhibition & CANNA_YOMI_INHIBIT_ASBUSHU ||
      yc->right || yc->left) {
    return NothingChangedWithBeep(d);
  }

  if (yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) {
    if (!(yc->status & CHIKUJI_OVERWRAP) && yc->nbunsetsu) {
      moveToChikujiTanMode(d);
      return TanKouhoIchiran(d);
    }
    else if (yc->nbunsetsu) {
      return NothingChanged(d);
    }
  }

  d->nbytes = yc->kEndp;
  WStrncpy(d->buffer_return, yc->kana_buffer, d->nbytes);

  /* 0 は、ConvertAsBushu から呼ばれたことを示す */
  res = bushuHenkan(d, 0, 1, 0, convBushuQuitCatch);
  if (res < 0) {
    makeYomiReturnStruct(d);
    return 0;
  }
  return res;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 共通部                                                                    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * 読みを部首辞書から部首変換する
 */
static
bushuBgnBun(st, yomi, length)
RkStat *st;
wchar_t *yomi;
int length;
{
  int nbunsetsu;
  extern defaultBushuContext;

  /* 連文節変換を開始する *//* 辞書にある候補のみ取り出す */
  if ((defaultBushuContext == -1)) {
    if (KanjiInit() == -1 || defaultBushuContext == -1) {
      jrKanjiError = KanjiInitError();
      return(NG);
    }
  }

  nbunsetsu = RkwBgnBun(defaultBushuContext, yomi, length, RK_CTRLHENKAN);
  if(nbunsetsu == -1) {
    if(errno == EPIPE)
      jrKanjiPipeError();
    jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\313\274\272\307\324\244\267\244\336\244\267\244\277"; 
	    /* かな漢字変換に失敗しました */
    return(NG);
  }
  
  if(RkwGetStat(defaultBushuContext, st) == -1) {
    if(errno == EPIPE)
      jrKanjiPipeError();
    jrKanjiError = "\245\271\245\306\245\244\245\277\245\271\244\362\274\350\244\352\275\320\244\273\244\336\244\273\244\363\244\307\244\267\244\277";
                  /* ステイタスを取り出せませんでした */
    return(NG);
  }

  return(nbunsetsu);
}

/*
 * 読みに半濁点を付加して候補一覧行を表示する
 *
 * 引き数	uiContext
 *		flag	ConvertAsBushuから呼ばれた 0
 *			BushuYomiHenkanから呼ばれた 1
 * 戻り値	正常終了時 0	異常終了時 -1
 *
 *
 * ここに来る時はまだ getForIchiranContext が呼ばれていないものとする
 */

static
bushuHenkan(d, flag, ext, cur, quitfunc)
uiContext	d;
int             flag, cur;
int             (*quitfunc) pro((uiContext, int, mode_context));
{
  forichiranContext fc;
  ichiranContext ic;
  unsigned inhibit = 0;
  wchar_t *yomi, **allBushuCands;
  RkStat	st;
  int nelem, currentkouho, nbunsetsu, length, retval = 0;
  extern defaultBushuContext;
  
  wchar_t **getIchiranList();

  if(flag) {
    yomi = (wchar_t *)bushu_key[cur];
    length = WStrlen(yomi);
    d->curbushu = (short)cur;
  } else {
    d->nbytes = RomajiFlushYomi(d, d->buffer_return, d->n_buffer);
    yomi = d->buffer_return;
    length = d->nbytes;
  }

  if((nbunsetsu = bushuBgnBun(&st, yomi, length)) == NG) {
    killmenu(d);
    (void)GLineNGReturn(d);
    return -1;
  }

  if((nbunsetsu != 1) || (st.klen > 1) || (st.maxcand == 0)) {
    /* 部首としての候補がない */

    d->kanji_status_return->length = -1;

    makeBushuIchiranQuit(d, flag);
    currentModeInfo(d);

    killmenu(d);
    if(flag) {
      makeGLineMessageFromString(d, "\244\263\244\316\311\364\274\363\244\316\270\365\312\344\244\317\244\242\244\352\244\336\244\273\244\363");
                                  /* この部首の候補はありません */
    } else {
      return(NothingChangedWithBeep(d));
    }
    return(0);
  }

  /* 候補一覧行を表示する */
  /* 0 は、カレント候補 + 0 をカレント候補にすることを示す */

  if((allBushuCands
      = getIchiranList(defaultBushuContext, &nelem, &currentkouho)) == 0) {
    killmenu(d);
    (void)GLineNGReturn(d);
    return -1;
  }

  /* 部首変換は学習しない。 */
  if(RkwEndBun(defaultBushuContext, 0) == -1) { /* 0:学習しない */
    if(errno == EPIPE)
      jrKanjiPipeError();
    jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\316\275\252\316\273\244\313\274\272\307\324\244\267\244\336\244\267\244\277";
                   /* かな漢字変換の終了に失敗しました */
    freeGetIchiranList(allBushuCands);
    killmenu(d);
    (void)GLineNGReturn(d);
    return -1;
  }

  if(getForIchiranContext(d) == NG) {
    freeGetIchiranList(allBushuCands);
    killmenu(d);
    (void)GLineNGReturn(d);
    return -1;
  }

  fc = (forichiranContext)d->modec;
  fc->allkouho = allBushuCands;

  if (!cannaconf.HexkeySelect)
    inhibit |= (unsigned char)NUMBERING;
  fc->curIkouho = currentkouho;	/* 現在のカレント候補番号を保存する */
  currentkouho = 0;	/* カレント候補から何番目をカレント候補とするか */

  if((retval = selectOne(d, fc->allkouho, &fc->curIkouho, nelem, BANGOMAX,
			 inhibit, currentkouho, WITH_LIST_CALLBACK,
			 bushuEveryTimeCatch, bushuExitCatch,
			 quitfunc, uiUtilIchiranTooSmall)) == NG) {
    freeGetIchiranList(allBushuCands);
    killmenu(d);
    (void)GLineNGReturnFI(d);
    return -1;
  }

  ic = (ichiranContext)d->modec;

  if(!flag) { /* convertAsBushu */
    ic->majorMode = ic->minorMode = CANNA_MODE_BushuMode;
  } else {
    if(ext) {
      ic->majorMode = ic->minorMode = CANNA_MODE_BushuMode;
    } else {
      ic->majorMode = CANNA_MODE_ExtendMode;
      ic->minorMode = CANNA_MODE_BushuMode;
    }
  }
  currentModeInfo(d);

  /* 候補一覧行が狭くて候補一覧が出せない */
  if(ic->tooSmall) {
    d->status = AUX_CALLBACK;
    killmenu(d);
    return(retval);
  }

  if ( !(ic->flags & ICHIRAN_ALLOW_CALLBACK) ) {
    makeGlineStatus(d);
  }
  /* d->status = EVERYTIME_CALLBACK; */

  return(retval);
}

/*
 * 候補行を消去し、部首モードから抜け、読みがないモードに移行する
 *
 * 引き数	uiContext
 *		flag	ConvertAsBushuから呼ばれた 0
 *			BushuYomiHenkanから呼ばれた 1
 * 戻り値	正常終了時 0	異常終了時 -1
 */
static
makeBushuIchiranQuit(d, flag)
uiContext	d;
int              flag;
{
  extern defaultBushuContext;

  /* 部首変換は学習しない。 */
  if(RkwEndBun(defaultBushuContext, 0) == -1) { /* 0:学習しない */
    if(errno == EPIPE)
      jrKanjiPipeError();
    jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\316\275\252\316\273\244\313\274\272\307\324\244\267\244\336\244\267\244\277";
                   /* かな漢字変換の終了に失敗しました */
    return(NG);
  }

  if(flag) {
    /* kanji_status_return をクリアする */
    d->kanji_status_return->length  = 0;
    d->kanji_status_return->revLen  = 0;
    
/*
    d->status = QUIT_CALLBACK;
*/
  } else {
    makeYomiReturnStruct(d);
  }
  GlineClear(d);
  
  return(0);
}


