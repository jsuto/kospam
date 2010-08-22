<?php

class ModelUserAuth extends Model {

   public function checkLogin($username = '', $password = '') {

      $query = $this->db->ldap_query(LDAP_USER_BASEDN, "(|(mail=$username)(mailalternateaddress=$username))", array("cn", "isadmin", "dn") );

      if(!isset($query->row['cn'])){ return 0; }

      $ldap = new LDAP(LDAP_HOST, $query->row['dn'], $password);

      if($ldap->is_bind_ok() == 0) {
         return 0;
      }

      $_SESSION['username'] = $query->row['cn'];
      $_SESSION['dn'] = $query->row['dn'];
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
