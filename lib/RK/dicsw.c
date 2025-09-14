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

#if !defined(lint) && !defined(__CODECENTER__)
static char rcsid[]="$Id: dicsw.c,v 1.1.1.1 2002/10/19 08:27:45 aida_s Exp $";
#endif
/*LINTLIBRARY*/

#include	"RKintern.h"

extern int _Rkpopen pro((struct DM *, char *, int, struct RkKxGram *));
extern int _Rkpclose pro((struct DM *, char *, struct RkKxGram *));
extern int _Rkpsearch
  pro((struct RkContext *, struct DM *, Wchar *, int, struct nread *,
       int, int *));
extern int _Rkpio pro((struct DM *, struct ncache *, int));
extern int _Rkpctl
  pro((struct DM *, struct DM *, int, Wchar *, struct RkKxGram *));
extern int _Rkpsync pro((struct RkContext *, struct DM *, struct DM *));

extern int _Rktopen pro((struct DM *, char *, int, struct RkKxGram *));
extern int _Rktclose pro((struct DM *, char *, struct RkKxGram *));
extern int _Rktsearch
  pro((struct RkContext *, struct DM *, Wchar *, int, struct nread *,
       int, int *));
extern int _Rktio pro((struct DM *, struct ncache *, int));
extern int _Rktctl
  pro((struct DM *, struct DM *, int, Wchar *, struct RkKxGram *));
extern int _Rktsync pro((struct RkContext *, struct DM *, struct DM *));

struct RkDST 	_RkDST[] = {
/* PERMDIC */ { _Rkpopen, _Rkpclose, _Rkpsearch, _Rkpio, _Rkpctl, _Rkpsync, },
/* TEMPDIC */ { _Rktopen, _Rktclose, _Rktsearch, _Rktio, _Rktctl, _Rktsync, },
};
