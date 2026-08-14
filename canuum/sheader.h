/*
 *  sheader.h,v 1.13 2003/05/11 18:29:24 hiroo Exp
 */

/*
 * FreeWnn is a network-extensible Kana-to-Kanji conversion system.
 * This file is part of FreeWnn.
 * 
 * Copyright Kyoto University Research Institute for Mathematical Sciences
 *                 1987, 1988, 1989, 1990, 1991, 1992
 * Copyright OMRON Corporation. 1987, 1988, 1989, 1990, 1991, 1992, 1999
 * Copyright ASTEC, Inc. 1987, 1988, 1989, 1990, 1991, 1992
 * Copyright FreeWnn Project 1999, 2000, 2002-2003
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

/**************************
 * header of standard i/o 
 **************************/

#ifndef CANNA
#include "jslib.h"
#else
typedef int WNN_DIC_INFO; /* dummy */
#endif
#include "wnn_os.h"

typedef struct _WnnEnv
{
  char *host_name;              /* server name */
  struct wnn_env *env;          /* env */
  int sticky;                   /* sticky bit */
  char *envrc_name;             /* envrc name */
  char env_name_str[32];        /* env name */
  struct _WnnEnv *next;
}
WnnEnv;

struct buf;
typedef struct _FunctionTable
{
/* functions depends on lang */
  int (*print_out_function) (w_char *, w_char *, int);
  int (*input_function) ();
  int (*call_t_redraw_move_function) (int x, int start, int end, int clt_l, int add);
  int (*call_t_redraw_move_1_function) (int x, int start, int end, int clt_l, int add1, int add2, int mode);
  int (*call_t_redraw_move_2_function) (int x, int start1, int start2, int end1, int end2, int clt_l, int add);
  int (*call_t_print_l_function) (int x, int add);
  int (*redraw_when_chmsig_function) ();
  int (*char_len_function) (w_char x);
  int (*char_q_len_function) (w_char x);
  int (*t_redraw_move_function) (int x, int start, int end, int clr_l);
  int (*t_print_l_function) (void);
  int (*c_top_function) (void);
  int (*c_end_function) (void);
  int (*c_end_nobi_function) (void);
  int (*call_redraw_line_function) (int x, int add);
  int (*hani_settei_function) (struct buf *c_b);
  void (*errorkeyin_function) (void);
  int (*call_jl_yomi_len_function) (void);
}
FunctionTable;

typedef int (*code_trans_t) (unsigned char *dst, unsigned char *src, int size);
typedef struct _FuncDadaBase
{
  char *lang;
  FunctionTable f_table;
  short tty_code, pty_code, internal_code, file_code;
  code_trans_t code_trans[16];
  char *ostr;
  char *getoptstr;
  int (*do_opt[6]) (void);
}
FuncDataBase;


extern int not_redraw;          /* c_b->bufferを用いていない時(エラーメッセージを
                                   表示しているなど)に、リドローがかかってもリドローしないためのフラグ */
extern int maxchg;              /*一度に変換できる文字数 */

extern int maxlength;           /* 画面の横幅を表す */
extern w_char *input_buffer;    /* 画面制御に使うバッファ */

extern struct wnn_buf *bun_data_;

extern WnnEnv *normal_env;
extern WnnEnv *reverse_env;
extern WnnEnv *cur_normal_env;
extern WnnEnv *cur_reverse_env;

extern int cur_bnst_;           /* current bunsetsu pointer */

/*extern  int   b_suu_;         *//* hold bunsetsu suu */
                                /* Use jl_bun_suu(bun_data_) */

extern char romkan_clear_tbl[TBL_CNT][TBL_SIZE];
extern int (*main_table[TBL_CNT][TBL_SIZE]) (); /* holding commands */

/*  extern w_char *p_holder; *//* points the end of data in buffer. */

extern w_char *knj_buffer;      /* 辞書ユーティリティー使用時の漢字バッファ */


