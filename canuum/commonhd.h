/*
 *  commonhd.h,v 1.9 2002/05/05 05:13:08 hiroo Exp
 *  Canna: $Id: commonhd.h,v 1.6 2003/02/01 20:16:33 aida_s Exp $
 */

/*
 * FreeWnn is a network-extensible Kana-to-Kanji conversion system.
 * This file is part of FreeWnn.
 * 
 * Copyright Kyoto University Research Institute for Mathematical Sciences
 *                 1987, 1988, 1989, 1990, 1991, 1992
 * Copyright OMRON Corporation. 1987, 1988, 1989, 1990, 1991, 1992, 1999
 * Copyright ASTEC, Inc. 1987, 1988, 1989, 1990, 1991, 1992
 * Copyright FreeWnn Project 1999, 2000, 2001, 2002
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

#ifndef WNN_COMMONHD_H
#define WNN_COMMONHD_H

/****************
 * Common header 
 ****************/
#include <stdio.h>

#define JSERVER_VERSION 0x4200  /* minor version */
#define _SERVER_VERSION "FreeWnn 1.1.0 pl20"

#ifndef JS
typedef unsigned int UINT;
typedef unsigned char UCHAR;
#ifndef w_char
# define w_char unsigned short
#endif /* w_char */
#endif /*JS */

#ifdef TAIWANESE
#ifndef CHINESE
#define CHINESE
#endif
#endif

#ifdef  CHINESE
#define CONVERT_from_TOP
#define CONVERT_by_STROKE       /* 筆形(Bi Xing) */
#define CONVERT_with_SiSheng    /* 四声(Si Sheng) */
#define NO_FZK                  /* 付属語は、ない */
#define NO_KANA                 /* ひらがな(読みと同じ候補)は、ない */
#endif

#ifdef KOREAN
#define CONVERT_from_TOP
#define NO_FZK
#endif

#ifdef luna
#ifdef uniosu
# ifndef        SYSVR2
#  define       SYSVR2
# endif
# ifndef        TERMINFO
#  define       TERMINFO
# endif
#else /* if defined(MACH) || defined(uniosb) */
# ifndef        BSD42
#  define       BSD42
# endif
# ifndef        BSD43
#  define       BSD43
# endif
#  if defined(luna68k)
#   ifndef      BSD44
#    define     BSD44
#   endif
#  endif /* defined(luna68k) */
# ifndef        TERMCAP
#  define       TERMCAP
# endif
#endif
#else /* defined(luna) */
#if defined(sun) && !defined(SVR4)
# ifndef        BSD42
#  define       BSD42
# endif
# ifndef        TERMCAP
#  define       TERMCAP
# endif
#else /* sun else */
#if defined(DGUX) || defined(linux)
# ifndef        SYSVR2
#  define       SYSVR2
# endif
# ifndef        TERMCAP
#  define       TERMCAP
# endif
#else
#if defined(SVR4) || defined(hpux) || defined(SYSV) || defined(USG)
# ifndef        SYSVR2
#  define       SYSVR2
# endif
# ifndef        TERMINFO
#  define       TERMINFO
# endif
# ifdef sun
#  define SOLARIS
# endif
#else
# ifndef        BSD43
#  define       BSD43
# endif
# ifndef        BSD42
#  define       BSD42
# endif
# ifndef        TERMCAP
#  define       TERMCAP
# endif
#endif /* defined(SVR4) || defined(hpux) || defined(SYSV) || defined(USG) */
#endif /* DGUX */
#endif /* sun */
#endif /* luna */

#ifdef CONFIG_TERMINFO
# undef TERMCAP
# undef TERMINFO
# ifdef HAVE_TERMINFO
#  define TERMINFO
# else
#  define TERMCAP
# endif
#endif

#if defined(SVR4) || defined(hpux)
#ifndef F_OK
#define F_OK    0
#endif
#ifndef R_OK
#define R_OK    4
#endif
#endif

#define MAXBUNSETSU     80
#define LIMITBUNSETSU   400
#define MAXJIKOUHO      400

#define J_IUJIS 0
#define J_EUJIS 1
#define J_JIS   2
#define J_SJIS  3

#define C_IUGB  0
#define C_EUGB  1

