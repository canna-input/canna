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
static char rcsid[] = "$Id: lisp.c,v 1.11.2.1 2004/04/26 22:49:21 aida_s Exp $";
#endif

/* 
** main program of lisp 
*/
#include "lisp.h"
#include "patchlevel.h"

#include <signal.h>

static FILE *outstream = (FILE *)0;

static char *celltop, *cellbtm, *freecell;
static char *memtop;

static int ncells = CELLSIZE;

static initIS();
static void finIS();
static allocarea(), skipspaces(), zaplin(), isterm();
static void prins();
static list mkatm(), read1(), ratom(), ratom2(), rstring();
static int tyipeek(), tyi();
static void tyo pro((int));
static void defatms(), epush();
static void push(), pop();
static int  evpsh();
static void freearea(), print();
static list getatm(), getatmz(), newsymbol(), copystring();
static list assq(), pop1();
static list Lprogn(), Lcons(), Lread();
static list Leval(), Lprint(), Lmodestr(), Lputd(), Lxcons(), Lncons();
static list NumAcc(), StrAcc();

/* parameter stack */

static list	*stack, *sp;

/* environment stack	*/

static list	*estack, *esp;

/* oblist */

static list	*oblist;	/* oblist hashing array		*/

#define LISPERROR	-1

typedef struct {
  FILE *f;
  char *name;
  unsigned line;
} lispfile;

static lispfile *files;
static int  filep;

/* lisp read buffer & read pointer */

static char *readbuf;		/* read buffer	*/
static char *readptr;		/* read pointer	*/

/* error functions	*/

static void	argnerr(), numerr(), error();

/* multiple values */

#define MAXVALUES 16
static list *values;	/* multiple values here	*/
static int  valuec;	/* number of values here	*/

/* symbols */

static list QUOTE, T, _LAMBDA, _MACRO, COND, USER;
static list BUSHU, GRAMMAR, RENGO, KATAKANA, HIRAGANA, HYPHEN;

#include <setjmp.h>

static struct lispcenv {
  jmp_buf jmp_env;
  int     base_stack;
  int     base_estack;
} *env; /* environment for setjmp & longjmp	*/
static int  jmpenvp = MAX_DEPTH;

static jmp_buf fatal_env;

/* external functions

   外部関数は以下の３つ

  (1) clisp_init()  --  カスタマイズファイルを読むための準備をする

    lisp の初期化を行い必要なメモリを allocate する。

  (2) clisp_fin()   --  カスタマイズ読み込み用の領域を解放する。

    上記の初期化で得たメモリを解放する。

  (3) YYparse_by_rcfilename((char *)s) -- カスタマイズファイルを読み込む。

    s で指定されたファイル名のカスタマイズファイルを読み込んでカスタ
    マイズの設定を行う。ファイルが存在すれば 1 を返しそうでなければ
    0 を返す。

 */

#ifdef __STDC__
static list getatmz(char *);
#else
static list getatmz();
#endif

/*********************************************************************
 *                      wchar_t replace begin                        *
 *********************************************************************/
#ifdef wchar_t
# error "wchar_t is already defined"
#endif
#define wchar_t cannawc


int
clisp_init()
{
  int  i;

  if ( !allocarea() ) {
    return 0;
  }

  if ( !initIS() ) {
    freearea();
    return 0;
  }

  /* stack pointer initialization	*/
  sp = stack + STKSIZE;
  esp = estack + STKSIZE;
  epush(NIL);

  /* initialize read pointer	*/
  readptr = readbuf;
  *readptr = '\0';
  files[filep = 0].f = stdin;
  files[filep].name = (char *)0;
  files[filep].line = 0;

  /* oblist initialization	*/
  for (i = 0; i < BUFSIZE ; i++)
    oblist[i] = 0;

  /* symbol definitions */
  defatms();
  return 1;
}

#ifndef NO_EXTEND_MENU
static void
fillMenuEntry()
{
  extern extraFunc *FindExtraFunc(), *extrafuncp;
  extraFunc *p, *fp;
  int i, n, fid;
  menuitem *mb;

  for (p = extrafuncp ; p ; p = p->next) {
    if (p->keyword == EXTRA_FUNC_DEFMENU) {
      n = p->u.menuptr->nentries;
      mb = p->u.menuptr->body;
      for (i = 0 ; i < n ; i++, mb++) {
	if (mb->flag == MENU_SUSPEND) {
	  list l = (list)mb->u.misc;
	  fid = symbolpointer(l)->fid;
	  if (fid < CANNA_FN_MAX_FUNC) {
	    goto just_a_func;
	  }
	  else {
	    fp = FindExtraFunc(fid);
	    if (fp && fp->keyword == EXTRA_FUNC_DEFMENU) {
	      mb->u.menu_next = fp->u.menuptr;
	      mb->flag = MENU_MENU;
	    }
	    else {
	    just_a_func:
	      mb->u.fnum = fid;
	      mb->flag = MENU_FUNC;
	    }
	  }
	}
      }
    }
  }
}
#endif /* NO_EXTEND_MENU */

#define UNTYIUNIT 32
static char *untyibuf = 0;
static int untyisize = 0, untyip = 0;

void
clisp_fin()
{
#ifndef NO_EXTEND_MENU
  /* 終るに当たって、menu 関連のデータを埋める */
  fillMenuEntry();
#endif

  finIS();

  while (filep >= 0) {
    if (files[filep].f && files[filep].f != stdin) {
      fclose(files[filep].f);
    }
    if (files[filep].name) {
      free(files[filep].name);
    }
    filep--;
  }

  freearea();
  if (untyisize) {
    free(untyibuf);
    untyisize = 0;
    untyibuf = (char *)0;
  }
}

int
YYparse_by_rcfilename(s)
char *s;
{
  extern ckverbose;
  int retval = 0;
  FILE *f;
  FILE *saved_outstream;

  if (setjmp(fatal_env)) {
    retval = 0;
    goto quit_parse_rcfile;
  }

  if (jmpenvp <= 0) { /* 再帰が深すぎる場合 */
    return 0;
  }
  jmpenvp--;

  if (ckverbose >= CANNA_HALF_VERBOSE) {
    saved_outstream = outstream;
    outstream = stdout;
  }

  f = fopen(s, "r");
  if (f) {
    if (ckverbose == CANNA_FULL_VERBOSE) {
      printf("カスタマイズファイルとして \"%s\" を用います。\n", s);
    }
    files[++filep].f = f;
    files[filep].name = malloc(strlen(s) + 1);
    if (files[filep].name) {
      strcpy(files[filep].name, s);
    }
    else {
      filep--;
      fclose(f);
      goto quit_parse_rcfile;
    }
    files[filep].line = 0;

    setjmp(env[jmpenvp].jmp_env);
    env[jmpenvp].base_stack = sp - stack;
    env[jmpenvp].base_estack = esp - estack;

    for (;;) {
      push(Lread(0));
      if (valuec > 1 && null(values[1])) {
	break;
      }
      (void)Leval(1);
    }
    retval = 1;
  }

  if (ckverbose >= CANNA_HALF_VERBOSE) {
    outstream = saved_outstream;
  }

  jmpenvp++;
 quit_parse_rcfile:
  return retval;
}

#define WITH_MAIN
#ifdef WITH_MAIN

static void
intr(sig)
int sig;
/* ARGSUSED */
{
  error("Interrupt:",NON);
  /* NOTREACHED */
}

/* cfuncdef

   parse_string -- 文字列をパースする。

*/

parse_string(str)
char *str;
{
  char *readbufbk;

  if (clisp_init() == 0) {
    return -1;
  }

  /* read buffer として与えられた文字を使う */
  readbufbk = readbuf;
  readptr = readbuf = str;

  if (setjmp(fatal_env)) {
    goto quit_parse_string;
  }

  if (jmpenvp <= 0) { /* 再帰が深すぎる場合 */
    return -1;
  }

  jmpenvp--;
  files[++filep].f = (FILE *)0;
  files[filep].name = (char *)0;
  files[filep].line = 0;

  setjmp(env[jmpenvp].jmp_env);
  env[jmpenvp].base_stack = sp - stack;
  env[jmpenvp].base_estack = esp - estack;

  for (;;) {
    list t;

    t = Lread(0);
    if (valuec > 1 && null(values[1])) {
      break;
    }
    else {
      push(t);
      Leval(1);
    }
  }
  jmpenvp++;
 quit_parse_string:
  readbuf = readbufbk;
  clisp_fin();
  return 0;
}

static void intr();

void
clisp_main()
{
  if (clisp_init() == 0) {	/* initialize data area	& etc..	*/
    fprintf(stderr, "CannaLisp: initialization failed.\n");
    exit(1);
  }

  if (setjmp(fatal_env)) {
    goto quit_clisp_main;
  }

  if (jmpenvp <= 0) { /* 再帰が深すぎる場合 */
    return;
  }
  jmpenvp--;

  fprintf(stderr,"CannaLisp listener %d.%d%s\n",
	  CANNA_MAJOR_MINOR / 1000, CANNA_MAJOR_MINOR % 1000,
	  CANNA_PATCH_LEVEL);

  outstream = stdout;

  setjmp(env[jmpenvp].jmp_env);
  env[jmpenvp].base_stack = sp - stack;
  env[jmpenvp].base_estack = esp - estack;

  signal(SIGINT, intr);
  for (;;) {
    prins("-> ");		/* prompt	*/
    push(Lread(0));
    if (valuec > 1 && null(values[1])) {
      break;
    }
    push(Leval(1));
    if (sp[0] == LISPERROR) {
      (void)pop1();
    }
    else {
      (void)Lprint(1);
      prins("\n");
    }
  }
  jmpenvp++;
 quit_clisp_main:
  prins("\nGoodbye.\n");
  clisp_fin();
}

#endif /* WITH_MAIN */

static int longestkeywordlen;

typedef struct {
  char *seq;
  int id;
} SeqToID;

static SeqToID keywordtable[] = {
  {"Space"      ,' '},
  {"Escape"     ,'\033'},
  {"Tab"        ,'\t'},
  {"Nfer"       ,CANNA_KEY_Nfer},
  {"Xfer"       ,CANNA_KEY_Xfer},
  {"Backspace"  ,'\b'},
  {"Delete"     ,'\177'},
  {"Insert"     ,CANNA_KEY_Insert},
  {"Rollup"     ,CANNA_KEY_Rollup},
  {"Rolldown"   ,CANNA_KEY_Rolldown},
  {"Up"         ,CANNA_KEY_Up},
  {"Left"       ,CANNA_KEY_Left},
  {"Right"      ,CANNA_KEY_Right},
  {"Down"       ,CANNA_KEY_Down},
  {"Home"       ,CANNA_KEY_Home},
  {"Clear"      ,'\013'},
  {"Help"       ,CANNA_KEY_Help},
  {"End"        ,CANNA_KEY_End},
  {"Enter"      ,'\n'},
  {"Return"     ,'\r'},
/* "F1" is processed by program */
  {"F2"         ,CANNA_KEY_F2},
  {"F3"         ,CANNA_KEY_F3},
  {"F4"         ,CANNA_KEY_F4},
  {"F5"         ,CANNA_KEY_F5},
  {"F6"         ,CANNA_KEY_F6},
  {"F7"         ,CANNA_KEY_F7},
  {"F8"         ,CANNA_KEY_F8},
  {"F9"         ,CANNA_KEY_F9},
  {"F10"        ,CANNA_KEY_F10},
/* "Pf1" is processed by program */
  {"Pf2"        ,CANNA_KEY_PF2},
  {"Pf3"        ,CANNA_KEY_PF3},
  {"Pf4"        ,CANNA_KEY_PF4},
  {"Pf5"        ,CANNA_KEY_PF5},
  {"Pf6"        ,CANNA_KEY_PF6},
  {"Pf7"        ,CANNA_KEY_PF7},
  {"Pf8"        ,CANNA_KEY_PF8},
  {"Pf9"        ,CANNA_KEY_PF9},
  {"Pf10"       ,CANNA_KEY_PF10},
  {"Hiragana"   ,CANNA_KEY_HIRAGANA},
  {"Katakana"   ,CANNA_KEY_KATAKANA},
  {"Hankakuzenkaku" ,CANNA_KEY_HANKAKUZENKAKU},
  {"Eisu"       ,CANNA_KEY_EISU},
  {"S-Nfer"     ,CANNA_KEY_Shift_Nfer},
  {"S-Xfer"     ,CANNA_KEY_Shift_Xfer},
  {"S-Up"       ,CANNA_KEY_Shift_Up},
  {"S-Down"     ,CANNA_KEY_Shift_Down},
  {"S-Left"     ,CANNA_KEY_Shift_Left},
  {"S-Right"    ,CANNA_KEY_Shift_Right},
  {"C-Nfer"     ,CANNA_KEY_Cntrl_Nfer},
  {"C-Xfer"     ,CANNA_KEY_Cntrl_Xfer},
  {"C-Up"       ,CANNA_KEY_Cntrl_Up},
  {"C-Down"     ,CANNA_KEY_Cntrl_Down},
  {"C-Left"     ,CANNA_KEY_Cntrl_Left},
  {"C-Right"    ,CANNA_KEY_Cntrl_Right},
  {0            ,0},
};

#define charToNum(c) charToNumTbl[(c) - ' ']

static int *charToNumTbl;

typedef struct {
  int id;
  int *tbl;
} seqlines;

static seqlines *seqTbl;	/* 内部の表(実際には表の表) */
static int nseqtbl;		/* 状態の数。状態の数だけ表がある */
static int nseq;
static int seqline;

static
initIS()
{
  SeqToID *p;
  char *s;
  int i;
  seqlines seqTbls[1024];

  seqTbl = (seqlines *)0;
  seqline = 0;
  nseqtbl = 0;
  nseq = 0;
  longestkeywordlen = 0;
  for (i = 0 ; i < 1024 ; i++) {
    seqTbls[i].tbl = (int *)0;
    seqTbls[i].id = 0;
  }
  charToNumTbl = (int *)calloc('~' - ' ' + 1, sizeof(int));
  if ( !charToNumTbl ) {
    return 0;
  }

  /* まず何文字使われているかを調べる。
     nseq は使われている文字数より１大きい値である */
  for (p = keywordtable ; p->id ; p++) {
    int len = 0;
    for (s = p->seq ; *s ; s++) {
      if ( !charToNumTbl[*s - ' '] ) {
	charToNumTbl[*s - ' '] = nseq; /* 各文字にシリアル番号を振る */
	nseq++;
      }
      len ++;
    }
    if (len > longestkeywordlen) {
      longestkeywordlen = len;
    }
  }
  /* 文字数分のテーブル */
  seqTbls[nseqtbl].tbl = (int *)calloc(nseq, sizeof(int));
  if ( !seqTbls[nseqtbl].tbl ) {
    goto initISerr;
  }
  nseqtbl++;
  for (p = keywordtable ; p->id ; p++) {
    int line, nextline;
    
    line = 0;
    for (s = p->seq ; *s ; s++) {
      if (seqTbls[line].tbl == 0) { /* テーブルがない */
	seqTbls[line].tbl = (int *)calloc(nseq, sizeof(int));
	if ( !seqTbls[line].tbl ) {
	  goto initISerr;
	}
      }
      nextline = seqTbls[line].tbl[charToNum(*s)];
      /* ちなみに、charToNum(*s) は絶対に０にならない */
      if ( nextline ) {
	line = nextline;
      }
      else { /* 最初にアクセスした */
	line = seqTbls[line].tbl[charToNum(*s)] = nseqtbl++;
      }
    }
    seqTbls[line].id = p->id;
  }
  seqTbl = (seqlines *)calloc(nseqtbl, sizeof(seqlines));
  if ( !seqTbl ) {
    goto initISerr;
  }
  for (i = 0 ; i < nseqtbl ; i++) {
    seqTbl[i].id  = seqTbls[i].id;
    seqTbl[i].tbl = seqTbls[i].tbl;
  }
  return 1;

 initISerr:
  free(charToNumTbl);
  charToNumTbl = (int *)0;
  if (seqTbl) {
    free(seqTbl);
    seqTbl = (seqlines *)0;
  }
  for (i = 0 ; i < nseqtbl ; i++) {
    if (seqTbls[i].tbl) {
      free(seqTbls[i].tbl);
      seqTbls[i].tbl = (int *)0;
    }
  }
  return 0;
}

