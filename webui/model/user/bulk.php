<?php

class ModelUserBulk extends Model {

   public function createUidList(){
      $uidlist = "";

      reset($_POST);

      while(list($k, $v) = each($_POST)){
         if(preg_match("/^aa_/", $k)){
            $a = explode("_", $k);
            if(is_numeric($a[1]) && $a[1] > 0){
               $uidlist .= "$a[1],";
            }
         }
      }

      $uidlist = preg_replace("/\,$/", "", $uidlist);

      return $uidlist;
   }


   public function bulkUpdateUser($domain = '', $policy_group = 0, $whitelist = '', $blacklist = '') {
      $uidlist = $this->createUidList();

      if($uidlist == ""){ return 0; }

      LOGGER("bulk update uids: $uidlist");

      $query = $this->db->query("UPDATE " . TABLE_USER . " SET domain='" . $this->db->escape($domain) . "', policy_group=" . (int)$policy_group . " WHERE uid IN ($uidlist)");
      $query = $this->db->query("UPDATE " . TABLE_WHITELIST . " SET whitelist='" . $this->db->escape($whitelist) . "' WHERE uid IN ($uidlist)");
      $query = $this->db->query("UPDATE " . TABLE_BLACKLIST . " SET blacklist='" . $this->db->escape($blacklist) . "' WHERE uid IN ($uidlist)");

         /* remove from memcached */

         if(MEMCACHED_ENABLED) {
            $memcache = Registry::get('memcache');

            $query = $this->db->query("SELECT email FROM " . TABLE_EMAIL . " WHERE uid IN ($uidlist)");
            foreach($query->rows as $q) {
               $memcache->delete("_c:" . $this->db->escape($q['email']));
            }
         }

      return 1;
   }


   public function bulkDeleteUser() {
      $uidlist = $this->createUidList();

      if($uidlist == ""){ return 0; }

      LOGGER("bulk remove uids: $uidlist");

      $query = $this->db->query("DELETE FROM " . TABLE_EMAIL . " WHERE uid IN ($uidlist)");
      $query = $this->db->query("DELETE FROM " . TABLE_USER . " WHERE uid IN ($uidlist)");
      $query = $this->db->query("DELETE FROM " . TABLE_WHITELIST . " WHERE uid IN ($uidlist)");
      $query = $this->db->query("DELETE FROM " . TABLE_BLACKLIST . " WHERE uid IN ($uidlist)");

      return 1;
   }


}


?>
