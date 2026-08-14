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

/*LINTLIBRARY*/

#include	"RKintern.h"

extern int _Rkpopen(struct DM *, char *, int, struct RkKxGram *);
extern int _Rkpclose(struct DM *, char *, struct RkKxGram *);
extern int _Rkpsearch(struct RkContext *, struct DM *, Wchar *, int, struct nread *,
       int, int *);
extern int _Rkpio(struct DM *, struct ncache *, int);
extern int _Rkpctl(struct DM *, struct DM *, int, Wchar *, struct RkKxGram *);
extern int _Rkpsync(struct RkContext *, struct DM *, struct DM *);

extern int _Rktopen(struct DM *, char *, int, struct RkKxGram *);
extern int _Rktclose(struct DM *, char *, struct RkKxGram *);
extern int _Rktsearch(struct RkContext *, struct DM *, Wchar *, int, struct nread *,
       int, int *);
extern int _Rktio(struct DM *, struct ncache *, int);
extern int _Rktctl(struct DM *, struct DM *, int, Wchar *, struct RkKxGram *);
extern int _Rktsync(struct RkContext *, struct DM *, struct DM *);

struct RkDST 	_RkDST[] = {
/* PERMDIC */ { _Rkpopen, _Rkpclose, _Rkpsearch, _Rkpio, _Rkpctl, _Rkpsync, },
/* TEMPDIC */ { _Rktopen, _Rktclose, _Rktsearch, _Rktio, _Rktctl, _Rktsync, },
};