extern int crow;
        /* holding row where i/f uses to display kanji line */

extern w_char *return_buf;
        /* 確定した漢字かな混じり文を返すためのバッファ */


extern char Term_Name[];

extern int rubout_code;         /* rubout に使われるコードを保持する */
extern int kk_on;               /* 仮名漢字変換可能モードか否かを示すフラグ */
extern int quote_code;
extern int quote_flag;

extern int max_history;
extern w_char jishopath[];
extern w_char hindopath[];
extern w_char fuzokugopath[];


extern short tty_c_flag;
extern short pty_c_flag;

extern int cursor_invisible_fun;        /* flag that cursor_invisible is in the termcap entry */
extern int keypad_fun;          /* flag that keypad is in the termcap entry */
extern int send_ascii_char;     /* flag that if send ascii characters when the buffer is empty */
extern int excellent_delete;
extern int convkey_on;

#define LANGDIRLEN 32
extern char lang_dir[];

extern char uumkey_name_in_uumrc[];
extern char convkey_name_in_uumrc[];
extern char rkfile_name_in_uumrc[];
extern short defined_by_option;

extern int conv_lines;
extern int flow_control;
extern int henkan_off_flag;     /* 立ち上げ時に変換をon/offにした状態にします */
extern int henkan_on_kuten;     /* 句点入力で変換する/しない */

extern char def_servername[];   /* V3.0 */
extern char def_reverse_servername[];
extern char username[];         /* V3.0 */
extern char user_dic_dir[];     /* V3.0 */

extern int remove_cs_from_termcap;

extern int disp_mode_length;    /* V3.0 Alternate for MHL */


extern int kanji_buf_size;
extern int maxbunsetsu;
extern int max_ichiran_kosu;

/*
#define MAXKUGIRI 32
extern w_char kugiri_str[];
*/

extern WNN_DIC_INFO *dicinfo;
extern int dic_list_size;

extern int touroku_comment;

extern short internal_code;
extern short file_code;

extern code_trans_t *code_trans;

extern struct msg_cat *cd;

extern FuncDataBase *lang_db;

extern FunctionTable *f_table;

extern FuncDataBase function_db[];

extern code_trans_t default_code_trans[];

/* ============================================================
 *   extern function prototypes
 * ============================================================ */
#ifndef CANNA
#include "rk_spclval.h"
#include "rk_fundecl.h"
#include "wnn_string.h"
#endif

#ifdef CANNA
/* w_string.c.c */
w_char *Strncpy(w_char *ws1, w_char *ws2, int cnt);
int eu_columlen(unsigned char *c);
/* basic_op.c */
void set_screen_vars_default(void);
/* prologue.c */
int init_uum(void);
/* epilogue.c */
void epilogue_no_close(void);
void epilogue(void);
/* functions.c */
int t_print_l_normal(void);
char *romkan_dispmode(void);
char *romkan_offmode(void);
/* etc/msg.c */
struct msg_cat *msg_open(char *name, char *nlspath, char *lang);
char *msg_get(struct msg_cat *cad, int n, char *mesg, register char *lang);
/* wnnrc_op.c */
char *get_kbd_env(void);
/* conv/cvt_read.c */
int convert_getterm(char *term, int flag);
int keyin1(int (*gch) (void), char *yyy);
/* canna.c */
void canna_mainloop(void);
char *wnn_perror(void);
/* etc/server_env.c */
char *get_server_env(char *lang);
/* touroku.c */
int hani_settei_normal(struct buf *c_b);
/* prologue.c */
int initial_message_out(void);
/* uif.c */
int set_cur_env(int s);
/* functions.c */
int char_len_normal(w_char x);
int c_top_normal(void);
int c_end_normal(void);
int call_t_print_l_normal(int x, int add);
int char_q_len_normal(w_char x);
int call_jl_yomi_len(void);
int t_redraw_move_normal(int x, int start, int end, int clr_l);
int call_t_redraw_move_normal(int x, int start, int end, int clt_l, int add);
int call_t_redraw_move_1_normal(int x, int start, int end, int clt_l, int add1, int add2, int mode);
int call_t_redraw_move_2_normal(int x, int start1, int start2, int end1, int end2, int clt_l, int add);
int call_redraw_line_normal(int x, int add);
#endif /* CANNA */

