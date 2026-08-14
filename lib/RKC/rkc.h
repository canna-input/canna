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


#if defined(ENGINE_SWITCH)
#include "RKrename.h"
#endif

#include "ccompat.h"
#include "rkcapi.h"

/* 文節情報レコード
 *
 */

#define MAX_HOSTNAME	256

typedef struct _RkcBun {
    unsigned short  *kanji  ;	/* 第一候補または全候補列 */
    short	    curcand ;	/* カレント漢字候補番号 */
    short	    maxcand ;	/* 漢字候補総数 */
    short	    flags   ;	/* フラグ */
#define NOTHING_KOUHO	 0x00
#define FIRST_KOUHO	0x01	   /* kanjiは先頭候補のみ */
#define NUMBER_KOUHO	0x02	   /* kanjiは候補一覧のポインタ */
} RkcBun ;			   /* この場合、curcandは0,maxcandは1 */


/*
 *  クライアントコンテクストレコ−ド
 *
 */
typedef struct _RkcContext {
    short	    server ;  /* サ−バ・コンテクスト番号 */
    short	    client ;  /* クライアント・コンテクスト番号 */
    RkcBun	    *bun   ;  /* 文節情報レコード配列へのポインタ */
    unsigned short *Fkouho ; /* 第一候補列へのポインタ */
    short	    curbun ;  /* カレント文節番号 */
    short	    maxbun ;  /* 文節総数 */
    short	    bgnflag ; /* RkBgnBunのフラグ */
    unsigned short *lastyomi;
    short	    maxyomi;
} RkcContext ;


typedef long (*initialize_t)(char *);
typedef int (*finalize_t)(void);
typedef int (*close_context_t)(RkcContext *);
typedef int (*create_context_t)(void);
typedef int (*duplicate_context_t)(RkcContext *);
typedef int (*dictionary_list_t)(RkcContext *, char *, int);
typedef int (*define_dic_t)(RkcContext *, char *, Ushort *);
typedef int (*delete_dic_t)(RkcContext *, char *, Ushort *);
typedef int (*mount_dictionary_t)(RkcContext *, char *, int);
typedef int (*remount_dictionary_t)(RkcContext *, char *, int);
typedef int (*umount_dictionary_t)(RkcContext *, char *);
typedef int (*mount_list_t)(RkcContext *, char *, int);
typedef int (*convert_t)(RkcContext *, Ushort *, int, int);
typedef int (*convert_end_t)(RkcContext *, int);
typedef int (*get_kanji_list_t)(RkcContext *);
typedef int (*get_stat_t)(RkcContext *, RkStat *);
typedef int (*resize_t)(RkcContext *, int);
typedef int (*store_yomi_t)(RkcContext *, Ushort *, int);
typedef int (*get_yomi_t)(RkcContext *, Ushort *);
typedef int (*get_lex_t)(RkcContext *, int, RkLex *);
typedef int (*autoconv_t)(RkcContext *, int, int);
typedef int (*subst_yomi_t)(RkcContext *, int, int, int, Ushort *, int);
typedef int (*flush_yomi_t)(RkcContext *);
typedef int (*get_last_yomi_t)(RkcContext *, Ushort *, int);
typedef int (*remove_bun_t)(RkcContext *, int);
typedef int (*get_simple_kanji_t)(RkcContext *, char *, Ushort *, int, Ushort *, int, Ushort *, int);
typedef int (*query_dic_t)(RkcContext *, char *, char *, struct DicInfo *);
typedef int (*get_hinshi_t)(RkcContext *, Ushort *, int);
typedef int (*store_range_t)(RkcContext *, Ushort *, int);
typedef int (*set_locale_t)(RkcContext *, char *);
typedef int (*set_app_name_t)(RkcContext *, char *);
typedef int (*notice_group_name_t)(RkcContext *, char *);
typedef int (*through_t)(RkcContext *, int, char *, int, int);
typedef int (*killserver_t)(void);
#ifdef EXTENSION
typedef int (*list_dictionary_t)(RkcContext *, char *, char *, int);
typedef int (*create_dictionary_t)(RkcContext *, char *, int);
typedef int (*remove_dictionary_t)(RkcContext *, char *, int);
typedef int (*rename_dictionary_t)(RkcContext *, char *, char *, int);
typedef int (*get_text_dictionary_t)(RkcContext *, char *, char *, Ushort *, int);
typedef int (*sync_t)(RkcContext *, char *);
typedef int (*chmod_dic_t)(RkcContext *, char *, int);
typedef int (*copy_dictionary_t)(RkcContext *, char *, char *, char *, int);
#endif

struct rkcproto {
  initialize_t initialize;
  finalize_t finalize;
  close_context_t close_context;
  create_context_t create_context;
  duplicate_context_t duplicate_context;
  dictionary_list_t dictionary_list;
  define_dic_t define_dic;
  delete_dic_t delete_dic;
  mount_dictionary_t mount_dictionary;
  remount_dictionary_t remount_dictionary;
  umount_dictionary_t umount_dictionary;
  mount_list_t mount_list;
  convert_t convert;
  convert_end_t convert_end;
  get_kanji_list_t get_kanji_list;
  get_stat_t get_stat;
  resize_t resize;
  store_yomi_t store_yomi;
  get_yomi_t get_yomi;
  get_lex_t get_lex;
  autoconv_t autoconv;
  subst_yomi_t subst_yomi;
  flush_yomi_t flush_yomi;
  get_last_yomi_t get_last_yomi;
  remove_bun_t remove_bun;
  get_simple_kanji_t get_simple_kanji;
  query_dic_t query_dic;
  get_hinshi_t get_hinshi;
  store_range_t store_range;
  set_locale_t set_locale;
  set_app_name_t set_app_name;
  notice_group_name_t notice_group_name;
  through_t through;
  killserver_t killserver;
#ifdef EXTENSION
  list_dictionary_t list_dictionary;
  create_dictionary_t create_dictionary;
  remove_dictionary_t remove_dictionary;
  rename_dictionary_t rename_dictionary;
  get_text_dictionary_t get_text_dictionary;
  sync_t sync;
  chmod_dic_t chmod_dic;
  copy_dictionary_t copy_dictionary;
#endif /* EXTENSION */
};

