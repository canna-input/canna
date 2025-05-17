#!/bin/sh
# $Id: mkrelease.sh,v 1.7 2004/04/25 14:16:48 aida_s Exp $
set -e
set -x
cp Canna.conf.dist Canna.conf
autoconf259
autoheader259
rm -rf autom4te.cache
cd canuum
autoconf259
autoheader259
rm -rf autom4te.cache
