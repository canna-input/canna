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

#include "sglobal.h"
#include "rkcw.h"
#include "canna/RK.h"
#include "rkc.h"
#include "RKindep/file.h"
#include "RKindep/ecfuncs.h"
#include "RKindep/strops.h"
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include "conf.h"
#include "confP.h"
#include <ctype.h>
#include <sys/stat.h>
#include <sys/wait.h>

RCSID("$Id: conf.c,v 1.12.2.1 2003/12/27 17:15:24 aida_s Exp $");
#define	MY_UINT32MAX 0xffffffffU
#define UINT32_NUMLEN 10

RkcErrorBuf rkc_errors;
RkcConfMgr rkc_config;

static char *
config_path(name)
const char *name;
{
  const char *home;
  RkiStrbuf buf;

  RkiStrbuf_init(&buf);
  home = getenv("HOME");
  if (home) {
    if (RkiStrbuf_add(&buf, home))
      goto nomem;
    if (buf.sb_curr != buf.sb_buf && *(buf.sb_curr - 1) != '/') {
      if (RKI_STRBUF_ADDCH(&buf, '/'))
	goto nomem;
    }
  }
  if (RkiStrbuf_add(&buf, CONFIG_DIR) || RkiStrbuf_add(&buf, name)
      || RkiStrbuf_term(&buf))
    goto nomem;
  return buf.sb_buf;

nomem:
  RkiStrbuf_destroy(&buf);
  return NULL;
}

