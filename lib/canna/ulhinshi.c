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
static char rcs_id[] = "@(#) 102.1 $Id: ulhinshi.c,v 7.8 1996/11/06 01:57:57 kon Exp $";
#endif

#include <errno.h>
#include "canna.h"

#ifndef NO_EXTEND_MENU
#ifdef luna88k
extern int errno;
#endif

static int tourokuYes pro((uiContext)),
           tourokuNo pro((uiContext)),
           makeDoushi pro((uiContext)),
           uuTDicExitCatch pro((uiContext, int, mode_context)),
           uuTDicQuitCatch pro((uiContext, int, mode_context)),
           tangoTouroku pro((uiContext));

static char *e_message[] = {
#ifndef WIN
  /*0*/"さらに細かい品詞分けのための質問をしても良いですか?(y/n)",
  /*1*/"読みと候補を 終止形で入力してください。",
  /*2*/"読みと候補の 活用が違います。入力しなおしてください。",
  /*3*/"読みと候補を 終止形で入力してください。例) 早い",
  /*4*/"読みと候補を 終止形で入力してください。例) 静かだ",
  /*5*/"「",
  /*6*/"する」は正しいですか?(y/n)",
  /*7*/"な」は正しいですか?(y/n)",
  /*8*/"」は人名ですか?(y/n)",
  /*9*/"」は地名ですか?(y/n)",
  /*10*/"ない」は正しいですか?(y/n)",
  /*11*/"」は名詞として使いますか?(y/n)",
  /*12*/"」は正しいですか?(y/n)",
  /*13*/"と」は正しいですか?(y/n)",
#ifdef STANDALONE
  /*14*/"かな漢字変換できません",
#else
  /*14*/"かな漢字変換サーバと通信できません",
#endif
  /*15*/"単語登録できませんでした",
  /*16*/"『",
  /*17*/"』",
  /*18*/"（",
  /*19*/"）を登録しました",
  /*20*/"単語登録に失敗しました",
#else
  /*0*/"\244\265\244\351\244\313\272\331\244\253\244\244\311\312\273\354\312\254\244\261\244\316\244\277\244\341\244\316\274\301\314\344\244\362\244\267\244\306\244\342\316\311\244\244\244\307\244\271\244\253?(y/n)",
       /* さらに細かい品詞分けのための質問をしても良いですか */

  /*1*/"\306\311\244\337\244\310\270\365\312\344\244\362\40\275\252\273\337\267\301\244\307\306\376\316\317\244\267\244\306\244\257\244\300\244\265\244\244\241\243",
       /* 読みと候補を 終止形で入力してください。*/

  /*2*/"\306\311\244\337\244\310\270\365\312\344\244\316\40\263\350\315\321\244\254\260\343\244\244\244\336\244\271\241\243\306\376\316\317\244\267\244\312\244\252\244\267\244\306\244\257\244\300\244\265\244\244\241\243",
       /* 読みと候補の 活用が違います。入力しなおしてください。*/

  /*3*/"\306\311\244\337\244\310\270\365\312\344\244\362\40\275\252\273\337\267\301\244\307\306\376\316\317\244\267\244\306\244\257\244\300\244\265\244\244\241\243\316\343) \301\341\244\244",
       /* 読みと候補を 終止形で入力してください。例) 早い */

  /*4*/"\306\311\244\337\244\310\270\365\312\344\244\362\40\275\252\273\337\267\301\244\307\306\376\316\317\244\267\244\306\244\257\244\300\244\265\244\244\241\243\316\343) \300\305\244\253\244\300",
       /* 読みと候補を 終止形で入力してください。例) 静かだ */

  /*5*/"\241\326",  /* 「 */

  /*6*/"\244\271\244\353\241\327\244\317\300\265\244\267\244\244\244\307\244\271\244\253?(y/n)",
       /* する」は正しいですか */

  /*7*/"\244\312\241\327\244\317\300\265\244\267\244\244\244\307\244\271\244\253?(y/n)",
       /* な」は正しいですか */

  /*8*/"\241\327\244\317\277\315\314\276\244\307\244\271\244\253?(y/n)",
       /* 」は人名ですか */

  /*9*/"\241\327\244\317\303\317\314\276\244\307\244\271\244\253?(y/n)",
       /* 」は地名ですか */

  /*10*/"\244\312\244\244\241\327\244\317\300\265\244\267\244\244\244\307\244\271\244\253?(y/n)",
       /* ない」は正しいですか */

  /*11*/"\241\327\244\317\314\276\273\354\244\310\244\267\244\306\273\310\244\244\244\336\244\271\244\253?(y/n)",
       /* 」は名詞として使いますか */

  /*12*/"\241\327\244\317\300\265\244\267\244\244\244\307\244\271\244\253?(y/n)",
       /* 」は正しいですか */

  /*13*/"\244\310\241\327\244\317\300\265\244\267\244\244\244\307\244\271\244\253?(y/n)",
       /* と」は正しいですか */

#ifdef STANDALONE
  /*14*/"\244\253\244\312\264\301\273\372\312\321\264\271\244\307\244\255\244\336\244\273\244\363",
       /* かな漢字変換できません */
#else
  /*14*/"\244\253\244\312\264\301\273\372\312\321\264\271\245\265\241\274\245\320\244\310\304\314\277\256\244\307\244\255\244\336\244\273\244\363",
       /* かな漢字変換サーバと通信できません */
#endif

  /*15*/"\303\261\270\354\305\320\317\277\244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277",
       /* 単語登録できませんでした */

  /*16*/"\241\330", /* 『 */

  /*17*/"\241\331", /* 』 */

  /*18*/"\241\312", /* （ */

  /*19*/"\241\313\244\362\305\320\317\277\244\267\244\336\244\267\244\277",
       /* ）を登録しました */

  /*20*/"\303\261\270\354\305\320\317\277\244\313\274\272\307\324\244\267\244\336\244\267\244\277",
       /* 単語登録に失敗しました */
#endif
};

