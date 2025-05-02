/* Copyright 1994 NEC Corporation, Tokyo, Japan.
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
static char rcsid[] = "$Id: bun.c,v 1.6 2003/09/21 10:16:49 aida_s Exp $";
#endif

/* LINTLIBRARY */

#include "RKintern.h"

#define NEED_DEF
#ifdef RkSetErrno
#undef RkSetErrno
#define RkSetErrno(no)
#endif

#ifdef OVERRUN_DEBUG
#define	OVERRUN_MARGIN	100
#else
#define	OVERRUN_MARGIN	0
#endif

#define	STRCMP(d, s)	strcmp((char *)(d), (char *)(s))
extern	void	usncopy();

#ifdef RK_LOG
#include	<stdio.h>
static FILE *
openLogFile(cxnum)
int cxnum;
{
    char file[128];
    FILE *fp;
    sprintf(file, "/tmp/henkan%03d.log", cxnum);
    fp = fopen(file, "a");
    return fp;
}

static char *
nword2str(cx, w, yomi)
struct RkContext *cx;
struct nword *w;
Wchar *yomi;
{
    static unsigned char msg[RK_LINE_BMAX];
    static unsigned char eyomi[RK_LINE_BMAX];
    struct nword *words[RK_CONC_NMAX], **p, *wp;
    int msg_idx = 0;
    char *hinsi;
    Wchar *kanji, *_RkGetKanji();
    unsigned char *ekanji, *ustoeuc();

    for (wp = w, p = words; wp; wp = wp->nw_left) 
	*p++ = wp;

    while (p-- > words) {
        int yomi_len, hinsi_len;

	wp = *p;
	if (!wp->nw_left)
	    continue;
	kanji = _RkGetKanji(wp, yomi + wp->nw_left->nw_ylen, cx->concmode);
	ekanji = ustoeuc(kanji, wp->nw_klen - wp->nw_left->nw_klen,
			 msg + msg_idx, RK_LINE_BMAX - msg_idx);
	msg_idx = ekanji - msg;
        ustoeuc(yomi + wp->nw_left->nw_ylen, 
                wp->nw_ylen - wp->nw_left->nw_ylen, eyomi, RK_LINE_BMAX);
        yomi_len = strlen(eyomi);
	hinsi = RkGetGramName(cx->gram->gramdic, wp->nw_rowcol);
	hinsi_len = strlen(hinsi);
        if (msg_idx + 1 + yomi_len + hinsi_len + 2 >= RK_LINE_BMAX)
	    break;
        sprintf(msg + msg_idx, "/%s[%s]", eyomi, hinsi);
        msg_idx += 1 + yomi_len + hinsi_len + 2;
    }
    msg[msg_idx] = 0;

    return msg;
}

static
dumpBunq(cx, from, end, log, fp)
struct RkContext *cx;
int from;
unsigned end;
int log; /* 0 候補変更 1 変換開始 2 確定 3 文節長変更 */
FILE *fp;
{
    int i;
    struct nstore *store = cx->store;
    struct nbun *bun = store->bunq + from;
    struct henkanlog *l = store->hlog;

    for (i = 0; i < from; i++) l = l->next;
    if (log & 1) {
	/* 文節長変更情報の初期化 */
	char **prev = store->blog;
	int nprev = store->nblog;
	store->nblog = end;
	store->blog = (char **)malloc(sizeof(char *) * end);
	/* この文節までの情報は全部コピー */
	if (log == 3)
	    for (i = 0; i <= from; i++)
		store->blog[i] = prev[i];
	/* 残りは捨てる */
	for (i = from + (log == 3); i < nprev; i++)
	    if (prev[i]) free(prev[i]);
	if (prev) free(prev);
	/* 以降は初期化 */
	for (i = from + (log == 3); i < end; i++)
	    store->blog[i] = NULL;
    }
    for ( i = from; i < end; i++, bun++) {
	struct nword *w;
	char *henkan;
	int n = bun->nb_curcand;
	/* 現在候補の取得 */
	for ( w = bun->nb_cand; w; w = w->nw_next ) {
	    if ( CanSplitWord(w) && bun->nb_curlen == w->nw_ylen )
		if ( n-- <= 0 ) break;
	}
	if (log & 1) {
	    /* ログを取る領域を確保 */
	    struct henkanlog *p = NULL, *q;
	    if (q = l->next) {
		p = q->next;
		if (q->henkan) {
		    if (log == 3 && from == i && !store->blog[i])
			store->blog[i] = q->henkan;
		    else
			if (q->henkan[0]) free(q->henkan);
		}
		free(q);
	    }
	    l = l->next = (struct henkanlog *)
		    malloc(sizeof(struct henkanlog));
	    l->next = p;
	} else if (log == 2) l = l->next;

	if (!w) {
	    if (log & 1) l->henkan = "";
	    else if (log == 2) {
		unsigned char msg[RK_LINE_BMAX];
		unsigned char *ekanji, *ustoeuc();
		ustoeuc(store->yomi + bun->nb_yoff, bun->nb_curlen,
			msg, RK_LINE_BMAX);
		fprintf(fp, "リテラル %s\n", msg);
	    }
	} else {
	    henkan = nword2str(cx, w, store->yomi + bun->nb_yoff);
	    if (log & 1) {
		l->henkan = (char *)malloc(strlen(henkan) + 1);
		strcpy(l->henkan, henkan);
	    }
	    else if (log == 2) {
		if (store->blog[i]) {
		    fprintf(fp, "誤変換 文節長変更 %s -> %s\n",
			    store->blog[i], henkan);
		}
		if (STRCMP(l->henkan, henkan))
		    fprintf(fp, "誤変換 %s -> %s\n", l->henkan, henkan);
		else
		    fprintf(fp, "正解 %s\n", henkan);
	    }
	}
    } 
    fprintf(fp, "\n");
    fflush(fp);
}
#endif

static void
freeBunStorage(s)
     struct nstore *s;
{
  if (s) {
    if (s->yomi)
      (void)free((char *)(s->yomi-OVERRUN_MARGIN));
    if (s->bunq)
      (void)free((char *)(s->bunq-OVERRUN_MARGIN));
    if (s->xq)
      (void)free((char *)(s->xq-OVERRUN_MARGIN));
    if (s->xqh)
      (void)free((char *)(s->xqh-OVERRUN_MARGIN));
    (void)free((char *)s);
  }
}

