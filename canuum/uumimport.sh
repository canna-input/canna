#!/bin/sh
# $Id: uumimport.sh,v 1.3.2.1 2003/01/11 17:53:57 aida_s Exp $
date=20021221
cd freewnn-uum
CVS_RSH=ssh cvs import -I ! canna/canuum FREEWNN freewnn_$date
