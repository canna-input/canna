/* Copyright (c) 2002 Canna Project. All rights reserved.
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

#if !defined(lint) && !defined(__CODECENTER__)
static char rcsid[] = "$Id: obind.c,v 1.5 2004/03/29 22:11:17 aida_s Exp $";
#endif

#include "cannaconf.h"
#define WARN_REFERENCES_EVAL(sym,msg) WARN_REFERENCES(sym,msg)

#if SUPPORT_OLD_WCHAR

#if defined(CANNA_WCHAR16) && !defined(WCHAR16)
# define WCHAR16
#endif

#include "widedef.h"
#include "canna.h"
#undef wcKanjiString
#undef wcKanjiControl
#undef wcCloseKanjiContext

typedef struct {
    wchar_t *echoStr;		/* local echo string */
    int length;		        /* length of echo string */
    int revPos;                 /* reverse position  */
    int revLen;                 /* reverse length    */
    unsigned long info;		/* その他の情報 */
    wchar_t  *mode;		/* モード情報 */
    struct {
      wchar_t       *line;
      int           length;
      int           revPos;
      int           revLen;
    } gline;			/* 一覧表示のための情報 */
} owcKanjiStatus;

typedef struct {
  int  val;
  wchar_t *buffer;
  int  n_buffer;
  owcKanjiStatus *ks;
} owcKanjiStatusWithValue;

typedef struct {
  char *client_data;
  int (*callback_func) pro((char *, int, wchar_t **, int, int *));
} owcListCallbackStruct;

typedef canna_uint16_t cannawc16;
typedef canna_uint32_t cannawc32;

extern int howToReturnModeInfo;
extern char *context_table;

exp(int) wcKanjiString pro((const int, const int, wchar_t *, const int,
			    owcKanjiStatus *));
exp(int) wcKanjiControl pro((const int, const int, char *));
exp(int) wcCloseKanjiContext pro((const int, owcKanjiStatusWithValue *));

static size_t
Oldwcsnlen(owcs, maxlen)
const wchar_t *owcs;
size_t maxlen;
{
  const wchar_t *p, *endp;
  for (p = owcs, endp = owcs + maxlen; *p && p < endp; ++p)
    ;
  return endp - p;
}

static size_t
wc32stowc16s(wc16s, wc32s, len)
cannawc16 *wc16s;
const cannawc32 *wc32s;
{
  unsigned int i;
  for (i = 0; i < len && wc32s[i]; i++) {
    cannawc32 wc32 = wc32s[i];
    cannawc32 wc16;
    switch (wc32 >> 28) {
    case 0:
      /* ASCII */
      wc16 = wc32 & 0x7f;
      break;
    case 1:
      /* 半角カナ */
      wc16 = 0x80 | (wc32 & 0x7f);
      break;
    case 2:
      /* 外字 */
      wc16 = 0x8000 | ((wc32 & 0x3f80) << 1) | (wc32 & 0x7f);
      break;
    case 3:
      /* 漢字 */
      wc16 = 0x8080 | ((wc32 & 0x3f80) << 1) | (wc32 & 0x7f);
      break;
    }
    wc16s[i] = (cannawc16)wc16;
  }
  if (i < len)
    wc16s[i] = (cannawc16)0;
  return i;
}

static size_t
wc16stowc32s(wc32s, wc16s, len)
cannawc32 *wc32s;
const cannawc16 *wc16s;
{
  unsigned int i;
  for (i = 0; i < len && wc16s[i]; i++) {
    cannawc32 wc16 = (cannawc32)wc16s[i];
    cannawc32 wc32;
    switch (wc16 & 0x8080) {
    case 0x0000:
      /* ASCII */
      wc32 = wc16 & 0x7f;
      break;
    case 0x0080:
      /* 半角カナ */
      wc32 = (1 << 28) | (wc16 & 0x7f);
      break;
    case 0x8000:
      /* 外字 */
      wc32 = (2 << 28) | ((wc16 & 0x7f00) >> 1) | (wc16 & 0x7f);
      break;
    case 0x8080:
      /* 漢字 */
      wc32 = (3 << 28) | ((wc16 & 0x7f00) >> 1) | (wc16 & 0x7f);
      break;
    }
    wc32s[i] = wc32;
  }
  if (i < len)
    wc32s[i] = (cannawc32)0;
  return i;
}