static struct nstore *
allocBunStorage(len)
     unsigned	len;
{
  struct nstore	*s;

  s = (struct nstore *)malloc((unsigned)sizeof(struct nstore));
  if (s) {
    Wchar	*p, *q, pat;
    int			i;

    s->yomi = (Wchar *)0;
    s->bunq = (struct nbun *)0;
    s->xq = (struct nqueue *)0;
    s->xqh = (struct nword **)0;
    s->nyomi = (unsigned)0;
    s->maxyomi = (unsigned)len;
#ifdef RK_LOG
    s->nblog = 0;
    s->blog = NULL;
    s->hlog = &s->dmi;
    s->dmi.next = NULL; s->dmi.henkan = NULL;
#endif

    s->yomi = (Wchar *)calloc((s->maxyomi+1+2*OVERRUN_MARGIN), sizeof(Wchar));
    s->maxbunq = (unsigned)len;
    s->maxbun = (unsigned)0;
    s->curbun = 0;
    s->bunq = (struct nbun *)calloc((unsigned)(s->maxbunq+1+2*OVERRUN_MARGIN),
				    sizeof(struct nbun));
    s->maxxq = len;
    s->xq = (struct nqueue *)calloc((unsigned)(s->maxxq+1+2*OVERRUN_MARGIN),
				    sizeof(struct nqueue));
    s->xqh = (struct nword **)calloc((unsigned)(s->maxxq+1+2*OVERRUN_MARGIN),
				     sizeof(struct nword *));
    if (!s->yomi || !s->bunq || !s->xq || !s->xqh) {
      RkSetErrno(RK_ERRNO_ENOMEM);
      freeBunStorage(s);
      return (struct nstore *)0;
    }
    s->yomi += OVERRUN_MARGIN;
    s->bunq += OVERRUN_MARGIN;
    s->xq += OVERRUN_MARGIN;
    s->xqh += OVERRUN_MARGIN;
    p = (Wchar*)&s->yomi[0];
    q = (Wchar*)&s->yomi[s->maxyomi+1];
    for (i = 0; pat = (Wchar)~i, i < OVERRUN_MARGIN; i++)
      p[-i-1] = q[i] = pat;
    p = (Wchar*)&s->bunq[0];
    q = (Wchar*)&s->bunq[s->maxbunq+1];
    for (i = 0; pat = (Wchar)~i, i < OVERRUN_MARGIN; i++)
      p[-i-1] = q[i] = pat;
    p = (Wchar*)&s->xq[0];
    q = (Wchar*)&s->xq[s->maxxq+1];
    for (i = 0; pat = (Wchar)~i, i < OVERRUN_MARGIN; i++)
      p[-i-1] = q[i] = pat;
    p = (Wchar*)&s->xqh[0];
    q = (Wchar*)&s->xqh[s->maxxq+1];
    for (i = 0; pat = (Wchar)~i, i < OVERRUN_MARGIN; i++)
      p[-i-1] = q[i] = pat;
    s->word_in_use = 0;
  };
  if (!s) /* EMPTY */RkSetErrno(RK_ERRNO_ENOMEM);
  return s;
}

struct nstore	*
_RkReallocBunStorage(src, len)
     struct nstore	*src;
     unsigned		len;
{
  struct nstore	*dst = allocBunStorage(len);

  if (dst) {
    int		i;
    
    if (src->yomi) {
      for (i = 0; i <= (int)src->maxyomi; i++)
	dst->yomi[i] = src->yomi[i];
      (void)free((char *)(src->yomi-OVERRUN_MARGIN));
    };
    dst->nyomi = src->nyomi;
    if (src->bunq) {
      for (i = 0; i <= (int)src->maxbun; i++) 
	dst->bunq[i] = src->bunq[i];
      (void)free((char *)(src->bunq-OVERRUN_MARGIN));
    };
    dst->maxbun = src->maxbun;
    dst->curbun = src->curbun;
    if (src->xq) {
      for (i = 0; i <= src->maxxq; i++)
	dst->xq[i] = src->xq[i];
      (void)free((char *)(src->xq-OVERRUN_MARGIN));
    };
    if (src->xqh) {
      for (i = 0; i <= src->maxxq; i++)
	dst->xqh[i] = src->xqh[i];
      (void)free((char *)(src->xqh-OVERRUN_MARGIN));
    };
    dst->word_in_use = src->word_in_use;
    (void)free((char *)src);
    return(dst);
  }
  return((struct nstore *)0);
}

static struct nbun *
getCurrentBun(store)
     struct nstore	*store;
{
  if (store && 0 <= store->curbun && store->curbun < (int)store->maxbun) 
    return &store->bunq[store->curbun];
  return (struct nbun *)0;
}

/* RkBgnBun
 *	renbunsetu henkan wo kaishi surutameno shokisettei wo okonau
 * reuturns:
 *	# >=0	shoki bunsetsu no kosuu
 *	-1	shoki ka sippai
 *		RK_ERRNO_ECTXNO
 *		RK_ERRNO_EINVAL
 *		RK_ERRNO_ENOMEM
 */
#ifdef __STDC__
int
RkwBgnBun(
     int	cx_num,
     Wchar	*yomi,
     int	n,
     int	kouhomode
)
#else
int
RkwBgnBun(cx_num, yomi, n, kouhomode)
     int	cx_num;
     Wchar	*yomi;
     int	n;
     int	kouhomode;
