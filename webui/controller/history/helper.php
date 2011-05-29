<?php


class ControllerHistoryHelper extends Controller {
   private $error = array();

   public function index(){

      $db = Registry::get('db_history');

      $query = $db->query("select ts from `connection` order by ts desc limit 1");

      if(isset($query->row['ts']) && $query->row['ts'] > 0) {
         print $query->row['ts'];
      }
      else {
         print "0";
      }
   }

}

?>
