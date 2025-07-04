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

#ifndef LINT
static char rcsid[]="@(#) 102.1 $Id: crxgram.c,v 1.6 2003/03/24 04:04:25 aida_s Exp $";
#endif

/* #include	"RKintern.h" */

#include	"ccompat.h"
#include	<stdio.h>
#include	"RKindep/file.h"

#define LOGIC_HACK

#define	MAXLINE		2048		/* maximum length of a line */
#define	MAXIDENT	1013		/* must be a PRIME number */
#define	MAXSTR		(MAXIDENT*10)	/* maximum string area */
#define ROWBITS		9		/* bits of maximum row number */

struct ident {
    char		*name;
    short		rownum;
    short		colnum;
    short		numrow;
    short		hasconnect;
};
/* hash table */
static struct ident	**Row, **Column;
/* string table */
static char		Str[MAXSTR];
static char		*nextStr = Str;

#define	LTOL4(l, l4)	{\
	(l4)[0] = ((l)>>24)&255;\
	(l4)[1] = ((l)>>16)&255;\
	(l4)[2] = ((l)>>8)&255;\
	(l4)[3] = (l)&255;\
}
#define	GetGramRow(g, r) 	((g)->ng_conj + (r)*(g)->ng_rowbyte)

struct RkKxGram {
/* setuzoku jouhou */
    int			ng_row;		/* row no kazu */
    int			ng_col;		/* col no kazu */
    int			ng_rowbyte;	/* row atari no byte suu */
    unsigned char	*ng_conj;	/* setuzoku gyouretu/code table */
    unsigned char	*ng_strtab;
#ifdef LOGIC_HACK
    int			ng_numneg;
    unsigned long	*ng_neg;
#endif
} gram;

/* error handling */
static char	fileName[256];
static int	lineNum;

static void
usage(prog)
  char	*prog;
{
    (void)fprintf(stderr,
		  "%s [-f inputs]\n",
		  RkiBasename(prog));
    exit(1);
}
/*VARARGS*/
void
alert(fmt, arg)
char	*fmt;
char	*arg;
{
    char	msg[256];
    (void)sprintf(msg, fmt, arg);
    (void)fprintf(stderr, "#line %d %s: %s\n", lineNum, fileName, msg);
}
void
alerti(fmt, arg)
char	*fmt;
int	arg;
{
    char	msg[256];
    (void)sprintf(msg, fmt, arg);
    (void)fprintf(stderr, "#line %d %s: %s\n", lineNum, fileName, msg);
}
void
fatal(fmt, arg)
char	*fmt;
char	*arg;
{
    char	msg[256];
    (void)sprintf(msg, fmt, arg);
    (void)fprintf(stderr, "#line %d %s: (FATAL) %s\n", lineNum, fileName, msg);
    exit(1);
}

#define EOL	'\n'
unsigned char	*
readLine(s, len, fp)
unsigned char	*s;
unsigned	len;
FILE		*fp;
{
    int	i;
    while ( fgets((char *)s, (int)len, fp) ) {
	i = strlen((char *)s);
	if (s[i - 1] == EOL)
	    s[--i] = (unsigned char)0;
	if ( s[0] == '#' ) 	/* syncronize the line information */
	    (void)sscanf((char *)s + 1, "%d %s", &lineNum, fileName);
	else {
	    lineNum++;
	    return s;
	};
    }
    return (unsigned char *)0;
}

unsigned char	*
scanToken(s, token, maxtoken)
unsigned char	*s;
unsigned char	*token;
int		maxtoken;
{
    int		i;

/* skip the leading blanks */
    while ( *s && *s <= ' ' )
	s++;
    i = 0;
    while ( *s > ' ' )
	if ( i < maxtoken - 1 )
	    token[i++] = *s++;
    token[i] = 0;
    return s;
}

static int	probeHole;

struct ident	*
probeIdent(hid, name)
struct ident	**hid;
char		*name;
{
    char		*p;
    register unsigned	h, i, count;

/* compute the hash unsigned value */
    for ( h = 0, p = name; *p; )
	h = (h<<4) + *p++;
    h %= MAXIDENT;
    if ( !h ) h = *name;

    i = h;
    probeHole = -1;
    for ( count = MAXIDENT; count; count-- ) {
	struct ident	*id = hid[i];

	if ( id ) {	
	    if ( !strcmp(id->name, name) )
		return id;
	    i += h;
	    if ( i >= MAXIDENT )
		i -= MAXIDENT;
	}
	else  {
	    probeHole = i;
	    return (struct ident *)0;
	};
    };
    return (struct ident *)0;
}