#define message_num (sizeof(e_message) / sizeof(char *))
static wchar_t *message[message_num];

#ifdef WIN
static char sgyouA[] = "かがさたなばまらわ";
static char sgyouI[] = "きぎしちにびみりい";
static char sgyouU[] = "くぐすつぬぶむるう";
#else
static char sgyouA[] = "\244\253\244\254\244\265\244\277\244\312\244\320\244\336\244\351\244\357";
                       /* かがさたなばまらわ */

static char sgyouI[] = "\244\255\244\256\244\267\244\301\244\313\244\323\244\337\244\352\244\244";
                       /* きぎしちにびみりい */

static char sgyouU[] = "\244\257\244\260\244\271\244\304\244\314\244\326\244\340\244\353\244\246";
                       /* くぐすつぬぶむるう */
#endif


#define KAGYOU 0
#define GAGYOU 1
#define SAGYOU 2
#define TAGYOU 3
#define NAGYOU 4
#define BAGYOU 5
#define MAGYOU 6
#define RAGYOU 7
#define WAGYOU 8

static wchar_t *gyouA;
static wchar_t *gyouI;
static wchar_t *gyouU;

/* 全てのメッセージを"unsigned char"から"wchar_t"に変換する */
int
initHinshiMessage()
{
  int i;

  for(i = 0; i < message_num; i++) {
    message[i] = WString(e_message[i]);
    if(!message[i]) {
      return(-1);
    }
  }
  return 0;
}

/* WSprintf(to_buf, x1, x2, from_buf)
   :WSprintf(to_buf,"x1%sx2",from_buf);
 */
static void
WSprintf(to_buf, x1, x2, from_buf)
wchar_t *to_buf, *x1, *x2, *from_buf;
{
    WStrcpy(to_buf, x1);
    WStrcat(to_buf, from_buf);
    WStrcat(to_buf, x2);
}
#endif /* NO_EXTEND_MENU */

#ifndef WIN
void
EWStrcat(buf, xxxx)
wchar_t *buf;
char *xxxx;
{
  wchar_t x[1024];

  MBstowcs(x, xxxx, 1024);
  WStrcat(buf, x);
}
#endif

#ifndef NO_EXTEND_MENU
static void
EWStrcpy(buf, xxxx)
wchar_t *buf;
char *xxxx;
{
  wchar_t x[1024];
  int len;

  len = MBstowcs(x, xxxx, 1024);
  WStrncpy(buf, x, len);
  buf[len] = (wchar_t)0;
}

static int
EWStrcmp(buf, xxxx)
wchar_t *buf;
char *xxxx;
{
  wchar_t x[1024];

  MBstowcs(x, xxxx, 1024);
  return(WStrncmp(buf, x, WStrlen(x)));
}

static int
EWStrncmp(buf, xxxx, len)
wchar_t *buf;
char *xxxx;
int len;
/* ARGSUSED */
{
  wchar_t x[1024];

  MBstowcs(x, xxxx, 1024);
  return(WStrncmp(buf, x, WStrlen(x)));
}

