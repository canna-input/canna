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

/*
 */

#ifndef _CANNA_H_
#define _CANNA_H_

#include "cannaconf.h"
#include "ccompat.h"
#include <stdio.h>

#define CANNA_NEW_WCHAR_AWARE
#include <canna/RK.h>
#include <canna/jrkanji.h>

#define POINTERINT canna_intptr_t
#define exp(x) x

#if 0
#define USE_MALLOC_FOR_BIG_ARRAY
#endif

#define	WCHARSIZE	(sizeof(cannawc))

#define MBstowcs CANNA_mbstowcs
#define WCstombs CANNA_wcstombs

#include "sglobal.h"

#define XLookupKanji2			 IROHA_G300_XLookupKanji2
#define XKanjiControl2			 IROHA_G301_XKanjiControl2
#define XwcLookupKanji2			 IROHA_G425_XwcLookupKanji2
#define XwcKanjiControl2		 IROHA_G426_XwcKanjiControl2
#define FirstTime			 CANNA_G271_FirstTime

#define STROKE_LIMIT 500 /* ストロークで接続を切る */

typedef unsigned char BYTE;

/*********************************************************************
 *                      wchar_t replace begin                        *
 *********************************************************************/
#ifdef wchar_t
# error "wchar_t is already defined"
#endif
#define wchar_t cannawc

/*
 * CANNALIBDIR  -- システムのカスタマイズファイルやローマ字かな変換
 *                 テーブルが置かれるディレクトリ。
 */

#ifndef CANNALIBDIR
#define CANNALIBDIR "/usr/lib/canna"
#endif

/* flags の情報 */
#define CANNA_KANJIMODE_TABLE_SHARED	01
#define CANNA_KANJIMODE_EMPTY_MODE	02

/* func の第三引数 */
#define KEY_CALL  0
#define KEY_CHECK 1
#define KEY_SET   2

extern BYTE default_kmap[];

/* menuitem -- メニュー表示の項目を定義するテーブル */

typedef struct _menuitem {
  int flag; /* 下を見よ */
  union {
    struct _menustruct *menu_next; /* メニューへのポインタ */
    int fnum;    /* 機能番号 */
    char *misc;  /* その他(lisp のシンボルなど) */
  } u;
} menuitem;

#define MENU_SUSPEND 0 /* まだ決まっていない(lisp のシンボル) */
#define MENU_MENU    1 /* メニュー */
#define MENU_FUNC    2 /* 機能番号 */

/* menustruct -- メニューをしきる構造体 */

typedef struct _menustruct {
  int     nentries; /* メニューの項目の数 */
  wchar_t **titles; /* メニューの見出しリスト */
  wchar_t *titledata; /* 上のリストの実態文字列 */
  menuitem *body;   /* メニューの中身(配列) */
  int     modeid;   /* メニューのモード番号 */
  struct _menustruct *prev; /* 一つ前のメニューへのポインタ */
} menustruct;

typedef struct _menuinfo {
  menustruct *mstruct; /* どのメニューの */
  int        curnum;   /* こないだ選択された番号はこれですよ */
  struct _menuinfo *next;
} menuinfo;

/* defselection で定義された記号関係の一覧をとっておく構造体 */

typedef struct {
  wchar_t	**kigo_data;	/* 一覧表示の各要素の配列 */
  wchar_t	*kigo_str;	/* 一覧表示の全要素を入れる配列 */
  int		kigo_size;	/* 要素の数 */
  int		kigo_mode;	/* そのときのモード */
} kigoIchiran;

typedef struct _selectinfo {
  kigoIchiran	*ichiran;	/* どの一覧の */
  int		curnum;		/* 前回選択された番号 */
  struct _selectinfo *next;
} selectinfo;

/* deldicinfo -- 単語削除の際に必要な辞書の情報をいれておく構造体 */

#define INDPHLENGTH 16 /* 自立語で一番長い品詞の長さ */

typedef struct _deldicinfo {
  wchar_t *name;
  wchar_t hcode[INDPHLENGTH];
} deldicinfo;
  
/*
 * glineinfo -- 候補一覧表示のための内部情報を格納しておくための構造体。
 * それぞれのメンバは以下の意味を持つ。
 *
 * glkosu -- その行にある候補の数
 * glhead -- その行の先頭候補が、kouhoinfoの何番目か(0から数える)
 * gllen  -- その行を表示するための文字列の長さ
 * gldata -- その行を表示するための文字列へのポインタ
 */

typedef struct {
  int glkosu;
  int glhead;
  int gllen;
  wchar_t *gldata;
} glineinfo;

/*
 * kouhoinfo -- 候補一覧のための内部情報を格納しておくための構造体
 * それぞれのメンバは以下の意味を持つ。
 *
 * khretsu -- その候補がある行
 * khpoint -- その候補の行のなかでの位置
 * khdata -- その候補の文字列へのポインタ
 */

typedef struct {
  int khretsu;
  int khpoint;
  wchar_t *khdata;
} kouhoinfo;

#define ROMEBUFSIZE 	1024
#define	BANGOSIZE	2	/* 候補行中の各候補の番号のコラム数 */
#define	BANGOMAX   	9	/* １候補行中の最大候補数 */

#define	KIGOBANGOMAX   	16	/* １候補行中の最大候補数 */
#define GOBISUU		9

#define	ON		1
#define	OFF		0

#define	NG		-1

#define NO_CALLBACK     (canna_callback_t)0
#define NCALLBACK	4

#define	JISHU_HIRA	0
#define JISHU_ZEN_KATA	1
#define JISHU_HAN_KATA	2
#define JISHU_ZEN_ALPHA	3
#define JISHU_HAN_ALPHA	4
#define MAX_JISHU	5

#define  SENTOU        0x01
#define  HENKANSUMI    0x02
#define  SUPKEY        0x04
#define  GAIRAIGO      0x08
#define  STAYROMAJI    0x10

/* 単語登録の品詞 */
#define MEISHI       0
#define KOYUMEISHI   1
#define DOSHI        2
#define KEIYOSHI     3
#define KEIYODOSHI   4
#define FUKUSHI      5
#define TANKANJI     6
#define SUSHI        7
#define RENTAISHI    8
#define SETSUZOKUSHI 9
#define SAHENMEISHI 10
#define MEISHIN     11
#define JINMEI      12
#define KOYUMEISHIN 13
#define GODAN       14
#define RAGYODOSHI  15
#define RAGYOGODAN  16
#define KAMISHIMO   17
#define KEIYOSHIY   18
#define KEIYOSHIN   19
#define KEIYODOSHIY 20
#define KEIYODOSHIN 21
#define FUKUSHIY    22
#define FUKUSHIN    23

