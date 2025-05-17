XCOMM!/bin/sh
XCOMM Copyright 1992 NEC Corporation, Tokyo, Japan.
XCOMM
XCOMM Permission to use, copy, modify, distribute and sell this software
XCOMM and its documentation for any purpose is hereby granted without
XCOMM fee, provided that the above copyright notice appear in all copies
XCOMM and that both that copyright notice and this permission notice
XCOMM appear in supporting documentation, and that the name of NEC
XCOMM Corporation not be used in advertising or publicity pertaining to
XCOMM distribution of the software without specific, written prior
XCOMM permission.  NEC Corporation makes no representations about the
XCOMM suitability of this software for any purpose.  It is provided 
XCOMM "as is" without express or implied warranty.
XCOMM
XCOMM NEC CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
XCOMM INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN 
XCOMM NO EVENT SHALL NEC CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
XCOMM CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
XCOMM USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
XCOMM OTHER TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
XCOMM PERFORMANCE OF THIS SOFTWARE. 

XCOMM $Id: mkbindic.cpp,v 1.7 2008/04/05 17:25:46 aida_s Exp $
#include "cannaconf.h"
#if defined(SYSV) || defined(SVR4)
# ifdef nec_ews
PATH=CANNABINDIR:/bin:/usr/bin:/etc:/usr/etc:/usr/ucb:/usr/nec/bin:$PATH;
# else
PATH=CANNABINDIR:/bin:/usr/bin:/etc:/usr/etc:/usr/ucb:$PATH;
# endif /* nec_ews */
#else
PATH=CANNABINDIR:/bin:/usr/bin:/etc:/usr/etc:/usr/5bin:$PATH;
#endif /* SYSV || SVR4 */

export PATH;
text_file=;
dic_name=;
cpp_text=;
spl_text=;
bck_text=;
flag=;
compat_flag=;
usage="usage: mkbindic [-m|-s] [-name dicname] [-c version] textfile [cpp-args ...]";
: ${TMPDIR:=/tmp}

/* main */
{
    while [ $# -gt 0 ]; do {
        case $1 in
	-m)
	    if [ -z "$flag" ]; then {
		flag="-m";
	    } else {
		echo "$usage";
		exit 1;
	    } fi;
	    ;;
	-s)
	    if [ -z "$flag" ]; then {
		flag="-s";
	    } else {
		echo "$usage";
		exit 1;
	    } fi;
	    ;;
	-name)
    	    shift;
	    if [ -z "$dic_name" ]; then {
		dic_name=$1;
	    } else {
		echo "$usage";
		exit 1;
	    } fi;
	    ;;
	-c)
    	    shift;
	    if [ -z "$compat_flag" ]; then {
		compat_flag="-c $1";
	    } else {
		echo "$usage";
		exit 1;
	    } fi;
	    ;;
	*)
	    if [ -z "$text_file" ]; then {
		text_file=$1;
	    } else {
		args="$args $1";
	    } fi;
	    ;;
	esac;
	shift;
    }; done
/* input file */
    if [ -z "$text_file" ]; then
	echo "$usage";
	exit 1;
    fi;
    if [ ! -r $text_file ]; then 
	echo "$usage";
	exit 1;
    fi;
    if [ -d $text_file ]; then 
	echo "$usage";
	exit 1;
    fi;
    if [ "$dic_name" != "" ]; then
	dic_ck="`echo $dic_name | \
		    awk -F. '{
			printf("%s", $NF)
		    }'`";
	case "$dic_ck" in
	    d) fqsuff="fq" ;;
	    cbd) fqsuff="cld" ;;
	    *)
	    echo "Invalid name : $dic_name"
            exit 1 ;;
	esac
        dic_ckn="`echo $dic_name | \
		awk -F/ '{print $NF}' | \
		awk -F. '{print NF}'`";
        if [ $dic_ckn -ne 2 ]; then
	    echo "Invalid name : $dic_name";
            exit 1;
        fi;
        dic_ck="`echo $dic_name | \
		awk -F/ '{print $NF}' | \
		awk -F. '{
			printf("%s", $(NF-1))
                }'`";
        if [ "$dic_ck" =  "" ]; then
	    echo "Invalid name : $dic_name";
            exit 1;
        fi
    fi;
/* mwd or swd */
    if [ -z "$flag" ]; then
	flag="-m";
    fi;
/* temp file of cpp */
    cpp_text="`echo $text_file | \
		awk -F/ '{print $NF}' | \
		awk -F. '{
		    for(i = 1; i < NF; i++)
			printf("%s.", $i)
		}'`";
    if [ -z "$cpp_text" ]; then
	cpp_text="`echo $text_file | \
		awk -F/ '{print $NF}'`".;
    fi;
    cpp_text=$TMPDIR/"$cpp_text"cpp;