int
initGyouTable()
{
  gyouA = WString(sgyouA);
  gyouI = WString(sgyouI);
  gyouU = WString(sgyouU);

  if (!gyouA || !gyouI || !gyouU) {
    return NG;
  }
  return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 単語登録の品詞選択 〜Yes/No 共通 Quit〜                                   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
uuTHinshiYNQuitCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  popCallback(d);
  
  return(dicTourokuHinshi(d));
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 単語登録の品詞選択 〜Yes/No 第２段階 共通コールバック〜                   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
uuTHinshi2YesCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  tourokuContext tc;

  popCallback(d); /* yesNo をポップ */

  tourokuYes(d);   /* 品詞が決まれば tc->hcode にセットする */

  tc = (tourokuContext)d->modec;

  if (!tc->qbuf[0]) {
    if (tc->hcode[0]) {
      /* 品詞が決まったので、登録するユーザ辞書の指定を行う */
      return(dicTourokuDictionary(d, uuTDicExitCatch, uuTDicQuitCatch));
    }
  }
  return(retval);
}

static
uuTHinshi2NoCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  tourokuContext tc;

  popCallback(d); /* yesNo をポップ */

  tourokuNo(d);   /* 品詞が決まれば tc->hcode にセットする */

  tc = (tourokuContext)d->modec;

  if (!tc->qbuf[0]) {
    if (tc->hcode[0]) {
      /* 品詞が決まったので、登録するユーザ辞書の指定を行う */
      return(dicTourokuDictionary(d, uuTDicExitCatch, uuTDicQuitCatch));
    }
  }

  return(retval);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 単語登録の品詞選択 〜Yes/No 第１段階 コールバック〜                       *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
uuTHinshi1YesCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  tourokuContext tc;
  coreContext ync;
  
  popCallback(d); /* yesNo をポップ */

  tourokuYes(d);   /* 品詞が決まれば tc->hcode にセットする */

  tc = (tourokuContext)d->modec;

  if(tc->qbuf[0]) {
    /* 質問する */
    makeGLineMessage(d, tc->qbuf, WStrlen(tc->qbuf));
    if((retval = getYesNoContext(d,
		 NO_CALLBACK, uuTHinshi2YesCatch,
		 uuTHinshiYNQuitCatch, uuTHinshi2NoCatch)) == NG) {
      defineEnd(d);
      return(GLineNGReturnTK(d));
    }
    ync = (coreContext)d->modec;
    ync->majorMode = CANNA_MODE_ExtendMode;
    ync->minorMode = CANNA_MODE_TourokuHinshiMode;
  } else if(tc->hcode[0]) {
    /* 品詞が決まったので、登録するユーザ辞書の指定を行う */
    return(dicTourokuDictionary(d, uuTDicExitCatch, uuTDicQuitCatch));
  }

  return(retval);
}

static
uuTHinshi1NoCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  tourokuContext tc;
  coreContext ync;

  popCallback(d); /* yesNo をポップ */

  tourokuNo(d);   /* 品詞が決まれば tc->hcode にセットする */

  tc = (tourokuContext)d->modec;

  if(tc->qbuf[0]) {
    /* 質問する */
    makeGLineMessage(d, tc->qbuf, WStrlen(tc->qbuf));
    if((retval = getYesNoContext(d,
		 NO_CALLBACK, uuTHinshi2YesCatch,
		 uuTHinshiYNQuitCatch, uuTHinshi2NoCatch)) == NG) {
      defineEnd(d); 
      return(GLineNGReturnTK(d));
    }
    ync = (coreContext)d->modec;
    ync->majorMode = CANNA_MODE_ExtendMode;
    ync->minorMode = CANNA_MODE_TourokuHinshiMode;
  } else if(tc->hcode[0]) {
    /* 品詞が決まったので、登録するユーザ辞書の指定を行う */
    return(dicTourokuDictionary(d, uuTDicExitCatch, uuTDicQuitCatch));
  }

  return(retval);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 単語登録の品詞分けする？                                                  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
uuTHinshiQYesCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  tourokuContext tc;
  coreContext ync;

  popCallback(d); /* yesNo をポップ */

  tc = (tourokuContext)d->modec;

  makeGLineMessage(d, tc->qbuf, WStrlen(tc->qbuf)); /* 質問 */
  if((retval = getYesNoContext(d,
	 NO_CALLBACK, uuTHinshi1YesCatch,
	 uuTHinshiYNQuitCatch, uuTHinshi1NoCatch)) == NG) {
    defineEnd(d);
    return(GLineNGReturnTK(d));
  }
  ync = (coreContext)d->modec;
  ync->majorMode = CANNA_MODE_ExtendMode;
  ync->minorMode = CANNA_MODE_TourokuHinshiMode;

  return(retval);
}

