/*
 *  sdefine.h,v 1.3 2001/06/14 18:16:08 ura Exp
 *  Canna: $Id: sdefine.h,v 1.3.2.1 2003/01/11 17:53:57 aida_s Exp $
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

/*************************
 * define of standard i/o
 *************************/

#define MAXCHG 80               /* 解析可能文字数 */
        /*漢字バッファ(次候補、単語検索など)の大きさ */
#define MAX_ICHIRAN_KOSU 36     /* 一覧表示字の表示個数 */
#define MAXWORDS        MAXJIKOUHO      /* 単語検索での検索語数 */
#define NBUN    2               /* n文節解析 */
#define JISHO_PRIO_DEFAULT 5    /*辞書プライオリティのデフォルト値 */

/* 評価関数の係数 */
#define HINDOVAL 3
#define LENGTHVAL 100
#define JIRITUGOVAL 1
#define FLAGVAL 200             /*今使ったよビットの係数 */
#define JISHOPRIOVAL 5

#define TBL_CNT 10              /* key_table no kazu */
/* Two tables are add. one is for selecting zenkouho's, and the other is jisho-ichiran. */
/* One more table is add, which is used in inspect */
/* Deleted table[7] because those are for Wnn3 */
#define TBL_SIZE 256

#define ESCAPE_CHAR(c)  (((c) < ' ') || ((c) == 127))
#define NORMAL_CHAR(c)  (((c) >= ' ') && ((c) < 127))
#define NOT_NORMAL_CHAR(c)      (((c) >= 128) && ((c) < 160))
#define ONEBYTE_CHAR(c)         (!((c) & ~0xff))        /* added by Nide */
/*
#define NISEMONO(c)             ((c) & 0x80000000)
#define HONMONO(c)              (!NISEMONO(c))
#define KANJI_CHAR(c)           ((c) >= 256 )
*/
#define KANJI_CHAR(c)           (((c) >= 256 ) && ((c) & 0xff00) != 0x8e00)

#define zenkaku(x)(KANJI_CHAR(x) || ESCAPE_CHAR(x))
#define hankaku(x) (!zenkaku(x))
#define ZENKAKU_HYOUJI(x)     zenkaku(x)
        /* CHANGE AFTERWARD TO BE ABLE TO TREAT hakaku katakana. */

#ifndef min
#define max(a , b) (((a) > (b))?(a) : (b))
#define min(a , b) (((a) <= (b))?(a) : (b))
#endif

#define numeric(x)      (((x >= S_NUM)&&(x <= E_NUM))? True : False)
#define ISKUTENCODE(x)  ((x) == 0xa1a3)

struct jisho_
{                               /* 辞書管理用structure */
  char name[1024];
  char hname[1024];             /* 頻度ファイル名 */
  int dict_no;                  /* server が返してくる辞書ナンバー */
  int prio;
  int rdonly;
};

struct kansuu
{                               /* kansuu_hyo no entry */
  char *kansuumei;
  char *comment;
  int romkan_flag;              /* Clear Romkan or Not.  */
  int (*func[TBL_CNT]) ();
};

#ifndef w_char
#define w_char  unsigned short
#endif

#define printf        PRINTF
#define fprintf       FPRINTF
#define remove        REMOVE

#define throw_c(col)  throw_col((col) + disp_mode_length)

#define MAX_HISTORY 10


#define flush() fflush(stdout)
#define print_msg(X) {push_cursor();throw_c(0); clr_line();printf(X);flush();pop_cursor();}
#define print_msg_getc(X) {push_cursor();throw_c(0); clr_line();printf(X);flush();keyin();pop_cursor();}


#define UNDER_LINE_MODE (0x02 | 0x08 | 0x20)

#define OPT_CONVKEY             0x01
#define OPT_RKFILE              0x02
#define OPT_WNNKEY              0x04
#define OPT_FLOW_CTRL           0x08
#define OPT_WAKING_UP_MODE      0x10
#define OPT_VERBOSE             0x20

#define convkey_defined_by_option       (defined_by_option & OPT_CONVKEY)
#define rkfile_defined_by_option        (defined_by_option & OPT_RKFILE)
#define uumkey_defined_by_option        (defined_by_option & OPT_WNNKEY)
#define verbose_option                  (defined_by_option & OPT_VERBOSE)

/*
#define char_len(X)((hankaku(X))? 1 : 2)
*/

/*
  GETOPT string & ALL OPTIONS string for configuration.
  see each config.h for detail.
  NOTE: WHEN YOU MODIFY THESE, YOU ALSO MODIFY do_opt[] ARRAY AND
  ALL config.h!!!!
 */

#define GETOPTSTR   "hHPxXk:c:r:l:D:n:v"
#define ALL_OPTIONS "hHujsUJSPxXkcrlDnvbtBT"
#define OPTIONS     "hHPxXkcrlDnv"

/* for message file */

struct msg_cat { /* dummy */
  int dummy;
};

struct msg_cat *msg_open();

#define MSG_GET(no)     msg_get(cd, no, NULL, NULL)

#define CWNN_PINYIN             0
#define CWNN_ZHUYIN             1

#define print_out_func          (f_table->print_out_function)
#define input_func              (f_table->input_function)
#define call_t_redraw_move_func (f_table->call_t_redraw_move_function)
#define call_t_redraw_move_1_func (f_table->call_t_redraw_move_1_function)
#define call_t_redraw_move_2_func (f_table->call_t_redraw_move_2_function)
#define call_t_print_l_func     (f_table->call_t_print_l_function)
#define redraw_when_chmsig_func (f_table->redraw_when_chmsig_function)
#define char_len_func           (f_table->char_len_function)
#define char_q_len_func         (f_table->char_q_len_function)
#define t_redraw_move_func      (f_table->t_redraw_move_function)
#define t_print_l_func          (f_table->t_print_l_function)
#define c_top_func              (f_table->c_top_function)
#define c_end_func              (f_table->c_end_function)
#define c_end_nobi_func         (f_table->c_end_nobi_function)
#define call_redraw_line_func   (f_table->call_redraw_line_function)
#define hani_settei_func        (f_table->hani_settei_function)
#define errorkeyin_func         (f_table->errorkeyin_function)
#define call_jl_yomi_len_func   (f_table->call_jl_yomi_len_function)

#define env_normal               (cur_normal_env->env)
#define env_reverse              (cur_reverse_env->env)

#define envrcname               (cur_normal_env->envrc_name)
#define reverse_envrcname       (cur_reverse_env->envrc_name)

#define env_name_s               (cur_normal_env->env_name_str)
#define reverse_env_name_s       (cur_reverse_env->env_name_str)

#define servername              (cur_normal_env->host_name)
#define reverse_servername      (cur_reverse_env->host_name)

#define normal_sticky           (cur_normal_env->sticky)
#define reverse_sticky          (cur_reverse_env->sticky)