static void
finIS() /* identifySequence に用いたメモリ資源を開放する */
{
  int i;

  if (seqTbl) {
    for (i = 0 ; i < nseqtbl ; i++) {
      if (seqTbl[i].tbl) free(seqTbl[i].tbl);
      seqTbl[i].tbl = (int *)0;
    }
    free(seqTbl);
    seqTbl = (seqlines *)0;
  }
  if (charToNumTbl) {
    free(charToNumTbl);
    charToNumTbl = (int *)0;
  }
}

/* cvariable

  seqline: identifySequence での状態を保持する変数

 */

#define CONTINUE 1
#define END	 0

static
identifySequence(c, val)
unsigned c;
int *val;
{
  int nextline;

  if (' ' <= c && c <= '~' && charToNum(c) &&
      (nextline = seqTbl[seqline].tbl[charToNum(c)]) ) {
    seqline = nextline;
    *val = seqTbl[seqline].id;
    if (*val) {
      seqline = 0;
      return END;
    }
    else {
      return CONTINUE; /* continue */
    }
  }
  else {
    *val = -1;
    seqline = 0;
    return END;
  }
}


static int
alloccell()
{
  int  cellsize, odd;
  char *p;

  cellsize = ncells * sizeof(list);
  p = malloc(cellsize);
  if (p == (char *)0) {
    return 0;
  }
  memtop = p;
  odd = (int)((pointerint)memtop % sizeof(list));
  freecell = celltop = memtop + (odd ? (sizeof(list)) - odd : 0);
  cellbtm = memtop + cellsize - odd;
  return 1;
}

/* うまく行かなかったら０を返す */

static
allocarea()
{
  /* まずはセル領域 */
  if (alloccell()) {
    /* スタック領域 */
    stack = (list *)calloc(STKSIZE, sizeof(list));
    if (stack) {
      estack = (list *)calloc(STKSIZE, sizeof(list));
      if (estack) {
	/* oblist */
	oblist = (list *)calloc(BUFSIZE, sizeof(list));
	if (oblist) {
	  /* I/O */
	  filep = 0;
	  files = (lispfile *)calloc(MAX_DEPTH, sizeof(lispfile));
	  if (files) {
	    readbuf = malloc(BUFSIZE);
	    if (readbuf) {
	      /* jump env */
	      jmpenvp = MAX_DEPTH;
	      env = (struct lispcenv *)
		calloc(MAX_DEPTH, sizeof(struct lispcenv));
	      if (env) {
		/* multiple values returning buffer */
		valuec = 1;
		values = (list *)calloc(MAXVALUES, sizeof(list));
		if (values) {
		  return 1;
		}
		free(env);
	      }
	      free(readbuf);
	    }
	    free(files);
	  }
	  free(oblist);
	}
	free(estack);
      }
      free(stack);
    }
    free(memtop);
  }
  return 0;
}

static void
freearea()
{
  free((char *)memtop);
  free((char *)stack);
  free((char *)estack);
  free((char *)oblist);
  free((char *)files);
  free((char *)env);
  free((char *)readbuf);
  if (values) {
    free(values);
    values = 0;
  }
}

static list
getatmz(name)
char *name;
{
  int  key;
  char *p;

  for (p = name, key = 0 ; *p ; p++)
    key += *p;
  return getatm(name,key);
}

/* mkatm -
	making symbol function	*/

static list 
mkatm(name)
char *name;
{
  list temp;
  struct atomcell *newatom;

  temp = newsymbol(name);
  newatom = symbolpointer(temp);
  newatom->value = (*name == ':') ? (list)temp : (list)UNBOUND;
  newatom->plist = NIL;			/* set null plist	*/
  newatom->ftype = UNDEF;		/* set undef func-type	*/
  newatom->func  = (list (*)())0;	/* Don't kill this line	*/
  newatom->valfunc  = (list (*)())0;	/* Don't kill this line	*/
  newatom->hlink = NIL;		/* no hash linking	*/
  newatom->mid = -1;
  newatom->fid = -1;

  return temp;
}

/* getatm -- get atom from the oblist if possible	*/

static list 
getatm(name,key)
char *name;
int  key;
{
  list p;
  struct atomcell *atomp;

  key &= 0x00ff;
  for (p = oblist[key] ; p ;) {
    atomp = symbolpointer(p);
    if (!strcmp(atomp->pname, name)) {
      return p;
    }
    p = atomp->hlink;
  }
  p = mkatm(name);
  atomp = symbolpointer(p);
  atomp->hlink = oblist[key];
  oblist[key] = p;
  return p;
}

#define MESSAGE_MAX 256

static void
error(msg,v)
char *msg;
list v;
/* ARGSUSED */
{
  char buf[MESSAGE_MAX];

  prins(msg);
  if (v != (list)NON)
    print(v);
  if (files[filep].f == stdin) {
    prins("\n");
  }
  else {
    if (files[filep].name) {
      sprintf(buf, " (%s near line %d)\n",
	      files[filep].name, files[filep].line);
    }
    else {
      sprintf(buf, " (near line %d)\n", files[filep].line);
    }
    prins(buf);
  }
  sp = &stack[env[jmpenvp].base_stack];
  esp = &estack[env[jmpenvp].base_estack];
/*  epush(NIL); */
  longjmp(env[jmpenvp].jmp_env,YES);
}

static void
fatal(msg,v)
char *msg;
list v;
/* ARGSUSED */
{
  char buf[MESSAGE_MAX];

  prins(msg);
  if (v != (list)NON)
    print(v);
  if (files[filep].f == stdin) {
    prins("\n");
  }
  else {
    if (files[filep].name) {
      sprintf(buf, " (%s near line %d)\n",
	      files[filep].name, files[filep].line);
    }
    else {
      sprintf(buf, " (near line %d)\n", files[filep].line);
    }
    prins(buf);
  }
  longjmp(fatal_env, 1);
}

static void
argnerr(msg)
char *msg;
{
  prins("incorrect number of args to ");
  error(msg, NON);
  /* NOTREACHED */
}

static void
numerr(fn,arg)
char *fn;
list arg;
{
  prins("Non-number ");
  if (fn) {
    prins("to ");
    prins(fn);
  }
  error(": ",arg);
  /* NOTREACHED */
}

static void
lisp_strerr(fn,arg)
char *fn;
list arg;
{
  prins("Non-string ");
  if (fn) {
    prins("to ");
    prins(fn);
  }
  error(": ",arg);
  /* NOTREACHED */
}

static list
Lread(n)
int n;
{
  list t;

  argnchk("read",0);
  valuec = 1;
  if ((t = read1()) == (list)LISPERROR) {
    readptr = readbuf;
    *readptr = '\0';
    if (files[filep].f != stdin) {
      if (files[filep].f)
	fclose(files[filep].f);
      if (files[filep].name) {
	free(files[filep].name);
      }
      filep--;
    }
    values[0] = NIL;
    values[1] = NIL;
    valuec = 2;
    return(NIL);
  }
  else {
    values[0] = t;
    values[1] = T;
    valuec = 2;
    return(t);
  }
  /* NOTREACHED */
}

static void untyi pro((int));
static list rcharacter pro((void));

static list
read1()
{
  int  c;
  list p, *pp;
  list t;
  char *eofmsg = "EOF hit in reading a list : ";

 lab:
  if ( !skipspaces() ) {
    return((list)LISPERROR);
  }
  switch (c = tyi()) {
  case '(':
    push(NIL);
    p = Lncons(1);	/* get a new cell	*/
    car(p) = p;
    push(p);
    pp = sp;
    
    for (;;) {
    lab2:
      if ( !skipspaces() ) {
	error(eofmsg,cdr(*pp));
	/* NOTREACHED */
      }
      switch (c = tyi()) {
      case ';':
	zaplin();
	goto lab2;
      case ')':
	return(cdr(pop1()));
      case '.':
	if ( !(c = tyipeek()) ) {
	  error(eofmsg,cdr(*pp));
	  /* NOTREACHED */
	}
	else if ( !isterm(c) ) {
	  push(ratom2('.'));
	  push(NIL);
	  car(*pp) = cdar(*pp) = Lcons(2);
	  break;
	}
	else {
	  cdar(*pp) = read1();
	  if (cdar(*pp) == (list)LISPERROR) {
	    error(eofmsg,cdr(*pp));
	    /* NOTREACHED */
	  }
	  while (')' != (c = tyi()))
	    if ( !c ) {
	      error(eofmsg,cdr(*pp));
	      /* NOTREACHED */
	    }
	  return(cdr(pop1()));
	}
      default:
	untyi(c);
	if ((t = read1()) == (list)LISPERROR) {
	  error(eofmsg,cdr(*pp));
	  /* NOTREACHED */
	}
	push(t);
	push(NIL);
	car(*pp) = cdar(*pp) = Lcons(2);
      }
    }
  case '\'':
    push(QUOTE);
    if ((t = read1()) == (list)LISPERROR) {
      error(eofmsg,NIL);
      /* NOTREACHED */
    }
    push(t);
    push(NIL);
    push(Lcons(2));
    return Lcons(2);
  case '"':
    return rstring();
  case '?':
    return rcharacter();
  case ';':
    zaplin();
    goto lab;
  default:
    untyi(c);
    return ratom();
  }
}

/* skipping spaces function -
	if eof read then return NO	*/

static
skipspaces()
{
  int c;

  while ((c = tyi()) <= ' ') {
    if ( !c ) {
      return(NO);
    }
#ifdef QUIT_IF_BINARY_CANNARC
/* 実は fatal() にしてしまうと read できなかったと思い、次のファイルを
   探しに行くのであまり良くない。return を受けたところも変えなければな
   らない。面倒なので、とりあえず外す */
    if (c != '\033' && c != '\n' && c != '\r' && c!= '\t' && c < ' ') {
      fatal("read: Binary data read.", NON);
    }
#endif
  }
  untyi(c);
  return(YES);
}

/* skip reading until '\n' -
	if eof read then return NO	*/

static
zaplin()
{
	int c;

	while ((c = tyi()) != '\n')
		if ( !c )
			return(NO);
	return(YES);
}

static void gc();

static list
newcons()
{
  list retval;

  if (freecell + sizeof(struct cell) >= cellbtm) {
    gc();
  }
  retval = CONS_TAG | (freecell - celltop);
  freecell += sizeof(struct cell);
  return retval;
}

static list
newsymbol(name)
char *name;
{
  list retval;
  struct atomcell *temp;
  int namesize;

  namesize = strlen(name);
  namesize = ((namesize / sizeof(list)) + 1) * sizeof(list); /* +1は'\0'の分 */
  if (freecell + (sizeof(struct atomcell)) + namesize >= cellbtm) {
    gc();
  }
  temp = (struct atomcell *)freecell;
  retval = SYMBOL_TAG | (freecell - celltop);
  freecell += sizeof(struct atomcell);
  (void)strcpy(freecell, name);
  temp->pname = freecell;
  freecell += namesize;
  
  return retval;
}

static void patom();

static void
print(l)
list l;
{
	if ( !l )	/* case NIL	*/
		prins("nil");
	else if (atom(l))
		patom(l);
	else {
		tyo('(');
		print(car(l));
		for (l = cdr(l) ; l ; l = cdr(l)) {
			tyo(' ');
			if (atom(l)) {
				tyo('.');
				tyo(' ');
				patom(l);
				break;
			}
			else 
				print(car(l));
		}
		tyo(')');
	}
}



/*
** read atom
*/


static list 
ratom()
{
	return(ratom2(tyi()));
}

/* read atom with the first one character -
	check if the token is numeric or pure symbol & return proper value */

static isnum();

static list 
ratom2(a)
int  a;
{
  int  i, c, flag;
  char atmbuf[BUFSIZE];

  flag = NO;
  if (a == '\\') {
    flag = YES;
    a = tyi();
  }
  atmbuf[0] = a;
  for (i = 1, c = tyi(); !isterm(c) ; i++, c = tyi()) {
    if ( !c ) {
      error("Eof hit in reading symbol.", NON);
      /* NOTREACHED */
    }
    if (c == '\\') {
      flag = YES;
    }
    if (i < BUFSIZE) {
      atmbuf[i] = c;
    }
    else {
      error("Too long symbol name read", NON);
      /* NOTREACHED */
    }
  }
  untyi(c);
  if (i < BUFSIZE) {
    atmbuf[i] = '\0';
  }
  else {
    error("Too long symbol name read", NON);
    /* NOTREACHED */
  }
  if ( !flag && isnum(atmbuf)) {
    return(mknum(atoi(atmbuf)));
  }
  else if ( !flag && !strcmp("nil",atmbuf) ) {
    return(NIL);
  }
  else {
    return (getatmz(atmbuf));
  }
}

static list
rstring()
{
  char strb[BUFSIZE];
  int c;
  int strp = 0;

  while ((c = tyi()) != '"') {
    if ( !c ) {
      error("Eof hit in reading a string.", NON);
      /* NOTREACHED */
    }
    if (strp < BUFSIZE) {
      if (c == '\\') {
	untyi(c);
	c = (char)(((canna_uintptr_t)rcharacter()) & 0xff);
      }
      strb[strp++] = (char)c;
    }
    else {
      error("Too long string read.", NON);
      /* NOTREACHED */
    }
  }
  if (strp < BUFSIZE) {
    strb[strp] = '\0';
  }
  else {
    error("Too long string read.", NON);
    /* NOTREACHED */
  }

  return copystring(strb, strp);
}

/* rcharacter -- 一文字読んで来る。 */

static list
rcharacter()
{
  char *tempbuf;
  unsigned ch;
  list retval;
  int bufp;

  tempbuf = malloc(longestkeywordlen + 1);
  if ( !tempbuf ) {
    fatal("read: malloc failed in reading character.", NON);
    /* NOTREACHED */
  }
  bufp = 0;

  ch = tyi();
  if (ch == '\\') {
    int code, res;

    do { /* キーワードと照合する */
      tempbuf[bufp++] = ch = tyi();
      res = identifySequence(ch, &code);
    } while (res == CONTINUE);
    if (code != -1) { /* キーワードと一致した。 */
      retval = mknum(code);
    }
    else if (bufp > 2 && tempbuf[0] == 'C' && tempbuf[1] == '-') {
      while (bufp > 3) {
	untyi(tempbuf[--bufp]);
      }
      retval = mknum(tempbuf[2] & (' ' - 1));
    }
    else if (bufp == 3 && tempbuf[0] == 'F' && tempbuf[1] == '1') {
      untyi(tempbuf[2]);
      retval = mknum(CANNA_KEY_F1);
    }
    else if (bufp == 4 && tempbuf[0] == 'P' && tempbuf[1] == 'f' &&
	     tempbuf[2] == '1') {
      untyi(tempbuf[3]);
      retval = mknum(CANNA_KEY_PF1);
    }
    else { /* 全然駄目 */
      while (bufp > 1) {
	untyi(tempbuf[--bufp]);
      }
      ch = (unsigned)(unsigned char)tempbuf[0];
      goto return_char;
    }
  }
  else {
  return_char:
    if (ch == 0x8f) { /* SS3 */
      ch <<= 8;
      ch += tyi();
      goto shift_more;
    }
    else if (ch & 0x80) { /* う〜ん、日本語に依存している */
    shift_more:
      ch <<= 8;
      ch += tyi();
    }
    retval = mknum(ch);
  }

  free(tempbuf);
  return retval;
}