static size_t
OldwcstoWCs(cwcs, owcs, len)
cannawc *cwcs;
const wchar_t *owcs;
size_t len;
{
  if (sizeof(wchar_t) == sizeof(cannawc)) {
    unsigned int i;
    for (i = 0; i < len && owcs[i]; ++i)
      cwcs[i] = (cannawc)owcs[i];
    if (i < len)
      cwcs[i] = (cannawc)0;
    return i;
  } else if (sizeof(wchar_t) == 4 /* && sizeof(cannawc) == 2 */)
    return wc32stowc16s((cannawc16 *)cwcs, (const cannawc32 *)owcs, len);
  else /* if (sizeof(wchar_t) == 2 && sizeof(cannawc) == 4) */
    return wc16stowc32s((cannawc32 *)cwcs, (const cannawc16 *)owcs, len);
}

static size_t
WCstoOldwcs(owcs, cwcs, len)
wchar_t *owcs;
const cannawc *cwcs;
size_t len;
{
  if (sizeof(wchar_t) == sizeof(cannawc)) {
    unsigned int i;
    for (i = 0; i < len && cwcs[i]; ++i)
      owcs[i] = (wchar_t)cwcs[i];
    if (i < len)
      owcs[i] = (cannawc)0;
    return i;
  } else if (sizeof(wchar_t) == 4 /* && sizeof(cannawc) == 2 */)
    return wc16stowc32s((cannawc32 *)owcs, (const cannawc16 *)cwcs, len);
  else /* if (sizeof(wchar_t) == 2 && sizeof(cannawc) == 4) */
    return wc32stowc16s((cannawc16 *)owcs, (const cannawc32 *)cwcs, len);
}

static int
StoreWCtoOldwc(wbuf, wbuflen, wks, owbuf, maxowbuf, owks, ch, nbytes)
const cannawc *wbuf;
int wbuflen;
const wcKanjiStatus *wks;
wchar_t *owbuf;
int maxowbuf;
owcKanjiStatus *owks;
wchar_t ch;
int nbytes;
{
  int ret, totallen = 0, len;
  wchar_t *p, *endp;
  static wchar_t *inbuf = 0;
  static int inbufsize = 0;

  /* info */

  owks->info = wks->info;
    
  /* 結果 */

  if (owks->info & KanjiThroughInfo) {
    if (nbytes)
      owbuf[0] = ch;
    ret = nbytes;
  }
  else {
    if (wbuflen <= 0)
      ret = 0;
    else {
      size_t fixlen = (wbuflen < maxowbuf) ? wbuflen : maxowbuf;
      ret = (int)WCstoOldwcs(owbuf, wbuf, fixlen);
      if (/* ret >= 0 && */ ret < maxowbuf)
	owbuf[ret] = (wchar_t)0;
    }
    if (owks->info & KanjiYomiInfo && wbuflen >= 0 && maxowbuf >= ret + 1) {
      const cannawc *ep;
      len = WCstoOldwcs(owbuf + ret + 1, wbuf + wbuflen + 1,
			maxowbuf - ret - 1);
      ep = wbuf + wbuflen + 1;
      while (*ep) ep++;
      if (maxowbuf >= ret + 1 + len + 1)
	WCstoOldwcs(owbuf + ret + 1 + len + 1, ep + 1,
		      maxowbuf - ret - 1 - len - 1);
    }
  }

  /* 大きさに注意 */
  if (wks->length > 0)
    totallen = wks->length + 1;
  if (wks->info & KanjiModeInfo)
    totallen += WStrlen(wks->mode) + 1;
  if (wks->info & KanjiGLineInfo)
    totallen += wks->gline.length + 1;

  if (inbufsize < totallen) {
    inbufsize = totallen; /* inbufsize will be greater than 0 */
    if (inbuf) free(inbuf);
    inbuf = (wchar_t *)malloc(inbufsize * sizeof(wchar_t));
    if (!inbuf) {
      inbufsize = 0;
      jrKanjiError = "\245\341\245\342\245\352\244\254\302\255\244\352\244\336\244\273\244\363";
                     /* メモリが足りません */
      return -1;
    }
  }

  p = inbuf;
  endp = inbuf + inbufsize;

  if (wks->length < 0) {
    owks->length = -1;
  }
  else {
    /* エコー文字 */

    owks->length = owks->revLen = owks->revPos = 0;

    if (wks->length > 0) {
      owks->echoStr = p;
      if (wks->revPos > 0) {
	len = owks->revPos = WCstoOldwcs(p, wks->echoStr, wks->revPos);
	p += len;
      }
      if (wks->revLen > 0) {
	len = owks->revLen 
	  = WCstoOldwcs(p, wks->echoStr + wks->revPos, wks->revLen);
	p += len;
      }
      len = 0;
      if (wks->length - wks->revPos - wks->revLen > 0) {
	len = WCstoOldwcs(p, wks->echoStr + wks->revPos + wks->revLen,
			    wks->length - wks->revPos - wks->revLen);
	p += len;
      }
      owks->length = owks->revLen + owks->revPos + len;
      *p++ = (wchar_t)0;
    }
  }

  /* モード表示 */

  if (wks->info & KanjiModeInfo) {
    len = WCstoOldwcs(p, wks->mode, endp - p - 1);
    owks->mode = p;
    p[len] = (wchar_t)0;
    p += len + 1;
  }

  /* 一覧行表示 */

  if (wks->info & KanjiGLineInfo) {
    owks->gline.length = owks->gline.revLen = owks->gline.revPos = 0;

    if (wks->gline.length > 0) {
      owks->gline.line = p;
      if (wks->gline.revPos > 0) {
	len = owks->gline.revPos 
	  = WCstoOldwcs(p, wks->gline.line, wks->gline.revPos);
	p += len;
      }
      if (wks->gline.revLen > 0) {
	len = owks->gline.revLen
	  = WCstoOldwcs(p, wks->gline.line + wks->gline.revPos, wks->gline.revLen);
	p += len;
      }
      len = 0;
      if (wks->gline.length - wks->gline.revPos - wks->gline.revLen > 0) {
	len = WCstoOldwcs(p,
	    wks->gline.line + wks->gline.revPos + wks->gline.revLen,
	    wks->gline.length - wks->gline.revPos - wks->gline.revLen);
	p += len;
      }
      owks->gline.length = owks->gline.revLen + owks->gline.revPos + len;
      *p++ = (wchar_t)0;
    }
  }
  return ret;
}

