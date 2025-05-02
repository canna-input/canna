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
static	char	rcs_id[] = "@(#) 102.1 $Id: henkan.c,v 1.1.1.1 2002/10/19 08:27:49 aida_s Exp $";
#endif /* lint */

#include	"canna.h"

#include	<errno.h>
#include	<fcntl.h>
#ifdef MEASURE_TIME
#include <sys/types.h>
#ifdef WIN
#include <sys/timeb.h>
#else
/* If you compile with Visual C++ on WIN, then please comment out next line. */
#include <sys/times.h>
#endif
#endif

#ifdef luna88k
extern int errno;
#endif

extern int defaultBushuContext;
extern int yomiInfoLevel;
extern int ckverbose;
extern int defaultContext;
extern struct dicname *RengoGakushu, *KatakanaGakushu, *HiraganaGakushu;
extern KanjiModeRec cy_mode, cb_mode, yomi_mode, tankouho_mode, empty_mode;
extern char saveapname[];
extern exp(int) RkwGetServerVersion pro((int *, int *));

#define DICERRORMESGLEN 78

static int doYomiHenkan();

static char dictmp[DICERRORMESGLEN];
static char *mountErrorMessage = "\244\362\245\336\245\246\245\363\245\310"
	"\244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277";
                                 /* をマウントできませんでした */

static int
kanakanError(d)
uiContext d;
{
  return makeRkError(d, "\244\253\244\312\264\301\273\372\312\321\264\271"
	"\244\313\274\272\307\324\244\267\244\336\244\267\244\277");
                        /* かな漢字変換に失敗しました */
}

static void
dicMesg(s, d)
char *s, *d;
{
#ifndef WIN
  if (ckverbose == CANNA_FULL_VERBOSE) {
    char buf[128];
    sprintf(buf, "\"%s\"", d);
    printf("%14s %-20s を指定しています。\n", s, buf);
  }
#endif
}

static void
RkwInitError()
{
  if (errno == EPIPE) {
    jrKanjiError = KanjiInitError();
  }
  else {
    jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\274\255"
	"\275\361\244\316\275\351\264\374\262\275\244\313\274\272\307\324"
	"\244\267\244\336\244\267\244\277";
                   /* かな漢字変換辞書の初期化に失敗しました */
  }
  addWarningMesg(jrKanjiError);
  RkwFinalize();
}

static void
mountError(dic)
char *dic;
{
  int mnterrlen;
  if (DICERRORMESGLEN < 
      (unsigned)(strlen(dic) + (mnterrlen = strlen(mountErrorMessage)) + 1)) {
    (void)strncpy(dictmp, dic, DICERRORMESGLEN - mnterrlen - 3/* ... */ - 1);
    (void)strcpy(dictmp + DICERRORMESGLEN - mnterrlen - 3 - 1, "...");
    strcpy(dictmp + DICERRORMESGLEN - mnterrlen - 1, mountErrorMessage);
  }
  else {
    sprintf(dictmp, "%s%s", dic, mountErrorMessage);
  }
  jrKanjiError = dictmp;
  addWarningMesg(dictmp);
}

static void
autodicError()
{
#ifndef WIN
  jrKanjiError = "自動登録用辞書が存在しません";
#else
  jrKanjiError = "\274\253\306\260\305\320\317\277\315\321\274\255\275\361"
                 "\244\254\302\270\272\337\244\267\244\336\244\273\244\363";
#endif
  addWarningMesg(jrKanjiError);
}

#ifdef WIN

static char *
FindLogname(void)
{
  extern jrUserInfoStruct *uinfo;

  if (uinfo)
    return uinfo->uname;
  return (char *)NULL;
}

static char *
FindGroupname(void)
{
  extern jrUserInfoStruct *uinfo;

  if (uinfo)
    return uinfo->gname;
  return (char *)NULL;
}

#define DDPATH              "canna"
#define DDUSER              "user"
#define DDGROUP             "group"

static int
RkwSetDicPathTmp(int Context, char *dirname)
{
  char *uname, *gname;
  int ret = -1;
#ifndef WIN
  char dichome[256];
#else
  char *dichome = malloc(256);
  if (!dichome) {
    return ret;
  }
#endif

  uname = FindLogname();
  gname = FindGroupname();

  if (uname && uname[0]) {
    if (gname && gname[0]) {
      sprintf(dichome, "%s/%s:%s/%s:%s",
                DDUSER, uname, DDGROUP, gname, DDPATH);
    }
    else {
      sprintf(dichome, "%s/%s:%s",
                DDUSER, uname, DDPATH);
    }
  }
  else {
    strcpy(dichome, DDPATH);
  }
  ret = RkwSetDicPath(Context, dichome);
#ifdef WIN
  (void)free(dichome);
#endif
  return ret;
}
#endif /* WIN */

/*
 * かな漢字変換のための初期処理
 *
 * ・RkwInitializeを呼んで、defaultContext を作成する
 * ・defaultBushuContext を作成する
 * ・辞書のサーチパスを設定する
 * ・システム辞書、部首用辞書、ユーザ辞書をマウントする
 *
 * 引き数	なし
 * 戻り値	0:まあ正常、 -1:とことん不良
 */
KanjiInit()
{
  char *ptr, *getenv(), *kodmesg = ""/* 辞書の種別毎のメッセージ */;
  int con;
  static int mountnottry = 1; /* マウント処理を行っているかどうか */
  struct dicname *stp;
  extern struct dicname *kanjidicnames;
  extern FirstTime;
  extern jrUserInfoStruct *uinfo;
  extern char *RkGetServerHost pro((void));
  int ret = -1;
#ifndef WIN
  char buf[256];
#else
  char *buf = malloc(256);
  if (!buf) {
    return ret;
  }
#endif

#if defined(DEBUG) && !defined(WIN)
  if (iroha_debug) {
    fprintf(stderr,"\nサーバに接続した strokelimit = %d (default:%d)\n",
              cannaconf.strokelimit, STROKE_LIMIT);
  }
#endif
  /* 連文節ライブラリを初期化する */
  if (uinfo) {
    RkwSetUserInfo(uinfo->uname, uinfo->gname, uinfo->topdir);
  }

  if (!(ptr = RkGetServerHost()) &&
      !(ptr = getenv("IROHADICDIR"))) {
    if (uinfo && uinfo->topdir) {
      strcpy(buf, uinfo->topdir);
      strcat(buf, "/dic");
      ptr = buf;
    }
    else {
      ptr = DICHOME;
    }
  }
  if ((defaultContext = RkwInitialize(ptr)) == -1) {
    RkwInitError();
    ret = -1;
    goto return_ret;
  }

  if (defaultContext != -1) {
    if((defaultBushuContext = RkwCreateContext()) == -1) {
      jrKanjiError = "\311\364\274\363\315\321\244\316\245\263\245\363\245\306"
	"\245\257\245\271\245\310\244\362\272\356\300\256\244\307\244\255"
	"\244\336\244\273\244\363\244\307\244\267\244\277";
                     /* 部首用のコンテクストを作成できませんでした */
      addWarningMesg(jrKanjiError);
      defaultContext = -1;
      RkwFinalize();
      ret = -1;
      goto return_ret;
    }
  } else {
    defaultBushuContext = -1;
  }

  debug_message("\245\307\245\325\245\251\245\353\245\310\245\263\245\363"
	"\245\306\245\255\245\271\245\310(%d), \311\364\274\363\245\263"
	"\245\363\245\306\245\255\245\271\245\310(%d)\n",
		defaultContext, defaultBushuContext, 0);
               /* デフォルトコンテキスト(%d), 部首コンテキスト(%d)\n */

  if (defaultContext != -1) {
#ifdef WIN
    if((RkwSetDicPathTmp(defaultContext, "iroha")) == -1) {
      jrKanjiError = "\274\255\275\361\244\316\245\307\245\243\245\354"
	"\245\257\245\310\245\352\244\362\300\337\304\352\244\307\244\255"
	  "\244\336\244\273\244\363\244\307\244\267\244\277";
      /* 辞書のディレクトリを設定できませんでした */
      RkwFinalize();
      ret = NG;
      goto return_ret;
    }
    if((RkwSetDicPathTmp(defaultBushuContext, "iroha")) == -1) {
      jrKanjiError = "\274\255\275\361\244\316\245\307\245\243\245\354"
	"\245\257\245\310\245\352\244\362\300\337\304\352\244\307\244\255"
	  "\244\336\244\273\244\363\244\307\244\267\244\277";
      /* 辞書のディレクトリを設定できませんでした */
      RkwFinalize();
      ret = NG;
      goto return_ret;
    }
#endif /* WIN */

    if (saveapname[0]) {
      RkwSetAppName(defaultContext, saveapname);
    }

    if (!FirstTime && !mountnottry) { /* KC_INITIALIZE で呼び出されていなくて、
					 既にマウント処理を行っている場合 */
      /* 文法辞書のマウント */
      for (stp = kanjidicnames; stp ; stp = stp->next) {
	if (stp->dictype == DIC_GRAMMAR) {
	  if (stp->dicflag == DIC_MOUNTED) {
	    if (RkwMountDic(defaultContext, stp->name,
			    cannaconf.kojin ? PL_ALLOW : PL_INHIBIT) == -1) {
	      stp->dicflag = DIC_MOUNT_FAILED;
	      mountError(stp->name);
	    }
	    else {
	      stp->dicflag = DIC_MOUNTED;
	      dicMesg("\312\270\313\241\274\255\275\361", stp->name);
                      /* 文法辞書 */
	    }
	  }
	}
      }
      /* システム辞書のマウント */
      for (stp = kanjidicnames ; stp ; stp = stp->next) {
        if (stp->dictype != DIC_GRAMMAR) {
          if (stp->dicflag == DIC_MOUNTED) {
            if (stp->dictype == DIC_BUSHU) {
              con = defaultBushuContext;
            }
            else {
              con = defaultContext;
            }
            if (RkwMountDic(con, stp->name,
			    cannaconf.kojin ? PL_ALLOW : PL_INHIBIT)
              == -1) {
#if defined(DEBUG) && !defined(WIN)
            if (iroha_debug) {
              fprintf(stderr, "saveddicname = %s\n", stp->name);
            }
#endif
	      stp->dicflag = DIC_MOUNT_FAILED;
	      mountError(stp->name);
	    }
	    dicMesg("saveddicname\244\316\274\255\275\361", stp->name);
                    /* saveddicnameの辞書 */
	  }
	}
      }
    }
    else { /* KC_INITIALIZE から呼び出されている場合。
              または、マウント処理を行っていない場合 */
#if defined(DEBUG) && !defined(WIN)
      if (iroha_debug) {
        fprintf(stderr, "辞書は.cannaの通りにマウントする\n");
      }
#endif

      mountnottry = 0; /* マウント処理を行うので mountnottry = 0 にする */
      /* 文法辞書のマウント */
      for (stp = kanjidicnames; stp ; stp = stp->next) {
	if (stp->dictype == DIC_GRAMMAR) {
	  if (RkwMountDic(defaultContext, stp->name,
			  cannaconf.kojin ? PL_ALLOW : PL_INHIBIT) == -1) {
	    stp->dicflag = DIC_MOUNT_FAILED;
	    mountError(stp->name);
	  }
	  else {
	    stp->dicflag = DIC_MOUNTED;
	    dicMesg("\312\270\313\241\274\255\275\361", stp->name);
                    /* 文法辞書 */
	  }
	}
      }

      /* システム辞書のマウント */
      for (stp = kanjidicnames ; stp ; stp = stp->next) {
        if (stp->dictype != DIC_GRAMMAR) {
          con = defaultContext;
          if (stp->dictype == DIC_PLAIN) {
            kodmesg = "\245\267\245\271\245\306\245\340\274\255\275\361";
                      /* "システム辞書"; */
          }
          else if (stp->dictype == DIC_USER) {
            /* ユーザ辞書のマウント */    
           kodmesg = "\303\261\270\354\305\320\317\277\315\321\274\255\275\361";
                     /* "単語登録用辞書"; */
          }
          else if (stp->dictype == DIC_RENGO) {
            /* 連語辞書のマウント */
            RengoGakushu = stp;
            kodmesg = "\317\242\270\354\274\255\275\361";
                      /* "連語辞書"; */
          }
          else if (stp->dictype == DIC_KATAKANA) {
            KatakanaGakushu = stp;
            kodmesg = "\274\253\306\260\305\320\317\277\315\321\274\255\275\361";
                      /* "自動登録用辞書"; */
          }
          else if (stp->dictype == DIC_HIRAGANA) {
            HiraganaGakushu = stp;
#ifdef HIRAGANAAUTO
            kodmesg = "\274\253\306\260\305\320\317\277\315\321\274\255\275\361";
                      /* "自動登録用辞書"; */
#else
            kodmesg = "\317\242\270\354\274\255\275\361";
                      /* "連語辞書"; */
#endif
          }
          else if (stp->dictype == DIC_BUSHU) {
            kodmesg = "\311\364\274\363\274\255\275\361";
                      /* "部首辞書"; */
            con = defaultBushuContext;
          }
          if (RkwMountDic(con, stp->name,
			  cannaconf.kojin ? PL_ALLOW : PL_INHIBIT) == -1) {
            extern int auto_define;

            stp->dicflag = DIC_MOUNT_FAILED;
            if (stp->dictype == DIC_KATAKANA
#ifdef HIRAGANAAUTO
                || stp->dictype == DIC_HIRAGANA
#endif
               ) {
              /* 自動登録辞書だったら、自動登録しない */
              auto_define = 0;
            }
            if (stp->dictype != DIC_USER || strcmp(stp->name, "user")) {
              /* ユーザ辞書で user という名前の場合はエラー表示 *
               * しないようにするため                           */
              int majv, minv;

              RkwGetServerVersion(&majv, &minv);
              if (!(canna_version(majv, minv) < canna_version(3, 4))
                  || ((stp->dictype != DIC_KATAKANA ||
                         strcmp(stp->name, "katakana"))
#ifdef HIRAGANAAUTO
                     && (stp->dictype != DIC_HIRAGANA ||
                           strcmp(stp->name, "hiragana"))
#endif
                  )) {
                /* V3.3 以前で、カタカナ辞書が katakana、ひらがな辞書が
                   hiragana の場合はエラーにしないため                  */
                extern char *kataautodic;
#ifdef HIRAGANAAUTO
                extern char *hiraautodic;
#endif

                if (!auto_define ||
                    ((kataautodic && strcmp(stp->name, kataautodic))
#ifdef HIRAGANAAUTO
                    && (hiraautodic && strcmp(stp->name, hiraautodic))
#endif
                   )) {
                  if (stp->dictype == DIC_KATAKANA
#ifdef HIRAGANAAUTO
                      || stp->dictype == DIC_HIRAGANA
#endif
                     ) {
                    autodicError();
                  }
                  else {
                    mountError(stp->name);
                  }
                }
              }
            }
          }
          else {
            stp->dicflag = DIC_MOUNTED;
            dicMesg(kodmesg, stp->name);
          }
        }
      }
    }
    ret = 0;
    goto return_ret;
  }
  ret = -1;
 return_ret:
#ifdef WIN
  (void)free(buf);
#endif
  return ret;
}