#endif
{
  struct RkContext	*cx;
  unsigned long		mask1, mask2;
  int			asset = 0;

  if (!(cx = RkGetContext(cx_num))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(-1);
  }
  if (IS_XFERCTX(cx)) {
    RkSetErrno(0);
    return(-1);
  }
  for (mask1 = (unsigned long) kouhomode, mask2 = 0L;
                           mask1; mask1 >>= RK_XFERBITS) {
    if ((mask1 & (unsigned long)RK_XFERMASK) == (unsigned long)RK_CTRLHENKAN) {
      mask1 >>= RK_XFERBITS;
      asset = 1;
      break;
    }
    mask2 = (mask2 << RK_XFERBITS) | ((unsigned long)RK_XFERMASK);
  }
  if (!(cx->store = allocBunStorage((unsigned)n))) {
    RkSetErrno(RK_ERRNO_ENOMEM);
    return(-1);
  }
  cx->flags |= (unsigned)CTX_XFER;
  cx->concmode = (RK_CONNECT_WORD
		  | (asset ? ((int)mask1 & ~RK_TANBUN) : 
		     (RK_MAKE_KANSUUJI | RK_MAKE_WORD | RK_MAKE_EISUUJI)));
  cx->kouhomode = ((unsigned long)kouhomode) & mask2;
  if (yomi) {
    int	i;
	
    if (n <= 0) {
      RkSetErrno(RK_ERRNO_EINVAL);
      RkwEndBun(cx_num, 0);
      return(-1);
    };
    for (i = 0; i < n; i++)
      cx->store->yomi[i] = yomi[i];
    cx->store->yomi[n] = 0;
    cx->store->nyomi = n;
    cx->store->bunq[0].nb_yoff = 0;
    i = _RkRenbun2(cx, mask1 & RK_TANBUN ? n : 0);
#ifdef RK_LOG
    {
	FILE *fp = openLogFile(cx_num);
	dumpBunq(cx, 0, cx->store->maxbun, 1, fp);
	fclose(fp);
    }
#endif
    return(i);
  } else {
    cx->concmode |= RK_MAKE_WORD;
    cx->flags |= (unsigned) CTX_XAUT;
    if (n < 0) {
      RkSetErrno(RK_ERRNO_EINVAL);
      RkwEndBun(cx_num, 0);
      return(-1);
    };
    return(0);
  }
}

/* RkEndBun
 *	bunsetsu henkan wo shuuryou suru
 *	hituyou ni oujite, henkan kekka wo motoni gakushuu wo okonau 
 *
 *	return	0
 *		-1(RK_ERRNO_ECTX)
 */
#ifdef __STDC__
int
RkwEndBun(
     int	cx_num,
     int	mode
)
#else
int
RkwEndBun(cx_num, mode)
     int	cx_num;
     int	mode;
#endif
{
  struct RkContext	*cx;
  struct nstore		*store;    
  int			i;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store)) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(-1);
  }
  if (mode) {
#define DO_LEARN	1
    if (mode != DO_LEARN) {
      RkSetErrno(RK_ERRNO_EINVAL);
      return -1;
    };
  }
#ifdef RK_LOG
  if (mode) {
      FILE *fp = openLogFile(cx_num);
      dumpBunq(cx, 0, store->maxbun, 2, fp);
      fclose(fp);
  }
#endif
  for (i = 0; i < (int)store->maxbun; i++)
    (void)_RkLearnBun(cx, i, mode);
  if (cx->flags & CTX_XAUT)
    _RkFreeQue(store, 0, store->maxxq + 1);
  cx->concmode &= ~(RK_CONNECT_WORD | RK_MAKE_WORD |
		    RK_MAKE_KANSUUJI | RK_MAKE_EISUUJI);
  _RkEndBun(cx);
  freeBunStorage(store);
  return(0);
}

/* RkRemoveBun
 *	current bunsetu made wo sakujo suru
 *	current bunsetu ha 0 ni naru.
 */

int RkwRemoveBun pro((int, int));

int
RkwRemoveBun(cx_num, mode)
     int	cx_num;
     int	mode;
{
  struct RkContext	*cx;
  struct nstore		*store;
  int			i, c;

  if (!(cx = RkGetXContext(cx_num))
      || !(store = cx->store)
      || !IS_XFERCTX(cx)
      || store->maxbun <= 0) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return -1;
  }
  for (i = 0; i <= store->curbun; i++)
    _RkLearnBun(cx, i, mode);
  c = store->bunq[store->curbun + 1].nb_yoff;
  for (i = store->curbun + 1; i <= (int)store->maxbun; i++) {
    store->bunq[i - store->curbun - 1] = store->bunq[i];
    store->bunq[i - store->curbun - 1].nb_yoff -= c;
  }
  store->nyomi -= c;
  usncopy(store->yomi, store->yomi + c, (unsigned)store->nyomi);
  store->maxbun -= store->curbun + 1;
  store->curbun = 0;
  return(store->maxbun);
}

/* RkSubstYomi
 *	change the contents of hiragana buffer
 * returns:
 *	# bunsetu
 */

int RkwSubstYomi pro((int, int, int, Wchar *, int));

RkwSubstYomi(cx_num, ys, ye, yomi, newLen)
     int	cx_num;
     int	ys, ye;
     Wchar	*yomi;
     int	newLen;
{
  struct RkContext	*cx;
  struct nstore	*store;
  struct nbun	*bun;
  
  if (!(cx = RkGetContext(cx_num))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(-1);
  }
  if (!(store = cx->store) ||
      !(bun = &store->bunq[store->maxbun]) ||
      !IS_XFERCTX(cx) ||
      !IS_XAUTCTX(cx) ||
      !(0 <= ys && ys <= ye && ye <= (int)(store->nyomi - bun->nb_yoff)) ||
      (newLen < 0)) {
    RkSetErrno(RK_ERRNO_EINVAL);
    return -1;
  }
  return _RkSubstYomi(cx, ys, ye, yomi, newLen);
}

/* RkFlushYomi
 *	force to convert the remaining hiragana
 * returns:
 *	# bunsetu
 */

int RkwFlushYomi pro((int));

int
RkwFlushYomi(cx_num)
     int		cx_num;
{
  struct RkContext	*cx;
  if (!(cx = RkGetContext(cx_num)) ||
      !IS_XFERCTX(cx) ||
      !IS_XAUTCTX(cx)) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(-1);
  }
  return(_RkFlushYomi(cx));
}

/* RkResize/RkEnlarge/RkShorten
 *	current bunsetsu no ookisa wo henkou
 */
