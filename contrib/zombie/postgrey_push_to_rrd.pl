#!/usr/bin/perl
##
##

use lib "/usr/local/rrdtool/lib/perl/5.10.1/i486-linux-thread-multi";
use RRDs;

$rrd = "/var/lib/clapf/stat/postgrey.rrd";

$greylisted = 0;
$passed_greylist = 0;
$unaffected = 0;
$whitelisted = 0;

&usage unless $config_file = $ARGV[0];

%cfg = &read_config($config_file);

$maillog = $cfg{'maillog'};



$now = time();
@t = localtime($now);

$Y = $t[5] + 1900;
$M = $t[4] + 1; if($M < 10){ $M = "0$M"; }
$D = $t[3]; ###if($D < 10){ $D = "0$D"; }

$hour = $t[2]; if($hour < 10){ $hour = "0$hour"; }

$hourstart = $now - ($now % 3600);

open(F, "<$maillog") || die "open: $maillog";
while(<F>){
   #Jul 28 13:07:08 aaaa/localhost postgrey[28931]: action=greylist, reason=new, client_name=unknown, 
   if(/^[A-Z][a-z]{2}\ / && /\ $D\ $hour\:/ && /postgrey/ ){
      ##print;

      if(/action=greylist, reason=new/){ $greylisted++; }
      elsif(/action=pass, reason=unlikely a zombie/){ $unaffected++; }
      elsif(/action=pass, reason=triplet found/){ $passed_greylist++; }
      elsif(/action=pass, reason=client whitelist/ || /action=pass, reason=recipient whitelist/ || /action=pass, reason=client AWL/){ $whitelisted++; }
   }

}


RRDs::update ($rrd,"$hourstart:$greylisted:$passed_greylist:$unaffected:$whitelisted");

my $ERR=RRDs::error;
die "ERROR while updating $rrd: $ERR\n" if $ERR;


sub usage {
   die "usage: $0 <config file>\n";
}


sub read_config {
   my ($cfg) = @_;

   %CFG = ();

   open(F, "<$cfg") || die "cannot open: $cfg\n";

   while(<F>){
      if($_ !~ /^\#/ && $_ !~ /^\;/) {
         chomp;
         @l = split("=", $_);

         $CFG{$l[0]} = $l[1];
      }
   }

   close F;

   return %CFG;
}

