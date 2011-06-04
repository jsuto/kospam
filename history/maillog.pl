#!/usr/bin/perl

use File::Tail;
use Date::Parse;
use DBI;
use POSIX qw(setsid);
use POSIX qw(strftime);

use Sys::Syslog;
use Sys::Syslog qw(:DEFAULT setlogsock);

$program = "clapf-maillog";
$priority = "mail|info";

$flush_interval = 10;
$this_partition = "";
$last_dropped_partition = "";

$db = "";

%clapf = ();
%connection = ();
%smtp = ();


&usage unless $config_file = $ARGV[0];

%cfg = &read_config($config_file);

$maillog = $cfg{'maillog'};
$database = $cfg{'historydb'};
$user = $cfg{'mysqluser'};
$password = $cfg{'mysqlpwd'};
$socket = $cfg{'mysqlsocket'};
$host = $cfg{'mysqlhost'};
$port = $cfg{'mysqlport'};
$pidfile = $cfg{'historypid'} || '/var/run/clapf/clapf-maillog.pid';
$days_to_retain_data = $cfg{'days_to_retain_data'} || 14;

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

openlog($program, 'pid', 'mail');

&daemonize;

syslog($priority, "started");

if($pidfile) { &writepid($pidfile); }


$SIG{'INT'} = 'nice_exit';
$SIG{'TERM'} = 'nice_exit';
$SIG{'ALRM'} = 'flush_results';

alarm $flush_interval;


$file = File::Tail->new(name => $maillog, maxinterval=> 5) or die "cannot read $maillog";


$dbh = DBI->connect($dsn, $user, $password) or die "cannot open database: $database";

if($db eq "mysql") { $dbh->{mysql_auto_reconnect} = 1; }

$stmt = "INSERT INTO `connection` (ts, queue_id, client, `from`, `from_domain`, `size`) VALUES(?, ?, ?, ?, ?, ?)";
$sth_connection = $dbh->prepare($stmt);

$stmt = "INSERT INTO smtp (ts, queue_id, `to`, to_domain, orig_to, orig_to_domain, relay, delay, `status`, clapf_id) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
$sth_smtp = $dbh->prepare($stmt);

$stmt = "INSERT INTO clapf (ts, clapf_id, `from`, `fromdomain`, rcpt, rcptdomain, result, spaminess, `size`, relay, delay, queue_id2, subject, virus) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
$sth_clapf = $dbh->prepare($stmt);


srand;