static isnum(name)
char *name;
{
	if (*name == '-') {
		name++;
		if ( !*name )
			return(NO);
	}
	for(; *name ; name++) {
		if (*name < '0' || '9' < *name) {
			if (*name != '.' || *(name + 1)) {
				return(NO);
			}
		}
	}
	return(YES);
}

/* tyi -- input one character from buffered stream	*/

static void
untyi(c)
int c;
{
  if (readbuf < readptr) {
    *--readptr = c;
  }
  else {
    if (untyip >= untyisize) {
      if (untyisize == 0) {
	untyibuf = malloc(UNTYIUNIT);
	if (untyibuf) {
	  untyisize = UNTYIUNIT;
	}
      }
      else {
	untyibuf = realloc(untyibuf, UNTYIUNIT + untyisize);
	if (untyibuf) {
	  untyisize += UNTYIUNIT;
	}
      }
    }
    if (untyip < untyisize) { /* それでもチェックする */
      untyibuf[untyip++] = c;
    }
  }
}

static int
tyi()
{
  char *gets(), *fgets();

  if (untyibuf) {
    int ret = untyibuf[--untyip];
    if (untyip == 0) {
      free(untyibuf);
      untyibuf = (char *)0;
      untyisize = 0;
    }
    return ret;
  }

  if (readptr && *readptr) {
    return ((int)(unsigned char)*readptr++);
  }
  else if (!files[filep].f) {
    return NO;
  }
  else if (files[filep].f == stdin) {
    readptr = fgets(readbuf, BUFSIZE, stdin);
    files[filep].line++;
    if ( !readptr ) {
      return NO;
    }
    else {
      return tyi();
    }
  }
  else {
    readptr = fgets(readbuf,BUFSIZE,files[filep].f);
    files[filep].line++;
    if (readptr) {
      return(tyi());
    }
    else {
      return(NO);
    }
  }
  /* NOTREACHED */
}

/* tyipeek -- input one character without advance the read pointer	*/

static int
tyipeek()
{
  int c = tyi();
  untyi(c);
  return c;
}

/* tyo -- output one character	*/

static void tyo(c)
int c;
{
  if (outstream) {
    (void)putc(c, outstream);
  }
}
	

/* prins -
	print string	*/

static void prins(s)
char *s;
{
	while (*s) {
		tyo(*s++);
	}
}


/* isterm -
	check if the character is terminating the lisp expression	*/

static isterm(c)
int  c;
{
	if (c <= ' ')
		return(YES);
	else {
		switch (c)
		{
		case '(':
		case ')':
		case ';':
			return(YES);
		default:
			return(NO);
		}
	}
}

/* push down an S-expression to parameter stack	*/

static void
push(value)
list value;
{
  if (sp <= stack) {
    error("Stack over flow",NON);
    /* NOTREACHED */
  }
  else
    *--sp = value;
}

/* pop up n S-expressions from parameter stack	*/

static void 
pop(x)
int  x;
{
  if (0 < x && sp >= &stack[STKSIZE]) {
    error("Stack under flow",NON);
    /* NOTREACHED */
  }
  sp += x;
}

/* pop up an S-expression from parameter stack	*/

static list 
pop1()
{
  if (sp >= &stack[STKSIZE]) {
    error("Stack under flow",NON);
    /* NOTREACHED */
  }
  return(*sp++);
}

static void
epush(value)
list value;
{
  if (esp <= estack) {
    error("Estack over flow",NON);
    /* NOTREACHED */
  }
  else
    *--esp = value;
}

static list 
epop()
{
  if (esp >= &estack[STKSIZE]) {
    error("Lstack under flow",NON);
    /* NOTREACHED */
  }
  return(*esp++);
}


/*
** output function for lisp S-Expression
*/


/*
**  print atom function
**  please make sure it is an atom (not list)
**  no check is done here.
*/

static void
patom(atm)
list atm;
{
  char namebuf[BUFSIZE];

  if (constp(atm)) {
    if (numberp(atm)) {
      (void)sprintf(namebuf,"%d",(int)xnum(atm));
      prins(namebuf);
    }
    else {		/* this is a string */
      int i, len = xstrlen(atm);
      char *s = xstring(atm);

      tyo('"');
      for (i = 0 ; i < len ; i++) {
	tyo(s[i]);
      }
      tyo('"');
    }
  }
  else {
    prins(symbolpointer(atm)->pname);
  }
}

static void markcopycell();

static char *oldcelltop;
static char *oldcellp;

#define oldpointer(x) (oldcelltop + celloffset(x))

static void
gc() /* コピー方式のガーベジコレクションである */
{
  int i;
  list *p;
  static int under_gc = 0;

  if (under_gc) {
    fatal("GC: memory exhausted.", NON);
  }
  else {
    under_gc = 1;
  }

  oldcellp = memtop; oldcelltop = celltop;

  if ( !alloccell() ) {
    fatal("GC: failed in allocating new cell area.", NON);
    /* NOTREACHED */
  }

  for (i = 0 ; i < BUFSIZE ; i++) {
    markcopycell(oblist + i);
  }
  for (p = sp ; p < &stack[STKSIZE] ; p++) {
    markcopycell(p);
  }
  for (p = esp ; p < &estack[STKSIZE] ; p++) {
    markcopycell(p);
  }
  for (i = 0 ; i < valuec ; i++) {
    markcopycell(values + i);
  }
  markcopycell(&T);
  markcopycell(&QUOTE);
  markcopycell(&_LAMBDA);
  markcopycell(&_MACRO);
  markcopycell(&COND);
  markcopycell(&USER);
  markcopycell(&BUSHU);
  markcopycell(&GRAMMAR);
  markcopycell(&RENGO);
  markcopycell(&KATAKANA);
  markcopycell(&HIRAGANA);
  markcopycell(&HYPHEN);
  free(oldcellp);
  if ((freecell - celltop) * 2 > cellbtm -celltop) {
    ncells = (freecell - celltop) * 2 / sizeof(list);
  }
  under_gc = 0;
}

static char *Strncpy();

static list
allocstring(n)
int n;
{
  int namesize;
  list retval;

  namesize = ((n + (sizeof(pointerint)) + 1 + 3)/ sizeof(list)) * sizeof(list);
  if (freecell + namesize >= cellbtm) { /* gc 中は起こり得ないはず */
    gc();
  }
  ((struct stringcell *)freecell)->length = n;
  retval = STRING_TAG | (freecell - celltop);
  freecell += namesize;
  return retval;
}

static list
copystring(s, n)
char *s;
int n;
{
  list retval;

  retval = allocstring(n);
  (void)Strncpy(xstring(retval), s, n);
  xstring(retval)[n] = '\0';
  return retval;
}

static list
copycons(l)
struct cell *l;
{
  list newcell;

  newcell = newcons();
  car(newcell) = l->head;
  cdr(newcell) = l->tail;
  return newcell;
}

static void
markcopycell(addr)
list *addr;
{
  list temp;
 redo:
  if (null(*addr) || numberp(*addr)) {
    return;
  }
  else if (alreadycopied(oldpointer(*addr))) {
    *addr = newaddr(gcfield(oldpointer(*addr)));
    return;
  }
  else if (stringp(*addr)) {
    temp = copystring(((struct stringcell *)oldpointer(*addr))->str,
		      ((struct stringcell *)oldpointer(*addr))->length);
    gcfield(oldpointer(*addr)) = mkcopied(temp);
    *addr = temp;
    return;
  }
  else if (consp(*addr)) {
    temp = copycons((struct cell *)(oldpointer(*addr)));
    gcfield(oldpointer(*addr)) = mkcopied(temp);
    *addr = temp;
    markcopycell(&car(temp));
    addr = &cdr(temp);
    goto redo;
  }
  else { /* symbol */
    struct atomcell *newatom, *oldatom;

    oldatom = (struct atomcell *)(oldpointer(*addr));
    temp = newsymbol(oldatom->pname);
    newatom = symbolpointer(temp);
    newatom->value = oldatom->value;
    newatom->plist = oldatom->plist;
    newatom->ftype = oldatom->ftype;
    newatom->func  = oldatom->func;
    newatom->fid   = oldatom->fid;
    newatom->mid   = oldatom->mid;
    newatom->valfunc  = oldatom->valfunc;
    newatom->hlink = oldatom->hlink;

    gcfield(oldpointer(*addr)) = mkcopied(temp);
    *addr = temp;

    if (newatom->value != (list)UNBOUND) {
      markcopycell(&newatom->value);
    }
    markcopycell(&newatom->plist);
    if (newatom->ftype == EXPR || newatom->ftype == MACRO) {
      markcopycell((int *)&newatom->func);
    }
    addr = &newatom->hlink;
    goto redo;
  }
}

static list
bindall(var,par,a,e)
list var, par, a, e;
{
  list *pa, *pe, retval;

  push(a); pa = sp;
  push(e); pe = sp;
 retry:
  if (constp(var)) {
    pop(2);
    return(*pa);
  }
  else if (atom(var)) {
    push(var);
    push(par);
    push(Lcons(2));
    push(*pa);
    retval = Lcons(2);
    pop(2);
    return retval;
  }
  else if (atom(par)) {
    error("Bad macro form ",e);
    /* NOTREACHED */
  }
  push(par);
  push(var);
  *pa = bindall(car(var),car(par),*pa,*pe);
  var = cdr(pop1());
  par = cdr(pop1());
  goto retry;
  /* NOTREACHED */
}

static list
Lquote()
{
	list p;

	p = pop1();
	if (atom(p))
		return(NIL);
	else
		return(car(p));
}

static list
Leval(n)
int n;
{
  list e, t, s, tmp, aa, *pe, *pt, *ps, *paa;
  list fn, (*cfn)(), *pfn;
  int i, j;
  argnchk("eval",1);
  e = sp[0];
  pe = sp;
  if (atom(e)) {
    if (constp(e)) {
      pop1();
      return(e);
    }
    else {
      struct atomcell *sym;

      t = assq(e, *esp);
      if (t) {
	(void)pop1();
	return(cdr(t));
      }
      else if ((sym = symbolpointer(e))->valfunc) {
	(void)pop1();
	return (sym->valfunc)(VALGET, 0);
      }
      else {
	if ((t = (sym->value)) != (list)UNBOUND) {
	  pop1();
	  return(t);
	}
	else {
	  error("Unbound variable: ",*pe);
	  /* NOTREACHED */
	}
      }
    }
  }
  else if (constp((fn = car(e)))) {	/* not atom	*/
    error("eval: undefined function ", fn);
    /* NOTREACHED */
  }
  else if (atom(fn)) {
    switch (symbolpointer(fn)->ftype) {
    case UNDEF:
      error("eval: undefined function ", fn);
      /* NOTREACHED */
      break;
    case SUBR:
      cfn = symbolpointer(fn)->func;
      i = evpsh(cdr(e));
      epush(NIL);
      t = (*cfn)(i);
      epop();
      pop1();
      return (t);
    case SPECIAL:
      push(cdr(e));
      t = (*(symbolpointer(fn)->func))();
      pop1();
      return (t);
    case EXPR:
      fn = (list)(symbolpointer(fn)->func);
      aa = NIL; /* previous env won't be used */
    expr:
      if (atom(fn) || car(fn) != _LAMBDA || atom(cdr(fn))) {
	error("eval: bad lambda form ", fn);
	/* NOTREACHED */
      }
/* Lambda binding begins here ...					*/
      s = cdr(e);		/* actual parameter	*/
      t = cadr(fn);		/* lambda list		*/
      push(s); ps = sp;
      push(t); pt = sp;
      push(fn); pfn = sp;
      push(aa); paa = sp;
      i = 0;			/* count of variables	*/
      for (; consp(*ps) && consp(*pt) ; *ps = cdr(*ps), *pt = cdr(*pt)) {
	if (consp(car(*pt))) {
	  tmp = cdar(*pt);	/* push the cdr of element */
	  if (!(atom(tmp) || null(cdr(tmp)))) {
	    push(cdr(tmp));
	    push(T);
	    push(Lcons(2));
	    i++;
	  }
	  push(caar(*pt));
	}
	else {
	  push(car(*pt));
	}
	push(car(*ps));
	push(Leval(1));
	push(Lcons(2));
	i++;
      }
      for (; consp(*pt) ; *pt = cdr(*pt)) {
	if (atom(car(*pt))) {
	  error("Too few actual parameters ",*pe);
	  /* NOTREACHED */
	}
	else {
	  tmp = cdar(*pt);
	  if (!(atom(tmp) || null(cdr(tmp)))) {
	    push(cdr(tmp));
	    push(NIL);
	    push(Lcons(2));
	    i++;
	  }
	  push(caar(*pt));
	  tmp = cdar(*pt); /* restore for GC */
	  if (atom(tmp))
	    push(NIL);
	  else {
	    push(car(tmp));
	    push(Leval(1));
	  }
	  push(Lcons(2));
	  i++;
	}
      }
      if (null(*pt) && consp(*ps)) {
	error("Too many actual arguments ",*pe);
	/* NOTREACHED */
      }
      else if (*pt) {
	push(*pt);
	for (j = 1 ; consp(*ps) ; j++) {
	  push(car(*ps));
	  push(Leval(1));
	  *ps = cdr(*ps);
	}
	push(NIL);
	for (; j ; j--) {
	  push(Lcons(2));
	}
	i++;
      }
      push(*paa);
      for (; i ; i--) {
	push(Lcons(2));
      }
/* Lambda binding finished, and a new environment is established.	*/
      epush(pop1());	/* set the new environment	*/
      push(cddr(*pfn));
      t = Lprogn();
      epop();
      pop(5);
      return (t);
    case MACRO:
      fn = (list)(symbolpointer(fn)->func);
      if (atom(fn) || car(fn) != _MACRO || atom(cdr(fn))) {
	error("eval: bad macro form ",fn);
	/* NOTREACHED */
      }
      s = cdr(e);	/* actual parameter	*/
      t = cadr(fn);	/* lambda list	*/
      push(fn);
      epush(bindall(t,s,NIL,e));
      push(cddr(pop1()));
      t = Lprogn();
      epop();
      push(t);
      push(t);
      s = Leval(1);
      t = pop1();
      if (!atom(t)) {
	car(*pe) = car(t);
	cdr(*pe) = cdr(t);
      }
      pop1();
      return (s);
    case CMACRO:
      push(e);
      push(t = (*(symbolpointer(fn)->func))());
      push(t);
      s = Leval(1);
      t = pop1();
      if (!atom(t)) {
	car(e) = car(t);
	cdr(e) = cdr(t);
      }
      pop1();
      return (s);
    default:
      error("eval: unrecognized ftype used in ", fn);
      /* NOTREACHED */
      break;
    }
    /* NOTREACHED */
  }
  else {	/* fn is list (lambda expression)	*/
    aa = *esp; /* previous environment is also used */
    goto expr;
  }
  /* maybe NOTREACHED */
  return NIL;
}

