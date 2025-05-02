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
static char rcs_id[] = "@(#) 102.1 $Id: uldelete.c,v 1.4 2003/09/17 08:50:53 aida_s Exp $";
#endif

#if !defined(NO_EXTEND_MENU)
#include	<errno.h>
#include 	"canna.h"

#ifdef luna88k
extern int errno;
#endif

/*********************************************************************
 *                      wchar_t replace begin                        *
 *********************************************************************/
#ifdef wchar_t
# error "wchar_t is already defined"
#endif
#define wchar_t cannawc

extern exp(int) RkwGetServerVersion pro((int *, int *));
extern exp(int) RkwChmodDic pro((int, char *, int));

static int dicSakujoYomi pro((uiContext)),
           dicSakujoEndBun pro((uiContext)),
           dicSakujoTango pro((uiContext)),
           dicSakujoDictionary pro((uiContext)),
           dicSakujoDo pro((uiContext));

void
freeWorkDic3(tc)
tourokuContext tc;
{
  if (tc->workDic3) {
    free((char *)tc->workDic3);
    tc->workDic3 = (deldicinfo *)0;
  }
}

void
freeWorkDic(tc)
tourokuContext tc;
{
  if (tc->workDic2) {
    free((char *)tc->workDic2);
    tc->workDic2 = (deldicinfo *)0;
  }
  freeWorkDic3(tc);
}

void
freeDic(tc)
tourokuContext tc;
{
  if (tc->udic) {
    wchar_t **p = tc->udic;

    for ( ; *p; p++) {
      WSfree(*p);
    }
    free((char *)tc->udic);
  }
  freeWorkDic(tc);
}

void
freeAndPopTouroku(d)
uiContext d;
{
  tourokuContext tc = (tourokuContext)d->modec;

  freeDic(tc);
  popTourokuMode(d);
  popCallback(d);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 単語削除の読みの入力                                                      *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static
uuSYomiEveryTimeCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  int len, echoLen, revPos;
  wchar_t tmpbuf[ROMEBUFSIZE];

  retval = 0;
  if((echoLen = d->kanji_status_return->length) < 0)
    return(retval);

  if (echoLen == 0) {
    d->kanji_status_return->revPos = 0;
    d->kanji_status_return->revLen = 0;
  }

  /* 取りあえず echoStr が d->genbuf かもしれないので copy しておく */
  WStrncpy(tmpbuf, d->kanji_status_return->echoStr, echoLen);

  revPos = MBstowcs(d->genbuf, "\306\311\244\337?[", ROMEBUFSIZE);
				/* 読み */
  WStrncpy(d->genbuf + revPos, tmpbuf, echoLen);
  *(d->genbuf + revPos + echoLen) = (wchar_t) ']';
  len = revPos + echoLen + 1;
  *(d->genbuf + len) = (wchar_t) '\0';
  d->kanji_status_return->gline.line = d->genbuf;
  d->kanji_status_return->gline.length = len;
  if (d->kanji_status_return->revLen) {
    d->kanji_status_return->gline.revPos =
      d->kanji_status_return->revPos + revPos;
    d->kanji_status_return->gline.revLen = d->kanji_status_return->revLen;
  }
  else { /* 反転領域がない場合 */
    d->kanji_status_return->gline.revPos = len - 1;
    d->kanji_status_return->gline.revLen = 1;
  }
  d->kanji_status_return->info &= ~(KanjiThroughInfo | KanjiEmptyInfo);
  d->kanji_status_return->info |= KanjiGLineInfo;
  echostrClear(d);
  checkGLineLen(d);

  return retval;
}

static
uuSYomiExitCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  tourokuContext tc;

  popCallback(d); /* 読みを pop */

  tc = (tourokuContext)d->modec;

  WStrncpy(tc->yomi_buffer, d->buffer_return, retval);
  tc->yomi_buffer[retval] = (wchar_t)'\0';
  tc->yomi_len = WStrlen(tc->yomi_buffer);

  return dicSakujoTango(d);
}

static
uuSYomiQuitCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  popCallback(d); /* 読みを pop */

  clearYomi(d);

  freeAndPopTouroku(d);
  GlineClear(d);
  currentModeInfo(d);

  return prevMenuIfExist(d);
}

static
dicSakujoYomi(d)
uiContext d;
{
  yomiContext yc;

  d->status = 0;

  yc = GetKanjiString(d, (wchar_t *)NULL, 0,
	       CANNA_NOTHING_RESTRICTED,
	       (int)CANNA_YOMI_CHGMODE_INHIBITTED,
	       (int)CANNA_YOMI_END_IF_KAKUTEI,
	       (CANNA_YOMI_INHIBIT_HENKAN | CANNA_YOMI_INHIBIT_ASHEX |
	       CANNA_YOMI_INHIBIT_ASBUSHU),
	       uuSYomiEveryTimeCatch, uuSYomiExitCatch,
	       uuSYomiQuitCatch);
  if (yc == (yomiContext)0) {
    deleteEnd(d);
    return NoMoreMemory();
  }
  yc->majorMode = CANNA_MODE_ExtendMode;
  yc->minorMode = CANNA_MODE_DeleteDicMode;
  currentModeInfo(d);

  return 0;
}