/* identifier for each context structures */
#define CORE_CONTEXT       ((BYTE)0)
#define YOMI_CONTEXT       ((BYTE)1)
#define ICHIRAN_CONTEXT    ((BYTE)2)
#define FORICHIRAN_CONTEXT ((BYTE)3)
#define MOUNT_CONTEXT      ((BYTE)4)
#define TOUROKU_CONTEXT    ((BYTE)5)
#define TAN_CONTEXT	   ((BYTE)6)

typedef struct _coreContextRec {
  BYTE id;
  BYTE majorMode, minorMode;
  struct _kanjiMode *prevMode; /* １つ前のモード */
  struct _coreContextRec *next;
} coreContextRec, *coreContext;

typedef coreContext mode_context;

typedef struct  _yomiContextRec {
  /* core 情報と同じ情報 */
  BYTE id;
  BYTE majorMode, minorMode;
  struct _kanjiMode *prevMode;	/* １つ前のモード */
  mode_context    next;

  struct _kanjiMode *curMode;
  struct _tanContextRec	 *left, *right;

  /* ローマ字かな変換関係 */
  struct RkRxDic *romdic;	/* ローマ字かな変換テーブル */
  wchar_t   romaji_buffer[ROMEBUFSIZE];
  /* ローマ字バッファは rStartp, rEndp の２つのインデックスによって管理され
   * る。rStartp はカナに変換できなかったローマ字の最初の文字へのインデッ
   * クスであり、rEndp は新たにローマ字を入力する時に、格納すべき 
   * romaji_buffer 内のインデックスである。新たに入力されるローマ字は、
   * romaji_buffer + rEndp より先に格納され、そのローマ字をカナに変換す
   * る時は、romaji_buffer + rStartp から rEndp - rStartp バイトの文字が
   * 対象となる。 */
  int		  rEndp, rStartp, rCurs; /* ローマ字バッファのポインタ */
  wchar_t         kana_buffer[ROMEBUFSIZE];
  BYTE            rAttr[ROMEBUFSIZE], kAttr[ROMEBUFSIZE];
  int		  kEndp; /* かなバッファの最後を押えるポインタ */
  int             kRStartp, kCurs;

  /* その他のオプション */
  BYTE            myMinorMode;  /* yomiContext 固有のマイナモード */
  struct _kanjiMode *myEmptyMode;		/* empty モードはどれか */
  long		  generalFlags;		/* see below */
  long		  savedFlags;		/* 上のフラグの一部のセーブ */
  BYTE		  savedMinorMode;	/* マイナモードのセーブ */
  BYTE		  allowedChars;		/* see jrkanji.h */
  BYTE		  henkanInhibition;	/* see below */
  int             cursup;		/* ロかなの補追の時に使う */
#define SUSPCHARBIAS 100
  int             n_susp_chars;

/* from henkanContext */
  /* カナ漢字変換関係 */
  int            context;
  int		 kouhoCount;	/* 何回 henkanNext が連続して押されたか */
  wchar_t        echo_buffer[ROMEBUFSIZE];
  wchar_t        **allkouho; /* RkGetKanjiListで得られる文字列を配列にして
				とっておくところ */
  int            curbun;     /* カレント文節 */
  int		 curIkouho;  /* カレント候補 */
  int            nbunsetsu;  /* 文節の数 */

/* ifdef MEASURE_TIME */
  long		 proctime;   /* 処理時間(変換で計測する) */
  long		 rktime;     /* 処理時間(RKにかかる時間) */
/* endif MEASURE_TIME */
/* end of from henkanContext */

/* 逐次コンテキストから */
  int		 ye, ys, status;
/* 逐次コンテキストから(ここまで) */
  int		 cStartp, cRStartp; /* 逐次で読みとして残っている部分 */

/* 字種コンテキストから */
  BYTE           inhibition;
  BYTE           jishu_kc, jishu_case;
  int            jishu_kEndp, jishu_rEndp;
  short          rmark;
/* 字種コンテキストから(ここまで) */

/* adjustContext から */
  int kanjilen, bunlen;           /* 漢字部分、文節の長さ */
/* adjustContext から(ここまで) */
  struct _kanjiMode *tanMode; /* 単候補のときのモード */
  int tanMinorMode;     /*        〃            */

  /* 作業用変数 */
  int		  last_rule;		/* 前回のロかな変換に使われたルール */
  wchar_t	  *retbuf, *retbufp;
  int		  retbufsize;
  short           pmark, cmark; /* １つ前のマークと今のマーク */
  BYTE            englishtype;  /* 英語タイプ(以下を見よ) */
} yomiContextRec, *yomiContext;

/* for generalFlags */
#define CANNA_YOMI_MODE_SAVED		0x01L /* savedFlags にしか使われない */

#define CANNA_YOMI_BREAK_ROMAN		0x01L
#define CANNA_YOMI_CHIKUJI_MODE		0x02L
#define CANNA_YOMI_CHGMODE_INHIBITTED	0x04L
#define CANNA_YOMI_END_IF_KAKUTEI	0x08L
#define CANNA_YOMI_DELETE_DONT_QUIT	0x10L

#define CANNA_YOMI_IGNORE_USERSYMBOLS	0x20L
#define CANNA_YOMI_IGNORE_HENKANKEY	0x40L

#define CANNA_YOMI_BASE_CHIKUJI		0x80L /* 心は逐次 */

/* for generalFlags also used in savedFlags */

/* 以下の ATTRFUNCS にマスクされるビットは defmode の属性として使われる */
#define CANNA_YOMI_KAKUTEI		0x0100L
#define CANNA_YOMI_HENKAN		0x0200L
#define CANNA_YOMI_ZENKAKU		0x0400L
#define CANNA_YOMI_HANKAKU		0x0800L /* 実際に半角 */
#define CANNA_YOMI_HIRAGANA		0x1000L
#define CANNA_YOMI_KATAKANA		0x2000L
#define CANNA_YOMI_ROMAJI		0x4000L
#define CANNA_YOMI_JISHUFUNCS		0x7c00L
#define CANNA_YOMI_ATTRFUNCS		0x7f00L

