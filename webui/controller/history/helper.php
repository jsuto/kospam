<?php


class ControllerHistoryHelper extends Controller {
   private $error = array();

   public function index(){


      require_once(DIR_DATABASE . "/sqlite.php");
      $db = new DB("sqlite", DB_HOSTNAME, DB_USERNAME, DB_PASSWORD, HISTORY_DATABASE, DB_PREFIX);

      $query = $db->query("select ts from cleanup order by ts desc limit 1");

      if(isset($query->row['ts']) && $query->row['ts'] > 0) {
         print $query->row['ts'];
      }
      else {
         print "0";
      }
   }

}

?>
