<?php


class SQLite {
   private $link;
   private $affected;

   public function __construct($hostname, $username, $password, $database, $prefix = NULL) {

      try {
         $this->link = new PDO("sqlite:$database", 'OPEN_CREATE');
      }
      catch(PDOException $exception) {
         exit('Error: ' . $exception->getMessage() . " on $database<br />");
      }

      $this->affected = 0;
   }


   public function select_db($database) { }


   public function query($sql) {
      //print "sql: $sql<p>\n";

      $time_start = microtime(true);

      $i = 0;
      $data = array();

      $s = $this->link->prepare($sql) or exit('Error: ' . $sql);
      $s->execute();

      $this->affected = $s->rowCount();

      $R = $s->fetchAll();

      while(list ($k, $v) = each($R)){
         $data[$i] = $v;
         $i++;
      }

      $query = new stdClass();

      $query->row      = isset($data[0]) ? $data[0] : array();
      $query->rows     = $data;
      $query->num_rows = $i;
      $query->query    = $sql;

      unset($data);

      $time_end = microtime(true);

      $query->exec_time = $time_end - $time_start;

      return $query;
   }


   public function escape($value) {
      //return $this->link->quote($value);
      return $value;
   }


   public function countAffected() {
      return $this->affected;
   }


   public function getLastId() {
      return $this->link->lastInsertId();
   }


   public function __destruct() { }

}


?>