/*
 * かな漢字変換のための後処理
 *
 * ・システム辞書、部首用辞書、ユーザ辞書をアンマウントする
 * ・RkwFinalizeを呼ぶ
 *
 * 引き数	なし
 * 戻り値	なし
 */
KanjiFin()
{
  struct dicname *dp, *np;
  int con;

  for (dp = kanjidicnames ; dp ;) {
    if (dp->dictype == DIC_BUSHU) {
      con = defaultBushuContext;
    }
    else {
      con = defaultContext;
    }
    if (dp->dicflag == DIC_MOUNTED) {
      if (RkwUnmountDic(con, dp->name) == -1) {
#ifdef WIN
	char *buf = malloc(128);
	if (buf) {
	  sprintf(buf, "%s \244\362\245\242\245\363\245\336\245\246\245\363"
	  "\245\310\244\307\244\255\244\336\244\273\244\363\244\307\244\267"
	  "\244\277", dp->name);
	  addWarningMesg(buf);
	  (void)free(buf);
	}
#else
        char buf[256];
	sprintf(buf, "%s をアンマウントできませんでした。", dp->name);
	addWarningMesg(buf);
#endif
      }
    }
    np = dp->next;
    free(dp->name);
    free((char *)dp);
    dp = np;
  }
  kanjidicnames = (struct dicname *)0;
	  
  /* 連文節ライブラリを終了させる */
  RkwFinalize();

  return(0);
}

static tanContext
newTanContext(majo, mino)
int majo, mino;
{
  tanContext tan;

  tan = (tanContext)malloc(sizeof(tanContextRec));
  if (tan) {
    bzero(tan, sizeof(tanContextRec));
    tan->id = TAN_CONTEXT;
    tan->majorMode = majo;
    tan->minorMode = mino;
    tan->left = tan->right = (tanContext)0;
    tan->next = (mode_context)0;
    tan->curMode = &tankouho_mode;
  }
  return tan;
}

void
freeTanContext(tan)
tanContext tan;
{
  if (tan->kanji) free((char *)tan->kanji);
  if (tan->yomi) free((char *)tan->yomi);
  if (tan->roma) free((char *)tan->roma);
  if (tan->kAttr) free((char *)tan->kAttr);
  if (tan->rAttr) free((char *)tan->rAttr);
  free((char *)tan);
}

static wchar_t *
DUpwstr(w, l)
wchar_t *w;
int l;
{
  wchar_t *res;

  res = (wchar_t *)malloc((l + 1) * sizeof(wchar_t));
  if (res) {
    WStrncpy(res, w, l);
    res[l] = (wchar_t)0;
  }
  return res;
}

static BYTE *
DUpattr(a, l)
BYTE *a;
int l;
{
  BYTE *res;

  res = (BYTE *)malloc((l + 1) * sizeof(BYTE));
  if (res) {
    bcopy(a, res, (l + 1) * sizeof(BYTE));
  }
  return res;
}

static void
copyYomiinfo2Tan(yc, tan)
yomiContext yc;
tanContext tan;
{
  tan->next = yc->next;
  tan->prevMode = yc->prevMode;
  tan->generalFlags = yc->generalFlags;
  tan->savedFlags = yc->savedFlags;

  tan->romdic = yc->romdic;
  tan->myMinorMode = yc->myMinorMode;
  tan->myEmptyMode = yc->myEmptyMode;
  tan->savedMinorMode = yc->savedMinorMode;
  tan->allowedChars = yc->allowedChars;
  tan->henkanInhibition = yc->henkanInhibition;
}

static void
copyTaninfo2Yomi(tan, yc)
tanContext tan;
yomiContext yc;
{
  /* next と prevMode は既に設定済み */
  yc->generalFlags = tan->generalFlags;
  yc->savedFlags = tan->savedFlags;

  yc->romdic = tan->romdic;
  yc->myMinorMode = tan->myMinorMode;
  yc->myEmptyMode = tan->myEmptyMode;
  yc->savedMinorMode = tan->savedMinorMode;
  yc->allowedChars = tan->allowedChars;
  yc->henkanInhibition = tan->henkanInhibition;
}

extern yomiContext dupYomiContext pro((yomiContext));
extern void setMode pro((uiContext, tanContext, int));

extern void trimYomi pro((uiContext, int, int, int, int));

/*
  全文節を tanContext に変換する
 */