while (defined($line = $file->read)) {
   if( ($line =~ /\ postfix([\w\W]{0,})\// && ($line =~ /\/smtpd\[/ || $line =~ /\/qmgr\[/ || $line =~ /\/(l|s)mtp\[/ || $line =~ /\/local\[/ || $line =~ /\/virtual\[/ || $line =~ /\/pipe\[/) ) || ($line =~ /\ clapf\[/) ) {
      chomp($line);

      $queue_id = $to = $orig_to = $to_domain = $orig_to_domain = $clapf_id = $relay = $status = $result = $queue_id2 = "";
      $delay = 0;

      $line =~ s/\ {1,}/ /g;

      @l = split(/ /, $line);

      #Sep  3 09:22:20 thorium postfix/smtpd[2270]

      $ts = str2time($l[0] . " " . $l[1] . " " . $l[2]);
      $hostname = $l[3];

      $now = time();

      # ... postfix/smtpd[8371]: NOQUEUE: reject: RCPT from unknown[70.96.35.34]: 554 5.7.1 Service unavailable; Client host [70.96.35.34] blocked using zen.spamhaus.org; http://www.spamhaus.org/query/bl?ip=70.96.35.34; from=<d2221028@ms29.hinet.net> to=<sj@acts.hu> proto=ESMTP helo=<[70.96.35.34]>

      if($line =~ /\/smtpd\[/ && $line =~ /reject: RCPT from ([\w\W]{0,})\: 554 5.7.1 Service unavailable\; Client ([\w\W]{1,}) from=\<([\w\W]{0,})\> to=\<([\w\W]{3,})\> proto=([\w]+) helo=/) {

         $queue_id = "xxxxxxxx" . &randomstring(8);

         $connection{$queue_id}{'now'} = $now;

         $connection{$queue_id}{'client'} = 'x.x.x.x';
         ($connection{$queue_id}{'client'}, undef) = split(/\:/, $1);

         $connection{$queue_id}{'ts'} = $ts;
         $connection{$queue_id}{'from'} = lc $3;

         (undef, $connection{$queue_id}{'from_domain'}) = split(/\@/, $connection{$queue_id}{'from'});
         $connection{$queue_id}{'size'} = 0;

         $smtp{$queue_id}{'ts'} = $ts;
         $smtp{$queue_id}{'status'} = 'rejected ' . $2;
         $smtp{$queue_id}{'to'} = lc $4;
         (undef, $smtp{$queue_id}{'to_domain'}) = split(/\@/, $smtp{$queue_id}{'to'});
      }


      # ... postfix/smtpd[12038]: 0561617020: client=unknown[92.84.77.34]

      if($line =~ /\/smtpd\[/ && $line =~ /\]\:\ ([\w]+)\: client=([\w\W]{3,})/) {
         $queue_id = $1;

         $connection{$queue_id}{'now'} = $now;
         $connection{$queue_id}{'ts'} = $ts;
         $connection{$queue_id}{'client'} = $2;
      }


      # Sep 3 09:30:22 thorium postfix/qmgr[3010]: D20E617022: from=<aaa@freemail.hu>, size=2731, nrcpt=1 (queue active)

      if($line =~ /\/qmgr\[/ && $line =~ /\]\:\ ([\w]+)\: from=\<([\w\W]{0,})\>, size=([\d]+),/) {
         $queue_id = $1;

         $connection{$queue_id}{'size'} = $3;
         $connection{$queue_id}{'from'} = lc $2;

         # fix the null sender, ie. from=<>
         if($connection{$queue_id}{'from'} eq "") { $connection{$queue_id}{'from'} = 'null'; }

         (undef, $connection{$queue_id}{'from_domain'}) = split(/\@/, $connection{$queue_id}{'from'});
         if($connection{$queue_id}{'from_domain'} eq "") { $connection{$queue_id}{'from_domain'} = 'null'; }
      }


      if( ($line =~ /\/(l|s)mtp\[/ || $line =~ /\/local\[/ || $line =~ /\/virtual\[/ || $line =~ /\/pipe\[/ ) && $line =~ /to=/) {
         ($queue_id, undef) = split(/\:/, $l[5]);

         $smtp{$queue_id}{'ts'} = $ts;
         $smtp{$queue_id}{'clapf_id'} = "";

         if($line =~ /\ to=\<([\w\d\.\-\_@\=\+]+)\>,/){
            $to = lc $1;
         }

         next if $to eq "";

         $smtp{$queue_id}{'to'} = $to;

         (undef, $smtp{$queue_id}{'to_domain'}) = split(/\@/, $smtp{$queue_id}{'to'});

         (undef, $x) = split(/orig_to=/, $line);
         ($smtp{$queue_id}{'orig_to'}, undef) = split(/ /, $x);

         $smtp{$queue_id}{'orig_to'} =~ s/\<|\>|\,//g;
         $smtp{$queue_id}{'orig_to'} = lc $smtp{$queue_id}{'orig_to'};

         (undef, $smtp{$queue_id}{'orig_to_domain'}) = split(/\@/, $smtp{$queue_id}{'orig_to'});


         (undef, $smtp{$queue_id}{'relay'}) = split(/relay=/, $line);
         ($smtp{$queue_id}{'relay'}, undef) = split(/,/, $smtp{$queue_id}{'relay'});

         (undef, $x) = split(/delay=/, $line);
         ($smtp{$queue_id}{'delay'}, undef) = split(/ /, $x);

         $smtp{$queue_id}{'delay'} =~ s/\,//;

         $smtp{$queue_id}{'status'} = "";

         if($line =~ /status=([\w\ ]+) \(([\w\W]+)\)/){
            $smtp{$queue_id}{'status'} = "$1 $2";
         }

         if($line =~ / ([a-f0-9]{30,32}) /){
            $smtp{$queue_id}{'clapf_id'} = $1;
         }

      }


      if($line =~ /\ clapf\[/ && $line =~ /status=/) {

         ($clapf_id, undef) = split(/\:/, $l[5]);

         if($line =~ /from=\<([\w\W]{0,})\>, to=\<([\w\W]{3,})\>, spaminess=([\d\.]+), result=([\w\W]{3,}), size=([\d]+), relay=([\w\W]{3,}), delay=([\d\.]+), delays=([\d\.\/]+), status=([\w\W]{0,})\,\ subject=([\w\W]{0,})/) {
            $clapf_id = $clapf_id . " " . $2;


            $clapf{$clapf_id}{'ts'} = $ts;

            $clapf{$clapf_id}{'from'} = "";
            $clapf{$clapf_id}{'fromdomain'} = "";
            $clapf{$clapf_id}{'rcpt'} = "";
            $clapf{$clapf_id}{'rcptdomain'} = "";
            $clapf{$clapf_id}{'spaminess'} = 0.5;
            $clapf{$clapf_id}{'result'} = "";
            $clapf{$clapf_id}{'size'} = 0;
            $clapf{$clapf_id}{'relay'} = "";
            $clapf{$clapf_id}{'delay'} = 0;
            $clapf{$clapf_id}{'status'} = "";
            $clapf{$clapf_id}{'subject'} = "";
            $clapf{$clapf_id}{'queue_id2'} = "";
            $clapf{$clapf_id}{'virus'} = "";


            $clapf{$clapf_id}{'from'} = $1;
            $clapf{$clapf_id}{'rcpt'} = $2;
            $clapf{$clapf_id}{'spaminess'} = $3;
            $clapf{$clapf_id}{'result'} = $4;
            $clapf{$clapf_id}{'size'} = $5;
            $clapf{$clapf_id}{'relay'} = $6;
            $clapf{$clapf_id}{'delay'} = $7;
            $clapf{$clapf_id}{'subject'} = $10;

            $status = $9;


            if($status =~ /250/ && $status =~ /Ok/i) {
               (undef, $clapf{$clapf_id}{'queue_id2'}) = split(/queued\ as\ /, $status);
            }


            (undef, $clapf{$clapf_id}{'fromdomain'}) = split(/\@/, $clapf{$clapf_id}{'from'});
            (undef, $clapf{$clapf_id}{'rcptdomain'}) = split(/\@/, $clapf{$clapf_id}{'rcpt'});

            if($result =~ /VIRUS\ \(([\w\W]+)\)/){
               $clapf{$clapf_id}{'result'} = 'VIRUS';
               $clapf{$clapf_id}{'virus'} = $1;
            }
         }

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

   syslog($priority, "terminated");

   closelog();

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


sub flush_results {
   $now = time();

   # determine if we have to create a new partition
   if($db eq "mysql") {
      $partition_name = strftime("p%Y%m%d", localtime);

      if($partition_name ne $this_partition) {

         $ts = $now + 86400;

         syslog($priority, "creating partitions: $partition_name (less than $ts)");

         $dbh->do("ALTER TABLE clapf ADD PARTITION ( PARTITION $partition_name VALUES LESS THAN ($ts) )");
         $dbh->do("ALTER TABLE `connection` ADD PARTITION ( PARTITION $partition_name VALUES LESS THAN ($ts) )");
         $dbh->do("ALTER TABLE smtp ADD PARTITION ( PARTITION $partition_name VALUES LESS THAN ($ts) )");

         $this_partition = $partition_name;
      }

      $partition_to_drop = strftime("p%Y%m%d", localtime(time() - $days_to_retain_data*86400));

      if($last_dropped_partition ne $partition_to_drop) {
         syslog($priority, "dropping partitions: $partition_to_drop");
      
         $dbh->do("ALTER TABLE clapf DROP PARTITION $partition_to_drop");
         $dbh->do("ALTER TABLE `connection` DROP PARTITION $partition_to_drop");
         $dbh->do("ALTER TABLE smtp DROP PARTITION $partition_to_drop");
   
         $last_dropped_partition = $partition_to_drop;
      }
   }


   foreach $queue_id (keys %connection) {
      if($connection{$queue_id}{'from'} && $connection{$queue_id}{'client'}) {
         $sth_connection->execute($connection{$queue_id}{'ts'}, $queue_id, $connection{$queue_id}{'client'}, $connection{$queue_id}{'from'}, $connection{$queue_id}{'from_domain'}, $connection{$queue_id}{'size'});
         delete $connection{$queue_id};
      }
      else {
         if($now > $connection{$queue_id}{'now'} + 2*$flush_interval) {
            delete $connection{$queue_id};
         }
      }
   }


   foreach $clapf_id (keys %clapf) {
      ($__clapf_id, undef) = split(/ /, $clapf_id);

      if($clapf{$clapf_id}{'result'} && $clapf{$clapf_id}{'from'}) {
         $sth_clapf->execute($clapf{$clapf_id}{'ts'}, $__clapf_id, lc $clapf{$clapf_id}{'from'}, lc $clapf{$clapf_id}{'fromdomain'}, lc $clapf{$clapf_id}{'rcpt'}, lc $clapf{$clapf_id}{'rcptdomain'}, $clapf{$clapf_id}{'result'}, $clapf{$clapf_id}{'spaminess'}, $clapf{$clapf_id}{'size'}, $clapf{$clapf_id}{'relay'}, $clapf{$clapf_id}{'delay'}, $clapf{$clapf_id}{'queue_id2'}, $clapf{$clapf_id}{'subject'}, $clapf{$clapf_id}{'virus'});
      }

      delete $clapf{$clapf_id};
   }


   foreach $queue_id (keys %smtp) {

      #$e = "ts:" . $postfix{$queue_id}{'ts'} . ", id:$queue_id, client:" . $postfix{$queue_id}{'client'} . ", to:" . $postfix{$queue_id}{'to'} . ", orig_to:" . $postfix{$queue_id}{'orig_to'} . ", relay:" . $postfix{$queue_id}{'relay'} . ", delay:" . $postfix{$queue_id}{'delay'} . ", status:" . $postfix{$queue_id}{'status'} . ", clapf_id:" . $postfix{$queue_id}{'clapf_id'};


      if($smtp{$queue_id}{'to'}) {
         $sth_smtp->execute($smtp{$queue_id}{'ts'}, $queue_id, $smtp{$queue_id}{'to'}, $smtp{$queue_id}{'to_domain'}, $smtp{$queue_id}{'orig_to'}, $smtp{$queue_id}{'orig_to_domain'}, $smtp{$queue_id}{'relay'}, $smtp{$queue_id}{'delay'}, $smtp{$queue_id}{'status'}, $smtp{$queue_id}{'clapf_id'});
      }

      delete $smtp{$queue_id};
   }


   alarm $flush_interval;
}


