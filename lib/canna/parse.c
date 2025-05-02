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
static char rcs_id[] = "@(#) 102.1 $Id: parse.c,v 1.1.1.1 2002/10/19 08:27:50 aida_s Exp $";
#endif /* lint */

#include "canna.h"

#ifndef WIN
#define CANNA_USE_LISP_CUSTOMIZE_FILE
#endif

#ifdef CANNA_USE_LISP_CUSTOMIZE_FILE /* Continues far away to almost bottom */

#include <stdio.h>
#include <fcntl.h>

extern char *CANNA_initfilename;

#define BUF_LEN 1024

static char CANNA_rcfilename[BUF_LEN] = "";

static int DISPLAY_to_hostname();

/* cfuncdef

   YYparse -- カスタマイズファイルを読む。

   ファイルディスクリプタで指定されたファイルを読み込む。

*/

extern ckverbose;

extern YYparse_by_rcfilename();

/* cfuncdef

  parse -- .canna ファイルを探してきて読み込む。

  parse はカスタマイズファイルを探し、そのファイルをオープンしパースす
  る。

  パース中のファイルの名前を CANNA_rcfilename に入れておく。

  */

#define NAMEBUFSIZE 1024
#ifdef WIN
#define RCFILENAME  ".can"
#else
#define RCFILENAME  ".canna"
#endif
#define FILEENVNAME "CANNAFILE"
#define HOSTENVNAME "CANNAHOST"

#ifdef WIN
#define OBSOLETE_RCFILENAME  ".irh"
#else
#define OBSOLETE_RCFILENAME  ".iroha"
#endif
#define OBSOLETE_FILEENVNAME "IROHAFILE"
#define OBSOLETE_HOSTENVNAME "IROHAHOST"

static
make_initfilename()
{
  if(!CANNA_initfilename) {
    CANNA_initfilename = malloc(1024);
    if (!CANNA_initfilename) {
      return -1;
    }
    strcpy(CANNA_initfilename, CANNA_rcfilename);
  }
  else {
    strcat(CANNA_initfilename, ",");
    strcat(CANNA_initfilename, CANNA_rcfilename);
  }
  return 0;
}

static void
fit_initfilename()
{
  char *tmpstr;

  if (CANNA_initfilename) {
    tmpstr = malloc(strlen(CANNA_initfilename) + 1);
    if (!tmpstr) return;
    strcpy(tmpstr, CANNA_initfilename);
    free(CANNA_initfilename);
    CANNA_initfilename = tmpstr;
  }
}