struct ident	*
addIdent(hid, name, rownum, colnum)
struct ident	**hid;
char		*name;
int		rownum, colnum;
{
    struct ident	*id;

    if ( !probeIdent(hid, name) ) {
	if ( id = (struct ident *)malloc(sizeof(struct ident)) ) {
	    id->name = (char *)malloc((unsigned)strlen(name) + 1);
	    if ( !id->name )
		fatal("No more memory", 0);
	    /*NOTREACHED*/
	    (void)strcpy(id->name, name);
	    id->colnum = colnum;
	    id->rownum = rownum;
	    id->numrow = 0;
	    id->hasconnect = 0;
	    hid[probeHole] = id;
	    return id;
	};
	fatal("No more memory", 0);
	/*NOTREACHED*/
    };
    return (struct ident *)0;
}

void
enterIdent(fp)
  FILE	*fp;
{
    unsigned char	S[MAXLINE], *s;
    FILE		*def, *swd, *mac;
    struct ident	*cc;	/* current column */

    gram.ng_row = gram.ng_col = 0;

    def = fopen("./cnj.h", "w");
    swd = fopen("./cnj.swd", "w");
    mac = fopen("./cnj.mac", "w");
    if ( !def ) fatal("Cannot create file %s", "cnj.h");
    if ( !swd ) fatal("Cannot create file %s", "cnj.swd");
    if ( !mac ) fatal("Cannot create file %s", "cnj.mac");

    cc = (struct ident *)0;
    while ( readLine(s = S, sizeof(S), fp)) {
	unsigned char	tag[256], words[MAXLINE];
	unsigned char	cname[256], rname[256];
	unsigned char	*w;
	struct ident	*r;

 	if ( s[0] == '%' )
 	    break;
    /* parse line ( [cname] tag word ) */
	if ( s[0] > ' ' ) {
	    struct ident	*c;

	    gram.ng_col = gram.ng_row;
	    s = scanToken(s, cname, sizeof(cname));
	    c = addIdent(Column, (char *)cname, gram.ng_row, gram.ng_col);
	    if ( !c ) {
		alert("column <%s> redefined", rname);
		continue;
	    };
	    cc = c;
	};

	s = scanToken(s, tag, sizeof(tag));
	if ( !tag[0]  ) 
	    continue;
        if ( !cc ) {
	    alert("no current column", 0);
	    continue;
	};

	(void)strcpy((char *)rname, cc->name);
	if ( tag[0] != '_' ) 
	    (void)strcat((char *)rname, (char *)tag);
	r = addIdent(Row, (char *)rname, gram.ng_row, cc->colnum);
	if ( !r ) {
	    alert("row <%s> redefined", rname);
	    continue;
	};
	cc->numrow++;
        gram.ng_row++;

    /* set the string table */
	if ( nextStr + strlen((char *)rname) + 1 >= &Str[MAXSTR] ) 
	    fatal("string table overflow at <%s>", (char *)rname);
	(void)strcpy(nextStr, (char *)rname);
	nextStr += strlen((char *)rname) + 1;
    /* */
	(void)fprintf(mac, "#define\t%s\t%d\n",
		r->name, r->rownum);
	(void)fprintf(def, "#define\tP_%s\t%3d\n",
		r->name, r->rownum);
	(void)fprintf(def, "#define\tR_%s\t%3d\n",
		r->name, r->rownum);

/* words wo dasu */
	s = scanToken(s, words, sizeof(words));
	if ( words[0] == '_' )
	    continue;
	w = words;
	while ( *w ) {
	    unsigned char	word[256], *p;
	    
	    p = word;
	    while ( *w ) {
		*p++ = *w++;
		if ( p[-1] == '/' ) {
		    p[-1] = 0;
		    break;
		};
	    };
	    *p = 0;
	    /* 互換性のため rownum を２回書く */
	    (void)fprintf(swd, "%s\t#%d#%d\t@\n", word, r->rownum, r->rownum);
	    (void)strcpy((char *)word, (char *)w);
	};
    };
    (void)fclose(def);
    (void)fclose(swd);
    (void)fclose(mac);

    gram.ng_rowbyte = (gram.ng_row + 7)/8;
    gram.ng_conj = (unsigned char *)calloc(gram.ng_rowbyte, gram.ng_row);
    if ( !gram.ng_conj )
       fatal("No more memory", 0);
}

