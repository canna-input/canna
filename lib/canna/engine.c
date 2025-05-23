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
 * suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
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
static char rcs_id[] = "$Id: engine.c,v 1.6 2003/09/21 10:16:49 aida_s Exp $";
#endif

#include "canna.h"

#ifdef ENGINE_SWITCH

#include "canna/RK.h"

#ifdef DL
#include <dlfcn.h>
#endif

/*********************************************************************
 *                      wchar_t replace begin                        *
 *********************************************************************/
#ifdef wchar_t
# error "wchar_t is already defined"
#endif
#define wchar_t cannawc

static struct rkfuncs *Rk;

#ifdef DL

#ifdef CANNA_WCHAR16
# define ENGINE_CONFIG_FILE "engine16.cnf"
#else /* !defined(CANNA_WCHAR16) */
# define ENGINE_CONFIG_FILE "engine.cnf"
#endif

#define LINEBUFSIZE 256
#define EBUFSIZE     64

typedef struct engines{
  char *name;
  char *libname;
};

static struct engines *enginetable = (struct engines *)0;
static int NENGINES = 0;

DSOHANDLE dlh = (DSOHANDLE)0;

#else /* !DL */

extern struct rkfuncs cannaRkFuncs, wnnRkFuncs;

typedef struct engines{
  char *name;
  struct rkfuncs *libname;
};

struct engines enginetable[] = {
  {"cannaserver",	&cannaRkFuncs},
  {"irohaserver",	&cannaRkFuncs},
  {"jserver",		&wnnRkFuncs},
};

#define NENGINES (sizeof(enginetable) / sizeof(struct engines))

#endif /* !DL */

static int current_engine = -1;

#ifdef DL

static char *
extoken(s, next_return)
char *s, **next_return;
{
  register char *p = s, ch;
  char *res;

  while ((ch = *p) && (ch == ' ' || ch == '\t')) p++;
  if (ch == '#') {
    *next_return = p;
    return (char *)0;
  }
  res = p;
  while ((ch = *p) && ch != ' ' && ch != '\t' && ch != '\n' && ch != '#') p++;
  if (p == res) { /* case EOS or EOL */
    *next_return = p;
    return (char *)0;
  }
  else {
    if (ch) *p = '\0';
    if (ch != '#') p++;
    *next_return = p;
    return res;
  }
}

struct engines *
getengines(nengines)
int *nengines;
{
  FILE *f;
  char *ename, *lib, *p;
  struct engines *res = (struct engines *)0;
  int n = 0;
#ifdef USE_MALLOC_FOR_BIG_ARRAY
  extern jrUserInfoStruct *uinfo;
  char *buf, *winbuf;
  struct engines *ebuf;

  buf = malloc(LINEBUFSIZE);
  ebuf = (struct engines *)malloc(sizeof(struct engines) * EBUFSIZE);
  winbuf = malloc(LINEBUFSIZE);
  if (!buf || !ebuf || !winbuf) {
    if (buf) {
      (void)free(buf);
    }
    if (ebuf) {
      (void)free((char *)ebuf);
    }
    if (winbuf) {
      (void)free(winbuf);
    }
    return res;
  }
#else
  char buf[LINEBUFSIZE];
  struct engines ebuf[EBUFSIZE];
#endif

  *nengines = 0;
  strcpy(buf, CANNALIBDIR);
  strcat(buf, "/");
  strcat(buf, ENGINE_CONFIG_FILE);
  if ((f = fopen(buf, "r")) != NULL) {
    while (n < EBUFSIZE && fgets(buf, LINEBUFSIZE, f)) {
      ename = extoken(buf, &p);
      lib = extoken(p, &p);
      if (ename && lib) {
#ifdef CANNA_WCHAR16
	strcat(lib, "16");
#endif
	strcat(lib, ".so.");
	strcat(lib, CANNA_DSOREV);
	if (ebuf[n].name = malloc(strlen(ename) + 1)) {
	  if (ebuf[n].libname = malloc(strlen(lib) + 1)) {
	    strcpy(ebuf[n].name, ename);
	    strcpy(ebuf[n].libname, lib);
	    n++;
	  }
	  else {
	    free(ebuf[n].name);
	  }
	}
      }
    }
    if (n > 0 &&
	(res = (struct engines *)malloc(n * sizeof(struct engines)))) {
      bcopy(ebuf, res, n * sizeof(struct engines));
      *nengines = n;
    }
    fclose(f);
  }
#ifdef USE_MALLOC_FOR_BIG_ARRAY
  (void)free(winbuf);
  (void)free((char *)ebuf);
  (void)free(buf);
#endif
  return res;
}