static list
assq(e,a)
list e, a;
{
  list i;

  for (i = a ; i ; i = cdr(i)) {
    if (consp(car(i)) && e == caar(i)) {
      return(car(i));
    }
  }
  return((list)NIL);
}

/* eval each argument and push down each value to parameter stack	*/

static int
evpsh(args)
list args;
{
  int  counter;
  list temp;

  counter = 0;
  while (consp(args)) {
    push(args);
    push(car(args));
    temp = Leval(1);
    args = cdr(pop1());
    counter++;
    push(temp);
  }
  return (counter);
}

/*
static int
psh(args)
list args;
{
  int  counter;

  counter = 0;
  while (consp(args)) {
    push(car(args));
    counter++;
    args = cdr(args);
  }
  return (counter);
}
*/

static list
Lprogn()
{
  list val, *pf;

  val = NIL;
  pf = sp;
  for (; consp(*pf) ; *pf = cdr(*pf)) {
    symbolpointer(T)->value = T;
    push(car(*pf));
    val = Leval(1);
  }
  pop1();
  return (val);
}

static list
Lcons(n)
int n;
{
	list temp;

	argnchk("cons",2);
	temp = newcons();
	cdr(temp) = pop1();
	car(temp) = pop1();
	return(temp);
}

static list 
Lncons(n)
int n;
{
	list temp;

	argnchk("ncons",1);
	temp = newcons();
	car(temp) = pop1();
	cdr(temp) = NIL;
	return(temp);
}

static list
Lxcons(n)
int n;
{
	list temp;

	argnchk("cons",2);
	temp = newcons();
	car(temp) = pop1();
	cdr(temp) = pop1();
	return(temp);
}

static list 
Lprint(n)
int n;
{
	print(sp[0]);
	pop(n);
	return (T);
}

static list
Lset(n)
int n;
{
  list val, t;
  list var;
  struct atomcell *sym;

  argnchk("set",2);
  val = pop1();
  var = pop1();
  if (!symbolp(var)) {
    error("set/setq: bad variable type  ",var);
    /* NOTREACHED */
  }
  sym = symbolpointer(var);
  t = assq(var,*esp);
  if (t) {
    return cdr(t) = val;
  }
  else if (sym->valfunc) {
    return (*(sym->valfunc))(VALSET, val);
  }
  else {
    return sym->value = val;	/* global set	*/
  }
}

static list
Lsetq()
{
  list a, *pp;

  a = NIL;
  for (pp = sp; consp(*pp) ; *pp = cdr(*pp)) {
    push(car(*pp));
    *pp = cdr(*pp);
    if ( atom(*pp) ) {
      error("Odd number of args to setq",NON);
      /* NOTREACHED */
    }
    push(car(*pp));
    push(Leval(1));
    a = Lset(2);
  }
  pop1();
  return(a);
}

static int equal();

static list 
Lequal(n)
int n;
{
  argnchk("equal (=)",2);
  if (equal(pop1(),pop1()))
    return(T);
  else
    return(NIL);
}

/* null 文字で終わらない strncmp */

static int
Strncmp(x, y, len)
char *x, *y;
int len;
{
  int i;

  for (i = 0 ; i < len ; i++) {
    if (x[i] != y[i]) {
      return (x[i] - y[i]);
    }
  }
  return 0;
}

/* null 文字で終わらない strncpy */

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

static int
equal(x,y)
list x, y;
{
 equaltop:
  if (x == y)
    return(YES);
  else if (null(x) || null(y))
    return(NO);
  else if (numberp(x) || numberp(y)) {
    return NO;
  }
  else if (stringp(x)) {
    if (stringp(y)) {
      return ((xstrlen(x) == xstrlen(y)) ?
	      (!Strncmp(xstring(x), xstring(y), xstrlen(x))) : 0);
    }
    else {
      return NO;
    }
  }
  else if (symbolp(x) || symbolp(y)) {
    return(NO);
  }
  else {
    if (equal(car(x), car(y))) {
      x = cdr(x);
      y = cdr(y);
      goto equaltop;
    }
    else 
      return(NO);
  }
}

static list 
Lgreaterp(n)
int n;
{
  list p;
  pointerint x, y;

  if ( !n )
    return(T);
  else {
    p = pop1();
    if (!numberp(p)) {
      numerr("greaterp",p);
      /* NOTREACHED */
    }
    x = xnum(p);
    for (n-- ; n ; n--) {
      p = pop1();
      if (!numberp(p)) {
	numerr("greaterp",p);
	/* NOTREACHED */
      }
      y = xnum(p);
      if (y <= x)		/* !(y > x)	*/
	return(NIL);
      x = y;
    }
    return(T);
  }
}

static list 
Llessp(n)
int n;
{
  list p;
  pointerint x, y;

  if ( !n )
    return(T);
  else {
    p = pop1();
    if (!numberp(p)) {
      numerr("lessp",p);
      /* NOTREACHED */
    }
    x = xnum(p);
    for (n-- ; n ; n--) {
      p = pop1();
      if (!numberp(p)) {
	numerr("lessp",p);
	/* NOTREACHED */
      }
      y = xnum(p);
      if (y >= x)		/* !(y < x)	*/
	return(NIL);
      x = y;
    }
    return(T);
  }
}

static list
Leq(n)
int n;
{
  list f;

  argnchk("eq",2);
  f = pop1();
  if (f == pop1())
    return(T);
  else
    return(NIL);
}

static list
Lcond()
{
  list *pp, t, a, c;

  pp = sp;
  for (; consp(*pp) ; *pp = cdr(*pp)) {
    t = car(*pp);
    if (atom(t)) {
      pop1();
      return (NIL);
    }
    else {
      push(cdr(t));
      if ((c = car(t)) == T || (push(c), (a = Leval(1)))) {
	/* if non NIL */
	t = pop1();
	if (null(t)) {	/* if cdr is NIL */
	  (void)pop1();
	  return (a);
	}
	else {
	  (void)pop1();
	  push(t);
	  return(Lprogn());
	}
      }
      else {
	(void)pop1();
      }
    }
  }
  pop1();
  return (NIL);
}

static list
Lnull(n)
int n;
{
  argnchk("null",1);
  if (pop1())
    return NIL;
  else
    return T;
}

static list 
Lor()
{
  list *pp, t;

  for (pp = sp; consp(*pp) ; *pp = cdr(*pp)) {
    push(car(*pp));
    t = Leval(1);
    if (t) {
      pop1();
      return(t);
    }
  }
  pop1();
  return(NIL);
}

static list 
Land()
{
  list *pp, t;

  t = T;
  for (pp = sp; consp(*pp) ; *pp = cdr(*pp)) {
    push(car(*pp));
    if ( !(t = Leval(1)) ) {
      pop1();
      return(NIL);
    }
  }
  pop1();
  return(t);
}

static list 
Lplus(n)
int n;
{
  list t;
  int  i;
  pointerint sum;

  i = n;
  sum = 0;
  while (i--) {
    t = sp[i];
    if ( !numberp(t) ) {
      numerr("+",t);
      /* NOTREACHED */
    }
    else {
      sum += xnum(t);
    }
  }
  pop(n);
  return(mknum(sum));
}

static list
Ltimes(n)
int n;
{
  list t;
  int  i;
  pointerint sum;

  i = n;
  sum = 1;
  while (i--) {
    t = sp[i];
    if ( !numberp(t) ) {
      numerr("*",t);
      /* NOTREACHED */
    }
    else
      sum *= xnum(t);
  }
  pop(n);
  return(mknum(sum));
}

static list
Ldiff(n)
int n;
{
  list t;
  int  i;
  pointerint sum;

  if ( !n )
    return(mknum(0));
  t = sp[n - 1];
  if ( !numberp(t) ) {
    numerr("-",t);
    /* NOTREACHED */
  }
  sum = xnum(t);
  if (n == 1) {
    pop1();
    return(mknum(-sum));
  }
  else {
    i = n - 1;
    while (i--) {
      t = sp[i];
      if ( !numberp(t) ) {
	numerr("-",t);
	/* NOTREACHED */
      }
      else
	sum -= xnum(t);
    }
    pop(n);
    return(mknum(sum));
  }
}

static list 
Lquo(n)
int n;
{
  list t;
  int  i;
  pointerint sum;

  if ( !n )
    return(mknum(1));
  t = sp[n - 1];
  if ( !numberp(t) ) {
    numerr("/",t);
    /* NOTREACHED */
  }
  sum = xnum(t);
  i = n - 1;
  while (i--) {
    t = sp[i];
    if ( !numberp(t) ) {
      numerr("/",t);
      /* NOTREACHED */
    }
    else if (xnum(t) != 0) {
      sum = sum / (pointerint)xnum(t); /* CP/M68K is bad...	*/
    }
    else { /* division by zero */
      error("Division by zero",NON);
    }
  }
  pop(n);
  return(mknum(sum));
}

static list 
Lrem(n)
int n;
{
  list t;
  int  i;
  pointerint sum;

  if ( !n )
    return(mknum(0));
  t = sp[n - 1];
  if ( !numberp(t) ) {
    numerr("%",t);
    /* NOTREACHED */
  }
  sum = xnum(t);
  i = n - 1;
  while (i--) {
    t = sp[i];
    if ( !numberp(t) ) {
      numerr("%",t);
      /* NOTREACHED */
    }
    else if (xnum(t) != 0) {
      sum = sum % (pointerint)xnum(t); /* CP/M68K is bad ..	*/
    }
    else { /* division by zero */
      error("Division by zero",NON);
    }
  }
  pop(n);
  return(mknum(sum));
}

/*
 *	Garbage Collection
 */

static list 
Lgc(n)
int n;
{
  argnchk("gc",0);
  gc();
  return(NIL);
}

static list
Lusedic(n)
int n;
{
  int i;
  list retval = NIL, temp;
  int dictype;
  extern struct dicname *kanjidicnames;
  struct dicname *kanjidicname;
  extern int auto_define;
  extern char *kataautodic;
#ifdef HIRAGANAAUTO
  extern char *hiraautodic;
#endif

  for (i = n ; i ; i--) {
    temp = sp[i - 1];
    dictype = DIC_PLAIN;
    if (symbolp(temp) && i - 1 > 0) {
      if (temp == USER) {
	dictype = DIC_USER;
      }
      else if (temp == BUSHU) {
	dictype = DIC_BUSHU;
      }
      else if (temp == GRAMMAR) {
	dictype = DIC_GRAMMAR;
      }
      else if (temp == RENGO) {
	dictype = DIC_RENGO;
      }
      else if (temp == KATAKANA) {
	dictype = DIC_KATAKANA;
        auto_define = 1;
      }
      else if (temp == HIRAGANA) {
	dictype = DIC_HIRAGANA;
      }
      i--; temp = sp[i - 1];
    }
    if (stringp(temp)) {
      kanjidicname  = (struct dicname *)malloc(sizeof(struct dicname));
      if (kanjidicname) {
	kanjidicname->name = malloc(strlen(xstring(temp)) + 1);
	if (kanjidicname->name) {
	  strcpy(kanjidicname->name , xstring(temp));
	  kanjidicname->dictype = dictype;
	  kanjidicname->dicflag = DIC_NOT_MOUNTED;
	  kanjidicname->next = kanjidicnames;
	  kanjidicnames = kanjidicname;
	  if (kanjidicname->dictype == DIC_KATAKANA) {
	    if (!kataautodic) { /* only the first one is valid */
	      kataautodic = kanjidicname->name;
	    }
	  }
#ifdef HIRAGANAAUTO
	  else if (kanjidicname->dictype == DIC_HIRAGANA) {
	    if (!hiraautodic) { /* only the first one is valid */
	      hiraautodic = kanjidicname->name;
	    }
	  }
#endif
	  retval = T;
	  continue;
	}
	free((char *)kanjidicname);
      }
    }
  }
  pop(n);
  return retval;
}

static list
Llist(n)
int n;
{
	push(NIL);
	for (; n ; n--) {
		push(Lcons(2));
	}
	return (pop1());
}

static list
Lcopysym(n)
int n;
{
  list src, dst;
  struct atomcell *dsta, *srca;

  argnchk("copy-symbol",2);
  src = pop1();
  dst = pop1();
  if (!symbolp(dst)) {
    error("copy-symbol: bad arg  ", dst);
    /* NOTREACHED */
  }
  if (!symbolp(src)) {
    error("copy-symbol: bad arg  ", src);
    /* NOTREACHED */
  }
  dsta = symbolpointer(dst);
  srca = symbolpointer(src);
  dsta->plist   = srca->plist;
  dsta->value   = srca->value;
  dsta->ftype   = srca->ftype;
  dsta->func    = srca->func;
  dsta->valfunc = srca->valfunc;
  dsta->mid     = srca->mid;
  dsta->fid     = srca->fid;
  return src;
}

static list
Lload(n)
int n;
{
  list p, t;
  FILE *instream, *fopen();

  argnchk("load",1);
  p = pop1();
  if ( !stringp(p) ) {
    error("load: illegal file name  ",p);
    /* NOTREACHED */
  }
  if ((instream = fopen(xstring(p), "r")) == (FILE *)NULL) {
    error("load: file not found  ",p);
    /* NOTREACHED */
  }
  prins("[load ");
  print(p);
  prins("]\n");

  if (jmpenvp <= 0) { /* 再帰が深すぎる場合 */
    return NIL;
  }
  jmpenvp--;
  files[++filep].f = instream;
  files[filep].name = malloc(xstrlen(p) + 1);
  if (files[filep].name) {
    strcpy(files[filep].name, xstring(p));
  }
  files[filep].line = 0;

  setjmp(env[jmpenvp].jmp_env);
  env[jmpenvp].base_stack = sp - stack;
  env[jmpenvp].base_estack = esp - estack;

  for (;;) {
    t = Lread(0);
    if (valuec > 1 && null(values[1])) {
      break;
    }
    else {
      push(t);
      Leval(1);
    }
  }
  jmpenvp++;
  return(T);
}

static list
Lmodestr(n)
int n;
{
  list p;
  int mode;

  argnchk(S_SetModeDisp, 2);
  if ( !null(p = sp[0]) && !stringp(p) ) {
    lisp_strerr(S_SetModeDisp, p);
    /* NOTREACHED */
  }
  if (!symbolp(sp[1]) || (mode = symbolpointer(sp[1])->mid) == -1) {
    error("Illegal mode ", sp[1]);
    /* NOTREACHED */
  }
  changeModeName(mode, null(p) ? 0 : xstring(p));
  pop(2);
  return p;
}

/* 機能シーケンスの取り出し */

static int
xfseq(fname, l, arr, arrsize)
char *fname;
list l;
unsigned char *arr;
int arrsize;
{
  int i;

  if (atom(l)) {
    if (symbolp(l) &&
	(arr[0] = (unsigned char)(symbolpointer(l)->fid)) != 255) {
      arr[1] = 0;
    }
    else {
      prins(fname);
      error(": illegal function ", l);
      /* NOTREACHED */
    }
    return 1;
  }
  else {
    for (i = 0 ; i < arrsize - 1 && consp(l) ; i++, l = cdr(l)) {
      list temp = car(l);

      if (!symbolp(temp) ||
	  (arr[i] = (unsigned char)(symbolpointer(temp)->fid)) == 255) {
	prins(fname);
	error(": illegal function ", temp);
	/* NOTREACHED */
      }
    }
    arr[i] = 0;
    return i;
  }
}