int
doTanConvertTb(d, yc)
uiContext d;
yomiContext yc;
{
  int cur = yc->curbun, i, len, ylen = 0, rlen = 0, ret = 0;
  int scuryomi, ecuryomi, scurroma, ecurroma;
  tanContext tan, prevLeft = yc->left, curtan = (tanContext)0;
  BYTE *p, *q, *r;
#ifndef WIN
  wchar_t xxx[ROMEBUFSIZE];
#else
  wchar_t *xxx = (wchar_t *)malloc(sizeof(wchar_t) * ROMEBUFSIZE);
  if (!xxx) {
    return ret;
  }
#endif

  yc->kouhoCount = 0;
  scuryomi = ecuryomi = scurroma = ecurroma = 0;

/*  jrKanjiError = "メモリが足りません"; */
  jrKanjiError = "malloc (doTanBubunMuhenkan) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277\241\243";
                 /* malloc (doTanBubunMuhenkan) できませんでした */ 
  for (i = 0 ; i < yc->nbunsetsu ; i++) {
    tan = newTanContext(yc->majorMode, CANNA_MODE_TankouhoMode);
    if (tan) {
      copyYomiinfo2Tan(yc, tan);
      RkwGoTo(yc->context, i);
      len = RkwGetKanji(yc->context, xxx, ROMEBUFSIZE);
      if (len >= 0) {
	tan->kanji = DUpwstr(xxx, len);
	if (tan->kanji) {
	  len = RkwGetYomi(yc->context, xxx, ROMEBUFSIZE);
	  if (len >= 0) {
	    tan->yomi = DUpwstr(xxx, len);
	    if (tan->yomi) {
	      tan->kAttr = DUpattr(yc->kAttr + ylen, len);
	      if (tan->kAttr) {
		r = yc->rAttr + rlen;
		for (p = yc->kAttr + ylen, q = p + len ; p < q ; p++) {
		  if (*p & SENTOU) {
		    r++;
		    while (!(*r & SENTOU)) {
		      r++;
		    }
		  }
		}
		ylen += len;
		len = r - yc->rAttr - rlen; /* ローマ字の長さ */
		tan->roma = DUpwstr(yc->romaji_buffer + rlen, len);
		if (tan->roma) {
		  tan->rAttr = DUpattr(yc->rAttr + rlen, len);
		  if (tan->rAttr) {
		    rlen += len;
		    /* とりあえず左につなげる */
		    tan->right = (tanContext)yc;
		    tan->left = yc->left;
		    if (yc->left) {
		      yc->left->right = tan;
		    }
		    yc->left = tan;
		    if (i == cur) {
		      curtan = tan;
		    }
		    continue;
		  }
		  free((char *)tan->roma);
		}
		free((char *)tan->kAttr);
	      }
	      free((char *)tan->yomi);
	    }
	  }
	  else {
	    makeRkError(d, KanjiInitError());
	  }
	  free((char *)tan->kanji);
	}
      }
      else {
	makeRkError(d, KanjiInitError());
      }
      freeTanContext(tan);
    }
    /* エラー処理をする */
  procerror:
    while ((tan = yc->left) != prevLeft) {
      yc->left = tan->left;
      freeTanContext(tan);
    }
    ret = -1;
    goto return_ret;
  }

  if (chikujip(yc) && chikujiyomiremain(yc)) {
    int rpos;
    yomiContext lyc = dupYomiContext(yc);

    if (!lyc) { /* エラー処理をする */
      goto procerror;
    }

    if (yc->right) { /* 逐次の場合ないはずだが念のため */
      yc->right->left = (tanContext)lyc;
    }
    lyc->right = yc->right;
    yc->right = (tanContext)lyc;
    lyc->left = (tanContext)yc;

    kPos2rPos(lyc, 0, yc->cStartp, (int *)0, &rpos);
    d->modec = (mode_context)lyc;
    moveToChikujiYomiMode(d);
    trimYomi(d, yc->cStartp, yc->kEndp, rpos, yc->rEndp);
    d->modec = (mode_context)yc;
    yc->status = lyc->status;
    lyc->cStartp = lyc->cRStartp = lyc->ys = lyc->ye = 0;
  }

  RkwGoTo(yc->context, cur);
  if (RkwEndBun(yc->context, cannaconf.Gakushu ? 1 : 0) == -1) {
    jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\316"
	"\275\252\316\273\244\313\274\272\307\324\244\267\244\336\244\267"
	"\244\277";
                   /* かな漢字変換の終了に失敗しました */
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
  }

  d->modec = (mode_context)curtan;
  setMode(d, curtan, 1);
  makeKanjiStatusReturn(d, (yomiContext)curtan);

  /* yc をリンクから抜く */
  if (yc->left) {
    yc->left->right = yc->right;
  }
  if (yc->right) {
    yc->right->left = yc->left;
  }
  abandonContext(d, yc);
  freeYomiContext(yc);

 return_ret:
#ifdef WIN
  (void)free((char *)xxx);
#endif

  return ret;
}

static int
doTanBubunMuhenkan(d, yc)
uiContext d;
yomiContext yc;
{
  int cur = yc->curbun, i, len, ylen = 0, rlen = 0, ret = 0;
  int scuryomi, ecuryomi, scurroma, ecurroma;
  tanContext tan, prevLeft = yc->left;
  BYTE *p, *q, *r;
#ifndef WIN
  wchar_t xxx[ROMEBUFSIZE];
#else
  wchar_t *xxx = (wchar_t *)malloc(sizeof(wchar_t) * ROMEBUFSIZE);
  if (!xxx) {
    return ret;
  }
#endif

  yc->kouhoCount = 0;
  scuryomi = ecuryomi = scurroma = ecurroma = 0;

/*  jrKanjiError = "メモリが足りません"; */
  jrKanjiError = "malloc (doTanBubunMuhenkan) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277\241\243";
                 /* malloc (doTanBubunMuhenkan) できませんでした */ 
  for (i = 0 ; i < yc->nbunsetsu ; i++) {
    tan = (tanContext)0;
    if (i == cur ||
	(tan = newTanContext(yc->majorMode, CANNA_MODE_TankouhoMode))) {
      if (tan) {
	copyYomiinfo2Tan(yc, tan);
      }
      RkwGoTo(yc->context, i);
      len = RkwGetKanji(yc->context, xxx, ROMEBUFSIZE);
      if (len >= 0) {
	if (!tan || (tan->kanji = DUpwstr(xxx, len))) {
	  len = RkwGetYomi(yc->context, xxx, ROMEBUFSIZE);
	  if (len >= 0) {
	    if (!tan || (tan->yomi = DUpwstr(xxx, len))) {
	      if (!tan || (tan->kAttr = DUpattr(yc->kAttr + ylen, len))) {
		r = yc->rAttr + rlen;
		for (p = yc->kAttr + ylen, q = p + len ; p < q ; p++) {
		  if (*p & SENTOU) {
		    r++;
		    while (!(*r & SENTOU)) {
		      r++;
		    }
		  }
		}
		if (i == cur) {
		  scuryomi = ylen;
		  ecuryomi = ylen + len;
		}
		ylen += len;
		len = r - yc->rAttr - rlen; /* ローマ字の長さ */
		if (!tan ||
		    (tan->roma = DUpwstr(yc->romaji_buffer + rlen, len))) {
		  if (!tan || (tan->rAttr = DUpattr(yc->rAttr + rlen, len))) {
		    if (i == cur) {
		      scurroma = rlen;
		      ecurroma = rlen + len;
		    }
		    rlen += len;
		    if (tan) {
		      if (i != cur) {
			/* とりあえず左につなげる */
			tan->right = (tanContext)yc;
			tan->left = yc->left;
			if (yc->left) {
			  yc->left->right = tan;
			}
			yc->left = tan;
		      }
#if defined(DEBUG) && !defined(WIN)
		      {
			char yyy[ROMEBUFSIZE];
			WCstombs(yyy, tan->kanji, ROMEBUFSIZE);
			printf("%s/", yyy);
			WCstombs(yyy, tan->yomi, ROMEBUFSIZE);
			printf("%s/", yyy);
			WCstombs(yyy, tan->roma, ROMEBUFSIZE);
			printf("%s\n", yyy);
		      }
#endif
		    }
		    continue;
		  }
		  if (tan) free((char *)tan->roma);
		}
		if (tan) free((char *)tan->kAttr);
	      }
	      if (tan) free((char *)tan->yomi);
	    }
	  }
	  else {
	    makeRkError(d, KanjiInitError());
	  }
	  if (tan) free((char *)tan->kanji);
	}
      }
      else {
	makeRkError(d, KanjiInitError());
      }
      if (tan) freeTanContext(tan);
    }
    /* エラー処理をする */
    while ((tan = yc->left) != prevLeft) {
      yc->left = tan->left;
      freeTanContext(tan);
    }
    ret = -1;
    goto return_ret;
  }

  if (chikujip(yc) && chikujiyomiremain(yc)) {
    int rpos;
    yomiContext lyc = dupYomiContext(yc);

    if (!lyc) { /* エラー処理をする */
      while ((tan = yc->left) != prevLeft) {
	yc->left = tan->left;
	freeTanContext(tan);
      }
      ret = -1;
      goto return_ret;
    }

    if (yc->right) { /* ないはず */
      yc->right->left = (tanContext)lyc;
    }
    lyc->right = yc->right;
    yc->right = (tanContext)lyc;
    lyc->left = (tanContext)yc;

    kPos2rPos(lyc, 0, yc->cStartp, (int *)0, &rpos);
    d->modec = (mode_context)lyc;
    moveToChikujiYomiMode(d);
    trimYomi(d, yc->cStartp, yc->kEndp, rpos, yc->rEndp);
    d->modec = (mode_context)yc;
    yc->status = lyc->status;
    lyc->cStartp = lyc->cRStartp = lyc->ys = lyc->ye = 0;
  }

  if (cur + 1 < yc->nbunsetsu) { /* yc が最後じゃない場合 */
    int n = yc->nbunsetsu - cur - 1;
    tan = yc->left;
    tan->right = yc->right;
    if (yc->right) {
      yc->right->left = tan;
    }
    for (i = 1 ; i < n ; i++) { /* yomi の right に来るべき tan を得たい */
      tan = tan->left;
    }
    if (tan->left) {
      tan->left->right = (tanContext)yc;
    }
    yc->left = tan->left;
    tan->left = (tanContext)yc;
    yc->right = tan;
  }
  RkwGoTo(yc->context, cur);
  if (RkwEndBun(yc->context, cannaconf.Gakushu ? 1 : 0) == -1) {
    jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\316"
	"\275\252\316\273\244\313\274\272\307\324\244\267\244\336\244\267"
	"\244\277";
                   /* かな漢字変換の終了に失敗しました */
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
  }

  trimYomi(d, scuryomi, ecuryomi, scurroma, ecurroma);

  yc->cRStartp = yc->rCurs = yc->rStartp = 0;
  yc->cStartp = yc->kCurs = yc->kRStartp =
    yc->ys = yc->ye = 0;
  yc->status &= CHIKUJI_NULL_STATUS;
  /* なんと逐次でなくなる */
  if (chikujip(yc)) {
    yc->generalFlags &= ~CANNA_YOMI_CHIKUJI_MODE;
    yc->generalFlags |= CANNA_YOMI_BASE_CHIKUJI;
  }

  d->current_mode = yc->curMode = &yomi_mode;
  yc->minorMode = getBaseMode(yc);

  /* 全部無変換にする */
  yc->nbunsetsu = 0;

  /* 単候補状態から読みに戻るときには無条件にmarkを先頭に戻す */
  yc->cmark = yc->pmark = 0;

  abandonContext(d, yc);
  ret = 0;

 return_ret:
#ifdef WIN
  (void)free((char *)xxx);
#endif

  return ret;
}

