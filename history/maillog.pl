#!/usr/bin/perl

use File::Tail;
use Date::Parse;

use DBI;

die "usage: $0 <logfile> <sqlite3 db file>\n" unless ( ($maillog = $ARGV[0]) && ($dbfile = $ARGV[1]) );


$dsn = "dbi:SQLite:dbname=$dbfile";

$file = File::Tail->new(name => $maillog, maxinterval=> 5) or die "cannot read $maillog";


$dbh = DBI->connect($dsn, "", "") or die "cannot open database: $dbfile";

$stmt = "INSERT INTO smtpd (ts, queue_id, client_ip) VALUES(?, ?, ?)";
$sth_smtpd = $dbh->prepare($stmt);

$stmt = "INSERT INTO cleanup (ts, queue_id, message_id) VALUES(?, ?, ?)";
$sth_cleanup = $dbh->prepare($stmt);

$stmt = "INSERT INTO qmgr (ts, queue_id, `from`, size) VALUES(?, ?, ?, ?)";
$sth_qmgr = $dbh->prepare($stmt);

$stmt = "INSERT INTO smtp (ts, queue_id, `to`, orig_to, relay, delay, result) VALUES(?, ?, ?, ?, ?, ?, ?)";
$sth_smtp = $dbh->prepare($stmt);

$stmt = "INSERT INTO clapf (ts, queue_id, result, spaminess, relay, delay, queue_id2, virus) VALUES(?, ?, ?, ?, ?, ?, ?, ?)";
$sth_clapf = $dbh->prepare($stmt);



while (defined($line = $file->read)) {

   if($line =~ /\ postfix\/smtpd\[/ || $line =~ /\ postfix\/cleanup\[/ || $line =~ /\ postfix\/qmgr\[/ || $line =~ /\ postfix\/(l|s)mtp\[/ || $line =~ /\ postfix\/local\[/ || $line =~ /\ clapf\[/ || $line =~ /\ postfix\/virtual\[/ || $line =~ /\ postfix\/pipe\[/) {
      chomp($line);

      $queue_id = $to = $orig_to = $relay = $status = $result = $queue_id2 = "";
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

         $sth_smtpd->execute($ts, $queue_id, $client_ip);
      }


      #Sep  3 09:30:22 thorium postfix/cleanup[2311]: D20E617022: message-id=<f14f01c89af3$8a784f50$3bc40658@acts.hu>

      if($line =~ /\ postfix\/cleanup\[/ && $line =~ /message\-id=/) {
         $message_id = "";

         ($queue_id, undef) = split(/\:/, $l[5]);
         (undef, $message_id) = split(/=/, $l[6]);

         $message_id =~ s/\<|\>|\,//g;

         $sth_cleanup->execute($ts, $queue_id, $message_id);
         ##print "cleanup: $ts $hostname $queue_id $message_id\n";
      }


      # Sep 3 09:30:22 thorium postfix/qmgr[3010]: D20E617022: from=<aaa@freemail.hu>, size=2731, nrcpt=1 (queue active)

      if($line =~ /\ postfix\/qmgr\[/ && $line =~ /from=/) {
         $from = ""; $size = 0;

         ($queue_id, undef) = split(/\:/, $l[5]);

         if($line =~ /from=\<([\w\W]+)\>/){
            $from = $1;
         }

         (undef, $size) = split(/=/, $l[7]);

         $size =~ s/\,//;

         ##print "qmgr: $ts $hostname $queue_id from: $from ** size: $size\n";
         $sth_qmgr->execute($ts, $queue_id, $from, $size);
      }

      # Sep 3 09:30:23 thorium postfix/smtp[2312]: D20E617022: to=<sj@acts.hu>, relay=127.0.0.1[127.0.0.1]:10025, delay=0.16, delays=0.01/0/0/0.14, dsn=2.0.0, status=sent (250 Ok 4a9f708ea516d9a0814a203f2e1662 <sj@acts.hu>)
      # Sep 3 10:19:34 thorium postfix/smtp[2119]: 55DDB14C32E: to=<sj@acts.hu>, relay=thorium.datanet.hu[194.149.0.116]:25, delay=0.1, delays=0.05/0.01/0.04/0.02, dsn=2.0.0, status=sent (250 2.0.0 Ok: queued as 6AE4517022)

      if($line =~ /\ postfix\/(l|s)mtp\[/ && $line =~ /to=/) {
         ($queue_id, undef) = split(/\:/, $l[5]);
         (undef, $to) = split(/=/, $l[6]);
         $to =~ s/\<|\>|\,//g;

         (undef, $relay) = split(/relay=/, $line);
         ($relay, undef) = split(/,/, $relay);

         (undef, $delay) = split(/delay=/, $line);
         ($delay, undef) = split(/,/, $delay);

         $status = "";

         if($line =~ /status=([\w\ ]+) \(([\w\W]+)\)/){
            $status = "$1 $2";
         }

         $sth_smtp->execute($ts, $queue_id, $to, $orig_to, $relay, $delay, $status);
      }


      # Sep  3 10:06:46 thorium postfix/local[2664]: 0536C6450: to=<sj@thorium.datanet.hu>, orig_to=<sj@acts.hu>, relay=local, delay=0.08, delays=0.04/0.01/0/0.03, dsn=2.0.0, status=sent (delivered to command: /usr/local/bin/maildrop)
      # Sep  3 11:17:35 cesium postfix/virtual[4276]: 2A96F14C6EE: to=<sj@acts.hu>, relay=virtual, delay=0.08, delays=0.06/0.02/0/0.01, dsn=2.0.0, status=sent (delivered to maildir)

      if($line =~ /\ postfix\/local\[/ || $line =~ /\ postfix\/virtual\[/ || $line =~ /\ postfix\/pipe\[/) {
         ($queue_id, undef) = split(/\:/, $l[5]);
         (undef, $to) = split(/=/, $l[6]);

         $to =~ s/\<|\>|\,//g;

         (undef, $x) = split(/orig_to=/, $line);
         ($orig_to, undef) = split(/ /, $x);

         $orig_to =~ s/\<|\>|\,//g;

         (undef, $relay) = split(/relay=/, $line);
         ($relay, undef) = split(/,/, $relay);

         (undef, $x) = split(/delay=/, $line);
         ($delay, undef) = split(/ /, $x);

         $delay =~ s/\,//;

         $status = "";

         if($line =~ /status=([\w\ ]+) \(([\w\W]+)\)/){
            $status = "$1 $2";
         }

         $sth_smtp->execute($ts, $queue_id, $to, $orig_to, $relay, $delay, $status);
      }


      # Sep  3 10:00:07 thorium clapf[2578]: 4a9f7787c9b56ae1a646fd8ca6cc5b: sj@acts.hu got SPAM, 1.0000, 2731, relay=127.0.0.1:10026, delay=0.06, delays=0.00/0.00/0.00/0.00/0.00/0.00/0.01/0.00/0.04, status=250 2.0.0 Ok: queued as E691917023

      if($line =~ /\ clapf\[/ && $line =~ /status=/) {
         $spaminess = 0.5;
         $virus = "";

         ($queue_id, undef) = split(/\:/, $l[5]);

         (undef, $relay) = split(/relay=/, $line);
         ($relay, undef) = split(/,/, $relay);

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

         ##print "clapf: $ts $hostname $queue_id delay: $delay, result: $result, q2: $queue_id2\n";
         $sth_clapf->execute($ts, $queue_id, $result, $spaminess, $relay, $delay, $queue_id2, $virus);

      }


      
   }

}

