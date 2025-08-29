#!/bin/perl
#
# Copyright (C) 2002 Red Hat, Inc.
#
# bracecut - duplicate entries with braces
# (but just cut currently)
#

while(<>) {
 @arry = split(' ');
 $kanji = $arry[1];

 $kanji =~ s/\([^\(]*\)$// while($kanji =~ /\([^\(]*\)$/);
# if( $kanji ne $arry[1] ) {
#   $arry[1] = $kanji;
#   print join(' ',@arry), "\n";
# }
 $arry[1] = $kanji;
 print join(' ',@arry), "\n";
 undef @arry;
}