static
useEngine(libname)
char *libname;
{
  if (dlh) {
    dlclose(dlh);
    dlh = (DSOHANDLE)0;
  }
#ifdef DEBUG
  dlh = dlopen(libname, RTLD_NOW);
#else /* !DEBUG */
  dlh = dlopen(libname, RTLD_LAZY);
#endif /* !DEBUG */
  if (dlh) {
    Rk = (struct rkfuncs *)dlsym(dlh, "RkFuncs");
    if (Rk) {
      return 0;
    }
    dlclose(dlh);
    dlh = (DSOHANDLE)0;
  }
  if (libname != enginetable[0].libname) {
    useEngine(enginetable[0].libname);
  }
  return -1; /* ���顼 */
}

#else /* !DL */

static
useEngine(libname)
struct rkfuncs *libname;
{
  Rk = libname;
  return 0;
}

#endif /* !DL */

static
switch_engine(engine)
char *engine;
{
  int i;

#ifdef DL
  if (!enginetable) {
    enginetable = getengines(&NENGINES);
    if (!enginetable) {
      return -1;
    }
  }
#endif

  if (engine) {
    for (i = 0 ; i < NENGINES ; i++) {
      if (!strcmp(enginetable[i].name, engine)) {
	if (current_engine != i) {
	  current_engine = i;
	  return useEngine(enginetable[i].libname);
	}
	return 0;
      }
    }
    if (!(i < NENGINES) && current_engine != 0) {
      current_engine = 0;
      useEngine(enginetable[0].libname);
      return -1;
    }
  }
  else if (current_engine != 0) {
    current_engine = 0;
    return useEngine(enginetable[0].libname);
  }
  return 0;
}

static char *server_host = (char *)0;
static char *server_engine = (char *)0;

RkSetServerName(s)
char *s;
{
  if (server_host) {
    free(server_host);
    server_host = (char *)0;
  }
  if (server_engine) {
    free(server_engine);
    server_engine = (char *)0;
  }

  if (s) {
    char *at, *index();

    at = index(s, ',');
    if (at) {
      return switch_engine((char *)0);
    }

    at = index(s, '@');
    if (at) {
      server_host = malloc(strlen(at)); /* strlen(at) == strlen(at + 1) + 1 */
      if (server_host) {
	strcpy(server_host, at + 1);
      }
      server_engine = malloc(at - s + 1);
      if (server_engine) {
	strncpy(server_engine, s, at - s);
	server_engine[at - s] = '\0';
      }
    }
    else {
      server_host = malloc(strlen(s) + 1);
      if (server_host) {
	strcpy(server_host, s);
      }
      server_engine = (char *)0;
    }
  }
  else {
    server_host = (char *)0;
    server_engine = (char *)0;
  }
  return switch_engine(server_engine);
}

/* RK functions */

char *
RkGetServerHost()
{
  return server_host;
}

char *
RkGetServerEngine()
{
  return server_engine;
}

int
RkwGetProtocolVersion(map, mip)
int *map, *mip;
{
  return Rk ? (*Rk->GetProtocolVersion)(map, mip) : -1;
}

char *
RkwGetServerName()
{
  return Rk ? (*Rk->GetServerName)() : (char *)0;
}

int
RkwGetServerVersion(map, mip)
int *map, *mip;
{
  return Rk ? (*Rk->GetServerVersion)(map, mip) : -1;
}

int
RkwInitialize(host)
char *host;
{
  return Rk ? (*Rk->Initialize)(host) : -1;
}

void
RkwFinalize()
{
  if (Rk) (*Rk->Finalize)();
}

int
RkwCreateContext()
{
  return Rk ? (*Rk->CreateContext)() : -1;
}

int
RkwDuplicateContext(cn)
int cn;
{
  return Rk ? (*Rk->DuplicateContext)(cn) : -1;
}

int
RkwCloseContext(cn)
int cn;
{
  return Rk ? (*Rk->CloseContext)(cn) : -1;
}

int
RkwSetDicPath(cn, path)
int cn;
char *path;
{
  return Rk ? (*Rk->SetDicPath)(cn, path) : -1;
}

int
RkwCreateDic(cn, dic, mode)
int cn, mode;
char *dic;
{
  return Rk ? (*Rk->CreateDic)(cn, dic, mode) : -1;
}