void
setVector(bits, s, op)
unsigned char	*bits;
unsigned char	*s;
int		op;
{
    for (;;) {
	unsigned char	name[256];
	int		i;
	struct ident	*r;
	struct ident	*c;

	s = scanToken(s, name, sizeof(name));
	if ( !name[0] )
	    return;
	if ( name[0]=='@' ) {	/* name と同じ品詞情報 */
	    if ( r = probeIdent(Row, (char *)name + 1) ) {
 		unsigned char	*rbits = GetGramRow(&gram, r->rownum);

		if ( r->hasconnect == 0 )
		    alert("Undefined row vector is referred: %s", name);
		else {
		    switch(op) {
		    case '+':
			for ( i = 0; i < gram.ng_rowbyte; i++)
			    bits[i] |= rbits[i];
			break;
		    case '-':
			for ( i = 0; i < gram.ng_rowbyte; i++)
			    bits[i] &= ~rbits[i];
			break;
		    };
		};
	    }
	    else 
		alert("unknown row %s", name);
	}
	else {
	    if ( c = probeIdent(Column, (char *)name) ) 
	      for (i = 0 ; i < c->numrow ; i++) {
		int n = c->colnum + i;

		switch(op) {
		case '+':
		    bits[n/8] |= (0x80>>(n%8));
		    break;
		case '-':
		    bits[n/8] &= ~(0x80>>(n%8));
		    break;
		}
	      }
#ifndef LOGIC_HACK
	    else 
		alert("unknown column %s", name);
#else
	    else if ( r = probeIdent(Row, (char *)name) ) {
	      int n = r->rownum;

	      switch(op) {
	      case '+':
		  bits[n/8] |= (0x80>>(n%8));
		  break;
	      case '-':
		  bits[n/8] &= ~(0x80>>(n%8));
		  break;
	      }
	    }
	    else
	      alert("unknown row/column %s", name);
#endif /* not LOGIC_HACK */
	};
    };
}

void
enterMatrix(fp)
  FILE	*fp;
{
    unsigned char	S[MAXLINE], *s;
    struct ident	*r;
    int			i, n;

/* try connection */
    while ( readLine(s = S, sizeof(S), fp) ) {
	char	row[256];
	int	op;

#ifdef LOGIC_HACK
 	if ( s[0] == '%' )
 	    break;
#endif
	if ( s[0] <= ' ' )
	    continue;
	s = scanToken(S, (unsigned char *)row, sizeof(row));
	op = row[strlen(row) - 1];
	row[strlen(row) - 1] = 0;
	if ( r = probeIdent(Row, row) ) {
	    switch(op) {
	    case	'+':	/* col... */
		r->hasconnect++;
	    case	'-':	/* col... */
		setVector(GetGramRow(&gram, r->rownum), s, op);
		break;
	    default:
		alerti("unknown operation %c", op);
		break;
	    };
	}
	else 
	    alert("unknown row ? %s", row);
    };
/* emit the message */
    for ( i = 0, n = 0; i < MAXIDENT; i++ ) {
      if ( (r = Row[i]) && r->hasconnect == 0 ) {
	if (!n) {
	  (void)fprintf(stderr, "Undefined row vectors:");
	}
	n++;
	(void)fprintf(stderr, " %s", r->name);
      }
    }
    if (n) {
      (void)fprintf(stderr, "\n");
    }
    for (i = 0, n = 0 ; i < MAXIDENT ; i++) {
      if (Column[i]) {
	n++;
      }
    }
    (void)fprintf(stderr, "rows %d cols %d\n", gram.ng_row, n);
}

#ifdef LOGIC_HACK
int
ulongcomp(i, j)
  unsigned long *i, *j;
{
    return *i - *j;
}

#define SEP "/"
#define ROWMASK ((1 << ROWBITS) - 1)
#define	TestGram(cnj, col)	((cnj) && ((cnj)[((col)>>3)]&(0x80>>(col&7))))