static
acDicSakujoYomi(d, dn, dm)
uiContext d;
int dn;
mode_context dm;
/* ARGSUSED */
{
  popCallback(d);
  return dicSakujoYomi(d);
}

static
acDicSakujoDictionary(d, dn, dm)
uiContext d;
int dn;
mode_context dm;
/* ARGSUSED */
{
  popCallback(d);
  return dicSakujoDictionary(d);
}

/*
 * マウントされている辞書から WRITE 権のあるものを取り出す
 */
static
wchar_t **
getMountDicName(d, num_return)
uiContext d;
int *num_return;
/* ARGSUSED */
{
  int nmmdic, check, majv, minv;
  struct dicname *p;
  wchar_t **tourokup, **tp;
  extern defaultContext;

  if (defaultContext < 0) {
    if ((KanjiInit() < 0) || (defaultContext < 0)) {
#ifdef STANDALONE
#ifndef CODED_MESSAGE
      jrKanjiError = "かな漢字変換できません";
#else
      jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\307\244\255\244\336\244\273\244\363";
#endif
#else
#ifndef CODED_MESSAGE
      jrKanjiError = "かな漢字変換サーバと通信できません";
#else
      jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\245\265"
	"\241\274\245\320\244\310\304\314\277\256\244\307\244\255\244\336"
	"\244\273\244\363";    /* かな漢字変換サーバと通信できません */
#endif
#endif /* STANDALONE */
      return 0;
    }
  }

  /* サーバの Version によって取り出す辞書を分ける */
  RkwGetServerVersion(&majv, &minv);

  if (canna_version(majv, minv) < canna_version(3, 2)) {
    /* Version3.2 より前のサーバの場合 */
    for (nmmdic = 0, p = kanjidicnames; p; p = p->next) {
      if (p->dicflag == DIC_MOUNTED && p->dictype == DIC_USER) {
        nmmdic++;
      }
    }
  }
  else {
    /* Version3.2 以降のサーバの場合 */
    for (nmmdic = 0, p = kanjidicnames ; p ; p = p->next) {
      if (p->dicflag == DIC_MOUNTED) {
        check = RkwChmodDic(defaultContext, p->name, 0);
        if (check >= 0 && (check & RK_ENABLE_WRITE)) {
          nmmdic++;
        } else {
	  check = RkwChmodDic(defaultContext, p->name, RK_GRP_DIC);
	  if (check >= 0 && (check & RK_ENABLE_WRITE)) {
	    nmmdic++;
	  }
	}
      }
    }
  }

  /* return BUFFER の alloc */
  if ((tourokup = (wchar_t **)calloc(nmmdic + 1, sizeof(wchar_t *)))
                                                  == (wchar_t **)NULL) {
    /* + 1 なのは打ち止めマークをいれるため */
    jrKanjiError = "malloc (getMountDicName) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
                       /* できませんでした */
    return 0;
  }

  if (canna_version(majv, minv) < canna_version(3, 2)) {
    /* Version3.2 より前のサーバの場合 */
    for (tp = tourokup + nmmdic, p = kanjidicnames ; p ; p = p->next) {
      if (p->dicflag == DIC_MOUNTED && p->dictype == DIC_USER) {
        *--tp = WString(p->name);
      }
    }
  }
  else {
    /* Version3.2 以降のサーバの場合 */
    for (tp = tourokup + nmmdic, p = kanjidicnames ; p ; p = p->next) {
      if (p->dicflag == DIC_MOUNTED) {
        check = RkwChmodDic(defaultContext, p->name, 0);
        if (check >= 0 && (check & RK_ENABLE_WRITE)) {
          *--tp = WString(p->name);
        } else {
	  check = RkwChmodDic(defaultContext, p->name, RK_GRP_DIC);
	  if (check >= 0 && (check & RK_ENABLE_WRITE)) {
	    *--tp = WString(p->name);
	  }
        }
      }
    }
  }
  tourokup[nmmdic] = (wchar_t *)0;
  *num_return = nmmdic;

  return tourokup;
}