static int
owcLookupKanji2(dpy, win, buffer_return, n_buffer, nbytes, functionalChar,
	      kanji_status_return)
unsigned int dpy, win;
int functionalChar, nbytes;
wchar_t *buffer_return;
int n_buffer;
owcKanjiStatus *kanji_status_return;
{
  int ret;
  wcKanjiStatus wks;
  wchar_t ch;
  cannawc *inbuf = NULL;
  int inbufsize;
  int r;

  /* 内部バッファをアロケートする */
  inbufsize = n_buffer; /* inbufsize will be greater than 0 */
  inbuf = (cannawc *)malloc(inbufsize * sizeof(cannawc));
  if (!inbuf) {
    inbufsize = 0;
    jrKanjiError = "\245\341\245\342\245\352\244\254\302\255\244\352\244\336\244\273\244\363";
		   /* メモリが足りません */
    return -1;
  }

  OldwcstoWCs(inbuf, buffer_return, nbytes ? nbytes : 1);
  ch = buffer_return[0];
  ret = XwcLookupKanji2(dpy, win, inbuf, inbufsize, nbytes, functionalChar,
			&wks);
  if (ret < 0) {
    free(inbuf);
    return ret;
  }
  if (ret >= inbufsize)
    ret = inbufsize - 1;
  inbuf[ret] = (cannawc)0;

  r = StoreWCtoOldwc(inbuf, ret, &wks,
		      buffer_return, n_buffer, kanji_status_return,
		      ch, nbytes);
  free(inbuf);
  return r;
}
		      