#define CANNA_YOMI_BASE_HANKAKU		0x8000L /* 心は半角 */

/* kind of allowed input keys */
#define CANNA_YOMI_INHIBIT_NONE		0
#define CANNA_YOMI_INHIBIT_HENKAN	1
#define CANNA_YOMI_INHIBIT_JISHU	2
#define CANNA_YOMI_INHIBIT_ASHEX	4
#define CANNA_YOMI_INHIBIT_ASBUSHU	8
#define CANNA_YOMI_INHIBIT_ALL		15

/* 候補一覧のためのフラグ */
#define NUMBERING 			1
#define CHARINSERT			2

#define CANNA_JISHU_UPPER		1
#define CANNA_JISHU_LOWER		2
#define CANNA_JISHU_CAPITALIZE		3
#define CANNA_JISHU_MAX_CASE		4

/* englishtype */
#define CANNA_ENG_KANA			0 /* 縮小すること */
#define CANNA_ENG_ENG1			1
#define CANNA_ENG_ENG2			2 /* 両端に空白が入っている */
#define CANNA_ENG_NO			3

/* yc->status のフラグ(逐次用) */

#define	CHIKUJI_ON_BUNSETSU		0x0001 /* 文節上にある */
#define	CHIKUJI_OVERWRAP		0x0002 /* 文節かつ読み状態？ */
#define	CHIKUJI_NULL_STATUS	        0 /* 上のを消す用 */

/* yc を使うモードの区別(優先順) */

#define adjustp(yc) (0< (yc)->bunlen)
#define jishup(yc) (0 < (yc)->jishu_kEndp)
#define chikujip(yc) ((yc)->generalFlags & CANNA_YOMI_CHIKUJI_MODE)
#define henkanp(yc) (0 < (yc)->nbunsetsu)

#define chikujiyomiremain(yc) ((yc)->cStartp < (yc)->kEndp)

typedef struct _ichiranContextRec {
  BYTE id;
  BYTE majorMode, minorMode;
  struct _kanjiMode *prevMode;	/* １つ前のモード */
  mode_context    next;

  int            svIkouho;   /* カレント候補を一時とっておく(一覧表示行) */
  int            *curIkouho; /* カレント候補 */
  int            nIkouho;    /* 候補の数(一覧表示行) */
  int		 tooSmall;   /* カラム数が狭くて候補一覧が出せないよフラグ */
  int            curIchar;   /* 未確定文字列ありの単語登録の単語入力の
    							先頭文字の位置 */
  BYTE           inhibit;
  BYTE           flags;	     /* 下を見てね */
  wchar_t        **allkouho; /* RkGetKanjiListで得られる文字列を配列にして
				とっておくところ */
  wchar_t        *glinebufp; /* 候補一覧のある一行を表示するための文字
				列へのポインタ */
  kouhoinfo      *kouhoifp;  /* 候補一覧関係の情報を格納しておく構造体
				へのポインタ */
  glineinfo      *glineifp;  /* 候補一覧関係の情報を格納しておく構造体
				へのポインタ */
} ichiranContextRec, *ichiranContext;

/* フラグの意味 */
#define ICHIRAN_ALLOW_CALLBACK 1 /* コールバックをしても良い */
#define ICHIRAN_STAY_LONG    0x02 /* 選ぶと抜ける */
#define ICHIRAN_NEXT_EXIT    0x04 /* 次の quit で抜ける */


typedef struct _foirchiranContextRec {
  BYTE id;
  BYTE majorMode, minorMode;
  struct _kanjiMode *prevMode;	/* １つ前のモード */
  mode_context    next;

  int            curIkouho;  /* カレント候補 */
  wchar_t        **allkouho; /* RkGetKanjiListで得られる文字列を配列にして
				とっておくところ */
  menustruct     *table;  /* 文字列と関数のテーブル */
  int            *prevcurp;  /* 前のカレント候補 */
} forichiranContextRec, *forichiranContext;

typedef struct _mountContextRec {
  BYTE id;
  BYTE majorMode, minorMode;
  struct _kanjiMode *prevMode;	/* １つ前のモード */
  mode_context    next;

  BYTE            *mountOldStatus; /* マウントされているかいないか */
  BYTE            *mountNewStatus; /* マウントされているかいないか */
  char            **mountList;   /* マウント可能な辞書の一覧 */
  int            curIkouho;     /* カレント候補 */
} mountContextRec, *mountContext;

typedef struct _tourokuContextRec {
  BYTE id;
  BYTE majorMode, minorMode;
  struct _kanjiMode *prevMode;	/* １つ前のモード */
  mode_context    next;

  wchar_t        genbuf[ROMEBUFSIZE];
  wchar_t        qbuf[ROMEBUFSIZE];
  wchar_t        tango_buffer[ROMEBUFSIZE];
  int            tango_len;  /* 単語登録の単語の文字列の長さ */
  wchar_t        yomi_buffer[ROMEBUFSIZE];
  int            yomi_len;   /* 単語登録の読みの文字列の長さ */
  int            curHinshi;  /* 品詞の選択 */
  int            workDic;    /* 作業用の辞書 */
  deldicinfo     *workDic2;  /* 単語削除可能な辞書 */
  int            nworkDic2;  /* 単語削除可能な辞書の数 */
  deldicinfo     *workDic3;  /* 単語削除する辞書 */
  int            nworkDic3;  /* 単語削除する辞書の数 */
  struct dicname *newDic;    /* 追加する辞書 */
  wchar_t        hcode[INDPHLENGTH];  /* 単語登録の品詞 */
  int            katsuyou;   /* 単語登録の動詞の活用形 */
  wchar_t        **udic;     /* 単語登録できる辞書 (辞書名) */
  int            nudic;      /* 単語登録できる辞書の数 */
  int            delContext; /* 単語削除で１つの辞書をマウントする */
} tourokuContextRec, *tourokuContext;