int
_RkResize(cx_num, len, t)
     int	cx_num;
     int	len;
     int	t;
{
  struct RkContext	*cx;
  struct nbun		*bun;
  struct nstore		*store;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !(bun = getCurrentBun(store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(-1);
  }
  if (t)
    len = HowManyChars(store->yomi + store->bunq[store->curbun].nb_yoff, len);
  if (0 < len && (unsigned)(bun->nb_yoff + len) <= store->nyomi) {
    bun->nb_flags |= RK_REARRANGED;
#ifndef RK_LOG
    return(_RkRenbun2(cx, len));
#else
    {
	int ret_val = _RkRenbun2(cx, len);
	FILE *fp = openLogFile(cx_num);
	dumpBunq(cx, store->curbun, store->maxbun, 3, fp);
	fclose(fp);
	return ret_val;
    }
#endif
  }
  return(store->maxbun);
}

int RkwResize pro((int, int));

int
RkwResize(cx_num, len)
     int	cx_num;
     int	len;
{
  return(_RkResize(cx_num, len, 0));
}

#ifdef __STDC__
int
RkeResize(
     int	cx_num,
     int	len
)
#else
int
RkeResize(cx_num, len)
     int	cx_num;
     int	len;
#endif
{
  return(_RkResize(cx_num, len, 1));
}

int RkwEnlarge pro((int));

int
 RkwEnlarge(cx_num)
     int	cx_num;
{
  struct RkContext	*cx;
  struct nstore		*store;
  struct nbun		*bun;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store)||
      !(bun = getCurrentBun(store))) {
    RkSetErrno(RK_ERRNO_ENOMEM);
    return(0);
  }
  if (store->nyomi > (unsigned)(bun->nb_yoff + bun->nb_curlen) &&
	   store->yomi[bun->nb_yoff + bun->nb_curlen]) {
    bun->nb_flags |= RK_REARRANGED;
#ifdef RK_LOG
    {
	int ret_val = _RkRenbun2(cx, (int)(bun->nb_curlen + 1)); 
	FILE *fp = openLogFile(cx_num);
	dumpBunq(cx, store->curbun, store->maxbun, 3, fp);
	fclose(fp);
	return ret_val;
    }
#else
    return(_RkRenbun2(cx, (int)(bun->nb_curlen + 1)));
#endif
  }
  return(store->maxbun);
}

int RkwShorten pro((int));

int
RkwShorten(cx_num)
     int	cx_num;
{
  struct RkContext	*cx;
  struct nstore		*store;
  struct nbun		*bun;
    
  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !(bun = getCurrentBun(store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return 0;
  }
  if (bun->nb_curlen > 1) {
    bun->nb_flags |= RK_REARRANGED;
#ifdef RK_LOG
    {
	int ret_val = _RkRenbun2(cx, (int)(bun->nb_curlen - 1)); 
	FILE *fp = openLogFile(cx_num);
	dumpBunq(cx, store->curbun, store->maxbun, 3, fp);
	fclose(fp);
	return ret_val;
    }
#else
    return(_RkRenbun2(cx, (int)(bun->nb_curlen - 1)));
#endif
  }
  return(store->maxbun);
}

/* RkStoreYomi
 *	current bunsetu no yomi wo sitei sareta mono to okikaeru
 *	okikaeta noti, saihen kan suru
 */

int RkwStoreYomi pro((int, Wchar *, int));

int
RkwStoreYomi(cx_num, yomi, nlen)
     int	cx_num;
     Wchar	*yomi;
     int	nlen;
{
  unsigned		nmax, omax, cp;
  Wchar			*s, *d, *e;
  int			i, olen, diff;
  struct RkContext	*cx;
  struct nstore		*store;
  struct nbun		*bun;
    
  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !(bun = getCurrentBun(store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return -1;
  }
  if ((nlen && !yomi) || nlen < 0 || uslen(yomi) < nlen) {
    RkSetErrno(RK_ERRNO_EINVAL);
    return -1;
  }
  nmax = store->nyomi + (diff = nlen - (olen = bun->nb_curlen));
  omax = store->nyomi;
  /* nobiru */
  if (nlen > olen) {
    if (!(store = _RkReallocBunStorage(store, store->maxyomi + diff))) {
      RkSetErrno(RK_ERRNO_ENOMEM);
      return -1;
    }
    cx->store = store;
    bun = getCurrentBun(store);
    /* shift yomi */
    s = store->yomi + omax;
    d = store->yomi + nmax;
    e = store->yomi + bun->nb_yoff + olen;
    while (s > e)
      *--d = *--s;
  } else if (nlen < olen) {	/* chizimu */
    s = store->yomi + bun->nb_yoff + olen;
    d = store->yomi + bun->nb_yoff + nlen;
    e = store->yomi + omax;
    while (s < e)
      *d++ = *s++;
  }
  store->yomi[nmax] = (Wchar)0;
  store->nyomi = nmax;
  for (i = store->curbun + 1; i <= (int)store->maxbun; i++) 
    store->bunq[i].nb_yoff += diff;
  cp = store->curbun;
  if (!nlen) {
    _RkFreeBunq(store);
    for (i = store->curbun; i < (int)store->maxbun; i++)
      store->bunq[i] = store->bunq[i + 1];
    store->maxbun--;
    cp = store->curbun;
    if (cp >= store->maxbun && cp > 0)
      cp -= 1;
  } else
    usncopy((store->yomi + bun->nb_yoff), yomi, (unsigned)nlen);
#ifdef RK_LOG
  {
      int ret_val = _RkRenbun2(cx, 0); 
      FILE *fp = openLogFile(cx_num);
      fputs("読みの置換\n", fp);
      dumpBunq(cx, store->curbun, store->maxbun, 1, fp);
      fclose(fp);
      if ((i = ret_val) != -1)
	  store->curbun = cp;
      return(i);
  }
#else
  if ((i = _RkRenbun2(cx, 0)) != -1)
    store->curbun = cp;
  return(i);
#endif
}

/* RkGoTo/RkLeft/RkRight
 * 	current bunsetu no idou
 */

int RkwGoTo pro((int, int));

int
RkwGoTo(cx_num, bnum)
     int	cx_num;
     int	bnum;
{
  struct RkContext	*cx;
  struct nstore	*store;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !store->maxbun) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return 0;
  }
  if ((0 <= bnum) && (bnum < (int)store->maxbun))
    store->curbun = bnum;
  return(store->curbun);
}

#ifdef __STDC__
int
RkwLeft(
     int	cx_num
)
#else
int
RkwLeft(cx_num)
     int	cx_num;
#endif
{
  struct RkContext	*cx;
  struct nstore	*store;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !store->maxbun) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return 0;
  }
  if (--store->curbun < 0)
    store->curbun = store->maxbun - 1;
  return store->curbun;
}