void
enterNeg(fp)
  FILE	*fp;
{
    unsigned char S[MAXLINE], *nextS;
    int i;
    unsigned long *negvec = NULL;
    int vecsize = 0, numneg = 0;
    unsigned long neg;

    while (readLine(nextS = S, sizeof(S), fp)) {
	short rdata[3][256];
	char opname[256];
	unsigned long op = 0;
	int l0, l1, l2;
	unsigned char *cj0, *cj1;

	if (nextS[0] <= ' ')
	    continue;

	for (i = 0; i < 3; i++) {
	    unsigned char namevec[256];
	    char *name;
	    struct ident *r;
	    int j;

	    nextS = scanToken(nextS, namevec, sizeof(namevec));
	    name = strtok(namevec, SEP);
	    for (j = 0; name && j < 255;) {
		if (r = probeIdent(Row, name))
		    rdata[i][j++] = r->rownum;
		else if (r = probeIdent(Column, name)) {
		    int offset;
		    if (j + r->numrow >= 255)
			break;
		    for (offset = 0 ; offset < r->numrow ; offset++) {
			short rownum = r->colnum + offset;
			rdata[i][j++] = rownum;
		    }
		} else
		    alert("unknown row/column ? %s", name);
		name = strtok(NULL, SEP);
	    }
	    rdata[i][j] = -1;
	}
	nextS = scanToken(nextS, opname, sizeof(opname));
	switch (opname[0])
	{
	case '-':
	    op = 0; break;
	case '+':
	    op = 1; break;
	default:
	    alert("unknown operation %c", opname);
	    continue;
	}

	for (l0 = 0; rdata[0][l0] >= 0; l0++) {
	  cj0 = GetGramRow(&gram, rdata[0][l0]);
	  neg = (unsigned long)rdata[0][l0] << ROWBITS * 2;
	  for (l1 = 0; rdata[1][l1] >= 0; l1++) {
	    if (!TestGram(cj0, rdata[1][l1]))
		continue;
	    cj1 = GetGramRow(&gram, rdata[1][l1]);
	    neg &= ~(ROWMASK << ROWBITS);
	    neg |= (unsigned long)rdata[1][l1] << ROWBITS;
	    for (l2 = 0; rdata[2][l2] >= 0; l2++) {
		if (!TestGram(cj1, rdata[2][l2]))
		    continue;
		neg &= ~ROWMASK;
		neg |= (unsigned long)rdata[2][l2];
		if (numneg >= vecsize) {
		    vecsize += 20;
		    negvec = (unsigned long *)
			((negvec) /* for non-ISO environments */
			 ? realloc(negvec, vecsize * sizeof(unsigned long))
			 : malloc(vecsize * sizeof(unsigned long)));
		    if (!negvec)
			fatal("No more memory", 0);
		}
		negvec[numneg++] = neg << 1 | op;
	    }
	  }
	}
    }
    qsort((char *)negvec, numneg, sizeof(unsigned long), ulongcomp);
    gram.ng_neg = negvec;
    gram.ng_numneg = numneg;

    (void)fprintf(stderr, "neg %d\n", gram.ng_numneg);
}
#endif /* LOGIC_HACK */

main (argc, argv)
  int argc;
  char *argv [];
{
    FILE		*cnj, *fp = (FILE *)0;
    unsigned long	pair;
    unsigned long	size;
    unsigned char	l4[4];
    int			i;
    unsigned char	*file = (unsigned char *)0;
#ifdef LOGIC_HACK
    unsigned long	*neg;
#endif

    if ( argc < 2 ) {
      usage( argv[0] );
    }
    
    for (i = 1; i < argc; i++) {
	if (!strcmp("-f", argv[i])) {
	    if ((++i < argc) && !file) {
		file = (unsigned char *)argv[i];
		continue;
	    }
	}
	(void)usage(argv[0]);
	/*NOTREACHED*/
    }
    if (file) {
	if (!(fp = fopen((char *)file, "r")))
	    fatal("cannot open %s\n", (char *)file);
    }
    if (!fp)
	fp = stdin;
    Row    = (struct ident **)calloc(sizeof(struct ident *), MAXIDENT);
    Column = (struct ident **)calloc(sizeof(struct ident *), MAXIDENT);
    if ( !Row || !Column )
       fatal("No more memory", 0);

    enterIdent(fp);
    enterMatrix(fp);
#ifdef LOGIC_HACK
    enterNeg(fp);
#endif
    (void)fclose(fp);
/* write out the conjunction file */
#ifdef __CYGWIN32__
    cnj = fopen("cnj.bits", "wb");
#else
    cnj = fopen("cnj.bits", "w");
#endif
    if ( !cnj )
       fatal("Cannot create file %s", "cnj.bits");
/* size */
    size = 4 + gram.ng_row * gram.ng_rowbyte + (nextStr - Str);
    LTOL4(size, l4);
    (void)fwrite((char *)l4, 1, 4, cnj);
/* row/col */
    pair = gram.ng_row;
    LTOL4(pair, l4);
    (void)fwrite((char *)l4, 1, 4, cnj);
/* conjunction matrix */
    (void)fwrite((char *)gram.ng_conj, gram.ng_rowbyte, gram.ng_row, cnj);
/* string table */
    (void)fwrite((char *)Str, nextStr - Str, 1, cnj);
#ifdef LOGIC_HACK
/* negative conjunction */
    size = gram.ng_numneg;
    LTOL4(size, l4);
    (void)fwrite((char *)l4, 1, 4, cnj);
    for (i = 0, neg = gram.ng_neg; i < gram.ng_numneg; i++, neg++) {
	LTOL4(*neg, l4);
	(void)fwrite((char *)l4, 1, 4, cnj);
    }
#endif
/* オプション用領域
 *	とりあえず、1byteに 0 を書いておく
 *
 */
	/* 誰がやったこれ !? */
    {
	static char	p = 0;
	(void)fwrite(&p, 1, 1, cnj);
    };
    (void)fclose(cnj);
    exit(0);
}