extern void restoreChikujiIfBaseChikuji pro((yomiContext));
extern void ReCheckStartp pro((yomiContext));
extern void fitmarks pro((yomiContext));

int YomiBubunKakutei pro((uiContext));

int
YomiBubunKakutei(d)
uiContext d;
{
  yomiContext yc = (yomiContext)d->modec;
  tanContext tan;
  int len;

  if (yc->id != YOMI_CONTEXT) {
    /* あり得ないのでは? */
  }
  else /* if (yc->left) */ {
    /* yomiContext で削除する部分をまず tanContext に切り出し、yc の左
       側に挿入する。次に yc の左をばっさりと確定する。
       全てこのロジックでやろうかしら。
       */
    tan = newTanContext(yc->majorMode, CANNA_MODE_TankouhoMode);
    if (tan) {
      copyYomiinfo2Tan(yc, tan);
      /* かなをコピーする */
      tan->kanji = DUpwstr(yc->kana_buffer, yc->kCurs);
      if (tan->kanji) {
	/* ここも同じかなをコピーする */
	tan->yomi = DUpwstr(yc->kana_buffer, yc->kCurs);
	if (tan->yomi) {
	  tan->kAttr = DUpattr(yc->kAttr, yc->kCurs);
	  if (tan->kAttr) {
	    tan->roma = DUpwstr(yc->romaji_buffer, yc->rCurs);
	    if (tan->roma) {
	      tan->rAttr = DUpattr(yc->rAttr, yc->rCurs);
	      if (tan->rAttr) {
		wchar_t *sb = d->buffer_return, *eb = sb + d->n_buffer;

		tan->left = yc->left;
		tan->right = (tanContext)yc;
		if (yc->left) {
		  yc->left->right = tan;
		}
		yc->left = tan;
		while (tan->left) {
		  tan = tan->left;
		}

		trimYomi(d, yc->kCurs, yc->kEndp, yc->rCurs, yc->rEndp);

		len = doKakutei(d, tan, (tanContext)yc, sb, eb,
				(yomiContext *)0);
		d->modec = (mode_context)yc;
		yc->left = (tanContext)0;
		goto done;
	      }
	      free((char *)tan->roma);
	    }
	    free((char *)tan->kAttr);
	  }
	  free((char *)tan->yomi);
	}
	free((char *)tan->kanji);
      }
      free((char *)tan); /* not freeTanContext(tan); */
    }
  }
#if 0
  /* 本来ここの処理をいれた方が効率が良いと思われるが、読みの一部を確
  定させて、しかもローマ字情報などもいれるのは面倒なのであとまわしとする */
  else {
    
    /* 確定させる。
       次に trim する*/
  }
#endif

 done:
  if (!yc->kEndp) {
    if (yc->savedFlags & CANNA_YOMI_MODE_SAVED) {
      restoreFlags(yc);
    }
    if (yc->right) {
      removeCurrentBunsetsu(d, (tanContext)yc);
      yc = (yomiContext)0;
    }
    else {
      /* 未確定文字列が全くなくなったのなら、φモードに遷移する */
      restoreChikujiIfBaseChikuji(yc);
      d->current_mode = yc->curMode = yc->myEmptyMode;
      d->kanji_status_return->info |= KanjiEmptyInfo;
    }
    currentModeInfo(d);
  }
  else {
    if (yc->kCurs != yc->kRStartp) {
      ReCheckStartp(yc);
    }
  }

  if (yc) {
    fitmarks(yc);
  }

  makeYomiReturnStruct(d);
  return len;
}

yomiContext
newFilledYomiContext(next, prev)
mode_context next;
KanjiMode prev;
{
  yomiContext yc;

  yc = newYomiContext((wchar_t *)NULL, 0, /* 結果は格納しない */
		      CANNA_NOTHING_RESTRICTED,
		      (int)!CANNA_YOMI_CHGMODE_INHIBITTED,
		      (int)!CANNA_YOMI_END_IF_KAKUTEI,
		      CANNA_YOMI_INHIBIT_NONE);
  if (yc) {
    yc->majorMode = yc->minorMode = CANNA_MODE_HenkanMode;
    yc->curMode = &yomi_mode;
    yc->myEmptyMode = &empty_mode;
    yc->romdic = romajidic;
    yc->next = next;
    yc->prevMode = prev;
  }
  return yc;
}

#ifdef DO_MERGE
static
yomiContext
mergeYomiContext(yc)
yomiContext yc;
{
  yomiContext res, a, b;

  res = yc;
  while (res->left && res->left->id == YOMI_CONTEXT) {
    res = (yomiContext)res->left;
  }
  for (a = (yomiContext)res->right ; a && a->id == YOMI_CONTEXT ; a = b) {
    b = (yomiContext)a->right;
    appendYomi2Yomi(a, res);
    if (yc == a) {
      res->kCurs = res->kRStartp = res->kEndp;
      res->rCurs = res->rStartp = res->rEndp;
      res->cmark = res->kCurs;
    }
    res->right = a->right;
    if (res->right) {
      res->right->left = (tanContext)res;
    }
    /* yc->context の close はいらないのかなあ。1996.10.30 今 */
    freeYomiContext(a);
  }
  return res;
}
#endif

/*
  tanContext を yomiContext にして、読み入力状態にする

   0          失敗
   otherwise  あたらしい読みコンテキストが返る

 */

static yomiContext
tanbunUnconvert(d, tan)
uiContext d;
tanContext tan;
{
  yomiContext yc;

  yc = newFilledYomiContext(tan->next, tan->prevMode);
  if (yc) {
    extern KanjiModeRec yomi_mode, empty_mode;

    appendTan2Yomi(tan, yc);
    copyTaninfo2Yomi(tan, yc);
    yc->right = tan->right;
    yc->left = tan->left;
    if (yc->myMinorMode) {
      yc->minorMode = yc->myMinorMode;
    }

    if (chikujip(yc)) { /* 逐次にはしない */
      yc->generalFlags &= ~CANNA_YOMI_CHIKUJI_MODE;
      yc->generalFlags |= CANNA_YOMI_BASE_CHIKUJI;
    }

    if (yc->left) {
      yc->left->right = (tanContext)yc;
    }
    if (yc->right) {
      yc->right->left = (tanContext)yc;
    }
    freeTanContext(tan);
#ifdef DO_MERGE /* 定義していない */
    yc = mergeYomiContext(yc);
#endif
    d->current_mode = yc->curMode;
    d->modec = (mode_context)yc;
    return yc;
  }
  jrKanjiError = "\245\341\245\342\245\352\244\254\302\255\244\352\244\336"
	"\244\273\244\363";
                 /* メモリが足りません */
  return (yomiContext)0;
}

static int
TbBubunMuhenkan(d)
uiContext d;
{
  tanContext tan = (tanContext)d->modec;
  yomiContext yc;

  yc = tanbunUnconvert(d, tan);
  if (yc) {
    currentModeInfo(d);
    makeKanjiStatusReturn(d, yc);
    return 0;
  }
  makeGLineMessageFromString(d, jrKanjiError);
  return NothingChangedWithBeep(d);
}

/*
  TanBubunMuhenkan -- 変換中の文字列を文節毎に分割する。

    その際、読みやローマ字も分割する
 */

int
TanBubunMuhenkan(d)
uiContext d;
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return TbBubunMuhenkan(d);
  }

  if (!yc->right && !yc->left && yc->nbunsetsu == 1) {
    return TanMuhenkan(d);
  }

  if (doTanBubunMuhenkan(d, yc) < 0) {
    makeGLineMessageFromString(d, jrKanjiError);
    return TanMuhenkan(d);
  }
  makeYomiReturnStruct(d);
  currentModeInfo(d);
  return 0;
}

int
prepareHenkanMode(d)
uiContext d;
{
  yomiContext yc = (yomiContext)d->modec;

  if (confirmContext(d, yc) < 0) {
    return 0;
  }
  d->current_mode = yc->curMode = &tankouho_mode;

  return 1;
}

doHenkan(d, len, kanji)
uiContext d;
int len;
wchar_t *kanji;
{
  /* よみを漢字に変換する */
  if(doYomiHenkan(d, len, kanji) == NG) {
    return -1;
  }

  /* kanji_status_returnを作る */
  makeKanjiStatusReturn(d, (yomiContext)d->modec);
  return 0;
}


/*
 * かな漢字変換を行う
 * ・d->yomi_bufferによみを取り出し、RkwBgnBunを呼んでかな漢字変換を開始する
 * ・カレント文節を先頭文節にして、エコー文字列を作る
 *
 * 引き数	uiContext
 *		len       len が指定されていたら文節長をその長さにする。
 *		kanji	  kanji が指定されていたら単文節変換して、
 *			  カレント候補を kanji で示された候補に合わせる。
 * 戻り値	正常終了時 0	異常終了時 -1
 */