typedef struct _tanContextRec {
  BYTE id;
  BYTE majorMode, minorMode;
  struct _kanjiMode *prevMode;	/* １つ前のモード */
  mode_context    next;

  struct _kanjiMode *curMode;
  struct _tanContextRec	 *left, *right;

  struct RkRxDic *romdic;	/* ローマ字かな変換テーブル */
  BYTE            myMinorMode;  /* yomiContext 固有のマイナモード */
  struct _kanjiMode *myEmptyMode;		/* empty モードはどれか */
  long generalFlags, savedFlags; /* yomiContext のコピー */
  BYTE		  savedMinorMode;	/* マイナモードのセーブ */
  BYTE		  allowedChars;		/* see jrkanji.h */
  BYTE		  henkanInhibition;	/* see below */

  wchar_t *kanji, *yomi, *roma;
  BYTE *kAttr, *rAttr;
} tanContextRec, *tanContext;

struct moreTodo {
  BYTE          todo; /* もっとあるの？を示す */
  BYTE          fnum; /* 関数番号。０なら次の文字で示されることをする */
  int		ch;   /* 文字 */
};

/* モード名を格納するデータの型定義 */

struct ModeNameRecs {
  int           alloc;
  wchar_t       *name;
};

/* 一覧の番号のセパレーターのデフォルトの定義 */

#define DEFAULTINDEXSEPARATOR     '.'

/*
   wcKanjiAttribute for internal use
 */

typedef struct {
  wcKanjiAttribute u;
  int len;
  char *sp, *ep;
} wcKanjiAttributeInternal;

/* 

  uiContext はローマ字かな変換、カナ漢字変換に使われる構造体である。
  XLookupKanjiString などによる変換は、ウィンドウに分離された複数の入
  力ポートに対応しているので、入力中のローマ字の情報や、カナ漢字変換
  の様子などをそれぞれのウィンドウ毎に分離して保持しておかなければな
  らない。この構造体はそのために使われる構造体である。
 
  構造体のメンバがどのようなものがあるかは、定義を参照すること
 
 */

typedef struct _uiContext {

  /* XLookupKanjiStringのパラメタ */
  wchar_t        *buffer_return;
  int            n_buffer;
  wcKanjiStatus    *kanji_status_return;

  /* XLookupKanjiStringの戻り値である文字列の長さ */
  int		 nbytes;

  /* キャラクタ */
  int ch;

  /* セミグローバルデータ */
  int		 contextCache;	 /* 変換コンテクストキャッシュ */
  struct _kanjiMode *current_mode;
  BYTE		 majorMode, minorMode;	 /* 直前のもの */

  short		 curkigo;	 /* カレント記号(記号全般) */
  char           currussia;	 /* カレント記号(ロシア文字) */
  char           curgreek;	 /* カレント記号(ギリシャ文字) */
  char           curkeisen;	 /* カレント記号(罫線) */
  short          curbushu;       /* カレント部首名 */
  int            ncolumns;	 /* 一行のコラム数、候補一覧の時に用いられる */
  wchar_t        genbuf[ROMEBUFSIZE];	/* 汎用バッファ */
  short          strokecounter;  /* キーストロークのカウンタ
				    ローマ字モードでクリアされる */
  wcKanjiAttributeInternal *attr;

  /* リストコールバック関連 */
  char           *client_data;   /* アプリケーション用データ */
  int            (*list_func)(char *, int, wchar_t **, int, int *);
  jrEUCListCallbackStruct elistcb; /* EUCの場合の実体(旧wcも兼用) */
                 /* リストコールバック関数 */
  /* その他 */
  char		 flags;		 /* 下を見てね */
  char		 status;	 /* どのような状態で返ったのかを示す値
				    そのモードとして、
				     ・処理中
				     ・処理終了
				     ・処理中断
				     ・その他
				    がある。(下を見よ) */

  /* コールバックチェーン */
  struct callback *cb;

  /* もっとすることがあるよという構造体 */
  struct moreTodo more;

  /* クイットチェーン */
  menustruct *prevMenu;

  /* 各メニューで選ばれた番号を記録しておく構造体へのポインタ */
  menuinfo *minfo;

  /* 各一覧で選ばれた番号を記録しておく構造体へのポインタ */
  selectinfo *selinfo;

  /* サブコンテクストへのリンク */
  mode_context   modec;		/* 全部ここにつなぐ予定 */
} uiContextRec, *uiContext;

/* uiContext の flags のビットの意味 */
#define PLEASE_CLEAR_GLINE	1	/* GLine を消してね */
#define PCG_RECOGNIZED		2	/* GLine を次は消しますよ */
#define MULTI_SEQUENCE_EXECUTED	4	/* さっきマルチシーケンスが行われた */

#define EVERYTIME_CALLBACK	0
#define EXIT_CALLBACK		1
#define QUIT_CALLBACK		2
#define AUX_CALLBACK		3

/* 
 * カナ漢字変換のための様々なキーマップテーブル 
 * キーマップテーブルは処理関数へのポインタの配列となっている。
 */

struct funccfunc {
  BYTE funcid;
  int (*cfunc)(struct _uiContext *);
};

typedef struct _kanjiMode {
  int (*func)(struct _uiContext *, struct _kanjiMode *, int, int, int);
  BYTE *keytbl;
  int flags;			/* 下を見よ */
  struct funccfunc *ftbl;
} *KanjiMode, KanjiModeRec;

struct callback {
  int (*func[NCALLBACK])(struct _uiContext *, int, mode_context);
  mode_context    env;
  struct callback *next;
};

/* ローマ字かな変換テーブル */
     
extern struct RkRxDic *romajidic;
extern struct RkRxDic *RkwOpenRoma(char *);

/*
 * 辞書の名前を入れておく変数
 */

struct dicname {
  struct dicname *next;
  char *name;
  int dictype;
  unsigned long dicflag;
};

/* dictype には以下のいずれかが入る */
#define DIC_PLAIN 0     /* 通常の利用 */
#define DIC_USER  1     /* 単語登録用辞書 */
#define DIC_BUSHU 2     /* 部首変換用辞書 */
#define DIC_GRAMMAR 3   /* 文法辞書 */
#define DIC_RENGO 4     /* 連語学習辞書 */
#define DIC_KATAKANA 5  /* カタカナ学習辞書 */
#define DIC_HIRAGANA 6  /* ひらがな学習辞書 */

/* dicflag には以下のいずれかが入る */
#define DIC_NOT_MOUNTED  0
#define DIC_MOUNTED      1
#define DIC_MOUNT_FAILED 2

extern struct dicname *kanjidicnames;

/*
 * エラーのメッセージを入れておく変数
 */

extern char *jrKanjiError;