dicSakujo(d)
uiContext d;
{
  wchar_t **mp, **p;
  tourokuContext tc;
  int num;
  yomiContext yc = (yomiContext)d->modec;

  if (yc->generalFlags & CANNA_YOMI_CHGMODE_INHIBITTED) {
    return NothingChangedWithBeep(d);
  }    
  d->status = 0;

  /* マウントされている辞書で WRITE 権のあるものを取ってくる */
  if ((mp = getMountDicName(d, &num)) != 0) {
    if (getTourokuContext(d) != NG) {
      tc = (tourokuContext)d->modec;

      tc->udic = mp;
      if(!*mp) {
        makeGLineMessageFromString(d, "\303\261\270\354\272\357\275\374"
	"\262\304\307\275\244\312\274\255\275\361\244\254\302\270\272\337"
	"\244\267\244\336\244\273\244\363");           
			/* 単語削除可能な辞書が存在しません */
     
        freeAndPopTouroku(d);
        deleteEnd(d);
        currentModeInfo(d);
        return 0;
      }
      tc->nudic = num;
      return dicSakujoYomi(d);
    }
    for ( p = mp; *p; p++) {
      WSfree(*p);
    }
    free((char *)mp);
  }
  deleteEnd(d);
  return GLineNGReturn(d);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 単語削除の単語の入力                                                      *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
CloseDeleteContext(tc)
tourokuContext tc;
{
  if(tc->delContext >= 0) {
    if (RkwCloseContext(tc->delContext) < 0) {
      if (errno == EPIPE) {
	jrKanjiPipeError();
      }
    }
  }
#ifdef DEBUG
  else
    printf("ERROR: delContext < 0\n");
#endif
}

/*
 * 指定された単語が登録されている辞書を取り出す
 */
static
getEffectDic(tc)
tourokuContext tc;
{
  int workContext, currentkouho, nbunsetsu, nelem = tc->nudic;
  wchar_t **mdic, **cands, **work;
  wchar_t **getIchiranList();
  char dicname[1024], tmpbuf[64];
  RkLex lex[5];
  deldicinfo *dic;

  dic = (deldicinfo *)malloc((nelem + 1) * sizeof(deldicinfo));
  if (dic == (deldicinfo *)NULL) {
#ifndef CODED_MESSAGE
    jrKanjiError = "malloc (getEffectDic) できませんでした";
#else
    jrKanjiError = "malloc (getEffectDic) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
#endif
    return NG;
  }
  tc->workDic2 = dic;

  if ((workContext = RkwCreateContext()) == NG) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
#ifndef CODED_MESSAGE
    jrKanjiError = "辞書検索用コンテクストを作成できませんでした";
#else
    jrKanjiError = "\274\255\275\361\270\241\272\367\315\321\245\263\245\363"
	"\245\306\245\257\245\271\245\310\244\362\272\356\300\256\244\307"
	"\244\255\244\336\244\273\244\363\244\307\244\267\244\277";
#endif
    return NG;
  }

#ifdef STANDALONE
  if ((RkwSetDicPath(workContext, "iroha")) == NG) {
#ifndef CODED_MESSAGE
    jrKanjiError = "辞書ディレクトリを設定できませんでした";
#else
    jrKanjiError = "\274\255\275\361\245\307\245\243\245\354\245\257\245\310\245\352\244\362\300\337\304\352\244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277";
#endif
    CloseDeleteContext(tc);
    return NG;
  }
#endif /* STANDALONE */

  for (mdic = tc->udic; *mdic; mdic++) {
    WCstombs(dicname, *mdic, sizeof(dicname));
    if (RkwMountDic(workContext, dicname, 0) == NG) {
      if (errno == EPIPE) {
        jrKanjiPipeError();
      }
      jrKanjiError = "\274\255\275\361\270\241\272\367\315\321\245\263\245\363"
	"\245\306\245\257\245\271\245\310\244\313\274\255\275\361\244\362"
	"\245\336\245\246\245\363\245\310\244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
              /* 辞書検索用コンテクストに辞書をマウントできませんでした */
      RkwCloseContext(workContext);
      return NG;
    }

    nbunsetsu = RkwBgnBun(workContext, tc->yomi_buffer, tc->yomi_len, 0);
    if (nbunsetsu == 1) {
      if ((cands = getIchiranList(workContext, &nelem, &currentkouho)) != 0) {
        work = cands;
        while (*work) {
          if (WStrcmp(*work, tc->tango_buffer) == 0) {
            dic->name = *mdic;
            if (RkwXfer(workContext, currentkouho) == NG) {
              if (errno == EPIPE)
                jrKanjiPipeError();
              jrKanjiError = "\245\253\245\354\245\363\245\310\270\365\312\344"
			     "\244\362\274\350\244\352\275\320\244\273\244\336"
			     "\244\273\244\363\244\307\244\267\244\277";
               /* カレント候補を取り出せませんでした */
              freeGetIchiranList(cands);
              RkwEndBun(workContext, 0);
              RkwUnmountDic(workContext, dicname);
              RkwCloseContext(workContext);
              return NG;
            }
            if (RkwGetLex(workContext, lex, 5) <= 0) {
              if (errno == EPIPE)
                jrKanjiPipeError();
              jrKanjiError = "\267\301\302\326\301\307\276\360\312\363\244\362"
		"\274\350\244\352\275\320\244\273\244\336\244\273\244\363"
		"\244\307\244\267\244\277";
                /* 形態素情報を取り出せませんでした */
              freeGetIchiranList(cands);
              RkwEndBun(workContext, 0);
              RkwUnmountDic(workContext, dicname);
              RkwCloseContext(workContext);
              return NG;
            }
            sprintf((char *)tmpbuf, "#%d#%d", lex[0].rownum, lex[0].colnum);
            MBstowcs(dic->hcode, tmpbuf, 16);
            dic++;
            break;
          }
          work++;
        }
        freeGetIchiranList(cands);
      }
    }

    if (RkwEndBun(workContext, 0) == NG) {
      if (errno == EPIPE) {
        jrKanjiPipeError();
      }
      jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\316"
	"\275\252\316\273\244\313\274\272\307\324\244\267\244\336\244\267"
	"\244\277";
       /* かな漢字変換の終了に失敗しました */
      RkwUnmountDic(workContext, dicname);
      RkwCloseContext(workContext);
      return NG;
    }

    if (RkwUnmountDic(workContext, dicname) == NG) {
      if (errno == EPIPE) {
        jrKanjiPipeError();
      }
      jrKanjiError = "\274\255\275\361\270\241\272\367\315\321\244\316\274\255"
	"\275\361\244\362\245\242\245\363\245\336\245\246\245\363\245\310"
	"\244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277";
       /* 辞書検索用の辞書をアンマウントできませんでした */
      RkwCloseContext(workContext);
      return NG;
    }
  }

  if (RkwCloseContext(workContext) < 0) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
    jrKanjiError = "\274\255\275\361\270\241\272\367\315\321\244\316\245\263"
	"\245\363\245\306\245\257\245\271\245\310\244\362\245\257\245\355"
	"\241\274\245\272\244\307\244\255\244\336\244\273\244\363\244\307"
	"\244\267\244\277";
     /* 辞書検索用のコンテクストをクローズできませんでした */
    return NG;
  }

  dic->name = (wchar_t *)0;
  tc->nworkDic2 = dic - tc->workDic2;
  return 0;
}