static
doYomiHenkan(d, len, kanji)
uiContext	d;
int len;
wchar_t *kanji;
{
  unsigned int mode;
  yomiContext yc = (yomiContext)d->modec;
  extern defaultContext;

#if defined(DEBUG) && !defined(WIN)
  if (iroha_debug) {
/*    printf("yomi     => "); Wprintf(hc->yomi_buffer); putchar('\n');*/
    printf("yomi len => %d\n", hc->yomilen);
  }
#endif

  /* 連文節変換を開始する *//* 辞書にない カタカナ ひらがな を付加する */
  mode = 0;
  mode = (RK_XFER<<RK_XFERBITS) | RK_KFER;
  if (kanji) {
    mode |= RK_HENKANMODE(RK_TANBUN |
			  RK_MAKE_WORD |
			  RK_MAKE_EISUUJI |
			  RK_MAKE_KANSUUJI) << (2 * RK_XFERBITS);
  }
  
  if (confirmContext(d, yc) < 0) {
    return NG;
  }

#ifdef MEASURE_TIME
  {
    struct tms timebuf;
    long RkTime, times();

    RkTime = times(&timebuf);
#endif /* MEASURE_TIME */

    if ((yc->nbunsetsu =
	 RkwBgnBun(yc->context, yc->kana_buffer, yc->kEndp, mode)) == -1) {
      yc->nbunsetsu = 0;
      return kanakanError(d);
    }
    
    if (len > 0 && (yc->nbunsetsu = RkwResize(yc->context, len)) == -1) {
      RkwEndBun(yc->context, 0);
      yc->nbunsetsu = 0;
      return kanakanError(d);
    }

    if (kanji) {
      /* kanji が指定されていたら、同じ候補がでるまで RkwNext をする */
      int i, n;

      n = RkwGetKanjiList(yc->context, d->genbuf, ROMEBUFSIZE);
      if (n < 0) {
	return kanakanError(d);
      }
      for (i = 0 ; i < n ; i++) {
	RkwXfer(yc->context, i);
	len = RkwGetKanji(yc->context, d->genbuf, ROMEBUFSIZE);
	if (len < 0) {
	  return kanakanError(d);
	}
	d->genbuf[len] = (wchar_t)'\0';
	if (!WStrcmp(kanji, d->genbuf)) {
	  break;
	}
      }
      if (i == n) {
	RkwXfer(yc->context, 0);
      }
    }

#ifdef MEASURE_TIME
    yc->rktime = times(&timebuf);
    yc->rktime -= RkTime;
  }
#endif /* MEASURE_TIME */

  /* カレント文節は先頭文節 */
  yc->curbun = 0;

  return(0);
}

int
TanNop(d)
uiContext	d;
{
  yomiContext yc = (yomiContext)d->modec;

  /* currentModeInfo でモード情報が必ず返るようにダミーのモードを入れておく */
  d->majorMode = d->minorMode = CANNA_MODE_AlphaMode;
  currentModeInfo(d);

  makeKanjiStatusReturn(d, yc);
  return 0;
}

static int
doGoTo(d, yc)
uiContext d;
yomiContext yc;
{
  if (RkwGoTo(yc->context, yc->curbun) == -1) {
    return makeRkError(d, "\312\270\300\341\244\316\260\334\306\260\244\313"
	"\274\272\307\324\244\267\244\336\244\267\244\277");
                          /* 文節の移動に失敗しました */
  }
  yc->status |= CHIKUJI_OVERWRAP;

  /* kanji_status_returnを作る */
  makeKanjiStatusReturn(d, yc);
  return 0;
}

/*
 * 次文節に移動する
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */

int
TanForwardBunsetsu(d)
uiContext	d;
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return TbForward(d);
  }

  yc->kouhoCount = 0;
  if (yc->curbun + 1 < yc->nbunsetsu) {
    yc->curbun++;
  }
  else if (yc->cStartp && yc->cStartp < yc->kEndp) { /* 逐次の読みが右にある */
    yc->kRStartp = yc->kCurs = yc->cStartp;
    yc->rStartp = yc->rCurs = yc->cRStartp;
    moveToChikujiYomiMode(d);
  }
  else if (yc->right) {
    return TbForward(d);
  }
  else if (cannaconf.kakuteiIfEndOfBunsetsu) {
    d->nbytes = TanKakutei(d);
    d->kanji_status_return->length 
     = d->kanji_status_return->revPos
       = d->kanji_status_return->revLen = 0;
    return d->nbytes;
  }
  else if (!cannaconf.CursorWrap) {
    return NothingForGLine(d);
  }
  else if (yc->left) {
    return TbBeginningOfLine(d);
  }
  else {
    yc->curbun = 0;
  }

  /* カレント文節を１つ右に移す */
  /* カレント文節が最右だったら、
     最左をカレント文節にする   */
  return doGoTo(d, yc);
}

/*
 * 前文節に移動する
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
int
TanBackwardBunsetsu(d)
uiContext	d;
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return TbBackward(d);
  }

  yc->kouhoCount = 0;
  if (yc->curbun) {
    yc->curbun--;
  }
  else if (yc->left) {
    return TbBackward(d);
  }
  else if (!cannaconf.CursorWrap) {
    return NothingForGLine(d);
  }
  else if (yc->right) {
    return TbEndOfLine(d);
  }
  else if (yc->cStartp && yc->cStartp < yc->kEndp) { /* 逐次の読みが右にある */
    yc->kCurs = yc->kRStartp = yc->kEndp;
    yc->rCurs = yc->rStartp = yc->rEndp;
    moveToChikujiYomiMode(d);
  }
  else {
    yc->curbun = yc->nbunsetsu - 1;
  }

  return doGoTo(d, yc);
}

/*
 * 次候補をカレント候補にする
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */

static
tanNextKouho(d, yc)
uiContext	d;
yomiContext   yc;
{
#ifdef MEASURE_TIME
  struct tms timebuf;
  long proctime, times();

  proctime = times(&timebuf);
#endif /* MEASURE_TIME */

  /* 次の候補をカレント候補とする */
  if (RkwNext(yc->context) == -1) {
    makeRkError(d, "\245\253\245\354\245\363\245\310\270\365\312\344\244\362"
	"\274\350\244\352\275\320\244\273\244\336\244\273\244\363\244\307"
	"\244\267\244\277");
                   /* カレント候補を取り出せませんでした */
    return TanMuhenkan(d);
  }

#ifdef MEASURE_TIME
  yc->rktime = times(&timebuf);
  yc->rktime -= proctime;
#endif /* MEASURE_TIME */

  /* kanji_status_returnを作る */
  makeKanjiStatusReturn(d, yc);

#ifdef MEASURE_TIME
  yc->proctime = times(&timebuf);
  yc->proctime -= proctime;
#endif /* MEASURE_TIME */

  return(0);
}

/*
  tanbunHenkan -- 変換する。

    みそは、kanji で示された候補と同じ候補が出るまで RkwNext をすること
    である。一周したことも検出しなければなるまい。
 */

static int
tanbunHenkan(d, yc, kanji)
uiContext d;
yomiContext yc;
wchar_t *kanji;
{
  if (!prepareHenkanMode(d)) {
    makeGLineMessageFromString(d, jrKanjiError);
    makeYomiReturnStruct(d);
    return 0;
  }
  yc->minorMode = CANNA_MODE_TankouhoMode;
  yc->kouhoCount = 1;
  if (doHenkan(d, 0, kanji) < 0) {
    makeGLineMessageFromString(d, jrKanjiError);
    makeYomiReturnStruct(d);
    return 0;
  }
  if (cannaconf.kouho_threshold > 0 &&
      yc->kouhoCount >= cannaconf.kouho_threshold) {
    return tanKouhoIchiran(d, 0);
  }
  
  currentModeInfo(d);
  makeKanjiStatusReturn(d, yc);
  return 0;
}

/*
  enterTanHenkanMode -- tanContext を yomiContext にして変換の準備をする

 */

static int
enterTanHenkanMode(d, fnum)
uiContext d;
{
  tanContext tan = (tanContext)d->modec;
  yomiContext yc;
  wchar_t *prevkanji;

  prevkanji = tan->kanji;
  tan->kanji = (wchar_t *)0;

  yc = tanbunUnconvert(d, tan);
  if (yc) {
    tanbunHenkan(d, yc, prevkanji);
    free((char *)prevkanji);

    /*ここで
      単候補モードの形にする
      */

    d->more.todo = 1;
    d->more.ch = d->ch;
    d->more.fnum = fnum;
    return 0;
  }
  else { /* 二重フリーをしないため強調的に else を書く */
    free((char *)prevkanji);
  }
  makeGLineMessageFromString(d, jrKanjiError);
  return NothingChangedWithBeep(d);
}

/*
 * 候補一覧行を表示する
 *
 * ・候補一覧表示のためのデータをテーブルに作成する
 * ・候補一覧表示行が狭いときは、一覧を表示しないで次候補をその場に表示する
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */

TanKouhoIchiran(d)
uiContext d;
{
  if (d->modec->id != YOMI_CONTEXT) {
    return enterTanHenkanMode(d, CANNA_FN_KouhoIchiran);
  }
  return tanKouhoIchiran(d, 1);
}

TanNextKouho(d)
uiContext d;
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return enterTanHenkanMode(d, CANNA_FN_Next);
  }
  yc->status |= CHIKUJI_OVERWRAP;
  yc->kouhoCount = 0;
  return tanNextKouho(d, yc);
}

/*

  TanHenkan -- 回数をチェックする以外は TanNextKouho とほぼ同じ

 */
static TanHenkan pro((uiContext));

static int
TanHenkan(d)
uiContext d;
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return enterTanHenkanMode(d, CANNA_FN_Henkan);
  }

  if (cannaconf.kouho_threshold &&
      ++yc->kouhoCount >= cannaconf.kouho_threshold) {
    return TanKouhoIchiran(d);
  }
  else {
    return tanNextKouho(d, yc);
  }
}

/*
 * 前候補をカレント候補にする
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
TanPreviousKouho(d)
uiContext	d;
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return enterTanHenkanMode(d, CANNA_FN_Prev);
  }

  yc->status |= CHIKUJI_OVERWRAP;
  yc->kouhoCount = 0;
  /* 前の候補をカレント候補とする */
  if (RkwPrev(yc->context) == -1) {
    makeRkError(d, "\245\253\245\354\245\363\245\310\270\365\312\344\244\362"
	"\274\350\244\352\275\320\244\273\244\336\244\273\244\363\244\307"
	"\244\267\244\277");
                   /* カレント候補を取り出せませんでした */
    return TanMuhenkan(d);
  }

  /* kanji_status_returnを作る */
  makeKanjiStatusReturn(d, yc);

  return 0;
}

/*
  tanJishuHenkan -- 特定の文節だけ字種変換する
 */

static int tanJishuHenkan pro((uiContext, int));

static int
tanJishuHenkan(d, fn)
uiContext d;
int fn;
{
  d->nbytes = TanBubunMuhenkan(d);
  d->more.todo = 1;
  d->more.ch = d->ch;
  d->more.fnum = fn;
  return d->nbytes;
}

TanHiragana(d)
uiContext	d;
{
  return tanJishuHenkan(d, CANNA_FN_Hiragana);
}

TanKatakana(d)
uiContext	d;
{
  return tanJishuHenkan(d, CANNA_FN_Katakana);
}

