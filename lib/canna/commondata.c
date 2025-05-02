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
static char rcs_id[] = "@(#) 102.1 $Id: commondata.c,v 1.4.2.1 2004/04/26 22:53:02 aida_s Exp $";
#endif /* lint */

#include "canna.h"
#include <canna/mfdef.h>
#include "patchlevel.h"

struct CannaConfig cannaconf;
     
/* デフォルトのローマ字かな変換用のバッファ */

int defaultContext = -1;
int defaultBushuContext = -1;

/* ローマ字かな変換テーブル */
/*
 * ローマ字かな変換テーブルは１個あれば良いでしょう。複数個必要なので
 * あれば RomeStruct のメンバとして入れておく必要もありましょうが...そ
 * の時はその時で考えましょう。
 */
     
struct RkRxDic *romajidic, *englishdic, *RkwOpenRoma();

/* 未定義キー打鍵時の処理のしかた */

int howToBehaveInCaseOfUndefKey = kc_normal;

/*
 * 辞書の名前を入れておく変数
 */

char saveapname[CANNA_MAXAPPNAME]; /* サーバとの接続を切るときのAP名 */

/*
 * irohacheck コマンドによって使われているかとか、
 * irohacheck ないでの verbose を表す値。
 */

int ckverbose = 0;

/*
 * エラーのメッセージを入れておく変数
 */

char *jrKanjiError = "";

/*
 * デバグメッセージを出すかどうかのフラグ
 */

int iroha_debug = 0;

/*
 * 始めての使用かどうかを示すフラグ
 */

int FirstTime = 1;

/*
 * ビープ音を鳴らす関数を格納するところ
 */

int (*jrBeepFunc)() = (int (*)())NULL;

/*
 * KC_INITIALIZE 直後に実行する機能の列
 */

BYTE *initfunc = (BYTE *)0;
int howToReturnModeInfo = ModeInfoStyleIsString;
char *RomkanaTable = (char *)NULL;
char *EnglishTable = (char *)NULL;
/* char *Dictionary = (char *)NULL; */
struct dicname *RengoGakushu = (struct dicname *)NULL;
struct dicname *KatakanaGakushu = (struct dicname *)NULL;
struct dicname *HiraganaGakushu = (struct dicname *)NULL;

int nKouhoBunsetsu = 16;

int KeepCursorPosition = 0;

int nothermodes = 0;

keySupplement keysup[MAX_KEY_SUP];
int nkeysup = 0;

/*
 * 初期化の際使用した初期化ファイル名を全てとっておくバッファ。
 * ファイル名は","で区切られる。(拡張機能で使用)
 */

char *CANNA_initfilename = (char *)NULL;

/*
 * バージョン
 */

int protocol_version = -1;
int server_version = -1;
char *server_name = (char *)NULL;

int chikuji_debug = 0;
int auto_define = 0;

void (*keyconvCallback)() = (void (*)())0;

extraFunc *extrafuncp = (extraFunc *)NULL;
struct dicname *kanjidicnames; /* .canna で指定している辞書リスト */
char *kataautodic = (char *)NULL; /* カタカナ語自動登録用辞書 */
#ifdef HIRAGANAAUTO
char *hiraautodic = (char *)NULL; /* ひらがな語自動登録用辞書 */
#endif

/* ユーザ情報 */
jrUserInfoStruct *uinfo = (jrUserInfoStruct *)NULL;

 /* マウント処理を行っているかどうか */
int mountnottry = 1;

void
InitCannaConfig(cf)
struct CannaConfig *cf;
{
  bzero(cf, sizeof(struct CannaConfig));
  cf->CannaVersion = CANNA_MAJOR_MINOR;
  cf->kouho_threshold = 2;
  cf->strokelimit = STROKE_LIMIT;
  cf->CursorWrap = 1;
  cf->SelectDirect = 1;
  cf->HexkeySelect = 1;
  cf->ChBasedMove = 1;
  cf->Gakushu = 1;
  cf->grammaticalQuestion = 1;
  cf->stayAfterValidate = 1;
  cf->kCount = 1;
  cf->hexCharacterDefiningStyle = HEX_USUAL;
  cf->ChikujiContinue = 1;
  cf->RenbunContinue = 1;
  cf->MojishuContinue = 1;
  cf->kojin = 1;
  cf->indexSeparator = DEFAULTINDEXSEPARATOR;
  cf->allowNextInput = 1;
  cf->chikujiRealBackspace = 1;
  cf->BackspaceBehavesAsQuit = 1;
  cf->doKatakanaGakushu = 1;
  cf->doHiraganaGakushu = 1;
  cf->auto_sync = 1;
}

static void freeUInfo pro((void));

static void
freeUInfo()
{
  if (uinfo) {
    if (uinfo->uname)
      free(uinfo->uname);
    if (uinfo->gname)
      free(uinfo->gname);
    if (uinfo->srvname)
      free(uinfo->srvname);
    if (uinfo->topdir)
      free(uinfo->topdir);
    if (uinfo->cannafile)
      free(uinfo->cannafile);
    if (uinfo->romkanatable)
      free(uinfo->romkanatable);
    free((char *)uinfo);
    uinfo = (jrUserInfoStruct *)NULL;
  }
}

/*
  デファールト値にもどす。
*/
void
restoreBindings()
{
  InitCannaConfig(&cannaconf);

  if (initfunc) free(initfunc);
  initfunc = (BYTE *)NULL;

  if (server_name) free(server_name);
  server_name = (char *)NULL;

  if (RomkanaTable) {
    free(RomkanaTable);
    RomkanaTable = (char *)NULL;
  }
  if (EnglishTable) {
    free(EnglishTable);
    EnglishTable = (char *)NULL;
  }
  romajidic = (struct RkRxDic *)NULL;
  englishdic = (struct RkRxDic *)NULL;
  RengoGakushu = (struct dicname *)NULL;
  KatakanaGakushu = (struct dicname *)NULL;
  HiraganaGakushu = (struct dicname *)NULL;
  howToBehaveInCaseOfUndefKey = kc_normal;
/*  kanjidicname[nkanjidics = 0] = (char *)NULL; 代わりのことをしなければ */
  kanjidicnames = (struct dicname *)NULL;
  kataautodic = (char *)NULL;
#ifdef HIRAGANAAUTO
  hiraautodic = (char *)NULL;
#endif
  auto_define = 0;
  saveapname[0] = '\0';
  KeepCursorPosition = 0;

  nothermodes = 0;
  protocol_version = server_version = -1;
  nKouhoBunsetsu = 16;
  nkeysup = 0;
  chikuji_debug = 0;
  keyconvCallback = (void (*)())0;
  freeUInfo();
}