/* temp file of splitword */
    spl_text="`echo $text_file | \
		awk -F/ '{print $NF}' | \
		awk -F. '{
		    for(i = 1; i < NF; i++)
			printf("%s.", $i)
		}'`";
    if [ -z "$spl_text" ]; then
	spl_text="`echo $text_file | \
		awk -F/ '{print $NF}'`".;
    fi;
    spl_text=$TMPDIR/"$spl_text"spl;
/* temp file of backup */
    bck_text="`echo $text_file | \
		awk -F/ '{print $NF}' | \
		awk -F. '{
		    for(i = 1; i < NF; i++)
			printf("%s.", $i)
		}'`";
    if [ -z "$bck_text" ]; then
	bck_text="$text_file".;
    fi;
    bck_text="$bck_text"bk;
/* output file */
    out="`echo $text_file | \
		awk -F/ '{print $NF}' | \
		awk -F. '{
		    for(i = 1; i < NF; i++)
			printf("%s.", $i)
		}'`";
    if [ -z "$out" ]; then
	out="`echo $text_file | \
		awk -F/ '{print $NF}'`".;
    fi
#ifdef USE_OBSOLETE_STYLE_FILENAME
    out="$out"d;
#else
    out="$out"cbd;
#endif
    if [ -z "$dic_name" ]; then
	dic_name=$out;
    fi
/* child name */
    child="`echo $text_file | \
		awk -F/ '{print $NF}' | \
		awk -F. '{
		    for(i = 1; i < NF; i++)
			printf("%s.", $i)
		}'`";
    if [ -z "$child" ]; then
	child="`echo $text_file | \
		awk -F/ '{print $NF}'`".;
    fi
    if [ "OPT$child" = "OPT." ]; then
	  echo "Invalid name : $text_file";
	  exit 1;
    fi

    toplen="`echo $child |awk '{printf("%d", index($1, "."))}'`"
    arglen="`echo $child |awk '{printf("%d", length($1))}'`"

    if [ $toplen -ne $arglen ]; then
        echo "Invalid name : $text_file";
        exit 1;
    fi

    if [ "x$fqsuff" != "x" ]; then
	fqoutopt="-o $child$fqsuff"
    fi
    if [ "OPT$flag" = "OPT-m" ]; then
	child="$child"mwd;
    else
	child="$child"swd;
    fi
/* main routin */
    trap "rm -f $cpp_text $spl_text; exit 1;" 2;
    if echo cpptest | CPP $args >/dev/null 2>&1; then
        echo "forcpp -7 < $text_file |" CPP "$args | forcpp -8 > $cpp_text";
        forcpp -7 < $text_file | CPP $args | forcpp -8 > $cpp_text;
    else
	echo "Cannot use cpp !!"
        echo "cp  $text_file $cpp_text";
        cp $text_file  $cpp_text;
    fi
    if [ $? != 0 ]; then
	echo "mkbindic: fatal error. exit";
	rm -f $cpp_text $spl_text;
	exit 1;
    fi
    echo "splitword $cpp_text > $spl_text";
    splitword $cpp_text > $spl_text;
    if [ $? != 0 ]; then
	echo "mkbindic: fatal error. exit";
	rm -f $cpp_text $spl_text;
	exit 1;
    fi;
    echo "mv $text_file $bck_text";
    mv $text_file $bck_text;
    echo "mergeword -s $spl_text > $text_file";
    mergeword -s "$spl_text" > "$text_file";
    if [ $? != 0 ]; then
        mv $bck_text $text_file;
	echo "mkbindic: fatal error. exit";
	rm -f $cpp_text $spl_text;
	exit 1;
    fi;
#ifdef nec_ews
/* \c for crxdic echo back unexpected \n */
    echo "crxdic $flag $compat_flag -o $dic_name $text_file\c";
#else
    echo "crxdic $flag $compat_flag -o $dic_name $text_file";
#endif
    crxdic $flag $compat_flag -o $dic_name $text_file;
    if [ $? != 0 ]; then
        mv $bck_text $text_file;
	echo "mkbindic: fatal error. exit";
	rm -f $cpp_text $spl_text;
	exit 1;
    fi;
    echo "crfreq -div 512 $fqoutopt $dic_name $child";
    crfreq -div 512 $fqoutopt $dic_name $child;
    if [ $? != 0 ]; then
        mv $bck_text $text_file;
	echo "mkbindic: fatal error. exit";
	rm -f $cpp_text $spl_text;
	exit 1;
    fi;
    mv $bck_text $text_file;
    echo "rm $cpp_text $spl_text";
    rm -f $cpp_text $spl_text;
    exit $?;
}