static list
Lsetkey(n)
int n;
{
  list p;
  int mode, slen;
  unsigned char fseq[256];
  unsigned char keyseq[256];
  int retval;

  argnchk(S_SetKey, 3);
  if ( !stringp(p = sp[1]) ) {
    lisp_strerr(S_SetKey, p);
    /* NOTREACHED */
  }
  if (!symbolp(sp[2]) || (mode = symbolpointer(sp[2])->mid) < 0 ||
      (CANNA_MODE_MAX_REAL_MODE <= mode &&
       mode < CANNA_MODE_MAX_IMAGINARY_MODE &&
       mode != CANNA_MODE_HenkanNyuryokuMode)) {
    error("Illegal mode for set-key ", sp[2]);
    /* NOTREACHED */
  }
  if (xfseq(S_SetKey, sp[0], fseq, 256)) {
    slen = xstrlen(p);
    Strncpy((char *)keyseq, xstring(p), slen);
    keyseq[slen] = 255;
    retval = changeKeyfunc(mode, (unsigned)keyseq[0],
		           slen > 1 ? CANNA_FN_UseOtherKeymap :
                           (fseq[1] != 0 ? CANNA_FN_FuncSequence : fseq[0]),
                           fseq, keyseq);
    if (retval == NG) {
      error("Insufficient memory.", NON);
      /* NOTREACHED */
    }
  }
  pop(3);
  return p;
}

static list
Lgsetkey(n)
int n;
{
  list p;
  int slen;
  unsigned char fseq[256];
  unsigned char keyseq[256];
  int retval;

  argnchk(S_GSetKey, 2);
  if ( !stringp(p = sp[1]) ) {
    lisp_strerr(S_GSetKey, p);
    /* NOTREACHED */
  }
  if (xfseq(S_GSetKey, sp[0], fseq, 256)) {
    slen = xstrlen(p);
    Strncpy((char *)keyseq, xstring(p), slen);
    keyseq[slen] = 255;
    retval = changeKeyfuncOfAll((unsigned)keyseq[0],
               slen > 1 ? CANNA_FN_UseOtherKeymap :
               (fseq[1] != 0 ? CANNA_FN_FuncSequence : fseq[0]),
               fseq, keyseq);
    if (retval == NG) {
      error("Insufficient memory.", NON);
      /* NOTREACHED */
    }
    pop(2);
    return p;
  }
  else {
    pop(2);
    return NIL;
  }
}

static list
Lputd(n)
int n;
{
  list body, a;
  list sym;
  struct atomcell *symp;

  argnchk("putd",2);
  a = body = pop1();
  sym = pop1();
  symp = symbolpointer(sym);
  if (constp(sym) || consp(sym)) {
    error("putd: function name must be a symbol : ",sym);
    /* NOTREACHED */
  }
  if (null(body)) {
    symp->ftype = UNDEF;
    symp->func = (list (*)())UNDEF;
  }
  else if (consp(body)) {
    if (car(body) == _MACRO) {
      symp->ftype = MACRO;
      symp->func = (list (*)())body;
    }
    else {
      symp->ftype = EXPR;
      symp->func = (list (*)())body;
    }
  }
  return(a);
}

static list
Ldefun()
{
  list form, res;

  form = sp[0];
  if (atom(form)) {
    error("defun: illegal form ",form);
    /* NOTREACHED */
  }
  push(car(form));
  push(_LAMBDA);
  push(cdr(form));
  push(Lcons(2));
  Lputd(2);
  res = car(pop1());
  return (res);
}

static list
Ldefmacro()
{
  list form, res;

  form = sp[0];
  if (atom(form)) {
    error("defmacro: illegal form ",form);
    /* NOTREACHED */
  }
  push(res = car(form));
  push(_MACRO);
  push(cdr(form));
  push(Lcons(2));
  Lputd(2);
  pop1();
  return (res);
}

static list
Lcar(n)
int n;
{
  list f;

  argnchk("car",1);
  f = pop1();
  if (!f)
    return(NIL);
  else if (atom(f)) {
    error("Bad arg to car ",f);
    /* NOTREACHED */
  }
  return(car(f));
}

static list
Lcdr(n)
int n;
{
  list f;

  argnchk("cdr",1);
  f = pop1();
  if (!f)
    return(NIL);
  else if (atom(f)) {
    error("Bad arg to cdr ",f);
    /* NOTREACHED */
  }
  return(cdr(f));
}

static list
Latom(n)
int n;
{
  list f;

  argnchk("atom",1);
  f = pop1();
  if (atom(f))
    return(T);
  else
    return(NIL);
}

static list
Llet()
{
  list lambda, args, p, *pp, *pq, *pl, *px;

  px = sp;
  *px = cdr(*px);
  if (atom(*px)) {
    (void)pop1();
    return(NIL);
  }
  else {
    push(NIL);
    args = Lncons(1);
    push(args); pq = sp;
    push(NIL);
    lambda = p = Lncons(1);
    push(lambda);

    push(p); pp = sp;
    push(*pq); pq = sp;
    push(NIL); pl = sp;
    for (*pl = car(*px) ; consp(*pl) ; *pl = cdr(*pl)) {
      if (atom(car(*pl))) {
	push(car(*pl));
	*pp = cdr(*pp) = Lncons(1);
	push(NIL);
	*pq = cdr(*pq) = Lncons(1);
      }
      else if (atom(cdar(*pl))) {
	push(caar(*pl));
	*pp = cdr(*pp) = Lncons(1);
	push(NIL);
	*pq = cdr(*pq) = Lncons(1);
      }
      else {
	push(caar(*pl));
	*pp = cdr(*pp) = Lncons(1);
	push(cadr(car(*pl)));
	*pq = cdr(*pq) = Lncons(1);
      }
    }
    pop(3);
    sp[0] = cdr(sp[0]);
    sp[1] = cdr(sp[1]);
    push(cdr(*px));
    push(Lcons(2));
    push(_LAMBDA);
    push(Lxcons(2));
    p = Lxcons(2);
    (void)pop1();
    return(p);
  }
}

/* (if con tr . falist) -> (cond (con tr) (t . falist))*/

static list
Lif()
{
  list x, *px, retval;

  x = cdr(sp[0]);
  if (atom(x) || atom(cdr(x))) {
    (void)pop1();
    return NIL;
  }
  else {
    push(x); px = sp;

    push(COND);

    push(car(x));
    push(cadr(x));
    push(Llist(2));

    push(T);
    push(cddr(*px));
    push(Lcons(2));

    retval = Llist(3);
    pop(2);
    return retval;
  }
}

static list
Lunbindkey(n)
int n;
{
  unsigned char fseq[2];
  static unsigned char keyseq[2] = {(unsigned char)CANNA_KEY_Undefine,
				      (unsigned char)255};
  int mode;
  list retval;

  argnchk(S_UnbindKey, 2);
  if (!symbolp(sp[1]) || (mode = symbolpointer(sp[1])->mid) == -1) {
    error("Illegal mode ", sp[1]);
    /* NOTREACHED */
  }
  if (xfseq(S_UnbindKey, sp[0], fseq, 2)) {
    int ret;
    ret = changeKeyfunc(mode, CANNA_KEY_Undefine,
                        fseq[1] != 0 ? CANNA_FN_FuncSequence : fseq[0],
                        fseq, keyseq);
    if (ret == NG) {
      error("Insufficient memory.", NON);
      /* NOTREACHED */
    }
    retval = T;
  }
  else {
    retval = NIL;
  }
  pop(2);
  return retval;
}

static list
Lgunbindkey(n)
int n;
{
  unsigned char fseq[2];
  static unsigned char keyseq[2] = {(unsigned char)CANNA_KEY_Undefine,
				      (unsigned char)255};
  list retval;

  argnchk(S_GUnbindKey, 1);
  if (xfseq(S_GUnbindKey, sp[0], fseq, 2)) {
    int ret;
    ret = changeKeyfuncOfAll(CANNA_KEY_Undefine,
		       fseq[1] != 0 ? CANNA_FN_FuncSequence : fseq[0],
		       fseq, keyseq);
    if (ret == NG) {
      error("Insufficient memory.", NON);
      /* NOTREACHED */
    }
    retval = T;
  }
  else {
    retval = NIL;
  }
  (void)pop1();
  return retval;
}

#define DEFMODE_MEMORY      0
#define DEFMODE_NOTSTRING   1
#define DEFMODE_ILLFUNCTION 2

static list
Ldefmode()
{
  list form, *sym, e, *p, fn, rd, md, us;
  extern extraFunc *extrafuncp;
  extern int nothermodes;
  extraFunc *extrafunc = (extraFunc *)0;
  int i, j;
  int ecode;
  list l, edata;

  form = pop1();
  if (atom(form)) {
    error("Bad form ", form);
    /* NOTREACHED */
  }
  push(car(form));
  sym = sp;
  if (!symbolp(*sym)) {
    error("Symbol data expected ", *sym);
    /* NOTREACHED */
  }

  /* 引数をプッシュする */
  for (i = 0, e = cdr(form) ; i < 4 ; i++, e = cdr(e)) {
    if (atom(e)) {
      for (j = i ; j < 4 ; j++) {
	push(NIL);
      }
      break;
    }
    push(car(e));
  }
  if (consp(e)) {
    error("Bad form ", form);
    /* NOTREACHED */
  }

  /* 評価する */
  for (i = 0, p = sym - 1 ; i < 4 ; i++, p--) {
    push(*p);
    push(Leval(1));
  }
  us = pop1();
  fn = pop1();
  rd = pop1();
  md = pop1();
  pop(4);

  ecode = DEFMODE_MEMORY;
  extrafunc = (extraFunc *)malloc(sizeof(extraFunc));
  if (extrafunc) {
    /* シンボルの関数値としての定義 */
    symbolpointer(*sym)->mid = CANNA_MODE_MAX_IMAGINARY_MODE + nothermodes;
    symbolpointer(*sym)->fid =
      extrafunc->fnum = CANNA_FN_MAX_FUNC + nothermodes;

    /* デフォルトの設定 */
    extrafunc->display_name = (wchar_t *)NULL;
    extrafunc->u.modeptr = (newmode *)malloc(sizeof(newmode));
    if (extrafunc->u.modeptr) {
      KanjiMode kanjimode;

      extrafunc->u.modeptr->romaji_table = (char *)0;
      extrafunc->u.modeptr->romdic = (struct RkRxDic *)0;
      extrafunc->u.modeptr->romdic_owner = 0;
      extrafunc->u.modeptr->flags = CANNA_YOMI_IGNORE_USERSYMBOLS;
      extrafunc->u.modeptr->emode = (KanjiMode)0;

      /* モード構造体の作成 */
      kanjimode = (KanjiMode)malloc(sizeof(KanjiModeRec));
      if (kanjimode) {
	int searchfunc();
	extern KanjiModeRec empty_mode;
	extern BYTE *emptymap;

	kanjimode->func = searchfunc;
	kanjimode->keytbl = emptymap;
	kanjimode->flags = 
	  CANNA_KANJIMODE_TABLE_SHARED | CANNA_KANJIMODE_EMPTY_MODE;
	kanjimode->ftbl = empty_mode.ftbl;
	extrafunc->u.modeptr->emode = kanjimode;

	/* モード表示文字列 */
	ecode = DEFMODE_NOTSTRING;
	edata = md;
	if (stringp(md) || null(md)) {
	  if (stringp(md)) {
	    extrafunc->display_name = WString(xstring(md));
	  }
	  ecode = DEFMODE_MEMORY;
	  if (null(md) || extrafunc->display_name) {
	    /* ローマ字かな変換テーブル */
	    ecode = DEFMODE_NOTSTRING;
	    edata = rd;
	    if (stringp(rd) || null(rd)) {
	      char *newstr;
	      long f = extrafunc->u.modeptr->flags;

	      if (stringp(rd)) {
		newstr = malloc(strlen(xstring(rd)) + 1);
	      }
	      ecode = DEFMODE_MEMORY;
	      if (null(rd) || newstr) {
		if (!null(rd)) {
		  strcpy(newstr, xstring(rd));
		  extrafunc->u.modeptr->romaji_table = newstr;
		}
		/* 実行機能 */
		for (e = fn ; consp(e) ; e = cdr(e)) {
		  l = car(e);
		  if (symbolp(l) && symbolpointer(l)->fid) {
		    switch (symbolpointer(l)->fid) {
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
		      goto defmode_not_function;
		    }
		  }
		  else {
		    goto defmode_not_function;
		  }
		}
		extrafunc->u.modeptr->flags = f;

		/* ユーザシンボルの使用の有無 */
		if (us) {
		  extrafunc->u.modeptr->flags &=
		    ~CANNA_YOMI_IGNORE_USERSYMBOLS;
		}
 
		extrafunc->keyword = EXTRA_FUNC_DEFMODE;
		extrafunc->next = extrafuncp;
		extrafuncp = extrafunc;
		nothermodes++;
		return pop1();

	      defmode_not_function:
		ecode = DEFMODE_ILLFUNCTION;
		edata = l;
		if (!null(rd)) {
		  free(newstr);
		}
	      }
	    }
	    if (extrafunc->display_name) {
	      WSfree(extrafunc->display_name);
	    }
	  }
	}
	free((char *)kanjimode);
      }
      free((char *)extrafunc->u.modeptr);
    }
    free((char *)extrafunc);
  }
  switch (ecode) {
  case DEFMODE_MEMORY:
    error("Insufficient memory", NON);
  case DEFMODE_NOTSTRING:
    error("String data expected ", edata);
  case DEFMODE_ILLFUNCTION:
    error("defmode: illegal subfunction ", edata);
  }
  /* NOTREACHED */
}

static list
Ldefsym()
{
  list form, res, e;
  int i, ncand, group;
  wchar_t cand[1024], *p, *mcand, **acand, key, xkey;
  int mcandsize;
  extern nkeysup;
  extern keySupplement keysup[];

  form = sp[0];
  if (atom(form)) {
    error("Illegal form ",form);
    /* NOTREACHED */
  }
  /* まず数をかぞえる */
  for (ncand = 0 ; consp(form) ; ) {
    e = car(form);
    if (!numberp(e)) {
      error("Key data expected ", e);
      /* NOTREACHED */
    }
    if (null(cdr(form))) {
      error("Illegal form ",sp[0]);
      /* NOTREACHED */
    }
    if (numberp(car(cdr(form)))) {
      form = cdr(form);
    }
    for (i = 0, form = cdr(form) ; consp(form) ; i++, form = cdr(form)) {
      e = car(form);
      if (!stringp(e)) {
	break;
      }
    }
    if (ncand == 0) {
      ncand = i;
    }
    else if (ncand != i) {
      error("Inconsist number for each key definition ", sp[0]);
      /* NOTREACHED */
    }
  }

  group = nkeysup;

  for (form = sp[0] ; consp(form) ;) {
    if (nkeysup >= MAX_KEY_SUP) {
      error("Too many symbol definitions", sp[0]);
      /* NOTREACHED */
    }
    /* The following lines are for xkey translation rule */
    key = (wchar_t)xnum(car(form));
    if (numberp(car(cdr(form)))) {
      xkey = (wchar_t)xnum(car(cdr(form)));
      form = cdr(form);
    }
    else {
      xkey = key;
    }
    p = cand;
    for (form = cdr(form) ; consp(form) ; form = cdr(form)) {
      int len;

      e = car(form);
      if (!stringp(e)) {
	break;
      }
      len = MBstowcs(p, xstring(e), 1024 - (p - cand));
      p += len;
      *p++ = (wchar_t)0;
    }
    *p++ = (wchar_t)0;
    mcandsize = p - cand;
    mcand = (wchar_t *)malloc(mcandsize * sizeof(wchar_t));
    if (mcand == 0) {
      error("Insufficient memory", NON);
      /* NOTREACHED */
    }
    acand = (wchar_t **)calloc(ncand + 1, sizeof(wchar_t *));
    if (acand == 0) {
      free(mcand);
      error("Insufficient memory", NON);
      /* NOTREACHED */
    }

    for (i = 0 ; i < p - cand ; i++) {
      mcand[i] = cand[i];
    }
    for (i = 0, p = mcand ; i < ncand ; i++) {
      acand[i] = p;
      while (*p++)
	/* EMPTY */
	;
    }
    acand[i] = 0;
    /* 実際に格納する */
    keysup[nkeysup].key = key;
    keysup[nkeysup].xkey = xkey;
    keysup[nkeysup].groupid = group;
    keysup[nkeysup].ncand = ncand;
    keysup[nkeysup].cand = acand;
    keysup[nkeysup].fullword = mcand;
    nkeysup++;
  }
  res = car(pop1());
  return (res);
}

