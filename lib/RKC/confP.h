/* Copyright (c) 2003 Canna Project. All rights reserved.
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

/* $Id: confP.h,v 1.7 2003/10/05 09:27:02 aida_s Exp $ */

/* 自動判別支援コメント: これはEUC-JPだぞ。幅という字があれば大丈夫。 */

#ifndef CONF_P_H
#define CONF_P_H

/*#define CONF_DEBUG*/
/*#define CONF_LEXER_DEBUG*/
/*#define CONF_EVAL_DEBUG*/

struct tagRkcErrorBuf {
  char **buf;
  size_t bufsize;
  size_t curr;
  int nomem;
};

typedef enum {
  TOK_NONE, /* dummy */
  TOK_INVAL,
  TOK_EOF,
  TOK_NUMBER,
  TOK_CHAR,
  TOK_SEMICOLON,
  TOK_YESNO,
  TOK_OPERATOR,
  TOK_STRING = 0x100,
  TOK_WORD,
  TOK_DUMMYCODE = 0x7fff
} TokenType;

typedef enum {
  OP_NONE,
  OP_COMMA,
  OP_LPAREN,
  OP_RPAREN,
  OP_QUESTION,
  OP_COLON,

  OP_UPLUS,
  OP_UMINUS,
  OP_BPLUS,
  OP_BMINUS,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_MODULUS,
  OP_LSHIFT,
  OP_RSHIFT,
  OP_BITAND,
  OP_BITOR,
  OP_BITXOR,
  OP_BITCOMPL,

  OP_EQUAL,
  OP_NEQUAL,
  OP_LESS,
  OP_GREATER,
  OP_LEQUAL,
  OP_GEQUAL,
  OP_LAND,
  OP_LOR,
  OP_LNOT,

  OP_DUMMY
} Operator;
#define DUMMY_PRIO (int)0xdeadbeef
#define COMMA_LPRIO 11
#define COLON_LPRIO 20

typedef unsigned int (*CalcProc) pro((unsigned int, unsigned int));
typedef struct {
  int lprio;
  int rprio;
  CalcProc calc;
} OperatorRec;

typedef union {
  int chval;
  unsigned int numval;
  Operator opval;
  char *strval;
} TokenVal;

typedef struct {
  TokenType type;
  TokenVal val;
} TokenRec;

typedef struct {
  const char *curr;
  const char *rdend;
  RkcErrorBuf *errorbuf;
  unsigned int lineno;
  int eof_occured;
  int linetop;
} Lexer;

typedef struct {
  Lexer *lexer;
  RkcConfMgr *confmgr;
  RkcErrorBuf *errorbuf;
  TokenRec currtok;
  int discard;
  unsigned int exprval;
} Parser;

typedef int (*StmtProc) pro((Parser *cx));

typedef struct {
  const char *name;
  ConfItem item;
  StmtProc proc;
} StmtRec;

typedef union {
  unsigned int numval;
  int yesnoval;
  char *strval;
} ConfVal;

typedef struct {
  ConfItem item;
  ConfVal val;
} ConfRec;

typedef struct tagHostRec {
  struct tagHostRec *next;
  char *hostname;
  ConfRec *hostconf;
  size_t n_hostconf;
  size_t hostconf_size;
} HostRec;

struct tagRkcConfMgr {
  ConfRec *topconf;
  size_t n_topconf;
  size_t topconf_size;
  HostRec *hosts;
  HostRec *currhost;
  RkcErrorBuf *errors;
};

static void RkcErrorBuf_init pro((RkcErrorBuf *cx));
static void RkcErrorBuf_destroy pro((RkcErrorBuf *cx));
static void RkcErrorBuf_add pro((RkcErrorBuf *cx, const char *msg));
#define RkcErrorBuf_nomem(cx) ((cx)->nomem = 1)

#define TOKEN_STRHDR(str) ((size_t *)((str) - sizeof(size_t)))
#define TOKEN_REF(tp) do { \
  if ((tp)->type >= TOK_STRING) { \
    size_t *hdrp = TOKEN_STRHDR((tp)->val.strval); \
    assert(*hdrp > 0); \
    ++*hdrp; \
  } \
} while(0)
#define TOKEN_UNREF(tp) do { \
  if ((tp)->type >= TOK_STRING) { \
    size_t *hdrp = TOKEN_STRHDR((tp)->val.strval); \
    assert(*hdrp > 0); \
    if (--*hdrp == 0) \
      free(hdrp); \
  } \
} while(0)