static char *
read_pipe_with_errors(cmd, errors, size)
const char *cmd;
RkcErrorBuf *errors;
size_t *size;
{
  int pipefds[4];
  pid_t pid;
  RkiStrbuf outbuf, errbuf;
  int i, r;
  rki_fd_set readmask0;
  int maxfd;
  int status = (int)0xdeadbeef, no_exitstatus = 0;

  for (i = 0; i < 4; ++i)
    pipefds[i] = -1;
  RkiStrbuf_init(&outbuf);
  RkiStrbuf_init(&errbuf);
  
  if (pipe(pipefds) || pipe(pipefds + 2)) {
    RkcErrorBuf_add(errors, "Cannot open pipe");
    goto fail;
  }
  pid = fork();
  if (pid < 0) {
    RkcErrorBuf_add(errors, "Fork failed");
    goto fail;
  } else if (pid == 0) {
#ifdef CONF_DEBUG
    fprintf(stderr, "CHILD: cmd=%s\n", cmd);
#endif
    errno = 0;
    if (dup2(pipefds[1], 1) < 0 || dup2(pipefds[3], 2) < 0)
      _exit(126);
    for (i = 0; i < 4; ++i)
      close(pipefds[i]);
    if (execl("/bin/sh", "sh", "-c", cmd, NULL))
      _exit(127);
  }
  close(pipefds[1]);
  close(pipefds[3]);
  pipefds[1] = pipefds[3] = -1;

  RKI_FD_ZERO(&readmask0);
  RKI_FD_SET(pipefds[0], &readmask0);
  RKI_FD_SET(pipefds[2], &readmask0);
  maxfd = pipefds[0] < pipefds[2] ? pipefds[2] : pipefds[0];
#ifdef CONF_DEBUG
  fprintf(stderr, "PARENT: start select loop\n");
#endif
  while (RKI_FD_ISSET(pipefds[0], &readmask0)
      && RKI_FD_ISSET(pipefds[2], &readmask0)) {
    rki_fd_set readmask = readmask0;
#ifdef CONF_DEBUG
    fprintf(stderr, "PARENT: invoke select\n");
#endif
    r = select(maxfd + 1, &readmask, NULL, NULL, NULL);
    if (r == -1) {
      if (errno == EINTR)
	continue;
      goto fail;
    }
    if (RKI_FD_ISSET(pipefds[0], &readmask)) {
      ssize_t rr;
      if (RKI_STRBUF_RESERVE(&outbuf, 1))
	goto nomem;
      rr = read(pipefds[0], outbuf.sb_curr, outbuf.sb_end - outbuf.sb_curr);
      if (rr < 0)
	goto fail;
      else if (rr == 0)
	RKI_FD_CLR(pipefds[0], &readmask0);
      else
	outbuf.sb_curr += rr;
    }
    if (RKI_FD_ISSET(pipefds[2], &readmask)) {
      ssize_t rr;
      if (RKI_STRBUF_RESERVE(&errbuf, 1))
	goto nomem;
      rr = read(pipefds[2], errbuf.sb_curr, errbuf.sb_end - errbuf.sb_curr);
      if (rr < 0)
	goto fail;
      else if (rr == 0)
	RKI_FD_CLR(pipefds[2], &readmask0);
      else
	errbuf.sb_curr += rr;
    }
  }

#ifdef CONF_DEBUG
  fprintf(stderr, "PARENT: read ok\n");
#endif
  if (errbuf.sb_buf) {
    char *ep, *eend;
    if (RkiStrbuf_term(&errbuf))
      goto fail;
    ep = errbuf.sb_buf;
    eend = errbuf.sb_curr;
    while (ep != eend) {
      char *bottom = strchr(ep, '\n');
      if (bottom == NULL) {
	bottom = eend - 1;
	if (bottom == ep)
	  break;
      }
      *bottom = '\0';
      RkcErrorBuf_add(errors, ep);
      ep = bottom + 1;
    }
  }

  close(pipefds[0]);
  close(pipefds[2]);
  RkiStrbuf_destroy(&errbuf);
  while (pid != waitpid(pid, &status, 0)) {
    if (errno != EINTR) {
      /* Client "ate" my child */
      no_exitstatus = 1;
      break;
    }
  }
  if ((no_exitstatus && errbuf.sb_curr == errbuf.sb_buf)
      || (WIFEXITED(status) && !WEXITSTATUS(status))) {
    RkiStrbuf_pack(&outbuf);
    *size = outbuf.sb_curr - outbuf.sb_buf;
    return outbuf.sb_buf;
  } else {
    char msg[sizeof "child received signal XXX"];
    if (no_exitstatus)
      ;
    else if (WIFEXITED(status))
      sprintf(msg, "child returned %d", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
      sprintf(msg, "child received signal %d", WTERMSIG(status));
    else
      assert(0);
    RkcErrorBuf_add(errors, msg);
    RkiStrbuf_destroy(&outbuf);
    return NULL;
  }

nomem:
  RkcErrorBuf_nomem(errors);
fail:
  for (i = 0; i < 4; ++i)
    if (pipefds[i] != -1)
      close(pipefds[i]);
  RkiStrbuf_destroy(&outbuf);
  RkiStrbuf_destroy(&errbuf);
  return NULL;
}

void
rkc_configure()
{
  const char *preproc;
  char *path = NULL, *cmd = NULL;
  Lexer *lexer = NULL;
  Parser *parser = NULL;
  char *input = NULL;
  struct stat st;
  size_t input_size;

  RkcErrorBuf_init(&rkc_errors);
  RkcConfMgr_init(&rkc_config, &rkc_errors);

  preproc = getenv("CANNA_RKC_PREPROCESSOR");
  if (!preproc || !*preproc)
    preproc = CPP;
  path = config_path(CONFIG_FILE);
  if (!path) {
    RkcErrorBuf_nomem(&rkc_errors);
    goto last;
  }
#ifdef CONF_DEBUG
  fprintf(stderr, "path=%s, preproc=%s\n", path, preproc);
#endif
  if (stat(path, &st)) {
    if (errno == ENOENT) {
      RkcErrorBuf_add(&rkc_errors, "RKC\244\316\300\337\304\352\245\325"
	  "\245\241\245\244\245\353\244\254\244\242\244\352\244\336\244"
	  "\273\244\363");
      /* RKCの設定ファイルがありません */
      goto last;
    } else
      goto input_error;
  }
  if (!(cmd = malloc(strlen(preproc) + strlen(path) + 2))) {
    RkcErrorBuf_nomem(&rkc_errors);
    goto last;
  }
  sprintf(cmd, "%s %s", preproc, path);
  if (!(input = read_pipe_with_errors(cmd, &rkc_errors, &input_size))
      || !(lexer = Lexer_new(input, input_size, &rkc_errors))
      || !(parser = Parser_new(&rkc_config, lexer, &rkc_errors)))
    goto input_error;
  Parser_run(parser);
  goto last;
input_error:
  RkcErrorBuf_add(&rkc_errors, "RKC\244\316\300\337\304\352\245\325\245"
      "\241\245\244\245\353\244\316\306\311\244\337\271\376\244\337\245"
      "\250\245\351\241\274\244\307\244\271");
    /* RKCの設定ファイルの読み込みエラーです */
last:
  Parser_delete(parser);
  Lexer_delete(lexer);
  free(input);
  free(cmd);
  free(path);
}

void
rkc_config_fin()
{
  RkcConfMgr_destroy(&rkc_config);
  RkcErrorBuf_destroy(&rkc_errors);
}

static void
RkcErrorBuf_init(cx)
RkcErrorBuf *cx;
{
  bzero(cx, sizeof(RkcErrorBuf));
}

static void
RkcErrorBuf_destroy(cx)
RkcErrorBuf *cx;
{
  if (cx->buf) {
    char **p = cx->buf, **endp = p + cx->curr;
    for (; p < endp; ++p)
      free(*p);
    free(cx->buf);
  }
}

static void
RkcErrorBuf_add(cx, msg)
RkcErrorBuf *cx;
const char *msg;
{
  char *newmsg;
  if (cx->nomem)
    return;
  if (!(newmsg = strdup(msg)))
    goto fail;

  /* reserve last 2 spaces for out-of-memory msg and NULL terminater */
  assert((cx->bufsize == 0 && cx->buf == NULL)
      || (cx->bufsize >= 10 && cx->curr + 2 <= cx->bufsize));
  if (cx->curr + 2 >= cx->bufsize) {
    size_t bufsize = cx->bufsize * 2 + 10;
    char **newbuf;
    newbuf = realloc(cx->buf, bufsize * sizeof(char *));
    if (!newbuf)
      goto fail;
    cx->buf = newbuf;
    cx->bufsize = bufsize;
  }
  cx->buf[cx->curr] = newmsg;
  ++cx->curr;
  return;
fail:
  free(newmsg);
  cx->nomem = 1;
}

const char *const *
RkcErrorBuf_get(cx)
RkcErrorBuf *cx;
{
  static const char *const altres1[] = { NULL };
  static const char *const altres2[] = { NOMEM_MSG, NULL };
  assert((cx->bufsize == 0 && cx->buf == NULL)
      || (cx->bufsize >= 10 && cx->curr + 2 <= cx->bufsize));
  if (cx->nomem) {
    if (cx->buf == NULL)
      return altres2;
    cx->buf[cx->curr] = NOMEM_MSG;
    cx->buf[cx->curr + 1] = NULL;
  } else {
    if (cx->buf == NULL)
      return altres1;
    cx->buf[cx->curr] = NULL;
  }
  return (const char **)cx->buf;
}

static int
Token_assignstr(tp, str, len, type)
TokenRec *tp;
const char *str;
size_t len;
int type;
{
  size_t *hdrp = malloc(sizeof(size_t) + len + 1);
  char *bodyp;
  assert(type >= TOK_STRING);
  if (!hdrp)
    return -1;
  *hdrp = 1;
  bodyp = (char *)hdrp + sizeof(size_t);
  memcpy(bodyp, str, len);
  bodyp[len] = '\0';
  assert(strlen(bodyp) == len);
  TOKEN_UNREF(tp);
  tp->type = type;
  tp->val.strval = bodyp;
  return 0;
}

static Lexer *
Lexer_new(srcdata, srcsize, errorbuf)
const char *srcdata;
size_t srcsize;
RkcErrorBuf *errorbuf;
{
  Lexer *cx;
  const char *p;
  cx = malloc(sizeof(Lexer));
  if (!cx) {
    RkcErrorBuf_nomem(errorbuf);
    return NULL;
  }
  for (p = srcdata; p < srcdata + srcsize; ++p)
    if (*p == '\0')
      goto fail; /* reject NUL in config file */
  cx->curr = srcdata;
  cx->rdend = srcdata + srcsize;
  cx->errorbuf = errorbuf;
  cx->lineno = 1;
  cx->eof_occured = 0;
  cx->linetop = 1;
  return cx;
fail:
  RkcErrorBuf_add(&rkc_errors, "\300\337\304\352\245\325\245\241\245"
      "\244\245\353\244\316\306\311\244\337\271\376\244\337\245"
      "\250\245\351\241\274\244\307\244\271");
    /* 設定ファイルの読み込みエラーです */
  free(cx);
  return NULL;
}

static void
Lexer_delete(cx)
Lexer *cx;
{
  if (!cx)
    return;
  free(cx);
}

static int
match_operator1(resp, postfix_op, ch)
TokenRec *resp;
int postfix_op;
int ch;
{
  static const struct {
    int op_char;
    Operator op_pre_code;
    Operator op_post_code;
  } oplist1[] = {
    { ',', OP_NONE, OP_COMMA },
    { '(', OP_LPAREN, OP_NONE },
    { ')', OP_NONE, OP_RPAREN },
    { '?', OP_NONE, OP_QUESTION },
    { ':', OP_NONE, OP_COLON },
    { '+', OP_UPLUS, OP_BPLUS },
    { '-', OP_UMINUS, OP_BMINUS },
    { '*', OP_NONE, OP_MULTIPLY },
    { '/', OP_NONE, OP_DIVIDE },
    { '%', OP_NONE, OP_MODULUS },
    { '&', OP_NONE, OP_BITAND },
    { '|', OP_NONE, OP_BITOR },
    { '^', OP_NONE, OP_BITXOR },
    { '~', OP_BITCOMPL, OP_NONE },
    { '<', OP_NONE, OP_LESS },
    { '>', OP_NONE, OP_GREATER },
    { '!', OP_LNOT, OP_NONE },
  };
  size_t i;
  for (i = 0; i < ARRAYLEN(oplist1); ++i) {
    if (ch == oplist1[i].op_char) {
      resp->type = TOK_OPERATOR;
      resp->val.opval = postfix_op
	? oplist1[i].op_post_code : oplist1[i].op_pre_code;
      return 1;
    }
  }
  return 0;
}

static int
match_operator2(resp, postfix_op, ch1, ch2)
TokenRec *resp;
int postfix_op;
int ch1;
int ch2;
{
  static const struct {
    char op_expr[2];
    Operator op_code;
  } oplist2[] = {
    { "<<", OP_LSHIFT },
    { ">>", OP_RSHIFT },
    { "==", OP_EQUAL },
    { "!=", OP_NEQUAL },
    { "<=", OP_LEQUAL },
    { ">=", OP_GEQUAL },
  };
  size_t i;
  for (i = 0; i < ARRAYLEN(oplist2); ++i) {
    if (ch1 == oplist2[i].op_expr[0]
	&& ch2 == oplist2[i].op_expr[1]) {
      resp->type = TOK_OPERATOR;
      resp->val.opval = postfix_op ? oplist2[i].op_code : OP_NONE;
      return 1;
    }
  }
  return 0;
}

static int
Lexer_next(cx, resp, postfix_op)
Lexer *cx;
TokenRec *resp;
int postfix_op;
{
  int ch = 0; /* stop gcc's warning */

  assert(!cx->eof_occured);

restart:
  while (cx->curr != cx->rdend) {
    ch = (int)(unsigned char)*cx->curr;
    if (ch == '\n') {
      ++cx->lineno;
      cx->linetop = 1;
    }
    if (!isspace(ch))
      break;
    ++cx->curr;
  }
  if (cx->curr == cx->rdend)
    goto eof;
  else if (cx->linetop && ch == '#') {
    const char *p = cx->curr + 1;
    while (p != cx->rdend && *p != '\n' && isspace(*p))
      ++p;
    if (p == cx->rdend || *p == '\n')
      goto not_a_directive;
    if (p + sizeof "pragma X" - 1 <= cx->rdend
	&& !memcmp(p, "pragma ", sizeof "pragma " - 1))
      goto skiptoeol;
    if (p + sizeof "line X" - 1 <= cx->rdend
	&& !memcmp(p, "line ", sizeof "line " - 1))
      p += sizeof "line " - 1;
    if (isdigit(*p)) {
      char numbuf[20];
      size_t count;
      count = cx->rdend - p;
      if (count > sizeof numbuf - 1)
	count = sizeof numbuf - 1;
      memcpy(numbuf, p, count);
      numbuf[count] = '\0';
      cx->lineno = (unsigned int)strtol(numbuf, NULL, 10);
      p = memchr(p, '\n', cx->rdend - p);
      goto skiptoeol;
    }
    goto not_a_directive;
skiptoeol:
    p = memchr(p, '\n', cx->rdend - p);
    if (!p) {
      cx->curr = cx->rdend;
      goto eof;
    } else {
      cx->curr = p + 1;
      goto restart;
    }
not_a_directive:;
  }
  cx->linetop = 0;
  if (isdigit(ch))
    goto getnum;
  else if (ch == '"')
    goto getstr;
  else if (ch == '_' || isalpha(ch))
    goto getword;
  else if (ch == '/' && (cx->curr + 1 != cx->rdend)) {
    if (*(cx->curr + 1) == '*')
      goto skipcomm1;
    else if (*(cx->curr + 1) == '/')
      goto skipcomm2;
  }
  if (cx->curr + 1 != cx->rdend
      && match_operator2(resp, postfix_op, ch,
	(int)(unsigned char)*(cx->curr + 1))) {
    cx->curr += 2;
  } else if (match_operator1(resp, postfix_op, ch)) {
    ++cx->curr;
  } else {
    ++cx->curr;
    resp->type = (ch == ';') ? TOK_SEMICOLON : TOK_CHAR;
    resp->val.chval = ch;
  }
  return 0;

eof:
  resp->type = TOK_EOF;
  cx->eof_occured = 1;
  return 0;
skipcomm1:
  cx->curr += 2;
  ch = '\0';
  while (cx->curr != cx->rdend) {
    int ch2 = (int)(unsigned char)*cx->curr++;
    if (ch2 == '\n')
      ++cx->lineno;
    else if (ch == '*' && ch2 == '/')
      goto restart;
    ch = ch2;
  }
  Lexer_error(cx, "EOF in comment");
  goto eof;
skipcomm2:
  cx->curr += 2;
  while (cx->curr != cx->rdend) {
    int ch = (int)(unsigned char)*cx->curr++;
    if (ch == '\n') {
      ++cx->lineno;
      goto restart;
    }
  }
  goto eof;
getword:
  {
    const char *startp = cx->curr;
    ++cx->curr;
    while (cx->curr != cx->rdend) {
      ch = (int)(unsigned char)*cx->curr;
      if (ch != '_' && !isalnum(ch))
	break;
      ++cx->curr;
    }
    if (cx->curr - startp == 3 && !strncmp(startp, "yes", 3)) {
      resp->type = TOK_YESNO;
      resp->val.numval = 1;
    } else if (cx->curr - startp == 2 && !strncmp(startp, "no", 2)) {
      resp->type = TOK_YESNO;
      resp->val.numval = 0;
    } else {
      if (Token_assignstr(resp, startp, cx->curr - startp, TOK_WORD)) {
	RkcErrorBuf_nomem(cx->errorbuf);
	return -1;
      }
    }
    return 0;
  }
getnum:
  {
    const char *startp = cx->curr;
    char numbuf[UINT32_NUMLEN + 1];
    size_t numlen;
    char *nextp;
    unsigned long i;
    while (cx->curr != cx->rdend
	&& isdigit((int)(unsigned char)*cx->curr))
      ++cx->curr;
    numlen = cx->curr - startp;
    if (numlen >= sizeof numbuf)
      goto numinval;
    memcpy(numbuf, startp, numlen);
    numbuf[numlen] = '\0';
    assert(numlen > 0);
    errno = 0;
    i = strtoul(numbuf, &nextp, 10);
    if (nextp != numbuf + numlen)
      goto numinval;
    if (i > UINT_MAX || errno == ERANGE)
      goto numinval;
    resp->type = TOK_NUMBER;
    resp->val.numval = i;
    return 0;
numinval:
    Lexer_error(cx, "Invalid number");
    goto tokinval;
  }
getstr:
  {
    const char *startp;
    char *p1, *p2;
    ++cx->curr;
    startp = cx->curr;
    while (cx->curr != cx->rdend) {
      ch = (int)(unsigned char)*cx->curr;
      if (ch == '"')
	break;
      else if (ch == '\\') {
	++cx->curr;
	if (cx->curr == cx->rdend) {
	  Lexer_error(cx, "Backslash at end of file");
	  return -1;
	}
      }
      ++cx->curr;
    }
    if (cx->curr == cx->rdend) {
      Lexer_error(cx, "Unterminated string");
      goto tokinval;
    }
    /* first store escaped string and then unescape */
    ++cx->curr;
    if (Token_assignstr(resp, startp, cx->curr - 1 - startp, TOK_STRING)) {
      RkcErrorBuf_nomem(cx->errorbuf);
      return -1;
    }
    for (p1 = p2 = resp->val.strval; *p2; ++p1, ++p2) {
      if (*p2 == '\\')
	++p2;
      assert(*p2); /* escape handling was wrong if this fails */
      *p1 = *p2;
    }
    *p1 = '\0';
    return 0;
  }
tokinval:
  resp->type = TOK_INVAL;
  return 0;
}

static void
Lexer_error(cx, msg)
const Lexer *cx;
const char *msg;
{
  char *newmsg;
  unsigned int lineno;
  
  newmsg = malloc(sizeof("line X(10col)XX: ") + strlen(msg));
  if (!newmsg) {
    RkcErrorBuf_nomem(cx->errorbuf);
    return;
  }
  lineno = cx->lineno;
  if (lineno > MY_UINT32MAX)
    lineno = MY_UINT32MAX;
  sprintf(newmsg, "line %u: %s", lineno, msg);
  RkcErrorBuf_add(cx->errorbuf, newmsg);
  free(newmsg);
}

#if defined(CONF_EVAL_DEBUG) || defined(CONF_LEXER_DEBUG)
static const char *op_dump[OP_DUMMY] = {
  "(none)", ",", "(", ")", "?", ":",
  "+(u)", "-(u)", "+(b)", "-(b)", "*", "/", "%",
  "<<", ">>", "&", "|", "^", "~",
  "==", "!=", "<", ">", "<=", ">=", "&&", "||", "!"
};
#endif
#ifdef CONF_LEXER_DEBUG
static void
Token_dump(tp)
const TokenRec *tp;
{
  switch (tp->type) {
    case TOK_INVAL:
      fprintf(stderr, "Invalid token\n");
      break;
    case TOK_EOF:
      fprintf(stderr, "End of file\n");
      break;
    case TOK_WORD:
      fprintf(stderr, "Word: %s\n", tp->val.strval);
      break;
    case TOK_NUMBER:
      fprintf(stderr, "Number: %u\n", tp->val.numval);
      break;
    case TOK_STRING:
      fprintf(stderr, "String: %s\n", tp->val.strval);
      break;
    case TOK_CHAR:
      fprintf(stderr, "Char: %c\n", tp->val.chval);
      break;
    case TOK_SEMICOLON:
      fprintf(stderr, "Semicolon\n");
      break;
    case TOK_OPERATOR:
      fprintf(stderr, "Operator: %s\n", op_dump[tp->val.opval]);
      break;
    default:
      fprintf(stderr, "BUG: unknown token id %d\n", tp->type);
      break;
  }
}
#endif /* CONF_LEXER_DEBUG */

static Parser *
Parser_new(confmgr, lexer, errorbuf)
RkcConfMgr *confmgr;
Lexer *lexer;
RkcErrorBuf *errorbuf;
{
  Parser *cx;
  cx = malloc(sizeof(Parser));
  if (!cx) {
    RkcErrorBuf_nomem(errorbuf);
    return NULL;
  }
  cx->confmgr = confmgr;
  cx->lexer = lexer;
  cx->errorbuf = errorbuf;
  cx->discard = 0;
  TOKEN_INIT(&cx->currtok);
  return cx;
}

static void
Parser_delete(cx)
Parser *cx;
{
  if (!cx)
    return;
  TOKEN_DESTROY(&cx->currtok);
  free(cx);
}

static void
Parser_run(cx)
Parser *cx;
{
  if (Parser_next(cx))
    goto fail;
  if (syn_top(cx))
    goto fail;
  assert(cx->currtok.type == TOK_EOF);
  return;
fail:
  return;
}

static int
Parser_next(cx)
Parser *cx;
{
  int r = Lexer_next(cx->lexer, &cx->currtok, 0);
#ifdef CONF_LEXER_DEBUG
  Token_dump(&cx->currtok);
#endif
  return r;
}

static int
Parser_next_postfixop(cx)
Parser *cx;
{
  int r;
  r = Lexer_next(cx->lexer, &cx->currtok, 1);
#ifdef CONF_LEXER_DEBUG
  Token_dump(&cx->currtok);
#endif
  return r;
}

static void
Parser_error(cx, msg)
Parser *cx;
const char *msg;
{
  if (!cx->discard)
    Lexer_error(cx->lexer, msg);
}

static int
Parser_stmt(cx, stmttab, nstmt)
Parser *cx;
const StmtRec *stmttab;
size_t nstmt;
{
  size_t i;
  
  assert(cx->currtok.type != TOK_EOF);
  if (cx->currtok.type != TOK_WORD) {
    Parser_error(cx, "Syntax error");
    goto skip;
  }
  for (i = 0; i < nstmt; ++i)
    if (!strcmp(cx->currtok.val.strval, stmttab[i].name))
      break;
  if (i == nstmt) {
    Parser_error(cx, "Unknown statement");
    goto skip;
  }

  if (Parser_next(cx))
    return -1;
  if (CONF_TYPE(stmttab[i].item) == CONF_SPECIAL) {
    if ((*stmttab[i].proc)(cx))
      return -1;
  } else if (cx->currtok.type == TOK_EOF
      || cx->currtok.type == TOK_SEMICOLON) {
    Parser_error(cx, "No arguments");
  } else {
    switch (CONF_TYPE(stmttab[i].item)) {
      case CONF_STRING:
	if (cx->currtok.type != TOK_STRING) {
	  Parser_error(cx, "String argument required");
	} else {
	  char *str = Parser_getstr(cx);
	  int r;
	  if (!str)
	    return -1;
	  r = !cx->discard
	      && RkcConfMgr_set_string(cx->confmgr, stmttab[i].item, str);
	  free(str);
	  if (r)
	    return -1;
	}
	break;
      case CONF_NUMBER:
	if (cx->currtok.type != TOK_NUMBER
	    && cx->currtok.type != TOK_OPERATOR) {
	  Parser_error(cx, "Numeric argument required");
	} else {
	  int r = Parser_eval(cx, COMMA_LPRIO);
	  if (r == -1)
	    return -1;
	  if (!r && !cx->discard
	      && RkcConfMgr_set_number(cx->confmgr,
		stmttab[i].item, cx->exprval))
	    return -1;
	}
	break;
      case CONF_YESNO:
	if (cx->currtok.type != TOK_YESNO) {
	  Parser_error(cx, "Yes or no required");
	} else {
	  if (!cx->discard
	      && RkcConfMgr_set_yesno(cx->confmgr,
		stmttab[i].item, cx->currtok.val.numval))
	    return -1;
	}
	if (Parser_next(cx))
	  return -1;
	break;
      default:
	assert(0);
    }
  }
  if (cx->currtok.type == TOK_EOF) {
    Parser_error(cx, "Unexpected EOF");
    return 0;
  } else if (cx->currtok.type != TOK_SEMICOLON) {
    Parser_error(cx, "Extra arguments");
    goto skip;
  }
  return Parser_next(cx);
  
skip:
  while (cx->currtok.type != TOK_SEMICOLON && cx->currtok.type != TOK_EOF)
    if (Parser_next(cx))
      return -1;
  return (cx->currtok.type == TOK_EOF) ? 0 : Parser_next(cx);
}

static const OperatorRec operators[] = {
  { DUMMY_PRIO,	DUMMY_PRIO,	(CalcProc)NULL },	/* OP_NONE */
  { 11,		10,		&calc_comma },		/* OP_COMMA */
  { 0,		DUMMY_PRIO,	(CalcProc)NULL },	/* OP_LPAREN */
  { DUMMY_PRIO,	0,		(CalcProc)NULL },	/* OP_RPAREN */
  { 20,		21,		(CalcProc)NULL },	/* OP_QUESTION */
  { 20,		20,		(CalcProc)NULL },	/* OP_COLON */
  
  { 150,	DUMMY_PRIO,	&calc_uplus },		/* OP_UPLUS */
  { 150,	DUMMY_PRIO,	&calc_uminus },		/* OP_UMINUS */
  { 121,	120,		&calc_bplus },		/* OP_BPLUS */
  { 121,	120,		&calc_bminus },		/* OP_MINUS */
  { 131,	130,		&calc_multiply },	/* OP_MULTIPLY */
  { 131,	130,		&calc_divide },		/* OP_DIVIDE */
  { 131,	130,		&calc_modulus },	/* OP_MODULUS */
  { 111,	110,		&calc_lshift },		/* OP_LSHIFT */
  { 111,	110,		&calc_rshift },		/* OP_RSHIFT */
  { 81,		80,		&calc_bitand },		/* OP_BITAND */
  { 61,		60,		&calc_bitor },		/* OP_BITOR */
  { 71,		70,		&calc_bitxor },		/* OP_BITXOR */
  { 150,	DUMMY_PRIO,	&calc_bitcompl },	/* OP_BITCOMPL */

  { 91,		90,		&calc_equal },		/* OP_EQUAL */
  { 91,		90,		&calc_nequal },		/* OP_NEQUAL */
  { 101,	100,		&calc_less },		/* OP_LESS */
  { 101,	100,		&calc_greater },	/* OP_GREATER */
  { 101,	100,		&calc_lequal },		/* OP_LEQUAL */
  { 101,	100,		&calc_gequal },		/* OP_GEQUAL */
  { 51,		50,		&calc_land },		/* OP_LAND */
  { 41,		40,		&calc_lor },		/* OP_LOR */
  { 150,	DUMMY_PRIO,	&calc_lnot },		/* OP_LNOT */
};

static void
Parser_eval_error(cx)
Parser *cx;
{
  Parser_error(cx, "Syntax error in an expression");
}

static int
Parser_eval(cx, lprio)
Parser *cx;
int lprio;
{
  int r;
  unsigned int val1;
  Operator op;

#ifdef CONF_EVAL_DEBUG
  fprintf(stderr, "Parser_eval: entering lprio=%d\n", lprio);
#endif
  switch (cx->currtok.type) {
    case TOK_OPERATOR:
      op = cx->currtok.val.opval;
#ifdef CONF_EVAL_DEBUG
      fprintf(stderr, "Parser_eval: unary prefix operator %s\n", op_dump[op]);
#endif
      if (op == OP_NONE) {
	Parser_eval_error(cx);
	return 1;
      }
      assert(operators[op].lprio != DUMMY_PRIO);
      if (Parser_next(cx))
	return -1;
      r = Parser_eval(cx, operators[op].lprio);
      if (r)
	return r;

      if (op == OP_LPAREN) {
	if (cx->currtok.type != TOK_OPERATOR
	    || cx->currtok.val.opval != OP_RPAREN) {
	  Parser_error(cx, "Open parenthesis");
	  return 1;
	}
	val1 = cx->exprval;
	if (Parser_next_postfixop(cx))
	  return -1;
      } else {
	assert(cx->currtok.type == TOK_SEMICOLON
	    || (cx->currtok.type == TOK_OPERATOR
	      && operators[op].lprio
		> operators[cx->currtok.val.opval].rprio));
	val1 = (*operators[op].calc)(cx->exprval, (int)0xdeadbeef);
      }
      break;

    case TOK_NUMBER:
      val1 = cx->currtok.val.numval;
#ifdef CONF_EVAL_DEBUG
      fprintf(stderr, "Parser_eval: got number %u\n", val1);
#endif
      if (Parser_next_postfixop(cx))
	return -1;
      break;
    default:
      Parser_eval_error(cx);
      return 1;
  }

checkpostfixop:
#ifdef CONF_EVAL_DEBUG
  fprintf(stderr, "Parser_eval: checkpostfixop: val1=%u\n", val1);
#endif
  switch (cx->currtok.type) {
    case TOK_OPERATOR:
      op = cx->currtok.val.opval;
#ifdef CONF_EVAL_DEBUG
      fprintf(stderr, "Parser_eval: postfix operator %s\n", op_dump[op]);
#endif
      if (op == OP_NONE) {
	Parser_eval_error(cx);
	return 1;
      }
      assert(operators[op].rprio != DUMMY_PRIO);
#ifdef CONF_EVAL_DEBUG
      fprintf(stderr, "Parser_eval: lprio=%d, operators[op].rprio=%d\n",
	  lprio, operators[op].rprio);
#endif
      if (lprio >= operators[op].rprio) {
	assert(op == OP_RPAREN || op == OP_COLON
	    || lprio > operators[op].rprio);
	cx->exprval = val1;
      } else {
#ifdef unused
	if (operators[op].lprio == DUMMY_PRIO) {
	  val1 = (*operators[op].calc) (val1, (int)0xdeadbeef);
	  if (Parser_next_postfixop(cx))
	    return -1;
	  goto checkpostfixop;
	}
#endif
	assert(operators[op].lprio != DUMMY_PRIO);
	if (Parser_next(cx))
	  return -1;
	r = Parser_eval(cx, operators[op].lprio);
	if (r)
	  return r;

	if (op == OP_QUESTION) {
	  unsigned int val2 = cx->exprval;
	  if (cx->currtok.type != TOK_OPERATOR
	      || cx->currtok.val.opval != OP_COLON) {
	    Parser_error(cx, "Isolated '?' operator");
	    return 1;
	  }
	  if (Parser_next(cx))
	    return -1;
	  r = Parser_eval(cx, COLON_LPRIO);
	  if (r)
	    return r;
	  assert(cx->currtok.type == TOK_SEMICOLON
	      || (cx->currtok.type == TOK_OPERATOR
		&& COLON_LPRIO > operators[cx->currtok.val.opval].rprio));
	  val1 = val1 ? val2 : cx->exprval;
	} else if (op == OP_COLON) {
	  Parser_error(cx, "Isolated ':' operator");
	  return 1;
	} else {
	  assert(cx->currtok.type == TOK_SEMICOLON
	      || (cx->currtok.type == TOK_OPERATOR
		&& operators[op].lprio
		  > operators[cx->currtok.val.opval].rprio));
	  val1 = (*operators[op].calc)(val1, cx->exprval);
	}
	goto checkpostfixop;
      }
      break;
    case TOK_SEMICOLON:
      cx->exprval = val1;
      break;
    default:
      Parser_eval_error(cx);
      return 1;
  }
#ifdef CONF_EVAL_DEBUG
  fprintf(stderr, "Parser_eval: result %u\n", cx->exprval);
#endif
  return 0;
}

static char *
Parser_getstr(cx)
Parser *cx;
{
  RkiStrbuf sb;

  RkiStrbuf_init(&sb);
  do {
    if (RkiStrbuf_add(&sb, cx->currtok.val.strval)) {
      RkcErrorBuf_nomem(cx->errorbuf);
      goto fail;
    }
    if (Parser_next(cx))
      goto fail;
  } while (cx->currtok.type == TOK_STRING);
  if (RkiStrbuf_term(&sb))
    goto fail;
  return sb.sb_buf;
fail:
  RkiStrbuf_destroy(&sb);
  return NULL;
}

DEF_CALCPROC(calc_comma)
/* ARGUSED */
{ return arg2; }
DEF_CALCPROC_OP1(calc_uplus,	+)
DEF_CALCPROC_OP1(calc_uminus,	-)
DEF_CALCPROC_OP2(calc_bplus,	+)
DEF_CALCPROC_OP2(calc_bminus,	-)
DEF_CALCPROC_OP2(calc_multiply,	*)
DEF_CALCPROC_OP2(calc_divide,	/)
DEF_CALCPROC_OP2(calc_modulus,	%)
DEF_CALCPROC_OP2(calc_lshift,	<<)
DEF_CALCPROC_OP2(calc_rshift,	>>)
DEF_CALCPROC_OP2(calc_bitand,	&)
DEF_CALCPROC_OP2(calc_bitor,	|)
DEF_CALCPROC_OP2(calc_bitxor,	^)
DEF_CALCPROC_OP1(calc_bitcompl,	~)
DEF_CALCPROC_OP2(calc_equal,	==)
DEF_CALCPROC_OP2(calc_nequal,	!=)
DEF_CALCPROC_OP2(calc_less,	<)
DEF_CALCPROC_OP2(calc_greater,	>)
DEF_CALCPROC_OP2(calc_lequal,	<=)
DEF_CALCPROC_OP2(calc_gequal,	>=)
DEF_CALCPROC_OP2(calc_land,	&&)
DEF_CALCPROC_OP2(calc_lor,	||)
DEF_CALCPROC_OP1(calc_lnot,	!)

static const StmtRec top_statements[] = {
  { "cannahost", CONF_CANNAHOST, (StmtProc)NULL},
  { "host", CONF_SPECIAL, (StmtProc)&syn_host},
};

static int
syn_top(cx)
Parser *cx;
{
  while (cx->currtok.type != TOK_EOF) {
    if (cx->currtok.type == TOK_SEMICOLON) {
      if (Parser_next(cx))
	return -1;
      continue;
    }
    if (Parser_stmt(cx, top_statements, ARRAYLEN(top_statements)))
      return -1;
  }
  return 0;
}

static const StmtRec host_statements[] = {
  { "server_timeout", CONF_SERVER_TIMEOUT, (StmtProc)NULL },
};

static int
syn_host(cx)
Parser *cx;
{
  int res;
  int old_discard = cx->discard;
  int need_close_host = 0;
  char *host = NULL;

  if (cx->currtok.type == TOK_EOF || cx->currtok.type == TOK_SEMICOLON) {
    /* discard nothing */
    Parser_error(cx, "Lack of hostname");
    goto normalend;
  }

  if (cx->currtok.type != TOK_STRING) {
    Parser_error(cx, "Invalid argument");
    /* parse but discard everything */
    if (Parser_next(cx))
      goto fatal;
    cx->discard = 1;
  }

  if (!(host = Parser_getstr(cx)))
    goto fatal;

  if (!(cx->currtok.type == TOK_CHAR && cx->currtok.val.chval == '{')) {
    /* discard only first arg */
    Parser_error(cx, "Lack of open brace");
    goto normalend;
  }

  assert(cx->currtok.type == TOK_CHAR && cx->currtok.val.chval == '{');
  if (Parser_next(cx))
    goto fatal;

  if (!cx->discard) {
    if (RkcConfMgr_openhost(cx->confmgr, host))
      goto fatal;
    need_close_host = 1;
  }

  while (cx->currtok.type != TOK_EOF
      && !(cx->currtok.type == TOK_CHAR && cx->currtok.val.chval == '}')) {
    if (cx->currtok.type == TOK_SEMICOLON) {
      if (Parser_next(cx))
	goto fatal;
      continue;
    }
    if (Parser_stmt(cx, host_statements, ARRAYLEN(host_statements)))
      goto fatal;
  }

  if (cx->currtok.type == TOK_EOF) {
    Parser_error(cx, "EOF in Host block");
    goto normalend;
  }
  assert(cx->currtok.type == TOK_CHAR && cx->currtok.val.chval == '}');
  res = Parser_next(cx);
  goto allend;

fatal:
  res = -1;
  goto allend;
normalend:
  res = 0;
allend:
  cx->discard = old_discard;
  if (need_close_host)
    RkcConfMgr_closehost(cx->confmgr);
  free(host);
  return res;
}

static void
RkcConfMgr_init(cx, errors)
RkcConfMgr *cx;
RkcErrorBuf *errors;
{
  bzero(cx, sizeof(RkcConfMgr));
  cx->errors = errors;
}

static void
RkcConfMgr_destroy(cx)
RkcConfMgr *cx;
{
  size_t pos;
  HostRec *currhost = cx->hosts, *nexthost;
  for (pos = 0; pos < cx->n_topconf; ++pos)
    if (CONF_TYPE(cx->topconf[pos].item) == CONF_STRING)
      free(cx->topconf[pos].val.strval);
  free(cx->topconf);

  while (currhost) {
    nexthost = currhost->next;
    free(currhost->hostname);
    for (pos = 0; pos < currhost->n_hostconf; ++pos)
      if (CONF_TYPE(currhost->hostconf[pos].item) == CONF_STRING)
	free(currhost->hostconf[pos].val.strval);
    free(currhost->hostconf);
    free(currhost);
    currhost = nexthost;
  }
}

static int
RkcConfMgr_openhost(cx, hostname)
RkcConfMgr *cx;
const char *hostname;
{
  HostRec *hostrec;

  assert(!cx->currhost);

  hostrec = calloc(1, sizeof(HostRec));
  if (!hostrec)
    goto nomem;
  hostrec->hostname = strdup(hostname);
  if (!hostrec->hostname)
    goto nomem;
  hostrec->next = cx->hosts;
  cx->hosts = cx->currhost = hostrec;
  return 0;

nomem:
  if (hostrec) {
    free(hostrec->hostname);
    free(hostrec);
  }
  RkcErrorBuf_nomem(cx->errors);
  return -1;
}

static void
RkcConfMgr_closehost(cx)
RkcConfMgr *cx;
{
  assert(cx->currhost);
  cx->currhost = NULL;
}

static ConfRec *
RkcConfMgr_get_target(cx, item)
RkcConfMgr *cx;
ConfItem item;
{
  ConfRec *rec, *endrec;
  ConfRec **conf;
  size_t *n_conf, *conf_size;

  if (cx->currhost) {
    conf = &cx->currhost->hostconf;
    n_conf = &cx->currhost->n_hostconf;
    conf_size = &cx->currhost->hostconf_size;
  } else {
    conf = &cx->topconf;
    n_conf = &cx->n_topconf;
    conf_size = &cx->topconf_size;
  }

  for (rec = *conf, endrec = *conf + *n_conf; rec; ++rec)
    if (rec->item == item)
      return rec;

  if (*n_conf == *conf_size) {
    size_t newsize = *conf_size * 2 + 2;
    ConfRec *tmp;

    tmp = realloc(*conf, newsize * sizeof(ConfRec));
    if (!tmp) {
      RkcErrorBuf_nomem(cx->errors);
      return NULL;
    }
    *conf = tmp;
    *conf_size = newsize;
  }
  rec = *conf + *n_conf;
  rec->item = item;
  ++*n_conf;
  return rec;
}

static int
RkcConfMgr_set_string(cx, item, val)
RkcConfMgr *cx;
ConfItem item;
const char *val;
{
  char *newval;
  ConfRec *target;

  assert(CONF_TYPE(item) == CONF_STRING);
  if (!(newval = strdup(val)))
    goto nomem;
  if (!(target = RkcConfMgr_get_target(cx, item)))
    return -1;
  target->val.strval = newval;
  return 0;
nomem:
  RkcErrorBuf_nomem(cx->errors);
  return -1;
}

int
RkcConfMgr_set_number(cx, item, val)
RkcConfMgr *cx;
ConfItem item;
unsigned int val;
{
  ConfRec *target;

  assert(CONF_TYPE(item) == CONF_NUMBER);
  if (!(target = RkcConfMgr_get_target(cx, item)))
    return -1;
  target->val.numval = val;
  return 0;
}

int
RkcConfMgr_set_yesno(cx, item, val)
RkcConfMgr *cx;
ConfItem item;
int val;
{
  ConfRec *target;

  assert(CONF_TYPE(item) == CONF_YESNO);
  if (!(target = RkcConfMgr_get_target(cx, item)))
    return -1;
  target->val.yesnoval = val;
  return 0;
}

static int
hostname_match(pattern, name)
const char *pattern;
const char *name;
{
  const char *p, *endp;
  size_t namelen = strlen(name);

  p = pattern;
  for (;;) {
    endp = strchr(p, ',');
    if (!endp)
      break;
    if ((endp - p == 1 && *p == '*')
	|| (endp - p == namelen && !strncmp(p, name, namelen)))
      return 1;
    p = endp + 1;
  }
  return !strcmp(p, "*") || !strcmp(p, name);
}

static const ConfRec *
RkcConfMgr_find(cx, item, hostname)
const RkcConfMgr *cx;
ConfItem item;
const char *hostname;
{
  ConfRec *confrec, *confend;
  if (hostname) {
    const HostRec *hostrec;
    for (hostrec = cx->hosts; hostrec; hostrec = hostrec->next) {
      if (hostname_match(hostrec->hostname, hostname)) {
	confrec = hostrec->hostconf;
	confend = confrec + hostrec->n_hostconf;
	for (; confrec != confend; ++confrec)
	  if (confrec->item == item)
	    return confrec;
      }
    }
  } else {
    for (confrec = cx->topconf, confend = cx->topconf + cx->n_topconf;
	confrec != confend; ++confrec)
      if (confrec->item == item)
	return confrec;
  }
  return NULL;
}

typedef struct {
  ConfItem item;
  const char *val;
} StrDefaultRec;

typedef struct {
  ConfItem item;
  unsigned int val;
} NumberDefaultRec;

const StrDefaultRec top_str_defaults[] = {
  { CONF_CANNAHOST, ""  },
};

const StrDefaultRec host_str_defaults[] = {
  { CONF_DUMMYCODE, NULL },
};

const NumberDefaultRec top_num_defaults[] = {
  { CONF_DUMMYCODE, 0u },
};

const NumberDefaultRec host_num_defaults[] = {
  { CONF_SERVER_TIMEOUT, 1500u },
};

const char *
RkcConfMgr_get_string(cx, item, hostname)
const RkcConfMgr *cx;
ConfItem item;
const char *hostname;
{
  const ConfRec *confrec;
  const StrDefaultRec *defrec, *endrec;

  assert(CONF_TYPE(item) == CONF_STRING);
  confrec = RkcConfMgr_find(cx, item, hostname);
  if (confrec)
    return confrec->val.strval;

  if (hostname) {
    defrec = host_str_defaults;
    endrec = defrec + ARRAYLEN(host_str_defaults);
  } else {
    defrec = top_str_defaults;
    endrec = defrec + ARRAYLEN(top_str_defaults);
  }
  for (; defrec != endrec; ++defrec)
    if (defrec->item == item)
      break;
  assert(defrec != endrec);
  return (char *)defrec->val;
}

unsigned int
RkcConfMgr_get_number(cx, item, hostname)
const RkcConfMgr *cx;
ConfItem item;
const char *hostname;
{
  const ConfRec *confrec;
  const NumberDefaultRec *defrec, *endrec;

  assert(CONF_TYPE(item) == CONF_NUMBER);
  confrec = RkcConfMgr_find(cx, item, hostname);
  if (confrec)
    return confrec->val.numval;

  if (hostname) {
    defrec = host_num_defaults;
    endrec = defrec + ARRAYLEN(host_num_defaults);
  } else {
    defrec = top_num_defaults;
    endrec = defrec + ARRAYLEN(top_num_defaults);
  }
  for (; defrec != endrec; ++defrec)
    if (defrec->item == item)
      break;
  assert(defrec != endrec);
  return defrec->val;
}

int
RkcConfMgr_get_yesno(cx, item, hostname)
const RkcConfMgr *cx;
ConfItem item;
const char *hostname;
{
  const ConfRec *confrec;
  const NumberDefaultRec *defrec, *endrec;

  assert(CONF_TYPE(item) == CONF_YESNO);
  confrec = RkcConfMgr_find(cx, item, hostname);
  if (confrec)
    return confrec->val.yesnoval;

  if (hostname) {
    defrec = host_num_defaults;
    endrec = defrec + ARRAYLEN(host_num_defaults);
  } else {
    defrec = top_num_defaults;
    endrec = defrec + ARRAYLEN(top_num_defaults);
  }
  for (; defrec != endrec; ++defrec)
    if (defrec->item == item)
      break;
  assert(defrec != endrec);
  return (int)defrec->val;
}

/* vim: set sw=2: */
