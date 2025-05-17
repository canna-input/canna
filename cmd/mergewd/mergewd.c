/* Copyright (c) 2008 Canna Project. All rights reserved.
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

#include	"RKintern.h" 
#include	"RKindep/ecfuncs.h" 
#include	<stdio.h>
#include	<assert.h>

RCSID("$Id: mergewd.c,v 1.4 2008/04/05 17:25:45 aida_s Exp $");

typedef struct LineList {
  struct LineList *next;
  char *yomi;
  char *hinshi;
  char *kanji;
  unsigned int freq;
} LineList;
#define CHECK_HEAD(h, t) assert((h) && (t) && ((*(h) && *(t)) || (!*(h) && !*(t))))
#define	ISSPACE(c) (isascii((unsigned char)(c)) && isspace((unsigned char)(c)))

/* pop first at most n elements from *h2...*t2 and push them after *t1 */
size_t
LineList_moveleft(h1, t1, h2, t2, n)
LineList **h1, **t1, **h2, **t2;
size_t n;
{
  size_t i;
  LineList *p;

  CHECK_HEAD(h1, t1);
  CHECK_HEAD(h2, t2);
  if (!*h2 || !n)
    return 0;
  for (i = 1, p = *h2; i != n && p->next; ++i, p = p->next)
    ;
  if (*h1)
    (*t1)->next = *h2;
  else
    *h1 = *h2;
  *t1 = p;
  if (p->next) {
    *h2 = p->next;
    p->next = NULL;
  } else {
    *h2 = *t2 = NULL;
  }
  return i;
}

/* merge sort */
void
LineList_sort(head, tail)
LineList **head, **tail;
{
  size_t unit = 1;
  int atonce;

  CHECK_HEAD(head, tail);
  do {
    LineList *h1 = NULL, *t1 = NULL;

    atonce = 1;
    while (*head) {
      LineList *h2, *t2, *h3, *t3;

      h2 = t2 = h3 = t3 = NULL;
      /* Take sublists to merge */
      LineList_moveleft(&h2, &t2, head, tail, unit);
      LineList_moveleft(&h3, &t3, head, tail, unit);
      if (*head)
	atonce = 0;
      /* no problem even if the source list (*head...*tail) were exhausted */
      while (h2 && h3) {
	int yomi_cmp = strcmp(h2->yomi, h3->yomi);
	if (yomi_cmp < 0 || (yomi_cmp == 0 && h2->freq >= h3->freq)) {
	  if (h1)
	    t1->next = h2;
	  else
	    h1 = h2;
	  t1 = h2;
	  h2 = h2->next;
	} else {
	  if (h1)
	    t1->next = h3;
	  else
	    h1 = h3;
	  t1 = h3;
	  h3 = h3->next;
	}
      }
      /* append the unmerged list to t1...h1 */
      if (h2) {
	if (h1)
	  t1->next = h2;
	else
	  h1 = h2;
	t1 = t2;
      } else if (h3) {
	if (h1)
	  t1->next = h3;
	else
	  h1 = h3;
	t1 = t3;
      }
    }
    /* the source list is exhausted. prepare the next iteration. */
    *head = h1;
    *tail = t1;
    unit *= 2;
  } while (!atonce);
}

void
LineList_clear(head, tail)
LineList **head, **tail;
{
  LineList *curr, *prev;

  CHECK_HEAD(head, tail);
  curr = *head;
  while (curr) {
    prev = curr;
    curr = curr->next;
    free(prev->yomi);
    free(prev->hinshi);
    free(prev->kanji);
    free(prev);
  }
  *head = *tail = NULL;
}

/* get a word delimited by space or tab */
char *
get_token(restartp)
char **restartp;
{
  char *p = *restartp, *q, *d;

  if (p == NULL)
    return NULL;
  while (ISSPACE(*p)) ++p;
  if (!*p) {
    *restartp = NULL;
    return NULL;
  }
  q = d = p;
  for (;;) {
    if (*q == '\\') {
      ++q;
      if (*q)
	*d++ = *q++;
      else
	break;
    } else if (*q && !ISSPACE(*q)) {
      *d++ = *q++;
    } else {
      break;
    }
  }
  *restartp = (*q) ? (q + 1) : NULL;
  *d = 0;
  return p;
}

size_t
escape_ws(dst, dstlen, src)
char *dst;
size_t dstlen;
const char *src;
{
  const char *s;
  char *d = dst;
  char *const dstend = dst + dstlen;

  for (s = src; *s; ++s) {
    if (ISSPACE(*s)) {
      if (d + 1 >= dstend)
	break;
      *d++ = '\\';
    } else {
      if (d == dstend)
	break;
    }
    *d++ = *s;
  }
  if (d != dstend)
    *d = 0;
  return d - dst;
}

