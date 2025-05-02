/* Copyright 1994 NEC Corporation, Tokyo, Japan.
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

/* LINTLIBRARY */
/* $Id: RKintern.h,v 1.15.2.2 2003/12/27 17:15:24 aida_s Exp $ */
#ifndef		_RKintern_h
#define		_RKintern_h

#define BUNMATU 

/* BUNMATU マクロを define すると、北陸先端科学技術大学院大学情報科学
   研究科の藤枝和宏(fujieda@jaist.ac.jp)さんが、『かんな』のかな漢字変
   換効率向上のために、1994 冬に組み込んだ「文章末記号」が有効になりま
   す。これにより文章末にしか現れない文字が文節の途中に出て来ることが
   なくなり変換効率が向上します。このコードをかいて下さいました藤枝さ
   んに感謝します。 */

#define LOGIC_HACK
/* 藤枝＠ＪＡＩＳＴのハックを有効にする */

#define EXTENSION_NEW

#include "cannaconf.h"

#if !defined(WINDOWS_STYLE_FILENAME) && !defined(USE_OBSOLETE_STYLE_FILENAME)
#define WINDOWS_STYLE_FILENAME
#endif

#ifdef __CYGWIN32__
#define USE_MALLOC_FOR_BIG_ARRAY
#endif

typedef unsigned short Wchar;

#define RK_INTERNAL
typedef Wchar cannawc;
#define CANNAWC_DEFINED
#define CANNA_NEW_WCHAR_AWARE
#include "canna/RK.h"

#include "ccompat.h"

#include <unistd.h>
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifndef AIXV3
#include	<ctype.h>
#endif

#ifdef NOT_DEF
#include <errno.h>
int	Rk_errno;
#define RkSetErrno(number) {\
    if (number)
	Rk_errno = (number);
    else
	Rk_errno = errno;
}
#else
#define RkSetErrno(number)
#endif

#ifndef RK_DEBUG
#define	RkDebug(fmt, p, q, r)
#endif
#ifdef __STDC__
#define QUOTE(s) #s
#else
#define QUOTE(s) "s"
#endif
#ifdef NDEBUG
#define RK_ASSERT(expr) ((void)0)
#else
#define RK_ASSERT(expr) ((expr) ? (void)0 : RkAssertFail(\
	    __FILE__, __LINE__, QUOTE(expr)))
#endif /* NDEBUG */

#define	MKDIR_MODE	0775
#define	CREAT_MODE	0664

typedef unsigned char	*pointer;
typedef unsigned char   Wrec;

#define WMASK		(~0x0303)
#define RK_WMASK	WMASK
#define WNILL		(unsigned short)0x0000
#define WNULL		(unsigned short *)0
#define	us_iscodeG0(wc)	(((wc) & 0x8080) == 0x0000)
#define	us_iscodeG1(wc)	(((wc) & 0x8080) == 0x8080)
#define	us_iscodeG2(wc)	(((wc) & 0x8080) == 0x0080)
#define	us_iscodeG3(wc)	(((wc) & 0x8080) == 0x8000)

#define RK_SS2 (unsigned char)0x8e
#define RK_SS3 (unsigned char)0x8f

#define RK_ESC_CHAR ((Wchar)'\\')

#define	euc_iscodeG0(c)	isascii(c)
#define	euc_iscodeG1(c)	(((c) & 0x80) && !((c) & RK_SS2) && !((c) & RK_SS3))
#define	euc_iscodeG2(c)	((c) & RK_SS2)
#define	euc_iscodeG3(c)	((c) & RK_SS3)

/* RkUnion
 *	private data at each class.
 */
typedef union _rkunion {
  pointer	ptr;
  long		var;
  unsigned long	uvar;
} RkUnion;

/*	kana kanji henkan jisho
 *		MS	870713
 *
 *	+---------------+
 *	|     HEADER	|	jisho header (256 byte)
 *	+---------------+256L
 *	|		|	yomigana no index
 *	|   DIRECTORY	|
 *	+---------------+
 *	|		|	tango jouhou
 *	|		|		yomi, {(row, col), tsuduri, hindo}...
 *	|     WORD	|
 *	|		|
 *	|		|
 *	+---------------+
 *	|		|	setuzoku jouhou(swd nomi)
 *	|   CONJUNCT	|		row...
 *	|		|
 *	+---------------+
 */

/* BASIC TYPE:
 *	subete no data ha MSB first(Motorolla order) de tenkai sareru
 *		unsigned char	w
 *		unsigned short	wx
 *		unsigned long	wxyz
 */	
#define LOMASK(x)	((x)&255)
/* #include	"type.h" */
/*
 * 	byte stream <=> long or short
 */
#define l_to_bst4(src, dst) {\
    dst[0] = (unsigned char)(((long)(src) >> 24) & 0xff);\
    dst[1] = (unsigned char)(((long)(src) >> 16) & 0xff);\
    dst[2] = (unsigned char)(((long)(src) >> 8) & 0xff);\
    dst[3] = (unsigned char)( (long)(src) & 0xff);\
}

#define bst4_to_l(src) \
    (unsigned long)(((long)(src)[0] << 24) |\
		    ((long)(src)[1] << 16) |\
		    ((long)(src)[2] << 8) |\
		     (long)(src)[3])

#define l_to_bst3(src, dst) {\
    dst[0] = (unsigned char)(((long)(src) >> 16) & 0xff);\
    dst[1] = (unsigned char)(((long)(src) >> 8) & 0xff);\
    dst[2] = (unsigned char)( (long)(src) & 0xff);\
}

#define bst3_to_l(src) \
    (unsigned long)(((long)(src)[0] << 16) | ((long)(src)[1] << 8) | (long)(src)[2])

