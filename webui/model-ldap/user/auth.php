<?php

class ModelUserAuth extends Model {

   public function checkLogin($username = '', $password = '') {

      $ldap = new LDAP(LDAP_HOST, "cn=$username," . LDAP_USER_BASEDN, $password);

      if($ldap->is_bind_ok() == 0) { return 0; }

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