static
uuTHinshiQNoCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  popCallback(d); /* yesNo をポップ */

  return(dicTourokuDictionary(d, uuTDicExitCatch, uuTDicQuitCatch));
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 単語登録の品詞選択                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static int makeHinshi();

dicTourokuHinshiDelivery(d)
uiContext	d;
{
  tourokuContext tc = (tourokuContext)d->modec;
  coreContext ync;
  int retval = 0;

  makeHinshi(d); /* 品詞、エラーメッセージ、質問をセットしてくる */

#if defined(DEBUG) && !defined(WIN)
  if(iroha_debug) {
    printf("tc->genbuf=%s, tc->qbuf=%s, tc->hcode=%s\n", tc->genbuf, tc->qbuf,
	   tc->hcode);
  }
#endif
  if(tc->genbuf[0]) {
    /* 入力されたデータに誤りがあったので、
       メッセージを表示して読み入力に戻る */
    clearYomi(d);
    return(dicTourokuTango(d, uuTTangoQuitCatch));
  } else if(tc->qbuf[0] && cannaconf.grammaticalQuestion) {
    /* 細かい品詞分けのための質問をする */
    WStrcpy(d->genbuf, message[0]);
    if((retval = getYesNoContext(d,
		 NO_CALLBACK, uuTHinshiQYesCatch,
		 uuTHinshiYNQuitCatch, uuTHinshiQNoCatch)) == NG) {
      defineEnd(d);
      return(GLineNGReturnTK(d));
    }
    makeGLineMessage(d, d->genbuf, WStrlen(d->genbuf));
    ync = (coreContext)d->modec;
    ync->majorMode = CANNA_MODE_ExtendMode;
    ync->minorMode = CANNA_MODE_TourokuHinshiMode;
    return(retval);
  } else if(tc->hcode[0]) {
    /* 品詞が決まったので、登録するユーザ辞書の指定を行う */
    return(dicTourokuDictionary(d, uuTDicExitCatch, uuTDicQuitCatch));
  }
  return 0;
}

/*
 * 選択された品詞から次の動作を行う
 * 
 * tc->hcode	品詞
 * tc->qbuf	質問
 * tc->genbuf	エラー
 */
