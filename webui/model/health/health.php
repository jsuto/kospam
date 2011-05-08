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


   public function get_last_maillog_entries($n = 10) {
      $query = $this->db_history->query("select * from clapf order by ts desc limit 0," . (int)$n);

      if(isset($query->rows)) { return $query->rows; }

      return array();
   }

}


?>
