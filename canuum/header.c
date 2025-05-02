/*
 *  header.c,v 1.5 2001/09/16 11:42:58 hiroo Exp
 *  Canna: $Id: header.c,v 1.3 2003/01/04 07:31:02 aida_s Exp $
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
#include "wnn_config.h"
#include "sdefine.h"
#include "sheader.h"
#include "buffer.h"

int not_redraw;
w_char *input_buffer;
w_char *return_buf;             /* return you mojiretsu buffer */
struct wnn_buf *bun_data_;

WnnEnv *normal_env = NULL;
WnnEnv *reverse_env = NULL;
WnnEnv *cur_normal_env = NULL;
WnnEnv *cur_reverse_env = NULL;

int cur_bnst_ = 0;              /* current bunsetsu pointer */

int crow;
int disp_mode_length = WNN_DISP_MODE_LEN;       /* モード表示に必要なcolumn数 */
char Term_Name[128];

int rubout_code = 127;
int kk_on = 0;
int quote_code = -1;
int quote_flag = 0;

int max_history;

int (*main_table[TBL_CNT][TBL_SIZE]) ();
char romkan_clear_tbl[TBL_CNT][TBL_SIZE];
w_char jishopath[64];
w_char hindopath[64];
w_char fuzokugopath[64];

struct buf *c_b;
short pty_c_flag;
short tty_c_flag;

int cursor_invisible_fun = 0;
int keypad_fun = 0;
int send_ascii_char = 0;
int excellent_delete = 1;
int convkey_on = 0;

int *bunsetsu;
int *bunsetsuend;
struct wnn_env **bunsetsu_env;
int *touroku_bnst;
int touroku_bnst_cnt;
/* int bunsetsucnt = 0; */
w_char *kill_buffer;
w_char *remember_buf;
int kill_buffer_offset = 0;

int touroku_comment = 0;        /* Comment for Words in Word_add */

char lang_dir[LANGDIRLEN] = { '\0' };

char uumkey_name_in_uumrc[PATHNAMELEN] = { '\0' };
char convkey_name_in_uumrc[PATHNAMELEN] = { '\0' };
char rkfile_name_in_uumrc[PATHNAMELEN] = { '\0' };
short defined_by_option = 0;

int conv_lines = 1;
int flow_control = 0;
int henkan_off_flag = 0;        /* 立ち上げ時に変換をoffにした状態にします */
int henkan_on_kuten = 0;        /* 句点入力で変換しない */

char def_servername[PATHNAMELEN] = { '\0' };    /* V3.0 */
char def_reverse_servername[PATHNAMELEN] = { '\0' };
char username[PATHNAMELEN] = { '\0' };  /* V3.0 */

int remove_cs_from_termcap = 1;

int maxbunsetsu;
int max_ichiran_kosu;
int maxchg;
int maxlength;

/*
w_char kugiri_str[MAXKUGIRI] = {' ',0};
*/

WNN_DIC_INFO *dicinfo;
int dic_list_size;

short internal_code;
short file_code;

int (**code_trans) ();

struct msg_cat *cd;

FuncDataBase *lang_db = NULL;

FunctionTable *f_table = NULL;

FuncDataBase function_db[] = {
#ifdef JAPANESE
  {WNN_J_LANG,
   {NULL, NULL, call_t_redraw_move_normal,
    call_t_redraw_move_1_normal, call_t_redraw_move_2_normal,
    call_t_print_l_normal, NULL, char_len_normal, char_q_len_normal,
    t_redraw_move_normal, t_print_l_normal, c_top_normal, c_end_normal,
    c_end_normal, call_redraw_line_normal, hani_settei_normal,
    errorkeyin, call_jl_yomi_len},
   TTY_KCODE, PTY_KCODE, J_IUJIS, J_EUJIS,
   {
    through, iujis_to_eujis, iujis_to_jis8, iujis_to_sjis,
    eujis_to_iujis, through, eujis_to_jis8, eujis_to_sjis,
    jis_to_iujis, jis_to_eujis, through, jis_to_sjis,
    sjis_to_iujis, sjis_to_eujis, sjis_to_jis8, through},
   "ujsUJS",
   "ujsUJS",
   {
    do_u_opt, do_j_opt, do_s_opt, do_U_opt, do_J_opt, do_S_opt}
   },
#endif /* JAPANESE */
#ifdef CHINESE
  {WNN_C_LANG,                  /* Simplified Chinese */
   {print_out_yincod, input_yincod, call_t_redraw_move_yincod,
    call_t_redraw_move_1_yincod, call_t_redraw_move_2_yincod,
    call_t_print_l_yincod, redraw_when_chmsig_yincod, char_len_yincod,
    char_q_len_yincod, t_redraw_move_yincod,
    t_print_l_yincod, c_top_yincod, c_end_yincod, c_end_nobi_yincod,
    call_redraw_line_yincod, hani_settei_yincod, errorkeyin_q,
    not_call_jl_yomi_len},
   TTY_CCODE, PTY_CCODE, C_IUGB, C_EUGB,
   {
    through, iugb_to_eugb, through, through,
    eugb_to_iugb, through, through, through,
    through, through, through, through,
    through, through, through, through},
   "",
   "",
   {
    NULL, NULL, NULL, NULL, NULL, NULL}
   },
  {WNN_T_LANG,                  /* Traditional Chinese */
   {print_out_yincod, input_yincod, call_t_redraw_move_yincod,
    call_t_redraw_move_1_yincod, call_t_redraw_move_2_yincod,
    call_t_print_l_yincod, redraw_when_chmsig_yincod, char_len_yincod,
    char_q_len_yincod, t_redraw_move_yincod,
    t_print_l_yincod, c_top_yincod, c_end_yincod, c_end_nobi_yincod,
    call_redraw_line_yincod, hani_settei_yincod, errorkeyin_q,
    not_call_jl_yomi_len},
   TTY_TCODE, PTY_TCODE, C_ICNS11643, C_ECNS11643,
   {
    through, icns_to_ecns, icns_to_big5, through,
    ecns_to_icns, through, ecns_to_big5, through,
    big5_to_icns, big5_to_ecns, through, through,
    through, through, through, through},
   "btBT",
   "btBT",
   {
    do_b_opt, do_t_opt, do_B_opt, do_T_opt, NULL, NULL}
   },
#endif /* CHINESE */
#ifdef   KOREAN
  {"ko_KR",
   {NULL, NULL, call_t_redraw_move_normal,
    call_t_redraw_move_1_normal, call_t_redraw_move_2_normal,
    call_t_print_l_normal, NULL, char_len_normal, char_q_len_normal,
    t_redraw_move_normal, t_print_l_normal, c_top_normal, c_end_normal,
    c_end_normal, call_redraw_line_normal, hani_settei_normal,
    errorkeyin, call_jl_yomi_len},
   TTY_HCODE, PTY_HCODE, K_IUKSC, K_EUKSC,
   {
    through, iuksc_to_euksc, through, through,
    euksc_to_iuksc, through, through, through,
    through, through, through, through,
    through, through, through, through},
   "uU",
   "uU",
   {
    do_u_opt, do_U_opt, NULL, NULL, NULL, NULL}
   },
#endif /* KOREAN */
  {NULL,
   {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL},
   0, 0, 0, 0,
   {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL},
   NULL,
   NULL,
   {
    NULL, NULL, NULL, NULL, NULL, NULL}
   }
};

int (*default_code_trans[]) () =
{
through, through, through, through, through, through, through, through, through, through, through, through, through, through, through, through};