#define s_to_bst2(src, dst) {\
    (dst)[0] = (unsigned char)((src >> 8) & 0xff);\
    (dst)[1] = (unsigned char)(src & 0xff);\
}

#define	bst2_to_s(src) \
    (unsigned short)(((unsigned short)(src)[0] << 8) | (src)[1])

#define l_to_bst	l_to_bst4
#define bst_to_l	bst4_to_l
#define s_to_bst	s_to_bst2

#define	L4TOL(l4)\
	(((((((unsigned long)(l4)[0]<<8)|(l4)[1])<<8) | (l4)[2])<<8)|(l4)[3])
#define	L3TOL(l3)\
	(((((unsigned long)(l3)[0]<<8)|(l3)[1])<<8) | (l3)[2])
#define	S2TOS(s2)	(((unsigned short)(s2)[0]<<8)|(s2)[1])
#define	LTOL4(l, l4)	{\
	(l4)[0] = LOMASK((l)>>24); (l4)[1] = LOMASK((l)>>16);\
	(l4)[2] = LOMASK((l)>> 8); (l4)[3] = LOMASK((l));\
	}
#define	LTOL3(l, l3)	{\
			   (l3)[0] = LOMASK((l)>>16);\
			   (l3)[1] = LOMASK((l)>> 8);\
			   (l3)[2] = LOMASK((l));\
			}
#define	STOS2(s, s2)	{\
			   (s2)[0] = LOMASK((s)>> 8);\
			   (s2)[1] = LOMASK((s));\
			 }

#define RkNumber(ary) (sizeof(ary)/sizeof(ary[0]))

#define rk_isdigit(c) (!((c) & ~0xff) ? isdigit((int)(c)) : 0)
#define rk_isascii(c) (!((c) & ~0xff) ? isascii((int)(c)) : 0)
#define rk_isspace(c) (!((c) & ~0xff) ? isspace((int)(c)) : 0)

/* WARNING: これを並べ変えるときはHdrtagに注意すること! */
enum {
  /* These must be first 5 tags in this order for new (>=300702L) dic */
  HD_MAG, /* mandatory */
  HD_SIZ, /* mandatory */
  HD_HSZ, /* mandatory */
  HD_CURV, /* mandatory for new dic, but must not exist in 300000L dic */
  HD_CMPV, /* mandatory for new dic, but must not exist in 300000L dic */
  /* Some of them are optional */
  HD_VER, /* only for 300000L format, must not exist in new format */
  HD_TIME, /* mandatory */
  HD_REC, /* mandatory */
  HD_CAN, /* mandatory */
  HD_L2P, /* mandatory except for freq file */
  HD_L2C, /* mandatory except for freq file */
  HD_PAG, /* mandatory except for freq file */
  HD_LND, /* mandatory except for freq file */
  HD_SND, /* mandatory except for freq file */
  HD_DROF, /* mandatory except for freq file */
  HD_PGOF, /* mandatory except for freq file */
  HD_DMNM, /* mandatory */
  HD_CODM, /* mandatory only in freq file, otherwise not referred */
  HD_LANG, /* record is set but not referred */
  HD_WWID, /* record is set but not referred */
  HD_WTYP, /* record is set but not referred */
  HD_COPY, /* optional */
  HD_NOTE, /* optional */
  HD_TYPE, /* record is set but not referred */
  /* Tags below must not appear in 300000L dic */
  HD_CRC, /* optional */
  HD_GRAM, /* optional */
  HD_GRSZ, /* optional */
  HD_MAXTAG
};

#define HD_TAGSIZ	4
#define HD_MIN_TAGSIZ	(3 * HD_TAGSIZ)
#define HD_VERSION(hd) (((hd)->flag[HD_CURV] == -1) \
    ? (hd)->data[HD_CURV].var : 0x300000L)

struct HD {
    RkUnion	data[HD_MAXTAG];
    int		flag[HD_MAXTAG];
};

#define JMWD	1
#define JSWD	2
#define JPRE	3
#define JSUC	4

#define BYTE2		2
#define BYTE4		4

#define DEF_WWID	2

#define WORD_NODE		(0x80)
#define LAST_NODE		(0x40)
#define BIT_UNIT		8

#define NOLOCALE	"NOLOCALE"
#define DEF_WTYP	"W16 "
#define DEF_TYPE	"jmwd"

#define	RK_PG_LOADED		0x01
#define	RK_PG_USED		0x02

#define	isLoadedPage(p)		((p)->flags & RK_PG_LOADED)
#define	isUsedPage(p)		((p)->flags & RK_PG_USED)

#define	thisPWO(p)	(unsigned long)((((unsigned long ) *(p) << 6) & 0x3fc0) \
				   | ((*((p)+1) >> 2) & 0x3f))
#define	thisLVO(p)	(unsigned long)((((unsigned long) *((p)+1) << 13) & 0x6000) \
				   | (((unsigned long) *((p)+2) << 5) & 0x1fe0) \
				   | ((*((p)+3) >> 3) & 0x1f))
#define	thisCSN(p)	(unsigned long)((((unsigned long) *((p)+3) << 8) & 0x700) | *((p)+4))

struct NP {
  unsigned	ndsz;
  unsigned	lnksz;
  unsigned long	lvo;
  unsigned long	csn;
  unsigned	flags;
  int		count;
  unsigned char	*buf;
};

/*
 * definition of off_t added by Kenji Rikitake 27-MAR-1995
 *
 * Include necessary headers for lseek() here. 
 * note: sys/types.h only applies for BSD/OS 2.0. 
 * Use the appropriate files for other systems.
 */

#include <sys/types.h>	

