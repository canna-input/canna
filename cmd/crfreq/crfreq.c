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
static char rcsid[]="@(#) 102.1 $Id: crfreq.c,v 1.5.2.2 2003/12/27 17:15:21 aida_s Exp $";
#endif

#include "RKintern.h"
#include <stdio.h>
#include <fcntl.h>

#define	create(a, b)	creat(a, b)
#define NDVAL_LEN	2
#define PG_HDR_SIZ	14

char	*program = "";

static char *
STrdup(s)
char *s;
{
  char *p = (char *)malloc(strlen(s) + 1);
  if (p) strcpy(p, s);
  return p;
}

unsigned char   fqbits[1024*1024];

int
CreateNL(fr, size1, size2)
     int	fr;
     int	size1;
     int	size2;
{
  unsigned char ll[4], *buf = (unsigned char *)0xdeadbeef; /* for gcc */
  
  size1 = size1 > 0 ? size1 : 0;
  size2 = size2 > 0 ? size2 : 0;
  if (size1) {
    if (!(buf = (unsigned char *)calloc(1, (unsigned)(5*size1)))) {
      (void)free((char *)buf);
      return(-1);
    }
  }
  l_to_bst4((unsigned) size1, ll);
  (void)write(fr, (char *)ll, 4);
  l_to_bst4((unsigned) 0, ll);
  (void)write(fr, (char *)ll, 4);
  (void)write(fr, (char *)ll, 4);
  (void)write(fr, (char *)ll, 4);
  if (size1) {
    (void)write(fr, (char *)buf, (unsigned) 5*size1);
    (void)free((char *)buf);
  }
  if (size2) {
    if (!(buf = (unsigned char *)calloc(1, (unsigned)size2))) {
      (void)free((char *)buf);
      return(-1);
    }
  }
  l_to_bst4((unsigned) size2, ll);
  (void)write(fr, (char *)ll, 4);
  l_to_bst4((unsigned) 0, ll);
  (void)write(fr, (char *)ll, 4);
  l_to_bst4((unsigned) 32, ll);
  (void)write(fr, (char *)ll, 4);
  l_to_bst4((unsigned) 0, ll);
  (void)write(fr, (char *)ll, 4);
  if (size2) {
    (void)write(fr, (char *)buf, (unsigned) size2);
    (void)free((char *)buf);
  }
  return(0);
}

static void
usage()
{
  (void)fprintf(stderr, "usage: %s <file name> [dictionary-name]\n", program);
  exit(1);
}

