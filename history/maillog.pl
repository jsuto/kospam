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

$stmt = "INSERT INTO smtp (ts, queue_id, `to`, to_domain, orig_to, orig_to_domain, relay, delay, result, clapf_id) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
$sth_smtp = $dbh->prepare($stmt);

$stmt = "INSERT INTO clapf (ts, clapf_id, `from`, `fromdomain`, rcpt, rcptdomain, result, spaminess, `size`, relay, delay, queue_id2, subject, virus) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
$sth_clapf = $dbh->prepare($stmt);


srand;


while (defined($line = $file->read)) {

   if($line =~ /\ postfix\/smtpd\[/ || $line =~ /\ postfix\/cleanup\[/ || $line =~ /\ postfix\/(l|s)mtp\[/ || $line =~ /\ postfix\/local\[/ || $line =~ /\ clapf\[/ || $line =~ /\ postfix\/virtual\[/ || $line =~ /\ postfix\/pipe\[/) {
      chomp($line);

      $queue_id = $to = $orig_to = $to_domain = $orig_to_domain = $clapf_id = $relay = $status = $result = $queue_id2 = "";
      $delay = 0;

      ##print "xx: " . $line;

      $line =~ s/\ {1,}/ /g;

      @l = split(/ /, $line);

      #Sep  3 09:22:20 thorium postfix/smtpd[2270]

      $ts = str2time($l[0] . " " . $l[1] . " " . $l[2]);
      $hostname = $l[3];


      # ... postfix/smtpd[8371]: NOQUEUE: reject: RCPT from unknown[70.96.35.34]: 554 5.7.1 Service unavailable; Client host [70.96.35.34] blocked using zen.spamhaus.org; http://www.spamhaus.org/query/bl?ip=70.96.35.34; from=<d2221028@ms29.hinet.net> to=<sj@acts.hu> proto=ESMTP helo=<[70.96.35.34]>

      if($line =~ /\ postfix\/smtpd\[/ && $line =~ /from=\<([\w\W]{0,})\> to=\<([\w\W]{3,})\> proto=([\w]+) helo=/) {

         $clapf_id = "xxxxxxxx" . &randomstring(22);
         $spaminess = 0.5; $delay = $size = 0;
         $subject = $queue_id2 = $virus = $relay = "";
         $result = "SPAM";

         $from = $1;
         $rcpt = $2;

         (undef, $fromdomain) = split(/\@/, $from);
         (undef, $rcptdomain) = split(/\@/, $rcpt);

         $sth_clapf->execute($ts, $clapf_id, lc $from, lc $fromdomain, lc $rcpt, lc $rcptdomain, $result, $spaminess, $size, $relay, $delay, $queue_id2, $subject, $virus) || print $line . "\n";
      }


      # ... postfix/smtpd[12038]: 0561617020: client=unknown[92.84.77.34]

      if($line =~ /\ postfix\/smtpd\[/ && $line =~ /\]\:\ ([\w]+)\: client=([\w\W]{3,})/) {
         $sth_smtpd->execute($ts, $1, $2) || print $line . "\n";
      }


      #Sep  3 09:30:22 thorium postfix/cleanup[2311]: D20E617022: message-id=<f14f01c89af3$8a784f50$3bc40658@acts.hu>

      if($line =~ /\ postfix\/cleanup\[/ && $line =~ /\]\:\ ([\w]+)\: message\-id=<([\w\W]+)>/) {
         $sth_cleanup->execute($ts, $1, $2) || print $line . "\n";
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

         $sth_smtp->execute($ts, $queue_id, lc $to, lc $to_domain, lc $orig_to, lc $orig_to_domain, $relay, $delay, $status, $clapf_id) || print $line . "\n";
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

         $sth_smtp->execute($ts, $queue_id, lc $to, lc $to_domain, lc $orig_to, lc $orig_to_domain, $relay, $delay, $status, $clapf_id) || print $line . "\n";
      }


      # Sep  3 10:00:07 thorium clapf[2578]: 4a9f7787c9b56ae1a646fd8ca6cc5b: sj@acts.hu got SPAM, 1.0000, 2731, relay=127.0.0.1:10026, delay=0.06, delays=0.00/0.00/0.00/0.00/0.00/0.00/0.01/0.00/0.04, status=250 2.0.0 Ok: queued as E691917023

      if($line =~ /\ clapf\[/ && $line =~ /status=/) {

         $clapf_id = $from = $fromdomain = $rcpt = $rcptdomain = "";
         $delay = $size = 0;
         $spaminess = 0.5;
         $virus = "";
         $subject = $queue_id2 = "";


         ($clapf_id, undef) = split(/\:/, $l[5]);


         if($line =~ /from=\<([\w\W]{0,})\>, to=\<([\w\W]{3,})\>, spaminess=([\d\.]+), result=([\w\W]{3,}), size=([\d]+), relay=([\w\W]{3,}), delay=([\d\.]+), delays=([\d\.\/]+), status=([\w\W]{0,})\,\ subject=([\w\W]+)/) {
            $from = $1; $rcpt = $2;
            $spaminess = $3;
            $result = $4;
            $size = $5;
            $relay = $6;
            $delay = $7;
            $status = $9;
            $subject = $10;
         }


         if($status =~ /250/ && $status =~ /Ok/i) {
            (undef, $queue_id2) = split(/queued\ as\ /, $status);
         }


         (undef, $fromdomain) = split(/\@/, $from);
         (undef, $rcptdomain) = split(/\@/, $rcpt);


         if($result =~ /VIRUS\ \(([\w\W]+)\)/){
            $result = 'VIRUS';
            $virus = $1;
         }


         $sth_clapf->execute($ts, $clapf_id, lc $from, lc $fromdomain, lc $rcpt, lc $rcptdomain, $result, $spaminess, $size, $relay, $delay, $queue_id2, $subject, $virus) || print $line . "\n";
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


sub randomstring {
   my ($length) = @_;
   my @chars = ('a'..'f', '0'..'9');
   my $s = "";

   for (my $i=0; $i <= $length; $i++) {
      $s .= $chars[rand @chars];
   }

   return $s;
}