/*
 * デバグ文を表示するかどうかのフラグ
 */

extern int iroha_debug;

/*
 * キーシーケンスを発生するようなキー
 */

#define IrohaFunctionKey(key) \
  ((0x80 <= (int)(unsigned char)(key) &&  \
    (int)(unsigned char)(key) <= 0x8b) || \
   (0x90 <= (int)(unsigned char)(key) &&  \
    (int)(unsigned char)(key) <= 0x9b) || \
   (0xe0 <= (int)(unsigned char)(key) &&  \
    (int)(unsigned char)(key) <= 0xff) )

/* selectOne でコールバックを伴うかどうかを表すマクロ */

#define WITHOUT_LIST_CALLBACK 0
#define WITH_LIST_CALLBACK    1

/*
 * Rk 関数をトレースするための名前の書き換え。
 */

#ifdef DEBUG
#include "traceRK.h"
#endif /* DEBUG */

/*
 * デバグメッセージ出力用のマクロ
 */

#ifdef DEBUG
#define debug_message(fmt, x, y, z)	dbg_msg(fmt, x, y, z)
#else /* !DEBUG */
#define debug_message(fmt, x, y, z)
#endif /* !DEBUG */

/*
 * malloc のデバグ
 */

#ifdef DEBUG_ALLOC
extern char *debug_malloc(int);
extern int fail_malloc;
#define malloc(n) debug_malloc(n)
#endif /* DEBUG_MALLOC */

/*
 * 新しいモードを定義する構造体
 */

typedef struct {
  char           *romaji_table; /* ローマ字かな変換テーブル名(EUC) */
  struct RkRxDic *romdic;	 /* ローマ字辞書構造体 */
  int             romdic_owner;  /* ローマ字辞書を自分でOpenしたか */
  long            flags;	 /* flags for yomiContext->generalFlags */
  KanjiMode       emode;	 /* current_mode に入る構造体 */
} newmode;

/* ローマ字かな変換を補足するキーと文字の変換テーブル */

typedef struct {
  wchar_t	key;		/* キー */
  wchar_t       xkey;
  int		groupid;	/* グループid */
  int           ncand;          /* 候補の数 */
  wchar_t       **cand;         /* 候補の配列 */
  wchar_t	*fullword;	/* 候補列 (候補1@候補2@...候補n@@) */
#ifdef WIN_CANLISP
  int		fullwordsize;	/* sizeof fullword by wchar_t unit */
#endif
} keySupplement;

#define MAX_KEY_SUP 64

#define HEX_USUAL     0
#define HEX_IMMEDIATE 1

#define ModeInfoStyleIsString		0
#define ModeInfoStyleIsNumeric		1
#define ModeInfoStyleIsExtendedNumeric	2
#define ModeInfoStyleIsBaseNumeric      3
#define MaxModeInfoStyle                ModeInfoStyleIsBaseNumeric

#define killmenu(d) ((d)->prevMenu = (menustruct *)0)
#define	defineEnd(d) killmenu(d)
#define	deleteEnd(d) killmenu(d)

/* defmode、defselection、defmenu 用の構造体 */

typedef struct _extra_func {
  int  		fnum;		/* 関数番号 */
  int		keyword;	/* 新しいモードが定義されたキーワード */
  wchar_t	*display_name;	/* モード表示名 */
  union {
    newmode 	*modeptr;	/* defmode に対応する構造体 */
    kigoIchiran	*kigoptr;	/* defselection に対応する構造体 */
    menustruct	*menuptr;	/* defmenu に対応する構造体 */
  } u;
#ifdef BINARY_CUSTOM
  int           mid;
  char          *symname;
#endif
  struct _extra_func *next;
} extraFunc;

#define EXTRA_FUNC_DEFMODE	1
#define EXTRA_FUNC_DEFSELECTION	2
#define EXTRA_FUNC_DEFMENU	3

#define tanbunMode(d, tan) /* tanContext 関連モードへの移行 */ \
  { extern KanjiModeRec tankouho_mode; (d)->current_mode = &tankouho_mode; \
    (d)->modec = (mode_context)(tan); currentModeInfo(d); }

#define freeForIchiranContext(fc) free((char *)fc)
#define freeIchiranContext(ic) free((char *)ic)
#define freeYomiContext(yc) free((char *)yc)
#define freeCoreContext(cc) free((char *)cc)

#ifndef DICHOME
#define DICHOME "/usr/lib/canna/dic"
#endif

#define DEFAULT_CANNA_SERVER_NAME "cannaserver"

#ifndef	_UTIL_FUNCTIONS_DEF_

#define	_UTIL_FUNCTIONS_DEF_

/* かんなのバージョンを調べる */
#define canna_version(majv, minv) ((majv) * 1024 + (minv))

/* よくスペルミスするのでコンパイル時にひっかかるように入れる */
extern int RkwGoto(char *, int);