TanRomaji(d)
uiContext	d;
{
  return tanJishuHenkan(d, CANNA_FN_Romaji);
}

TanUpper(d)
uiContext	d;
{
  return tanJishuHenkan(d, CANNA_FN_ToUpper);
}

TanCapitalize(d)
uiContext	d;
{
  return tanJishuHenkan(d, CANNA_FN_Capitalize);
}

TanZenkaku(d)
uiContext d;
{
  return tanJishuHenkan(d, CANNA_FN_Zenkaku);
}

TanHankaku(d)
uiContext d;
{
  return tanJishuHenkan(d, CANNA_FN_Hankaku);
}

int TanKanaRotate pro((uiContext));

TanKanaRotate(d)
uiContext d;
{
  return tanJishuHenkan(d, CANNA_FN_KanaRotate);
}

int TanRomajiRotate pro((uiContext));

TanRomajiRotate(d)
uiContext d;
{
  return tanJishuHenkan(d, CANNA_FN_RomajiRotate);
}

int TanCaseRotateForward pro((uiContext));

TanCaseRotateForward(d)
uiContext d;
{
  return tanJishuHenkan(d, CANNA_FN_CaseRotate);
}

static int
gotoBunsetsu(yc, n)
yomiContext yc;
int n;
{
  /* カレント文節を移動する */
  if (RkwGoTo(yc->context, n) == -1) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
    jrKanjiError = "\312\270\300\341\244\316\260\334\306\260\244\313\274\272"
	"\307\324\244\267\244\336\244\267\244\277";
                   /* 文節の移動に失敗しました */
    return NG;
  }
  yc->curbun = n;
  return 0;
}

/*
 * 最左文節に移動する
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
int
TanBeginningOfBunsetsu(d)
uiContext	d;
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT || yc->left) {
    return TbBeginningOfLine(d);
  }
  yc->kouhoCount = 0;
  if (gotoBunsetsu(yc, 0) < 0) {
    return NG;
  }
  makeKanjiStatusReturn(d, yc);
  return 0;
}

/*
 * 最右文節に移動する
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
int
TanEndOfBunsetsu(d)
uiContext	d;
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT || yc->right) {
    return TbEndOfLine(d);
  }

  yc->kouhoCount = 0;
  if (yc->cStartp && yc->cStartp < yc->kEndp) {
    yc->kRStartp = yc->kCurs = yc->kEndp;
    yc->rStartp = yc->rCurs = yc->rEndp;
    moveToChikujiYomiMode(d);
  }
  if (gotoBunsetsu(yc, yc->nbunsetsu - 1) < 0) {
    return NG;
  }
  yc->status |= CHIKUJI_OVERWRAP;
  makeKanjiStatusReturn(d, yc);
  return 0;
}

int
tanMuhenkan(d, kCurs)
uiContext d;
int kCurs;
{
  extern KanjiModeRec yomi_mode;
  yomiContext yc = (yomiContext)d->modec;
  int autoconvert = (yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE);

  if (RkwEndBun(yc->context, 0) == -1) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
  }

  if (autoconvert) {
    yc->status &= CHIKUJI_NULL_STATUS;
    d->current_mode = yc->curMode = &cy_mode;
    yc->ys = yc->ye = yc->cStartp = yc->cRStartp = 0;
    yc->rCurs = yc->rStartp = yc->rEndp;
    yc->kCurs = yc->kRStartp = yc->kEndp;
    clearHenkanContext(yc);
  }
  else {
    d->current_mode = yc->curMode = &yomi_mode;
  }
  yc->minorMode = getBaseMode(yc);

  if (kCurs >= 0) {
    int rpos;

    kPos2rPos(yc, 0, kCurs, (int *)0, &rpos);
    yc->kCurs = yc->kRStartp = kCurs;
    yc->rCurs = yc->rStartp = rpos;
  }

  /* 全部無変換にする */
  yc->nbunsetsu = 0;

  /* 単候補状態から読みに戻るときには無条件にmarkを先頭に戻す */
  yc->cmark = yc->pmark = 0;

  abandonContext(d, yc);

  return 0;
}

/*
 * 全ての文節を読みに戻し、YomiInputMode に戻る
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */

TanMuhenkan(d)
uiContext	d;
{
  yomiContext yc = (yomiContext)d->modec, newyc;
  tanContext tan;

  if (yc->id != YOMI_CONTEXT || yc->left || yc->right) {
    tan = (tanContext)yc;
    while (tan->left) {
      tan = tan->left;
    }
    if (tan->id == YOMI_CONTEXT) {
      newyc = (yomiContext)tan;
    }
    else {
      newyc = newFilledYomiContext(yc->next, yc->prevMode);
      if (newyc) {
	tan->left = (tanContext)newyc;
	newyc->right = tan;
	newyc->generalFlags = tan->generalFlags;
	newyc->savedFlags = tan->savedFlags;
	if (chikujip(newyc)) {
	  newyc->curMode = &cy_mode;
	}
	newyc->minorMode = getBaseMode(newyc);
      }
      else {
	jrKanjiError = "\245\341\245\342\245\352\244\254\302\255\244\352"
	"\244\336\244\273\244\363";
                       /* メモリが足りません */
	makeGLineMessageFromString(d, jrKanjiError);
	return NothingChangedWithBeep(d);
      }
    }
    d->modec = (mode_context)newyc;
    d->current_mode = newyc->curMode;

    doMuhenkan(d, newyc);

    if (newyc->generalFlags &
	(CANNA_YOMI_CHIKUJI_MODE | CANNA_YOMI_BASE_CHIKUJI)) {
      /* 「心は逐次だった」のであれば、逐次モードに戻す */
      newyc->generalFlags |= CANNA_YOMI_CHIKUJI_MODE;
      newyc->generalFlags &= ~CANNA_YOMI_BASE_CHIKUJI;
      newyc->minorMode = getBaseMode(newyc);
      d->current_mode = newyc->curMode = &cy_mode;
    }

    makeYomiReturnStruct(d);
    currentModeInfo(d);
    return 0;
  }

  if (yc->generalFlags & 
      (CANNA_YOMI_CHIKUJI_MODE | CANNA_YOMI_BASE_CHIKUJI)) {
    /* 「心は逐次だった」のであれば、逐次モードに戻す */
    yc->generalFlags |= CANNA_YOMI_CHIKUJI_MODE;
    yc->generalFlags &= ~CANNA_YOMI_BASE_CHIKUJI;
    /* ヌルステータスに戻す */
    yc->status &= CHIKUJI_NULL_STATUS;
  }

  tanMuhenkan(d, -1);
  makeYomiReturnStruct(d);
  currentModeInfo(d);
  return 0;
}

int
TanDeletePrevious(d)
uiContext	d;
{
  yomiContext yc = (yomiContext)d->modec;
  int i, j, l = -1, ret = 0;
#ifndef WIN
  wchar_t tmpbuf[ROMEBUFSIZE];
#else
  wchar_t *tmpbuf = (wchar_t *)malloc(sizeof(wchar_t) * ROMEBUFSIZE);
  if (!tmpbuf) {
    return ret;
  }
#endif

  if (yc->id != YOMI_CONTEXT) {
    ret = TanMuhenkan(d);
    goto return_ret;
  }

  if ((yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) &&
      !cannaconf.BackspaceBehavesAsQuit) {
    ret = ChikujiTanDeletePrevious(d);
    goto return_ret;
  }
  else {
    if (cannaconf.keepCursorPosition) {
      for (i = 0, l = 0; i <= yc->curbun; i++) {
	if (RkwGoTo(yc->context, i) == -1
	    || (j = RkwGetYomi(yc->context, tmpbuf, ROMEBUFSIZE)) == -1) {
	  l = -1;
	  break;
	}
	l += j;
      }
    }
    yc->status &= CHIKUJI_NULL_STATUS;
    tanMuhenkan(d, l);
    makeYomiReturnStruct(d);
    currentModeInfo(d);
    ret = 0;
  }
 return_ret:
#ifdef WIN
  (void)free((char *)tmpbuf);
#endif
  return ret;
}

#if 0
/*
  doTanKakutei -- 確定させる動作をする

  retval 0 -- 問題無く確定した。
         1 -- 確定したらなくなった。
        -1 -- エラー？
 */

static
doTanKakutei(d, yc)
uiContext	d;
yomiContext yc;
{
  if ((yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) &&
      (yc->cStartp < yc->kEndp)) {
    (void)RomajiFlushYomi(d, (wchar_t *)0, 0);
  }

  return 0;
}
#endif /* 0 */