#ifdef __STDC__
int
RkwRight(
     int	cx_num
)
#else
int
RkwRight(cx_num)
     int	cx_num;
#endif
{
  struct RkContext	*cx;
  struct nstore	*store;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !store->maxbun) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return 0;
  }
  if (++store->curbun >= (int)store->maxbun)
    store->curbun = 0;
  return(store->curbun);
}

/* RkXfer/RkNfer/RkNext/RkPrev
 *	current kouho wo henkou
 */
static int
countCand(cx)
     struct RkContext	*cx;
{
  struct nbun		*bun;
  int			maxcand = 0;
  unsigned long		mask;
  
  bun = getCurrentBun(cx->store);
  if (bun) {
    maxcand = bun->nb_maxcand;
    for (mask = cx->kouhomode; mask; mask >>= RK_XFERBITS)
      maxcand++;
  };
  return(maxcand);
}

static int
getXFER(cx, cnum)
     struct RkContext	*cx;
     int		cnum;
{
  struct nbun	*bun = getCurrentBun(cx->store);

  cnum -= ((int)bun->nb_maxcand);
  return(cnum < 0 ? RK_NFER : (cx->kouhomode>>(RK_XFERBITS*cnum))&RK_XFERMASK);
}

#ifdef __STDC__
int
RkwXfer(
     int	cx_num,
     int  	knum
)
#else
int
RkwXfer(cx_num, knum)
     int	cx_num;
     int  	knum;
