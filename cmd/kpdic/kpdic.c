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

#ifndef lint
static char rcsid[]="@(#) 102.1 $Id: kpdic.c,v 1.4.2.2 2003/12/27 17:15:23 aida_s Exp $";
#endif

#if defined(__STDC__) || defined(SVR4)
#include <locale.h>
#endif

#ifdef SVR4
extern char *gettxt();
#else
#define	gettxt(x,y)  (y)
#endif

#include "ccompat.h"

#ifdef __CYGWIN32__
#include <fcntl.h> /* for O_BINARY */
#endif

#include	<stdio.h>
#include	<ctype.h>

#define		MAXKEY	((1 << 16) / 4)
#define		MAXSIZE	(1 << 16)

#define		LMAXKEY		(1 << 16)
#define		LMAXSIZE	(1 << 20)

#define LOMASK(x)	((x)&255)

static char	fileName[256];
static int	lineNum;
static int	errCount;
int chk_dflt pro((int c));

	struct  def_tbl {
	    int   used  ;
	    char  *roma ; 
	    char  *kana ;
	    char  *intr ;
        }    ;
	static struct def_tbl def [] = { 
	    {0,"kk","っ","k"},
	    {0,"ss","っ","s"},
	    {0,"tt","っ","t"},
	    {0,"hh","っ","h"},
	    {0,"mm","っ","m"},
	    {0,"yy","っ","y"},
	    {0,"rr","っ","r"},
	    {0,"ww","っ","w"},
	    {0,"gg","っ","g"},
	    {0,"zz","っ","z"},
	    {0,"dd","っ","d"},
	    {0,"bb","っ","b"},
	    {0,"pp","っ","p"},
	    {0,"cc","っ","c"},
	    {0,"ff","っ","f"},
	    {0,"jj","っ","j"},
	    {0,"qq","っ","q"},
	    {0,"vv","っ","v"}
	}  ; 


/*VARARGS*/
void
alert(fmt, arg)
char	*fmt;
char	*arg;
{
    char	msg[256];
    (void)sprintf(msg, fmt, arg);
    (void)fprintf(stderr, gettxt("cannacmd:23", 
		 "#line %d %s: (WARNING) %s\n"), lineNum, fileName, msg);
    ++errCount;
}
void
fatal(fmt, arg)
char	*fmt;
char	*arg;
{
    char	msg[256];
    (void)sprintf(msg, fmt, arg);
    (void)fprintf(stderr, gettxt("cannacmd:24", 
		 "#line %d %s: (FATAL) %s\n"), lineNum, fileName, msg);
    exit(1);
}

int
getWORD(s, news, word, maxword)
unsigned char	*s, **news;
unsigned char	*word;
int		maxword;
{
    unsigned 	c;
    int	 	i;

    i = 0;
    while ( *s && *s <= ' ' )
	s++;
    while ( (c = *s) > ' ' ) {
	s++;
	if ( c == '\\' ) {
	    switch(*s) {
	    case 0:
		break;
	    case '0':
		if ( s[1] == 'x' && isxdigit(s[2]) && isxdigit(s[3]) ) {
		    unsigned char   xx[3];

		    s += 2;
		    xx[0] = *s++; xx[1] = *s++; xx[2] = 0;
		    sscanf((char *)xx, "%x", &c);
		}
		else {
		    c = 0;
		    while ( isdigit(*s) ) 
			c = 8*c + (*s++ - '0');
		};
		break;
	    case 'x':
		{
		    unsigned char   xx[3];
		    unsigned char   *xxp = xx;
		    s++;
		    if ( isxdigit(*s) )
			*xxp++ = *s++;
		    if ( isxdigit(*s) )
			*xxp++ = *s++;
		    *xxp = '\0';
		    sscanf((char *)xx, "%x", &c);
		}
		break;
	    default:
		c = *s++;
		break;
	    };
	};
	if ( i < maxword - 1 )
	    word[i++] = c;
    };
    word[i] = 0;
    *news = s;
    return i;
}

unsigned char
*allocs  (s)
unsigned char	*s;
{
    unsigned char	*d;

    if ( (d = (unsigned char *)malloc(strlen((char *)s) + 1)) != NULL )
	 strcpy((char *)d, (char *)s);
    else {
	fprintf(stderr, "Out of memory\n");
	exit(1);
    }
    return d;
}

