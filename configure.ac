# Process this file with autoconf to produce a configure script.
AC_INIT([canna], [3.7p3], [canna-dev@lists.sourceforge.jp])
AC_CONFIG_SRCDIR([WHATIS.jp])
AC_CONFIG_HEADER([accanna.h])

if test "x$IN_MAKE" != xyes; then
  AC_MSG_ERROR([You cannot invloke configure directly.])
fi

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h locale.h memory.h netdb.h netinet/in.h stddef.h stdlib.h string.h]dnl
 [strings.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h sys/timeb.h syslog.h unistd.h]dnl
 [sys/select.h arpa/inet.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_SETPGRP
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([bzero dup2 gethostbyaddr gethostbyname gethostname inet_ntoa isascii memchr]dnl
 [memset mkdir munmap select setlocale socket strchr strdup strrchr strspn]dnl
 [index bcopy memcpy setsid rename FD_ISSET strlcpy syslog vsyslog inet_aton] )

AC_CHECK_TYPES([fd_set],,,[dnl
#include <sys/types.h>
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
])
AC_CHECK_TYPES([int32_t],,,[dnl
#if defined(HAVE_INTTYPES_H)
# include <inttypes.h>
#elif defined(HAVE_STDINT_H)
# include <stdint.h>
#else
# include <sys/types.h>
#endif
])
AC_CHECK_TYPES([time_t])
AC_CHECK_TYPES([in_addr_t],,,[dnl
#include <sys/types.h>
#include <netinet/in.h>
])
AC_CHECK_TYPES([socklen_t],,,[dnl
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
])
AC_CHECK_SIZEOF([void *])
AC_CHECK_MEMBERS([struct hostent.h_addr_list],,,[#include <netdb.h>])
AC_CONFIG_FILES([])

dnl PUBCONF_DEFINE(NAME,VALUE)
define([PUBCONF_PREFIX],[CANNA_])
AC_DEFUN([PUBCONF_DEFINE],[dnl
cat << PUBCONF_EOF >> "$pubconf_tmp"
[#]define PUBCONF_PREFIX[$1] [$2]
PUBCONF_EOF[]dnl
])
dnl PUBCONF_NOTDEF(NAME)
AC_DEFUN([PUBCONF_NOTDEF],[dnl
echo "/* [#]undef PUBCONF_PREFIX[$1] */" >> "$pubconf_tmp"[]dnl
])
dnl PUBCONF_DEFINE_IF(NAME,COND)
AC_DEFUN([PUBCONF_DEFINE_IF],[dnl
if [$2]; then
  PUBCONF_DEFINE([$1],1)
else
  PUBCONF_NOTDEF([$1])
fi
])
dnl PUBCONF_DEFINE_YESNO(NAME,VAR)
AC_DEFUN([PUBCONF_DEFINE_YESNO],[PUBCONF_DEFINE_IF([$1],[test "x$][$2" = xyes])])

AC_CONFIG_COMMANDS([pubconf.h],[
pubconf_hdr="pubconf.h"
pubconf_tmp="$pubconf_hdr.tmp"
AC_MSG_NOTICE([creating "$pubconf_hdr"])

cat << PUBCONF_EOF > "$pubconf_tmp"
/* Generated file. Modify "configure.ac" instead of this file. */
#ifndef CANNA_PUBCONF_H
#define CANNA_PUBCONF_H

PUBCONF_EOF

PUBCONF_DEFINE_YESNO([HAVE_INTTYPES_H],[pubconf_header_inttypes_h])
PUBCONF_DEFINE_YESNO([HAVE_STDINT_H],[pubconf_header_stdint_h])
PUBCONF_DEFINE_YESNO([HAVE_INT32_T],[pubconf_type_int32_t])

cat << PUBCONF_EOF >> "$pubconf_tmp"

#endif /* CANNA_PUBCONF_H */
PUBCONF_EOF

if cmp -s "$pubconf_hdr" "$pubconf_tmp"; then
  AC_MSG_NOTICE(["$pubconf_hdr" is unchanged])
  rm "$pubconf_tmp"
else
  mv "$pubconf_tmp" "$pubconf_hdr"
  fi
],[
  m4_foreach([Var],[header_inttypes_h,header_stdint_h,type_int32_t],[dnl
    [pubconf_]Var="[$ac_cv_]Var"
  ])
])

AC_OUTPUT
# $Id: configure.ac,v 1.12.2.5 2004/05/19 15:47:33 aida_s Exp $