struct ND {
  unsigned long time;
  unsigned long crc;
  int           crc_found;
  unsigned long rec;
  unsigned long can;
  unsigned long sz;
  off_t         doff;
  unsigned long drsz;
  unsigned long pgsz;
  unsigned long ttlpg;
  int           fd;
  unsigned char *buf;
  struct NP     *pgs;
  long		version;
};

/* HEADER:
 *	unsigned char	magic[2];
 *	unsigned char	pakeuc;		key ga asshuku sareteiru
 *	unsigned char	name[];		jisho mei
 *					xxx.mwd xxx.swd xxx.pre xxx.suc
 *					\n de shuuryou
 */
#define		ND_HDMAGICSIZ		4
#define		ND_HDRSIZ		256		/* header size */
#if 0 /* iranai? */
#define		ND_HDRMAG (('H'<<8)|'N')/* magic number */
#endif
/* jisho class */
#define		ND_MWD		RK_MWD
#define		ND_SWD		RK_SWD
#define		ND_PRE		RK_PRE
#define		ND_SUC		RK_SUC
#define		ND_EMP		4		/* kuugo */
#define		ND_OPN		5		/* hiraki kakko */
#define		ND_CLS		6		/* toji kakko */
#define		ND_PUN		7		/* kutouten */


struct wcand {
    unsigned char	*addr;
    unsigned short	row;
    unsigned char	klen;
};

struct RkXwrec {
    unsigned		ncands, wrecsz;
    unsigned char	*wrec;
};

/* DIRECTORY:
 *	xxxx NODE....
 *	    xxxx niha, DIRECTORY no ookisa ga byte de hairu.
 *
 *	4 byte no NODE kara kousei 
 *	
 *	+--+--+--+--+
 *	|KY|X1 X2 X3|
 *	+--+--+--+--+
 *	 !  !
 *	 !  +-------------	X1
 *	 !			word record/directory offset
 *	 !			 (X1<<16) | (X2<<8) | (X3)
 *	 !
 *	 +----------------	yomi (ascii/euc hiragara lower byte)
 *				0
 */
#define	ND_NODSIZ	6		/* node size in byte */
#define	ND_DSPENT	189		/* #entry in the dispatch table */
/* mask values */
#define	ND_OFFMASK	0x00ffffffL	/* offset mask (24 bit) */
/* X1 bit flags */

#define	ND_NULLOFF	ND_OFFMASK	/* tango ga sakujo sareta */
#define	VMASK		0x7fffffL

/* predicators */
#define	ND_LAST		0x0080L	/* directory no saishuu node (entry) */
#define	ND_WORD		0x0040L	/* offset ha tango joushou wo sasu */
#define	ND_RFU		0x0020L	/* Mishiyou (offset ni tuika ?) */
#define	ISLASTNOD(flag)	((flag)&ND_LAST)
#define	ISWORDNOD(flag)	((flag)&ND_WORD)


#define HN (unsigned long) 256
#define HashFunc(key) (unsigned long)((key) % HN)

struct WRT{
       unsigned long sz;
       unsigned long cs;
       unsigned long frst;
       unsigned long tm;
       unsigned char *buf;
};

struct CTdata{
       struct CTdata *next;
       unsigned long ct[2];
};       

struct RUT{
       unsigned long sz;
       unsigned long cs;
       unsigned long frst;
       unsigned long tm;
       struct CTdata **dp;
};

#define a_csn(x)  (unsigned long) \
(((((unsigned long)(x)[0] << 8) | (x)[1]) << 4) | (((unsigned long)(x)[2] & 0xf0) >> 4))

#define a_tick(x) (unsigned long) \
((((((unsigned long)(x)[2] & 0x0f) << 8) | (unsigned long)(x)[3]) << 8) | (x)[4]) 

#define WriteVal(csn, tick, buf){\
   (buf)[0] = (unsigned char)(((csn)>>12) & 0xff);\
   (buf)[1] = (unsigned char)(((csn)>>4) & 0xff);\
   (buf)[2] = (unsigned char)((((csn)<< 4) & 0xf0)|(((tick)>>16) & 0x0f));\
   (buf)[3] = (unsigned char)(((tick)>>8) & 0xff);\
   (buf)[4] = (unsigned char)((tick) & 0xff);\
}

#define WriteCT(csn, tick, arr){\
   (arr)[0] = (unsigned long) (csn);\
   (arr)[1] = (unsigned long) (tick);\
}  

#define freeWRT(wruc) {\
     free(wruc->buf);\
     free(wruc);\
}

#define RKmaxDN (unsigned long) 0xff
#define RKmaxCN (unsigned long) 0xffffff


/* WORD:
 *	xxxx {yomi nkouho {flags row col freq kouho}...}...
 *	    xxxx niha, WORD no ookisa ga byte de hairu.
 *
 * tango:
 *	unsigned char	yomi[];			yomigana 
 *	unsigned char	nkouho;			kouho no kazu 
 *		unsigned char	flags;		kouho no nagasa + flags
 *		unsigned char	row;		kouho no row bangou
 *		unsigned char	col;		kouho no col bangou
 *		unsigned char	freq		kouho no shiyou hindo
 *		unsigned char	kouho[];
 *	
 */

#define	NW_PREFIX	2	/* the length of kouho prefix in byte */

/* flags */
#define	NW_LEN		0x7f	/* kouho no nagasa ( zenkaku 31 moji) */
#ifdef LOGIC_HACK
#define NW_RCBITS	9	/* bits of maximum rowcol number */
#endif

#define	candlen(flag)	((flag) & NW_LEN)
#define wordlen(flag)	((candlen(flag) << 1) + NW_PREFIX)
/* RkWcand
 *	data structure used to handle the word record 
 */
