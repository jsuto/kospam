<?php

class ModelUserWbl extends Model {

   public function list($type = 'white') {
      $session = Registry::get('session');

      $uid = $session->get("uid");

      $query = $this->db->query("SELECT w.whitelist AS white, b.blacklist AS black FROM " . TABLE_WHITELIST . " w, " . TABLE_BLACKLIST . " b WHERE w.uid=b.uid AND w.uid=?", array($uid));

      if($this->request->get['type'] == $type and isset($query->row[$type])) { return $query->row[$type]; }

      return "";
   }


   public function update($list = '', $type = 'white') {
      $session = Registry::get('session');

      $uid = $session->get("uid");

      if($type == 'white') {
         $table = TABLE_WHITELIST;
         $column = 'whitelist';
      }
      else {
         $table = TABLE_BLACKLIST;
         $column = 'blacklist';
      }

      $query = $this->db->query("UPDATE $table SET $column=? WHERE uid=?", array($list, $uid));

      return 1;
   }


}

?>
