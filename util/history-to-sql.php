<?php

class MySQL {
   private $link;
   private $affected;

   public function __construct($hostname, $username, $password, $database, $prefix = NULL) {

      try {
         $this->link = new PDO("mysql:host=$hostname;dbname=$database", $username, $password,
                                  array(
                                     PDO::MYSQL_ATTR_INIT_COMMAND => "SET NAMES utf8",
                                     PDO::MYSQL_ATTR_INIT_COMMAND => "SET CHARACTER SET utf8"
                                  )
                              );

      }
      catch(PDOException $exception) {
         exit('Error: ' . $exception->getMessage() . " on database: $database<br />");
      }

      $this->affected = 0;
   }


   public function query($sql, $arr = array()) {
      $query = new stdClass();

      $query->error = 1;
      $query->errmsg = "Error";
      $query->query = $sql;

      $time_start = microtime(true);

      $i = 0;
      $data = array();

      $s = $this->link->prepare($sql);
      if(!$s) { return $query; }

      try {
         $s->execute($arr);
      }
      catch(PDOException $exception) { }

      $this->affected = $s->rowCount();

      $R = $s->fetchAll();

      while(list ($k, $v) = each($R)){
         $data[$i] = $v;
         $i++;
      }

      $query->row      = isset($data[0]) ? $data[0] : array();
      $query->rows     = $data;
      $query->num_rows = $i;

      $query->error = 0;
      $query->errmsg = "";

      unset($data);

      $time_end = microtime(true);

      $query->exec_time = $time_end - $time_start;

      return $query;
   }


   public function countAffected() {
      return $this->affected;
   }


   public function getLastId() {
      return $this->link->lastInsertId();
   }


   public function __destruct() { }

}


$history_dir = "/var/clapf/history/new";
$dry_run = 0;
$verbose = 0;
$delimiter = '';

$opts = 'dhv';
$lopts = array(
               'dir:',
               'dry-run',
               'verbose'
         );
    
if($options = getopt($opts, $lopts)) {

    if(isset($options['dir'])) { 
        $history_dir = $options['dir'];
    }

    if(isset($options['dry-run']) || isset($options['d'])) {
        $dry_run = 1;
    }

    if(isset($options['h'])) { 
        display_help();
        exit;
    }

    if(isset($options['verbose']) || isset($options['v'])) {
        $verbose = 1;
    }

}

openlog("history-to-sql.php", LOG_PID, LOG_MAIL);

$n = processdir($history_dir);

syslog(LOG_INFO, "processed $n messages in '$history_dir'");


function processdir($dir = '') {
   global $delimiter;
   global $dry_run;
   global $verbose;
   $n = 0;
   $stat = array(
                 'rcvd'  => 0,
                 'ham'   => 0,
                 'spam'  => 0,
                 'virus' => 0,
                 'size'  => 0
                );


   if($dir == '') { return $n; }

   $db = new MySQL("", "clapf", "changeme", "clapf");

   $dh = opendir($dir);
   if($dh) {
      while(($f = readdir($dh))) {
         if(substr($f, 0, 1) == '.') { continue; }

         $filename = "$dir/$f";

         $fp = fopen($filename, "r");
         if($fp) {
            $s = fread($fp, 8192);
            fclose($fp);

            $s = zlib_decode($s);

            $clapf_id = $ts = $sender = $rcpt = $subject = $size = $attachments = $spam = $relay = $status = '';

           $a = explode($delimiter, $s);
           if(count($a) == 10) {
               list($clapf_id, $ts, $sender, $rcpt, $size, $attachments, $spam, $relay, $status, $subject) = $a;
            }

            if(count($a) == 10) {
               $stat['rcvd']++;
               $stat['size'] += $size;
               if($spam == 0) { $stat['ham']++; }
               if($spam == 1) { $stat['spam']++; }
               if($spam == 2) { $stat['virus']++; }
            }

            $db->query("INSERT INTO history (clapf_id, ts, sender, rcpt, size, attachments, spam, relay, status, subject) VALUES(?,?,?,?,?,?,?,?,?,?)", array($clapf_id, $ts, $sender, $rcpt, $size, $attachments, $spam, $relay, $status, $subject));

            if($db->countAffected() == 1) {
               $n++;
               unlink($filename);
            }

         }         
      }

      closedir($dh);
   }
   else {
      syslog(LOG_INFO, "cannot open: '$dir'");
   }

   if($stat['rcvd'] > 0) {
      $db->query("UPDATE counter SET rcvd=rcvd+?, size=size+?, spam=spam+?, ham=ham+?, virus=virus+?", array($stat['rcvd'], $stat['size'], $stat['spam'], $stat['ham'], $stat['virus']));
   }

   return $n;
}


function display_help() {
   print "--dir|-d               history directory\n";
   print "\n\n";
}


?>
