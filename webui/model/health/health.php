<?php

class ModelHealthHealth extends Model {

   public function format_time($time = 0) {
      if($time >= 1) {
         return $time . " sec";
      }
      else {
         return sprintf("%.2f", 1000*$time) . " ms";
      }
   }


   public function checksmtp($smtp = array(), $error = '') {

      $ret = $error;
      $time = 0;

      if($smtp[0] && $smtp[1] && is_numeric($smtp[1]) && $smtp[1] > 0 && $smtp[1] < 65536) {
         $time_start = microtime(true);

         $s = @fsockopen($smtp[0], $smtp[1]);

         if($s) {
            $ret = trim(fgets($s, 4096));
            fputs($s, "QUIT\r\n");
            fclose($s);
         }
      }

      $time = microtime(true) - $time_start;
      return array($smtp[0] . ":" . $smtp[1], $ret, $this->format_time($time), $smtp[2]);
   }


   public function check_postgrey($policy = array(), $error = '') {
      $ret = $error;
      $time = 0;

      if($policy[0] && $policy[1] && is_numeric($policy[1]) &&  $policy[1] > 0 && $policy[1] < 65536) {
         $time_start = microtime(true);

         $s = @fsockopen($policy[0], $policy[1]);

         if($s) {
            $req  = "request=smtpd_access_policy" . EOL;
            $req .= "protocol_state=RCPT" . EOL;
            $req .= "protocol_name=SMTP" . EOL;
            $req .= "client_address=192.168.100.100" . EOL;
            $req .= "client_name=spamgw.yourdomain.com" . EOL;
            $req .= "reverse_client_name=spamgw.yourdomain.com" . EOL;
            $req .= "helo_name=spamgw.yourdomain.com" . EOL;
            $req .= "sender=testaccount@spamgw.yourdomain.com" . EOL;
            $req .= "recipient=testaccount@spamgw.yourdomain.com" . EOL;
            $req .= "recipient_count=0" . EOL;
            $req .= "instance=1eb5.4dcd83ab.2ccae.4" . EOL;
            $req .= EOL;

            fputs($s, $req);

            $ret = trim(fgets($s, 4096));
            fclose($s);
         }
      }

      $time = microtime(true) - $time_start;
      return array($policy[0] . ":" . $policy[1], $ret, $this->format_time($time), $policy[2]);
   }


   public function count_processed_emails() {
      $today = $last_7_days = $last_30_days = 0;
      $now = time();

      $ts = $now - 86400;
      $query = $this->db_history->query("select count(*) as count from clapf where ts > $ts");
      if(isset($query->row['count'])) { $today = $query->row['count']; }

      $ts = $now - 604800;
      $query = $this->db_history->query("select count(*) as count from clapf where ts > $ts");
      if(isset($query->row['count'])) { $last_7_days = $query->row['count']; }

      $ts = $now - 2592000;
      $query = $this->db_history->query("select count(*) as count from clapf where ts > $ts");
      if(isset($query->row['count'])) { $last_30_days = $query->row['count']; }

      return array($today, $last_7_days, $last_30_days);
   }


   public function uptime() {
      $s = exec("uptime");
      list ($uptime, $loadavg) = preg_split("/ load average\: /", $s);

      return array(preg_replace("/\,\ {0,}$/", "", $uptime), $loadavg);
   }


   public function meminfo() {
      $m = explode("\n", file_get_contents("/proc/meminfo"));

      while(list($k, $v) = each($m)) {
         $a = preg_split("/\ {1,}/", $v);
         if(isset($a[0]) && $a[0]) { $_m[$a[0]] = $a[1]; }
      }

      $mem_percentage = sprintf("%.2f", 100*($_m['MemTotal:'] - $_m['MemFree:'] - $_m['Cached:']) / $_m['MemTotal:']);
      $swap_percentage = sprintf("%.2f", 100*($_m['SwapTotal:'] - $_m['SwapFree:']) / $_m['SwapTotal:']);

      return array(sprintf("%.0f", $_m['MemTotal:'] / 1000), $mem_percentage, sprintf("%.0f", $_m['SwapTotal:'] / 1000), $swap_percentage);
   }


   public function diskinfo() {
      $shortinfo = array();

      $s = exec("df -h", $output);

      $partitions = Registry::get('partitions_to_monitor');

      while(list($k, $v) = each($output)) {
         if($k > 0) {
            $p = preg_split("/\ {1,}/", $v);
            if(isset($p[5]) && in_array($p[5], $partitions)) {
               $shortinfo[] = array(
                                    'partition' => $p[5],
                                    'utilization' => preg_replace("/\%/", "", $p[4])
                              );
            }
         }
      }

      return $shortinfo;
   }


   public function countSpamMessages() {
      $query = $this->db->query("SELECT COUNT(*) AS num FROM ". TABLE_QUARANTINE . " WHERE is_spam='s'");

      if(isset($query->row['num'])) { return $query->row['num']; }

      return 0;
   }

}


?>