void
parse()
{
  char *p, *getenv();
  int n;
  extern iroha_debug;
  int home_canna_exist = 0;
  extern char *initFileSpecified;
  extern int auto_define;
#ifndef WIN
  char buf[256];
#else
  char *buf = malloc(256);
  if (!buf) {
    return;
  }
#endif

  if (clisp_init() == 0) {

#ifndef WIN
    if (ckverbose) {
      printf("カスタマイズファイルは読み込みません。\n");
    }
#endif

    addWarningMesg("\245\341\245\342\245\352\244\254\302\255\244\352\244\336"
	"\244\273\244\363\241\243\245\253\245\271\245\277\245\336\245\244"
	"\245\272\245\325\245\241\245\244\245\353\244\362\306\311\244\337"
	"\271\376\244\341\244\336\244\273\244\363\241\243\\n");
          /* メモリが足りません。カスタマイズファイルを読み込めません。 */
    goto quitparse;
  }

  if (initFileSpecified) {
    strcpy(CANNA_rcfilename, initFileSpecified);
    if (YYparse_by_rcfilename(CANNA_rcfilename)) {
      make_initfilename();
      goto quitparse;
    }
    else {
#ifndef WIN
      if (ckverbose) {
	printf("カスタマイズファイルは読み込みません。\n");
      }
#endif

      sprintf(buf, "\273\330\304\352\244\265\244\354\244\277\245\253\245\271"
	"\245\277\245\336\245\244\245\272\245\325\245\241\245\244\245\353"
	"\40\45\163\40\244\254\302\270\272\337\244\267\244\336\244\273"
	"\244\363\241\243",
	      CANNA_rcfilename);
              /* 指定されたカスタマイズファイル %s が存在しません。 */
      addWarningMesg(buf);
      goto quitparse;
    }
  }
  p = getenv(FILEENVNAME);
  if (p) {
    strcpy(CANNA_rcfilename, p);
    if (YYparse_by_rcfilename(CANNA_rcfilename)) {
      make_initfilename();
      goto quitparse;
    }
  }
#ifdef OBSOLETE_FILEENVNAME
  else if ((p = getenv(OBSOLETE_FILEENVNAME)) != (char *)0) {
    sprintf(buf, "\303\355\260\325\72\40\245\253\245\271\245\277\245\336"
	"\245\244\245\272\245\325\245\241\245\244\245\353\244\362\273\330"
	"\304\352\244\271\244\353\244\277\244\341\244\316\264\304\266\255"
	"\312\321\277\364\40\45\163\40\244\254\273\330\304\352\244\265"
	"\244\354\244\306\244\244"
	    , OBSOLETE_FILEENVNAME);
    /* 注意: カスタマイズファイルを指定するための環境変数 %s が指定されてい */
    addWarningMesg(buf);
    sprintf(buf, "\40\40\40\40\40\40\244\336\244\271\244\254\241\242\277\267"
	"\267\301\274\260\244\316\245\253\245\271\245\277\245\336\245\244"
	"\245\272\245\325\245\241\245\244\245\353\244\362\273\330\304\352"
	"\244\271\244\353\40\45\163\40\244\254\273\330\304\352"
	    , FILEENVNAME);
    /*       ますが、新形式のカスタマイズファイルを指定する %s が指定 */
    addWarningMesg(buf);
    addWarningMesg("\40\40\40\40\40\40\244\265\244\354\244\306\244\244"
	"\244\336\244\273\244\363\241\243\277\267\267\301\274\260\244\316"
	"\245\253\245\271\245\277\245\336\245\244\245\272\245\325\245\241"
	"\245\244\245\353\244\362\272\356\300\256\244\267\241\242\264\304"
	"\266\255\312\321\277\364"
		   );
    /*       されていません。新形式のカスタマイズファイルを作成し、環境変数 */
    sprintf(buf, "\40\40\40\40\40\40\45\163\40\244\362\300\337\304\352"
	"\244\267\244\306\262\274\244\265\244\244\241\243"
	    , FILEENVNAME);
    /*      %s を設定して下さい。 */
    addWarningMesg(buf);
  }
#endif
  p = getenv("HOME");
  if (p) {
    strcpy(CANNA_rcfilename, p);
    strcat(CANNA_rcfilename, "/");
    strcat(CANNA_rcfilename, RCFILENAME);
    n = strlen(CANNA_rcfilename);

    /* $HOME/.canna */

    home_canna_exist = YYparse_by_rcfilename(CANNA_rcfilename);
    if (home_canna_exist) {
      make_initfilename();

      /* $HOME/.canna-DISPLAY */

#ifndef WIN
      p = getenv("DISPLAY");
      if (p) {
	char display[NAMEBUFSIZE];
	
	DISPLAY_to_hostname(p, display, NAMEBUFSIZE);
	
	CANNA_rcfilename[n] = '-';
	strcpy(CANNA_rcfilename + n + 1, display);
	
	if(YYparse_by_rcfilename(CANNA_rcfilename)) {
	  make_initfilename();
	}
      }
      
      /* $HOME/.canna-TERM */
      
      p = getenv("TERM");
      if (p) {
	CANNA_rcfilename[n] = '-';
	strcpy(CANNA_rcfilename + n + 1, p);
	if(YYparse_by_rcfilename(CANNA_rcfilename)) {
	  make_initfilename();
	}	  
      }
#endif
    }
#ifdef OBSOLETE_RCFILENAME
    else { /* .canna が存在していない */
      strcpy(CANNA_rcfilename, p);
      strcat(CANNA_rcfilename, "/");
      strcat(CANNA_rcfilename, OBSOLETE_RCFILENAME);
      n = strlen(CANNA_rcfilename);
      if (close(open(CANNA_rcfilename, O_RDONLY)) == 0) { /* ある */
	sprintf(buf, "\303\355\260\325\72\40\265\354\267\301\274\260\244\316"
	"\245\253\245\271\245\277\245\336\245\244\245\272\245\325\245\241"
	"\245\244\245\353\40\45\163\40\244\254\302\270\272\337\244\267"
	"\244\306\244\244\244\336\244\271\244\254\277\267\267\301\274\260"
	"\244\316"
		, OBSOLETE_RCFILENAME);
        /* 注意: 旧形式のカスタマイズファイル %s が存在していますが新形式の */
	addWarningMesg(buf);
	sprintf(buf, "\40\40\40\40\40\40\245\253\245\271\245\277\245\336"
	"\245\244\245\272\245\325\245\241\245\244\245\353\40\45\163\40"
	"\244\254\302\270\272\337\244\267\244\306\244\244\244\336\244\273"
	"\244\363\241\243\143\141\156\166\145\162\164\40\245\263\245\336"
	"\245\363\245\311\244\362"
		, RCFILENAME);
        /*    カスタマイズファイル %s が存在していません。canvert コマンドを */
	addWarningMesg(buf);
	sprintf(buf, "\40\40\40\40\40\40\315\370\315\321\244\267\244\306"
	"\277\267\267\301\274\260\244\316\245\253\245\271\245\277\245\336"
	"\245\244\245\272\245\325\245\241\245\244\245\353\40\45\163\40\244\362"
	"\272\356\300\256\244\267\244\306\262\274\244\265\244\244\241\243"
		, RCFILENAME);
        /*     利用して新形式のカスタマイズファイル %s を作成して下さい。 */
	addWarningMesg(buf);
	sprintf(buf, "\40\40\40\40\40\40\50\316\343\51\40\143\141\156\166"
	"\145\162\164\40\55\143\40\55\157\40\176\57\45\163\40\55\156\40"
	"\176\57\45\163"
		, OBSOLETE_RCFILENAME, RCFILENAME);
        /*       (例) canvert -c -o ~/%s -n ~/%s" */
	addWarningMesg(buf);
      }
    }
#endif
  }

  if ( !home_canna_exist ) {
    /* 最後はシステムデフォルトのファイルを読む */
    strcpy(CANNA_rcfilename, CANNALIBDIR);
    n = strlen(CANNA_rcfilename);
 
    strcpy(CANNA_rcfilename + n, "/default");
    strcat(CANNA_rcfilename + n, RCFILENAME);
    if (YYparse_by_rcfilename(CANNA_rcfilename)) {
      make_initfilename();
#ifndef WIN
      p = getenv("DISPLAY");
      if (p) {
	char display[NAMEBUFSIZE];
	
	DISPLAY_to_hostname(p, display, NAMEBUFSIZE);

	CANNA_rcfilename[n] = '/';
	strcpy(CANNA_rcfilename + n + 1, display);
	strcat(CANNA_rcfilename, RCFILENAME);
	if(YYparse_by_rcfilename(CANNA_rcfilename)) {
	  make_initfilename();
	}
      }

      p = getenv("TERM");
      if (p) {
	CANNA_rcfilename[n] = '/';
	strcpy(CANNA_rcfilename + n + 1, p);
	strcat(CANNA_rcfilename, RCFILENAME);
	if(YYparse_by_rcfilename(CANNA_rcfilename)) {
	  make_initfilename();
	}
      }
#endif
    }
    else {
#ifndef WIN
      if (ckverbose) {
	printf("カスタマイズファイルは読み込みません。\n");
      }
#endif
      sprintf(buf, 
#ifndef WIN
      "システムのカスタマイズファイル %s が存在しません。",
#else
      "\245\267\245\271\245\306\245\340\244\316\245\253\245\271"
      "\245\277\245\336\245\244\245\272\245\325\245\241\245\244\245\353"
      "\40\45\163\40\244\254\302\270\272\337\244\267\244\336\244\273"
      "\244\363\241\243",
#endif
	      CANNA_rcfilename);
      /* システムのカスタマイズファイル %s が存在しません。 */
      addWarningMesg(buf);
    }
  }

 quitparse:
  /* CANNA_initfilename をジャストサイズに刈り込む */
  fit_initfilename();
  clisp_fin();

#ifdef WIN
  (void)free(buf);
#endif
}


