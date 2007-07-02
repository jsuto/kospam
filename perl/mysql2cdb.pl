#!/usr/bin/perl
##
## mysql2cdb.pl, 2007.01.07, SJ
##

$freq_min = 5;
$cnt = 0;

$token_only_in_ham_spamicity = 0.0001;
$token_only_in_spam_spamicity = 0.9999;

die "usage: $0 <num of ham> <num of spam> <cdb base>" unless ( ($num_of_ham = $ARGV[0]) && ($num_of_spam = $ARGV[1]) && ($CDBBASE = $ARGV[2]) );

unlink $CDBBASE . ".cdb";

$rawtextfile = $CDBBASE . ".raw";
open(RAW, ">$rawtextfile") || die "cannot open: $rawtextfile\n";

while(<STDIN>){
   chomp;

   $_ =~ s/\s{1,}/ /g;

   ($w, $h, $s) = split(/ /, $_);

   if($s == 0){ $spamicity = $token_only_in_ham_spamicity; }
   if($h == 0){ $spamicity = $token_only_in_spam_spamicity; }

   # skip hapaxes

   if($h > 0 || $s > 0){
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

      # Markovian
      #$spamicity = &calc_probability_markov($c1, $c2, $h, $s, $wmax, $w);
   }

   if($spamicity < $token_only_in_ham_spamicity){ $spamicity = $token_only_in_ham_spamicity; }
   if($spamicity > $token_only_in_spam_spamicity){ $spamicity = $token_only_in_spam_spamicity; }

   # insert to cdb file

   $spamicity = sprintf("%.4f", $spamicity);
   #$cdb->insert($w, $spamicity);

   print RAW "$w $spamicity\n";

   $cnt++;
}

#$cdb->finish or die "$0: CDB_File finish failed: $!\n";

close RAW;

print STDERR "Put $cnt records\n";

print STDERR "Creating cdb file . . .\n";

system("cdb -c -m -w $CDBBASE.cdb $CDBBASE.raw");

print STDERR "Done.\n";
