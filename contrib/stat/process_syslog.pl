#!/usr/bin/perl
##
## process_syslog.pl, 2009.12.29, SJ
##

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
   #if(/^$MON\ / && /$DAY\ $HOUR\:/ && (/clapf/ || /spamdrop/) && /\:\ 0\./){
   if(/^$MON\ / && /$DAY\ $HOUR\:/ && (/clapf/ || /spamdrop/) && /\ delay\=/){
      $_ =~ s/\s{1,}/ /g;
      $_ =~ s/\,//g;

      @x = split(/ /, $_);

      #Sep 11 16:16:35 thorium clapf[28688]: 4aaa5bc2e143b68ed701d8833be296: sj@acts.hu got SPAM, 1.000, 2378, relay=127.0.0.1:10026, delay=0.12, delays=0.01/0.01/0.00/0.00/0.00/0.02/0.00/0.04/0.00/0.05, status=250 2.0.0 Ok: queued as 5597A17021

      $is_spam = $x[8];
      $size = $x[10];
      (undef, $time) = split(/=/, $x[12]);

      $time *= 1000;

      $n++;
      $tot_size += $size;
      $tot_time += $time;

      if($is_spam eq "HAM"){
         $n_ham++;
         $tot_spam_size += $size;
         $tot_spam_time += $time;
      }
      else {
         $n_spam++;
         $tot_ham_size += $size;
         $tot_ham_time += $time;
      }

      #print "$is_spam $size $time\n";
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