static int
makeHinshi(d)
uiContext	d;
{
  tourokuContext tc = (tourokuContext)d->modec;
  int tlen, ylen, yomi_katsuyou;
  wchar_t tmpbuf[256];

  tc->hcode[0] = 0;
  tc->qbuf[0] = 0;
  tc->genbuf[0] = 0;

  tlen = tc->tango_len;
  ylen = tc->yomi_len;

  switch(tc->curHinshi) {
  case MEISHI:
    EWStrcpy(tc->hcode, "#T35");
    tc->katsuyou = 0;
    WSprintf(tc->qbuf, message[5], message[6], tc->tango_buffer);
    break;

  case KOYUMEISHI:
    EWStrcpy(tc->hcode, "#KK");
    WSprintf(tc->qbuf, message[5], message[8], tc->tango_buffer);
    break;
    
  case DOSHI:

    /* 入力が終止形か？ */
    tc->katsuyou = 0;
    while (tc->katsuyou < GOBISUU &&
	   tc->tango_buffer[tlen - 1] != gyouU[tc->katsuyou]) {
      tc->katsuyou++;
    }
    yomi_katsuyou = 0;
    while (yomi_katsuyou < GOBISUU &&
	   tc->yomi_buffer[ylen - 1] != gyouU[yomi_katsuyou]) {
      yomi_katsuyou++;
    }
    if((tc->katsuyou == GOBISUU) || (yomi_katsuyou == GOBISUU)){
      WStrcpy(tc->genbuf, message[1]);
      return(0);
    }
    if(tc->katsuyou != yomi_katsuyou){
      WStrcpy(tc->genbuf, message[2]);
      return(0);
    }

    makeDoushi(d);  /* 詳細の品詞を必要としない場合 */
    if (tc->katsuyou == RAGYOU) {
      tc->curHinshi = RAGYODOSHI;
      /* 未然形をつくる */
      WStrncpy(tmpbuf, tc->tango_buffer, tlen-1);  
      tmpbuf[tlen - 1] = gyouA[tc->katsuyou];
      tmpbuf[tlen] = (wchar_t)0;
      WSprintf(tc->qbuf, message[5], message[10], tmpbuf);
    }
    else {
      tc->curHinshi = GODAN;
      WStrncpy(tmpbuf, tc->tango_buffer, tlen - 1);
      tmpbuf[tlen - 1] = gyouI[tc->katsuyou];
      tmpbuf[tlen] = (wchar_t)'\0';
      WSprintf(tc->qbuf, message[5], message[11], tmpbuf);
    }
    break;

  case KEIYOSHI:
    tc->katsuyou = 1;
    if(tlen >= 1 && ylen >= 1 &&
       ((EWStrncmp(tc->tango_buffer+tlen-1, "\244\244", 1) != 0) ||
	(EWStrncmp(tc->yomi_buffer+ylen-1, "\244\244", 1) != 0))) {
                                           /* い */
      WStrcpy(tc->genbuf, message[3]);
      return(0);
    }

    EWStrcpy(tc->hcode, "#KY"); /* 詳細の品詞を必要としない場合 */
    WStrncpy(tmpbuf, tc->tango_buffer, tlen-1);  
    tmpbuf[tlen-1] = 0;
    WSprintf(tc->qbuf, message[5], message[11], tmpbuf);
    break;

  case KEIYODOSHI:
    tc->katsuyou = 1;
    if(tlen >= 1 && ylen >= 1 &&
       ((EWStrncmp(tc->tango_buffer+tlen-1, "\244\300", 1)) ||
	(EWStrncmp(tc->yomi_buffer+ylen-1, "\244\300", 1)))) {
                                           /* だ */
      WStrcpy(tc->genbuf, message[4]);
      return(0);
    }
    EWStrcpy(tc->hcode, "#T05"); /* 詳細の品詞を必要としない場合 */
    WStrncpy(tmpbuf, tc->tango_buffer, tlen-1);  
    tmpbuf[tlen-1] = 0;  
    WSprintf(tc->qbuf, message[5], message[6], tmpbuf);
    break;

  case FUKUSHI:
    EWStrcpy(tc->hcode, "#F14"); /* 詳細の品詞を必要としない場合 */
    tc->katsuyou = 0;
    WSprintf(tc->qbuf, message[5], message[6], tc->tango_buffer);
    break;

  case TANKANJI:
    EWStrcpy(tc->hcode, "#KJ");
    break;

  case SUSHI:
    EWStrcpy(tc->hcode, "#NN");
    break;

  case RENTAISHI:
    EWStrcpy(tc->hcode, "#RT");
    break;

  case SETSUZOKUSHI:  /* 接続詞・感動詞 */
    EWStrcpy(tc->hcode, "#CJ");
    break;

  case SAHENMEISHI:
  case MEISHIN:
    tc->katsuyou = 0;
    WSprintf(tc->qbuf, message[5], message[7], tc->tango_buffer);
    break;

  case JINMEI:
  case KOYUMEISHIN:
    WSprintf(tc->qbuf, message[5], message[9], tc->tango_buffer);
    break;

  case RAGYOGODAN:
    WStrncpy(tmpbuf, tc->tango_buffer, tlen - 1);
    tmpbuf[tlen - 1] = gyouI[tc->katsuyou];
    tmpbuf[tlen] = (wchar_t)'\0';
    WSprintf(tc->qbuf, message[5], message[11], tmpbuf);
    break;

  case KAMISHIMO:
    WStrncpy(tmpbuf, tc->tango_buffer, tlen - 1);
    tmpbuf[tlen - 1] = (wchar_t)'\0';
    WSprintf(tc->qbuf, message[5], message[11], tmpbuf);
    break;

  case KEIYODOSHIY:
  case KEIYODOSHIN: 
    WStrncpy(tmpbuf, tc->tango_buffer, tlen - 1);
    tmpbuf[tlen - 1] = 0;
    WSprintf(tc->qbuf, message[5], message[11], tmpbuf);
    break;

  case FUKUSHIY:
  case FUKUSHIN:
    WSprintf(tc->qbuf, message[5], message[13], tc->tango_buffer);
    break;
  }

  return(0);
}