/* storing customize configuration to the following structure. */
struct CannaConfig { /* 以下のコメントはダイアログなどに記述するときなどに
			用いる語彙。! が先頭についているのはロジックが反転
			していることを表す */
  int CannaVersion;  /* (互換用) かんなのバージョン */
  int kouho_threshold; /* 変換キーを何回打つと一覧が出るか */
  int strokelimit;  /* (互換用) 何ストロークアルファベットを入れると切断か */
  int indexSeparator; /* (互換用) 一覧時のインデックスのセパレータ */
  BYTE ReverseWidely; /* 反転領域を広くとる       */
  BYTE chikuji;       /* 逐次自動変換             */
  BYTE Gakushu;       /* 学習するかどうか         */
  BYTE CursorWrap;    /* 右端で右で左端へ行く     */
  BYTE SelectDirect;  /* 一覧時、選択で一覧を抜ける */
  BYTE HexkeySelect;  /* (互換用) 16進数字でも一覧選択可 */
  BYTE BunsetsuKugiri; /* 変換時文節間に空白を挿入  */
  BYTE ChBasedMove;   /* !ローマ字かな変換単位のカーソル移動   */
  BYTE ReverseWord;   /* (互換用) 一覧で語を反転する */
  BYTE QuitIchiranIfEnd; /* 一覧末尾で一覧を閉じる */
  BYTE kakuteiIfEndOfBunsetsu; /* 文節末尾で右移動で確定する */
  BYTE stayAfterValidate; /* !一覧で選択後次の文節へ移動 */
  BYTE BreakIntoRoman;    /* BSキーでローマ字へ戻す */
  BYTE grammaticalQuestion; /* (互換用) 単語登録時文法的質問をする */
  BYTE forceKana;           /* Isn't this used? */
  BYTE kCount;        /* (互換用) 候補が何番目かを表示する */
  BYTE LearnNumericalType;  /* Isn't this used? */
  BYTE BackspaceBehavesAsQuit; /* 逐次自動変換時 BS キーで全体を読みに戻す */
  BYTE iListCB;       /* (互換用) リストコールバックを禁止する */
  BYTE keepCursorPosition;  /* !変換時にBS打鍵時カーソル位置を末尾に移動 */
  BYTE abandonIllegalPhono; /* ローマ字かな変換に使われないキーを捨てる */
  BYTE hexCharacterDefiningStyle; /* Isn't this used? */
  BYTE kojin;         /* 個人学習 */
  BYTE indexHankaku;  /* (互換用) 一覧時のインデックスを半角にする */
  BYTE allowNextInput; /* 候補一覧表示時、次の入力が可能にする */
  BYTE doKatakanaGakushu; /* Isn't this used? */
  BYTE doHiraganaGakushu; /* Isn't this used? */ 
  BYTE ChikujiContinue; /* 逐次自動変換時次の入力で既変換部分を確定しない */
  BYTE RenbunContinue;  /* 連文節変換時次の入力で既変換部分を確定しない */
  BYTE MojishuContinue; /* 字種変換時次の入力で既変換部分を確定しない */
  BYTE chikujiRealBackspace; /* 逐次自動変換時BSで必ず一文字消去する */
  BYTE ignore_case;   /* 大文字小文字を区別しない */
  BYTE romaji_yuusen; /* ローマ字かな変換を優先する */
  BYTE auto_sync;     /* 定期的に辞書を書き戻す */
  BYTE quickly_escape; /* (互換用) 一覧表示時、選択で即座に一覧を抜ける */
  BYTE InhibitHankakuKana; /* 半角カタカナの禁止 */
  BYTE code_input;    /* コード(0: jis, 1: sjis, 2: 区点) */
  BYTE DelayConnect;  /* 初期化時にすぐにサーバに接続しない */
};

#define CANNA_CODE_JIS   0
#define CANNA_CODE_SJIS  1
#define CANNA_CODE_KUTEN 2
#define CANNA_MAX_CODE   3

typedef int (* canna_callback_t)(uiContext, int, mode_context);
typedef void (* canna_keyconv_callback_t)(int, char *, char *, int);
struct map;

/* for VC++ 4.0, by kon */

/* RKkana.c */
extern int RkCvtNone(unsigned char *, int, unsigned char *, int);
extern int RkCvtEuc(unsigned char *, int, unsigned char *, int);
extern int RkwCvtNone(wchar_t *, int, wchar_t *, int);
extern int RkwCvtHan(wchar_t *, int, wchar_t *, int);
extern int RkwCvtHira(wchar_t *, int, wchar_t *, int);
extern int RkwCvtKana(wchar_t *, int, wchar_t *, int);
extern int RkwCvtZen(wchar_t *, int, wchar_t *, int);
extern int RkwMapPhonogram(struct RkRxDic *, wchar_t *, int, wchar_t *, int, wchar_t, int,
       int *, int *, int *, int *);
extern int RkwCvtRoma(struct RkRxDic *, wchar_t *, int, wchar_t *, int, int);
extern int RkMapRoma(struct RkRxDic *, unsigned char *, int, unsigned char *, int,
       int, int *);
extern int RkMapPhonogram(struct RkRxDic *, unsigned char *, int, unsigned char *, int,
       unsigned, int,
       int *, int *, int *, int *);
extern int RkCvtRoma(struct RkRxDic *, unsigned char *, int, unsigned char *, int,
       unsigned);

/* bunsetsu.c */
extern int enterAdjustMode(uiContext, yomiContext);
extern int leaveAdjustMode(uiContext, yomiContext);
extern void popForIchiranMode(uiContext);

/* bushu.c */
extern int initBushuTable(void);
extern int getForIchiranContext(uiContext);
extern int BushuMode(uiContext);
extern int ConvertAsBushu(uiContext);

/* chikuji.c */
extern void clearHenkanContext(yomiContext);
extern int chikujiInit(uiContext);
extern int ChikujiSubstYomi(uiContext);
extern int ChikujiTanDeletePrevious(uiContext);
extern void moveToChikujiTanMode(uiContext);
extern void moveToChikujiYomiMode(uiContext);

/* commondata.c */
extern struct CannaConfig cannaconf;
extern int (*jrBeepFunc)(void);
extern canna_keyconv_callback_t keyconvCallback;
extern void InitCannaConfig(struct CannaConfig *);
extern void restoreBindings(void);

/* defaultmap.c */
extern int searchfunc(uiContext, KanjiMode, int, int, int);
extern int CYsearchfunc(uiContext, KanjiMode, int, int, int);

/* ebind.c */
extern int EUCListCallback(char *, int, wchar_t **, int, int *);
extern int XKanjiControl2(unsigned int, unsigned int, unsigned int, BYTE *);

/* empty.c */
extern extraFunc *FindExtraFunc(int);
extern int getBaseMode(yomiContext);
extern void EmptyBaseModeInfo(uiContext, yomiContext);
extern int EmptyBaseHira(uiContext);
extern int EmptyBaseKata(uiContext);
extern int EmptyBaseEisu(uiContext);
extern int EmptyBaseZen(uiContext);
extern int EmptyBaseHan(uiContext);
extern int EmptyBaseKana(uiContext);
extern int EmptyBaseKakutei(uiContext);
extern int EmptyBaseHenkan(uiContext);

/* engine.c */
extern int RkSetServerName(char *);
extern exp(char *) RkwGetServerName(void);
extern char *RkGetServerHost(void);
extern char *RkGetServerEngine(void);
extern int RkwSetAppName(int, char *);
extern void close_engine(void);

/* hex.c */
extern int HexMode(uiContext);