#ifndef NO_EXTEND_MENU

/* 
   defselection で一覧の文字を取り出すために必要なので、以下を定義する
 */

#define SS2	((char)0x8e)
#define SS3	((char)0x8f)

#define G0	0
#define G1	1
#define G2	2
#define G3	3

static int cswidth[4] = {1, 2, 2, 3};


/*
   getKutenCode -- 文字の区点コードを取り出す
 */

static int
getKutenCode(data, ku, ten)
char *data;
int *ku, *ten;
{
  int codeset;

  *ku = (data[0] & 0x7f) - 0x20;
  *ten = (data[1] & 0x7f) - 0x20;
  if (*data == SS2) {
    codeset = G2;
    *ku = 0;
  }
  else if (*data == SS3) {
    codeset = G3;
    *ku = *ten;
    *ten = (data[2] & 0x7f) - 0x20;
  }
  else if (*data & 0x80) {
    codeset = G1;
  }
  else {
    codeset = G0;
    *ten = *ku;
    *ku = 0;
  }
  return codeset;
}
    
/* 
   howManuCharsAre -- defselection で範囲指定した場合に
                      その範囲内の図形文字の個数を返す
 */

static int
howManyCharsAre(tdata, edata, tku, tten, codeset)
char *tdata, *edata;
int *tku, *tten, *codeset;
{
  int eku, eten, kosdata, koedata;

  kosdata = getKutenCode(tdata, tku, tten);
  koedata = getKutenCode(edata, &eku, &eten);
  if (kosdata != koedata) {
    return 0;
  }
  else {
    *codeset = kosdata;
    return ((eku - *tku) * 94 + eten - *tten + 1);
  }
}


/*
   pickupChars -- 範囲内の図形文字を取り出す
 */

static char *
pickupChars(tku, tten, num, kodata)
int tku, tten, num, kodata;
{
  char *dptr, *tdptr, *edptr;

  dptr = (char *)malloc(num * cswidth[kodata] + 1);
  if (dptr) {
    tdptr = dptr;
    edptr = dptr + num * cswidth[kodata];
    for (; dptr < edptr ; tten++) {
      if (tten > 94) {
        tku++;
        tten = 1;
      }
      switch(kodata) {
        case G0:
          *dptr++ = (tten + 0x20);
          break;
        case G1:
          *dptr++ = (tku + 0x20) | 0x80;
          *dptr++ = (tten + 0x20) | 0x80;
          break;
        case G2:
          *dptr++ = SS2;
          *dptr++ = (tten + 0x20) | 0x80;
          break;
        case G3:
          *dptr++ = SS3;
          *dptr++ = (tku + 0x20) | 0x80;
          *dptr++ = (tten + 0x20) | 0x80;
          break;
        default:
          break;
      }
    }
    *dptr++ = '\0';
    return tdptr;
  }
  else {
    error("Insufficient memory", NON);
    /* NOTREACHED */
  }
}

/* 
   numtostr -- Key data から文字を取り出す
 */

static void
numtostr(num, str)
unsigned long num;
char *str;
{
  if (num & 0xff0000) {
    *str++ = (char)((num >> 16) & 0xff);
  }
  if (num & 0xff00) {
    *str++ = (char)((num >> 8) & 0xff);
  }
  *str++ = (char)(num & 0xff);
  *str = '\0';
}

/*
  defselection -- 文字一覧の定義

  《形式》
  (defselection function-symbol "モード表示" '(character-list))
 */

static list
Ldefselection()
{
  list form, sym, e, e2, md, kigo_list, buf;
  extern extraFunc *extrafuncp;
  extern int nothermodes;
  int i, len, cs, nkigo_data = 0, kigolen = 0;
  wchar_t *p, *kigo_str, **akigo_data;
  extraFunc *extrafunc = (extraFunc *)0;

  form = sp[0];

  if (atom(form) || atom(cdr(form)) || atom(cdr(cdr(form)))) {
    error("Illegal form ",form);
    /* NOTREACHED */
  }

  sym = car(form);
  if (!symbolp(sym)) {
    error("Symbol data expected ", sym);
    /* NOTREACHED */
  }

  md = car(cdr(form));
  if (!stringp(md) && !null(md)) {
    error("String data expected ", md);
    /* NOTREACHED */
  }
    
  push(car(cdr(cdr(form))));
  push(Leval(1));

  kigo_list = sp[0];
  if (atom(kigo_list)) {
    error("Illegal form ", kigo_list);
    /* NOTREACHED */
  }

  /* まず領域を確保する */
  buf = kigo_list;
  while (!atom(buf)) {
    if (!atom(cdr(buf)) && (car(cdr(buf)) == HYPHEN)) {
      /* 範囲指定のとき */
      if (atom(cdr(cdr(buf)))) {
        error("Illegal form ", buf);
        /* NOTREACHED */
      }
      else {
        int sku, sten, num;
        char ss[4], ee[4];

        e = car(buf);
        if (!numberp(e)) {
          error("Key data expected ", e);
          /* NOTREACHED */
        }
        e2 = car(cdr(cdr(buf)));
        if (!numberp(e2)) {
          error("Key data expected ", e2);
          /* NOTREACHED */
        }

        numtostr(xnum(e), ss);
        numtostr(xnum(e2), ee);
        num = howManyCharsAre(ss, ee, &sku, &sten, &cs);
        if (num <= 0) {
          error("Inconsistent range of charcter code ", buf);
          /* NOTREACHED */
        }
        kigolen = kigolen + (cswidth[cs] + 1) * num;
        nkigo_data += num;
      }
      buf = cdr(cdr(cdr(buf)));
    }
    else {
     /* 要素指定のとき */
      char xx[4], *xxp;

      e = car(buf);
      if (!numberp(e) && !stringp(e)) {
        error("Key or string data expected ", e);
        /* NOTREACHED */
      }
      else if (numberp(e)) {
        numtostr(xnum(e), xx);
        xxp = xx;
      }
      else {
        xxp = xstring(e);
      }

      for ( ; *xxp ; xxp += cswidth[cs] ) {
        if (*xxp == SS2) {
          cs = G2;
        }
        else if (*xxp == SS3) {
          cs = G3;
        }
        else if (*xxp & 0x80) {
          cs = G1;
        }
        else {
          cs = G0;
        }
        kigolen = kigolen + cswidth[cs];
      }
      kigolen += 1;  /* 各要素の最後に \0 を入れる */
      nkigo_data++;
      buf = cdr(buf);
    }
  }

  kigo_str = (wchar_t *)malloc(kigolen * sizeof(wchar_t));
  if (!kigo_str) {
    error("Insufficient memory ", NON);
    /* NOTREACHED */
  }
  p = kigo_str;

  /* 一覧を取り出す */
  while (!atom(kigo_list)) {
    if (!atom(cdr(kigo_list)) && (car(cdr(kigo_list)) == HYPHEN)) {
    /* 範囲指定のとき */
      int sku, sten, codeset, num;
      char *ww, *sww, *eww, ss[4], ee[4], bak;

      e = car(kigo_list);
      e2 = car(cdr(cdr(kigo_list)));
      numtostr(xnum(e), ss);
      numtostr(xnum(e2), ee);
      num = howManyCharsAre(ss, ee, &sku, &sten, &codeset);
      sww = ww = pickupChars(sku, sten, num, codeset);
      cs = cswidth[codeset];
      eww = ww + num * cs;
      while (ww < eww) {
        bak = ww[cs];
        ww[cs] = '\0';
        len = MBstowcs(p, ww, kigolen - (p - kigo_str));
        p += len;
        *p++ = (wchar_t)0;
        ww += cs;
        ww[0] = bak;
      }
      free(sww);
      kigo_list = cdr(cdr(cdr(kigo_list)));
    }
    else {
      /* 要素指定のとき */
      char xx[4], *xxp;

      e = car(kigo_list);
      if (numberp(e)) {
        numtostr(xnum(e), xx);
        xxp = xx;
      }
      else {
        xxp = xstring(e);
      }
      len = MBstowcs(p, xxp, kigolen - (p - kigo_str));
      p += len;
      *p++ = (wchar_t)0;
      kigo_list = cdr(kigo_list);
    }
  }

  akigo_data = (wchar_t **)calloc(nkigo_data + 1, sizeof(wchar_t *));
  if (akigo_data == 0) {
    free(kigo_str);
    error("Insufficient memory", NON);
    /* NOTREACHED */
  }

  for (i = 0, p = kigo_str ; i < nkigo_data ; i++) {
    akigo_data[i] = p;
    while (*p++)
      /* EMPTY */
      ;
  }

  /* 領域を確保する */
  extrafunc = (extraFunc *)malloc(sizeof(extraFunc));
  if (!extrafunc) {
    free((char *)kigo_str);
    free((char *)akigo_data);
    error("Insufficient memory", NON);
    /* NOTREACHED */
  }
  extrafunc->u.kigoptr = (kigoIchiran *)malloc(sizeof(kigoIchiran));
  if (!extrafunc->u.kigoptr) {
    free((char *)kigo_str);
    free((char *)akigo_data);
    free((char *)extrafunc);
    error("Insufficient memory", NON);
    /* NOTREACHED */
  }

  /* シンボルの関数値としての定義 */
  symbolpointer(sym)->mid = extrafunc->u.kigoptr->kigo_mode
                          = CANNA_MODE_MAX_IMAGINARY_MODE + nothermodes;
  symbolpointer(sym)->fid = extrafunc->fnum
                          = CANNA_FN_MAX_FUNC + nothermodes;

  /* 実際に格納する */
  extrafunc->u.kigoptr->kigo_data = akigo_data;
  extrafunc->u.kigoptr->kigo_str = kigo_str;
  extrafunc->u.kigoptr->kigo_size = nkigo_data;
  if (stringp(md)) {
    extrafunc->display_name = WString(xstring(md));
  }
  else {
    extrafunc->display_name = (wchar_t *)0;
  }

  extrafunc->keyword = EXTRA_FUNC_DEFSELECTION;
  extrafunc->next = extrafuncp;
  extrafuncp = extrafunc;
  pop(2);
  nothermodes++;
  return sym;
}

/*
  defmenu -- メニューの定義

  《形式》
  (defmenu first-menu
    ("登録" touroku)
    ("サーバ操作" server))
 */

static list
Ldefmenu()
{
  list form, sym, e;
  extern extraFunc *extrafuncp;
  extern int nothermodes;
  extraFunc *extrafunc = (extraFunc *)0;
  int i, n, clen, len;
  wchar_t foo[512];
  menustruct *men;
  menuitem *menubody;
  wchar_t *wp, **wpp;
  extern menustruct *allocMenu();

  form = sp[0];
  if (atom(form) || atom(cdr(form))) {
    error("Bad form ", form);
    /* NOTREACHED */
  }
  sym = car(form);
  if (!symbolp(sym)) {
    error("Symbol data expected ", sym);
    /* NOTREACHED */
  }

  /* 引数を数える。ついでに表示文字列の文字数を数える */
  for (n = 0, clen = 0, e = cdr(form) ; !atom(e) ; n++, e = cdr(e)) {
    list l = car(e), d, fn;
    if (atom(l) || atom(cdr(l))) {
      error("Bad form ", form);
    }
    d = car(l);
    fn = car(cdr(l));
    if (!stringp(d) || !symbolp(fn)) {
      error("Bad form ", form);
    }
    len = MBstowcs(foo, xstring(d), 512);
    if (len >= 0) {
      clen += len + 1;
    }
  }

  extrafunc = (extraFunc *)malloc(sizeof(extraFunc));
  if (extrafunc) {
    men = allocMenu(n, clen);
    if (men) {
      menubody = men->body;
      /* タイトル文字をデータバッファにコピー */
      for (i = 0, wp = men->titledata, wpp = men->titles, e = cdr(form) ;
	   i < n ; i++, e = cdr(e)) {
	len = MBstowcs(wp, xstring(car(car(e))), 512);
	*wpp++ = wp;
	wp += len + 1;

	menubody[i].flag = MENU_SUSPEND;
	menubody[i].u.misc = (char *)car(cdr(car(e)));
      }
      men->nentries = n;

      /* シンボルの関数値としての定義 */
      symbolpointer(sym)->mid =
	men->modeid = CANNA_MODE_MAX_IMAGINARY_MODE + nothermodes;
      symbolpointer(sym)->fid =
	extrafunc->fnum = CANNA_FN_MAX_FUNC + nothermodes;
      extrafunc->keyword = EXTRA_FUNC_DEFMENU;
      extrafunc->display_name = (wchar_t *)0;
      extrafunc->u.menuptr = men;

      extrafunc->next = extrafuncp;
      extrafuncp = extrafunc;
      nothermodes++;
      (void)pop1();
      return sym;
    }
    free((char *)extrafunc);
  }
  error("Insufficient memory", NON);
  /* NOTREACHED */
}
#endif /* NO_EXTEND_MENU */

static list
Lsetinifunc(n)
int n;
{
  unsigned char fseq[256];
  int i, len;
  list ret = NIL;
  extern BYTE *initfunc;

  argnchk(S_SetInitFunc, 1);

  len = xfseq(S_SetInitFunc, sp[0], fseq, 256);

  if (len > 0) {
    if (initfunc) free(initfunc);
    initfunc = (BYTE *)malloc(len + 1);
    if (!initfunc) {
      error("Insufficient memory", NON);
      /* NOTREACHED */
    }
    for (i = 0 ; i < len ; i++) {
      initfunc[i] = fseq[i];
    }
    initfunc[i] = 0;
    ret = T;
  }
  (void)pop1();
  return ret;
}

static list
Lboundp(n)
int n;
{
  list e;
  struct atomcell *sym;

  argnchk("boundp",1);
  e = pop1();

  if (!atom(e)) {
    error("boundp: bad arg ", e);
    /* NOTREACHED */
  }
  else if (constp(e)) {
    error("boundp: bad arg ", e);
    /* NOTREACHED */
  }

  if (assq(e, *esp)) {
    return T;
  }
  else if ((sym = symbolpointer(e))->valfunc) {
    return T;
  }
  else {
    if (sym->value != (list)UNBOUND) {
      return T;
    }
    else {
      return NIL;
    }
  }
}