static
tourokuYes(d)
uiContext	d;
{
  tourokuContext tc = (tourokuContext)d->modec;

  tc->hcode[0] = 0;
  tc->qbuf[0] = 0;
  tc->genbuf[0] = 0;

  switch(tc->curHinshi) {
  case MEISHI:
    tc->curHinshi = SAHENMEISHI;
    makeHinshi(d);
    break;

  case KOYUMEISHI:
    tc->curHinshi = JINMEI;
    makeHinshi(d);
    break;

  case GODAN:  /* ラ行以外の五段活用動詞 */
    makeDoushi(d);
    EWStrcat(tc->hcode, "r");              /* 書く、急ぐ、移す */
    break;

  case RAGYODOSHI:
    tc->curHinshi = RAGYOGODAN;
    makeHinshi(d);
    break;

  case KEIYOSHI:
    EWStrcpy(tc->hcode, "#KYT");           /* きいろい */
    break;

  case KEIYODOSHI:
    tc->curHinshi = KEIYODOSHIY;
    makeHinshi(d);
    break;

  case FUKUSHI:
    tc->curHinshi = FUKUSHIY;
    makeHinshi(d);
    break;

  case MEISHIN:
    EWStrcpy(tc->hcode, "#T15");          /* 色々、強力 */
    break;

  case SAHENMEISHI:
    EWStrcpy(tc->hcode, "#T10");          /* 安心、浮気 */
    break;

  case KOYUMEISHIN:
    EWStrcpy(tc->hcode, "#CN");	          /* 東京 */
    break;

  case JINMEI:
    EWStrcpy(tc->hcode, "#JCN");          /* 福島 */
    break;

  case RAGYOGODAN:
    EWStrcpy(tc->hcode, "#R5r");          /* 謝る */
    break;

  case KAMISHIMO:
    EWStrcpy(tc->hcode, "#KSr");          /* 生きる、預ける */
    break;

  case KEIYODOSHIY:
    EWStrcpy(tc->hcode, "#T10");          /* 関心だ */
    break;

  case KEIYODOSHIN:
    EWStrcpy(tc->hcode, "#T15");          /* 意外だ、可能だ */
    break;

  case FUKUSHIY:
    EWStrcpy(tc->hcode, "#F04");          /* ふっくら */
    break;

  case FUKUSHIN:
    EWStrcpy(tc->hcode, "#F06");          /* 突然 */
    break;
  }

  return(0);
}

static
tourokuNo(d)
uiContext	d;
{
  tourokuContext tc = (tourokuContext)d->modec;
  int ylen;

  tc->hcode[0] = 0;
  tc->qbuf[0] = 0;
  tc->genbuf[0] = 0;

  switch( tc->curHinshi ) {
  case MEISHI:
    tc->curHinshi = MEISHIN;
    makeHinshi(d);
    break;

  case KOYUMEISHI:
    tc->curHinshi = KOYUMEISHIN;
    makeHinshi(d);
    break;

  case GODAN:  /* ラ行以外の五段活用動詞 */
    makeDoushi(d);
    break;

  case RAGYODOSHI:
    ylen = tc->yomi_len;
    if (ylen >= 2 && !(EWStrcmp(tc->yomi_buffer + ylen - 2, "\244\257\244\353"))) {   /* くる */
      EWStrcpy(tc->hcode, "#KX");         /* 来る */
    }
    else if (ylen >=2 && !(EWStrcmp(tc->yomi_buffer + ylen - 2, "\244\271\244\353"))) { /* する */
      EWStrcpy(tc->hcode, "#SX");         /* する */
    }
    else if (ylen >=2 && !(EWStrcmp(tc->yomi_buffer + ylen - 2, "\244\272\244\353"))) {  /* ずる */
      EWStrcpy(tc->hcode, "#ZX");         /* 準ずる */
    }
    else {
      tc->curHinshi = KAMISHIMO;
      makeHinshi(d);
    }
    break;

  case KEIYOSHI:
    EWStrcpy(tc->hcode, "#KY");           /* 美しい、早い */
    break;

  case KEIYODOSHI:
    tc->curHinshi = KEIYODOSHIN;
    makeHinshi(d);
    break;

  case FUKUSHI:
    tc->curHinshi = FUKUSHIN;
    makeHinshi(d);
    break;

  case MEISHIN:
    EWStrcpy(tc->hcode, "#T35");          /* 山、水 */
    break;

  case SAHENMEISHI:
    EWStrcpy(tc->hcode, "#T30");          /* 努力、検査 */
    break;

  case KOYUMEISHIN:
    EWStrcpy(tc->hcode, "#KK");           /* 日本電気 */
    break;

  case JINMEI:
    EWStrcpy(tc->hcode, "#JN");           /* 三竿 */
    break;

  case RAGYOGODAN:
    EWStrcpy(tc->hcode, "#R5");           /* 威張る */
    break;

  case KAMISHIMO:
    EWStrcpy(tc->hcode, "#KS");           /* 降りる、与える */
    break;

  case KEIYODOSHIY:
    EWStrcpy(tc->hcode, "#T13");          /* 多慌てだ */
    break;

  case KEIYODOSHIN:
    EWStrcpy(tc->hcode, "#T18");          /* 便利だ、静かだ */
    break;

  case FUKUSHIY:
    EWStrcpy(tc->hcode, "#F12");          /* そっと */
    break;

  case FUKUSHIN:
    EWStrcpy(tc->hcode, "#F14");          /* 飽くまで */
    break;
  }
  return(0);
}

