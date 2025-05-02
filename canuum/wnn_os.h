/*
 *  wnn_os.h,v 1.12 2002/06/22 13:24:31 hiroo Exp
 *  Canna: $Id: wnn_os.h,v 1.5 2003/01/06 01:41:28 aida_s Exp $
 */

/*
 * FreeWnn is a network-extensible Kana-to-Kanji conversion system.
 * This file is part of FreeWnn.
 * 
 * Copyright Kyoto University Research Institute for Mathematical Sciences
 *                 1987, 1988, 1989, 1990, 1991, 1992
 * Copyright OMRON Corporation. 1987, 1988, 1989, 1990, 1991, 1992, 1999
 * Copyright ASTEC, Inc. 1987, 1988, 1989, 1990, 1991, 1992
 * Copyright FreeWnn Project 1999, 2000, 2002
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

#ifndef WNN_OS_H
#define WNN_OS_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* system headers needed for system dependent configuration */
#include <signal.h>
#if STDC_HEADERS
#  include <stdlib.h>
#  include <limits.h>
#endif /* STDC_HEADERS */

#include <sys/types.h>
#include <sys/file.h>
#if HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif

/* ncurses' term.h defines them to either 0 or 1! */
#ifndef HAVE_TERMIOS_H
#  define DONT_HAVE_TERMIOS_H
#endif
#ifndef HAVE_TERMIO_H
#  define DONT_HAVE_TERMIO_H
#endif
#ifndef HAVE_SYS_TERMIO_H
#  define DONT_HAVE_SYS_TERMIO_H
#endif
#ifndef HAVE_SGTTY_H
#  define DONT_HAVE_SGTTY_H
#endif

#ifdef CONFIG_TERMINFO
#  if defined(HAVE_TERMINFO)
#    if defined(HAVE_CURSES_H)
#      include <curses.h>
#    elif defined(HAVE_NCURSES_H)
#      include <ncurses.h>
#    else
#      error "no terminfo header"
#    endif
#    ifdef HAVE_TERM_H
#      include <term.h>
#    endif
#  else /* !HAVE_TERMINFO */
#    if defined(HAVE_TERMCAP_H)
#       include <termcap.h>
#    endif
#  endif
#endif

#ifndef CONFIG_TERMINFO
#ifdef TERMINFO
#include <curses.h>
#include <term.h>
#endif
#endif

#ifdef DONT_HAVE_TERMIOS_H
#  undef HAVE_TERMIOS_H
#  undef DONT_HAVE_TERMIOS_H
#endif
#ifdef DONT_HAVE_TERMIO_H
#  undef HAVE_TERMIO_H
#  undef DONT_HAVE_TERMIO_H
#endif
#ifdef DONT_HAVE_SYS_TERMIO_H
#  undef HAVE_SYS_TERMIO_H
#  undef DONT_HAVE_SYS_TERMIO_H
#endif
#ifdef DONT_HAVE_SGTTY_H
#  undef HAVE_SGTTY_H
#  undef DONT_HAVE_SGTTY_H
#endif

/* strchr vs. index, etc. */
#if (HAVE_MEMSET) && !(HAVE_BZERO)
#  define bzero(adr,n)  memset((adr),0,(n))
#endif
#if !(HAVE_STRCHR) && (HAVE_INDEX)
#  define strchr  index
#endif
#if !(HAVE_STRRCHR) && (HAVE_RINDEX)
#  define strrchr rindex
#endif

/* SIGNAL */

#ifdef SYSVR2
#ifndef re_signal
# define re_signal(x, y) signal((x), (y))
#endif
#else
# define re_signal(x, y)
#endif

#if !defined (SIGCHLD) && defined (SIGCLD)
#  define SIGCHLD SIGCLD
#endif

typedef RETSIGTYPE (*intfnptr) ();

/* Temporally place the number of filedescripters hack here. */
#if HAVE_GETDTABLESIZE
#  define WNN_NFD getdtablesize()
#elif defined (OPEN_MAX)
#  define WNN_NFD OPEN_MAX
#elif defined (NOFILE)
#  define WNN_NFD NOFILE
#endif /* HAVE_GETDTABLESIZE */

