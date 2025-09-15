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
static char rcs_id[] = "@(#) 102.1 $Id: parse.c,v 1.4 2003/09/17 08:50:53 aida_s Exp $";
#endif /* lint */

#include "canna.h"

#include <stdio.h>
#include <fcntl.h>

/*********************************************************************
 *                      wchar_t replace begin                        *
 *********************************************************************/
#ifdef wchar_t
# error "wchar_t is already defined"
#endif
#define wchar_t cannawc

extern char *CANNA_initfilename;

#define BUF_LEN 1024

static char CANNA_rcfilename[BUF_LEN] = "";

static int DISPLAY_to_hostname();

/* cfuncdef

   YYparse -- カスタマイズファイルを読む。

   ファイルディスクリプタで指定されたファイルを読み込む。

*/

extern int ckverbose;

extern int YYparse_by_rcfilename();

/* cfuncdef

  parse -- .canna ファイルを探してきて読み込む。

  parse はカスタマイズファイルを探し、そのファイルをオープンしパースす
  る。

  パース中のファイルの名前を CANNA_rcfilename に入れておく。

  */

#define NAMEBUFSIZE 1024
#define RCFILENAME  ".canna"
#define FILEENVNAME "CANNAFILE"
#define HOSTENVNAME "CANNAHOST"

#define OBSOLETE_FILEENVNAME "IROHAFILE"
#define OBSOLETE_HOSTENVNAME "IROHAHOST"

static int
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

extern int clisp_init pro((void)); /* lisp.c */

void
parse()
{
  char *p, *getenv();
  int n;
  extern int iroha_debug;
  int home_canna_exist = 0;
  extern char *initFileSpecified;
  extern int auto_define;
#ifndef USE_MALLOC_FOR_BIG_ARRAY
  char buf[256];
#else
  char *buf = malloc(256);
  if (!buf) {
    return;
  }
#endif

  if (clisp_init() == 0) {

    if (ckverbose) {
      printf("カスタマイズファイルは読み込みません。\n");
    }

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
      if (ckverbose) {
	printf("カスタマイズファイルは読み込みません。\n");
      }

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
    }
    else {
      if (ckverbose) {
	printf("カスタマイズファイルは読み込みません。\n");
      }
      sprintf(buf, 
#ifndef CODED_MESSAGE
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

#ifdef USE_MALLOC_FOR_BIG_ARRAY
  (void)free(buf);
#endif
}

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static int
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

#ifndef wchar_t
# error "wchar_t is already undefined"
#endif
#undef wchar_t
/*********************************************************************
 *                       wchar_t replace end                         *
 *********************************************************************/
