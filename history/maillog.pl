#!/usr/bin/perl

use File::Tail;
use Date::Parse;

use DBI;
use POSIX qw(setsid);


$db = "";

&usage unless $config_file = $ARGV[0];

%cfg = &read_config($config_file);

$maillog = $cfg{'maillog'};
$database = $cfg{'historydb'};
$user = $cfg{'mysqluser'};
$password = $cfg{'mysqlpwd'};
$host = $cfg{'mysqlhost'};
$port = $cfg{'mysqlport'};
$pidfile = $cfg{'historypid'};

if($database =~ /\//){ $db = "sqlite3"; }
else { $db = "mysql"; }

if($maillog eq "" || $db eq "") { &usage; }


if($db eq "sqlite3") { $dsn = "dbi:SQLite:dbname=$database"; }
if($db eq "mysql") { $dsn = "dbi:mysql:database=$database;host=$host;port=$port"; }

&daemonize;

if($pidfile) { &writepid($pidfile); }


$SIG{'INT'} = 'nice_exit';
$SIG{'TERM'} = 'nice_exit';


$file = File::Tail->new(name => $maillog, maxinterval=> 5) or die "cannot read $maillog";


$dbh = DBI->connect($dsn, $user, $password) or die "cannot open database: $database";

$stmt = "INSERT INTO smtpd (ts, queue_id, client_ip) VALUES(?, ?, ?)";
$sth_smtpd = $dbh->prepare($stmt);

$stmt = "INSERT INTO cleanup (ts, queue_id, message_id) VALUES(?, ?, ?)";
$sth_cleanup = $dbh->prepare($stmt);

$stmt = "INSERT INTO qmgr (ts, queue_id, `from`, `from_domain`, size) VALUES(?, ?, ?, ?, ?)";
$sth_qmgr = $dbh->prepare($stmt);

$stmt = "INSERT INTO smtp (ts, queue_id, `to`, to_domain, orig_to, orig_to_domain, relay, delay, result, clapf_id) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
$sth_smtp = $dbh->prepare($stmt);

$stmt = "INSERT INTO clapf (ts, queue_id, rcpt, result, spaminess, relay, delay, queue_id2, virus) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)";
$sth_clapf = $dbh->prepare($stmt);



while (defined($line = $file->read)) {

   if($line =~ /\ postfix\/smtpd\[/ || $line =~ /\ postfix\/cleanup\[/ || $line =~ /\ postfix\/qmgr\[/ || $line =~ /\ postfix\/(l|s)mtp\[/ || $line =~ /\ postfix\/local\[/ || $line =~ /\ clapf\[/ || $line =~ /\ postfix\/virtual\[/ || $line =~ /\ postfix\/pipe\[/) {
      chomp($line);

      $queue_id = $to = $orig_to = $to_domain = $orig_to_domain = $clapf_id = $relay = $status = $result = $queue_id2 = "";
      $delay = 0;

      ##print "xx: " . $line;

      $line =~ s/\ {1,}/ /g;

      @l = split(/ /, $line);

      #Sep  3 09:22:20 thorium postfix/smtpd[2270]

      $ts = str2time($l[0] . " " . $l[1] . " " . $l[2]);
      $hostname = $l[3];


      # ... postfix/smtpd[12038]: 0561617020: client=unknown[92.84.77.34]

      if($line =~ /\ postfix\/smtpd\[/ && $line =~ /client=/) {
         ($queue_id, undef) = split(/\:/, $l[5]);

         (undef, $client_ip) = split(/client=/, $line);
         ($client_ip, undef) = split(/,/, $client_ip);

         $sth_smtpd->execute($ts, $queue_id, $client_ip) || print $line . "\n";
      }


      #Sep  3 09:30:22 thorium postfix/cleanup[2311]: D20E617022: message-id=<f14f01c89af3$8a784f50$3bc40658@acts.hu>

      if($line =~ /\ postfix\/cleanup\[/ && $line =~ /message\-id=/) {
         $message_id = "";

         ($queue_id, undef) = split(/\:/, $l[5]);
         (undef, $message_id) = split(/=/, $l[6]);

         $message_id =~ s/\<|\>|\,//g;

         $sth_cleanup->execute($ts, $queue_id, $message_id) || print $line . "\n";
      }


      # Sep 3 09:30:22 thorium postfix/qmgr[3010]: D20E617022: from=<aaa@freemail.hu>, size=2731, nrcpt=1 (queue active)

      if($line =~ /\ postfix\/qmgr\[/ && $line =~ /from=/) {
         $from = ""; $from_domain = ""; $size = 0;

         ($queue_id, undef) = split(/\:/, $l[5]);

         if($line =~ /from=\<([\w\W]+)\>/){
            $from = $1;
            (undef, $from_domain) = split(/\@/, $from);
         }

         (undef, $size) = split(/=/, $l[7]);

         $size =~ s/\,//;

         $sth_qmgr->execute($ts, $queue_id, $from, $from_domain, $size) || print $line . "\n";
      }


      if($line =~ /\ postfix\/(l|s)mtp\[/ && $line =~ /to=/) {
         ($queue_id, undef) = split(/\:/, $l[5]);

         ###if($line =~ /to=\<([\w\W]+)\>,/){
         if($line =~ /\ to=\<([\w\d\.\-\_@\=\+]+)\>,/){
            $to = $1;
         }

         next if $to eq "";

         (undef, $to_domain) = split(/\@/, $to);

         (undef, $relay) = split(/relay=/, $line);
         ($relay, undef) = split(/,/, $relay);

         (undef, $delay) = split(/delay=/, $line);
         ($delay, undef) = split(/,/, $delay);

         $status = "";

         if($line =~ /status=([\w\ ]+) \(([\w\W]+)\)/){
            $status = "$1 $2";
         }

         if($line =~ / ([a-f0-9]{30,32}) /){
            $clapf_id = $1;
         }

         $sth_smtp->execute($ts, $queue_id, $to, $to_domain, $orig_to, $orig_to_domain, $relay, $delay, $status, $clapf_id) || print $line . "\n";
      }


      if($line =~ /\ postfix\/local\[/ || $line =~ /\ postfix\/virtual\[/ || $line =~ /\ postfix\/pipe\[/) {
         ($queue_id, undef) = split(/\:/, $l[5]);

         ###if($line =~ /to=\<([\w\W]+)\>,/){
         if($line =~ /\ to=\<([\w\d\.\-\_@\=\+]+)\>,/){
            $to = $1;
         }

         next if $to eq "";

         (undef, $to_domain) = split(/\@/, $to);


         (undef, $x) = split(/orig_to=/, $line);
         ($orig_to, undef) = split(/ /, $x);

         $orig_to =~ s/\<|\>|\,//g;
         (undef, $orig_to_domain) = split(/\@/, $orig_to);

         (undef, $relay) = split(/relay=/, $line);
         ($relay, undef) = split(/,/, $relay);

         (undef, $x) = split(/delay=/, $line);
         ($delay, undef) = split(/ /, $x);

         $delay =~ s/\,//;

         $status = "";

         if($line =~ /status=([\w\ ]+) \(([\w\W]+)\)/){
            $status = "$1 $2";
         }

         if($line =~ / ([a-f0-9]{30,32}) /){
            $clapf_id = $1;
         }

         $sth_smtp->execute($ts, $queue_id, $to, $to_domain, $orig_to, $orig_to_domain, $relay, $delay, $status, $clapf_id) || print $line . "\n";
      }


      # Sep  3 10:00:07 thorium clapf[2578]: 4a9f7787c9b56ae1a646fd8ca6cc5b: sj@acts.hu got SPAM, 1.0000, 2731, relay=127.0.0.1:10026, delay=0.06, delays=0.00/0.00/0.00/0.00/0.00/0.00/0.01/0.00/0.04, status=250 2.0.0 Ok: queued as E691917023

      if($line =~ /\ clapf\[/ && $line =~ /status=/) {
         $spaminess = 0.5;
         $virus = "";

         ($queue_id, undef) = split(/\:/, $l[5]);

         (undef, $relay) = split(/relay=/, $line);
         ($relay, undef) = split(/,/, $relay);

         $rcpt = $l[6];

         $result = $l[8];
         $spaminess = $l[9];

         if($line =~ /VIRUS\ \(([\w\W]+)\)\,\ ([\w\.]+),/){
            $virus = $1;
            $spaminess = $2;
         }

         (undef, $x) = split(/delay=/, $line);
         ($delay, undef) = split(/ /, $x);

         $result =~ s/\,//;
         $spaminess =~ s/\,//;
         $delay =~ s/\,//;

         (undef, $x) = split(/status=/, $line);
         if($x =~ /250/ && $x =~ /Ok/i) {
            (undef, $queue_id2) = split(/queued\ as\ /, $x);
         }

         $sth_clapf->execute($ts, $queue_id, $rcpt, $result, $spaminess, $relay, $delay, $queue_id2, $virus) || print $line . "\n";

      }


      
   }

}


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


sub daemonize {
   chdir '/'                 or die "Can't chdir to /: $!";
   open STDIN, '/dev/null'   or die "Can't read /dev/null: $!";
   open STDOUT, '>>/dev/null' or die "Can't write to /dev/null: $!";
   open STDERR, '>>/dev/null' or die "Can't write to /dev/null: $!";
   defined(my $pid = fork)   or die "Can't fork: $!";
   exit if $pid;
   setsid                    or die "Can't start a new session: $!";
   umask 022;
}


sub writepid {
   my ($pidfile) = @_;

   open(PID, ">$pidfile") || warn "cannot write pidfile: $pidfile\n";

   print PID $$;

   close PID;
}


sub nice_exit {
   if($pidfile) { unlink $pidfile; }

   exit;
}


