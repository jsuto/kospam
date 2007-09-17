#!/usr/bin/perl
##
## process_syslog.pl, 2007.05.18, SJ
##

$spam_limit = 0.92;
$n = 0;
$n_ham = 0;
$n_spam = 0;
$tot_size = 0;
$tot_time = 0;
$tot_ham_size = 0;
$tot_ham_time = 0;
$tot_spam_size = 0;
$tot_spam_time = 0;

$avg_size = 0;
$avg_time = 0;
$avg_ham_size = 0;
$avg_ham_time = 0;
$avg_spam_size = 0;
$avg_spam_time = 0;

die "usage: $0 <mon> <day> <hour>" unless ( ($MON = $ARGV[0]) && ($DAY = $ARGV[1]) && ($HOUR = $ARGV[2]) );

while(<STDIN>){
   if(/^$MON\ / && /$DAY\ $HOUR\:/ && (/clapf/ || /spamdrop/) && /\:\ 0\./){
      $_ =~ s/\s{1,}/ /g;

      @x = split(/ /, $_);
      #Feb  7 09:56:21 thorium clapf[20117]: 1e444677176339a7c766d47db982a8: 0.5086 3490 in 291443 [usec]
      #Mar  7 11:32:06 thorium clapf[3987]: 8f7d96b0517f1e47db1e40851a86ac: 1.0000 3226 in 12 [ms]
      $spamicity = $x[6];
      $size = $x[7];
      $time = $x[9];

      $n++;
      $tot_size += $size;
      $tot_time += $time;

      if($spamicity < $spam_limit){
         $n_ham++;
         $tot_spam_size += $size;
         $tot_spam_time += $time;
      }
      else {
         $n_spam++;
         $tot_ham_size += $size;
         $tot_ham_time += $time;
      }

      #print "$spamicity $size $time\n";
   }
}

if($n > 0){
   $avg_size = sprintf("%.0f", $tot_size / $n);
   $avg_time = sprintf("%.0f", $tot_time / $n);
}

if($n_ham > 0){
   $avg_ham_size = sprintf("%.0f", $tot_ham_size / $n_ham);
   $avg_ham_time = sprintf("%.0f", $tot_ham_time / $n_ham);
}

if($n_spam > 0){
   $avg_spam_size = sprintf("%.0f", $tot_spam_size / $n_spam);
   $avg_spam_time = sprintf("%.0f", $tot_spam_time / $n_spam);
}

print time() . " $n $avg_size $avg_time $n_ham $avg_ham_size $avg_ham_time $n_spam $avg_spam_size $avg_spam_time\n";

#print "total: $n, avg size: $avg_size, avg time: $avg_time [ms]\n";
#print "ham: $n_ham, avg size: $avg_ham_size, avg time: $avg_ham_time [ms]\n";
#print "spam: $n_spam, avg size: $avg_spam_size, avg time: $avg_spam_time [ms]\n";


