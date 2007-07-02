#!/usr/bin/perl
##
## shrink.pl, 2007.05.29, SJ
## eliminate multiple occurances of tokens

%occ;

$min_word_len = 3;
$max_word_len= 19;

while(<STDIN>){
   # flush all entries
   if(/\*\*\* NEW_MSG_STARTS_HERE /){
      (undef, undef, $tot, undef) = split(/ /, $_);
      &flush1($_);

   }
   else {
      chomp;
      $occ{$_}++;

   }
}

&flush1();

print STDERR "$tot\n";

sub flush1 {
   my ($l) = @_;

   if(%occ){
      foreach $w (sort keys %occ){ print "$w\n"; }
   }

   undef %occ;
}