/* parse the dictionary and store them into *first_line...*last_line */
int
read_textdic(first_line, last_line, infile, filename)
LineList **first_line, **last_line;
FILE *infile;
const char *filename;
{
  unsigned int lineno;
  char buf[RK_LINE_BMAX*10];

  CHECK_HEAD(first_line, last_line);
  for (lineno = 1; fgets(buf, sizeof buf, infile); ++lineno) {
    LineList *l;
    char *restartp, *hinshi, *kanji, *freqp;
    size_t linelen = strlen(buf);

    if (buf[linelen - 1] == '\n') {
      buf[--linelen] = 0;
    } else if (linelen == sizeof buf - 1) {
      fprintf(stderr, "%s:%u: too long line\n", filename, lineno);
      return 1;
    }
    if (!buf[0] || ISSPACE(buf[0]) || buf[0] == '#')
      continue;
    restartp = buf;
    get_token(&restartp);
    hinshi = get_token(&restartp);
    kanji = get_token(&restartp);
    if (!hinshi || hinshi[0] != '#' || !kanji) {
      fprintf(stderr, "%s:%u: syntax error\n", filename, lineno);
      return 1;
    } else if (get_token(&restartp)) {
      fprintf(stderr, "%s:%u: multiple words at a line\n", filename, lineno);
      return 1;
    }
    if (!(l = (LineList *)malloc(sizeof(LineList)))) {
      fprintf(stderr, "out of memory\n");
      return 1;
    }
    l->next = NULL;
    l->yomi = strdup(buf);
    l->hinshi = strdup(hinshi);
    l->kanji = strdup(kanji);
    if (!l->yomi || !l->hinshi  || !l->kanji) {
      free(l->yomi);
      free(l->hinshi);
      free(l->kanji);
      free(l);
      fprintf(stderr, "out of memory\n");
      return 1;
    }
    if ((freqp = strchr(hinshi, '*')) != NULL)
      l->freq = (unsigned int)strtoul(freqp + 1, NULL, 10);
    else
      l->freq = 0;
    if (*first_line)
      (*last_line)->next = l;
    else
      *first_line = l;
    *last_line = l;
  }
  return 0;
}

size_t
euc_to_uslen(es)
unsigned char *es;
{
  Wchar buf[RK_LINE_BMAX*10];

  return euctous(es, strlen((char *)es), buf, sizeof(buf) / sizeof(Wchar)) - buf;
}

int
mergeword(first_line)
const LineList *first_line;
{
  const LineList *l, *prev;
  char buf[RK_LINE_BMAX*10];
  size_t outlen = 0;
  unsigned int kouho = 0;
  size_t wrec_size = 0;

  for (prev = NULL, l = first_line; l; prev = l, l = l->next) {
    if (!prev || strcmp(prev->yomi, l->yomi))
      goto yomi_changed;
    else if (strcmp(prev->hinshi, l->hinshi))
      goto hinshi_changed;
    else
      goto nothing_changed;
    /* rec  = klen + hinshi + kanji */
    /*        1    + 1      + ?     */
    /* wrec = ylen  + yomi  + knum  + fqoffset + rec */
    /*        1byte + ?byte + 2byte + 3byte    + rec */
yomi_changed:
    wrec_size = 1 + euc_to_uslen((unsigned char *)l->yomi) + 2 + 3;
    kouho = 0;
    outlen = escape_ws(buf, sizeof buf, l->yomi);
hinshi_changed:
    if (outlen < sizeof buf)
      buf[outlen++] = ' ';
    outlen += escape_ws(buf + outlen, sizeof buf - outlen, l->hinshi);
nothing_changed:
    wrec_size += NW_PREFIX + euc_to_uslen((unsigned char *)l->kanji);
    ++kouho;
    if (outlen < sizeof buf)
      buf[outlen++] = ' ';
    outlen += escape_ws(buf + outlen, sizeof buf - outlen, l->kanji);
    if (!l->next || strcmp(l->yomi, l->next->yomi)) {
      if(wrec_size > RK_WREC_BMAX || (kouho > RK_CAND_NMAX)) {
	fprintf(stderr, "%s: over word [%u %u]\n",
	    l->yomi, kouho, (unsigned int)wrec_size);
	return 1;
      } else if (outlen >= sizeof buf - 1) {
	/* The length of each line is limited to RK_LINE_BMAX*10 - 1
	 * including the trailing '\n'.  See _Rktopen at lib/RK/tempdic.c */
	fprintf(stderr, "too long line\n");
	return 1;
      } else {
	buf[outlen] = 0;
	puts(buf);
      }
    }
  }
  return 0;
}

int
main(argc, argv)
int argc;
char *argv[];
{
  LineList *first_line = NULL, *last_line = NULL;
  int status;
  int i;
  int opt_sort = 0;

#ifdef __EMX__
  _fsetmode(stdout, "b");
#endif

  /* get arguments */
  for (i = 1; i < argc && argv[i][0] == '-' && argv[i][1]; ++i) {
    if (!strcmp(argv[i], "-X")) {
      /* ignore for backward compatibility */
    } else if (!strcmp(argv[i], "-s")) {
      opt_sort = 1;
    } else {
      fprintf(stderr, "usage: mergeword [-s] [TEXTDIC] ...\n");
      status = 1;
      goto last;
    }
  }

  /* read all files corresponding to arguments */
  if (i == argc) {
    status = read_textdic(&first_line, &last_line, stdin, "(stdin)");
    if (status)
      goto last;
  } else {
    do {
      if (!strcmp(argv[i], "-")) {
	status = read_textdic(&first_line, &last_line, stdin, "(stdin)");
      } else {
	FILE *fp = NULL;
	if (!(fp = fopen(argv[i], "r"))) {
	  fprintf(stderr, "can't open %s\n", argv[i]);
	  status = 1;
	  goto last;
	}
	status = read_textdic(&first_line, &last_line, fp, argv[i]);
	fclose(fp);
      }
      if (status)
	goto last;
    } while (++i != argc);
  }
  if (!first_line)
    fprintf(stderr, "warning: no input\n");

  /* run the stable sort */
  if (opt_sort)
    LineList_sort(&first_line, &last_line);

  /* merge */
  status = mergeword(first_line);

  /* cleanup */
last:
  LineList_clear(&first_line, &last_line);
  return status;
}
/* vim: set sw=2 ts=8 noet: */
