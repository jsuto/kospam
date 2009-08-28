<?php

class ModelUserAuth extends Model {

   public function checkLogin($username = '', $password = '') {

      $ldap = new LDAP(LDAP_HOST, "cn=$username," . LDAP_USER_BASEDN, $password);

      if($ldap->is_bind_ok() == 0) {

         /* try to auth at remote AD/LDAP server */

         $query = $this->db->ldap_query(LDAP_USER_BASEDN, "cn=$username", array("domain") );

         if(isset($query->row['domain']) && strlen($query->row['domain']) > 2) {

            $remote_user = preg_replace("/\@" . $query->row['domain'] . "/", "", $username);

            /* query remote AD/LDAP server parameters */

            $query = $this->db->ldap_query(LDAP_REMOTE_BASEDN, "remotedomain=" . $query->row['domain'], array("remotehost", "basedn") );

            if(isset($query)) {
               $remote_ldap = new LDAP($query->row['remotehost'], "cn=$remote_user," . $query->row['basedn'], $password);
               if($remote_ldap->is_bind_ok()) {
                  $_SESSION['username'] = $username;
                  $_SESSION['admin_user'] = 0;

                  /* update the password field at the local LDAP server */

                  $this->changePassword($username, $password);

                  return 1;
               }
            }

         }
         
         return 0;
      }

      $query = $ldap->query(LDAP_USER_BASEDN, "cn=$username", array("isadmin") );

      $_SESSION['username'] = $username;
      $_SESSION['admin_user'] = (int)$query->row['isadmin'];

      return 1;
   }


   public function changePassword($username = '', $password = '') {
      if($username == "" || $password == ""){ return 0; }

      $entry = array();

      $entry["cn"] = $username;
      $entry["userPassword"] = "{MD5}" . base64_encode(md5($_POST['password'], TRUE));

      if($this->db->ldap_modify("cn=$username," . LDAP_USER_BASEDN, $entry) == TRUE) {
         return 1;
      }

      return 0;
   }

}

?>