/* BASIC TYPE:
 *	subete no data ha MSB first(Motorolla order) de tenkai sareru
 *		unsigned char	w
 *		unsigned short	wx
 *		unsigned long	wxyz
 */	
#define LOMASK(x)	((x)&255)
#define	LTOL4(l, l4)	{\
	(l4)[0] = LOMASK((l)>>24); (l4)[1] = LOMASK((l)>>16);\
	(l4)[2] = LOMASK((l)>> 8); (l4)[3] = LOMASK((l));\
}
#define	LTOL3(l, l3)	{\
(l3)[0] = LOMASK((l)>>16); (l3)[1] = LOMASK((l)>> 8); (l3)[2] = LOMASK((l));\
}
#define	STOS2(s, s2)	{\
	(s2)[0] = LOMASK((s)>> 8); (s2)[1] = LOMASK((s));\
}

#define RK_LINE_BMAX 1024 /* これは RKintern.h のと同じ値でなければならない */

#if 0
#define I16toI32(x) (((x) & 0x8000) ? ((x) | 0xffff8000) : (x))
#endif
#define I16toI32(x) (x)
#define I8toI32(x) (((x) & 0x80) ? ((x) | 0xffffff80) : (x))

#ifndef YES
#define YES 1
#endif
#ifndef NO
#define NO  0
#endif

#define SIZEOFSHORT 2 /* for protocol */
#define SIZEOFLONG  4 /* for protocol */

#define MAX_CX 100

typedef struct {
  char *uname;        /* user name */
  char *gname;        /* group name */
  char *topdir;       /* install dir */
} RkUserInfo;

/* function prototypes .. */

/* convert.c */
int rkc_initialize(char *);
int rkc_finalize(void);
int rkc_create_context(void);
int rkc_duplicate_context(RkcContext *);
int rkc_close_context(RkcContext *);
int rkc_dictionary_list(RkcContext *, char *, int);
int rkc_define_dic(RkcContext *, char *, Ushort *);
int rkc_delete_dic(RkcContext *, char *, Ushort *);
int rkc_mount_dictionary(RkcContext *, char *, int);
int rkc_remount_dictionary(RkcContext *, char *, int);
int rkc_umount_dictionary(RkcContext *, char *);
int rkc_mount_list(RkcContext *, char *, int);
int rkc_convert(RkcContext *, Ushort *, int, int);
int rkc_convert_end(RkcContext *, int);
int rkc_get_kanji_list(RkcContext *);
int rkc_resize(RkcContext *, int);
int rkc_store_yomi(RkcContext *, Ushort *, int);
int rkc_get_yomi(RkcContext *, Ushort *);
int rkc_get_stat(RkcContext *, RkStat *);
int rkc_get_lex(RkcContext *, int, RkLex *);
int rkc_autoconv(RkcContext *, int, int);
int rkc_subst_yomi(RkcContext *, int, int, int, Ushort *, int);
int rkc_flush_yomi(RkcContext *);
int rkc_get_last_yomi(RkcContext *, Ushort *, int);
int rkc_remove_bun(RkcContext *, int);
int rkc_get_simple_kanji(RkcContext *, char *, Ushort *, int, Ushort *, int, Ushort *, int);
int rkc_query_dic(RkcContext *, char *, char *, struct DicInfo *);
int rkc_get_hinshi(RkcContext *, Ushort *, int);
int rkc_store_range(RkcContext *, Ushort *, int);
int rkc_set_locale(RkcContext *, char *);
int rkc_sync(RkcContext *, char *);
int rkc_set_app_name(RkcContext *, char *);
int rkc_notice_group_name(RkcContext *, char *);
int rkc_chmod_dic(RkcContext *, char *, int);
int rkc_through(RkcContext *, int, char *, int, int);

/* rkc.c */
int _RkwGetYomi(RkcContext *, Ushort *, int);
int G070_RkcGetServerFD(void);
int G069_RkcConnectIrohaServer(char *);
int RkThrough(int, int, unsigned char *, int, int);

/* wconvert.c */
int rkc_Connect_Iroha_Server(char *);
int rkcw_get_server_info(int *, int *);
BYTE *copyS8(BYTE *, BYTE *, int);

/* wutil.c */
int ushort2eucsize(Ushort *, int);
int ushort2euc(Ushort *, int, char *, int);
int eucchars(unsigned char *, int);
int euc2ushort(char *, int, Ushort *, int);
int Wineuc2ushort(char *, int, Ushort *, int);
int wchar2ushort(cannawc *, int, Ushort *, int);
int ushort2wchar(Ushort *, int, cannawc *, int);
int Winushort2wchar(Ushort *, int, cannawc *, int);
int wcharstrlen(cannawc *);
int ushortstrlen(Ushort *);
int ushortstrcpy(Ushort *, Ushort *);
int ushortstrncpy(Ushort *, Ushort *, int);

