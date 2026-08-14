/*
 *  printf.c,v 1.3 2001/06/14 18:16:07 ura Exp
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
#include <string.h>
#include <stdarg.h>
#include "commonhd.h"
#include "sdefine.h"
#include "sheader.h"

extern int cursor_colum;

static int
char_q_len(w_char x)
{
  return ((*char_q_len_func) (x));
}

static int
VFPRINTF(FILE *file, const char *format, va_list ap)
{
  char buf2[512];
  int r;

  r = vsnprintf(buf2, sizeof buf2, format, ap);
  cursor_colum += eu_columlen ((unsigned char *)buf2);
  puteustring (buf2, file);
  return r;
}

int
FPRINTF(FILE *file, const char *format, ...)
{
  va_list ap;
  int r;

  va_start(ap, format);
  r = VFPRINTF(file, format, ap);
  va_end(ap);
  return r;
}

int
PRINTF(const char *format, ...)
{
  va_list ap;
  int r;

  va_start(ap, format);
  r = VFPRINTF(stdout, format, ap);
  va_end(ap);
  return r;
}

void
puteustring(char *buf2, FILE *file)
{
  unsigned char buf[512];
  int len;
  unsigned char *c;

  len = (*code_trans[(file_code << 2) | tty_c_flag]) (buf, (unsigned char *)buf2, strlen (buf2) + 1);
  for (c = buf, len--; len > 0; len--, c++)
    {
      putc (*c, file);
    }
}

#define W_BUFLEN 32
static w_char w_buf[W_BUFLEN];
static int w_maxbuf = 0;

int
w_putchar(w_char w)
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
putchar_norm(int c)
{
  push_hrus ();
  putchar1 (c);
  pop_hrus ();
}

void
putchar1(int c)
{
  putchar (c);
  flush ();
  cursor_colum += 1;
}

void
flushw_buf(void)
{
  unsigned char *c;
  int len;

  static unsigned char buf[W_BUFLEN * 8];
  len = (*code_trans[(internal_code << 2) | tty_c_flag]) (buf, (unsigned char *)w_buf, sizeof (w_char) * w_maxbuf);
  for (c = buf; len > 0; len--, c++)
    {
      putchar (*c);
    }
  w_maxbuf = 0;
  flush ();
}

void
errorkeyin(void)
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
