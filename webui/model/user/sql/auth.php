<?php

class ModelUserAuth extends Model {

   public function checkLogin($username = '', $password = '') {

      $query = $this->db->query("SELECT " . TABLE_USER . ".username, " . TABLE_USER . ".password, " . TABLE_USER . ".isadmin, " . TABLE_USER . ".domain FROM " . TABLE_USER . ", " . TABLE_EMAIL . " WHERE " . TABLE_EMAIL . ".email='" . $this->db->escape($username) . "' AND " . TABLE_EMAIL . ".uid=" . TABLE_USER . ".uid");

      if(!isset($query->row['password'])) { return 0; }

      $pass = crypt($password, $query->row['password']);

      if($pass != $query->row['password']){

         // try to auth at remote AD/LDAP server

         if(isset($query->row['domain']) && strlen($query->row['domain']) > 2) {

            $remote_user = preg_replace("/\@" . $query->row['domain'] . "/", "", $query->row['username']);

            // query remote AD/LDAP server parameters

            $query = $this->db->query("SELECT remotehost, basedn FROM " . TABLE_REMOTE . " WHERE remotedomain='" . $this->db->escape($query->row['domain']) . "'");

            if(isset($query->row['remotehost']) && isset($query->row['remotehost']) ) {
               $remote_ldap = new LDAP($query->row['remotehost'], "cn=$remote_user," . $query->row['basedn'], $password);
               if($remote_ldap->is_bind_ok()) {
                  $_SESSION['username'] = $query->row['username'];
                  $_SESSION['admin_user'] = 0;

                  // update the password field at the local LDAP server

                  $this->changePassword($query->row['username'], $password);

                  return 1;
               }
            }

         }

         return 0;
      }


      $_SESSION['username'] = $query->row['username'];
      $_SESSION['admin_user'] = $query->row['isadmin'];

      return 1;
   }


   public function changePassword($username = '', $password = '') {
      if($username == "" || $password == ""){ return 0; }

      $query = $this->db->query("UPDATE " . TABLE_USER . " SET password='" . $this->db->escape(crypt($password)) . "' WHERE username='" . $this->db->escape($username) . "'");

      return $this->db->countAffected();
   }

}

?>
