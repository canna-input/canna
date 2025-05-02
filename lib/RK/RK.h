/* Copyright 1993 NEC Corporation, Tokyo, Japan.
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

/* $Id: RK.h,v 1.10 2003/09/21 10:16:49 aida_s Exp $ */
#ifndef		_RK_h
#define		_RK_h


#ifdef canna_export
# define CANNA_EXP_PREDEFINED
#else /* canna_export has not been not defined */
# define canna_export(x) x
#endif

#ifdef pro
#define CANNA_PRO_PREDEFINED
#else
#if defined(__STDC__) || defined(__cplusplus)
#define pro(x) x
#else
#define pro(x) ()
#endif
#endif

#include <canna/sysdep.h>

typedef	struct {
   int		ylen;		/* yomigana no nagasa (in byte) */ 
   int		klen;		/* kanji no nagasa (in byte) */
   int		rownum;		/* row number */
   int		colnum;		/* column number */
   int		dicnum;		/* dic number */
}		RkLex;

typedef	struct {
   int		bunnum;		/* bunsetsu bangou */
   int		candnum;	/* kouho bangou */
   int		maxcand;  	/* sou kouho suu */
   int		diccand;	/* jisho ni aru kouho suu */
   int		ylen;		/* yomigana no nagasa (in byte) */ 
   int		klen;		/* kanji no nagasa (in byte) */
   int		tlen;		/* tango no kosuu */
}		RkStat;

struct DicInfo {
    unsigned char	*di_dic;
    unsigned char	*di_file;
    int			di_kind;
    int			di_form;
    unsigned		di_count;
    int			di_mode;
    long		di_time;
};

/* romaji/kanakanji henkan code */
#define	RK_XFERBITS	4	/* bit-field width */
#define	RK_XFERMASK	((1<<RK_XFERBITS)-1)
#define	RK_NFER		0	/* muhenkan */
#define	RK_XFER		1	/* hiragana henkan */
#define	RK_HFER		2	/* hankaku henkan */
#define	RK_KFER		3	/* katakana henkan */
#define	RK_ZFER		4	/* zenkaku  henkan */

#define	RK_CTRLHENKAN		0xf
#define	RK_HENKANMODE(flags)	(((flags)<<RK_XFERBITS)|RK_CTRLHENKAN)

#define RK_TANBUN		0x01
#define RK_MAKE_WORD		0x02
#define RK_MAKE_EISUUJI		0x04
#define RK_MAKE_KANSUUJI	0x08

/* RkRxDic
 *	romaji/kana henkan jisho 
 */
struct RkRxDic	{
    int                 dic;		/* dictionary version: see below */
    unsigned char	*nr_string;	/* romaji/kana taiou hyou */
    int			nr_strsz;	/* taiou hyou no size */
    unsigned char	**nr_keyaddr;	/* romaji key no kaishi iti */
    int			nr_nkey;	/* romaji/kana taiou suu */
    unsigned char       *nr_bchars;     /* backtrack no trigger moji */
    unsigned char       *nr_brules;     /* backtrack no kanouseino aru rule */
};

#define RX_KPDIC 0 /* new format dictionary */
#define RX_RXDIC 1 /* old format dictionary */
#define RX_PTDIC 2 /* large format dictionary (almost equal to KPDIC) */

/* kanakanji henkan */

/* romaji hennkan code */
#define	RK_FLUSH	0x8000	/* flush */
#define	RK_SOKON	0x4000	/* sokuon shori */
#define RK_IGNORECASE	0x2000  /* ignore case */

#define	RK_BIN		0
#define	RK_TXT		0x01

#define	RK_MWD	        0
#define	RK_SWD		1
#define	RK_PRE		2
#define	RK_SUC		3

#define KYOUSEI		0x01		/* jisho_overwrite_mode */

#define	Rk_MWD		0x80		/* jiritsugo_jisho */
#define	Rk_SWD		0x40		/* fuzokugo_jisho */
#define	Rk_PRE		0x20		/* settougo_jisho */
#define	Rk_SUC		0x10		/* setsubigo_jisho */

