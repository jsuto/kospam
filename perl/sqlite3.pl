#!/usr/bin/perl
##
## prepare.pl, 2005.12.02, SJ
##
## prepare raw data file to import it to mysql tables
##

die "usage: $0 <HAM> <SPAM> <temp file>"
     unless ( ($HAM = $ARGV[0]) && ($SPAM = $ARGV[1]) && ($TEMP = $ARGV[2]) );

open(HAM, "<$HAM") || die "cannot open $HAM";
open(SPAM, "<$SPAM") || die "cannot open $SPAM";
open(TEMP, ">$TEMP") || die "cannot open $TEMP";

print STDERR "reading HAM . . .";

$ts1 = time();

while(<HAM>){
   chomp;

   if($T{$_}){
      ($h, $s) = split(/:/, $T{$_});
      $h++;
   }
   else {
      $h = 1;
      $s = 0;
   }
   $T{$_} = $h . ":" . $s;
   #print "$_ $h:$s\n";
}
close HAM;

$delta = time() - $ts1; print STDERR " $delta [sec]\n";


print STDERR "reading SPAM . . .";

$ts1 = time();

while(<SPAM>){
   chomp;

   if($T{$_}){
      ($h, $s) = split(/:/, $T{$_});
      $s++;
   }
   else {
      $s = 1;
      $h = 0;
   }
   $T{$_} = $h . ":" . $s;
   #print "$_ $h:$s\n";
}
close SPAM;

$delta = time() - $ts1; print STDERR " $delta [sec]\n";

print STDERR "writing TEMP . . .";

$ts1 = time();

print TEMP "BEGIN;\n";

foreach $w (keys %T){
   ($nham, $nspam) = split(/:/, $T{$w});
   print TEMP "INSERT INTO t_token (token, uid, nham, nspam) VALUES('$w', 0, $nham, $nspam);\n";
}

print TEMP "COMMIT;\n";

close TEMP;

$delta = time() - $ts1; print STDERR " $delta [sec]\n";
