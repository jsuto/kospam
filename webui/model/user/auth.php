<?php

class ModelUserAuth extends Model {

   public function checkLogin($username = '', $password = '') {

      $query = $this->db->query("SELECT " . TABLE_USER . ".username, " . TABLE_USER . ".dn, " . TABLE_USER . ".password, " . TABLE_USER . ".isadmin, " . TABLE_USER . ".domain FROM " . TABLE_USER . ", " . TABLE_EMAIL . " WHERE " . TABLE_EMAIL . ".email='" . $this->db->escape($username) . "' AND " . TABLE_EMAIL . ".uid=" . TABLE_USER . ".uid");

      if(!isset($query->row['password'])) { return 0; }

      $pass = crypt($password, $query->row['password']);

      if($pass == $query->row['password']){
         $_SESSION['username'] = $query->row['username'];
         $_SESSION['admin_user'] = $query->row['isadmin'];
         return 1;
      }

      if(strlen($query->row['dn']) > 3) { return $this->checkLoginAgainstLDAP($query->row, $password); }

      return 0;
   }


   private function checkLoginAgainstLDAP($user = array(), $password = '') {
      if($password == '' || !isset($user['username']) || !isset($user['domain']) || !isset($user['dn']) || strlen($user['domain']) < 2){ return 0; }

      $query = $this->db->query("SELECT remotehost, basedn FROM " . TABLE_REMOTE . " WHERE remotedomain='" . $this->db->escape($user['domain']) . "'");
      if($query->num_rows != 1) { return 0; }

      $ldap = new LDAP($query->row['remotehost'], $user['dn'], $password);

      if($ldap->is_bind_ok()) {
         $_SESSION['username'] = $user['username'];
         $_SESSION['admin_user'] = 0;

         $this->changePassword($user['username'], $password);

         return 1;
      }

      return 0; 
   }


   public function changePassword($username = '', $password = '') {
      if($username == "" || $password == ""){ return 0; }

      $query = $this->db->query("UPDATE " . TABLE_USER . " SET password='" . $this->db->escape(crypt($password)) . "' WHERE username='" . $this->db->escape($username) . "'");

      return $this->db->countAffected();
   }

}

?>