static list
Lfboundp(n)
int n;
{
  list e;

  argnchk("fboundp",1);
  e = pop1();

  if (!atom(e)) {
    error("fboundp: bad arg ", e);
    /* NOTREACHED */
  }
  else if (constp(e)) {
    error("fboundp: bad arg ", e);
    /* NOTREACHED */
  }
  if (symbolpointer(e)->ftype == UNDEF) {
    return NIL;
  }
  else {
    return T;
  }
}

static list
Lgetenv(n)
int n;
{
  list e;
  char strbuf[256], *ret, *getenv();
  list retval;

  argnchk("getenv",1);
  e = sp[0];

  if (!stringp(e)) {
    error("getenv: bad arg ", e);
    /* NOTREACHED */
  }

  strncpy(strbuf, xstring(e), xstrlen(e));
  strbuf[xstrlen(e)] = '\0';
  ret = getenv(strbuf);
  if (ret) {
    retval = copystring(ret, strlen(ret));
  }
  else {
    retval = NIL;
  }
  (void)pop1();
  return retval;
}

static list
LdefEscSeq(n)
int n;
{
  extern void (*keyconvCallback)();

  argnchk("define-esc-sequence",3);

  if (!stringp(sp[2])) {
    error("define-esc-sequence: bad arg ", sp[2]);
    /* NOTREACHED */
  }
  if (!stringp(sp[1])) {
    error("define-esc-sequence: bad arg ", sp[1]);
    /* NOTREACHED */
  }
  if (!numberp(sp[0])) {
    error("define-esc-sequence: bad arg ", sp[0]);
    /* NOTREACHED */
  }
  if (keyconvCallback) {
    (*keyconvCallback)(CANNA_CTERMINAL, 
		       xstring(sp[2]), xstring(sp[1]), xnum(sp[0]));
  }
  pop(3);
  return NIL;
}

static list
Lconcat(n)
int n;
{
  list t, res;
  int  i, len;
  char *p;

  /* まず長さを数える。 */
  for (len= 0, i = n ; i-- ;) {
    t = sp[i];
    if (!stringp(t)) {
      lisp_strerr("concat", t);
      /* NOTREACHED */
    }
    len += xstrlen(t);
  }
  res = allocstring(len);
  for (p = xstring(res), i = n ; i-- ;) {
    t = sp[i];
    len = xstrlen(t);
    Strncpy(p, xstring(t), len);
    p += len;
  }
  *p = '\0';
  pop(n);
  return res;
}

/* lispfuncend */

extern char *RkGetServerHost();

static void
ObtainVersion()
{
#if !defined(STANDALONE) && !defined(WIN_CANLISP)
  int a, b;
  char *serv;
  extern int protocol_version, server_version;
  extern char *server_name;

  serv = RkGetServerHost();
  if (!serv) {
    serv = DICHOME;
  }
  RkwInitialize(serv);

  /* プロトコルバージョン */
  RkwGetProtocolVersion(&a, &b);
  protocol_version = a * 1000 + b;

  /* サーババージョン */
  RkwGetServerVersion(&a, &b);
  server_version = a * 1000 + b;

  /* サーバ名 */
  if (server_name)
    free(server_name);
  server_name = malloc(strlen(DEFAULT_CANNA_SERVER_NAME) + 1);
  if (server_name) {
    strcpy(server_name, DEFAULT_CANNA_SERVER_NAME);
  }

  RkwFinalize();
#endif /* STANDALONE */
}

/* 変数アクセスのための関数 */

static list
VTorNIL(var, setp, arg)
BYTE *var;
int setp;
list arg;
{
  if (setp == VALSET) {
    *var = (arg == NIL) ? 0 : 1;
    return arg;
  }
  else { /* get */
    return *var ? T : NIL;
  }
}

static list
StrAcc(var, setp, arg)
char **var;
int setp;
list arg;
{
  if (setp == VALSET) {
    if (null(arg) || stringp(arg)) {
      if (*var) {
	free(*var);
      }
      if (stringp(arg)) {
	*var = malloc(strlen(xstring(arg)) + 1);
	if (*var) {
	  strcpy(*var, xstring(arg));
	  return arg;
	}
	else {
	  error("Insufficient memory.", NON);
	  /* NOTREACHED */
	}
      }
      else {
	*var = (char *)0;
	return NIL;
      }
    }
    else {
      lisp_strerr((char *)0, arg);
      /* NOTREACHED */
    }
  }
  /* else { .. */
  if (*var) {
    return copystring(*var, strlen(*var));
  }
  else {
    return NIL;
  }
  /* end else .. } */
}

static list
NumAcc(var, setp, arg)
int *var;
int setp;
list arg;
{
  if (setp == VALSET) {
    if (numberp(arg)) {
      *var = (int)xnum(arg);
      return arg;
    }
    else {
      numerr((char *)0, arg);
      /* NOTREACHED */
    }
  }
  return (list)mknum(*var);
}

/* ここから下がカスタマイズの追加等で良くいじる部分 */

/* 実際のアクセス関数 */

#define DEFVAR(fn, acc, ty, var) \
static list fn(setp, arg) int setp; list arg; { \
  extern ty var; return acc(&var, setp, arg); }

#define DEFVAREX(fn, acc, var) \
static list fn(setp, arg) int setp; list arg; { \
  extern struct CannaConfig cannaconf; return acc(&var, setp, arg); }

static list Vnkouhobunsetsu(setp, arg) int setp; list arg;
{
  extern int nKouhoBunsetsu;

  arg = NumAcc(&nKouhoBunsetsu, setp, arg);
#ifdef RESTRICT_NKOUHOBUNSETSU
  if (nKouhoBunsetsu < 3 || nKouhoBunsetsu > 60)
    nKouhoBunsetsu = 16;
#else
  if (nKouhoBunsetsu < 0) {
    nKouhoBunsetsu = 0;
  }
#endif
  return arg;
}

static list VProtoVer(setp, arg) int setp; list arg;
{
#ifndef STANDALONE
  extern protocol_version;

  if (protocol_version < 0) {
    ObtainVersion();
  }
  return NumAcc(&protocol_version, setp, arg);
#endif /* STANDALONE */
}

static list VServVer(setp, arg) int setp; list arg;
{
#ifndef STANDALONE
  extern server_version;

  if (server_version < 0) {
    ObtainVersion();
  }
  return NumAcc(&server_version, setp, arg);
#endif /* STANDALONE */
}

static list VServName(setp, arg) int setp; list arg;
{
#ifndef STANDALONE
  extern char *server_name;

  if (!server_name) {
    ObtainVersion();
  }
  return StrAcc(&server_name, setp, arg);
#endif /* STANDALONE */
}

static list
VCannaDir(setp, arg) int setp; list arg;
{
  char *canna_dir = CANNALIBDIR;

  if (setp == VALGET) {
    return StrAcc(&canna_dir, setp, arg);
  }
  else {
    return NIL;
  }
}

static list VCodeInput(setp, arg) int setp; list arg;
{
  extern struct CannaConfig cannaconf;
  static char *input_code[CANNA_MAX_CODE] = {"jis", "sjis", "kuten"};

  if (setp == VALSET) {
    if (null(arg) || stringp(arg)) {
      if (stringp(arg)) {
	int i;
	char *s = xstring(arg);

	for (i = 0 ; i < CANNA_MAX_CODE ; i++) {
	  if (!strcmp(s, input_code[i])) {
	    cannaconf.code_input = i;
	    break;
	  }
	}
	if (i < CANNA_MAX_CODE) {
	  return arg;
	}
	else {
	  return NIL;
	}
      }
      else {
	cannaconf.code_input = 0; /* use default */
	return copystring(input_code[0], strlen(input_code[0]));
      }
    }
    else {
      lisp_strerr((char *)0, arg);
      /* NOTREACHED */
    }
  }
  /* else { .. */
  if (/* 0 <= cannaconf.code_input && /* unsigned にしたので冗長になった */
      cannaconf.code_input <= CANNA_CODE_KUTEN) {
    return copystring(input_code[cannaconf.code_input],
		      strlen(input_code[cannaconf.code_input]));
  }
  else {
    return NIL;
  }
  /* end else .. } */
}


DEFVAR(Vromkana         ,StrAcc  ,char * ,RomkanaTable)
DEFVAR(Venglish         ,StrAcc  ,char * ,EnglishTable)

DEFVAREX(Vnhenkan       ,NumAcc          ,cannaconf.kouho_threshold)
DEFVAREX(Vndisconnect   ,NumAcc          ,cannaconf.strokelimit)
DEFVAREX(VCannaVersion  ,NumAcc          ,cannaconf.CannaVersion)
DEFVAREX(VIndexSeparator,NumAcc          ,cannaconf.indexSeparator)

DEFVAREX(Vgakushu       ,VTorNIL         ,cannaconf.Gakushu)
DEFVAREX(Vcursorw       ,VTorNIL         ,cannaconf.CursorWrap)
DEFVAREX(Vselectd       ,VTorNIL         ,cannaconf.SelectDirect)
DEFVAREX(Vnumeric       ,VTorNIL         ,cannaconf.HexkeySelect)
DEFVAREX(Vbunsets       ,VTorNIL         ,cannaconf.BunsetsuKugiri)
DEFVAREX(Vcharact       ,VTorNIL         ,cannaconf.ChBasedMove)
DEFVAREX(Vreverse       ,VTorNIL         ,cannaconf.ReverseWidely)
DEFVAREX(VreverseWord   ,VTorNIL         ,cannaconf.ReverseWord)
DEFVAREX(Vquitich       ,VTorNIL         ,cannaconf.QuitIchiranIfEnd)
DEFVAREX(Vkakutei       ,VTorNIL         ,cannaconf.kakuteiIfEndOfBunsetsu)
DEFVAREX(Vstayaft       ,VTorNIL         ,cannaconf.stayAfterValidate)
DEFVAREX(Vbreakin       ,VTorNIL         ,cannaconf.BreakIntoRoman)
DEFVAREX(Vgrammati      ,VTorNIL         ,cannaconf.grammaticalQuestion)
DEFVAREX(Vforceka       ,VTorNIL         ,cannaconf.forceKana)
DEFVAREX(Vkouhoco       ,VTorNIL         ,cannaconf.kCount)
DEFVAREX(Vauto          ,VTorNIL         ,cannaconf.chikuji)
DEFVAREX(VlearnNumTy    ,VTorNIL         ,cannaconf.LearnNumericalType)
DEFVAREX(VBSasQuit      ,VTorNIL         ,cannaconf.BackspaceBehavesAsQuit)
DEFVAREX(Vinhibi        ,VTorNIL         ,cannaconf.iListCB)
DEFVAREX(Vkeepcupos     ,VTorNIL         ,cannaconf.keepCursorPosition)
DEFVAREX(VAbandon       ,VTorNIL         ,cannaconf.abandonIllegalPhono)
DEFVAREX(VHexStyle      ,VTorNIL         ,cannaconf.hexCharacterDefiningStyle)
DEFVAREX(VKojin         ,VTorNIL         ,cannaconf.kojin)
DEFVAREX(VIndexHankaku  ,VTorNIL         ,cannaconf.indexHankaku)
DEFVAREX(VAllowNext     ,VTorNIL         ,cannaconf.allowNextInput)
DEFVAREX(VkanaGaku      ,VTorNIL         ,cannaconf.doKatakanaGakushu)
DEFVAREX(VhiraGaku      ,VTorNIL         ,cannaconf.doHiraganaGakushu)
DEFVAREX(VChikujiContinue ,VTorNIL       ,cannaconf.ChikujiContinue)
DEFVAREX(VRenbunContinue  ,VTorNIL       ,cannaconf.RenbunContinue)
DEFVAREX(VMojishuContinue ,VTorNIL       ,cannaconf.MojishuContinue)
DEFVAREX(VcRealBS       ,VTorNIL         ,cannaconf.chikujiRealBackspace)
DEFVAREX(VIgnoreCase    ,VTorNIL         ,cannaconf.ignore_case)
DEFVAREX(VRomajiYuusen  ,VTorNIL         ,cannaconf.romaji_yuusen)
DEFVAREX(VAutoSync      ,VTorNIL         ,cannaconf.auto_sync)
DEFVAREX(VQuicklyEscape ,VTorNIL         ,cannaconf.quickly_escape)
DEFVAREX(VInhibitHankana,VTorNIL         ,cannaconf.InhibitHankakuKana)
DEFVAREX(VDelayConnect  ,VTorNIL         ,cannaconf.DelayConnect)

#ifdef DEFINE_SOMETHING
DEFVAR(Vchikuji_debug, VTorNIL, int, chikuji_debug)
#endif

/* Lisp の関数と C の関数の対応表 */

static struct atomdefs initatom[] = {
  {"quote"		,SPECIAL,Lquote		},
  {"setq"		,SPECIAL,Lsetq		},
  {"set"		,SUBR	,Lset		},
  {"equal"		,SUBR	,Lequal		},
  {"="			,SUBR	,Lequal		},
  {">"			,SUBR	,Lgreaterp	},
  {"<"			,SUBR	,Llessp		},
  {"progn"		,SPECIAL,Lprogn		},
  {"eq"			,SUBR	,Leq   		},
  {"cond"		,SPECIAL,Lcond		},
  {"null"		,SUBR	,Lnull		},
  {"not"		,SUBR	,Lnull		},
  {"and"		,SPECIAL,Land		},
  {"or"			,SPECIAL,Lor		},
  {"+"			,SUBR	,Lplus		},
  {"-"			,SUBR	,Ldiff		},
  {"*"			,SUBR	,Ltimes		},
  {"/"			,SUBR	,Lquo		},
  {"%"			,SUBR	,Lrem		},
  {"gc"			,SUBR	,Lgc		},
  {"load"		,SUBR	,Lload		},
  {"list"		,SUBR	,Llist		},
  {"sequence"		,SUBR	,Llist		},
  {"defun"		,SPECIAL,Ldefun		},
  {"defmacro"		,SPECIAL,Ldefmacro	},
  {"cons"		,SUBR	,Lcons		},
  {"car"		,SUBR	,Lcar		},
  {"cdr"		,SUBR	,Lcdr		},
  {"atom"		,SUBR	,Latom		},
  {"let"		,CMACRO	,Llet		},
  {"if"			,CMACRO	,Lif		},
  {"boundp"		,SUBR	,Lboundp	},
  {"fboundp"		,SUBR	,Lfboundp	},
  {"getenv"		,SUBR	,Lgetenv	},
  {"copy-symbol"	,SUBR	,Lcopysym	},
  {"concat"		,SUBR	,Lconcat	},
  {S_FN_UseDictionary	,SUBR	,Lusedic	},
  {S_SetModeDisp	,SUBR	,Lmodestr	},
  {S_SetKey		,SUBR	,Lsetkey	},
  {S_GSetKey		,SUBR	,Lgsetkey	},
  {S_UnbindKey		,SUBR	,Lunbindkey	},
  {S_GUnbindKey		,SUBR	,Lgunbindkey	},
  {S_DefMode		,SPECIAL,Ldefmode	},
  {S_DefSymbol		,SPECIAL,Ldefsym	},
#ifndef NO_EXTEND_MENU
  {S_DefSelection	,SPECIAL,Ldefselection	},
  {S_DefMenu		,SPECIAL,Ldefmenu	},
#endif
  {S_SetInitFunc	,SUBR	,Lsetinifunc	},
  {S_defEscSequence	,SUBR	,LdefEscSeq	},
  {0			,UNDEF	,0		}, /* DUMMY */
};

