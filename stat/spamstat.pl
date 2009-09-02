#!/usr/bin/perl
##
##

die "usage: $0 <mon> <day> <hour>" unless ( ($MON = $ARGV[0]) && ($DAY = $ARGV[1]) && ($HOUR = $ARGV[2]) );

%HAM = ();
%SPAM = ();

while(<STDIN>){
   if(/^$MON\ / && /$DAY\ $HOUR\:/ && (/clapf/ || /spamdrop/) && /(H|SP)AM/){

      #May 18 10:57:40 fuku clapf[30787]: 88c75182c1058b2e62f688d90e4441: sj@thorium.datanet.hu got HAM
      #Sep  2 16:09:45 thorium clapf[27091]: 4a9e7ca99bed868c323aa90f9b2a50: sj@acts.hu got HAM, 0.0035, 3541, delay=0.10, delays=0.00/0.01/0.00/0.00/0.00/0.00/0.04/0.00/0.05, relay said: 250 2.0.0 Ok: queued as 95723174AD
      chomp;
      $_ =~ s/\s{1,}/ /g;
      @x = split(/ /, $_);
      $email = $x[6];
      $status = $x[8];

      if($HAM{$email} eq ""){ $HAM{$email} = 0; }
      if($SPAM{$email} eq ""){ $SPAM{$email} = 0; }

      if($status =~ /^HAM/){ $HAM{$email}++; }
      if($status =~ /^SPAM/){ $SPAM{$email}++; }
   }
}

foreach $x (keys %HAM){print "$x $HAM{$x} $SPAM{$x}\n"; }

