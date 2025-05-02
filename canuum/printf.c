/*
 *  printf.c,v 1.3 2001/06/14 18:16:07 ura Exp
 *  Canna: $Id: printf.c,v 1.3.2.1 2003/01/11 17:53:57 aida_s Exp $
 */

/*
 * FreeWnn is a network-extensible Kana-to-Kanji conversion system.
 * This file is part of FreeWnn.
 * 
 * Copyright Kyoto University Research Institute for Mathematical Sciences
 *                 1987, 1988, 1989, 1990, 1991, 1992
 * Copyright OMRON Corporation. 1987, 1988, 1989, 1990, 1991, 1992, 1999
 * Copyright ASTEC, Inc. 1987, 1988, 1989, 1990, 1991, 1992
 * Copyright FreeWnn Project 1999, 2000
 *
 * Maintainer:  FreeWnn Project   <freewnn@tomo.gr.jp>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include "commonhd.h"
#include "sdefine.h"
#include "sheader.h"

extern int cursor_colum;

int
char_q_len (x)
     w_char x;
{
  return ((*char_q_len_func) (x));
}

void
fprintf (file, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13)
     FILE *file;
     char *x2, *x3, *x4, *x5, *x6, *x7, *x8, *x9, *x10, *x11, *x12, *x13;
{
  char buf2[512];

  sprintf (buf2, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13);
  cursor_colum += eu_columlen (buf2);

  puteustring (buf2, file);
}

void
puteustring (buf2, file)
     char *buf2;
     FILE *file;
{
  char buf[512];
  register int len;
  register char *c;

  len = (*code_trans[(file_code << 2) | tty_c_flag]) (buf, buf2, strlen (buf2) + 1);
  for (c = buf, len--; len > 0; len--, c++)
    {
      putc (*c, file);
    }
}

void
printf (format, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13)
     char *format;
     char *x3, *x4, *x5, *x6, *x7, *x8, *x9, *x10, *x11, *x12, *x13;
{
  fprintf (stdout, format, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13);

}

#define W_BUFLEN 32
static w_char w_buf[W_BUFLEN];
static int w_maxbuf = 0;

int
w_putchar (w)
     w_char w;
{
  w_char wch = w;
  w_char tmp_wch[10];
  int len, i, c_len = 0;
  int ret_col = 0;

  wnn_delete_w_ss2 (&wch, 1);
  if (ESCAPE_CHAR (wch))
    {
      ret_col = char_q_len (wch);
      w_buf[w_maxbuf++] = (w_char) ('^');
      if (wch == 0x7f)
        w_buf[w_maxbuf++] = (w_char) ('?');
      else
        w_buf[w_maxbuf++] = (w_char) (wch + 'A' - 1);
    }
  else
    {
      if (print_out_func)
        {
          len = (*print_out_func) (tmp_wch, &wch, 1);
          wnn_delete_w_ss2 (tmp_wch, len);
          for (i = 0; i < len; i++)
            {
              w_buf[w_maxbuf++] = tmp_wch[i];
              c_len = char_q_len (tmp_wch[i]);
              ret_col += c_len;
            }
        }
      else
        {
          ret_col = char_q_len (wch);
          w_buf[w_maxbuf++] = wch;
        }
    }
  cursor_colum += ret_col;
  if (w_maxbuf >= W_BUFLEN - 2)
    {
      flushw_buf ();
    }
  return (ret_col);
}

void
putchar_norm (c)
     int c;
{
  push_hrus ();
  putchar1 (c);
  pop_hrus ();
}

void
putchar1 (c)
     int c;
{
  putchar (c);
  flush ();
  cursor_colum += 1;
}

void
flushw_buf ()
{
  register char *c;
  register int len;

  static char buf[W_BUFLEN * 8];
  len = (*code_trans[(internal_code << 2) | tty_c_flag]) (buf, w_buf, sizeof (w_char) * w_maxbuf);
  for (c = buf; len > 0; len--, c++)
    {
      putchar (*c);
    }
  w_maxbuf = 0;
  flush ();
}

extern char *wnn_perror ();
void
errorkeyin ()
{
  push_cursor ();
  throw_c (0);
  clr_line ();
  printf (wnn_perror ());
  printf (MSG_GET (8));
  /*
     printf(" (Ç¡²¿)");
   */
  flush ();
  keyin ();
  pop_cursor ();
}
