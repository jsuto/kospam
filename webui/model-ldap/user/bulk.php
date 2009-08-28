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


   private function getNameByUid($uid = 0) {
      if(!is_numeric($uid) || $uid < 1) { return ""; }

      $query = $this->db->ldap_query(LDAP_USER_BASEDN, "uid=" . (int)$uid, array("cn") );

      if(isset($query->row['cn'])){
         return $query->row['cn'];
      }

      return "";
   }


   public function bulkUpdateUser($policy_group = 0, $whitelist = '', $blacklist = '') {
      $n = 0;
      $entry = array();

      $uids = explode(",", $this->createUidList());

      foreach ($uids as $uid) {

         $username = $this->getNameByUid((int)$uid);

         $entry["cn"] = $username;
         $entry["policygroupid"] = (int)$policy_group;

         $entry["filtersender"] = $whitelist;
         $entry["blacklist"] = $blacklist;

         if($username) {
            if($this->db->ldap_modify("cn=$username," . LDAP_USER_BASEDN, $entry) == TRUE) {
               $n++;
            }
         }
      }

      return $n;
   }


   public function bulkDeleteUser() {
      $n = 0;

      $uids = explode(",", $this->createUidList());

      foreach ($uids as $uid) {

         $username = $this->getNameByUid((int)$uid);

         if($username) {
            if($this->db->ldap_delete("cn=$username," . LDAP_USER_BASEDN) == TRUE) {
               $n++;
            }
         }
      }

      return $n;
   }


}


?>