int
owcListCallback(client_data, func, items, nitems, cur_item)
char *client_data;
int func;
cannawc **items;
int nitems, *cur_item;
{
  const owcListCallbackStruct *owlistcb;
  int r = -1;
  wchar_t **owitems = NULL;
  wchar_t *owbuf = NULL;
  wchar_t *owp;
  size_t buflen = 0;
  int i;

  owlistcb = (const owcListCallbackStruct *)client_data;
  if (!items) /* CANNA_LIST_Insert sets 'nitems' to the pressed key (!=0) */
    return owlistcb->callback_func(owlistcb->client_data,
	func, NULL, nitems, cur_item);
  for (i = 0; i < nitems; i++)
    buflen += WStrlen(items[i]) + 1;
  owbuf = (wchar_t *)malloc(buflen * sizeof(wchar_t));
  owitems = (wchar_t **)malloc((nitems + 1) * sizeof(wchar_t **));
  if (!owbuf || !owitems)
    goto last;	/* XXX: 単に-1を返していいのか？ */
  owp = owbuf;
  for (i = 0; i < nitems; i++) {
    size_t len = WCstoOldwcs(owp, items[i], owbuf + buflen - owp);
    owitems[i] = owp;
    owp += len + 1;	/* バッファは常に足りていてヌル終端がある */
  }
  owitems[nitems] = NULL;
  r = owlistcb->callback_func(owlistcb->client_data,
      func, owitems, nitems, cur_item);
last:
  free(owbuf);
  free(owitems);
  return r;
}

static int
owcKanjiControl2(display, window, request, arg)
unsigned int display, window, request;
BYTE *arg;
{
  int ret = -1, len1, len2;
  wcKanjiStatusWithValue wksv;
  wcKanjiStatus wks;
  owcKanjiStatusWithValue *ksvarg = (owcKanjiStatusWithValue *)arg;
  wchar_t *owarg = (wchar_t *)arg;
  jrListCallbackStruct list_cb;
  wchar_t ch;
  cannawc *arg2, *wbuf, *wbuf1, *wbuf2;

  arg2 = (cannawc *)malloc(sizeof(cannawc) * 256);
  wbuf = (cannawc *)malloc(sizeof(cannawc) * 320);
  wbuf1 = (cannawc *)malloc(sizeof(cannawc) * 320);
  wbuf2 = (cannawc *)malloc(sizeof(cannawc) * 320);
  if (!arg2 || !wbuf || !wbuf1 || !wbuf2) {
    free(arg2);
    free(wbuf);
    free(wbuf1);
    free(wbuf2);
    return ret;
  }

  wksv.buffer = wbuf;
  wksv.n_buffer = 320;
  wksv.ks = &wks;

  switch (request) {
  case KC_DO: /* val と buffer_return に入れるタイプ */
    wbuf[0] = (cannawc)ksvarg->buffer[0];
    /* FALLTHROUGH */
  case KC_CHANGEMODE: /* val を与えるタイプ */
    wksv.val = ksvarg->val;
    goto withksv;
  case KC_STOREYOMI: /* echoStr と length と mode を与えるタイプ */
    /* まず mode をワイドにしてみよう */
    if (((owcKanjiStatusWithValue *)arg)->ks->mode) {
      len2 = OldwcstoWCs(wbuf2, ksvarg->ks->mode, 319);
      wbuf2[len2] = (wchar_t)0;
      wks.mode = wbuf2;
    } else
      wks.mode = NULL;
    /* FALLTHROUGH */
  case KC_DEFINEKANJI: /* echoStr と length を与えるタイプ */
    /* echoStr をワイドにして与えてみよう */
    len1 = OldwcstoWCs(wbuf1, ksvarg->ks->echoStr, 319);
    wbuf1[len1] = (wchar_t)0;
    wks.echoStr = wbuf1;
    wks.length = len1;
    /* FALLTHROUGH */
  case KC_KAKUTEI: /* ただ単に与えて返って来るタイプ */
  case KC_KILL:
    goto withksv;
  case KC_CLOSEUICONTEXT:
    goto closecont;
  case KC_QUERYMODE: /* querymode */
    ret = XwcKanjiControl2(display, window, request, (BYTE *)arg2);
    if (!ret) {
      switch (howToReturnModeInfo) {
      case ModeInfoStyleIsString:
	WCstoOldwcs(owarg, arg2, 256); /* XXX */
	break;
      case ModeInfoStyleIsBaseNumeric:
        owarg[2] = (wchar_t)arg2[2];
      case ModeInfoStyleIsExtendedNumeric:
	owarg[1] = (wchar_t)arg2[1];
      case ModeInfoStyleIsNumeric:
	owarg[0] = (wchar_t)arg2[0];
	break;
      }
    }
    goto return_ret;
  case KC_SETLISTCALLBACK: /* dirty, dirty hack */
    /* list_cbはKC_setListCallbackでd->elistcbに引っ越す */
    list_cb.client_data = (char *)arg;
    list_cb.callback_func = &owcListCallback;
    ret = XwcKanjiControl2(display, window, request, (char *)&list_cb);
    goto return_ret;
  default: /* 新ワイドと変わらないもの */
    ret = XwcKanjiControl2(display, window, request, arg);
    goto return_ret;
  }
 withksv:
  ch = ksvarg->buffer[0];
  ret = XwcKanjiControl2(display, window, request, (BYTE *)&wksv);
  if (ret < 0) {
    goto return_ret;
  }
  else {
    wksv.buffer[ret] = (wchar_t)0;
    ksvarg->val = StoreWCtoOldwc(wksv.buffer, wksv.val, wksv.ks,
		   ksvarg->buffer, ksvarg->n_buffer, ksvarg->ks,
		   ch, ksvarg->val);
    ret = ksvarg->val;
    goto return_ret;
  }
 closecont:
  ch = ksvarg->buffer[0];
  ret = XwcKanjiControl2(display, window, request, (BYTE *)&wksv);
  if (ret < 0) {
    goto return_ret;
  }
  else {
    wksv.val = 0;
    ksvarg->val = StoreWCtoOldwc(wksv.buffer, wksv.val, wksv.ks,
		   ksvarg->buffer, ksvarg->n_buffer, ksvarg->ks,
		   ch, ksvarg->val);
    goto return_ret;
  }
 return_ret:
  free(wbuf2);
  free(wbuf1);
  free(wbuf);
  free(arg2);
  return ret;
}