#endif
{
  struct RkContext	*cx;
  struct nbun		*bun;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(cx->store) ||
      !(bun = getCurrentBun(cx->store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return 0;
  }
  if (0 <= knum && knum < countCand(cx))
    bun->nb_curcand = knum;
  return(bun->nb_curcand);
}

#ifdef __STDC__
int
RkwNfer(
     int	cx_num
)
#else
int
RkwNfer(cx_num)
     int	cx_num;
#endif
{
  struct RkContext	*cx;
  struct nbun	*bun;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(cx->store) ||
      !(bun = getCurrentBun(cx->store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(0);
  }
  return(bun->nb_curcand = bun->nb_maxcand);
}

int RkwNext pro((int));

int
RkwNext(cx_num)
     int	cx_num;
{
  struct RkContext	*cx;
  struct nbun	*bun;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(cx->store) ||
      !(bun = getCurrentBun(cx->store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(0);
  }
  if (++bun->nb_curcand >= (Wchar)countCand(cx))
    bun->nb_curcand = 0;
  return(bun->nb_curcand);
}

int RkwPrev pro((int));

int
RkwPrev(cx_num)
     int	cx_num;
{
  struct RkContext	*cx;
  struct nbun		*bun;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(cx->store) ||
      !(bun = getCurrentBun(cx->store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(0);
  }
  
  if (!bun->nb_curcand)
    bun->nb_curcand = countCand(cx);
  return(--bun->nb_curcand);
}

/* findBranch
 * 	shiteisareta kouho wo fukumu path wo motomeru
 */
static
struct nword *
findBranch(store, cnum)
     struct nstore	*store;	
     int		cnum;
{
  struct nbun		*bun;
  struct nword		*w;
  
  if (!(bun = getCurrentBun(store)) ||
      (0 > cnum) ||
      (cnum >= (int)bun->nb_maxcand))
    return((struct nword *)0);
  for (w = bun->nb_cand; w; w = w->nw_next) {
    if (CanSplitWord(w) && bun->nb_curlen == w->nw_ylen) {
      if (cnum-- <= 0)
	return(w);
    }
  }
  return((struct nword *)0);
}

/* RkGetStat
 */
#ifdef __STDC__
int
RkwGetStat(
     int	cx_num,
     RkStat	*st
)
#else
int
RkwGetStat(cx_num, st)
     int	cx_num;
     RkStat	*st;
#endif
{
  struct RkContext	*cx;
  struct nstore		*store;
  struct nbun		*bun;
  struct nword		*cw, *lw;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !(bun = getCurrentBun(store)) ||
      !st) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return -1;
  }
  /* set up void values */
  st->bunnum = st->candnum = st->maxcand =
    st->diccand = st->ylen = st->klen = st->tlen = 0;
  st->bunnum  = store->curbun;
  st->candnum = bun->nb_curcand;
  
  st->maxcand = countCand(cx);
  st->diccand = bun->nb_maxcand;
  st->ylen    = bun->nb_curlen;
  st->klen    = bun->nb_curlen;
  st->tlen    = 1;
  /* look up the word node containing the current candidate */
  cw = findBranch(store, (int)bun->nb_curcand);
  if (cw) {
    st->klen = st->tlen = 0;
    for (; cw; cw = cw->nw_left) {
      if (!(lw = cw->nw_left))
	break;
      if (cw->nw_klen == lw->nw_klen)
	st->klen += (cw->nw_ylen - lw->nw_ylen);
      else
	st->klen += (cw->nw_klen - lw->nw_klen);
      st->tlen++;
    }
  } else {
    Wchar	*yomi = store->yomi + bun->nb_yoff;
    switch(getXFER(cx, (int)bun->nb_curcand)) {
    default:
    case RK_XFER:
      st->klen = RkwCvtHira((Wchar *)0, 0, yomi, st->ylen);
      break;
    case RK_KFER:
      st->klen = RkwCvtKana((Wchar *)0, 0, yomi, st->ylen);
      break;
    case RK_HFER:
      st->klen = RkwCvtHan((Wchar *)0, 0, yomi, st->ylen);
      break;
    case RK_ZFER:
      st->klen = RkwCvtZen((Wchar *)0, 0, yomi, st->ylen);
      break;
    }
  }
  return 0;
}

/* RkGetStat
 */
#ifdef __STDC__
int
RkeGetStat(
     int	cx_num,
     RkStat	*st
)
#else
int
RkeGetStat(cx_num, st)
     int	cx_num;
     RkStat	*st;
#endif
{
  struct RkContext *cx;
  struct nstore *store;
  Wchar *yomi;
  int res, klen;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
  Wchar kanji[RK_LEN_WMAX+1];
#else
  Wchar *kanji = (Wchar *)malloc(sizeof(Wchar) * (RK_LEN_WMAX + 1));
  if (!kanji) {
    return -1;
  }
#endif
  
  if (!(cx = RkGetXContext(cx_num)) || !(store = cx->store)) {
    RkSetErrno(RK_MSG_ECTXNO);
    res = -1;
    goto return_res;
  }

  yomi = store->yomi + store->bunq[store->curbun].nb_yoff;
  klen = RkwGetKanji(cx_num, kanji, RK_LEN_WMAX + 1);
  res = RkwGetStat(cx_num, st);
  if (res < 0 || klen != st->klen) {
    res = -1;
    goto return_res;
  }
  if (st) {
    st->ylen = HowManyBytes(yomi, st->ylen);
    st->klen = HowManyBytes(kanji, klen);
  }
 return_res:
#ifdef USE_MALLOC_FOR_BIG_ARRAY
  (void)free((char *)kanji);
#endif
  return res;
}

static int addIt pro((struct nword *, Wchar *, 
		      int (*proc)(Wchar *, int, int, Wchar *, Wchar *,
				  RkLex *, struct RkContext *),
		      Wchar *dst, int, int, unsigned long,
		      struct RkContext *));
static int
addIt(cw, key, proc, dst, ind, maxdst, mode, cx)
struct nword *cw;
Wchar *key;
int (*proc) pro((Wchar *, int, int, Wchar *, Wchar *,
		 RkLex *, struct RkContext *));
Wchar *dst;
int ind;
int maxdst;
unsigned long mode;
struct RkContext *cx;
{
  struct nword	*lw;
  Wchar		*y, *_RkGetKanji();
  RkLex		lex;
  
  lw = cw->nw_left;
  if (lw) {
    ind = addIt(lw, key, proc, dst, ind, maxdst, mode, cx);
    y = key + lw->nw_ylen;
    lex.ylen = cw->nw_ylen - lw->nw_ylen;
    lex.klen = cw->nw_klen - lw->nw_klen;
    lex.rownum = cw->nw_rowcol;
#ifdef NEED_DEF
    lex.colnum = cw->nw_rowcol;
#endif
    lex.dicnum = cw->nw_class;
    ind = (*proc)(dst, ind, maxdst, y, _RkGetKanji(cw, y, mode), &lex, cx);
  }
  return ind;
}

static int
getIt(cx, cnum, proc, dst, max)
struct RkContext *cx;
int cnum;
int (*proc) pro((Wchar *, int, int, Wchar *, Wchar *,
		 RkLex *, struct RkContext *));
Wchar *dst;
int max;
{
  struct nstore *store = cx->store;
  struct nbun	*bun;
  struct nword	*w;

  if (!(bun = getCurrentBun(store)) ||
      !(w = findBranch(store, cnum)))
    return(-1);
  return addIt(w, store->yomi + bun->nb_yoff, proc, dst, 0, max,
	       (unsigned long)cx->concmode, cx);
}

/*ARGSUSED*/
static int
addYomi(dst, ind, max, yomi, kanji, lex)
     Wchar	*dst;
     int	ind;
     int	max;
     Wchar	*yomi;
     Wchar	*kanji;
     RkLex	*lex;
{
  int		ylen;
    
  ylen = lex->ylen;
  while (ylen--) {
    if (ind < max) {
      if (dst)
	dst[ind] = *yomi++;
      ind++;
    }
  }
  return ind;
}
/* RkGetYomi
 *	current bunsetu no yomi wo toru
 */

int RkwGetYomi pro((int, Wchar *, int));

int
RkwGetYomi(cx_num, yomi, maxyomi)
     int	cx_num;
     Wchar	*yomi;
     int	maxyomi;
{
  struct RkContext	*cx;
  struct nbun	*bun;
  RkLex	lex;
  int		i;
  struct nstore	*store;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !(bun = getCurrentBun(store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return -1;
  };
  if (!yomi) {
    RkSetErrno(RK_ERRNO_EINVAL);
    return -1;
  };
  lex.ylen = bun->nb_curlen;
  i = addYomi(yomi, 0, maxyomi - 1,
	      store->yomi + bun->nb_yoff,
	      store->yomi+bun->nb_yoff,
	      &lex);
  if (yomi && i < maxyomi)
    yomi[i] = (Wchar)0;
  return i;
}

int RkwGetLastYomi pro((int, Wchar *, int));

int
RkwGetLastYomi(cx_num, yomi, maxyomi)
     int	cx_num;
     Wchar	*yomi;
     int	maxyomi;
{
  struct RkContext	*cx;
  struct nbun	*bun;
  struct nstore	*store;
  int		nyomi;
    
  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store) ||
      !(bun = &store->bunq[store->maxbun])) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return -1;
  }
  if (!(cx->flags & CTX_XAUT) || maxyomi < 0) {
    RkSetErrno(RK_ERRNO_EINVAL);
    return -1;
  }
  nyomi = store->nyomi - bun->nb_yoff;
  if (yomi) {
    usncopy(yomi, store->yomi + bun->nb_yoff, (unsigned)(maxyomi));
    if (nyomi + 1 < maxyomi) {
      yomi[nyomi] = (Wchar)0;
    } else {
      yomi[maxyomi - 1] = (Wchar)0;
    };
  }
  return nyomi;
}

/*ARGSUSED*/
static int
addKanji(dst, ind, max, yomi, kanji, lex, cx)
     Wchar	*dst;
     int	ind;
     int	max;
     Wchar	*yomi;
     Wchar	*kanji;
     RkLex	*lex;
     struct RkContext	*cx; /* ARGSUSED */
{
  int		klen;
  
  klen = lex->klen;
  while (klen-- > 0) {
    if (ind < max) {
      if (dst)
	dst[ind] = *kanji++;
      ind++;
    }
  }
  return ind;
}

static int
getKanji(cx, cnum, dst, maxdst)
     struct RkContext	*cx;
     int		cnum;
     Wchar		*dst;
     int		maxdst;
{
  struct nbun	*bun = getCurrentBun(cx->store);
  Wchar		*yomi;
  int		i, ylen;

  i = getIt(cx, cnum, addKanji, dst, maxdst - 1);
  if (i < 0) {
    yomi = cx->store->yomi + bun->nb_yoff;
    ylen = bun->nb_curlen;
    switch(getXFER(cx, cnum)) {
    default:
    case RK_XFER:
      i = RkwCvtHira(dst, maxdst, yomi, ylen); break;
    case RK_KFER:
      i = RkwCvtKana(dst, maxdst, yomi, ylen); break;
    case RK_HFER:
      i = RkwCvtHan(dst, maxdst, yomi, ylen); break;
    case RK_ZFER:
      i = RkwCvtZen(dst, maxdst, yomi, ylen); break;
    }
  }
  if (dst && i < maxdst)
    dst[i] = (Wchar)0;
  return i;
}

