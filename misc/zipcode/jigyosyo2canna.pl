#!/usr/bin/perl
#
# Copyright (C) 2002 Red Hat, Inc.
#
# Converter from jigyosyo.csv to jigyosyo.p file
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
  undef @arry;
  @arry = split(/,/);
  $zip = &cutdq($arry[7]);

  $jigyosyo = &cutdq($arry[2]);
  $kanji0 = &cutdq($arry[3]);
  $kanji1 = &cutdq($arry[4]);
  $kanji2 = &cutdq($arry[5]);
  $kanji3 = &cutdq($arry[6]);

  push(@sum, "$zip #KK $kanji0$kanji1$kanji2$kanji3\\ $jigyosyo\n");
}

#print $#sum, "\n";
for($i=0;$i<=$#sum;$i++) {
  print $sum[$i];
}