/* permission for RkwChmod() */
#define RK_ENABLE_READ   0x01
#define RK_DISABLE_READ  0x02
#define RK_ENABLE_WRITE  0x04
#define RK_DISABLE_WRITE 0x08
/* chmod for directories */
#define RK_USR_DIR       0x3000
#define RK_GRP_DIR       0x1000
#define RK_SYS_DIR       0x2000
#define RK_DIRECTORY     (RK_USR_DIR | RK_GRP_DIR | RK_SYS_DIR)
/* chmod for dictionaries */
#define RK_USR_DIC       0	/* specify user dic */
#define RK_GRP_DIC       0x4000	/* specify group dic */
#define RK_SYS_DIC       0x8000	/* specify system dic */

#define PL_DIC		 0x0100
#define PL_ALLOW	 0x0200
#define PL_INHIBIT	 0x0400
#define PL_FORCE	 0x0800

#define	NOENT	-2	/* No such file or directory		*/
#define	IO	-5	/* I/O error				*/
#define	NOTALC	-6	/* Cann't alloc. 			*/
#define	BADF	-9	/* irregal argument			*/
#define	BADDR	-10	/* irregal dics.dir	 		*/
#define	ACCES	-13	/* Permission denied 			*/
#define	NOMOUNT	-15	/* cannot mount				*/
#define	MOUNT	-16	/* file already mounted			*/
#define	EXIST	-17	/* file already exits			*/
#define	INVAL	-22	/* irregal argument			*/
#define	TXTBSY	-26	/* text file busy			*/
#define BADARG	-99	/* Bad Argment				*/
#define BADCONT -100	/* Bad Context				*/
#define OLDSRV    -110
#define NOTUXSRV  -111
#define NOTOWNSRV -112

/* kanakanji henkan */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CANNAWC_DEFINED
# if defined(CANNA_NEW_WCHAR_AWARE)
#  define CANNAWC_DEFINED
#  ifdef CANNA_WCHAR16
typedef canna_uint16_t cannawc;
#  else
typedef canna_uint32_t cannawc;
#  endif
# elif defined(_WCHAR_T)
#  error "You can't use old wide character for RK interface"
# endif
#endif

#ifdef CANNAWC_DEFINED