int
RkwGetDicList(cn, buf, maxbuf)
int cn, maxbuf;
char *buf;
{
  return Rk ? (*Rk->GetDicList)(cn, buf, maxbuf) : -1;
}

int
RkwGetMountList(cn, buf, maxbuf)
int cn, maxbuf;
char *buf;
{
  return Rk ? (*Rk->GetMountList)(cn, buf, maxbuf) : -1;
}

int
RkwMountDic(cn, dic, f)
int cn, f;
char *dic;
{
  return Rk ? (*Rk->MountDic)(cn, dic, f) : -1;
}

int
RkwRemountDic(cn, dic, where)
int cn, where;
char *dic;
{
  return Rk ? (*Rk->RemountDic)(cn, dic, where) : -1;
}

int
RkwUnmountDic(cn, dic)
int cn;
char *dic;
{
  return Rk ? (*Rk->UnmountDic)(cn, dic) : -1;
}

int
RkwDefineDic(cn, dic, word)
int cn;
char *dic;
wchar_t *word;
{
  return Rk ? (*Rk->DefineDic)(cn, dic, word) : -1;
}

int
RkwDeleteDic(cn, dic, word)
int cn;
char *dic;
wchar_t *word;
{
  return Rk ? (*Rk->DeleteDic)(cn, dic, word) : -1;
}

int
RkwGetHinshi(cn, buf, maxbuf)
int cn, maxbuf;
wchar_t *buf;
{
  return Rk ? (*Rk->GetHinshi)(cn, buf, maxbuf) : -1;
}

int
RkwGetKanji(cn, buf, maxbuf)
int cn, maxbuf;
wchar_t *buf;
{
  return Rk ? (*Rk->GetKanji)(cn, buf, maxbuf) : -1;
}

int
RkwGetYomi(cn, buf, maxbuf)
int cn, maxbuf;
wchar_t *buf;
{
  return Rk ? (*Rk->GetYomi)(cn, buf, maxbuf) : -1;
}

int
RkwGetLex(cn, buf, maxbuf)
int cn, maxbuf;
RkLex *buf;
{
  return Rk ? (*Rk->GetLex)(cn, buf, maxbuf) : -1;
}

int
RkwGetStat(cn, buf)
int cn;
RkStat *buf;
{
  return Rk ? (*Rk->GetStat)(cn, buf) : -1;
}

int
RkwGetKanjiList(cn, buf, maxbuf)
int cn, maxbuf;
wchar_t *buf;
{
  return Rk ? (*Rk->GetKanjiList)(cn, buf, maxbuf) : -1;
}

int
RkwFlushYomi(cn)
int cn;
{
  return Rk ? (*Rk->FlushYomi)(cn) : -1;
}

int
RkwGetLastYomi(cn, buf, maxbuf)
int cn, maxbuf;
wchar_t *buf;
{
  return Rk ? (*Rk->GetLastYomi)(cn, buf, maxbuf) : -1;
}

int
RkwRemoveBun(cn, mode)
int cn, mode;
{
  return Rk ? (*Rk->RemoveBun)(cn, mode) : -1;
}

int
RkwSubstYomi(cn, s, e, yomi, len)
int cn, s, e, len;
wchar_t *yomi;
{
  return Rk ? (*Rk->SubstYomi)(cn, s, e, yomi, len) : -1;
}

int
RkwBgnBun(cn, yomi, len, f)
int cn, len, f;
wchar_t *yomi;
{
  return Rk ? (*Rk->BgnBun)(cn, yomi, len, f) : -1;
}

int
RkwEndBun(cn, mode)
int cn, mode;
{
  return Rk ? (*Rk->EndBun)(cn, mode) : -1;
}

int
RkwGoTo(cn, where)
int cn, where;
{
  return Rk ? (*Rk->GoTo)(cn, where) : -1;
}

int
RkwLeft(cn)
int cn;
{
  return Rk ? (*Rk->Left)(cn) : -1;
}

int
RkwRight(cn)
int cn;
{
  return Rk ? (*Rk->Right)(cn) : -1;
}

int
RkwNext(cn)
int cn;
{
  return Rk ? (*Rk->Next)(cn) : -1;
}

int
RkwPrev(cn)
int cn;
{
  return Rk ? (*Rk->Prev)(cn) : -1;
}

int
RkwNfer(cn)
int cn;
{
  return Rk ? (*Rk->Nfer)(cn) : -1;
}