/* henkan.c */
extern int KanjiInit(void);
extern int KanjiFin(void);
extern void freeTanContext(tanContext);
extern yomiContext newFilledYomiContext(mode_context, KanjiMode);
extern int TanBubunMuhenkan(uiContext);
extern int prepareHenkanMode(uiContext);
extern int doHenkan(uiContext, int, wchar_t *);
extern int TanNop(uiContext);
extern int TanForwardBunsetsu(uiContext);
extern int TanBackwardBunsetsu(uiContext);
extern int TanKouhoIchiran(uiContext);
extern int TanNextKouho(uiContext);
extern int TanPreviousKouho(uiContext);
extern int TanHiragana(uiContext);
extern int TanKatakana(uiContext);
extern int TanRomaji(uiContext);
extern int TanUpper(uiContext);
extern int TanCapitalize(uiContext);
extern int TanZenkaku(uiContext);
extern int TanHankaku(uiContext);
extern int TanKanaRotate(uiContext);
extern int TanRomajiRotate(uiContext);
extern int TanCaseRotateForward(uiContext);
extern int YomiBubunKakutei(uiContext);
extern int TanBeginningOfBunsetsu(uiContext);
extern int TanEndOfBunsetsu(uiContext);
extern int tanMuhenkan(uiContext, int);
extern int TanMuhenkan(uiContext);
extern int TanDeletePrevious(uiContext);
extern void finishTanKakutei(uiContext);
extern int TanKakutei(uiContext);
extern int TanPrintBunpou(uiContext);
extern void jrKanjiPipeError(void);
extern int TbForward(uiContext);
extern int TbBackward(uiContext);
extern int TbBeginningOfLine(uiContext);
extern int TbEndOfLine(uiContext);

/* ichiran.c */
extern int initIchiran(void);
extern int allocIchiranBuf(uiContext);
extern ichiranContext newIchiranContext(void);
extern wchar_t **getIchiranList(int, int *, int *);
extern void makeGlineStatus(uiContext);
extern void freeIchiranBuf(ichiranContext);
extern void freeGetIchiranList(wchar_t **);
extern int selectOne(uiContext, wchar_t **, int *, int, int, unsigned, int, int,
       canna_callback_t, canna_callback_t, canna_callback_t,canna_callback_t);
extern int tanKouhoIchiran(uiContext, int);
extern int IchiranQuit(uiContext);
extern int IchiranNop(uiContext);
extern int IchiranForwardKouho(uiContext);
extern int IchiranBackwardKouho(uiContext);
extern int IchiranPreviousKouhoretsu(uiContext);
extern int IchiranNextKouhoretsu(uiContext);
extern int IchiranBeginningOfKouho(uiContext);
extern int IchiranEndOfKouho(uiContext);
extern void ichiranFin(uiContext);

/* jishu.c */
extern void enterJishuMode(uiContext, yomiContext);
extern void leaveJishuMode(uiContext, yomiContext);

/* jrbind.c */
extern int XwcLookupKanji2(unsigned int, unsigned int, wchar_t *, int,
                                int, int, wcKanjiStatus *);
extern int XwcKanjiControl2(unsigned int, unsigned int, unsigned int, BYTE *);
extern struct callback *pushCallback(uiContext, mode_context,
       canna_callback_t, canna_callback_t,
       canna_callback_t, canna_callback_t);
extern void popCallback(uiContext);

/* kctrl.c */
extern uiContext keyToContext(unsigned int, unsigned int);
extern int initRomeStruct(uiContext, int);
extern void freeRomeStruct(uiContext);
extern struct bukRec *internContext(unsigned int, unsigned int, uiContext);
extern void rmContext(unsigned int, unsigned int);
extern void addWarningMesg(char *);
extern int escapeToBasicStat(uiContext, int);
extern void makeAllContextToBeClosed(int);
extern int _doFunc(uiContext, int);
extern int _afterDoFunc(uiContext, int);
extern int doFunc(uiContext, int);
/* extern int kanjiControl(int, uiContext, caddr_t); */

/* keydef.c */
extern int initKeyTables(void);
extern void restoreDefaultKeymaps(void);
extern unsigned char *actFromHash(unsigned char *, int);
extern struct map *mapFromHash(KanjiMode tbl, int key, struct map ***ppp);
extern int askQuitKey(unsigned);

/* kigo.c */
extern void initKigoTable(void);
extern int KigoIchiran(uiContext);

/* lisp.c */
extern int YYparse_by_rcfilename(char *);
extern void clisp_fin(void);
extern int parse_string(char *);
extern void clisp_main(void);

/* mode.c */
extern newmode *findExtraKanjiMode(int);
extern void currentModeInfo(uiContext);
extern void initModeNames(void);
extern void resetModeNames(void);
extern int JapaneseMode(uiContext);
extern int AlphaMode(uiContext);
extern int HenkanNyuryokuMode(uiContext);
extern int queryMode(uiContext, wchar_t *);

/* multi.c */
extern int UseOtherKeymap(uiContext);

/* obind.c */
#if SUPPORT_OLD_WCHAR
extern int owcListCallback(char *, int, wchar_t **, int, int *);
#endif

/* parse.c */
#ifdef BINARY_CUSTOM
extern int binparse(void);
#else
extern void parse(void);
#endif

/* romaji.c */
extern void kPos2rPos(yomiContext, int, int, int *, int *);
extern void makeYomiReturnStruct(uiContext);
extern int RomkanaInit(void);
extern void RomkanaFin(void);
extern yomiContext
  newYomiContext(wchar_t *, int, int, int, int, int),
  GetKanjiString(uiContext, wchar_t *, int, int, int, int, int,
		      canna_callback_t, canna_callback_t, canna_callback_t);