struct roman { 
    unsigned char	*roma;
    unsigned char	*kana;
    unsigned char	*temp;
    int                 bang;
};

static void
freeallocs(roman, nKey)
struct roman *roman;
int nKey;
{
  int i;

  for (i = 0 ; i < nKey ; i++) {
    /* free them */
    free((char *)roman[i].roma); roman[i].roma = (unsigned char *)0;
    free((char *)roman[i].kana); roman[i].kana = (unsigned char *)0;
    if (roman[i].temp) {
      free((char *)roman[i].temp); roman[i].temp = (unsigned char *)0;
    }
  }
}

int
compar(p, q)
struct roman	*p, *q;
{	
    unsigned char	*s = p->roma;
    unsigned char	*t = q->roma;

    while ( *s == *t )
	if ( *s )
	    s++, t++;
	else
	    return 0;
    return ((int)*s) - ((int)*t);
}

main(argc, argv)
  int    argc ;
  char **argv ; 
{
  struct roman *roman;
  unsigned char	rule[256], *r;
  int			nKey, size;
  int			i, p;
  int                   flag_old ;
  int                   flag_large = 0;
  int                   werr ;
  long maxkey, maxsize;
  unsigned char	l4[4], *bangchars = 0, *pp;

#if defined(__STDC__)  || defined(SVR4)
    (void)setlocale(LC_ALL,"");
#endif
#ifdef __EMX__
    _fsetmode(stdout, "b");
#endif
#ifdef __CYGWIN32__
    setmode(fileno(stdout), O_BINARY);
#endif

/* option */
    flag_old =  0 ; 
    werr = 0 ; 
    while(--argc) {
    	argv++ ;
        if (!strcmp(*argv,"-m")) {
    		flag_old = 1 ; 
        }
        else if (!strcmp(*argv,"-x")) {
    		flag_large = 1 ; 
        }
    }

  if (flag_large) {
    maxkey = LMAXKEY;
    maxsize = LMAXSIZE;
  }
  else {
    maxkey = MAXKEY;
    maxsize = MAXSIZE;
  }

  roman = (struct roman *)malloc(sizeof(struct roman) * maxkey);
  if (!roman) {
    fatal(gettxt("cannacmd:8", "No more memory\n"), 0); 
  }

  nKey = 0;
  size  = 0;
  while (fgets((char *)(r = rule), sizeof(rule), stdin)) {
    unsigned char	roma[256];
  
    lineNum++;
    if ( *r == '#' ) {
      continue;
    }
    if ( getWORD(r, &r, roma, sizeof(roma)) ) {
      if (nKey < maxkey) {
	for ( i = 0; i < nKey; i++ ) {
	  if ( !strcmp((char *)roman[i].roma, (char *)roma) ) {
	    break;
	  }
	}
	if ( i < nKey ) {
	  alert(gettxt("cannacmd:25", "multiply defined key <%s>"), roma);
	  continue;
	}
	roman[nKey].roma = allocs(roma);
      }
      else {
	  freeallocs(roman, nKey);
	  free((char *)roman);
	  fatal(gettxt("cannacmd:26", 
	       "More than %d romaji rules are given."), maxkey);
      }
      if ( getWORD(r, &r, roma, sizeof(roma)) ) {
	roman[nKey].kana = allocs(roma);
	roman[nKey].temp = (unsigned char *)0;
	roman[nKey].bang = 0;
	if ( getWORD(r, &r, roma, sizeof(roma)) ) {
	  roman[nKey].temp = allocs(roma);
	  if ( getWORD(r, &r, roma, sizeof(roma)) ) {
	    roman[nKey].bang = 1;
	  }
	}
        size += strlen((char *)roman[nKey].roma) + 1 +
	        strlen((char *)roman[nKey].kana) + 1 +
	        (roman[nKey].temp ? strlen((char *)roman[nKey].temp) : 0) + 1;

/*  add  */
	if (flag_old == 1) {
	  if (roman[nKey].temp && 0) {
	    /* free them */
	    free((char *)roman[nKey].roma);
	    free((char *)roman[nKey].kana);
	    free((char *)roman[nKey].temp);
	    roman[nKey].roma = (unsigned char *)0;
	    roman[nKey].kana = (unsigned char *)0;
	    roman[nKey].temp = (unsigned char *)0;
	    nKey--; /* ひとつ戻しておく */

	    werr = 1;
	  }
	  else {
	    p = chk_dflt((int)(unsigned char)roman[nKey].roma[0]);
	    if (p--) {
	      if (def[p].used == 0) { /* if not used */
		if (nKey < maxkey) {
		  nKey++ ; 
		  roman[nKey].roma = allocs(def[p].roma);
		  roman[nKey].kana = allocs(def[p].kana);
		  roman[nKey].temp = allocs(def[p].intr);
		  size += strlen((char *)roman[nKey].roma) + 1
		        + strlen((char *)roman[nKey].kana) + 1
		        + strlen((char *)roman[nKey].temp) + 1;
		  def[p].used = 1;
		}
		else {
		  freeallocs(roman, maxkey);
		  free((char *)roman);
		  fatal("more than %d romaji rules are given.", maxkey);
		}
	      }
	    }
	  }
	}

	nKey++;
      }
      else {
	if (roman[nKey].roma &&
	    roman[nKey].roma[0] == '!' &&
	    roman[nKey].roma[1] != (unsigned char)0) {
	  if (bangchars) {
	    free((char *)bangchars);
	  }
	  bangchars = allocs(roman[nKey].roma + 1);
	}
	else {
	  alert(gettxt("cannacmd:28", "syntax error"), 0);
	}
	if (roman[nKey].roma) {
	  free(roman[nKey].roma);
	  roman[nKey].roma = (unsigned char *)0;
	}
      }
    }
  }

  if ( errCount ) {
    freeallocs(roman, nKey);
    free((char *)roman);
    fatal(gettxt("cannacmd:29", "Romaji dictionary is not produced."), 0);
  }
  qsort((char *)roman, nKey, sizeof(struct roman), 
        (int (*) pro((const void *, const void *)))compar);
  if (!flag_large) {
    putchar('K'); putchar('P');
  }
  else {
    putchar('P'); putchar('T');
  }
  size += (bangchars ? strlen((char *)bangchars) : 0) + 1;

  if (size >= maxsize) {
    freeallocs(roman, nKey);
    free((char *)roman);
    fatal(gettxt("cannacmd:32", "Too much rules.  Size exhausted."), 0);
  }

  if (!flag_large) {
    l4[0] = LOMASK(size >> 8); l4[1] = LOMASK(size);
    l4[2] = LOMASK(nKey >> 8); l4[3] = LOMASK(nKey);
    putchar(l4[0]); putchar(l4[1]); putchar(l4[2]); putchar(l4[3]);
  }
  else {
    l4[0] = LOMASK(size >> 24); l4[1] = LOMASK(size >> 16);
    l4[2] = LOMASK(size >> 8); l4[3] = LOMASK(size);
    putchar(l4[0]); putchar(l4[1]); putchar(l4[2]); putchar(l4[3]);
    l4[0] = LOMASK(nKey >> 24); l4[1] = LOMASK(nKey >> 16);
    l4[2] = LOMASK(nKey >> 8); l4[3] = LOMASK(nKey);
    putchar(l4[0]); putchar(l4[1]); putchar(l4[2]); putchar(l4[3]);
  }

  if (bangchars) {
    for (pp = bangchars ; pp && *pp ; pp++) {
      putchar(*pp);
    }
    free((char *)bangchars);
  }
  putchar('\0');

  for ( i = 0; i < nKey; i++ ) {
    r = roman[i].roma; do { putchar(*r); } while (*r++);
    r = roman[i].kana; do { putchar(*r); } while (*r++);
    if (roman[i].temp) {
      r = roman[i].temp; while (*r) putchar(*r++);
    }
    putchar(roman[i].bang); /* temp がなくて、bang が１はありえない */
  };
  freeallocs(roman, nKey);
  free((char *)roman);
  fprintf(stderr, gettxt("cannacmd:30", "SIZE %d KEYS %d\n"), size, nKey);
  if (werr == 1 ) 
    fprintf(stderr,gettxt("cannacmd:31",
	  "warning: Option -m is specified for new dictionary format.\n")) ;
  exit(0);
}

/* sub */
int
chk_dflt(c) int c ; {
    int  i,n ; 
    char cc = (char)c;
    n = sizeof(def) / sizeof(struct def_tbl) ; 
    for (i=0; i < n ; i++) {
	if (cc == def[i].intr[0]) {
	    return(i+1) ;
	}
    }
    return(0);
}


