<?php


class ModelUserUser extends Model {

   public function getUsers($search = '', $page = 0, $page_len = 0) {
      $data = $emails = array();
      $n_users = 0;
      $from = $page * $page_len;
      $to = ($page+1) * $page_len;

      if($search) {
         $query = $this->db->ldap_query(LDAP_USER_BASEDN, "(|(cn=*$search*)(mail=*$search*))", array("uid", "mail", "cn", "policygroupid", "domain") );
      }
      else {
         $query = $this->db->ldap_query(LDAP_USER_BASEDN, "(|(cn=*)(mail=*))", array("uid", "mail", "cn", "policygroupid", "domain") );
      }

      foreach ($query->rows as $result) {

         if($n_users >= $from && $n_users < $to) {

            $data[] = array(
                          'uid'          => $result['uid'],
                          'username'     => $result['cn'],
                          'email'        => $result['mail'],
                          'policy_group' => $result['policygroupid'],
                          'domain'       => isset($result['domain']) ? $result['domain'] : ""
                          );


         }

         $n_users++;
      }


      return $data;
   }


   public function getUserByUid($uid = 0) {
      if(!is_numeric($uid) || (int)$uid < 0){
         return array();
      }

      $query = $this->db->ldap_query(LDAP_USER_BASEDN, "uid=$uid", array() );

      $result = $query->row;

      $aliases = "";

      if(is_array($result['mailalternateaddress']) ) {
         for($i = 0; $i < $result['mailalternateaddress']['count']; $i++) {
            $aliases .= $result['mailalternateaddress'][$i] . "\n";
         }

         $aliases = rtrim($aliases);
      }

      $data = array(
                    'uid'          => $result['uid'],
                    'username'     => $result['cn'],
                    'email'        => $result['mail'],
                    'policy_group' => $result['policygroupid'],
                    'isadmin'      => $result['isadmin'],
                    'aliases'      => $aliases,
                    'whitelist'    => $result['filtersender'],
                    'blacklist'    => $result['blacklist'],
                    'domain'       => isset($result['domain']) ? $result['domain'] : ""
                   );


      return $data;
   }


   public function howManyUsers($search = '') {

      $query = $this->db->ldap_query(LDAP_USER_BASEDN, "(|(cn=$search*)(mail=$search*))", array("uid") );

      return $query->num_rows;
   }


   public function getUidByName($username = '') {
      if($username == ""){ return -1; }

      $query = $this->db->ldap_query(LDAP_USER_BASEDN, "cn=$username", array("uid") );

      if(isset($query->row['uid'])){
         return $query->row['uid'];
      }

      return -1;

   }


   public function getNameByUid($uid = 0) {
      if(!is_numeric($uid) || $uid < 1) { return ""; }

      $query = $this->db->ldap_query(LDAP_USER_BASEDN, "uid=" . (int)$uid, array("cn") );

      if(isset($query->row['cn'])){
         return $query->row['cn'];
      }

      return "";
   }


   public function getNextUid() {
      $x = 0;

      $query = $this->db->ldap_query(LDAP_USER_BASEDN, "uid=*", array("uid") );

      foreach ($query->rows as $result) {
         if($result['uid'] > $x){
            $x = $result['uid'];
         }
      }

      return $x + 1;
   }


   public function getEmailAddress($username = '') {
      if($username == ""){ return ""; }

      $query = $this->db->ldap_query(LDAP_USER_BASEDN, "cn=$username", array("mail") );

      if(isset($query->row['mail'])){
         return $query->row['mail'];
      }

      return "";
   }


   public function getEmailDomains() {
      $data = array();

      if(Registry::get('domain_admin') == 1) {
         $my_domain = $this->getDomains();
         //$query = $this->db->ldap_query("SELECT domain FROM " . TABLE_DOMAIN . " WHERE mapped='" . $this->db->escape($my_domain[0]) . "'");
      }
      else {
         $query = $this->db->ldap_query(LDAP_DOMAIN_BASEDN, "maildomain=*", array("maildomain") );
      }

      foreach ($query->rows as $q) {
         array_push($data, $q['maildomain']);
      }

      return $data;
   }


   public function getDomains(){
      $data = array();

      if(Registry::get('domain_admin') == 1) {
         //$query = $this->db->query("SELECT domain FROM " . TABLE_USER . " WHERE username='" . $this->db->escape($_SESSION['username']) . "'");
      }
      else {
         $query = $this->db->ldap_query(LDAP_DOMAIN_BASEDN, "maildomain=*", array("maildomain") );
      }

      foreach ($query->rows as $q) {
         array_push($data, $q['maildomain']);
      }

      return $data;
   }