/* RkGetKanji
 *	current bunsetu no kanji tuduri wo toru
 */

int RkwGetKanji pro((int, Wchar *, int));

int
RkwGetKanji(cx_num, dst, maxdst)
     int	cx_num;
     Wchar	*dst;
     int	maxdst;
{
  RkContext	*cx;
  struct nbun	*bun;
  int		i;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(cx->store) ||
      !(bun = getCurrentBun(cx->store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return -1;
  }
  
  i = getKanji(cx, (int)bun->nb_curcand, dst, maxdst);
  if (dst && i < maxdst)
    dst[i] = 0;
  return i;
}

/* RkGetKanjiList
 * 	genzai sentaku sareta kouho mojiretu wo toridasu
 */
#ifdef __STDC__
int
RkwGetKanjiList(
     int	cx_num,
     Wchar	*dst,
     int	maxdst
)
#else
int
RkwGetKanjiList(cx_num, dst, maxdst)
     int	cx_num;
     Wchar	*dst;
     int	maxdst;
#endif
{
  struct RkContext	*cx;
  int			i, len, ind = 0, num = 0;
  int			maxcand;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(cx->store)) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return -1;
  }
  maxcand = countCand(cx);
  for (i = 0; i < maxcand; i++) {
    if (dst)
      len = getKanji(cx, i, dst + ind, maxdst - ind - 1);
    else
      len = getKanji(cx, i, dst, maxdst - ind - 1);
    if (0 < len && ind + len + 1 < maxdst - 1) {
      if (dst)
	dst[ind + len] = (Wchar)0;
      ind += len + 1;
      num++;
    }
  }
  if (dst && ind < maxdst)
    dst[ind] = (Wchar)0;
  return num;
}

/* RkGetLex
 *	current bunsetu no hishi jouhou wo toru
 */
/*ARGSUSED*/
static int
addLex(dst, ind, max, yomi, kanji, lex, cx)
     RkLex	*dst;
     int	ind;
     int	max;
     Wchar	*yomi;
     Wchar	*kanji;
     RkLex	*lex;
     struct RkContext	*cx; /* ARGSUSED */
{
  if (ind + 1 <= max) {
    if (dst)
      dst[ind] = *lex;
    ind++;
  }
  return ind;
}

#ifdef __STDC__
int
RkwGetLex(
     int	cx_num,
     RkLex	*dst,
     int	maxdst
)
#else
int
RkwGetLex(cx_num, dst, maxdst)
     int	cx_num;
     RkLex	*dst;
     int	maxdst;