#define C_ICNS11643  0
#define C_ECNS11643  1
#define C_BIG5  2

#define K_IUKSC 0
#define K_EUKSC 1
#define K_KSC   2

#ifndef True
#define True    1
#endif
#ifndef False
#define False   0
#endif

#define KANJI(x)        ((x) & 0x80)


#define Ctrl(X)         ((X) & 0x1f)

#define NEWLINE         Ctrl('J')
#define CR              Ctrl('M')
#define ESC             '\033'

#ifdef luna
#ifdef uniosu
#define RUBOUT          0x08    /* BS */
#else
#define RUBOUT          '\177'
#endif
#else
#define RUBOUT          '\177'
#endif
#define SPACE           ' '


#define JSPACE          0xa1a1
#ifdef KOREAN
#define BAR             0xA1aa  /* ー   */
#else
#define BAR             0xA1BC  /* ー   */
#endif
#define KUTEN_NUM       0xA1A3  /* 。   */
#define TOUTEN_NUM      0xA1A2  /* 、   */
#define S_NUM           0xA3B0  /* ０   */
#define E_NUM           0xA3B9  /* ９   */
#ifdef KOREAN
#define S_HIRA          0xAAA1  /* ぁ   */
#define E_HIRA          0xAAF3  /* ん   */
#define S_KATA          0xABA1  /* ァ   */
#define E_KATA          0xABF6  /* ヶ   */
#else
#define S_HIRA          0xA4A1  /* ぁ   */
#define E_HIRA          0xA4F3  /* ん   */
#define S_KATA          0xA5A1  /* ァ   */
#define E_KATA          0xA5F6  /* ヶ   */
#endif
#define S_HANKATA       0x00A1  /* ｡   */
#define E_HANKATA       0x00DF  /* ﾟ    */

#ifdef KOREAN
#define S_JUMO          0xa4a1  /* ぁ */
#define E_JUMO          0xa4fe  /* �� */
#define S_HANGUL        0xb0a1  /* 亜 */
#define E_HANGUL        0xc8fe  /* 美 */
#define S_HANJA         0xcaa1  /* 福 */
#define E_HANJA         0xfdfe  /* �� */

#define ishanja(x)      ((unsigned)((x) - S_HANJA) <= (E_HANJA - S_HANJA))
#define ishangul(x)     ((unsigned)((x) - S_HANGUL) <= (E_HANGUL - S_HANGUL))
#endif

#define HIRAP(X) ((X) >= S_HIRA && (X) <= E_HIRA)
#define KATAP(X) (((X) >= S_KATA && (X) <= E_KATA) || ((X) == BAR))
#define ASCIIP(X) ((X) < 0x7f)
#define KANJIP(X) (!(HIRAP(X) || KATAP(X) || ASCIIP(X)))

#define YOMICHAR(X) ((HIRAP(X)) || \
                     ('0'<=(X)&&'9'>=(X)) || \
                     ('A'<=(X)&&'Z'>=(X)) || \
                     ('a'<=(X)&&'z'>=(X)) || \
                     (BAR == X) \
                    )
#define HIRA_OF(X) ((KATAP(X) && !(BAR == (X)))? ((X) & ~0x0100) : (X))

#ifdef  CONVERT_by_STROKE
# define Q_MARK         '?'
#endif /* CONVERT_by_STROKE */

#define LENGTHYOMI      256     /* jisho ni touroku suru yomi no nagasa */
#define LENGTHKANJI     256     /* jisho ni touroku suru kanji no nagasa */
#define LENGTHBUNSETSU  264     /* 文節の最大長 */
#define LENGTHCONV      512     /* 変換可能最大文字数 */

#define JISHOKOSUU     20

#define DIC_RDONLY 1            /* 辞書がリード・オンリーである。 */


/* 複数のファイルにまたがって用いられているバッファサイズの定義 */
#define EXPAND_PATH_LENGTH 256  /* expand_expr()が用いるバッファのサイズ */


#define WNN_FILE_STRING "Ｗｎｎのファイル"
#define WNN_FILE_STRING_LEN 16


#define F_STAMP_NUM 64
#define FILE_ALREADY_READ -2
#define FILE_NOT_READ -3

/*      file ID */

#endif  /* WNN_COMMONHD_H */
