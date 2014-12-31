<?php

class ModelStatOnline extends Model {

   public function online($username = '') {
      if($username == '') { return 0; }

      $query = $this->db->query("INSERT INTO " . TABLE_ONLINE . " (username, ts, last_activity, ipaddr) VALUES(?,?,?,?)", array($username, NOW, NOW, $_SERVER['REMOTE_ADDR']));

      if($this->db->countAffected() == 0) {
         $query = $this->db->query("UPDATE " . TABLE_ONLINE . " SET ts=?, last_activity=? WHERE username=? AND ipaddr=?", array(NOW, NOW, $username, $_SERVER['REMOTE_ADDR']));
      }

      return 1;
   }


   public function offline($username = '') {
      if($username == '') { return 0; }

      $query = $this->db->query("DELETE FROM " . TABLE_ONLINE . " WHERE username=? AND ipaddr=?", array($username, $_SERVER['REMOTE_ADDR']));

      return 1;
   }


   public function count_online() {
      $query = $this->db->query("DELETE FROM " . TABLE_ONLINE . " WHERE last_activity < ?", array(NOW - 3600));

      $query = $this->db->query("SELECT COUNT(*) AS num FROM " . TABLE_ONLINE);

      return $query->row['num'];
   }


   public function get_online_users() {
      $query = $this->db->query("DELETE FROM " . TABLE_ONLINE . " WHERE last_activity < ?", array(NOW - 3600));

      $query = $this->db->query("SELECT * FROM " . TABLE_ONLINE . " ORDER BY username ASC");

      return $query->rows;
   }



}


?>