typedef struct RkWcand {
    Wrec		*addr;		/* houho sentou address */
    short		rcnum;		/* row column number */
    unsigned char	klen;		/* kouho no nagasa */
} Candidate;

/* CONJUNCT:
 *	xxxx rrcc
		row_0  row_1  ... row_rr-1
 *		name_0 name_1 ... name_rr-1
 *	    xxxx niha, CONJUNCT no ookisa ga byte de hairu.
 *	    rr, cc ha sorezore, row, col no kosuu wo simesu.
 *	    row_i ha byte kyoukai kara hajimaru cc bit karanaru bit retu
 *	    name_i ha EOS de owaru mojiretu
 *
 */

struct RkKxGram;
struct RkGramIterator {
    int			rowcol;
};
#define RkNextGram(iter) ((void)++(iter)->rowcol)

/* hinshi no bunrui
 *	renbunsetu henkan de siyou sareru
 */
#define	IsShuutan(g, r)		RkTestGram(g, r, 0)
#ifdef BUNMATU
#define	IsBunmatu(g, r)		RkTestGram(g, r, 1)
#endif

/* RkGram -- grammatical information
 */

struct RkGram {
  int			refcount; /* reference counter */
  struct RkKxGram	*gramdic; /* grammar dictionary */
  int	 		P_BB, P_NN, P_T00, P_T30, P_T35; /* hinshi codes */
#ifdef LOGIC_HACK
  int			P_KJ; /* tankanji */
#endif
  int			P_Ftte; /* FIXME: should go away */
};

extern struct RkGram SG;


/* ncache
 *	tango record no caching list 
 *  ncache ha, jisho kara itido yomikomareta tango record wo cache kanri suru
 * tameno jouhou kouzou dearu.
 *  tango record ha key (dicnum, address) de skibetu sareru.
 *  taiou suru tango record ha nc_word ni yori simesareru HEAP ryouiki ni aru.
 *	+------------+
 *	| nc_dicnum  |	key part
 *	| nc_address |
 *	+------------+
 *	|  nc_word   |	value part
 *	+------------+
 *  ncache ha 2tu no souhoukou list hash/free list ni tunagareru.
 *  (* koreha UNIX i/o system no buffer cache kara hint wo eta *)
 *  hash list ha key no kensaku wo kousoku ka suru tame ni shiyou sareru.
 *  free list ha kaihou sareta cache ga tunagareru.
 */
struct ncache	{
    struct ncache	*nc_hnext;	/* hash list */
    struct ncache	*nc_hprev;
    struct ncache	*nc_anext;	/* free list */
    struct ncache	*nc_aprev;
    Wrec		*nc_word;	/* heap jouno tango record */
    struct DM		*nc_dic;	/* yomikomareta jisho */
    unsigned short	nc_flags;
    unsigned long	nc_count;	/* word karano sanshou dosuu */
    long		nc_address;	/* word record no file address */
};
/* flags */
#define	NC_DIRTY	1	/* word no naiyou ga henkou sareta */
#define	NC_ERROR	2	/* cache no naiyou ga tadasiku nai */
#define	NC_NHEAP	4	/* word ha heap wo sasite inai */

struct nread {
  struct ncache	*cache;
  unsigned long	offset;
  unsigned long	csn;
  int		nk;
};

/* nword
 *	bunsetsu kaiseki kekka 
 * 	jisho kara no tango yomidasi kekka
 * nword ha, jisho kara search sareta tango record wo kirokusi, 
 * bunsetsu kaiseki ni yori sakusei sareru bunsetsu tree wo hyougen suru.
 */
struct nword {
    struct ncache	*nw_cache;	/* tango record wo fukumu cache */
    short		nw_rowcol;	/* setsuzoku  jouhou */
    unsigned char	nw_ylen;	/* #chars in yomi */
    unsigned char	nw_klen;	/* #chars in kanji */
    unsigned char	nw_class;	/* word class */
    unsigned char	nw_flags;	/* word flags (see below) */
    unsigned char	nw_lit;		/* literal conversion */
    unsigned long	nw_prio;	        /* kouzou ni yoru priority */  /* True ? by tamano */
    unsigned long   nw_csn;
    unsigned char	nw_count;	/* setsuzoku suu */
    struct nword	*nw_left;	/* hidari ni tunagaru word */
    struct nword	*nw_next;	/* onaji nw_len wo motu list */
    unsigned char	*nw_kanji;	/* kanji kouho ichi/douteki na kouho */
/* nw_klen ha fuyou ni naru kanousei ari */
    struct DM		*nw_freq;	/* pointer to frequncy file */
};
/* 
 * word flags
 */
#define NW_MWD		0x80
#define NW_SWD		0x40
#define NW_PRE		0x20
#define NW_SUC		0x10
#define NW_LOWPRI	0x08
#ifdef BUNMATU
#define NW_BUNMATU	0x04
#endif
#define	NW_FOLLOW	0x02		/* has some words following to me */
#define	NW_NEVER_SPLIT	0x01		/* cannot terminate here */
/* accessor & predicates */
#define	IsFollowed(w)	((w)->nw_flags&NW_FOLLOW)
#define	DontSplitWord(w)	((w)->nw_flags |= NW_NEVER_SPLIT)
#define	CanSplitWord(w)		(!((w)->nw_flags&NW_NEVER_SPLIT))
#ifdef BUNMATU
#define	OnlyBunmatu(w)		((w)->nw_flags & NW_BUNMATU)
#endif

/* conversion number */
#define	LIT2GRP(cn)	((cn)>>4)
#define	LIT2MEM(cn)	((cn)&15)
#define	MAKELIT(g, w)	(((g)<<4)|(w))
#define	MAXLIT		16

