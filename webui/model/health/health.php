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


   public function checksmtp($host = '', $port = 0, $error = '') {
      $ret = $error;
      $time = 0;

      if($host && $port && is_numeric($port) &&  $port > 0 && $port < 65536) {
         $time_start = microtime(true);

         $s = @fsockopen($host, $port);

         if($s) {
            $ret = trim(fgets($s, 4096));
            fputs($s, "QUIT\r\n");
            fclose($s);
         }
      }

      $time = microtime(true) - $time_start;
      return array("$host:$port", $ret, $this->format_time($time));
   }


   public function check_postgrey($host = '', $port = 0, $error = '') {
      $ret = $error;
      $time = 0;

      if($host && $port && is_numeric($port) &&  $port > 0 && $port < 65536) {
         $time_start = microtime(true);

         $s = @fsockopen($host, $port);

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
      return array("$host:$port", $ret, $this->format_time($time));
   }


   public function get_last_maillog_entries($n = 10) {
      $query = $this->db_history->query("select * from " . TABLE_SUMMARY . " order by ts desc limit 0," . (int)$n);

      if(isset($query->rows)) { return $query->rows; }

      return array();
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
      return $s;
   }


   public function meminfo() {
      $s = exec("free", $output);
      return implode("\n", $output);
   }


   public function diskinfo() {
      $s = exec("df -h", $output);
      return implode("\n", $output);
   }


   public function get_number_of_quarantined_messages() {
       return file_get_contents(NUMBER_OF_QUARANTINED_MESSAGES);
   }

}


?>