extern void popYomiMode(uiContext);
extern void removeCurrentBunsetsu(uiContext, tanContext);
extern int YomiInsert(uiContext);
extern int findSup(wchar_t);
extern void moveStrings(wchar_t *, BYTE *, int, int, int);
extern int RomajiFlushYomi(uiContext, wchar_t *, int);
extern void restoreFlags(yomiContext);
extern int xString(wchar_t *, int, wchar_t *, wchar_t *);
extern int appendTan2Yomi(tanContext, yomiContext);
extern void doMuhenkan(uiContext, yomiContext);
extern int doKakutei(uiContext, tanContext, tanContext, wchar_t *, wchar_t *, yomiContext *);
extern int cutOffLeftSide(uiContext, yomiContext, int);
extern int YomiKakutei(uiContext);
extern void clearYomiContext(yomiContext);
extern void RomajiClearYomi(uiContext);
extern int YomiExit(uiContext, int);
extern void RomajiStoreYomi(uiContext, wchar_t *, wchar_t *);
extern int KanaDeletePrevious(uiContext);
extern int alphaMode(uiContext);
extern int YomiQuotedInsert(uiContext);
extern int selectKeysup(uiContext, yomiContext, int);
extern int containUnconvertedKey(yomiContext);
extern int YomiBaseHiraKataToggle(uiContext);
extern int YomiBaseZenHanToggle(uiContext);
extern int YomiBaseRotateForw(uiContext);
extern int YomiBaseRotateBack(uiContext);
extern int YomiBaseKanaEisuToggle(uiContext);
extern int YomiBaseKakuteiHenkanToggle(uiContext);
extern int YomiModeBackup(uiContext);
extern int exitJishu(uiContext);
extern int YomiMark(uiContext);
extern int Yomisearchfunc(uiContext, KanjiMode, int, int, int);
extern int TanBubunKakutei(uiContext);
extern void removeKana(uiContext, yomiContext, int, int);

/* uiutil.c */
#ifndef NO_EXTEND_MENU
extern void freeMenu(menustruct *);
extern menustruct *allocMenu(int, int);
extern void finExtMenu(void);
#endif
extern int initExtMenu(void);
extern int uiUtilIchiranTooSmall(uiContext, int, mode_context);
#ifndef NO_EXTEND_MENU
extern int prevMenuIfExist(uiContext);
#endif
extern int UiUtilMode(uiContext);
#ifndef NO_EXTEND_MENU
extern void freeAllMenuInfo(menuinfo *);
extern int showmenu(uiContext, menustruct *);
#endif

/* uldefine.c */
extern int dicTouroku(uiContext);
extern int initHinshiTable(void);
extern int dicTourokuTango(uiContext, canna_callback_t);
extern int dicTourokuControl(uiContext, wchar_t *, canna_callback_t);
extern int uuTTangoQuitCatch(uiContext, int, mode_context);

/* uldelete.c */
extern int dicSakujo(uiContext);
extern void freeDic(tourokuContext);
extern void freeAndPopTouroku(uiContext);

/* ulhinshi.c */
extern int initHinshiMessage(void);
extern int initGyouTable(void);
extern int dicTourokuHinshiDelivery(uiContext);
extern int dicTourokuDictionary(uiContext, canna_callback_t, canna_callback_t);

/* ulserver.c */
extern int serverChange(uiContext);
extern int serverFin(uiContext);

/* ulmount.c */
extern int getMountContext(uiContext);
extern int dicMount(uiContext);
extern void popMountMode(uiContext);

/* onoff.c */
extern int initOnoffTable(void);
extern int selectOnOff(uiContext, wchar_t **, int *, int, int, int,
       unsigned char *, canna_callback_t, canna_callback_t,
       canna_callback_t, canna_callback_t);

/* ulkigo.c */
extern int initUlKigoTable(void);
extern int initUlKeisenTable(void);
extern int uuKigoGeneralExitCatch(uiContext, int, mode_context);
extern int uuKigoMake(uiContext, wchar_t **, int, int, int, canna_callback_t,
       int *);
extern int kigoRussia(uiContext);
extern int kigoGreek(uiContext);
extern int kigoKeisen(uiContext);

/* util.c */
extern void GlineClear(uiContext);
extern void echostrClear(uiContext);
extern int NothingChanged(uiContext);
extern int NothingForGLine(uiContext);
extern void CannaBeep(void);
extern int checkGLineLen(uiContext);
extern int NothingChangedWithBeep(uiContext);
extern int NothingForGLineWithBeep(uiContext);
extern int extractTanString(tanContext, wchar_t *, wchar_t *);
extern int extractTanYomi(tanContext, wchar_t *, wchar_t *);
extern int extractTanRomaji(tanContext, wchar_t *, wchar_t *);
extern void makeKanjiStatusReturn(uiContext, yomiContext);
extern void makeGLineMessage(uiContext, wchar_t *, int);
extern void makeGLineMessageFromString(uiContext, char *);
extern int setWStrings(wchar_t **, char **, int);
extern int NoMoreMemory(void);
extern int GLineNGReturn(uiContext);
extern int GLineNGReturnFI(uiContext);
extern int WStrlen(wchar_t *);
extern wchar_t *WStrcpy(wchar_t *, wchar_t *);
extern wchar_t *WStrncpy(wchar_t *, wchar_t *, int);
extern wchar_t *WStraddbcpy(wchar_t *, wchar_t *, int);
extern wchar_t *WStrcat(wchar_t *, wchar_t *);
extern int WStrcmp(wchar_t *, wchar_t *);
extern int WStrncmp(wchar_t *, wchar_t *, int);
extern int WWhatGPlain(wchar_t);
extern int WIsG0(wchar_t), WIsG1(wchar_t);
extern int WIsG2(wchar_t), WIsG3(wchar_t);
extern int CANNA_mbstowcs(wchar_t *, char *, int);
extern int CNvW2E(wchar_t *, int, char *, int);
extern int CANNA_wcstombs(char *, wchar_t *, int);
extern int WStringOpen(void);
extern wchar_t *WString(char *);
extern void WStringClose(void);
extern int WSfree(wchar_t *);
extern void generalReplace(wchar_t *, BYTE *, int *, int *, int *, int, wchar_t *, int, int);
extern int WToupper(wchar_t);
extern int WTolower(wchar_t);
extern wchar_t key2wchar(int, int *);
extern int confirmContext(uiContext, yomiContext);
extern int abandonContext(uiContext, yomiContext);
extern int makeRkError(uiContext, char *);
extern char *KanjiInitError(void);
extern int canna_alert(uiContext, char *, canna_callback_t);

/* yesno.c */
extern int getYesNoContext(uiContext, canna_callback_t, canna_callback_t,
       canna_callback_t, canna_callback_t);

/* unknown */
extern int WCinit(void);
extern int DoFuncSequence(uiContext);
extern void makeGLineMessageFromStrings(uiContext, char *);
extern void prepare_autodic(void);

#endif /* _UTIL_FUNCTIONS_DEF_ */

#ifndef wchar_t
# error "wchar_t is already undefined"
#endif
#undef wchar_t
/*********************************************************************
 *                       wchar_t replace end                         *
 *********************************************************************/
#endif /* !_CANNA_H_ */
