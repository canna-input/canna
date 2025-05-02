#!/bin/sh
# $Id: uumimport.sh,v 1.3 2003/01/06 02:43:28 aida_s Exp $
date=20021221
cd freewnn-uum
CVS_RSH=ssh cvs import -I ! canna/canuum FREEWNN freewnn_$date
