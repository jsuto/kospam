#!/usr/bin/perl
##
## black.pl, 2007.10.02, SJ
##

use Sys::Syslog;

$hdr = 1;
$ipcnt = 0;
@ip = ();
$now = time();
$mode = 0644;

$IP = "";

$opened = 0;

# to save the letter (1) or discard it (0)
$saveletter = 0;
$messagefile = "/home/you/mail/trapped";
$ipdir = "/opt/av/blackhole";

@mon = qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec);
@wday = qw(Sun Mon Tue Wed Thu Fri Sat Sun);



if($saveletter == 1 && open(AA, ">>$messagefile")){
   @ts = localtime();

   $year = $ts[5]+1900;
   $mon = $mon[$ts[4]];
   $day = $ts[3]; if($day < 10){ $day = " $day"; }
   $wday = $wday[$ts[6]];

   $hour = $ts[2]; if($hour < 10){ $hour = "0$hour"; }
   $min = $ts[1]; if($min < 10){ $min = "0$min"; }
   $sec = $ts[0]; if($sec < 10){ $sec = "0$sec"; }

   $date = "$wday $mon $day $hour:$min:$sec $year";

   $opened = 1;
   print AA "From aaa\@xxx.com $date\n";
}

while(<STDIN>){
   if($saveletter == 1 && $opened == 1){ print AA; }

   if(/^\n$/ || /^\r\n$/){ $hdr = 0; }

   if($hdr == 0){ next; }

   if(/^Received\:\ from\ /){
      $ipcnt++;

      if($_ =~ /(\[)(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})(\])/){
         $ip = $2;

         # get the smtp host connecting to us, ie. the second Received: from header line
         # of course you may get other hosts being further from you in the mail route,
         # just modify $ipcnt.....

         if($ipcnt == 2 && $ip !~ /^127\./ && $ip !~ /^192\.168\./ && $ip !~ /^10\./ && $ip !~ /172\.16\./){
            $IP = $ip;
         }
      }
   }

}

# create or update ip file

if($IP){
   $f = $ipdir . "/" . $IP;

   if(-f $f){ unlink($f); }

   open(F, ">$f");

   chmod $mode, $f;

   openlog('clapf', 'pid', 'mail');
   syslog("info", "putting $IP to blackhole");
   closelog();
}
