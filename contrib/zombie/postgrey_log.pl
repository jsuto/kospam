#!/usr/bin/perl
##
##

use File::Tail;
use Date::Parse;

use DBI;
use POSIX qw(setsid);

$greylisted = 0;
$passed_greylist = 0;
$not_affected = 0;
$whitelisted = 0;

$db = "";

&usage unless $config_file = $ARGV[0];

%cfg = &read_config($config_file);

$maillog = $cfg{'maillog'};
$database = $cfg{'historydb'};
$user = $cfg{'mysqluser'};
$password = $cfg{'mysqlpwd'};
$socket = $cfg{'mysqlsocket'};
$host = $cfg{'mysqlhost'};
$port = $cfg{'mysqlport'};
$pidfile = $cfg{'historypid'};

if($database =~ /\//){ $db = "sqlite3"; }
else { $db = "mysql"; }

if($maillog eq "" || $db eq "") { &usage; }


if($db eq "sqlite3") { $dsn = "dbi:SQLite:dbname=$database"; }
if($db eq "mysql") { 
  if ($socket eq undef) {
    $dsn = "dbi:mysql:database=$database;host=$host;port=$port";}
  else {
    $dsn = "dbi:mysql:database=$database;mysql_socket=$socket";}
}




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
      elsif(/action=pass, reason=unlikely a zombie/){ $not_affected++; }
      elsif(/action=pass, reason=triplet found/){ $passed_greylist++; }
      elsif(/action=pass, reason=client whitelist/ || /action=pass, reason=recipient whitelist/ || /action=pass, reason=client AWL/){ $whitelisted++; }
   }

}


$dbh = DBI->connect($dsn, $user, $password) or die "cannot open database: $database";

$stmt = "INSERT INTO postgrey (ts, greylisted, passed_greylist, not_affected, whitelisted) VALUES(?, ?, ?, ?, ?)";
$sth_postgrey = $dbh->prepare($stmt);

$sth_postgrey->execute($hourstart, $greylisted, $passed_greylist, $not_affected, $whitelisted);

$dbh->disconnect;


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