/* pseudo ramdom number */
#if !defined (RAND_MAX)
#  if defined (INT_MAX)
#    define RAND_MAX INT_MAX
#  else
#    define RAND_MAX 0x7fffffff
#  endif /* INT_MAX */
#endif /* RAND_MAX*/

#if HAVE_DRAND48
#  define DRAND()  drand48 ()
#  define SDRAND(x)  srand48 (x)
#elif HAVE_RAMDOM
#  define DRAND()  ((double) random() / (double) RAND_MAX)
#  define SDRAND(x)  srandom (x)
#else
#  define DRAND()  ((double) rand() / (double) RAND_MAX)
#  define SDRAND(x)  srand (x)
#endif /* HAVE_DRAND48 */

#if HAVE_RANDOM
#  define RAND()  random ()
#  define SRAND(x)  srandom (x)
#else
#  define RAND()  rand ()
#  define SRAND(x)  srand (x)
#endif /* HAVE_RANDOM */

#if !defined(HAVE_FD_SET) && !defined(FD_SET) && defined(HAVE_UNISTD_H)
#include <unistd.h> /* to define FD_SET */
#endif
#if defined(HAVE_FD_SET) || defined(FD_SET)
typedef fd_set wnn_fd_set;
#define WNN_FD_SET		FD_SET
#define WNN_FD_CLEAR		FD_CLEAR
#define WNN_FD_ISSET		FD_ISSET
#define WNN_FD_ZERO		FD_ZERO
#else
typedef unsigned long wnn_fd_mask;
#define BINTSIZE		(sizeof(unsigend long) *8)
#define WNN_FD_SETSIZE		WNN_NFD
#define WNN_FD_SET_WIDTH	((WNN_FD_SETSIZE) + (BINTSIZE - 1U) / (BINTSIZE))
typedef struct wnn_fd_set {
  wnn_fd_mask fds_bits[WNN_FD_SET_WIDTH];
}
#define WNN_FD_SET(pos,array)	(array[pos/BINTSIZE] |= (1<<(pos%BINTSIZE)))
#define WNN_FD_CLR(pos,array)	(array[pos/BINTSIZE] &= ~(1<<(pos%BINTSIZE)))
#define WNN_FD_ISSET(pos,array)	(array[pos/BINTSIZE] &  (1<<(pos%BINTSIZE)))
#define WNN_FD_ZERO(array)	(bzero (array, WNN_FD_SET_WIDTH))
#endif /* !(HAVE_FD_ZERO || defined (FD_ZERO)) */

#ifdef HAVE_KILLPG
# define KILLPG(pgrp, sig) killpg(pgrp, sig)
#else
# define KILLPG(pgrp, sig) kill(-(pgrp), sig)
#endif /* HAVE_KILLPG */

#if defined(HAVE_GETPGID) /* SVR4 and most modern systems */
# define GETPGID(pid) getpgid(pid)
#elif defined(HAVE_GETPGRP) && !defined(GETPGRP_VOID) /* 4.3BSD */
# define GETPGID(pid) getpgrp(pid)
#elif defined(uniosu)
# define GETPGID(pid) ngetpgrp(pid)
#else
/* no way to get process group id */
#endif /* GETPGID */

#if defined(HAVE_GETPGRP) && defined(GETPGRP_VOID) /* POSIX, SysV */
# define GETMYPGRP() getpgrp()
#elif defined(GETPGID)
# define GETMYPGRP() GETPGID(getpid())
#else
/* probably some build error occured */
# error "don't know how to get my process group id"
#endif /* GETMYPGRP */

/* function prototypes (temporal use. need reconstruction) */
unsigned int create_cswidth (char *s);	/* xutoj.c */
int euksc_to_ksc (unsigned char *ksc,
		  unsigned char *euksc,
		  int eusiz);		/* xutoj.c */
int get_short (short *sp, FILE *ifpter);	/* bdic.c */

#endif  /* WNN_OS_H */
