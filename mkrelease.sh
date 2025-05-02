#!/bin/sh
# $Id: mkrelease.sh,v 1.5.2.1 2003/12/27 17:15:20 aida_s Exp $
set -e
set -x
cp Canna.conf.dist Canna.conf
autoconf259
autoheader259
rm -rf autom4te.cache
cd canuum
autoconf213
autoheader259
rm -rf autom4te.cache