static
makeDoushi(d)
uiContext	d;
{
  tourokuContext tc = (tourokuContext)d->modec;

    switch(tc->katsuyou){
    case  KAGYOU:
      EWStrcpy( tc->hcode, "#K5" );     /* 置く */
      break;
    case  GAGYOU:
      EWStrcpy( tc->hcode, "#G5" );     /* 仰ぐ */
      break;
    case  SAGYOU:
      EWStrcpy( tc->hcode, "#S5" );     /* 返す */
      break;
    case  TAGYOU:
      EWStrcpy( tc->hcode, "#T5" );     /* 絶つ */
      break;
    case  NAGYOU:
      EWStrcpy( tc->hcode, "#N5" );     /* 死ぬ */
      break;
    case  BAGYOU:
      EWStrcpy( tc->hcode, "#B5" );     /* 転ぶ */
      break;
    case  MAGYOU:
      EWStrcpy( tc->hcode, "#M5" );     /* 住む */
      break;
    case  RAGYOU:
      EWStrcpy( tc->hcode, "#R5" );     /* 威張る */
      break;
    case  WAGYOU:
      EWStrcpy( tc->hcode, "#W5" );     /* 言う */
      break;
    }
}    

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 辞書の一覧                                                                *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
uuTDicExitCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  forichiranContext fc;
  int cur;
  tourokuContext tc;

  d->nbytes = 0;

  popCallback(d); /* 一覧を pop */

  fc = (forichiranContext)d->modec;
  cur = fc->curIkouho;

  popForIchiranMode(d);
  popCallback(d);

  tc = (tourokuContext)d->modec;

  tc->workDic = cur;

  return(tangoTouroku(d));
}

static
uuTDicQuitCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  popCallback(d); /* 一覧を pop */

  popForIchiranMode(d);
  popCallback(d);

  return(dicTourokuHinshi(d));
}

dicTourokuDictionary(d, exitfunc, quitfunc)
uiContext d;
int (*exitfunc)();
int (*quitfunc)();
{
  tourokuContext tc = (tourokuContext)d->modec;
  forichiranContext fc;
  ichiranContext ic;
  wchar_t **work;
  unsigned inhibit = 0;
  int retval, upnelem = 0;

  retval = d->nbytes = 0;
  d->status = 0;

  for(work = tc->udic; *work; work++)
    upnelem++;

  if((retval = getForIchiranContext(d)) == NG) {
    freeDic(tc);
    defineEnd(d);
    return(GLineNGReturnTK(d));
  }
  fc = (forichiranContext)d->modec;

  /* selectOne を呼ぶための準備 */

  fc->allkouho = tc->udic;

  fc->curIkouho = 0;
  if (!cannaconf.HexkeySelect)
    inhibit |= ((unsigned char)NUMBERING | (unsigned char)CHARINSERT); 
  else
    inhibit |= (unsigned char)CHARINSERT;

   if((retval = selectOne(d, fc->allkouho, &fc->curIkouho, upnelem,
		 BANGOMAX, inhibit, 0, WITHOUT_LIST_CALLBACK,
		 NO_CALLBACK, exitfunc, quitfunc, uiUtilIchiranTooSmall)) 
                 == NG) {
    if(fc->allkouho)
      free(fc->allkouho);
    popForIchiranMode(d);
    popCallback(d);
    defineEnd(d);
    return(GLineNGReturnTK(d));
  }

  ic = (ichiranContext)d->modec;
  ic->majorMode = CANNA_MODE_ExtendMode;
  ic->minorMode = CANNA_MODE_TourokuDicMode;
  currentModeInfo(d);

  /* 候補一覧行が狭くて候補一覧が出せない */
  if(ic->tooSmall) {
    d->status = AUX_CALLBACK;
    return(retval);
  }

  makeGlineStatus(d);
  /* d->status = ICHIRAN_EVERYTIME; */

  return(retval);
}

/*
 * 単語登録を行う
 */
