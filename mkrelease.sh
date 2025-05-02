#!/bin/sh
# $Id: mkrelease.sh,v 1.5 2003/01/26 04:36:31 aida_s Exp $
set -e
set -x
cp Canna.conf.dist Canna.conf
autoconf
autoheader
rm -rf autom4te.cache
cd canuum
autoconf213
autoheader
rm -rf autom4te.cache