#ifndef WIN
static
DISPLAY_to_hostname(name, buf, bufsize)
char *name, *buf;
int bufsize;
{
  if (name[0] == ':' || !strncmp(name, "unix", 4)) {
    gethostname(buf, bufsize);
  }
  else {
    int i, len = strlen(name);
    for (i = 0 ; i < len && i < bufsize ; i++) {
      if (name[i] == ':') {
	break;
      }
      else {
	buf[i] = name[i];
      }
    }
    if (i < bufsize) {
      buf[i] = '\0';
    }
  }
}
#endif

#else /* !CANNA_USE_LISP_CUSTOMIZE_FILE */

/*

   Providing three functions to parse user configurations from the
   registry of Windows 95

  parse:
    Parse the registration to get the configuration.

  parse_string:
    This should be a void function in Windows 95.  This function originally
    reads a string to configure some kind of customization even after
    initialization process has finished.

  clisp_main:
    This should be a void function in Windows 95.  This function originally
    provides an configuration interpreter to user.

 */

#include "cannacnf.h"

static void adddic pro((char *, int, char *));

static void
adddic(char *str, int dictype, char *con)
{
  extern struct dicname *kanjidicnames;
  struct dicname *kanjidicname;
  extern char *kataautodic;
  extern auto_define;
#ifdef HIRAGANAAUTO
  extern char *hiraautodic;
#endif

  kanjidicname  = (struct dicname *)malloc(sizeof(struct dicname));
  if (kanjidicname) {
    kanjidicname->name = malloc(strlen(str) + 1);
    if (kanjidicname->name) {
      strcpy(kanjidicname->name , str);
      kanjidicname->dictype = dictype;
      kanjidicname->dicflag = DIC_NOT_MOUNTED;
      kanjidicname->next = kanjidicnames;
      kanjidicnames = kanjidicname;

      if (kanjidicname->dictype == DIC_KATAKANA) {
	auto_define = 1;
	if (!kataautodic) { /* only the first one is valid */
	  kataautodic = kanjidicname->name;
	}
      }
#ifdef HIRAGANAAUTO
      else if (kanjidicname->dictype == DIC_HIRAGANA) {
	auto_define = 1;
	if (!hiraautodic) { /* only the first one is valid */
	  hiraautodic = kanjidicname->name;
	}
      }
#endif
      return;
    }
    free((char *)kanjidicname);
  }
}