   public function getEmails($username = '') {
   }


   public function addUser($user) {
      $entry = array();

      $a = array();

      $a[0] = "top";
      $a[1] = "person";
      $a[2] = "qmailUser";
      $a[3] = "qmailGroup";

      $entry["objectClass"] = $a;

      $entry["cn"] = $user['username'];
      $entry["sn"] = "x";
      $entry["uid"] = (int)$user['uid'];
      $entry["mail"] = $user['email'];
      $entry["policygroupid"] = (int)$user['policy_group'];
      $entry["isadmin"] = (int)$user['isadmin'];
      $entry["userPassword"] = "{MD5}" . base64_encode(md5($user['password'], TRUE));

      $entry["mailalternateaddress"] = $this->trim_to_array($user['mailalternateaddress']);
      $entry["filtersender"] = $user['whitelist'];
      $entry["blacklist"] = $user['blacklist'];

      if(isset($user['proxydn'])) { $entry["proxydn"] = $user['proxydn']; }
      if(isset($user['domain'])) { $entry["domain"] = $user['domain']; }

      $entry["mailMessageStore"] = "";

      if($this->db->ldap_add("cn=" . $user['username'] . "," .  LDAP_USER_BASEDN, $entry) == TRUE) {
         return 1;
      }

      return 0;
   }


   public function updateUser($user) {
      $entry = array();


      $entry["cn"] = $user['username'];
      $entry["mail"] = $user['email'];
      $entry["policygroupid"] = (int)$user['policy_group'];
      $entry["isadmin"] = (int)$user['isadmin'];

      if(strlen($user['password']) > 6) {
         $entry["userPassword"] = "{MD5}" . base64_encode(md5($user['password'], TRUE));
      }

      $entry["mailalternateaddress"] = $this->trim_to_array($user['mailalternateaddress']);
      $entry["filtersender"] = $user['whitelist'];
      $entry["blacklist"] = $user['blacklist'];

      if($this->db->ldap_modify("cn=" . $user['username'] . "," .  LDAP_USER_BASEDN, $entry) == TRUE) {
         return 1;
      }

      return 0;
   }


   public function deleteUser($uid = 0) {
      if($uid < 1){ return 0; }

      $username = $this->getNameByUid((int)$uid);

      if($this->db->ldap_delete("cn=$username," . LDAP_USER_BASEDN) == TRUE) {
         return 1;
      }

      return 0;
   }


   public function addEmail($uid = 0, $email = '') {
      if($uid < 1 || $email == ""){ return 0; }

      $username = $this->getNameByUid((int)$uid);

      $query = $this->db->ldap_query(LDAP_USER_BASEDN, "cn=$username", array("mailalternateaddress") );

      $entry = array();

      foreach ($query->rows as $aa) {
         array_push($entry, $aa['mailalternateaddress']);
      }

      array_push($entry, $email);

      $data['mailalternateaddress'] = $entry;


      $query = $this->db->ldap_replace("cn=$username," . LDAP_USER_BASEDN, $data);

      return 0;
   }


   public function trim_to_array($x){
      $a = array();

      $z = explode("\n", $x);
      while(list($k, $v) = each($z)){
         $v = rtrim($v);
         if($v){
            array_push($a, $v);
         }
      }

      if(count($a) == 0){ return ""; }

      return $a;
   }


   public function getWhitelist($username = '') {
      if($username == ""){ return array(); }

      $query = $this->db->ldap_query(LDAP_USER_BASEDN, "cn=$username", array("filtersender") );

      return $query->row['filtersender'];
   }


   public function setWhitelist($username = '', $whitelist = '') {

      $entry = array();

      $entry["filtersender"] = $whitelist;

      if($this->db->ldap_replace("cn=$username," . LDAP_USER_BASEDN, $entry) == TRUE) {
         return 1;
      }

      return 0;
   }


   public function getBlacklist($username = '') {
      if($username == ""){ return array(); }

      $query = $this->db->ldap_query(LDAP_USER_BASEDN, "cn=$username", array("blacklist") );

      return $query->row['blacklist'];
   }


   public function setBlacklist($username = '', $blacklist = '') {

      $entry = array();

      $entry["blacklist"] = $blacklist;

      if($this->db->ldap_replace("cn=$username," . LDAP_USER_BASEDN, $entry) == TRUE) {
         return 1;
      }

      return 0;
   }




}


?>