canna_export(void) RkwFinalize pro((void));
canna_export(int) RkwInitialize pro((char *));
canna_export(int) RkwCreateContext pro((void));
canna_export(int) RkwCloseContext pro((int));
canna_export(int) RkwDuplicateContext pro((int));
canna_export(int) RkwSetDicPath pro((int, char *));
canna_export(int) RkwGetDirList pro((int, char *,int));
canna_export(int) RkwGetDicList pro((int, char *,int));
canna_export(int) RkwMountDic pro((int, char *, int));
canna_export(int) RkwUnmountDic pro((int, char *));
canna_export(int) RkwRemountDic pro((int, char *, int));
canna_export(int) RkwSync pro((int, char *));
canna_export(int) RkwGetMountList pro((int, char *, int));
canna_export(int) RkwDefineDic pro((int, char *, cannawc *));
canna_export(int) RkwDeleteDic pro((int, char *, cannawc *));
canna_export(int) RkwBgnBun pro((int, cannawc *, int, int));
canna_export(int) RkwEndBun pro((int, int));
canna_export(int) RkwGoTo pro((int, int));
canna_export(int) RkwLeft pro((int));
canna_export(int) RkwRight pro((int));
canna_export(int) RkwXfer pro((int, int));
canna_export(int) RkwNfer pro((int));
canna_export(int) RkwNext pro((int));
canna_export(int) RkwPrev pro((int));
canna_export(int) RkwResize pro((int, int));
canna_export(int) RkwEnlarge pro((int));
canna_export(int) RkwShorten pro((int));
canna_export(int) RkwSubstYomi pro((int, int, int, cannawc *, int));
canna_export(int) RkwStoreYomi pro((int, cannawc *, int));
canna_export(int) RkwGetLastYomi pro((int, cannawc *, int));
canna_export(int) RkwFlushYomi pro((int));
canna_export(int) RkwRemoveBun pro((int, int));
canna_export(int) RkwGetStat pro((int, RkStat *));
canna_export(int) RkwGetYomi pro((int, cannawc *, int));
canna_export(int) RkwGetHinshi pro((int, cannawc *, int));
canna_export(int) RkwGetKanji pro((int, cannawc *, int));
canna_export(int) RkwGetKanjiList pro((int, cannawc *, int));
canna_export(int) RkwGetLex pro((int, RkLex *, int));
canna_export(int) RkwCvtHira pro((cannawc *, int, cannawc *, int));
canna_export(int) RkwCvtKana pro((cannawc *, int, cannawc *, int));
canna_export(int) RkwCvtHan pro((cannawc *, int, cannawc *, int));
canna_export(int) RkwCvtZen pro((cannawc *, int, cannawc *, int));
canna_export(int) RkwCvtEuc pro((cannawc *, int, cannawc *, int));
canna_export(int) RkwCreateDic pro((int, char *, int));
canna_export(int) RkwQueryDic pro((int, char *, char *, struct DicInfo *));
canna_export(void) RkwCloseRoma pro((struct RkRxDic *));
canna_export(struct) RkRxDic * RkwOpenRoma pro((char *));
canna_export(int) RkwSetUserInfo pro((char *, char *, char *));
canna_export(char *) RkwGetServerName pro((void));
canna_export(int) RkwGetServerVersion pro((int *, int *));
canna_export(int) RkwListDic pro((int, char *, char *, int));
canna_export(int) RkwCopyDic pro((int, char *, char *, char *, int));
canna_export(int) RkwRemoveDic pro((int, char *, int));
canna_export(int) RkwRenameDic pro((int, char *, char *, int));
canna_export(int) RkwChmodDic pro((int, char *, int));
canna_export(int) RkwGetWordTextDic pro((int, unsigned char *,
					 unsigned char *, cannawc *, int));
canna_export(int) RkwGetSimpleKanji pro((int, char *, cannawc *, int,
					 cannawc *, int, cannawc *, int));
canna_export(int) RkwStoreRange pro((int, cannawc *, int));

#endif

void	RkFinalize pro((void));
int     RkInitialize pro((char *));
int    	RkCreateContext pro((void));
int     RkCloseContext pro((int));
int	RkDuplicateContext pro((int));
int	RkSetDicPath pro((int, char *));
int	RkGetDirList pro((int, char *,int));
int	RkGetDicList pro((int, char *,int));
int	RkMountDic pro((int, char *, int));
int	RkUnmountDic pro((int, char *));
int	RkRemountDic pro((int, char *, int));
int	RkSync pro((int, char *));
int	RkGetMountList pro((int, char *, int));
int	RkDefineDic pro((int, char *, char *));
int	RkDeleteDic pro((int, char *, char *));
int	RkBgnBun pro((int, char *, int, int));
int	RkEndBun pro((int, int));
int	RkGoTo pro((int, int));
int	RkLeft pro((int));
int	RkRight pro((int));
int	RkXfer pro((int, int));
int	RkNfer pro((int));
int	RkNext pro((int));
int	RkPrev pro((int));
int	RkResize pro((int, int));
int	RkEnlarge pro((int));
int	RkShorten pro((int));
int	RkSubstYomi pro((int, int, int, char *, int));
int	RkStoreYomi pro((int, char *, int));
int	RkGetLastYomi pro((int, char *, int));
int	RkFlushYomi pro((int));
int	RkRemoveBun pro((int, int));
int	RkGetStat pro((int, RkStat *));
int	RkGetYomi pro((int, unsigned char *, int));
int	RkGetHinshi pro((int, unsigned char *, int));
int	RkGetKanji pro((int, unsigned char *, int));
int	RkGetKanjiList pro((int, unsigned char *, int));
int	RkGetLex pro((int, RkLex *, int));
int	RkCvtHira pro((unsigned char *, int, unsigned char *, int));
int	RkCvtKana pro((unsigned char *, int, unsigned char *, int));
int	RkCvtHan pro((unsigned char *, int, unsigned char *, int));
int	RkCvtZen pro((unsigned char *, int, unsigned char *, int));
int	RkCvtNone pro((unsigned char *, int, unsigned char *, int));
int	RkCvtEuc pro((unsigned char *, int, unsigned char *, int));
int	RkQueryDic pro((int, char *, char *, struct DicInfo *));