static
uuSTangoExitCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  forichiranContext fc;
  tourokuContext tc;

  popCallback(d); /* 一覧を pop */

  fc = (forichiranContext)d->modec;
  freeGetIchiranList(fc->allkouho);

  popForIchiranMode(d);
  popCallback(d);

  tc = (tourokuContext)d->modec;
  WStrcpy(tc->tango_buffer, d->buffer_return);
  tc->tango_buffer[d->nbytes] = 0;
  tc->tango_len = d->nbytes;

  d->nbytes = 0;

  if (getEffectDic(tc) == NG) {
    freeDic(tc);
    deleteEnd(d);
    return GLineNGReturnTK(d);
  }

  return dicSakujoDictionary(d);
}

static
uuSTangoQuitCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  forichiranContext fc;

  popCallback(d); /* 一覧を pop */

  fc = (forichiranContext)d->modec;
  freeGetIchiranList(fc->allkouho);

  popForIchiranMode(d);
  popCallback(d);

  clearYomi(d);
  return dicSakujoYomi(d);
}

/*
 * 読みを指定された辞書から変換する
 */
static
dicSakujoBgnBun(d, st)
uiContext d;
RkStat *st;
{
  tourokuContext tc = (tourokuContext)d->modec;
  int nbunsetsu;
  char dicname[1024];
  wchar_t **mdic;

  if(!tc) {
#if !defined(DEBUG)
    printf("tc = NULL\n");
#endif
  }
  if(!tc->udic) {
#if !defined(DEBUG)
    printf("tc->udic = NULL\n");
#endif
  }

  if((tc->delContext = RkwCreateContext())== NG) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
    jrKanjiError = "\303\261\270\354\272\357\275\374\315\321\244\316\245\263"
	"\245\363\245\306\245\257\245\271\245\310\244\362\272\356\300\256"
	"\244\307\244\255\244\336\244\273\244\363";
     /* 単語削除用のコンテクストを作成できません */
    return(NG);
  }

#ifdef STANDALONE
  if ((RkwSetDicPath(tc->delContext, "iroha")) == NG) {
#ifndef CODED_MESSAGE
    jrKanjiError = "辞書ディレクトリを設定できませんでした";
#else
    jrKanjiError = "\274\255\275\361\245\307\245\243\245\354\245\257\245\310\245\352\244\362\300\337\304\352\244\307\244\255\244\336\244\273\244\363\244\307\244\267\244\277";
#endif
    CloseDeleteContext(tc);
    return NG;
  }
