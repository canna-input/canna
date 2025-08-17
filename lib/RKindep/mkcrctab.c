#include "cannaconf.h"
#include "ccompat.h"
#include <stdio.h>

RCSID("$Id: mkcrctab.c,v 1.2 2004/11/21 06:37:40 aida_s Exp $");

int
main(argc, argv)
int argc;
char *argv[];
{
  canna_uint32_t i, r;
  unsigned int j;
  static canna_uint32_t table[256];

  for (i = 0; i < 256; ++i) {
    r = i << 24;
    for (j = 0; j < 8; ++j) {
      if (r & (1UL << 31))
	r = (r << 1) ^ 0x04c11db7UL;
      else
	r <<= 1;
    }
    table[i] = r;
  }
  for (j = 0; j < 64; ++j)
    printf("  0x%08x, 0x%08x, 0x%08x, 0x%08x,\n",
	table[j*4], table[j*4+1], table[j*4+2], table[j*4+3]);
  return 0;
}

/* vim: set sw=2: */
