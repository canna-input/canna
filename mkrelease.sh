#!/bin/sh
# $Id: mkrelease.sh,v 1.1.2.1 2003/01/11 17:53:56 aida_s Exp $
set -e
set -x
cp Canna.conf.dist Canna.conf
cd canuum
autoconf213
autoheader