#endif /* STANDALONE */

  for (mdic = tc->udic; *mdic; mdic++) {
    WCstombs(dicname, *mdic, sizeof(dicname));
    if (RkwMountDic(tc->delContext, dicname, 0) == NG) {
      if (errno == EPIPE) {
        jrKanjiPipeError();
      }
      jrKanjiError = "\303\261\270\354\272\357\275\374\315\321\244\316\274"
	"\255\275\361\244\362\245\336\245\246\245\363\245\310\244\307\244"
	"\255\244\336\244\273\244\363\244\307\244\267\244\277";
        /* 単語削除用の辞書をマウントできませんでした */
      CloseDeleteContext(tc);
      return(NG);
    }
  }

  if((nbunsetsu = RkwBgnBun(tc->delContext, tc->yomi_buffer, tc->yomi_len, 0))
	== -1) {
    if (errno == EPIPE) {
      jrKanjiPipeError();
    }
    jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\313"
	"\274\272\307\324\244\267\244\336\244\267\244\277";
      /* かな漢字変換に失敗しました */
    CloseDeleteContext(tc);
    return(NG);
  }
  
  if(RkwGetStat(tc->delContext, st) == -1) {
    RkwEndBun(tc->delContext, 0); /* 0:学習しない */
    if(errno == EPIPE)
      jrKanjiPipeError();
    jrKanjiError = "\245\271\245\306\245\244\245\277\245\271\244\362\274\350"
	"\244\352\275\320\244\273\244\336\244\273\244\363\244\307\244\267"
	"\244\277";
               /* ステイタスを取り出せませんでした */
    /* アンマウントしてない */
    CloseDeleteContext(tc);
    return(NG);
  }

  return(nbunsetsu);
}

static
dicSakujoEndBun(d)
uiContext d;
{
  tourokuContext tc = (tourokuContext)d->modec;

  if(RkwEndBun(tc->delContext, 0) == -1) {	/* 0:学習しない */
    if(errno == EPIPE)
      jrKanjiPipeError();
    jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\316"
	"\275\252\316\273\244\313\274\272\307\324\244\267\244\336\244\267"
	"\244\277";
       /* かな漢字変換の終了に失敗しました */
    return(NG);
  }

  return(0);
}

static
dicSakujoTango(d)
uiContext d;
{
  tourokuContext tc = (tourokuContext)d->modec;
  forichiranContext fc;
  ichiranContext ic;
  wchar_t **allDelCands, **getIchiranList();
  BYTE inhibit = 0;
  int nbunsetsu, nelem, currentkouho, retval = 0;
  RkStat st;

  if(tc->yomi_len < 1) {
    return canna_alert(d, "\306\311\244\337\244\362\306\376\316\317\244\267"
	"\244\306\244\257\244\300\244\265\244\244", acDicSakujoYomi);
		/* 読みを入力してください */ 
  }

  if((nbunsetsu = dicSakujoBgnBun(d, &st)) == NG) {
    freeDic(tc);
    deleteEnd(d);
    return(GLineNGReturnTK(d));
  }
  if((nbunsetsu != 1) || (st.maxcand == 0)) {
    /* 候補がない */
    if(dicSakujoEndBun(d) == NG) {
      freeDic(tc);
      CloseDeleteContext(tc);
      deleteEnd(d);
      return(GLineNGReturnTK(d));
    }

    makeGLineMessageFromString(d, "\244\263\244\316\306\311\244\337\244\307"
	"\305\320\317\277\244\265\244\354\244\277\303\261\270\354\244\317"
	"\302\270\272\337\244\267\244\336\244\273\244\363");
         /* この読みで登録された単語は存在しません */
    CloseDeleteContext(tc);
    freeAndPopTouroku(d);
    deleteEnd(d);
    currentModeInfo(d);
    return(0);
  }

  /* すべての候補を取り出す */
  if((allDelCands = 
      getIchiranList(tc->delContext, &nelem, &currentkouho)) == 0) {
    freeDic(tc);
    dicSakujoEndBun(d);
    CloseDeleteContext(tc);
    deleteEnd(d);
    return(GLineNGReturnTK(d));
  }

  if (dicSakujoEndBun(d) == NG) {
    freeDic(tc); 
    CloseDeleteContext(tc);
    deleteEnd(d);
    return GLineNGReturnTK(d);
  }
  CloseDeleteContext(tc);

  if(getForIchiranContext(d) == NG) {
    freeDic(tc);
    freeGetIchiranList(allDelCands);
    deleteEnd(d);
    return(GLineNGReturnTK(d));
  }

  fc = (forichiranContext)d->modec;
  fc->allkouho = allDelCands;

  if (!cannaconf.HexkeySelect)
    inhibit |= ((BYTE)NUMBERING | (BYTE)CHARINSERT);
  else
    inhibit |= (BYTE)CHARINSERT;

  fc->curIkouho = currentkouho;	/* 現在のカレント候補番号を保存する */
  currentkouho = 0;	/* カレント候補から何番目をカレント候補とするか */

  /* 候補一覧に移行する */
  if((retval = selectOne(d, fc->allkouho, &fc->curIkouho, nelem, BANGOMAX,
               inhibit, currentkouho, WITHOUT_LIST_CALLBACK,
	       NO_CALLBACK, uuSTangoExitCatch,
	       uuSTangoQuitCatch, uiUtilIchiranTooSmall)) == NG) {
    freeDic(tc);
    freeGetIchiranList(fc->allkouho);
    deleteEnd(d);
    return(GLineNGReturnTK(d));
  }

  ic = (ichiranContext)d->modec;
  ic->majorMode = CANNA_MODE_ExtendMode;
  ic->minorMode = CANNA_MODE_DeleteDicMode;
  currentModeInfo(d);

  if(ic->tooSmall) {
    d->status = AUX_CALLBACK;
    return(retval);
  }

  makeGlineStatus(d);
  /* d->status = EVERYTIME_CALLBACK; */

  return(retval);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 単語削除の辞書一覧                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
getDeleteDic(mc)
mountContext mc;
{
  tourokuContext tc = (tourokuContext)mc->next;
  int i, num = 0;
  deldicinfo *dic, *srcp;

  /* まず、単語削除する辞書の数を数える */
  for (i = 0; mc->mountList[i]; i++) {
    if (mc->mountOldStatus[i] != mc->mountNewStatus[i]) {
      num++;
    }
  }

  dic = (deldicinfo *)malloc((num + 1) * sizeof(deldicinfo));
  if (dic != (deldicinfo *)NULL) {
    tc->workDic3 = dic;

    /* どの辞書から単語を削除するか */
    srcp = tc->workDic2;
    for (i = 0; mc->mountList[i]; i++, srcp++) {
      if (mc->mountOldStatus[i] != mc->mountNewStatus[i]) {
        *dic++ = *srcp;
      }
    }
    dic->name = (wchar_t *)0;
    tc->nworkDic3 = dic - tc->workDic3;
    return 0;
  }
  jrKanjiError ="malloc (uuSDicExitCatch) \244\307\244\255\244\336\244\273"
	"\244\363\244\307\244\267\244\277";
                 /* できませんでした */
  return NG;
}


static
uuSDicExitCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  mountContext mc;
  tourokuContext tc;

  d->nbytes = 0;

  popCallback(d); /* 一覧を pop */

  mc = (mountContext)d->modec;

  if (getDeleteDic(mc) == NG) {
    popMountMode(d);
    popCallback(d);
    tc = (tourokuContext)d->modec;
    freeDic(tc);
    deleteEnd(d);
    return GLineNGReturnTK(d);
  }

  popMountMode(d);
  popCallback(d);

  tc = (tourokuContext)d->modec;
  /* 辞書が選択されなかったときは、メッセージを出し、
     何かのキーが入力されたら、 辞書選択に戻る。     */
  if (tc->nworkDic3 == 0) {
    return canna_alert(d, "\274\255\275\361\244\362\301\252\302\362\244\267"
	"\244\306\244\257\244\300\244\265\244\244", acDicSakujoDictionary);
		/* 辞書を選択してください */
  }

  return dicSakujoDo(d);
}

