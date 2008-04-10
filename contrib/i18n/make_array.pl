#!/usr/bin/perl

$n = 0;
$max = 300;

print "unsigned long long lang_xxx[MAX_NGRAM] = {\n   ";

while(<STDIN>){
   chomp;
   $n++;
   (undef, $x) = split(/ /, $_);
   print $x . "ULL, ";

   if($n >= $max-2){ last; }

   if($n % 7 == 0){ print "\n   "; }
}

print "\n   0\n};\n";

