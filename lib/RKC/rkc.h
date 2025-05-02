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

/* sccs_id[]="@(#) NEC UNIX( PC-UX/EWS-UX ) rkc.h 2.4 91/11/11 12:01:18"; */
/* #ident	"@(#) NEC/V(386) R3.0B rkc.h 5.9 90/03/26 21:04:36" */
/* $Id: rkc.h,v 4.13 1996/11/27 07:21:35 kon Exp $ */

#if (defined(_WINDOWS) || defined(WIN32)) && !defined(WIN)
#define WIN
#endif

#ifdef WIN
#ifndef USE_MALLOC_FOR_BIG_ARRAY
#define USE_MALLOC_FOR_BIG_ARRAY
#endif
#ifndef OMIT_EUC_FUNCS
#define OMIT_EUC_FUNCS
#endif
#endif

#if defined(ENGINE_SWITCH) && !defined(WIN)
#include "RKrename.h"
#endif

#if defined(SYSV) || defined(SVR4) || defined(__STDC__) || defined(WIN)
# if defined(SYSV) || defined(SVR4) || defined(WIN)
#  include <memory.h>
# endif
# ifndef __EMX__
#  ifndef bzero
#   define bzero(buf, size) memset((char *)(buf), 0x00, (size))
#  endif
#  ifndef bcopy
#   define bcopy(src, dst, size) memcpy((char *)(dst), (char *)(src), (size))
#  endif
# endif
#endif

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

extern int ushort2euc(), euc2ushort(), ushort2wchar(), wchar2ushort(),
    wcharstrlen(), ushortstrlen(), ushortstrcpy() ;

#ifdef WIN 
typedef char *caddr_t;
#define SIGINT 2
#endif

#ifdef __STDC__
#include <stdlib.h>
#else
#ifndef WIN
extern char *malloc(), *realloc(), *calloc();
extern void free();
#endif
#endif

#ifndef pro
#if defined(__STDC__) || defined(WIN)
#define pro(x) x
#else
#define pro(x) ()
#endif
#endif

typedef long (*initialize_t) pro((char *));
typedef int (*finalize_t) pro((void));
typedef int (*close_context_t) pro((RkcContext *));
typedef int (*create_context_t) pro((void));
typedef int (*duplicate_context_t) pro((RkcContext *));
typedef int (*dictionary_list_t) pro((RkcContext *, char *, int));
typedef int (*define_dic_t) pro((RkcContext *, char *, Ushort *));
typedef int (*delete_dic_t) pro((RkcContext *, char *, Ushort *));
typedef int (*mount_dictionary_t) pro((RkcContext *, char *, int));
typedef int (*remount_dictionary_t) pro((RkcContext *, char *, int));
typedef int (*umount_dictionary_t) pro((RkcContext *, char *));
typedef int (*mount_list_t) pro((RkcContext *, char *, int));
typedef int (*convert_t) pro((RkcContext *, Ushort *, int, int));
typedef int (*convert_end_t) pro((RkcContext *, int));
typedef int (*get_kanji_list_t) pro((RkcContext *));
typedef int (*get_stat_t) pro((RkcContext *, RkStat *));
typedef int (*resize_t) pro((RkcContext *, int));
typedef int (*store_yomi_t) pro((RkcContext *, Ushort *, int));
typedef int (*get_yomi_t) pro((RkcContext *, Ushort *));
typedef int (*get_lex_t) pro((RkcContext *, int, RkLex *));
typedef int (*autoconv_t) pro((RkcContext *, int, int));
typedef int (*subst_yomi_t) pro((RkcContext *, int, int, int, Ushort *, int));
typedef int (*flush_yomi_t) pro((RkcContext *));
typedef int (*get_last_yomi_t) pro((RkcContext *, Ushort *, int));
typedef int (*remove_bun_t) pro((RkcContext *, int));
typedef int (*get_simple_kanji_t)
    pro((RkcContext *, char *, Ushort *, int, Ushort *, int, Ushort *, int));
typedef int (*query_dic_t)
    pro((RkcContext *, char *, char *, struct DicInfo *));
typedef int (*get_hinshi_t) pro((RkcContext *, Ushort *, int));
typedef int (*store_range_t) pro((RkcContext *, Ushort *, int));
typedef int (*set_locale_t) pro((RkcContext *, char *));
typedef int (*set_app_name_t) pro((RkcContext *, char *));
typedef int (*notice_group_name_t) pro((RkcContext *, char *));
typedef int (*through_t) pro((RkcContext *, int, char *, int, int));
typedef int (*killserver_t) pro((void));
#ifdef EXTENSION
typedef int (*list_dictionary_t) pro((RkcContext *, char *, char *, int));
typedef int (*create_dictionary_t) pro((RkcContext *, char *, int));
typedef int (*remove_dictionary_t) pro((RkcContext *, char *, int));
typedef int (*rename_dictionary_t) pro((RkcContext *, char *, char *, int));
typedef int (*get_text_dictionary_t)
     pro((RkcContext *, char *, char *, Ushort *, int));
typedef int (*sync_t) pro((RkcContext *, char *));
typedef int (*chmod_dic_t) pro((RkcContext *, char *, int));
typedef int (*copy_dictionary_t)
     pro((RkcContext *, char *, char *, char *, int));
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

extern rkc_Connect_Iroha_Server pro((char *));
