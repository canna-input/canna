#!/bin/sh
set -e
set -x
if test -r ./autolocal.sh; then
  . ./autolocal.sh
fi
: ${AUTOCONF:=autoconf2.69}
: ${AUTOHEADER:=autoheader2.69}
cp Canna.conf.dist Canna.conf
$AUTOCONF
$AUTOHEADER
rm -rf autom4te.cache
cd canuum
$AUTOCONF
$AUTOHEADER
rm -rf autom4te.cache