main(argc, argv)
  int argc;
  char *argv[];
{
  struct HD	hd;
  off_t		off, doff; 
  size_t	sz;
  unsigned char	ll[4], *buf;
  char		*flnm = (char *)0, *dmnm, freq[RK_OLD_MAX_HDRSIZ];
  char		*frqf, *frqe, freqfile[RK_OLD_MAX_HDRSIZ];
  int		bit_size, fd, fr, fqoffset, i, j, k, lk, nc, nw, vds = 0, err;
  long		fqbytes;

  freq[0] = '\0';
  dmnm = (program = argv[0]) + strlen(argv[0]);
  if (*dmnm == '/')
    *dmnm = (char)0;
  while (dmnm-- >= program)
    if (*dmnm == '/') {
      program = dmnm + 1;
    }

  if (argc < 3) {
    (void)fprintf(stderr, "usage: %s <file name> [dictionary-name]\n", program);
    exit(1);
  };
  i = 1;

  for (i = 1; i < argc; i++) {
    if (!strcmp("-div", argv[i])) {
      if (++i < argc) {
	vds = atoi(argv[i]);
	continue;
      }
    }
    if (!strcmp("-o", argv[i])) {
      if (++i < argc && !freq[0]) {
	strcpy(freq, argv[i]);
	continue;
      }
    }
    else if (i + 1 < argc &&
	     !flnm && (flnm = argv[i++]) && (dmnm = argv[i])) {
      continue;
    }
    usage();
  }
  if (!flnm)
    usage();

  if (strlen(flnm) >= RK_OLD_MAX_HDRSIZ || strlen(dmnm) >= RK_OLD_MAX_HDRSIZ ||
      (fd = open(flnm, O_RDONLY)) < 0) {
    (void)fprintf(stderr, "%s: cannot open %s\n", program, flnm);
    exit(1);
  }
#ifdef __CYGWIN32__
  setmode(fd, O_BINARY);
#endif

  for (off = 0, lk = 1, doff = 0, err = 0;
       !err && lk && _RkReadHeader(fd, &hd, off) >= 0;
       lk = strcmp(dmnm, (char *)hd.data[HD_DMNM].ptr)) {
    doff = off;
    off += hd.data[HD_SIZ].var;
    if (HD_VERSION(&hd) < 0x300702L &&
	!strncmp(".swd", (char *)(hd.data[HD_DMNM].ptr + strlen((char *)hd.data[HD_DMNM].ptr) - 4), 4)) {
      if (lseek(fd, off, 0) < 0 || read(fd, (char *)ll, 4) != 4) 
	err = 1;
      off += bst4_to_l(ll) + 4;
    }
  }
  if (lk) {
    (void)fprintf(stderr, "%s: cannot find %s in %s.\n", program, dmnm, flnm);
    exit(1);
  }

  if (!freq[0]) {
    (void)strcpy(freq, dmnm);
#ifdef WINDOWS_STYLE_FILENAME
    (void)strcpy(freq + strlen(freq) - 3, "cld");
#else
    (void)strcpy(freq + strlen(freq) - 3, "fq");
#endif
  }
  (void)strcpy(freqfile, flnm);

  frqf = freqfile;
  for (frqe = frqf + strlen(frqf); frqe >= frqf; frqe--)
      if ( *frqe == '/') break;

  if(*frqf) {
      if(frqe == frqf + strlen(frqf) -1)
         freqfile[0] = (char) 0;
      else
         *(frqe+1)= (char) 0;
  }
  (void)strcat(freqfile, freq);

  if ((fr = create(freqfile, 0666)) == -1) {
    (void)close(fd);
    (void)fprintf(stderr, "%s: cannot create freqency file %s\n", program, freqfile);
    exit(1);
  }
#ifdef __CYGWIN32__
  setmode(fr, O_BINARY);
#endif
  hd.flag[HD_CODM] = hd.flag[HD_DMNM];
  hd.data[HD_CODM].ptr = hd.data[HD_DMNM].ptr;
  hd.data[HD_DMNM].ptr = (unsigned char *)STrdup(freq);
  hd.flag[HD_DMNM] = strlen(dmnm);
  if (!(buf = _RkCreateHeader(&hd, &sz))) {
    (void)fprintf(stderr, "%s: cannot alloc work space.\n", program);
    exit(1);
  }
  free((char *)buf);
  hd.data[HD_HSZ].var = sz;
  if (!(buf = _RkCreateHeader(&hd, &sz))) {
    (void)fprintf(stderr, "%s: cannot alloc work space.\n", program);
    exit(1);
  }
  if (write(fr, (char *)buf, sz) < 0) {
    (void)close(fd);
    (void)fprintf(stderr, "%s: cannot write header to \"%s\"\n", program, freqfile);
    exit(1);
  }
  free((char *)buf);

  doff += hd.data[HD_PGOF].var;
  sz = _RkCalcUnlog2(hd.data[HD_L2P].var) + 1;
  lk = hd.data[HD_PAG].var;
    
  if (!(buf = (unsigned char *)malloc(sz))) {
    (void)fprintf(stderr, "%s: cannot alloc work space.\n", program);
    exit(1);
  }
  fqoffset = 0;
  for (i = 0; i < lk; i++) {
    (void)lseek(fd, doff, 0);
    doff += sz;
    if (read(fd, (char *)buf, sz) != sz) {
      (void)fprintf(stderr, "%s: cannot read %s.\n", program, dmnm);
      exit(1);
    }
    off = bst2_to_s(buf + 2) * (hd.data[HD_WWID].var + NDVAL_LEN) + bst2_to_s(buf + 4) * 5 + PG_HDR_SIZ;
    nw = bst2_to_s(buf + 4);
    for (j = 0; j < nw; j++) {
      nc = _RkCandNumber(buf + off);
      bit_size = _RkCalcLog2(nc + 1)+1;
      for (k = 0; k < nc; k++) {
	unsigned    w = k<<1;
	
	fqoffset = _RkPackBits(fqbits, fqoffset, bit_size, &w, 1);
      }
      off += _RkWordLength(buf + off);
    }
  }
  fqbytes = (fqoffset + 7)/8;
  (void)fprintf(stderr, "size %d bits %ld bytes\n", fqoffset, fqbytes);
  if (fqbytes >= sizeof(fqbits)) {
    (void)close(fd);
    (void)close(fr);
    (void)fprintf(stderr, "%s: insufficient internal table.\n", program);
    exit(1);
  };
  l_to_bst4(fqbytes, ll);
  (void)write(fr, (char *)ll, 4);
  (void)write(fr, (char *)fqbits, (unsigned)fqbytes);
  (void)close(fd);
  if (CreateNL(fr, (int)(hd.data[HD_CAN].var * 0.05), vds) < 0) {
    (void)fprintf(stderr, "%s: '%s' is created, but it has wrong size.\n", program, freq);
    (void)close(fr);
    exit(1);
  }
  (void)close(fr);
  exit(0);
}
