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
extoken(char *s, char **next_return)
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
getengines(int *nengines)
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
useEngine(char *libname)
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
  return -1; /* ¥¨¥é¡¼ */
}

#else /* !DL */

static
useEngine(struct rkfuncs *libname)
{
  Rk = libname;
  return 0;
}

#endif /* !DL */

static
switch_engine(char *engine)
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

int
RkSetServerName(char *s)
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
RkGetServerHost(void)
{
  return server_host;
}

char *
RkGetServerEngine(void)
{
  return server_engine;
}

int
RkwGetProtocolVersion(int *map, int *mip)
{
  return Rk ? (*Rk->GetProtocolVersion)(map, mip) : -1;
}

char *
RkwGetServerName(void)
{
  return Rk ? (*Rk->GetServerName)() : (char *)0;
}

int
RkwGetServerVersion(int *map, int *mip)
{
  return Rk ? (*Rk->GetServerVersion)(map, mip) : -1;
}

int
RkwInitialize(char *host)
{
  return Rk ? (*Rk->Initialize)(host) : -1;
}

void
RkwFinalize(void)
{
  if (Rk) (*Rk->Finalize)();
}

int
RkwCreateContext(void)
{
  return Rk ? (*Rk->CreateContext)() : -1;
}

int
RkwDuplicateContext(int cn)
{
  return Rk ? (*Rk->DuplicateContext)(cn) : -1;
}

int
RkwCloseContext(int cn)
{
  return Rk ? (*Rk->CloseContext)(cn) : -1;
}

int
RkwSetDicPath(int cn, char *path)
{
  return Rk ? (*Rk->SetDicPath)(cn, path) : -1;
}

int
RkwCreateDic(int cn, char *dic, int mode)
{
  return Rk ? (*Rk->CreateDic)(cn, dic, mode) : -1;
}

int
RkwGetDicList(int cn, char *buf, int maxbuf)
{
  return Rk ? (*Rk->GetDicList)(cn, buf, maxbuf) : -1;
}

int
RkwGetMountList(int cn, char *buf, int maxbuf)
{
  return Rk ? (*Rk->GetMountList)(cn, buf, maxbuf) : -1;
}

int
RkwMountDic(int cn, char *dic, int f)
{
  return Rk ? (*Rk->MountDic)(cn, dic, f) : -1;
}

int
RkwRemountDic(int cn, char *dic, int where)
{
  return Rk ? (*Rk->RemountDic)(cn, dic, where) : -1;
}

int
RkwUnmountDic(int cn, char *dic)
{
  return Rk ? (*Rk->UnmountDic)(cn, dic) : -1;
}

int
RkwDefineDic(int cn, char *dic, wchar_t *word)
{
  return Rk ? (*Rk->DefineDic)(cn, dic, word) : -1;
}

int
RkwDeleteDic(int cn, char *dic, wchar_t *word)
{
  return Rk ? (*Rk->DeleteDic)(cn, dic, word) : -1;
}

int
RkwGetHinshi(int cn, wchar_t *buf, int maxbuf)
{
  return Rk ? (*Rk->GetHinshi)(cn, buf, maxbuf) : -1;
}

int
RkwGetKanji(int cn, wchar_t *buf, int maxbuf)
{
  return Rk ? (*Rk->GetKanji)(cn, buf, maxbuf) : -1;
}

int
RkwGetYomi(int cn, wchar_t *buf, int maxbuf)
{
  return Rk ? (*Rk->GetYomi)(cn, buf, maxbuf) : -1;
}

int
RkwGetLex(int cn, RkLex *buf, int maxbuf)
{
  return Rk ? (*Rk->GetLex)(cn, buf, maxbuf) : -1;
}

int
RkwGetStat(int cn, RkStat *buf)
{
  return Rk ? (*Rk->GetStat)(cn, buf) : -1;
}

int
RkwGetKanjiList(int cn, wchar_t *buf, int maxbuf)
{
  return Rk ? (*Rk->GetKanjiList)(cn, buf, maxbuf) : -1;
}

int
RkwFlushYomi(int cn)
{
  return Rk ? (*Rk->FlushYomi)(cn) : -1;
}

int
RkwGetLastYomi(int cn, wchar_t *buf, int maxbuf)
{
  return Rk ? (*Rk->GetLastYomi)(cn, buf, maxbuf) : -1;
}