#define	LIT_NONE	0
#define	LIT_NUM		1
#define	LIT_ALPHA	2
#define	LIT_HIRA	3

/* nbun
 *	bunsetsu queue
 * renbunsetsu henkan kekka wo kiroku 
 */
struct nbun {
    struct nword	*nb_cand;	/* kouho wo kousei suru word
				         * nw_next niyori sort sareru */
    unsigned short	nb_yoff;	/* yomigana offset in buffer  */
    unsigned short	nb_curlen;	/* current candidate length in char */
    unsigned short	nb_maxcand;	/* number of candidates */
    unsigned short	nb_curcand;	/* current candidate */
    unsigned short	nb_flags;	/* ?? */
};

/* DM/DF/DD
 */
/* MS1214
 *	now DM supports frequency files as well
 */
typedef struct DM {
/* pointers for the file structure */
    struct DM		*dm_next;
    struct DM		*dm_prev;
    struct DF		*dm_file;	/* ptr to the file containing this */
/* attributes */
    char		*dm_dicname;		
    char		*dm_nickname;
    unsigned		dm_class;	/* MWD/SWD/PRE/SUC */
    unsigned		dm_flags;	/* flags */
    unsigned char	dm_packbyte;	/* shouryaku sareta joui byte */
/* reference  count from MD */
    int			dm_rcount;
/* setuzoku jouhou (SWD nomi) */
    struct RkGram	*dm_gram;
    RkUnion		dm_extdata;
    unsigned char	*dm_qbits;	/* MS1214 gakushuu jouhou */
/* */
    struct DDT		*dm_line;	/* pointer to dd line */
    long		dm_offset;
    struct RUT		*dm_rut;
    struct NV		*dm_nv;
} DM;
/* flag values */
#define	DM_EXIST	0x01		/* member exists */
#define	DM_WRITABLE	(DM_EXIST << 1)	/* jisho ga write dekiru */
#define	DM_LOADWORD	(DM_EXIST << 2)	/* word rec wo memory ni yomi */
#define	DM_UPDATED	(DM_EXIST << 4)	/* member changed */
#define DM_READOK	(DM_EXIST << 5) /* jisho no read ken */
#define DM_WRITEOK	(DM_EXIST << 6) /* jisho no write ken */

typedef struct DF {
/* DF doubly linked list */
    struct DF	*df_next;
    struct DF	*df_prev;
    struct DD	*df_direct;	/* backward pointer to the directory */
    struct DM	df_members;	/* the header of the dictionary */
/* attributes */
    char	*df_link; /* link name of the file */
    unsigned	df_type;	/* type of file */
    unsigned	df_flags;	/* access mode */
    int		df_rcount;	/* # the mounted members */
    RkUnion	df_extdata;
    long	df_size;
} DF;
#define	DM2TYPE(dm)	((dm)->dm_file->df_type)
/* jisho no shurui */
#define	DF_PERMDIC	0	/* file ni aru jisho */
#define	DF_TEMPDIC	1	/* zantei jisho */
#define DF_FREQDIC 	2	/* gakushuu hindo jisho MS1214 */
#define DF_RUCDIC 	3	/* gakushuu hindo jisho MS1214 */

/* flag values */
#define	DF_EXIST	1	/* file exists */
#define	DF_WRITABLE	2	/* file is writable */

/*
 * DD
 */
struct DDT	{
   struct DDT		*ddt_next;
   struct DDT		*ddt_prev;
   int			ddt_status;
   char			*ddt_spec;
};

typedef struct DD {
/* DD doubly linked list */
    struct DD		*dd_next;
    struct DD		*dd_prev;
    char		*dd_path;	/* path name of the directory */
    char		*dd_name;	/* unique name of the directory */
    int			dd_rcount;	/* # opened files+reference from DDP */
    struct DF		dd_files;	/* the header of the file list */
    unsigned		dd_flags;	/* flag */
/* dics.dir file */
    struct DDT		dd_text;
} DD, *DDP;

#define	DD_WRITEOK	1	/* directory is allowed to write */
#define DD_READOK	2

/* MD ----------------------------------------------------------------
 *  context ni mount sareta jisho/jisho wo mount siteiru jisho list no taiou
 * ---------------------------------------------------------------- */
typedef struct MD {
    struct MD		*md_next;	/* dictionary list */
    struct MD		*md_prev;
    struct DM		*md_dic;	/* jisho heno pointer */
    struct DM		*md_freq;	/* MS1214 gakushuu jisho heno pointer */
    int			md_flags;	/* mount flags */
} MD, *MDP;
#define	MD_WRITE	1		/* gakushuu shitei */
#define	MD_MPEND	2		/* mount pending */
#define	MD_UPEND	4		/* unmount pending */

/* RkParam
 *	common parameter block
 */
struct RkParam {
/* cache */
    struct ncache	*cache;
    int			maxcache;
/* heap */
    unsigned char	*heap;
    int			maxheap;
    unsigned char	*ham;
    int			maxham;
/* word */
    struct nword	*word;		/* henkyaku sareta word wo tunageru */
    struct nword	*page;		/* word page list */
    int			word_in_use;
    int			page_in_use;
/* jisho */
    struct DD		dd;
    char        	*ddhome;	/* dictionary home directory */
    struct DD		**ddpath;	/* system ddpath */
/* flags */
    unsigned		flag;		/* checks initialize */
/* */
};

extern struct RkParam SX;

#define SX_INITED	1

#define RK_REARRANGED	0x01

struct NVE {
  unsigned char	*data;
  struct NVE	*left;
  struct NVE	*right;
  struct NVE	*next;
};