#define TOKEN_INIT(utp) ((utp)->type = TOK_NONE)
#define TOKEN_DESTROY(tp) TOKEN_UNREF(tp)
#define TOKEN_COPYINIT(utp, src) do { \
  *(utp) = *(src); \
  TOKEN_REF(utp); \
} while(0)
#define TOKEN_ASSIGN(tp, src) do { \
  TOKEN_UNREF(dst); \
  *(dst) = *(src); \
  TOKEN_REF(dst); \
} while(0)
#define TOKEN_SWAP(tp1, tp2) do { \
  TokenRec tmp; \
  tmp = *(tp1); \
  *(tp1) = *(tp2); \
  *(tp2) = tmp; \
} while(0)
#define TOKEN_CLEAR(tp) do { \
  TOKEN_UNREF(tp); \
  TOKEN_INIT(tp); \
} while(0)
static int Token_assignstr pro((
      TokenRec *tp, const char *str, size_t len, int type));
#ifdef CONF_DEBUG
static void Token_dump pro((const TokenRec *tp));
#endif

static Lexer *Lexer_new pro((
      const char *srcdata, size_t srcsize, RkcErrorBuf *errorbuf));
static void Lexer_delete pro((Lexer *cx));
static int Lexer_next pro((Lexer *cx, TokenRec *resp, int prefix_op));
static void Lexer_error pro((const Lexer *cx, const char *msg));

static Parser *Parser_new pro((
      RkcConfMgr *confmgr, Lexer *lexer, RkcErrorBuf *errorbuf));
static void Parser_delete pro((Parser *cx));
static void Parser_run pro((Parser *cx));
static int Parser_next pro((Parser *cx));
static int Parser_next_postfixop pro((Parser *cx));
static void Parser_error pro((Parser *cx, const char *msg));
static void Parser_eval_error pro((Parser *cx));
static int Parser_eval pro((Parser *cx, int lprio));
static char *Parser_getstr pro((Parser *cx));

static int syn_top pro((Parser *cx));
static int syn_host pro((Parser *cx));

#define DECL_CALCPROC(x) static unsigned int x \
pro((unsigned int, unsigned int))
#define DEF_CALCPROC(x) static unsigned int \
x(arg1, arg2) \
unsigned int arg1; \
unsigned int arg2;
#define DEF_CALCPROC_OP1(x, op) DEF_CALCPROC(x) \
/* ARGUSED */ \
{ return op arg1; }
#define DEF_CALCPROC_OP2(x, op) DEF_CALCPROC(x) \
{ return arg1 op arg2; }
DECL_CALCPROC(calc_comma);
DECL_CALCPROC(calc_uplus);
DECL_CALCPROC(calc_uminus);
DECL_CALCPROC(calc_bplus);
DECL_CALCPROC(calc_bminus);
DECL_CALCPROC(calc_multiply);
DECL_CALCPROC(calc_divide);
DECL_CALCPROC(calc_modulus);
DECL_CALCPROC(calc_lshift);
DECL_CALCPROC(calc_rshift);
DECL_CALCPROC(calc_bitand);
DECL_CALCPROC(calc_bitor);
DECL_CALCPROC(calc_bitxor);
DECL_CALCPROC(calc_bitcompl);
DECL_CALCPROC(calc_equal);
DECL_CALCPROC(calc_nequal);
DECL_CALCPROC(calc_less);
DECL_CALCPROC(calc_greater);
DECL_CALCPROC(calc_lequal);
DECL_CALCPROC(calc_gequal);
DECL_CALCPROC(calc_land);
DECL_CALCPROC(calc_lor);
DECL_CALCPROC(calc_lnot);

static void RkcConfMgr_init pro((RkcConfMgr *cx, RkcErrorBuf *errors));
static void RkcConfMgr_destroy pro((RkcConfMgr *cx));
static int RkcConfMgr_openhost pro((RkcConfMgr *cx, const char *hostname));
static void RkcConfMgr_closehost pro((RkcConfMgr *cx));
static int RkcConfMgr_set_string pro((RkcConfMgr *cx,
      ConfItem item, const char *val));
static int RkcConfMgr_set_number pro((RkcConfMgr *cx,
      ConfItem item, unsigned int val));
static int RkcConfMgr_set_yesno pro((RkcConfMgr *cx,
      ConfItem item, int val));

#define CONFIG_DIR ".cannax/"
#define CONFIG_FILE "rkc.conf"
#define POSSTR CONFIG_FILE " line "
#define NOMEM_MSG "\245\341\245\342\245\352\244\254\311\324\302\255\244" \
	"\267\244\306\244\244\244\336\244\271\241\243"
	/* メモリが不足しています。 */
#define ARRAYLEN(a) (sizeof(a) / sizeof(a[0]))

#endif /* CONF_P_H */
/* vim: set sw=2: */