/* cursor.c */
void throw_col(int col);
void h_r_on(void);
void h_r_off(void);
void u_s_on(void);
void u_s_off(void);
void b_s_on(void);
void b_s_off(void);
void kk_cursor_invisible(void);
void kk_cursor_normal(void);
void kk_save_cursor(void);
void kk_restore_cursor(void);
void reset_cursor_status(void);
void set_cursor_status(void);
void scroll_up(void);
void clr_line_all(void);
void reset_cursor(void);
void push_cursor(void);
void pop_cursor(void);
void push_hrus(void);
void pop_hrus(void);
void set_hanten_ul(int x, int y);
void set_bold(int x);
void reset_bold(int x);

/* jhlp.c */
void uum_err (char *);
int do_u_opt(void);
int do_j_opt(void);
int do_s_opt(void);
int do_U_opt(void);
int do_J_opt(void);
int do_S_opt(void);
int do_b_opt(void);
int do_t_opt(void);
int do_B_opt(void);
int do_T_opt(void);
int conv_keyin(char *inkey);
int keyin(void);
unsigned char keyin0(void);
int arrange_ioctl(int jflg);
#if !(HAVE_SETENV)
int setenv(char *var, char *value, int overwrite);
#endif

/* printf.c */
int FPRINTF(FILE *file, const char *format, ...);
int PRINTF(const char *format, ...);
void puteustring(char *buf2, FILE *file);
int w_putchar(w_char w);
void putchar_norm(int c);
void putchar1(int c);
void flushw_buf(void);
void errorkeyin(void);

/* screen.c */
void throw(int x);
int char_len(w_char x);
void t_redraw_one_line(void);
void init_screen(void);
int check_vst(void);
int t_redraw_move(int x, int start, int end, int clr_l);
int t_move(int x);
int t_print_l(void);
void t_print_line(int st, int end, int clr_l);
void t_cont_line_note_delete(void);
int cur_ichi(int cp, int start_point);
void print_buf_msg(char *msg);
char * get_rk_modes(void);
int disp_mode(void);
void display_henkan_off_mode(void);
void t_throw(void);
void clr_line(void);

/* termcap.c */
#ifdef TERMCAP
int getTermData(void);
int set_TERMCAP(void);
void set_keypad_on(void);
void set_keypad_off(void);
void set_scroll_region(int start, int end);
void clr_end_screen(void);
void throw_cur_raw(int col, int row);
void h_r_on_raw(void);
void h_r_off_raw(void);
void u_s_on_raw(void);
void u_s_off_raw(void);
void b_s_on_raw(void);
void b_s_off_raw(void);
void ring_bell(void);
void save_cursor_raw(void);
void restore_cursor_raw(void);
void cursor_invisible_raw(void);
void cursor_normal_raw(void);
#endif /* TERMCAP */

/* termio.c */
#ifdef TERMINFO
int openTermData(void);
void closeTermData(void);
void set_keypad_on(void);
void set_keypad_off(void);
void set_scroll_region(int start, int end);
void clr_end_screen(void);
void throw_cur_raw(int col, int row);
void h_r_on_raw(void);
void h_r_off_raw(void);
void u_s_on_raw(void);
void u_s_off_raw(void);
void b_s_on_raw(void);
void b_s_off_raw(void);
void ring_bell(void);
void save_cursor_raw(void);
void restore_cursor_raw(void);
void cursor_invisible_raw(void);
void cursor_normal_raw(void);
#endif /* TERMINFO */