struct NV {
  unsigned long	sz;
  unsigned long	tsz;
  long		csz;
  long		cnt;
/* int		lst; delete by tamano(tajima) */
  struct NVE	head;
  struct NVE	**buf;
};

#ifdef RK_LOG
struct henkanlog {
    int n;
    char *henkan;
    struct henkanlog *next;
};
#endif

struct nstore {
    Wchar		*yomi;		/* yomigana buffer */
#ifdef RK_LOG
    unsigned		nblog;
    char		**blog;
    struct henkanlog	*hlog;
    struct henkanlog	dmi;
#endif
    unsigned		nyomi;		/* number of yomigana chars */
    unsigned		maxyomi;	/* maximum number of yomigana chars */
    struct nbun		*bunq;		/* 文節キュー */
    unsigned		maxbunq;	/* saidai bunnsetu suu */
    unsigned		maxbun;		/* saidai bunsetsu suu */
    int			curbun;		/* カレント文節 */
    int			maxxq;		/* saidai queue == bunsetu moji suu */
    struct nqueue	*xq;		/* henkan queue */
    struct nword	**xqh;		/* used in _RkparseBun() */
    int			word_in_use;   	/* #words used in context */
};

/* context
 *	ncontext ha rennbunnsetu hennkann client wo seigyo suru jouhou
 *		jisho kankyou
 *		renbunsetsu henkan sagyou ryouiki 
 */
struct 	nqueue {
    struct nword	*tree;
    short		maxlen;
    short		status;
};

typedef struct RkContext {
    struct nstore	*store;
    struct MD		*md[4];		/* mwd/swd/pre/suc */
    struct DD   	**ddpath;	/* dictionary search path */
    struct DM   	*dmprev;	/* dictionary search path : dic */
    struct DM   	*qmprev;	/* dictionary search path : freq */
    unsigned long	time;		/* tic */
    unsigned		flags;		/* context control flags */
    unsigned long	kouhomode;	/* kana kouho mode */
    unsigned long	*litmode;	/* literal conversion table */
    int			concmode;	/* connect words mode */
    int			poss_cont;	/* */
    RkUnion		cx_extdata;	/* etc data */
    struct NV		*nv;
    struct RkGram	*gram;
} RkContext;

/* Extension Data in DM */
typedef struct TD {
    struct TN   *td_node;	/* array of node TN */
    unsigned	td_n;		/* # effective nodes */
    unsigned	td_max;		/* # maximum nodes in the array */
} TD;

struct TW {
  Wrec		*word;
  unsigned long	lucks[2];
};

typedef struct TN {
  unsigned char	tn_flags;	/* type of node (see blow) */
  Wchar		tn_key;
  union {
    struct TD	*tree;
    struct TW	*word;
  }	tn_value;				/* type specific data */
  /* syntax sugar */
#define tn_tree		tn_value.tree
#define tn_word		tn_value.word
} TN;
/* flags  values */
#define	TN_WORD		1		/* is a word */
#define	TN_WDEF		2		/* wrec has been defined */
#define	TN_WDEL		4		/* wrec has been deleted */
#define	IsWordNode(tn)	((tn)->tn_flags&TN_WORD)

/* td_and_n structure 
 *
 * tupple of td and n for RkGetWordTextDic()
 */
struct td_n_tupple {
  char               *td; /* in fact this type is (struct TD *) */
  int                n;
  struct td_n_tupple *next;
};

/* jishowo 1gyouzutu yomu tokino pointer :used in RkGetWordTextDic */
typedef struct _rec {
    int                 gwt_cx;
    unsigned char       *gwt_dicname;
    struct td_n_tupple  *tdn;
} GWT;

/* flag values */
#define	CTX_LIVE		1	/* allocate sareta */
#define	CTX_XFER		2	/* bunsetu henkann chuu */
#define	CTX_XAUT		4	/* jidou henkan chuu */
#define	CTX_XFSH	      010
#define	CTX_NODIC	      020	/* have never tried to mount dic */

#define	MAX_CONTEXT		256
#define	INIT_CONTEXT		32
#define	ADD_CONTEXT		32

#ifndef SYSTEM_DDHOME_NAME
#define SYSTEM_DDHOME_NAME	"canna"
#endif

#define USER_DIC_DIR		"user"
#define GROUP_DIC_DIR		"group"

#define	IS_LIVECTX(cx)	((cx)->flags & CTX_LIVE)
#define IS_XFERCTX(cx)	(IS_LIVECTX(cx) && ((cx)->flags & CTX_XFER))
#define IS_XAUTCTX(cx)	(IS_LIVECTX(cx) && ((cx)->flags & CTX_XAUT))
#define RK_CONNECT_WORD	0x01
#define	IsCxNum(cn) (0<=((unsigned long)cn)&&((unsigned long)cn)<now_context)
#define	IsLiveCxNum(cn) (IsCxNum(cn) && IS_LIVECTX(&CX[cn]))
#define	IsXferCxNum(cn) (IsLiveCxNum(cn) && IS_XFERCTX(&CX[cn]))
#define Is_Word_Connect(cx)\
    ((cx) && (((cx)->concmode & RK_CONNECT_WORD) == RK_CONNECT_WORD))
#define Is_Word_Make(cx)\
    ((cx) && (((cx)->concmode & RK_MAKE_WORD) == RK_MAKE_WORD))


/* RkDST
 *	dictionary switch table
 */
