/*
 * Copyright (C) 2002 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * Written by Yukihiro Nakai <ynakai@redhat.com>
 */

#include "ccompat.h"
#include <stdio.h>
#include "RKindep/file.h"
#include "RKindep/strops.h"

RCSID("$Id: zen2han.c,v 1.1 2004/08/06 14:09:54 aida_s Exp $");

char* zen2han[][2] = {
  {"\241\335", "-"}, /* ¡Ý */
  {"\241\312", "("}, /* ¡Ê */
  {"\241\313", ")"}, /* ¡Ë */
  {"\243\260", "0"}, /* £° */
  {"\243\261", "1"}, /* £± */
  {"\243\262", "2"}, /* £² */
  {"\243\263", "3"}, /* £³ */
  {"\243\264", "4"}, /* £´ */
  {"\243\265", "5"}, /* £µ */
  {"\243\266", "6"}, /* £¶ */
  {"\243\267", "7"}, /* £· */
  {"\243\270", "8"}, /* £¸ */
  {"\243\271", "9"}  /* £¹ */
};

static size_t eucjplen(const unsigned char *p) {
  if (p[0] == 0)
    return 0;
  else if (!(p[0] & 0x80))
    return 1;
  else if (p[0] == 0x8f) /* SS3 */
    return ((p[1]&0x80) && (p[2]&0x80)) ? 3 : -1;
  else
    return (p[1]&0x80) ? 2 : -1;
}

int main(int argc, char** argv) {
  FILE* fp;
  int exitval = 0;
  unsigned char *buf = NULL;
  RkiStrbuf outbuf;

  if( argc != 2 ) {
    fprintf(stderr, "Usage: zen2han file \n");
    return 1;
  }
  fp = fopen(argv[1], "r");
  if (fp == NULL) {
    fprintf(stderr, "Cannot open %s\n", argv[1]);
    return 1;
  }
  RkiStrbuf_init(&outbuf);

  for(;;) {
    int idx = 0;

    free(buf);
    buf = (unsigned char *)RkiGetLine(fp);
    if (!buf) break;

    while(strlen(buf+idx)>0 ) {
      int ret = eucjplen(buf+idx);
      if( ret <= 0 ) {
        fprintf(stderr, "Illegal sequence found.\n");
	exitval = 1;
	goto last;
      }
      if( ret > 1 ) {
        int i;
        int flag = 0;
        for( i=0;i<sizeof(zen2han)/sizeof(zen2han[0]);i++) {
          if( strncmp(zen2han[i][0], buf+idx, ret) == 0 ) {
            if (RkiStrbuf_add(&outbuf, zen2han[i][1]))
	      goto nomem;
            flag = 1;
            break;
          }
        }
        if( *(buf+idx) == 0xa3 && *(buf+idx+1) >= 0xc1
            && *(buf+idx+1) <= 0xda ) {
          char c = *(buf+idx+1) - 0xc1 + 'A';
	  if (RKI_STRBUF_ADDCH(&outbuf, c))
	    goto nomem;
          flag = 1;
        } else if( *(buf+idx) == 0xa3 && *(buf+idx+1) >= 0xe1
            && *(buf+idx+1) <= 0xfa ) {
          char c = *(buf+idx+1) - 0xfa + 'a';
	  if (RKI_STRBUF_ADDCH(&outbuf, c))
	    goto nomem;
          flag = 1;
        }
        if( flag == 0 ) {
	  if (RkiStrbuf_addmem(&outbuf, buf+idx, ret))
	    goto nomem;
        }
      } else {
	if (RKI_STRBUF_ADDCH(&outbuf, *(buf+idx)))
	  goto nomem;
      }
      idx += ret;
    }
    RkiStrbuf_term(&outbuf);
    printf("%s", outbuf.sb_buf);
    RkiStrbuf_clear(&outbuf);
  }
  goto last;
nomem:
  fprintf(stderr, "Out of memory\n");
  exitval = 1;
last:
  RkiStrbuf_destroy(&outbuf);
  free(buf);
  if (fp)
    fclose(fp);
  
  return exitval;
}

/* vim: set sw=2 ts=8: */