static
uuSDicQuitCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  popCallback(d); /* 一覧を pop */

  popMountMode(d);
  popCallback(d);

  freeWorkDic((tourokuContext)d->modec);
  return dicSakujoTango(d);
}

static
dicSakujoDictionary(d)
uiContext d;
{
  tourokuContext tc = (tourokuContext)d->modec;
  mountContext mc;
  ichiranContext ic;
  deldicinfo *work;
  BYTE inhibit = 0;
  int retval, i, upnelem = tc->nworkDic2;
  char *dicLbuf, **dicLp, *wptr;
  BYTE *soldp, *snewp;
  wchar_t *xxxx[100];

  retval = d->nbytes = 0;
  d->status = 0;

  if (upnelem == 1) {
    work
      = (deldicinfo *)malloc((1 /* upnelem(==1) */ + 1) * sizeof(deldicinfo));
    if (work != (deldicinfo *)NULL) {
      tc->workDic3 = work;
      *work++ = *tc->workDic2; /* 構造体の代入 */
      work->name = (wchar_t *)0;
      tc->nworkDic3 = 1; /* work - tc->workDic3 == 1 */
      return dicSakujoDo(d);
    }
    jrKanjiError = "malloc (dicSakujoDictionary) \244\307\244\255\244\336"
	"\244\273\244\363\244\307\244\267\244\277";
                 /* できませんでした */
    freeDic(tc);
    deleteEnd(d);
    return GLineNGReturnTK(d);
  }

  if ((dicLbuf = (char *)malloc(ROMEBUFSIZE)) != (char *)NULL) {
    if ((dicLp = (char **)calloc(upnelem + 1, sizeof(char *)))
                                               != (char **)NULL) {
      wptr = dicLbuf;
      for (work = tc->workDic2; work->name; work++) {
        i = WCstombs(wptr, work->name, ROMEBUFSIZE);
        wptr += i;
        *wptr++ = '\0';
      }
      for (wptr = dicLbuf, i = 0; i < upnelem ; i++) {
        dicLp[i] = wptr;
        while (*wptr++)
          /* EMPTY */
          ;
      }
      dicLp[i] = (char *)NULL;

      /* 現在の状態はすべて off にしておく */
      if ((soldp = (BYTE *)calloc(upnelem + 1, sizeof(BYTE)))
                                               != (BYTE *)NULL) {
        if ((snewp = (BYTE *)calloc(upnelem + 1, sizeof(BYTE)))
                                                 != (BYTE *)NULL) {
          if ((retval = getMountContext(d)) != NG) {
            mc = (mountContext)d->modec;
            mc->mountOldStatus = soldp;
            mc->mountNewStatus = snewp;
            mc->mountList = dicLp;

            /* selectOnOff を呼ぶための準備 */

            mc->curIkouho = 0;
            if (!cannaconf.HexkeySelect)
              inhibit |= ((BYTE)NUMBERING | (BYTE)CHARINSERT);
            else
              inhibit |= (BYTE)CHARINSERT;

            retval = setWStrings(xxxx, mc->mountList, 0);
            if (retval == NG) {
              popMountMode(d);
              popCallback(d);
              deleteEnd(d);
              return GLineNGReturnTK(d);
            }
            if ((retval = selectOnOff(d, xxxx, &mc->curIkouho, upnelem,
		            BANGOMAX, 0, mc->mountOldStatus,
		            NO_CALLBACK, uuSDicExitCatch, 
                            uuSDicQuitCatch, uiUtilIchiranTooSmall)) == NG) {
              popMountMode(d);
              popCallback(d);
              deleteEnd(d);
              return GLineNGReturnTK(d);
            }

            ic = (ichiranContext)d->modec;
            ic->majorMode = CANNA_MODE_ExtendMode;
            ic->minorMode = CANNA_MODE_DeleteDicMode;
            currentModeInfo(d);

            /* 候補一覧行が狭くて候補一覧が出せない */
            if (ic->tooSmall) {
              jrKanjiError = "\274\255\275\361\260\354\315\367\315\321\244\316"
		"\311\375\244\254\266\271\244\244\244\316\244\307\274\255"
		"\275\361\244\316\301\252\302\362\244\254\244\307\244\255"
		"\244\336\244\273\244\363";
                /* 辞書一覧用の幅が狭いので辞書の選択ができません */
              ichiranFin(d);
              popCallback(d); /* OnOff をポップ */
              popMountMode(d);
              popCallback(d);
              currentModeInfo(d);
              freeDic(tc);
              deleteEnd(d);
              return GLineNGReturnTK(d);
            }

            makeGlineStatus(d);
            /* d->status = ICHIRAN_EVERYTIME; */

            return(retval);
          }
          free((char *)snewp);
        }
        free((char *)soldp);
      }
      free((char *)dicLp);
    }
    free(dicLbuf);
  }
  jrKanjiError = "malloc (dicSakujoDictionary) \244\307\244\255\244\336"
	"\244\273\244\363\244\307\244\267\244\277";
     /* できませんでした */
  freeDic(tc);
  deleteEnd(d);
  return GLineNGReturnTK(d);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 単語削除                                                                  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static
uuSDeleteYesCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  tourokuContext tc;
  char dicname[1024];
  wchar_t *WStraddbcpy();
  deldicinfo *dic;
  int bufcnt, l;
  extern defaultContext;

  deleteEnd(d);
  popCallback(d); /* yesNo をポップ */

  tc = (tourokuContext)d->modec;

  if(defaultContext == -1) {
    if((KanjiInit() < 0) || (defaultContext == -1)) {
#ifdef STANDALONE
#ifndef CODED_MESSAGE
      jrKanjiError = "かな漢字変換できません";
#else
      jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\244\307\244\255\244\336\244\273\244\363";
#endif
#else
#ifndef CODED_MESSAGE
      jrKanjiError = "かな漢字変換サーバと通信できません";
#else
      jrKanjiError = "\244\253\244\312\264\301\273\372\312\321\264\271\245\265"
	"\241\274\245\320\244\310\304\314\277\256\244\307\244\255\244\336"
	"\244\273\244\363";
#endif
#endif
      freeAndPopTouroku(d);
      return(GLineNGReturn(d));
    }
  }

  /* 辞書から単語を削除する */
  /* 単語削除用の一行を作る(各辞書共通) */
  WStraddbcpy(d->genbuf, tc->yomi_buffer, ROMEBUFSIZE);
  l = WStrlen(tc->yomi_buffer);
  d->genbuf[l] = (wchar_t)' ';
  l += 1;
  for (dic = tc->workDic3; dic->name; dic++) {
    /* 単語削除用の一行を作る(各辞書固有) */
    WStrcpy(d->genbuf + l, dic->hcode);
    bufcnt = l + WStrlen(dic->hcode);
    d->genbuf[bufcnt] = (wchar_t)' ';
    bufcnt += 1;
    WStraddbcpy(d->genbuf + bufcnt, tc->tango_buffer, 
                                                 ROMEBUFSIZE - bufcnt);

    WCstombs(dicname, dic->name, sizeof(dicname));
    if (RkwDeleteDic(defaultContext, dicname, d->genbuf) == NG) {
      if (errno == EPIPE)
        jrKanjiPipeError();
      MBstowcs(d->genbuf, "\303\261\270\354\272\357\275\374\244\307\244\255"
	"\244\336\244\273\244\363\244\307\244\267\244\277", 512);
		/* 単語削除できませんでした */
      goto close;
    }
    if (cannaconf.auto_sync) {
      RkwSync(defaultContext, dicname);
    }
  }

  /* 削除の完了を表示する */
  l = MBstowcs(d->genbuf, "\241\330", ROMEBUFSIZE);
			/* 『 */
  WStrcpy(d->genbuf + l, tc->tango_buffer);
  l += WStrlen(tc->tango_buffer);
  l += MBstowcs(d->genbuf + l, "\241\331(", ROMEBUFSIZE - l);
				/* 』 */
  WStrcpy(d->genbuf + l, tc->yomi_buffer);
  l += WStrlen(tc->yomi_buffer);
  l += MBstowcs(d->genbuf + l, ")\244\362\274\255\275\361 ", ROMEBUFSIZE - l);
			/* を辞書 */
  dic = tc->workDic3;
  WStrcpy(d->genbuf + l, dic->name);
  l += WStrlen(dic->name);
  for (dic++; dic->name; dic++) {
    l += MBstowcs(d->genbuf + l, " \244\310 ", ROMEBUFSIZE - l);
				/* と */
    WStrcpy(d->genbuf + l, dic->name);
    l += WStrlen(dic->name);
  }
  l += MBstowcs(d->genbuf + l, " \244\253\244\351\272\357\275\374\244\267"
	"\244\336\244\267\244\277", ROMEBUFSIZE - l);
			/* から削除しました */

 close:
  makeGLineMessage(d, d->genbuf, WStrlen(d->genbuf));

  freeAndPopTouroku(d);

  currentModeInfo(d);

  return(0);
}