void
finishTanKakutei(d)
uiContext d;
{
  yomiContext yc = (yomiContext)d->modec;
  int autoconvert = yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE;

#ifdef DO_RENGO_LEARNING
#define RENGOBUFSIZE 256

  /* This will not be defined when WIN is defined.  So I don't care
     about the local array located below.  1996.6.5 kon */

  if (RengoGakushu && hc->nbunsetsu > 1) { /* 連語学習をしようかなぁ */
    RkLex  lex[2][RENGOBUFSIZE];
    wchar_t yomi[2][RENGOBUFSIZE];
    wchar_t kanji[2][RENGOBUFSIZE];
    wchar_t word[1024], *w;
    unsigned char xxxx[ROMEBUFSIZE];
    int    nword[2], wlen;

    *(w = word) = (wchar_t) '\0';
    wlen = 1024;

    RkwGoTo(hc->context, 0);
    nword[0] = RkwGetLex(hc->context, lex[0], RENGOBUFSIZE);
    yomi[0][0] =
      (wchar_t) '\0'; /* yomi[current][0]の真理値 ≡ RkwGetYomiしたか */

    for (i = 1 ; i < hc->nbunsetsu ; i++) {
      int current, previous, mighter;

      current = i % 2;
      previous = 1 - current;

      nword[current] = 0;
      if ( !nword[previous] ) {
	nword[previous] = RkwGetLex(hc->context, lex[previous], RENGOBUFSIZE);
      }
      RkwRight(hc->context);

      if (nword[previous] == 1) {
	nword[current] = RkwGetLex(hc->context, lex[current], RENGOBUFSIZE);
	yomi[current][0] = (wchar_t) '\0';
	if (((lex[previous][0].ylen <= 3 && lex[previous][0].klen == 1) ||
	     (lex[current][0].ylen <= 3 && lex[current][0].klen == 1)) &&
	    (lex[current][0].rownum < R_K5 ||
	     R_NZX < lex[current][0].rownum)) {
	  if ( !yomi[previous][0] ) {
	    RkwLeft(hc->context);
	    RkwGetYomi(hc->context, yomi[previous], RENGOBUFSIZE);
	    RkwGetKanji(hc->context, kanji[previous], RENGOBUFSIZE);
	    RkwRight(hc->context);
	  }
	  RkwGetYomi(hc->context, yomi[current], RENGOBUFSIZE);
	  RkwGetKanji(hc->context, kanji[current], RENGOBUFSIZE);

	  WStrncpy(yomi[previous] + lex[previous][0].ylen,
		   yomi[current], lex[current][0].ylen);
	  yomi[previous][lex[previous][0].ylen + lex[current][0].ylen] =
	    (wchar_t) '\0';

	  WStrncpy(kanji[previous] + lex[previous][0].klen,
		   kanji[current], lex[current][0].klen);
	  kanji[previous][lex[previous][0].klen + lex[current][0].klen] =
	    (wchar_t) '\0';

#ifdef NAGASADEBUNPOUWOKIMEYOU
	  if (lex[previous][0].klen >= lex[current][0].klen) {
	    /* 前の漢字の長さ       >=    後ろの漢字の長さ */
	    mighter = previous;
	  }
	  else {
	    mighter = current;
	  }
#else /* !NAGASADEBUNPOUWOKIMEYOU */
	  mighter = current;
#endif /* !NAGASADEBUNPOUWOKIMEYOU */
	  WStrcpy(w, yomi[previous]);
	  printf(xxxx, " #%d#%d ", lex[mighter][0].rownum,
		 lex[mighter][0].colnum);
	  MBstowcs(w + WStrlen(w), xxxx, wlen - WStrlen(w));
	  WStrcat(w, kanji[previous]);
	  wlen -= (WStrlen(w) + 1); w += WStrlen(w) + 1; *w = (wchar_t) '\0';
	}
      }
    }
  }
#endif /* DO_RENGO_LEARNING */

  if (RkwEndBun(yc->context, cannaconf.Gakushu ? 1 : 0) == -1) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
  }

#ifdef DO_RENGO_LEARNING
  if (RengoGakushu && yc->nbunsetsu > 1) { /* 連語学習をしようかなぁ */
    for (w = word ; *w ; w += WStrlen(w) + 1) {
      RkwDefineDic(yc->context, RengoGakushu, w);
    }
  }
#endif /* DO_RENGO_LEARNING */

  if (autoconvert) {
    yc->status &= CHIKUJI_NULL_STATUS;
    yc->ys = yc->ye = yc->cStartp = yc->cRStartp = 0;
    clearHenkanContext(yc);
    yc->kEndp = yc->rEndp = yc->kCurs = yc->rCurs =
      yc->cStartp = yc->cRStartp = 
	yc->rStartp = yc->kRStartp = 0;
    yc->kAttr[0] = yc->rAttr[0] = SENTOU;
    yc->kana_buffer[0] = yc->romaji_buffer[0] = 0;
/*    d->kanji_status_return->info |= KanjiEmptyInfo; 多分要らないので.. */
    d->current_mode = yc->curMode = yc->myEmptyMode;
  }
  yc->minorMode = getBaseMode(yc);
  
  /* 単候補状態から読みに戻るときには無条件にmarkを先頭に戻す */
  yc->nbunsetsu = 0;
  yc->cmark = yc->pmark = 0;

  abandonContext(d, yc);

  if (yc->savedFlags & CANNA_YOMI_MODE_SAVED) {
    restoreFlags(yc);
  }
}

TanKakutei(d)
uiContext d;
{
  return YomiKakutei(d);
}

/*
 * 漢字候補を確定させ、ローマ字をインサートする
 *
 * renbun-continue が t のときは、実際には確定しないので処理が
 * 面倒だったりする。
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */

static TanKakuteiYomiInsert pro((uiContext));

static int
TanKakuteiYomiInsert(d)
uiContext d;
{
  yomiContext yc = (yomiContext)d->modec;
  tanContext tan;

  if (cannaconf.RenbunContinue || cannaconf.ChikujiContinue) {
    d->nbytes = 0;
    for (tan = (tanContext)yc ; tan->right ; tan = tan->right)
      /* bodyless 'for' */;
    yc = (yomiContext)0; /* 念のため */
    d->modec = (mode_context)tan;
    setMode(d, tan, 1);

    if (tan->id == YOMI_CONTEXT) {
      yc = (yomiContext)tan;

      if (yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) {
	/* 逐次なら普通に続けるだけだからなあ */
	yc->minorMode = CANNA_MODE_ChikujiTanMode;
	d->current_mode = yc->curMode = &cb_mode;
	currentModeInfo(d);
	yc->status &= ~CHIKUJI_OVERWRAP;
	if (yc->kCurs != yc->kEndp) {
	  yc->rStartp = yc->rCurs = yc->rEndp;
	  yc->kRStartp = yc->kCurs = yc->kEndp;
	}
	yc->ys = yc->ye = yc->cStartp;
	return YomiInsert(d);
      }
      else { /* 逐次じゃない場合 */
	extern nKouhoBunsetsu;
    
	yc->curbun = yc->nbunsetsu;
	if (doTanBubunMuhenkan(d, yc) < 0) {
	  makeGLineMessageFromString(d, jrKanjiError);
	  return NothingChangedWithBeep(d);
	}
	if (nKouhoBunsetsu) {
	  (void)cutOffLeftSide(d, yc, nKouhoBunsetsu);
	}
      }
    }
    else {
      yc = newFilledYomiContext(tan->next, tan->prevMode);
      /* あり得ない if (tan->right) yc->right = tan->right;
	 yc->right->left = yc; */
      tan->right = (tanContext)yc;
      yc->left = tan;
      d->modec = (mode_context)yc;
      /* d->current_mode = yc->curMode = yc->myEmptyMode; */
    }
  }
  else {
    d->nbytes = YomiKakutei(d);
  }
  /* YomiKakutei(d) で d->modec が変更された可能性があるので再読み込みする */
  yc = (yomiContext)d->modec;

  if (yc->id == YOMI_CONTEXT) {
    yc->minorMode = getBaseMode(yc);
  }
  currentModeInfo(d);
  d->more.todo = 1;
  d->more.ch = d->ch;
  d->more.fnum = 0;    /* 上の ch で示される処理をせよ */
  return d->nbytes;
}


/* cfuncdef

  pos で指定された文節およびそれより後の文節の字種変換情報を
  クリアする。
*/

static int
doTbResize(d, yc, n)
uiContext d;
yomiContext yc;
int n;
{
  int len;

  if (doTanBubunMuhenkan(d, yc) < 0) {
    makeGLineMessageFromString(d, jrKanjiError);
    return NothingChangedWithBeep(d);
  }
  len = yc->kEndp;
  doMuhenkan(d, yc); /* yc から右をみんな無変換にして yc につなげる */
  if (!prepareHenkanMode(d)) {
    makeGLineMessageFromString(d, jrKanjiError);
    makeYomiReturnStruct(d);
    currentModeInfo(d);
    return 0;
  }
  yc->minorMode = CANNA_MODE_TankouhoMode;
  yc->kouhoCount = 0;
  if (doHenkan(d, len + n, (wchar_t *)0) < 0) {
    makeGLineMessageFromString(d, jrKanjiError);
    makeYomiReturnStruct(d);
    currentModeInfo(d);
    return 0;
  }
  currentModeInfo(d);
  makeKanjiStatusReturn(d, yc);
  return 0;
}

/*
 * 文節を伸ばす
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
static TanExtendBunsetsu pro((uiContext));

static int
TanExtendBunsetsu(d)
uiContext	d;
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return enterTanHenkanMode(d, CANNA_FN_Extend);
  }

  d->nbytes = 0;
  yc->kouhoCount = 0;
  if (yc->right) {
    return doTbResize(d, yc, 1);
  }
  if ((yc->nbunsetsu = RkwEnlarge(yc->context)) <= 0) {
    makeRkError(d, "\312\270\300\341\244\316\263\310\302\347\244\313\274\272"
	"\307\324\244\267\244\336\244\267\244\277");
                   /* 文節の拡大に失敗しました */
    return TanMuhenkan(d);
  }
  makeKanjiStatusReturn(d, yc);
  return(d->nbytes);
}

/*
 * 文節を縮める
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
static TanShrinkBunsetsu pro((uiContext));

static int
TanShrinkBunsetsu(d)
uiContext	d;
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return enterTanHenkanMode(d, CANNA_FN_Shrink);
  }

  d->nbytes = 0;
  yc->kouhoCount = 0;

  if (yc->right) {
    return doTbResize(d, yc, -1);
  }

  /* 文節を縮める */
  if ((yc->nbunsetsu = RkwShorten(yc->context)) <= 0) {
    makeRkError(d, "\312\270\300\341\244\316\275\314\276\256\244\313\274\272"
	"\307\324\244\267\244\336\244\267\244\277");
                   /* 文節の縮小に失敗しました */
    return TanMuhenkan(d);
  }
  makeKanjiStatusReturn(d, yc);
  
  return(d->nbytes);
}

#define BUNPOU_DISPLAY

#ifdef BUNPOU_DISPLAY
/*
 * 文法情報をプリントする
 *
 * 引き数	uiContext
 * 戻り値	正常終了時 0	異常終了時 -1
 */
TanPrintBunpou(d)
uiContext	d;
{
  yomiContext yc = (yomiContext)d->modec;
  static wchar_t mesg[512]; /* static! */

  if (yc->id != YOMI_CONTEXT) {
    return enterTanHenkanMode(d, CANNA_FN_ConvertAsHex);
  }

#ifdef notdef
#ifdef DO_GETYOMI
  if (RkwGetYomi(yc->context, buf, 256) == -1) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
      TanMuhenkan(d);
    }
    fprintf(stderr, "カレント候補の読みを取り出せませんでした。\n");
  }
  Wfprintf(stderr, "%s\n", buf);
#endif /* DO_GETYOMI */

  if(RkwGetKanji(yc->context, buf, 256) == -1) {
    if(errno == EPIPE) {
      jrKanjiPipeError();
    }
    jrKanjiError = "\245\253\245\354\245\363\245\310\270\365\312\344\244\362"
	"\274\350\244\352\275\320\244\273\244\336\244\273\244\363\244\307"
	"\244\267\244\277";
                   /* カレント候補を取り出せませんでした */
    return NG;
  }