/* xutoj.c */
int flush_designate(w_char *buf);
int through(unsigned char *x, unsigned char *y, int z);
int get_cswidth_by_char(int c);
void wnn_delete_w_ss2(register w_char *s, register int n);
#ifdef JAPANESE
#ifdef JIS7
int iujis_to_jis(unsigned char *jis, unsigned char *iujis, int iusiz);
int eujis_to_jis(unsigned char *jis, unsigned char *eujis, int eusiz);
int sjis_to_jis(unsigned char *jis, unsigned char *sjis, int siz);
#endif
int iujis_to_jis8(unsigned char *jis, unsigned char *iujis, int iusiz);
int eujis_to_jis8(unsigned char *jis, unsigned char *eujis, int eusiz);
int iujis_to_eujis(unsigned char *eujis, unsigned char *iujis, int iusiz);
int jis_to_eujis(unsigned char *eujis, unsigned char *jis, int jsiz);
int eujis_to_sjis(unsigned char *sjis, unsigned char *eujis, int eusiz);
int iujis_to_sjis(unsigned char *sjis, unsigned char *iujis, int iusiz);
int sjis_to_iujis(unsigned char *iujis, unsigned char *sjis, int ssiz);
int sjis_to_eujis(unsigned char *eujis, unsigned char *sjis, int ssiz);
int sjis_to_jis8(unsigned char *jis, unsigned char *sjis, int siz);
int jis_to_iujis(unsigned char *iujis, unsigned char *jis, int jsiz);
int jis_to_sjis(unsigned char *sjis, unsigned char *jis, int siz);
int eujis_to_iujis(unsigned char *iujis, unsigned char *eujis, int eusiz);
#endif /* JAPANESE */
#ifdef CHINESE
int ecns_to_icns(unsigned char *icns, unsigned char *ecns, int siz);
int icns_to_ecns(unsigned char *ecns, unsigned char *icns, int siz);
int icns_to_big5(unsigned char *big5, unsigned char *icns, int siz);
int ecns_to_big5(unsigned char *big5, unsigned char *ecns, int siz);
int big5_to_icns(unsigned char *icns, unsigned char *big5, int siz);
int big5_to_ecns(unsigned char *ecns, unsigned char *big5, int siz);
int iugb_to_eugb(unsigned char *eugb, unsigned char *iugb, int siz);
int eugb_to_iugb(unsigned char *iugb, unsigned char *eugb, int siz);
#endif /* CHINESE */
#ifdef KOREAN
int iuksc_to_ksc(unsigned char *ksc, unsigned char *iuksc, int iusiz);
int euksc_to_ksc(unsigned char *ksc, unsigned char *euksc, int eusiz);
int iuksc_to_euksc(unsigned char *euksc, unsigned char *iuksc, int iusiz);
int ksc_to_euksc(unsigned char *euksc, unsigned char *ksc, int jsiz);
int ksc_to_iuksc(unsigned char *iuksc, unsigned char *ksc, int jsiz);
int euksc_to_iuksc(unsigned char *iuksc, unsigned char *euksc, int eusiz);
#endif /* KOREAN */