#ifdef __cplusplus
}
#endif

#if defined(ENGINE_SWITCH)
struct rkfuncs {
  int (*GetProtocolVersion) pro((int *, int *));
  char *(*GetServerName) pro((void));
  int (*GetServerVersion) pro((int *, int *));
  int (*Initialize) pro((char *));
  void (*Finalize) pro((void));
  int (*CreateContext) pro((void));
  int (*DuplicateContext) pro((int));
  int (*CloseContext) pro((int));
  int (*SetDicPath) pro((int, char *));
  int (*CreateDic) pro((int, unsigned char *, int));
  int (*SyncDic) pro((int, char *));
  int (*GetDicList) pro((int, char *, int));
  int (*GetMountList) pro((int, char *, int));
  int (*MountDic) pro((int, char *, int));
  int (*RemountDic) pro((int, char *, int));
  int (*UnmountDic) pro((int, char *));
  int (*DefineDic) pro((int, char *, wchar_t *));
  int (*DeleteDic) pro((int, char *, wchar_t *));
  int (*GetHinshi) pro((int, wchar_t *, int));
  int (*GetKanji) pro((int, wchar_t *, int));
  int (*GetYomi) pro((int, wchar_t *, int));
  int (*GetLex) pro((int, RkLex *, int));
  int (*GetStat) pro((int, RkStat *));
  int (*GetKanjiList) pro((int, wchar_t *, int));
  int (*FlushYomi) pro((int));
  int (*GetLastYomi) pro((int, wchar_t *, int));
  int (*RemoveBun) pro((int, int));
  int (*SubstYomi) pro((int, int, int, wchar_t *, int));
  int (*BgnBun) pro((int, wchar_t *, int, int));
  int (*EndBun) pro((int, int));
  int (*GoTo) pro((int, int));
  int (*Left) pro((int));
  int (*Right) pro((int));
  int (*Next) pro((int));
  int (*Prev) pro((int));
  int (*Nfer) pro((int));
  int (*Xfer) pro((int, int));
  int (*Resize) pro((int, int));
  int (*Enlarge) pro((int));
  int (*Shorten) pro((int));
  int (*StoreYomi) pro((int, wchar_t *, int));
  int (*SetAppName) pro((int, char *));
  int (*SetUserInfo) pro((char *, char *, char *));
  int (*QueryDic) pro((int, char *, char *, struct DicInfo *));
  int (*CopyDic) pro((int, char *, char *, char *, int));
  int (*ListDic) pro((int, char *, char *, int));
  int (*RemoveDic) pro((int, char *, int));
  int (*RenameDic) pro((int, char *, char *, int));
  int (*ChmodDic) pro((int, char *, int));
  int (*GetWordTextDic) pro((int, unsigned char	*, unsigned char *,
			     wchar_t *, int));
  int (*GetSimpleKanji) pro((int, char *, wchar_t *, int, wchar_t *, int,
			     wchar_t *, int));
};
#endif /* ENGINE_SWITCH */

#ifdef CANNA_EXP_PREDEFINED
#undef CANNA_EXP_PREDEFINED
#else
#undef canna_export
#endif

#ifdef CANNA_PRO_PREDEFINED
#undef CANNA_PRO_PREDEFINED
#else
#undef pro
#endif

#endif	/* _RK_h */
/* don't add stuff after this line */
