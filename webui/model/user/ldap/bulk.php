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
      $n = 0;
      $entry = array();

      $uids = explode(",", $this->createUidList());

      foreach ($uids as $uid) {

         $dn = $this->model_user_user->getDNByUid((int)$uid);

         $entry["domain"] = $domain;
         $entry["policygroupid"] = (int)$policy_group;

         $entry["filtersender"] = $whitelist;
         $entry["blacklist"] = $blacklist;

         if($dn) {
            if($this->db->ldap_modify($dn, $entry) == TRUE) {

               $query = $this->model_user_user->getUserByUid((int)$uid);

               /* remove from memcached */

               if(MEMCACHED_ENABLED) {
                  $memcache = Registry::get('memcache');

                  $memcache->delete("_c:" . $query['email']);

                  $aliases = $this->model_user_user->trim_to_array($query['aliases']);

                  if(is_array($aliases)) {
                     foreach ($aliases as $email) {
                        $memcache->delete("_c:" . $email);
                     }
                  }
               }

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

         $dn = $this->model_user_user->getDNByUid((int)$uid);

         if($dn) {
            if($this->db->ldap_delete($dn) == TRUE) {
               $n++;
            }
         }
      }

      return $n;
   }


}


?>