exp(int)
wcKanjiString(context_id, ch, buffer_return, nbuffer, kanji_status_return)
int context_id, ch, nbuffer;
wchar_t *buffer_return;
owcKanjiStatus  *kanji_status_return;
{
  *buffer_return = (wchar_t)ch;

  return owcLookupKanji2((unsigned int)0, (unsigned int)context_id,
		       buffer_return, nbuffer,
		       1/* byte */, 1/* functional char*/,
		       kanji_status_return);
}

exp(int)
wcKanjiControl(context, request, arg)
int context;
int request;
char *arg;
{
  return owcKanjiControl2((unsigned int)0, (unsigned int)context,
			request, (BYTE *)arg);
}

exp(int)
wcCloseKanjiContext(context,ksva)
int context;
owcKanjiStatusWithValue *ksva;
{
  /* really working? */
  context_table[context] = 0;
  return  owcKanjiControl2(0, context, KC_CLOSEUICONTEXT, (BYTE *)ksva);
}

#define WARNSTR "warning: libcanna: using old wchar API; consider to use new one."
#else /* !SUPPORT_OLD_WCHAR */

#include "canna.h"
#undef wcKanjiString
#undef wcKanjiControl
#undef wcCloseKanjiContext

exp(int) wcKanjiString pro((const int, const int, cannawc *, const int,
			    wcKanjiStatus *));
exp(int) wcKanjiControl pro((const int, const int, char *));
exp(int) wcCloseKanjiContext pro((const int, wcKanjiStatusWithValue *));

static int
wc_unsupported()
{
  jrKanjiError = "Old wide character API is disabled on this environment.";
  return -1;
}

exp(int)
wcKanjiString(context_id, ch, buffer_return, nbuffer, kanji_status_return)
int context_id, ch, nbuffer;
cannawc *buffer_return;
wcKanjiStatus  *kanji_status_return;
{
  return wc_unsupported();
}

exp(int)
wcKanjiControl(context, request, arg)
int context;
int request;
char *arg;
{
  return wc_unsupported();
}

exp(int)
wcCloseKanjiContext(context,ksva)
int context;
wcKanjiStatusWithValue *ksva;
{
  return  wc_unsupported();
}

#define WARNSTR "warning: libcanna: old API is disabled; consider to use new one."
#endif /* !SUPPORT_OLD_WCHAR */

WARN_REFERENCES_EVAL(wcKanjiString, WARNSTR);
WARN_REFERENCES_EVAL(wcKanjiControl, WARNSTR);
WARN_REFERENCES_EVAL(wcCloseKanjiContext, WARNSTR);
/* vim: set sw=2: */