static
tangoTouroku(d)
uiContext	d;
{
  tourokuContext tc = (tourokuContext)d->modec;
  wchar_t ktmpbuf[256];
  wchar_t ttmpbuf[256];
  wchar_t line[ROMEBUFSIZE], line2[ROMEBUFSIZE];
  wchar_t xxxx[1024];
  char dicname[1024];
  extern int defaultContext;
  int linecnt;
  wchar_t *WStraddbcpy();

  defineEnd(d);
  if(tc->katsuyou || (EWStrncmp(tc->hcode, "#K5", 3) == 0)) {
    WStrncpy(ttmpbuf, tc->tango_buffer, tc->tango_len - 1);
    ttmpbuf[tc->tango_len - 1] = (wchar_t)0;
    WStrncpy(ktmpbuf, tc->yomi_buffer, tc->yomi_len - 1);
    ktmpbuf[tc->yomi_len - 1] = 0;
  } else {
    WStrcpy(ttmpbuf, tc->tango_buffer);
    WStrcpy(ktmpbuf, tc->yomi_buffer);
  }

  /* 辞書書き込み用の一行を作る */
  WStraddbcpy(line, ktmpbuf, ROMEBUFSIZE);
  linecnt = WStrlen(line);
  line[linecnt] = (wchar_t)' ';
  linecnt++;
  WStrcpy(line + linecnt, tc->hcode);
  linecnt += WStrlen(tc->hcode);
  line[linecnt] = (wchar_t)' ';
  linecnt++;
  WStraddbcpy(line + linecnt, ttmpbuf, ROMEBUFSIZE - linecnt);

  if(defaultContext == -1) {
    if((KanjiInit() < 0) || (defaultContext == -1)) {
      jrKanjiError = (char *)e_message[14];
      freeAndPopTouroku(d);
      return(GLineNGReturn(d));
    }
  }
  /* 辞書に登録する */
  WCstombs(dicname, tc->udic[tc->workDic], sizeof(dicname));

  if (RkwDefineDic(defaultContext, dicname, line) != 0) {
    /* 品詞が #JCN のときは、登録に失敗したら、#JN と #CN で登録する */
    if (EWStrncmp(tc->hcode, "#JCN", 4) == 0) {
      wchar_t xxx[3];

      /* まず #JN で登録する */
      EWStrcpy(xxx, "#JN");
      WStraddbcpy(line, ktmpbuf, ROMEBUFSIZE);
      EWStrcat(line, " ");
      WStrcat(line, xxx);
      EWStrcat(line, " ");
      linecnt = WStrlen(line);
      WStraddbcpy(line + linecnt, ttmpbuf, ROMEBUFSIZE - linecnt);

      if (RkwDefineDic(defaultContext, dicname, line) == 0) {
        /* #JN で登録できたとき、次に #CN で登録する */
        EWStrcpy(xxx, "#CN");
        WStraddbcpy(line2, ktmpbuf, ROMEBUFSIZE);
        EWStrcat(line2, " ");
        WStrcat(line2, xxx);
        EWStrcat(line2, " ");
        linecnt = WStrlen(line2);
        WStraddbcpy(line2 + linecnt, ttmpbuf, ROMEBUFSIZE - linecnt);

        if (RkwDefineDic(defaultContext, dicname, line2) == 0) {
          goto success;
        }

        /* #CN で登録できなかったとき、#JN を削除する */
        if (RkwDeleteDic(defaultContext, dicname, line) == NG) {
          /* #JN が削除できなかったら、"失敗しました" */
          if (errno == EPIPE)
            jrKanjiPipeError();
          WStrcpy(d->genbuf, message[20]);
          goto close;
        }
      }
    }
    /* #JCN 以外のとき
       #JN が登録できなかったとき
       #CN が登録できず、#JN が削除できたとき */
    if (errno == EPIPE)
      jrKanjiPipeError();
    WStrcpy(d->genbuf, message[15]);
    goto close;
  }

 success:
  if (cannaconf.auto_sync) {
    RkwSync(defaultContext, dicname);
  }
  /* 登録の完了を表示する */
  WSprintf(d->genbuf, message[16], message[17], tc->tango_buffer);
  WSprintf(xxxx, message[18], message[19], tc->yomi_buffer);
  WStrcat(d->genbuf, xxxx);

 close:
  makeGLineMessage(d, d->genbuf, WStrlen(d->genbuf));

  freeAndPopTouroku(d);
  currentModeInfo(d);

  return(0); /* 単語登録完了 */
}
#endif /* NO_EXTEND_MENU */