int
RkwXfer(cn, knum)
int cn, knum;
{
  return Rk ? (*Rk->Xfer)(cn, knum) : -1;
}

int
RkwResize(cn, len)
int cn, len;
{
  return Rk ? (*Rk->Resize)(cn, len) : -1;
}

int
RkwEnlarge(cn)
int cn;
{
  return Rk ? (*Rk->Enlarge)(cn) : -1;
}

int
RkwShorten(cn)
int cn;
{
  return Rk ? (*Rk->Shorten)(cn) : -1;
}

int
RkwStoreYomi(cn, yomi, len)
int cn, len;
wchar_t *yomi;
{
  return Rk ? (*Rk->StoreYomi)(cn, yomi, len) : -1;
}

int
RkwSetAppName(cn, name)
int cn;
char *name;
{
  return Rk ? (*Rk->SetAppName)(cn, name) : -1;
}

int
RkwSync(cn, name)
int cn;
char *name;
{
  return Rk ? (*Rk->SyncDic)(cn, name) : -1;
}

int
RkwSetUserInfo(user, group, topdir)
char *user, *group, *topdir;
{
  return Rk ? (*Rk->SetUserInfo)(user, group, topdir) : -1;
}

int
RkwListDic(cn, dirname, names, size)
int cn, size;
char *dirname, *names;
{
  return Rk ? (*Rk->ListDic)(cn, dirname, names, size) : -1;
}

RkwCopyDic(cn, dir, from, to, mode)
int cn, mode;
char *dir, *from, *to;
{
  return Rk ? (*Rk->CopyDic)(cn, dir, from, to, mode) : -1;
}

RkwRemoveDic(cn, dicname, mode)
int cn, mode;
char *dicname;
{
  return Rk ? (*Rk->RemoveDic)(cn, dicname, mode) : -1;
}

RkwRenameDic(cn, from, to, mode)
int cn, mode;
char *from, *to;
{
  return Rk ? (*Rk->RenameDic)(cn, from, to, mode) : -1;
}

RkwChmodDic(cn, dicname, mode)
int cn, mode;
char *dicname;
{
  return Rk ? (*Rk->ChmodDic)(cn, dicname, mode) : -1;
}

RkwQueryDic(cn, dir, dic, stat)
int cn;
char *dir, *dic;
struct DicInfo *stat;
{
  return Rk ? (*Rk->QueryDic)(cn, dir, dic, stat) : -1;
}

int
RkwGetWordTextDic(cx_num, dirname, dicname, info, infolen)
int cx_num, infolen;
unsigned char *dirname, *dicname;
wchar_t *info;
{
  return 
    Rk ? (*Rk->GetWordTextDic)(cx_num, dirname, dicname, info, infolen) : -1;
}

int
RkwGetSimpleKanji(cxnum, dicname, yomi, maxyomi,
		  kanjis, maxkanjis, hinshis, maxhinshis)
int cxnum, maxyomi, maxkanjis, maxhinshis;
char *dicname;
wchar_t *yomi, *kanjis, *hinshis;
{
  return
    Rk ? (*Rk->GetSimpleKanji)(cxnum, dicname, yomi, maxyomi, kanjis,
			       maxkanjis, hinshis, maxhinshis) : -1;
}

#ifndef wchar_t
# error "wchar_t is already undefined"
#endif
#undef wchar_t
/*********************************************************************
 *                       wchar_t replace end                         *
 *********************************************************************/

#else /* !ENGINE_SWITCH */
#define CANNA_SERVER_NAME_LEN 128
static char iroha_server_name[CANNA_SERVER_NAME_LEN] = {0, 0};

RkSetServerName(s)
char *s;
{
  if (s)
    (void)strncpy(iroha_server_name, s, CANNA_SERVER_NAME_LEN);
  else
    iroha_server_name[0] = '\0';
  return 0;
}

char *
RkGetServerHost()
{
  if (iroha_server_name[0]) {
    return iroha_server_name;
  }
  else {
    return (char *)0;
  }
}
#endif /* !ENGINE_SWITCH */

void
close_engine()
{
#ifdef ENGINE_SWITCH
#ifdef DL
  if (dlh) {
    (void)dlclose(dlh);
    dlh = (DSOHANDLE)0;
  }
#endif /* DL */
  Rk = (struct rkfuncs *)0;
  current_engine = -1;
#endif /* ENGINE_SWITCH */
}

