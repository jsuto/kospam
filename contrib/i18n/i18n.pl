#!/usr/bin/perl

%NGRAMS = ();

while(<STDIN>){
   $_ =~ s/[\,\.\!\?\(\)\/\:\"\'\%\d\#\&\@\$\=\+]/ /g;
   $_ =~ s/\s{1,}/ /g;
   $_ = lc;
   @x = split(/ /, $_);

   foreach $x (@x){   
      print_ngrams($x);
   }
}

foreach $x (keys %NGRAMS){
   print "$NGRAMS{$x} $x\n";
}

sub print_ngrams {
   my ($word) = @_;
   my $len = length ($word);

   &print_npart($word, 1);
   &print_npart($word, 2);
   &print_npart($word, 3);
   &print_npart($word, 4);
   &print_npart($word, 5);

   #print " * $word\n";
}


sub print_npart {
   my ($word, $x) = @_;
   $len = length $word;

   for($i=0; $i<$len; $i++){
      $ngram = substr($word, $i, $x);
      for($j=length $ngram; $j<$x; $j++){
         $ngram .= "_";
      }

      $NGRAMS{$ngram}++;
      #print "$ngram ";
   }

}

sub my_sort {
   print $a <=> $b;
}
