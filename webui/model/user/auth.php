<?php

class ModelUserAuth extends Model {

   public function checkLogin($username = '', $password = '') {
      $ret = 0;

      $query = $this->db->query("SELECT password, isadmin FROM " . TABLE_USER . " WHERE username='" . $this->db->escape($username) . "'");

      if(isset($query->row['password'])){

         $pass = crypt($password, $query->row['password']);
         if($pass == $query->row['password']){

            $_SESSION['username'] = $username;

            if($query->row['isadmin'] == 1){
               $_SESSION['admin_user'] = 1;
            } else {
               $_SESSION['admin_user'] = 0;
            }

            $ret = 1;
         }
      }

      return $ret;
   }


   public function QQchangePassword($username = '', $password = '') {
      if($username == "" || $password == ""){ return 0; }

      $query = $this->db->query("UPDATE " . TABLE_USER . " SET password='" . $this->db->escape(crypt($password)) . "' WHERE username='" . $this->db->escape($username) . "'");

      return $this->db->countAffected();
   }

}

?>