static void
addphonodic(char *dic, char *con)
{
  extern char *RomkanaTable;

  RomkanaTable = strdup(dic);
}

/* strncpy which does not terminate at NULL */

static char *
Strncpy(x, y, len)
char *x, *y;
int len;
{
  int i;

  for (i = 0 ; i < len ; i++) {
    x[i] = y[i];
  }
  return x;
}

extern changeKeyfunc(int, int, int, unsigned char *, unsigned char *);
extern changeKeyfuncOfAll(int, int, unsigned char *, unsigned char *);

static int
  setkey pro((unsigned mode,
	      unsigned char *keys, int keylen,
	      unsigned char *funcs, int funclen,
	      char *con));

static int
setkey(mode, keys, keylen, funcs, funclen, con)
unsigned mode;
unsigned char *keys;
int keylen;
unsigned char *funcs;
int funclen;
char *con;
{
  int func;
  unsigned char *buf;
  int retval = -1;

  buf = (unsigned char *)malloc(256);
  if (buf) {
    Strncpy((char *)buf, funcs, funclen);
    funcs[funclen] = 255;
    if (keylen > 1) {
      func = CANNA_FN_UseOtherKeymap;
    }
    else if (funclen > 1) {
      func = CANNA_FN_FuncSequence;
    }
    else {
      func = funcs[0];
    }
    if (mode == 255) { /* global */
      retval = changeKeyfuncOfAll((int)keys[0], func, funcs, keys);
    }
    else {
      retval = changeKeyfunc(mode, (int)keys[0], func, funcs, keys);
    }
    free((char *)buf);
  }
  return retval;
}

static void
defsymbol(int ncols, int nrows, int total,
	  unsigned char *keys, unsigned char *xkeys,
	  wchar_t *cands, char *con)
{
  wchar_t *pc = cands, *ps, *mcand, **acand, *pt;
  unsigned char *pk = keys, *px = xkeys;
  int i, j, k, group;
  extern int nkeysup;
  extern keySupplement keysup[];

  if (nkeysup < MAX_KEY_SUP) {
    group = nkeysup;
    for (i = 0 ; i < nrows ; i++, pk++) {
      for (ps = pc, j = 0 ; j < ncols ; j++) {
	while (*pc++);
      }
      mcand = (wchar_t *)malloc(sizeof(wchar_t) * ((pc - ps) + 1));
      if (mcand) {
	acand = (wchar_t **)malloc(sizeof(wchar_t *) * (ncols + 1));
	if (acand) {
	  wchar_t *psrc, *epp, *pdst;
	  for (psrc = ps, pdst = mcand, epp = pdst + (pc - ps); pdst < epp ;) {
	    *pdst++ = *psrc++;
	  }
	  *pdst = (wchar_t)0;
	  for (k = 0, pt = mcand ; k < ncols ; k++) {
	    acand[k] = pt;
	    while (*pt++);
	  }
	  acand[k] = 0;

	  keysup[nkeysup].key = *pk;
	  keysup[nkeysup].xkey = xkeys ? *px++ : *pk;
	  keysup[nkeysup].groupid = group;
	  keysup[nkeysup].ncand = ncols;
	  keysup[nkeysup].cand = acand;
	  keysup[nkeysup].fullword = mcand;
	  nkeysup++;
	  continue;
	}
	free((char *)mcand);
      }
    }
  }
}  