#endif

  if (RkwGetHinshi(yc->context, mesg, sizeof(mesg) / sizeof(wchar_t)) < 0) {
    jrKanjiError = "\311\312\273\354\276\360\312\363\244\362\274\350\244\352"
	"\275\320\244\273\244\336\244\273\244\363\244\307\244\267\244\277";
                   /* 品詞情報を取り出せませんでした */
    makeGLineMessageFromString(d, jrKanjiError);
    makeKanjiStatusReturn(d, yc);
    return 0;
  }

  makeKanjiStatusReturn(d, yc);
  d->kanji_status_return->info |= KanjiGLineInfo;
  d->kanji_status_return->gline.line = mesg;
  d->kanji_status_return->gline.length = WStrlen(mesg);
  d->kanji_status_return->gline.revPos = 0;
  d->kanji_status_return->gline.revLen = 0;
  d->flags |= PLEASE_CLEAR_GLINE;
  return 0;
}
#endif /* BUNPOU_DISPLAY */

#ifdef MEASURE_TIME
static
TanPrintTime(d)
uiContext	d;
{
  /* MEASURE_TIME will not be defined when WIN is defined.  So I will not
     care about arrays located on stack below.  1996.6.5 kon */
  unsgined char tmpbuf[1024];
  static wchar_t buf[256];
  yomiContext yc = (yomiContext)d->modec;

  ycc->kouhoCount = 0;
  sprintf(tmpbuf, "\312\321\264\271\273\376\264\326 %d [ms]、\244\246\244\301"
	" UI \311\364\244\317 %d [ms]",
	   (yc->proctime) * 50 / 3,
	   (yc->proctime - yc->rktime) * 50 / 3);
               /* 変換時間 %d [ms]、うち UI 部は %d [ms] */
  MBstowcs(buf, tmpbuf, 1024);
  d->kanji_status_return->info |= KanjiGLineInfo;
  d->kanji_status_return->gline.line = buf;
  d->kanji_status_return->gline.length = WStrlen(buf);
  d->kanji_status_return->gline.revPos = 0;
  d->kanji_status_return->gline.revLen = 0;
  d->kanji_status_return->length = -1;
  d->flags |= PLEASE_CLEAR_GLINE;
  return 0;
}
#endif /* MEASURE_TIME */

void
jrKanjiPipeError()
{
  extern defaultContext, defaultBushuContext;

  defaultContext = -1;
  defaultBushuContext = -1;

  makeAllContextToBeClosed(0);

  RkwFinalize();
#if defined(DEBUG) && !defined(WIN)
  if (iroha_debug) {
    fprintf(stderr, "\300\334\302\263\244\254\300\332\244\354\244\277\n");
                    /* 接続が切れた */
  }
#endif
}

/* cfuncdef

  TanBunsetsuMode -- 単候補モードから文節伸ばし縮めモードへ移行する

 */

static TanBunsetsuMode pro((uiContext));

static
TanBunsetsuMode(d)
uiContext	d;
{
  yomiContext yc = (yomiContext)d->modec;

  if (yc->id != YOMI_CONTEXT) {
    return enterTanHenkanMode(d, CANNA_FN_AdjustBunsetsu);
  }
  if (yc->right) {
    doTbResize(d, yc, 0);
    yc = (yomiContext)d->modec;
  }
  if (enterAdjustMode(d, yc) < 0) {
    return TanMuhenkan(d);
  }
  makeKanjiStatusReturn(d, yc);
  currentModeInfo(d);
  return 0;
}

static void
chikujiSetCursor(d, forw)
uiContext d;
int forw;
{
  yomiContext yc = (yomiContext)d->modec;

  if (forw) { /* 一番左へ行く */
    if (yc->nbunsetsu) { /* 文節がある？ */
      gotoBunsetsu(yc, 0);
      moveToChikujiTanMode(d);
    }
    else {
      yc->kRStartp = yc->kCurs = yc->cStartp;
      yc->rStartp = yc->rCurs = yc->cRStartp;
      moveToChikujiYomiMode(d);
    }
  }
  else { /* 一番右へ行く */
    if (yc->cStartp < yc->kEndp) { /* 読みがある？ */
      yc->kRStartp = yc->kCurs = yc->kEndp;
      yc->rStartp = yc->rCurs = yc->rEndp;
      moveToChikujiYomiMode(d);
    }
    else {
      gotoBunsetsu(yc, yc->nbunsetsu - 1);
      moveToChikujiTanMode(d);
    }
  }
}


void
setMode(d, tan, forw)
uiContext d;
tanContext tan;
int forw;
{
  yomiContext yc = (yomiContext)tan;

  d->current_mode = yc->curMode;
  currentModeInfo(d);
  if (tan->id == YOMI_CONTEXT) {
    if (yc->generalFlags & CANNA_YOMI_CHIKUJI_MODE) {
      chikujiSetCursor(d, forw);
    }
    else if (yc->nbunsetsu) {
      if (forw) {
	gotoBunsetsu(yc, 0);
      }
      else {
	gotoBunsetsu(yc, yc->nbunsetsu - 1);
      }
    }
    else /* 読みモード */ if (forw) {
      yc->kCurs = yc->kRStartp = yc->cStartp;
      yc->rCurs = yc->rStartp = yc->cRStartp;
    }
    else {
      yc->kCurs = yc->kRStartp = yc->kEndp;
      yc->rCurs = yc->rStartp = yc->rEndp;
    }
  }
}

int
TbForward(d)
uiContext d;
{
  tanContext tan = (tanContext)d->modec;

  if (tan->right) {
    d->modec = (mode_context)tan->right;
    tan = (tanContext)d->modec;
  }
  else if (cannaconf.CursorWrap && tan->left) {
    while (tan->left) {
      tan = tan->left;
    }
    d->modec = (mode_context)tan;
  }
  else {
    return NothingChanged(d);
  }
  setMode(d, tan, 1);
  makeKanjiStatusReturn(d, (yomiContext)d->modec);
  return 0;
}

int
TbBackward(d)
uiContext d;
{
  tanContext tan = (tanContext)d->modec;

  if (tan->left) {
    d->modec = (mode_context)tan->left;
    tan = (tanContext)d->modec;
  }
  else if (cannaconf.CursorWrap && tan->right) {
    while (tan->right) {
      tan = tan->right;
    }
    d->modec = (mode_context)tan;
  }
  else {
    return NothingChanged(d);
  }
  setMode(d, tan, 0);
  makeKanjiStatusReturn(d, (yomiContext)d->modec);
  return 0;
}

int
TbBeginningOfLine(d)
uiContext d;
{
  tanContext tan = (tanContext)d->modec;

  while (tan->left) {
    tan = tan->left;
  }
  d->modec = (mode_context)tan;
  setMode(d, tan, 1);
  makeKanjiStatusReturn(d, (yomiContext)d->modec);
  return 0;
}

int
TbEndOfLine(d)
uiContext d;
{
  tanContext tan = (tanContext)d->modec;

  while (tan->right) {
    tan = tan->right;
  }
  d->modec = (mode_context)tan;
  setMode(d, tan, 0);
  makeKanjiStatusReturn(d, (yomiContext)d->modec);
  return 0;
}

static TbChooseChar pro((uiContext, int));

static
TbChooseChar(d, head)
uiContext d;
int head;
{
  tanContext tan = (tanContext)d->modec;

  if (!head) {
    int len = WStrlen(tan->kanji);
    tan->kanji[0] = tan->kanji[len - 1];
  }
  tan->yomi[0] = tan->roma[0] = tan->kanji[0];
  tan->yomi[1] = tan->roma[1] = tan->kanji[1] = (wchar_t)0;
  tan->rAttr[0] = SENTOU;
  tan->kAttr[0] = SENTOU | HENKANSUMI;
  tan->rAttr[1] = tan->kAttr[1] = 0;

  makeKanjiStatusReturn(d, (yomiContext)tan);
  return 0;
}

static int
TanChooseChar(d, head)
uiContext d;
int head;
{
  int retval, len;
  yomiContext yc = (yomiContext)d->modec;
#ifndef WIN
  wchar_t xxx[ROMEBUFSIZE];
#else
  wchar_t *xxx;
#endif

  if (yc->id != YOMI_CONTEXT) {
    return TbChooseChar(d, head);
  }
#ifdef WIN
  xxx = (wchar_t *)malloc(sizeof(wchar_t) * ROMEBUFSIZE);
  if (!xxx) {
    return 0;
  }
#endif
  RkwGoTo(yc->context, yc->curbun);
  len = RkwGetKanji(yc->context, xxx, ROMEBUFSIZE);
  if (len >= 0) {
    retval = TanBubunMuhenkan(d);
    if (retval >= 0) {
      tanContext tan;
      yc = (yomiContext)d->modec;
      tan = newTanContext(yc->majorMode, CANNA_MODE_TankouhoMode);
      if (tan) {
	copyYomiinfo2Tan(yc, tan);
	tan->kanji = DUpwstr(xxx + (head ? 0 : len - 1), 1);
	tan->yomi = DUpwstr(yc->kana_buffer, yc->kEndp);
	tan->roma = DUpwstr(yc->romaji_buffer, yc->rEndp);
	tan->kAttr = DUpattr(yc->kAttr, yc->kEndp);
	tan->rAttr = DUpattr(yc->rAttr, yc->rEndp);
	tan->right = yc->right;
	if (tan->right) tan->right->left = tan;
	yc->right = tan;
	tan->left = (tanContext)yc;
	removeCurrentBunsetsu(d, (tanContext)yc);
	makeKanjiStatusReturn(d, (yomiContext)tan);
	goto done;
      }
    }
  }
  retval = NothingChangedWithBeep(d);
 done:
#ifdef WIN
  (void)free((char *)xxx);
#endif
  return retval;
}

static TanChooseHeadChar pro((uiContext));
static TanChooseTailChar pro((uiContext));

static
TanChooseHeadChar(d)
uiContext d;
{
  return TanChooseChar(d, 1);
}

static
TanChooseTailChar(d)
uiContext d;
{
  return TanChooseChar(d, 0);
}

#include	"tanmap.h"
