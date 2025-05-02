/* Copyright (c) 2003 Canna Project. All rights reserved.
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

/* $Id: file.h,v 1.5 2003/09/21 12:56:28 aida_s Exp $ */

#ifndef	RKINDEP_FILE_H
#define RKINDEP_FILE_H

#ifdef NEED_RKINDEP_SUBST
# include "RKindep/file.sub"
#endif

/* NFD and FD_SET macros are based on canuum/wnn_os.h */
#include <unistd.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#include <limits.h>
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#include <sys/socket.h>

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <stdio.h>

#if defined (OPEN_MAX)
# define RKI_NFD OPEN_MAX
#elif defined (NOFILE)
# define RKI_NFD NOFILE
#endif

#if (defined(HAVE_FD_ISSET) || defined(FD_ISSET)) && defined(HAVE_FD_SET)
typedef fd_set rki_fd_set;
# define RKI_FD_SET(fd, set) FD_SET(fd, set)
# define RKI_FD_CLR(fd, set) FD_CLR(fd, set)
# define RKI_FD_ISSET(fd, set) FD_ISSET(fd, set)
# define RKI_FD_ZERO(set) FD_ZERO(set)
# ifdef FD_SETSIZE
#  define RKI_FD_SETSIZE FD_SETSIZE
# else
#  define RKI_FD_SETSIZE (sizeof(fd_set) * 8)
# endif
#else
typedef unsigned long rki_fd_mask;
# define BINTSIZE (sizeof(unsigned long) * 8)
# define RKI_FD_SETSIZE RKI_NFD
# define RKI_FD_SET_WIDTH ((RKI_FD_SETSIZE + BINTSIZE - 1U) / BINTSIZE)
typedef struct {
  rki_fd_mask fds_bits[RKI_FD_SET_WIDTH];
} rki_fd_set;
# define RKI_FD_SET(fd, set) \
    ((set)->fds_bits[fd/BINTSIZE] |= (1<<(fd%BINTSIZE)))
# define RKI_FD_CLR(fd, set) \
    ((set)->fds_bits[fd/BINTSIZE] &= ~(1<<(fd%BINTSIZE)))
# define RKI_FD_ISSET(fd, set) \
    ((set)->fds_bits[fd/BINTSIZE] &  (1<<(fd%BINTSIZE)))
# define RKI_FD_ZERO(set) bzero((set)->fds_bits, RKI_FD_SET_WIDTH)
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int RkiConnect pro((int fd, struct sockaddr *addrp, size_t len, 
      const struct timeval *timeout));
extern char *RkiGetLine pro((FILE *src));
extern void *RkiReadWholeFile pro((FILE *src, size_t *retsize));

extern char *RkiBasename pro((const char *src));
#ifdef __cplusplus
}
#endif

#endif	/* RKINDEP_FILE_H */
