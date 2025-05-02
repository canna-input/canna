#!/bin/sh
# $Id: mkrelease.sh,v 1.1.2.2 2003/01/25 08:46:29 aida_s Exp $
set -e
set -x
cp Canna.conf.dist Canna.conf
cd canuum
autoconf213
autoheader
rm -rf autom4te.cache