static void
deflispfunc()
{
  struct atomdefs *p;

  for (p = initatom ; p->symname ; p++) {
    struct atomcell *atomp;
    list temp;

    temp = getatmz(p->symname);
    atomp = symbolpointer(temp);
    atomp->ftype = p->symtype;
    if (atomp->ftype != UNDEF) {
      atomp->func = p->symfunc;
    }
  }
}


/* 変数表 */

static struct cannavardefs cannavars[] = {
  {S_VA_RomkanaTable		,Vromkana},
  {S_VA_EnglishTable		,Venglish},
  {S_VA_CursorWrap		,Vcursorw},
  {S_VA_SelectDirect		,Vselectd},
  {S_VA_NumericalKeySelect	,Vnumeric},
  {S_VA_BunsetsuKugiri		,Vbunsets},
  {S_VA_CharacterBasedMove	,Vcharact},
  {S_VA_ReverseWidely		,Vreverse},
  {S_VA_ReverseWord		,VreverseWord},
  {S_VA_Gakushu			,Vgakushu},
  {S_VA_QuitIfEOIchiran		,Vquitich},
  {S_VA_KakuteiIfEOBunsetsu	,Vkakutei},
  {S_VA_StayAfterValidate	,Vstayaft},
  {S_VA_BreakIntoRoman		,Vbreakin},
  {S_VA_NHenkanForIchiran	,Vnhenkan},
  {S_VA_GrammaticalQuestion	,Vgrammati},
  {"gramatical-question"	,Vgrammati}, /* 以前のスペルミスの救済 */
  {S_VA_ForceKana		,Vforceka},
  {S_VA_KouhoCount		,Vkouhoco},
  {S_VA_Auto			,Vauto},
  {S_VA_LearnNumericalType	,VlearnNumTy},
  {S_VA_BackspaceBehavesAsQuit	,VBSasQuit},
  {S_VA_InhibitListCallback	,Vinhibi},
  {S_VA_nKouhoBunsetsu		,Vnkouhobunsetsu},
  {S_VA_keepCursorPosition	,Vkeepcupos},
  {S_VA_CannaVersion		,VCannaVersion},
  {S_VA_Abandon			,VAbandon},
  {S_VA_HexDirect		,VHexStyle},
  {S_VA_ProtocolVersion		,VProtoVer},
  {S_VA_ServerVersion		,VServVer},
  {S_VA_ServerName		,VServName},
  {S_VA_CannaDir		,VCannaDir},
  {S_VA_Kojin			,VKojin},
  {S_VA_IndexHankaku	       	,VIndexHankaku},
  {S_VA_IndexSeparator	       	,VIndexSeparator},
  {S_VA_AllowNextInput		,VAllowNext},
  {S_VA_doKatakanaGakushu	,VkanaGaku},
  {S_VA_doHiraganaGakushu	,VhiraGaku},
#ifdef	DEFINE_SOMETHING
  {S_VA_chikuji_debug		,Vchikuji_debug},
#endif	/* DEFINE_SOMETHING */
  {S_VA_ChikujiContinue		,VChikujiContinue},
  {S_VA_RenbunContinue		,VRenbunContinue},
  {S_VA_MojishuContinue		,VMojishuContinue},
  {S_VA_ChikujiRealBackspace	,VcRealBS},
  {S_VA_nDisconnectServer	,Vndisconnect},
  {S_VA_ignoreCase		,VIgnoreCase},
  {S_VA_RomajiYuusen		,VRomajiYuusen},
  {S_VA_AutoSync		,VAutoSync},
  {S_VA_QuicklyEscape		,VQuicklyEscape},
  {S_VA_InhibitHanKana		,VInhibitHankana},
  {S_VA_CodeInput		,VCodeInput},
  {S_VA_DelayConnect		,VDelayConnect},
  {0				,0},
};

static void
defcannavar()
{
  struct cannavardefs *p;

  for (p = cannavars ; p->varname ; p++) {
    symbolpointer(getatmz(p->varname))->valfunc = p->varfunc;
  }
}



/* モード表 */

static struct cannamodedefs cannamodes[] = {
  {S_AlphaMode			,CANNA_MODE_AlphaMode},
  {S_YomiganaiMode		,CANNA_MODE_EmptyMode},
  {S_YomiMode			,CANNA_MODE_YomiMode},
  {S_MojishuMode		,CANNA_MODE_JishuMode},
  {S_TankouhoMode		,CANNA_MODE_TankouhoMode},
  {S_IchiranMode		,CANNA_MODE_IchiranMode},
  {S_KigouMode			,CANNA_MODE_KigoMode},
  {S_YesNoMode			,CANNA_MODE_YesNoMode},
  {S_OnOffMode			,CANNA_MODE_OnOffMode},
  {S_ShinshukuMode		,CANNA_MODE_AdjustBunsetsuMode},

  {S_AutoYomiMode		,CANNA_MODE_ChikujiYomiMode},
  {S_AutoBunsetsuMode		,CANNA_MODE_ChikujiTanMode},

  {S_HenkanNyuuryokuMode	,CANNA_MODE_HenkanNyuryokuMode},
  {S_HexMode			,CANNA_MODE_HexMode},
  {S_BushuMode			,CANNA_MODE_BushuMode},
  {S_ExtendMode			,CANNA_MODE_ExtendMode},
  {S_RussianMode		,CANNA_MODE_RussianMode},
  {S_GreekMode			,CANNA_MODE_GreekMode},
  {S_LineMode			,CANNA_MODE_LineMode},
  {S_ChangingServerMode		,CANNA_MODE_ChangingServerMode},
  {S_HenkanMethodMode		,CANNA_MODE_HenkanMethodMode},
  {S_DeleteDicMode		,CANNA_MODE_DeleteDicMode},
  {S_TourokuMode		,CANNA_MODE_TourokuMode},
  {S_TourokuHinshiMode		,CANNA_MODE_TourokuHinshiMode},
  {S_TourokuDicMode		,CANNA_MODE_TourokuDicMode},
  {S_QuotedInsertMode		,CANNA_MODE_QuotedInsertMode},
  {S_BubunMuhenkanMode		,CANNA_MODE_BubunMuhenkanMode},
  {S_MountDicMode		,CANNA_MODE_MountDicMode},
  {S_ZenHiraHenkanMode		,CANNA_MODE_ZenHiraHenkanMode},
  {S_HanHiraHenkanMode		,CANNA_MODE_HanHiraHenkanMode},
  {S_ZenKataHenkanMode		,CANNA_MODE_ZenKataHenkanMode},
  {S_HanKataHenkanMode		,CANNA_MODE_HanKataHenkanMode},
  {S_ZenAlphaHenkanMode		,CANNA_MODE_ZenAlphaHenkanMode},
  {S_HanAlphaHenkanMode		,CANNA_MODE_HanAlphaHenkanMode},
  {S_ZenHiraKakuteiMode		,CANNA_MODE_ZenHiraKakuteiMode},
  {S_HanHiraKakuteiMode		,CANNA_MODE_HanHiraKakuteiMode},
  {S_ZenKataKakuteiMode		,CANNA_MODE_ZenKataKakuteiMode},
  {S_HanKataKakuteiMode		,CANNA_MODE_HanKataKakuteiMode},
  {S_ZenAlphaKakuteiMode	,CANNA_MODE_ZenAlphaKakuteiMode},
  {S_HanAlphaKakuteiMode	,CANNA_MODE_HanAlphaKakuteiMode},
  {0				,0},
};

static void
defcannamode()
{
  struct cannamodedefs *p;

  for (p = cannamodes ; p->mdname ; p++) {
    symbolpointer(getatmz(p->mdname))->mid = p->mdid;
  }
}



/* 機能表 */

static struct cannafndefs cannafns[] = {
  {S_FN_Undefined		,CANNA_FN_Undefined},
  {S_FN_SelfInsert		,CANNA_FN_FunctionalInsert},
  {S_FN_QuotedInsert		,CANNA_FN_QuotedInsert},
  {S_FN_JapaneseMode		,CANNA_FN_JapaneseMode},
  {S_AlphaMode			,CANNA_FN_AlphaMode},
  {S_HenkanNyuuryokuMode	,CANNA_FN_HenkanNyuryokuMode},
  {S_HexMode			,CANNA_FN_HexMode},
  {S_BushuMode			,CANNA_FN_BushuMode},
  {S_KigouMode			,CANNA_FN_KigouMode},
  {S_FN_Forward			,CANNA_FN_Forward},
  {S_FN_Backward		,CANNA_FN_Backward},
  {S_FN_Next			,CANNA_FN_Next},
  {S_FN_Prev			,CANNA_FN_Prev},
  {S_FN_BeginningOfLine		,CANNA_FN_BeginningOfLine},
  {S_FN_EndOfLine		,CANNA_FN_EndOfLine},
  {S_FN_DeleteNext		,CANNA_FN_DeleteNext},
  {S_FN_DeletePrevious		,CANNA_FN_DeletePrevious},
  {S_FN_KillToEndOfLine		,CANNA_FN_KillToEndOfLine},
  {S_FN_Henkan			,CANNA_FN_Henkan},
  {S_FN_HenkanNaive		,CANNA_FN_HenkanOrInsert}, /* for compati */
  {S_FN_HenkanOrSelfInsert	,CANNA_FN_HenkanOrInsert},
  {S_FN_HenkanOrDoNothing	,CANNA_FN_HenkanOrNothing},
  {S_FN_Kakutei			,CANNA_FN_Kakutei},
  {S_FN_Extend			,CANNA_FN_Extend},
  {S_FN_Shrink			,CANNA_FN_Shrink},
  {S_ShinshukuMode		,CANNA_FN_AdjustBunsetsu},
  {S_FN_Quit			,CANNA_FN_Quit},
  {S_ExtendMode			,CANNA_FN_ExtendMode},
  {S_FN_Touroku			,CANNA_FN_Touroku},
  {S_FN_ConvertAsHex		,CANNA_FN_ConvertAsHex},
  {S_FN_ConvertAsBushu		,CANNA_FN_ConvertAsBushu},
  {S_FN_KouhoIchiran		,CANNA_FN_KouhoIchiran},
  {S_FN_BubunMuhenkan		,CANNA_FN_BubunMuhenkan},
  {S_FN_Zenkaku			,CANNA_FN_Zenkaku},
  {S_FN_Hankaku			,CANNA_FN_Hankaku},
  {S_FN_ToUpper			,CANNA_FN_ToUpper},
  {S_FN_Capitalize		,CANNA_FN_Capitalize},
  {S_FN_ToLower			,CANNA_FN_ToLower},
  {S_FN_Hiragana		,CANNA_FN_Hiragana},
  {S_FN_Katakana		,CANNA_FN_Katakana},
  {S_FN_Romaji			,CANNA_FN_Romaji},
  {S_FN_KanaRotate		,CANNA_FN_KanaRotate},
  {S_FN_RomajiRotate		,CANNA_FN_RomajiRotate},
  {S_FN_CaseRotate		,CANNA_FN_CaseRotate},
  {S_FN_BaseHiragana		,CANNA_FN_BaseHiragana},
  {S_FN_BaseKatakana		,CANNA_FN_BaseKatakana},
  {S_FN_BaseKana		,CANNA_FN_BaseKana},	
  {S_FN_BaseEisu		,CANNA_FN_BaseEisu},	
  {S_FN_BaseZenkaku		,CANNA_FN_BaseZenkaku},
  {S_FN_BaseHankaku		,CANNA_FN_BaseHankaku},
  {S_FN_BaseKakutei		,CANNA_FN_BaseKakutei},
  {S_FN_BaseHenkan		,CANNA_FN_BaseHenkan},
  {S_FN_BaseHiraKataToggle	,CANNA_FN_BaseHiraKataToggle},
  {S_FN_BaseZenHanToggle	,CANNA_FN_BaseZenHanToggle},
  {S_FN_BaseKanaEisuToggle	,CANNA_FN_BaseKanaEisuToggle},
  {S_FN_BaseKakuteiHenkanToggle	,CANNA_FN_BaseKakuteiHenkanToggle},
  {S_FN_BaseRotateForward	,CANNA_FN_BaseRotateForward},
  {S_FN_BaseRotateBackward	,CANNA_FN_BaseRotateBackward},
  {S_FN_Mark			,CANNA_FN_Mark},
  {S_FN_Temporary		,CANNA_FN_TemporalMode},
  {S_FN_SyncDic			,CANNA_FN_SyncDic},
  {S_RussianMode		,CANNA_FN_RussianMode},
  {S_GreekMode			,CANNA_FN_GreekMode},
  {S_LineMode			,CANNA_FN_LineMode},
  {S_FN_DefineDicMode		,CANNA_FN_DefineDicMode},
  {S_FN_DeleteDicMode		,CANNA_FN_DeleteDicMode},
  {S_FN_DicMountMode		,CANNA_FN_DicMountMode},
  {S_FN_EnterChikujiMode	,CANNA_FN_EnterChikujiMode},
  {S_FN_EnterRenbunMode		,CANNA_FN_EnterRenbunMode},
  {S_FN_DisconnectServer	,CANNA_FN_DisconnectServer},
  {S_FN_ChangeServerMode	,CANNA_FN_ChangeServerMode},
  {S_FN_ShowServer		,CANNA_FN_ShowServer},
  {S_FN_ShowGakushu		,CANNA_FN_ShowGakushu},
  {S_FN_ShowVersion		,CANNA_FN_ShowVersion},
  {S_FN_ShowPhonogramFile	,CANNA_FN_ShowPhonogramFile},
  {S_FN_ShowCannaFile		,CANNA_FN_ShowCannaFile},
  {S_FN_PageUp			,CANNA_FN_PageUp},
  {S_FN_PageDown		,CANNA_FN_PageDown},
  {S_FN_Edit			,CANNA_FN_Edit},
  {S_FN_BubunKakutei		,CANNA_FN_BubunKakutei},
  {S_FN_HenkanRegion		,CANNA_FN_HenkanRegion},
  {S_FN_PhonoEdit		,CANNA_FN_PhonoEdit},
  {S_FN_DicEdit			,CANNA_FN_DicEdit},
  {S_FN_Configure		,CANNA_FN_Configure},
  {S_FN_KanaRotate		,CANNA_FN_KanaRotate},
  {S_FN_RomajiRotate		,CANNA_FN_RomajiRotate},
  {S_FN_CaseRotate		,CANNA_FN_CaseRotate},
  {0				,0},
};

static void
defcannafunc()
{
  struct cannafndefs *p;

  for (p = cannafns ; p->fnname ; p++) {
    symbolpointer(getatmz(p->fnname))->fid = p->fnid;
  }
}


static void
defatms()
{
  deflispfunc();
  defcannavar();
  defcannamode();
  defcannafunc();
  QUOTE		= getatmz("quote");
  T		= getatmz("t");
  _LAMBDA	= getatmz("lambda");
  _MACRO	= getatmz("macro");
  COND		= getatmz("cond");
  USER		= getatmz(":user");
  BUSHU		= getatmz(":bushu");
  RENGO		= getatmz(":rengo");
  KATAKANA	= getatmz(":katakana");
  HIRAGANA	= getatmz(":hiragana");
  GRAMMAR       = getatmz(":grammar");
  HYPHEN	= getatmz("-");
  symbolpointer(T)->value = T;
}

#ifndef wchar_t
# error "wchar_t is already undefined"
#endif
#undef wchar_t
/*********************************************************************
 *                       wchar_t replace end                         *
 *********************************************************************/
/* vim: set sw=2: */