#endif
{
  RkContext	*cx;
  struct nbun	*bun;
  int	i;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(cx->store) ||
      !(bun = getCurrentBun(cx->store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return -1;
  }
  i = getIt(cx, (int)bun->nb_curcand, addLex, (Wchar *)dst, maxdst - 1);
  if (i < 0) {
    if (dst && 1 < maxdst) {
      dst[0].ylen = bun->nb_curlen;
      dst[0].klen = bun->nb_curlen;
      dst[0].rownum  = cx->gram->P_BB; /* 文節 */
      dst[0].colnum  = cx->gram->P_BB; /* 文節 */
      dst[0].dicnum  = ND_EMP;
    }
    i = 1;
  }
  return i;
}

/* RkeGetLex -- ほぼ RkwGetLex と同じだが長さはバイト長で返る */

#ifdef __STDC__
int
RkeGetLex(
     int	cx_num,
     RkLex	*dst,
     int	maxdst
)
#else
int
RkeGetLex(cx_num, dst, maxdst)
     int	cx_num;
     RkLex	*dst;
     int	maxdst;
#endif
{
  struct RkContext *cx;
  struct nstore *store;
  Wchar *yomi, *kp;
  int nwords, i;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
  Wchar kanji[RK_LEN_WMAX+1];
#else
  Wchar *kanji = (Wchar *)malloc(sizeof(Wchar) * (RK_LEN_WMAX + 1));
  if (!kanji) {
    return -1;
  }
#endif

  if (!(cx = RkGetXContext(cx_num)) ||
      !(store = cx->store)) {
    RkSetErrno(RK_MSG_ECTXNO);
    nwords = -1;
    goto return_nwords;
  }

  yomi = store->yomi + store->bunq[store->curbun].nb_yoff;
  (void)RkwGetKanji(cx_num, kanji, RK_LEN_WMAX + 1);
  kp = kanji;
  nwords = RkwGetLex(cx_num, dst, maxdst);
  if (dst) {
    for (i = 0; i < nwords; i++) {
      int tmp;
      tmp = dst[i].ylen;
      dst[i].ylen = HowManyBytes(yomi, tmp); yomi += tmp;
      tmp = dst[i].klen;
      dst[i].klen = HowManyBytes(kp, tmp); kp += tmp;
    }
  }
 return_nwords:
#ifdef USE_MALLOC_FOR_BIG_ARRAY
  (void)free((char *)kanji);
#endif
  return nwords;
}

/*ARGSUSED*/
static int
addHinshi(dst, ind, max, yomi, kanji, lex, cx)
     Wchar	*dst;
     int	ind;
     int	max;
     Wchar	*yomi;
     Wchar	*kanji;
     RkLex	*lex;
     struct RkContext	*cx;
{
  int	bytes;
  Wchar	*p;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
  Wchar	hinshi[256];
#else
  Wchar *hinshi = (Wchar *)malloc(sizeof(Wchar) * 256);
  if (!hinshi) {
    return ind;
  }
#endif

  if (cx) {
    p = RkUparseGramNum(cx->gram->gramdic, lex->rownum, hinshi, 256);
    if (p) {
      bytes = p - hinshi;
      if (ind + bytes  < max) {
	if (dst)
	  usncopy(dst + ind, hinshi, bytes);
	ind += bytes;
      }
    }
  }
#ifdef USE_MALLOC_FOR_BIG_ARRAY
  (void)free((char *)hinshi);
#endif
  return ind;
}

/* RkGetHinshi
 *	current bunsetu no hinshi mojiretu wo toru
 */

int RkwGetHinshi pro((int, Wchar *, int));

int
RkwGetHinshi(cx_num, dst, maxdst)
     int	cx_num;
     Wchar	*dst;
     int	maxdst;
{
  struct RkContext	*cx;
  struct nbun		*bun;
  int			i;

  if (!(cx = RkGetXContext(cx_num)) ||
      !(cx->store) ||
      !(bun = getCurrentBun(cx->store))) {
    RkSetErrno(RK_ERRNO_ECTXNO);
    return(-1);
  }
  i = getIt(cx, (int)bun->nb_curcand, addHinshi, dst, maxdst - 1);
  if (i < 0) {
    if (dst && 1 < maxdst) 
      dst[0] = (Wchar)0;
    i = 1;
  }
  return(i);
}

#include <sys/types.h>
#include <sys/stat.h>

#define	CloseContext(a)	{if ((a) != cx_num) RkwCloseContext(a);}

int
#ifdef __STDC__
RkwQueryDic(
     int		cx_num,
     char	*dirname,
     char	*dicname,
     struct DicInfo	*status
)
#else
RkwQueryDic(cx_num, dirname, dicname, status)
     int		cx_num;
     char	*dirname;
     char	*dicname;
     struct DicInfo	*status;
#endif
{
  struct RkContext	*cx;
  int			new_cx_num, size;
  unsigned char		*buff;
  char			*file;
  struct DM		*dm;
  struct DF		*df;
  struct stat		st;
    
  if (!(cx = RkGetContext(cx_num))
      || !status || !dirname || !dicname || !dicname[0])
    return(-1);
  size = strlen(dicname) + 1;
  if (!(buff = (unsigned char *)malloc(size)))
    return(-1);
  (void)strcpy((char *)buff, dicname);
  if (*dirname && strcmp(dirname, cx->ddpath[0]->dd_name)
      && strcmp(dirname, (char *)SYSTEM_DDHOME_NAME)) {
    if((new_cx_num = RkwCreateContext()) < 0) {
      (void)free((char *)buff);
      return BADCONT;
    }
    if(RkwSetDicPath(new_cx_num, dirname) < 0) {
      CloseContext(new_cx_num);
      (void)free((char *)buff);
      return NOTALC;
    }
    if (!(cx = RkGetContext(new_cx_num))) {
      (void)free((char *)buff);
      return(-1);
    }
  } else {
    if (!strcmp(dirname, (char *)SYSTEM_DDHOME_NAME))
      dirname = SYSTEM_DDHOME_NAME;
    else
      dirname = cx->ddpath[0]->dd_name;
    new_cx_num = cx_num;
  }
  if (!strcmp(dirname, (char *)SYSTEM_DDHOME_NAME)) {
    if (!(dm = _RkSearchDDP(cx->ddpath, dicname))) {
      CloseContext(new_cx_num);
      (void)free((char *)buff);
      return NOENT;
    }
  } else {
    if (!(dm = _RkSearchUDDP(cx->ddpath, dicname))) {
      CloseContext(new_cx_num);
      (void)free((char *)buff);
      return NOENT;
    }
  }
  df = dm->dm_file;
  if (df) {
    file = _RkCreatePath(df->df_direct, df->df_link);
    if (file) {
      status->di_dic = (unsigned char *)dm->dm_nickname;
      status->di_file = (unsigned char *)df->df_link;
      status->di_form = (DM2TYPE(dm) == DF_TEMPDIC) ? RK_TXT : RK_BIN;
      status->di_kind = dm->dm_class;
      status->di_count = stat(file, &st) >= 0 ? st.st_size : 0;
      status->di_time = stat(file, &st) >= 0 ? st.st_ctime : 0;
      status->di_mode = 0;
      CloseContext(new_cx_num);
      free((char *)file);
      free((char *)buff);
      return(0);
    }
  }
  free((char *)buff);
  CloseContext(new_cx_num);
  return(-1);
}

#define DL_SIZE 1024


int
_RkwSync(cx, dicname)
     struct RkContext	*cx;
     char *dicname;
{
  struct DM	*dm, *qm;

  dm = _RkSearchDicWithFreq(cx->ddpath, dicname, &qm);  

  if (dm)
    return(DST_SYNC(cx, dm, qm));
  else 
    return (-1);
}


int
RkwSync(cx_num, dicname)
     int cx_num;
     char *dicname;
{
  struct RkContext	*cx;
  int ret = -1;
  
  if (!(cx = RkGetContext(cx_num)))
    return (-1);

  if (!dicname || !*dicname) {
    int i, rv;
    char *p;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
    char diclist[DL_SIZE];
#else
    char *diclist = malloc(DL_SIZE);
    if (!diclist) {
      return -1;
    }
#endif
    
    if (!(i = RkwGetDicList(cx_num, diclist, DL_SIZE))) {
      ret = 0;
    }
    else {
      if (i > 0) {
	for (p = diclist, rv = 0; *p;) {
	  rv += _RkwSync(cx, p);
	  p += strlen(p) + 1;
	}
	if (!rv) {
	  ret = 0;
	  goto return_ret;
	}
      }
      ret = -1;
    }
  return_ret:;
#ifdef USE_MALLOC_FOR_BIG_ARRAY
    (void)free((char *)diclist);
#endif
  } else {
    ret = _RkwSync(cx, dicname);
  }
  return ret;
}

/*ARGSUSED*/
RkwGetSimpleKanji(cxnum, dicname, yomi, maxyomi,
		  kanjis, maxkanjis, hinshis, maxhinshis)
int cxnum, maxyomi, maxkanjis, maxhinshis;
char *dicname;
Wchar *yomi, *kanjis, *hinshis;
{
  return -1;
}

/*ARGSUSED*/
int
RkwStoreRange(cx_num, yomi, maxyomi)
     int		cx_num;
     Wchar		*yomi;
     int		maxyomi;
{
  return(0);
}

/*ARGSUSED*/
int
RkwSetLocale(cx_num, locale)
     int		cx_num;
     unsigned char	*locale;
{
  return(0);
}
