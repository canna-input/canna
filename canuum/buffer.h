/*
 *  buffer.h,v 1.3 2001/06/14 18:16:06 ura Exp
 *  Canna: $Id: buffer.h,v 1.2.2.1 2003/01/11 17:53:57 aida_s Exp $
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

/* bufferの表示のされ方は、hantenによって決定される。
反転するか、下線状態にするかを各独立に、
バッファーの内で、文頭からmarkまで、markからcursorまで、
cursorからbufferの最後までの状態を独立に設定することができる。
         when mark is set
                hanten bit0(0x01),bit1(0x02):      --- mark
                bit2(0x04),bit3(0x08):        mark --- cursor
                bit4(0x10),bit5(0x20):      cursor --- 
          when mark is not set(-1)
                bit4,bit5:      
                bit6(0x40):     t_b_st --- t_b_end ボールド表示
    first one....hanten
    second one...under_line
*/
struct buf
{
  w_char *buffer;               /* 登録用バッファ */
  char *buf_msg; /** 仮名を入力する時のバッファーの前に表示する文字列 */
  int start_col; /** 仮名入力時の画面上のコラムのスタート位置 */
  int t_c_p; /** 仮名入力用バッファー上でのカーソル位置 */
  int hanten; /** 仮名入力用バッファーを反転表示させているかどうか */
  int maxlen; /**仮名入力用バッファーの中にはいっている文字数 */
  int buflen; /** length of the buffer */
  int t_m_start;                /* -1 : markが設定されていない。 */
  /* 0 以上 : マークの位置 */
  int t_b_st;   /**ボールド表示の開始位置 */
  int t_b_end;  /**ボールド表示の終了位置 */
  int vlen;                     /*バッファー中でスクリーンに表示される幅 */
  int duplicate;                /* スクリーンがスクロールする時に、重複して表示する文字数 */

  int vst;                      /* bufferの中で画面に表示されている最初の位置 */
  /*これは、勝手に上のルーチンでいじらない方が望ましい。 */
  int (*key_in_fun) ();         /*ここに関数が設定されていると、key_tableにバインドが
                                   ない文字の入力を
                                   受けた時にバッファーにその文字を入れずに代わりにその関数が呼ばれる */
  int (*redraw_fun) ();         /* romkan kara redraw ga kaette kitatoki */
  int (*ctrl_code_fun) ();      /*ここに関数が設定されていると、key_tableにバインドが
                                   ないコントロール文字の入力を受けた時にベルを鳴らす代わりに
                                   その関数が呼ばれる */
  int (**key_table) ();         /*キーバインドのテーブル */
  char *rk_clear_tbl;           /* romakn_clear が hituyouka */
};


extern int *bunsetsu;
extern int *bunsetsuend;
extern struct wnn_env **bunsetsu_env;   /* 文節の変換に使った env */
extern int *touroku_bnst;
/* extern int bunsetsucnt; */
extern int touroku_bnst_cnt;

extern struct buf *c_b;         /* 現在用いられているbufを示すグローバル変数 */
extern w_char *kill_buffer;     /* キルバッファー大きさは、maxchgとなっている */
extern int kill_buffer_offset;  /* キルバッファー使われている長さ */
extern w_char *remember_buf;    /* リメンバァバッファ大きさは、maxchgとなっている */
