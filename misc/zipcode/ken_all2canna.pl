#!/usr/bin/perl
#
# Copyright (C) 2002 Red Hat, Inc.
#
# Converter from ken_all.csv to zipcode.p file
# 

sub cutdq {
  local($str) = $_[0];
  $str =~ s/^\"//;
  $str =~ s/\"$//;
  return $str;
}

undef @sum;
@sum = ();
$brace = 0;

while(<>) {
  next if( $_ eq "\x1a" );
  # /の次に番地がくる場合/
  next if( /\244\316\274\241\244\313\310\326\303\317\244\254\244\257\244\353\276\354\271\347/ );
  undef @arry;
  @arry = split(/,/);
  if( $brace == 1 && defined($zip) && $zip eq &cutdq($arry[2]) ) {
    pop(@sum);
    $yomi2 .= &cutdq($arry[5]);
    $kanji2 .= &cutdq($arry[8]);
    $brace = 0 if( ! $kanji2 =~ /\([^\)]*$/ );
    push(@sum, "$zip $kanji0$kanji1$kanji2 #CNS 0\n");
    next;
  }
  $brace = 0;
  $zip = &cutdq($arry[2]);
  $yomi0 = &cutdq($arry[3]);
  $yomi1 = &cutdq($arry[4]);
  $yomi2 = &cutdq($arry[5]);

  $kanji0 = &cutdq($arry[6]);
  $kanji1 = &cutdq($arry[7]);
  $kanji2 = &cutdq($arry[8]);
  # s/\(([0-9]*階)\)/$1/
  $kanji2 =~ s/\(([0-9]*\263\254)\)/$1/;
  $brace = 1 if( $kanji2 =~ /\([^\)]*$/ );

  # "以下に掲載がない場合"
  if( $kanji2 eq "\260\312\262\274\244\313\267\307\272\334\244\254\244\312\244\244\276\354\271\347" ) {
    $zip = substr($zip, 0, 3) if( substr($zip, 3, 7) eq "0000");
    $kanji2 = "";
  }
  push(@sum, "$zip $kanji0$kanji1$kanji2 #CNS 0\n");
}

#print $#sum, "\n";
for($i=0;$i<=$#sum;$i++) {
  print $sum[$i];
}
