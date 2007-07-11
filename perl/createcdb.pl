#!/usr/bin/perl
##
## createcdb.pl, 2007.01.08, SJ
##
## creates a consolidated CDB file with tokens as key and
## the value of spamicity as the value

$freq_min = 5;

$token_only_in_ham_spamicity = 0.0001;
$token_only_in_spam_spamicity = 0.9999;

$spam_1_skipped = 0;
$ham_1_skipped = 0;

$cnt = 0;

die "usage: $0 <HAM> <SPAM> <num of ham> <num of spam> <CDB basename>"
      unless ( ($HAM = $ARGV[0]) && ($SPAM = $ARGV[1]) && ($num_of_ham = $ARGV[2]) && ($num_of_spam = $ARGV[3]) && ($CDBBASE = $ARGV[4]) );

unlink $CDBBASE . ".cdb";

$TEMP = $CDBBASE . ".raw";

# read HAM and SPAM files

open(HAM, "<$HAM") || die "cannot open $HAM";
open(SPAM, "<$SPAM") || die "cannot open $SPAM";
open(TEMP, ">$TEMP") || die "cannot open $TEMP";

$ts1 = time();

print STDERR "reading HAM . . .";

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

$ts2 = time();

$delta = $ts2 - $ts1;
print " $delta [sec]\n";

print "reading SPAM . . .";
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

$ts2 = time(); $delta = $ts2 - $ts1; print " $delta [sec]\n";

# and create the consolidated database

#$cdb = new CDB_File($CDBBASE . ".cdb", "CDBFILE.$$") or die "$0: new CDB_File failed: $!\n";

print STDERR "creating CDB file . . .";

$ts1 = time();

foreach $w (keys %T){

   # calculate spamicity

   ($h, $s) = split(/:/, $T{$w});

   # skip hapaxes (tokens occurring once)

   if($h == 0 && $s == 1){ $spam_1_skipped++; next; }
   if($h == 1 && $s == 0){ $ham_1_skipped++; next; }

   if($s == 0){ $spamicity = $token_only_in_ham_spamicity; }
   if($h == 0){ $spamicity = $token_only_in_spam_spamicity; }

   $ham_prob = $h / $num_of_ham;
   $spam_prob = $s / $num_of_spam;

   if($ham_prob > 1){ $ham_prob = 1; }
   if($spam_prob > 1){ $spam_prob = 1; }

   $spamicity = $spam_prob / ($ham_prob + $spam_prob);

   # how to deal with rare words < 5

   if($h < $freq_min && $s < $freq_min){
      $n = $h;
      if($s > $n){
         $n = $s;
      }
      $spamicity = (0.5 + $n * $spamicity) / (1 + $n);
   }

   # insert to cdb file

   if($spamicity < $token_only_in_ham_spamicity){ $spamicity = $token_only_in_ham_spamicity; }
   if($spamicity > $token_only_in_spam_spamicity){ $spamicity = $token_only_in_spam_spamicity; }

   $spamicity = sprintf("%.4f", $spamicity);
   #$cdb->insert($w, $spamicity);

   # create raw token file too
   print TEMP "$w $spamicity\n";

   $cnt++;
}

close TEMP;

system("cdb -c -m -w $CDBBASE.cdb $CDBBASE.raw");

#$cdb->finish or die "$0: CDB_File finish failed: $!\n";

$ts2 = time(); $delta = $ts2 - $ts1; print " $delta [sec]\n";

print STDERR "Put $cnt records\n";
print STDERR "Skipped: $ham_1_skipped ham only token\n";
print STDERR "Skipped: $spam_1_skipped spam only token\n";

unlink $TEMP;