/* to be classified */
#ifndef CANNA
extern int backward (void);
extern int buffer_in (void);
extern int change_ascii_to_int (char*, int*);
extern void change_to_empty_mode (void);
extern void change_to_insert_mode (void);
extern int connect_jserver (int);
extern int convert_getterm ();
extern int convert_key_setup ();
extern int dai_end (struct wnn_buf *, int);
extern int dai_top (struct wnn_buf *, int);
extern int dic_nickname (int, char*);
extern int disconnect_jserver (void);
extern int empty_modep (void);
extern void epilogue (void);
extern void epilogue_no_close (void);
extern int eu_columlen (unsigned char *);
extern int expand_argument (char *);
extern int expand_expr (char *);
extern void fill (char *, int);
extern int find_dic_by_no (int);
extern int find_end_of_tango (int);
extern int find_entry (char *);
extern int forward_char (void);
extern int backward_char (void);
extern void get_end_of_history ();
extern void getfname ();
extern int henkan_gop ();
extern void henkan_if_maru ();
extern int henkan_off ();
extern int hextodec ();
extern int hinsi_in ();
extern int init_history ();
extern int init_key_table ();
extern void initialize_vars ();
extern int input_a_char_from_function ();
extern int insert_char ();
extern int insert_char_and_change_to_insert_mode ();
extern int insert_modep ();
extern int jtosj ();
extern int jutil ();
extern int kakutei ();
extern int kana_in ();
extern int kana_in_w_char_msg ();
extern int kk ();
extern int make_history ();
extern int make_info_out ();
extern int make_jikouho_retu ();
extern void make_kanji_buffer ();
extern int make_string_for_ke ();
extern int next_history1 ();
extern int nobasi_tijimi_mode ();
extern int nobi_conv ();
extern int previous_history1 ();
extern int reconnect_jserver_body ();
extern int redraw_line ();
extern int redraw_nisemono ();
extern void remove_key_bind ();
extern int isconect_jserver ();
extern int ren_henkan0 ();
extern int select_jikouho1 ();
extern int select_line_element ();
extern int select_one_dict1 ();
extern int select_one_element ();
extern void set_escape_code ();
extern void set_lc_offset ();
extern int st_colum ();
extern int t_delete_char ();
extern int t_kill ();
extern int t_rubout ();
extern int t_yank ();
extern int tan_conv ();
extern int tan_henkan1 ();
extern void touroku ();
extern int update_dic_list ();
extern int uumrc_get_entries ();
extern void w_printf ();
extern void w_sttost ();
extern int wchartochar ();
extern int yes_or_no ();
extern int yes_or_no_or_newline ();
extern int zenkouho_dai_c ();
extern void find_yomi_for_kanji ();
extern int push_unget_buf ();
extern unsigned int *get_unget_buf ();
extern int if_unget_buf ();

extern char env_state ();
extern void get_new_env ();

extern int call_t_redraw_move ();
extern int call_t_redraw_move_1 ();
extern int call_t_redraw_move_2 ();
extern int call_t_print_l ();
extern int c_end_nobi_normal ();
extern int call_redraw_line ();
extern void call_errorkeyin ();
extern int sStrcpy ();
extern int Sstrcpy ();
extern char *sStrncpy ();
extern w_char *Strcat ();
extern w_char *Strncat ();
extern int Strncmp ();
extern w_char *Strcpy ();
extern int Strlen ();
extern void conv_ltr_to_ieuc ();
#endif /* !CANNA */

#ifdef CHINESE
extern int call_t_redraw_move_yincod ();
extern int call_t_redraw_move_1_yincod ();
extern int call_t_redraw_move_2_yincod ();
extern int call_t_print_l_yincod ();
extern int input_yincod ();
extern int redraw_when_chmsig_yincod ();
extern int c_top_yincod ();
extern int c_end_yincod ();
extern int c_end_nobi_yincod ();
extern int print_out_yincod ();
extern int char_q_len_yincod ();
extern int char_len_yincod ();
extern int t_redraw_move_yincod ();
extern int t_print_l_yincod ();
extern int call_redraw_line_yincod ();
extern int hani_settei_yincod ();
extern void errorkeyin_q ();
extern int not_call_jl_yomi_len ();
extern int cwnn_pzy_yincod ();
extern int cwnn_yincod_pzy_str ();

extern int do_b_opt ();
extern int do_t_opt ();
extern int do_B_opt ();
extern int do_T_opt ();
#endif /* CHINESE */

#ifdef  KOREAN
extern int do_u_opt ();
extern int do_U_opt ();
#endif /* KOREAN */

extern void romkan_set_lang ();
