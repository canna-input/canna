/*
 *  cursor.c,v 1.3 2001/06/14 18:16:06 ura Exp
 *  Canna: $Id: cursor.c,v 1.2.2.1 2003/01/11 17:53:57 aida_s Exp $
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
#include <ctype.h>
#include <sys/errno.h>
#include "commonhd.h"

#include "sdefine.h"
#include "sheader.h"

int cursor_colum = 0;
static int cursor_upscreen = 1;
static int cursor_reverse = 0;
static int cursor_underline = 0;
static int cursor_bold = 0;
int uum_cursor_invisible = 0;

void
throw_col (col)
     int col;
{
  if (cursor_upscreen)
    {
      kk_save_cursor ();
    }
  throw_cur_raw (col % maxlength, crow + (col / maxlength));
  cursor_colum = col;
}

void
h_r_on ()
{
  if (!cursor_reverse)
    {
      h_r_on_raw ();
      cursor_reverse = 1;
    }
}

void
h_r_off ()
{
  if (cursor_reverse)
    {
      h_r_off_raw ();
      cursor_reverse = 0;
    }
  if (cursor_bold)
    {
      b_s_on_raw ();
    }
}

void
u_s_on ()
{
  if (!cursor_underline)
    {
      u_s_on_raw ();
      cursor_underline = 1;
    }
}

void
u_s_off ()
{
  if (cursor_underline)
    {
      u_s_off_raw ();
      cursor_underline = 0;
    }
}

void
b_s_on ()
{
  if (!cursor_bold)
    {
      b_s_on_raw ();
      cursor_bold = 1;
    }
  if (cursor_reverse)
    {
      h_r_on_raw ();
    }
}

void
b_s_off ()
{
  if (cursor_bold)
    {
      b_s_off_raw ();
      cursor_bold = 0;
    }
}

void
kk_cursor_invisible ()
{
  if (cursor_invisible_fun && (uum_cursor_invisible == 0))
    {
      cursor_invisible_raw ();
      flush ();
    }
  uum_cursor_invisible = 1;
}

void
kk_cursor_normal ()
{
  if (cursor_invisible_fun && uum_cursor_invisible)
    {
      cursor_normal_raw ();
      flush ();
    }
  uum_cursor_invisible = 0;
}

void
kk_save_cursor ()
{
  if (cursor_upscreen)
    {
      save_cursor_raw ();
      cursor_upscreen = 0;
      set_cursor_status ();
      flush ();
    }
}

void
kk_restore_cursor ()
{
  if (!cursor_upscreen)
    {
      reset_cursor_status ();
      restore_cursor_raw ();
      cursor_upscreen = 1;
      flush ();
    }
}

void
reset_cursor_status ()
{
  if (!cursor_upscreen)
    {
      kk_cursor_normal ();
      h_r_off ();
      u_s_off ();
      flush ();
    }
}

void
set_cursor_status ()
{
  if (!cursor_upscreen)
    {
      if (uum_cursor_invisible)
        {
          if (cursor_invisible_fun)
            {
              cursor_invisible_raw ();
            }
          else
            {
              throw_col (maxlength - 1);
            }
        }
      else
        {
          if (cursor_invisible_fun)
            {
              cursor_normal_raw ();
            }
        }
      if (cursor_reverse)
        {
          h_r_on_raw ();
        }
      if (cursor_underline)
        {
          u_s_on_raw ();
        }
      throw_cur_raw (cursor_colum % maxlength, crow + (cursor_colum / maxlength));
      flush ();
    }
}

void
scroll_up ()
{
  int k;

  for (k = conv_lines; k > 0; k--)
    {
      putchar ('\n');
    }
}


void
clr_line_all ()
{
  throw_c (0);
  clr_end_screen ();
}

#define STACKLENGTH 20

struct cursor_state
{
  int colum;
  int upscreen;
  int reverse;
  int underline;
  int invisible;
};
static struct cursor_state savedupscreen[STACKLENGTH];
static int top = 0;

void
reset_cursor ()
{
  top = 0;
}

/* 現在のカーソルの位置を(上か下か)保存する*/
/* You can push the status of cursor. 
   buf terminal can remember only one cursor point!!.
*/
void
push_cursor ()
{
  if (top >= STACKLENGTH)
    {
      printf ("Error Cursor Stack");
    }
  savedupscreen[top].upscreen = cursor_upscreen;
  savedupscreen[top].colum = cursor_colum;
  savedupscreen[top].reverse = cursor_reverse;
  savedupscreen[top].underline = cursor_underline;
  savedupscreen[top].invisible = uum_cursor_invisible;
  top++;
  reset_cursor_status ();
}

/* push_cursorで保存された位置にカーソルを飛ばす。*/
void
pop_cursor ()
{
  if (top <= 0)
    {
      printf ("Error Empty Stack");
    }
  top--;
  if (savedupscreen[top].upscreen)
    {
      kk_restore_cursor ();
    }
  else
    {
      kk_save_cursor ();
    }
  cursor_colum = savedupscreen[top].colum;
  cursor_reverse = savedupscreen[top].reverse;
  cursor_underline = savedupscreen[top].underline;
  uum_cursor_invisible = savedupscreen[top].invisible;
  set_cursor_status ();
  flush ();
}


static int saved_cursor_rev;
static int saved_cursor_und;
void
push_hrus ()
{
  saved_cursor_rev = cursor_reverse;
  saved_cursor_und = cursor_underline;
  h_r_off ();
  u_s_off ();
}

void
pop_hrus ()
{
  if (saved_cursor_rev)
    h_r_on ();
  if (saved_cursor_und)
    u_s_on ();
}

void
set_hanten_ul (x, y)
     int x, y;
{
  if (!x)
    h_r_off ();
  if (!y)
    u_s_off ();
  if (x)
    h_r_on ();
  if (y)
    u_s_on ();
  flush ();
}

void
set_bold (x)
     int x;
{
  if (x)
    b_s_on ();
  flush ();
}

void
reset_bold (x)
     int x;
{
  if (x)
    b_s_off ();
  flush ();
}