static
uuSDeleteQuitCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  tourokuContext tc = (tourokuContext)env;

  popCallback(d); /* yesNo をポップ */

  if (tc->nworkDic2 == 1) {
    freeWorkDic(tc);
    return dicSakujoTango(d);
  }
  freeWorkDic3(tc);
  return dicSakujoDictionary(d);
}

static
uuSDeleteNoCatch(d, retval, env)
uiContext d;
int retval;
mode_context env;
/* ARGSUSED */
{
  popCallback(d); /* yesNo をポップ */

  freeAndPopTouroku(d);
  deleteEnd(d);
  currentModeInfo(d);

  GlineClear(d);

  return(retval);
}

static
dicSakujoDo(d)
uiContext d;
{
  tourokuContext tc = (tourokuContext)d->modec;
  int l;
  deldicinfo *dic;

  l = MBstowcs(d->genbuf, "\241\330", ROMEBUFSIZE);
			/* 『 */
  WStrcpy(d->genbuf + l, tc->tango_buffer);
  l += WStrlen(tc->tango_buffer);
  l += MBstowcs(d->genbuf + l, "\241\331(", ROMEBUFSIZE - l);
				/* 』 */
  WStrcpy(d->genbuf + l, tc->yomi_buffer);
  l += WStrlen(tc->yomi_buffer);
  l += MBstowcs(d->genbuf + l, ")\244\362\274\255\275\361 ", ROMEBUFSIZE - l);
			/* を辞書 */
  dic = tc->workDic3;
  WStrcpy(d->genbuf + l, dic->name);
  l += WStrlen(dic->name);
  for (dic++; dic->name; dic++) {
    l += MBstowcs(d->genbuf + l, " \244\310と ", ROMEBUFSIZE - l);
				/* と */
    WStrcpy(d->genbuf + l, dic->name);
    l += WStrlen(dic->name);
  }
  l += MBstowcs(d->genbuf + l, " \244\253\244\351\272\357\275\374\244\267"
	"\244\336\244\271\244\253?(y/n)", ROMEBUFSIZE - l);
		/* から削除しますか */
  if (getYesNoContext(d,
	     NO_CALLBACK, uuSDeleteYesCatch,
	     uuSDeleteQuitCatch, uuSDeleteNoCatch) == NG) {
    freeDic(tc);
    deleteEnd(d);
    return(GLineNGReturnTK(d));
  }
  makeGLineMessage(d, d->genbuf, WStrlen(d->genbuf));

  return(0);
}

#ifndef wchar_t
# error "wchar_t is already undefined"
#endif
#undef wchar_t
/*********************************************************************
 *                       wchar_t replace end                         *
 *********************************************************************/

#endif /* !NO_EXTEND_MENU */