struct RkDST {
  int (*d_open) /* jisho ga open sareta toki */
    pro((struct DM *, char *, int, struct RkKxGram *));
  int (*d_close) /* jisho ga close sareta toki */
    pro((struct DM *, char *, struct RkKxGram *));
  int (*d_search) /* jisho kara tango wo search suru */
    pro((struct RkContext *, struct DM *, Wchar *,
	 int, struct nread *, int, int *));
  int (*d_io) /* jisho he tango to cache no io */
    pro((struct DM *, struct ncache *, int));
  int (*d_ctl)	/* jisho koyuuno sousa */
    pro((struct DM *, struct DM *, int, Wchar *, struct RkKxGram *));
  int (*d_sync) /* jisho sync suru */
    pro((struct RkContext *, struct DM *, struct DM *));
};
extern struct RkDST	_RkDST[];

#define	_RkEnrefCache(a)	((a)->nc_count++)
#define	DST_PROC(dm)		_RkDST[DM2TYPE(dm)]
#define	DST_OPEN(dm, file, mode, gram)\
	(DST_PROC(dm).d_open)(dm, file, mode, gram)
#define	DST_CLOSE(dm, file, gram)	(DST_PROC(dm).d_close)(dm, file, gram)
#define	DST_SEARCH(cx, dm, k, n, cp, max, cf)\
	(DST_PROC(dm).d_search)(cx, dm, k, n, cp, max, cf)
#define	DST_IO(dm, cp, io)	(DST_PROC(dm).d_io)(dm, cp, io)
#define	DST_READ(dm, cp)	DST_IO(dm, cp, 0)
#define	DST_WRITE(dm, cp)	DST_IO(dm, cp, 1)
#define	DST_CTL(dm, qm, what, arg, gram)\
	(DST_PROC(dm).d_ctl)(dm, qm, what, arg, gram)
#define	DST_SYNC(cx, dm, qm)	(DST_PROC(dm).d_sync)(cx, dm, qm)

/* ctl command */
#define DST_DoDefine		0	/* define words */
#define DST_DoDelete		1	/* delete words */
/* more commands (not implemented) */
#define DST_DoQuery		2	/* query on the dictionary info */

#define DST_DoPrint		9999	/* print debug information */



/* Internal Functions */
struct RkParam		*RkGetSystem();
struct DD		*RkGetSystemDD();
struct DD		*RkGetUserDD();
struct RkContext	*RkGetContext();
struct RkContext	*RkGetXContext();
struct RkKxGram		*RkReadGram pro((int, size_t));
struct RkKxGram		*RkOpenGram();
struct RkKxGram		*RkDuplicateGram();
void			RkCloseGram();

int			_RkInitializeCache();
void			_RkFinalizeCache();
struct ncache		*_RkFindCache();
struct ncache		*_RkReadCache();
void			_RkFreeCache();
void		 	_RkKillCache();
void		 	_RkPurgeCache();
void			_RkDerefCache();

int			_RkRenbun2();
void			_RkLearnBun();

int			RkScanWcand();
int			RkUniqWcand();
int			RkUnionWcand();
int			RkSubtractWcand();

unsigned char		*RkGetGramName();
int			RkGetGramNum();
Wchar			*RkUparseWrec();
Wchar			*_RkUparseWrec();
Wrec			*RkParseWrec();
Wrec			*RkParseOWrec();
Wchar			*RkUparseGramNum();
Wchar			*RkParseGramNum();

/* Context */
char			*allocStr();
void			_RkEndBun();
void			freeDF();

int			_RkCandNumber();
int			_RkWordLength();
int			_RkCalcLog2 pro((int));
int			_RkCalcUnlog2();

/* etc. */

#ifdef OPTIMIZE
#define RkGetContext(cx_num)\
    ((IsCxNum(cn) && IS_LIVECTX(&CX[cn])) ? \
     (&CX[cx_num]) : ((struct RkContext *)0))
    