int
RkwRemoveBun(int cn, int mode)
{
  return Rk ? (*Rk->RemoveBun)(cn, mode) : -1;
}

int
RkwSubstYomi(int cn, int s, int e, wchar_t *yomi, int len)
{
  return Rk ? (*Rk->SubstYomi)(cn, s, e, yomi, len) : -1;
}

int
RkwBgnBun(int cn, wchar_t *yomi, int len, int f)
{
  return Rk ? (*Rk->BgnBun)(cn, yomi, len, f) : -1;
}

int
RkwEndBun(int cn, int mode)
{
  return Rk ? (*Rk->EndBun)(cn, mode) : -1;
}

int
RkwGoTo(int cn, int where)
{
  return Rk ? (*Rk->GoTo)(cn, where) : -1;
}

int
RkwLeft(int cn)
{
  return Rk ? (*Rk->Left)(cn) : -1;
}

int
RkwRight(int cn)
{
  return Rk ? (*Rk->Right)(cn) : -1;
}

int
RkwNext(int cn)
{
  return Rk ? (*Rk->Next)(cn) : -1;
}

int
RkwPrev(int cn)
{
  return Rk ? (*Rk->Prev)(cn) : -1;
}

int
RkwNfer(int cn)
{
  return Rk ? (*Rk->Nfer)(cn) : -1;
}

int
RkwXfer(int cn, int knum)
{
  return Rk ? (*Rk->Xfer)(cn, knum) : -1;
}

int
RkwResize(int cn, int len)
{
  return Rk ? (*Rk->Resize)(cn, len) : -1;
}

int
RkwEnlarge(int cn)
{
  return Rk ? (*Rk->Enlarge)(cn) : -1;
}

int
RkwShorten(int cn)
{
  return Rk ? (*Rk->Shorten)(cn) : -1;
}

int
RkwStoreYomi(int cn, wchar_t *yomi, int len)
{
  return Rk ? (*Rk->StoreYomi)(cn, yomi, len) : -1;
}

int
RkwSetAppName(int cn, char *name)
{
  return Rk ? (*Rk->SetAppName)(cn, name) : -1;
}

int
RkwSync(int cn, char *name)
{
  return Rk ? (*Rk->SyncDic)(cn, name) : -1;
}

int
RkwSetUserInfo(char *user, char *group, char *topdir)
{
  return Rk ? (*Rk->SetUserInfo)(user, group, topdir) : -1;
}

int
RkwListDic(int cn, char *dirname, char *names, int size)
{
  return Rk ? (*Rk->ListDic)(cn, dirname, names, size) : -1;
}

int
RkwCopyDic(int cn, char *dir, char *from, char *to, int mode)
{
  return Rk ? (*Rk->CopyDic)(cn, dir, from, to, mode) : -1;
}

int
RkwRemoveDic(int cn, char *dicname, int mode)
{
  return Rk ? (*Rk->RemoveDic)(cn, dicname, mode) : -1;
}

int
RkwRenameDic(int cn, char *from, char *to, int mode)
{
  return Rk ? (*Rk->RenameDic)(cn, from, to, mode) : -1;
}

int
RkwChmodDic(int cn, char *dicname, int mode)
{
  return Rk ? (*Rk->ChmodDic)(cn, dicname, mode) : -1;
}

int
RkwQueryDic(int cn, char *dir, char *dic, struct DicInfo *stat)
{
  return Rk ? (*Rk->QueryDic)(cn, dir, dic, stat) : -1;
}

int
RkwGetWordTextDic(int cx_num, unsigned char *dirname, unsigned char *dicname, wchar_t *info, int infolen)
{
  return 
    Rk ? (*Rk->GetWordTextDic)(cx_num, dirname, dicname, info, infolen) : -1;
}

int
RkwGetSimpleKanji(int cxnum, char *dicname, wchar_t *yomi, int maxyomi, wchar_t *kanjis, int maxkanjis, wchar_t *hinshis, int maxhinshis)
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

int
RkSetServerName(char *s)
{
  if (s) {
    (void)strncpy(iroha_server_name, s, CANNA_SERVER_NAME_LEN - 1);
    iroha_server_name[CANNA_SERVER_NAME_LEN - 1] = '\0';
  } else {
    iroha_server_name[0] = '\0';
  }
  return 0;
}

char *
RkGetServerHost(void)
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
close_engine(void)
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

