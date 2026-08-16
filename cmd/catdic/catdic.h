/* Copyright (c) 2026 Canna Project. All rights reserved.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of the
 * author and contributors not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written
 * prior permission.  The author and contributors no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * THE AUTHOR AND CONTRIBUTORS DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE AUTHOR AND CONTRIBUTORS BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTUOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */

#ifndef CATDIC_H
#define CATDIC_H

#include <stdio.h>

#ifdef SVR4
extern char *gettxt(const char *, const char *);
#else
#define	gettxt(x,y)  (y)
#endif

/* RKdelline.c */
int RkDeleteLine(int, char *, char *);

/* rutil.c */
int RkDefineLine(int, unsigned char *, char *);
int CopyDic(int, unsigned char *, unsigned char *, unsigned char *, int);
void PrintMessage(int, unsigned char *);
int makeDictionary(int, unsigned char *, int);
int rmDictionary(int, unsigned char *, int);
void Message(const char *, ...);

/* can.c */
extern char init[];
int DownLoadDic(FILE *, unsigned char *);
int renameDictionary(int, char *, char *, int);
int scan_opt(int, char **, int *);
void shrink_opt(int, char *[], int);

#endif /* CATDIC_H */