#define RkGetXContext(cx_num)\
    ((IsCxNum(cn) && IS_LIVECTX(&CX[cn]) && \
      ((IS_XFERCTX(cx)) || (IS_XAUTCTX(cx))) ? \
      (&CX[cx_num]) : ((struct RkContext *)0))

#define RkGetNXContext(cx_num)\
    ((IsCxNum(cn) && IS_LIVECTX(&CX[cn]) && \
      !(IS_XFERCTX(cx)) && !(IS_XAUTCTX(cx))) ? \
      (&CX[cx_num]) : ((struct RkContext *)0))
    
#endif


/* lang dep part */
#define IS_WC_G1_HYPHEN(wc)	((wc) == 0xa1bc)
#define IS_HYPHEN(wc)		(IS_WC_G1_HYPHEN(wc) || ((wc) == '-'))
/* space も候補として使いたいが今の所は space */
#define IS_DIC_PUNCT(euc)	isspace(euc)

	int		_RkRealizeDF();

	struct DM	*_RkSearchDDQ();
	struct DM	*_RkSearchDDP();
	struct DM	*_RkSearchUDDP();
	struct DM	*_RkSearchDDMEM();

	int		_RkIsinDDP();
struct DD		**_RkCopyDDP();
struct DD		**_RkCreateDDP();
void			_RkFreeDDP();

struct DM		*DMcreate();
int			DMremove();
int			DMrename();

int			_RkMountMD();
void			_RkUmountMD();

char			*_RkCreatePath();
char			*_RkCreateUniquePath();
char			*_RkMakePath();

unsigned char		*_RkCreateHeader pro((struct HD *, size_t *size));
int			_RkReadHeader pro((int, struct HD *, off_t));
void			_RkClearHeader pro((struct HD *));
void			_RkRehashCache();

/*
 * limits
 */
/*
 *   NOTE: The following number does not includes EOS at the end of string.
 *	RK_PATH_BMAX	maximum path name length
 *	RK_LINK_BMAX	maximum file name length
 *	RK_MEMBER_BMAX	maximum member name length
 *	RK_NICK_BMAX	maximum nickname length
 */
#ifdef PATH_MAX
#define	RK_PATH_BMAX	PATH_MAX
#else
#define RK_PATH_BMAX 1024
#endif
#ifdef  NAME_MAX
#define RK_LINK_BMAX	NAME_MAX
#else
#ifdef	MAXNAMLEN
#define RK_LINK_BMAX	MAXNAMLEN
#else
#define RK_LINK_BMAX	14
#endif 
#endif
#define	RK_MEMBER_BMAX	255
#define	RK_NICK_BMAX	255

#define RK_LINE_BMAX    1024
#define RK_BUFFER_SIZE  1024

/* 
 * RK_KEY_WMAX <= RK_LEN_WMAX
 */
#define	RK_KEY_WMAX		0x7f
#define	RK_LEFT_KEY_WMAX	0x3f
#define	RK_LEN_WMAX		0x7f
/*
 * RK_CAND_WMAX <= RK_LEN_WMAX
 */
/* 候補数 */
#define	NW_MAXCAND	0x07	/* 7 */
#define	EX_NW_MAXCAND	0xfff	/* 4096 */

/* 候補長 */
#define	NW_MAXCANDLEN	0x7f
#define RK_CAND_WMAX	0x7f
#define RK_CAND_NMAX    0xfff /* 4095 */

/* wrec len */
#define NW_MAXWREC	0x3f	/* 63 */
#define EX_NW_MAXWREC	0x1fff	/* 8191 */

#define RK_RCNAME_BMAX	16	/* essential ?! */

#define	RK_DIV_NMAX	2048
/*
#define MAX_TEXT_LEN \
    ((EX_NW_MAXCAND * \
      ((NW_MAXCANDLEN + 2) * sizeof(unsigned short) + RK_RCNAME_BMAX)) + \
     (RK_LEFT_KEY_WMAX *  sizeof(unsigned short) \
      + 2 * sizeof(unsigned short)))
*/
#define	RK_WREC_BMAX	EX_NW_MAXWREC
#define	RK_MAX_TEXT_LEN	MAX_WREC_BMAX

#define	RK_CONC_NMAX	16	/* 接続する付属語の数(効いてるのかな？) */

#define RK_OLD_MAX_HDRSIZ	1024

#ifndef	_RK_INTERN_FUNCTIONS_DEF_
#define	_RK_INTERN_FUNCTIONS_DEF_

struct DM *_RkSearchDicWithFreq pro((struct DD **, char *, struct DM **));
#ifdef __STDC__
void _Rkpanic pro((const char *, ...));
#else
void _Rkpanic();
#endif
void RkAssertFail pro((const char *, int, const char *));
unsigned long _RkGetTick pro((int));
struct TW *RkCopyWrec pro((struct TW *));
struct TW *RkUnionWrec pro((struct TW *, struct TW *));
struct TW *RkSubtractWrec pro((struct TW *, struct TW *));
void _RkFreeQue pro((struct nstore *, int, int));
void freeTdn pro((struct RkContext *));
void _RkFreeBunq pro((struct nstore *));
int _RkRealizeDD pro((struct DD *));
int RkCvtWide pro((Wchar *, int, char *, int));
int RkCvtNarrow pro((char *, int, Wchar *, int));

#if defined(MMAP)
int _RkDoInvalidateCache pro((long, unsigned long));
#endif

int _RkGetLink pro((struct ND *, long, unsigned long,
		    unsigned long *, unsigned long *));
long _RkUnpackBits pro((unsigned *, unsigned char *, long, int, int));
long _RkPackBits pro((unsigned char *, long, int, unsigned *, int));
int _RkSetBitNum pro((unsigned char *, unsigned long, int, int, int));
int FQsync pro((struct RkContext *, struct DM *, struct DM *, char *));

int RkwCvtSuuji pro((Wchar *, int, Wchar *, int, int));
int RkwCvtNone pro((Wchar *, int, Wchar *, int));

int _RkRowNumber pro((unsigned char *));
int RkTestGram pro((const struct RkKxGram *, int, int));
#ifdef LOGIC_HACK
int RkCheckNegGram pro((const struct RkKxGram *, int, int, int));
#endif
void RkFirstGram pro((struct RkGramIterator *, const struct RkKxGram *));
void RkEndGram pro((struct RkGramIterator *, const struct RkKxGram *));
int _RkRegisterNV pro((struct NV *, Wrec *, int, int));
int FQopen pro((struct DM *, struct DM *, char *, int));
void FQclose pro((struct RkContext *, struct DM *, struct DM *, char *));
int _RkSubstYomi pro((struct RkContext *, int, int, Wchar *, int));
long _RkCopyBits
     pro((unsigned char *, long, int, unsigned char *, long, int));
int _RkIsInDDP pro((struct DD **, struct DD *));
int _RkEql pro((Wchar *, unsigned char *, int));
int DMcheck pro((char *, char *));
int copyFile pro((struct DM *, struct DM *));
int DDchmod pro((struct DD *, int));
int DMchmod pro((struct DM *, int));
int uslen pro((Wchar *));
unsigned char *ustoeuc pro((Wchar *, int, unsigned char *, int));
int _RkSubstYomi pro((struct RkContext *, int, int, Wchar *, int));
int HowManyChars pro((Wchar *, int));
int HowManyBytes pro((Wchar *, int));
int _RkFlushYomi pro((struct RkContext *));
int parse_string pro((char *));

#endif /* _RK_INTERN_FUNCTIONS_DEF_ */

#endif /* _RKintern_h */
/* vim: set sw=2: */
/* don't add stuff after this line */