static void
defmode(int mode, char *display, char *romdic,
	unsigned char *funcs, int nfuncs, int sym)
{
  extraFunc *extrafunc = (extraFunc *)0;
  unsigned char *p, *ep;

  extrafunc = (extraFunc *)malloc(sizeof(extraFunc));
  if (extrafunc) {
    extrafunc->fnum = mode;
    extrafunc->display_name = (wchar_t *)NULL;
    extrafunc->u.modeptr = (newmode *)malloc(sizeof(newmode));
    if (extrafunc->u.modeptr) {
      KanjiMode kanjimode;

      extrafunc->u.modeptr->romaji_table = (char *)0;
      extrafunc->u.modeptr->romdic = (struct RkRxDic *)0;
      extrafunc->u.modeptr->romdic_owner = 0;
      extrafunc->u.modeptr->flags = CANNA_YOMI_IGNORE_USERSYMBOLS;
      extrafunc->u.modeptr->emode = (KanjiMode)0;
      kanjimode = (KanjiMode)malloc(sizeof(KanjiModeRec));
      if (kanjimode) {
	int searchfunc();
	extern KanjiModeRec empty_mode;
	extern BYTE *emptymap;
	long f;

	kanjimode->func = searchfunc;
	kanjimode->keytbl = emptymap;
	kanjimode->flags = 
	  CANNA_KANJIMODE_TABLE_SHARED | CANNA_KANJIMODE_EMPTY_MODE;
	kanjimode->ftbl = empty_mode.ftbl;
	extrafunc->u.modeptr->emode = kanjimode;

	if (sym) {
	  extrafunc->u.modeptr->flags &= ~CANNA_YOMI_IGNORE_USERSYMBOLS;
	}

	if (display) {
	  extrafunc->display_name = WString(display);
	  if (!extrafunc->display_name) {
	    goto defmode_error;
	  }
	}

	f = extrafunc->u.modeptr->flags;

	for (p = funcs, ep = funcs + nfuncs ; p < ep ; p++) {
	  switch (*p) {
	  case CANNA_FN_Kakutei:
	    f |= CANNA_YOMI_KAKUTEI;
	    break;
	  case CANNA_FN_Henkan:
	    f |= CANNA_YOMI_HENKAN;
	    break;
	  case CANNA_FN_Zenkaku:
	    f |= CANNA_YOMI_ZENKAKU;
	    break;
	  case CANNA_FN_Hankaku:
	    f |= CANNA_YOMI_HANKAKU;
	    break;
	  case CANNA_FN_Hiragana:
	    f |= CANNA_YOMI_HIRAGANA;
	    break;
	  case CANNA_FN_Katakana:
	    f |= CANNA_YOMI_KATAKANA;
	    break;
	  case CANNA_FN_Romaji:
	    f |= CANNA_YOMI_ROMAJI;
	    break;
	    /* 以下はそのうちやろう */
	  case CANNA_FN_ToUpper:
	    break;
	  case CANNA_FN_Capitalize:
	    break;
	  case CANNA_FN_ToLower:
	    break;
	  default:
	    goto defmode_error;
	  }
	}
	return;

      defmode_error:
	(void)free((char *)kanjimode);
      }
      (void)free((char *)extrafunc->u.modeptr);
    }
    (void)free((char *)extrafunc);
  }
}

#define CONFIGLIB "cannacnf.dll"
#define DLLDIR "/bin/"

void parse(void)
{
  extern jrUserInfoStruct *uinfo;
  struct libconf conf;
  extern struct CannaConfig cannaconf;
  char *buf;

  buf = malloc(256); /* 256は汚いなあ */
  if (buf) {
    if (uinfo) {
      strcpy(buf, uinfo->topdir);
      strcat(buf, DLLDIR);
    }
    else {
      buf[0] = '\0';
    }
    strcat(buf, CONFIGLIB);

    conf.cf = &cannaconf;
    conf.dicfn = adddic;
    conf.romfn = addphonodic;
    conf.keyfn = setkey;
    conf.symfn = defsymbol;

    CannaGetConfigure(buf, (char *)0, &conf, NULL, NULL, NULL);
    free(buf);
  }
}

int parse_string(char *str)
/* ARGSUSED */
{
  return 0;
}

void clisp_main(void)
{
  /* Nothing to do for Microsoft Windows */
}

#endif /* !CANNA_USE_LISP_CUSTOMIZE_FILE */
