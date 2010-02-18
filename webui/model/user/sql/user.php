<?php

class ModelUserUser extends Model {

   public function getWhitelist($username = '') {

      $uid = $this->getUidByName($username);

      $query = $this->db->query("SELECT whitelist FROM " . TABLE_WHITELIST . " WHERE uid=" . (int)$uid);

      if(isset($query->row['whitelist'])){
         return $query->row['whitelist'];
      }

      return "";
   }


   public function setWhitelist($username = '', $whitelist = '') {

      $uid = $this->getUidByName($username);

      $query = $this->db->query("UPDATE " . TABLE_WHITELIST . " SET whitelist='" . $this->db->escape($whitelist) . "' WHERE uid=" . (int)$uid);

      return $this->db->countAffected();
   }


   public function getBlacklist($username = '') {

      $uid = $this->getUidByName($username);

      $query = $this->db->query("SELECT blacklist FROM " . TABLE_BLACKLIST . " WHERE uid=" . (int)$uid);

      if(isset($query->row['blacklist'])){
         return $query->row['blacklist'];
      }

      return "";
   }


   public function setBlacklist($username = '', $blacklist = '') {

      $uid = $this->getUidByName($username);

      $query = $this->db->query("UPDATE " . TABLE_BLACKLIST . " SET blacklist='" . $this->db->escape($blacklist) . "' WHERE uid=" . (int)$uid);

      return $this->db->countAffected();
   }


   public function getUidByName($username = '') {
      if($username == ""){ return -1; }

      $query = $this->db->query("SELECT uid FROM " . TABLE_USER . " WHERE username='" . $this->db->escape($username) . "'");

      if(isset($query->row['uid'])){
         return $query->row['uid'];
      }

      return -1;
   }


   public function getEmailAddress($username = '') {

      $query = $this->db->query("SELECT " . TABLE_EMAIL . ".email AS email FROM " . TABLE_EMAIL . "," . TABLE_USER . " WHERE " . TABLE_EMAIL . ".uid=" . TABLE_USER . ".uid AND " . TABLE_USER . ".username='" . $this->db->escape($username) . "' LIMIT 1");

      if(isset($query->row['email'])){
         return $query->row['email'];
      }

      return "";
   }


   public function getEmails($username = '') {
      $emails = "";

      $query = $this->db->query("SELECT " . TABLE_EMAIL . ".email AS email FROM " . TABLE_EMAIL . "," . TABLE_USER . " WHERE " . TABLE_EMAIL . ".uid=" . TABLE_USER . ".uid AND " . TABLE_USER . ".username='" . $this->db->escape($username) . "'");

      foreach ($query->rows as $q) {
         $emails .= $q['email'] . "\n";
      }

      return preg_replace("/\n$/", "", $emails);
   }


   public function getDomains() {
      $data = array();

      if(Registry::get('domain_admin') == 1) {
         $query = $this->db->query("SELECT domain FROM " . TABLE_USER . " WHERE username='" . $this->db->escape($_SESSION['username']) . "'");
      }
      else {
         $query = $this->db->query("SELECT DISTINCT mapped AS domain FROM " . TABLE_DOMAIN);
      }

      foreach ($query->rows as $q) {
         array_push($data, $q['domain']);
      }

      return $data;
   }


   public function getDomainsByUid($uid = 0) {
      $data = array();

      $query = $this->db->query("SELECT domain FROM " . TABLE_USER . " WHERE uid=" . (int)$uid);

      foreach ($query->rows as $q) {
         array_push($data, $q['domain']);
      }

      return $data;
   }


   public function getEmailDomains() {
      $data = array();

      if(Registry::get('domain_admin') == 1) {
         $my_domain = $this->getDomains();
         $query = $this->db->query("SELECT domain FROM " . TABLE_DOMAIN . " WHERE mapped='" . $this->db->escape($my_domain[0]) . "'");
      }
      else {
         $query = $this->db->query("SELECT domain FROM " . TABLE_DOMAIN);
      }

      foreach ($query->rows as $q) {
         array_push($data, $q['domain']);
      }

      return $data;
   }


   public function isUserInMyDomain($username = '') {
      if($username == "") { return 0; }

      /* query my domain */

      $query = $this->db->query("SELECT domain FROM " . TABLE_USER . " WHERE username='" . $this->db->escape($_SESSION['username']) . "'");
      if(!isset($query->row['domain'])) { return 0; }

      /* query user's domain */

      $query2 = $this->db->query("SELECT domain FROM " . TABLE_USER . " WHERE username='" . $this->db->escape($username) . "'");
      if(!isset($query2->row['domain'])) { return 0; }

      if($query->row['domain'] == $query2->row['domain']) { return 1; }

      return 0;
   }


   public function isUidInMyDomain($uid = 0) {
      if($uid == 0) { return 0; }

      /* query my domain */

      $query = $this->db->query("SELECT domain FROM " . TABLE_USER . " WHERE username='" . $this->db->escape($_SESSION['username']) . "'");
      if(!isset($query->row['domain'])) { return 0; }

      /* query user's domain */

      $query2 = $this->db->query("SELECT domain FROM " . TABLE_USER . " WHERE uid=" . (int)$uid);
      if(!isset($query2->row['domain'])) { return 0; }

      if($query->row['domain'] == $query2->row['domain']) { return 1; }

      return 0;
   }


   public function getUserByUid($uid = 0) {
      if(!is_numeric($uid) || (int)$uid < 0){
         return array();
      }

      $query = $this->db->query("SELECT * FROM " . TABLE_USER . " WHERE uid=" . (int)$uid);

      return $query->row;
   }


   public function getUsers($search = '', $page = 0, $page_len = 0, $uid = '', $user = '', $email = '', $domain = '') {
      $where_cond = "";
      $order = "";
      $users = array();
      $my_domain = array();

      $from = (int)$page * (int)$page_len;

      if($search){
         $where_cond .= " WHERE uid IN (SELECT DISTINCT uid FROM " . TABLE_USER . " WHERE username LIKE '%" . $this->db->escape($search) . "%') OR uid IN (SELECT DISTINCT uid FROM " . TABLE_EMAIL . " WHERE email LIKE '%" . $this->db->escape($search) . "%')";
      }

      if(Registry::get('domain_admin') == 1) {
         $my_domain = $this->getDomains();
      }

      /* sort orders */

      if($uid != ""){
         if((int)$uid == 1) { $order = "ORDER BY uid DESC"; }
         if((int)$uid == 0) { $order = "ORDER BY uid ASC"; }
      }

      if($user != ""){
         if((int)$user == 1) { $order = "ORDER BY username DESC"; }
         if((int)$user == 0) { $order = "ORDER BY username ASC"; }
      }

      if($domain != ""){
         if((int)$domain == 1) { $order = "ORDER BY domain DESC"; }
         if((int)$domain == 0) { $order = "ORDER BY domain ASC"; }
      }


      $query = $this->db->query("SELECT uid, username, domain, policy_group FROM " . TABLE_USER . " $where_cond $order LIMIT " . (int)$from . ", " . (int)$page_len);

      foreach ($query->rows as $q) {
         $email = $this->db->query("SELECT email FROM " . TABLE_EMAIL . " WHERE uid=" . (int)$q['uid'] . " LIMIT 1");

         if(Registry::get('admin_user') == 1 || (isset($q['domain']) && $q['domain'] == $my_domain[0]) ) {
            $users[] = array(
                          'uid'          => $q['uid'],
                          'username'     => $q['username'],
                          'domain'       => isset($q['domain']) ? $q['domain'] : "",
                          'policy_group' => $q['policy_group'],
                          'email'        => $email->row['email']
                         );
         }

      }

      return $users;
   }


   public function howManyUsers($search = '') {
      $where_cond = "";

      if($search){
         $where_cond .= " WHERE uid IN (SELECT DISTINCT uid FROM " . TABLE_USER . " WHERE username LIKE '%" . $this->db->escape($search) . "%') OR uid IN (SELECT DISTINCT uid FROM " . TABLE_EMAIL . " WHERE email LIKE '%" . $this->db->escape($search) . "%')";
      }

      $query = $this->db->query("SELECT COUNT(*) AS num_users FROM " . TABLE_USER . $where_cond);

      return $query->row['num_users'];
   }


   public function getNextUid() {

      $query = $this->db->query("SELECT MAX(uid) AS last_id FROM " . TABLE_USER);

      if(isset($query->row['last_id']) && $query->row['last_id'] > 0) {
         return (int)$query->row['last_id'] + 1;
      }

      return 1;
   }


   public function addUser($user) {

      $encrypted_password = crypt($user['password']);

      $query = $this->db->query("INSERT INTO " . TABLE_USER . " (uid, username, password, domain, dn, policy_group, isadmin) VALUES(" . (int)$user['uid'] . ", '" . $this->db->escape($user['username']) . "', '" . $this->db->escape($encrypted_password) . "', '" . $this->db->escape($user['domain']) . "', '" . $this->db->escape(@$user['dn']) . "', " . (int)$user['policy_group'] . ", " . (int)$user['isadmin'] . ")");

      if($this->db->countAffected() == 0){ return -1; }

      $emails = explode("\n", $user['email']);
      foreach ($emails as $email) {
         $email = rtrim($email);

         $query = $this->db->query("SELECT COUNT(*) AS count FROM " . TABLE_EMAIL . " WHERE email='" . $this->db->escape($email) . "'");

         if($query->row['count'] == 0) {
            $query = $this->db->query("INSERT INTO " . TABLE_EMAIL . " (uid, email) VALUES(" . (int)$user['uid'] . ", '" . $this->db->escape($email) . "')");
         }
      }

      //$query = $this->db->query("INSERT INTO " . TABLE_EMAIL . " (uid, email) VALUES(" . (int)$user['uid'] . ", '" . $this->db->escape($user['email']) . "')");

      if($this->db->countAffected() == 0){ return -2; }


      $query = $this->db->query("INSERT INTO " . TABLE_WHITELIST . " (uid, whitelist) VALUES(" . (int)$user['uid'] . ", '" . $this->db->escape($user['whitelist']) . "')");

      $query = $this->db->query("INSERT INTO " . TABLE_BLACKLIST . " (uid, blacklist) VALUES(" . (int)$user['uid'] . ", '" . $this->db->escape($user['blacklist']) . "')");

      $query = $this->db->query("INSERT INTO " . TABLE_MISC . " (uid, nham, nspam) VALUES(" . (int)$user['uid'] . ", 0, 0)");

      return 1;
   }


   public function addEmail($uid = 0, $email = '') {
      if($uid < 1 || $email == ""){ return 0; }

      $query = $this->db->query("INSERT INTO " . TABLE_EMAIL . " (uid, email) VALUES(" . (int)$uid . ", '" . $this->db->escape($email) . "')");

      return $this->db->countAffected(); 
   }


   public function updateUser($user) {

      /* update password field if we have to */
 
      if(strlen($user['password']) > 6) {
         $query = $this->db->query("UPDATE " . TABLE_USER . " SET password='" . $this->db->escape(crypt($user['password'])) . "' WHERE uid=" . (int)$user['uid']);
         if($this->db->countAffected() != 1) { return 0; }
      }

      $query = $this->db->query("UPDATE " . TABLE_USER . " SET username='" . $this->db->escape($user['username']) ."', domain='" . $this->db->escape($user['domain']) . "', dn='" . @$this->db->escape($user['dn']) . "', policy_group=" . (int)$user['policy_group'] . ", isadmin=" . $user['isadmin'] . " WHERE uid=" . (int)$user['uid']);


      /* first, remove all his email addresses */

      $query = $this->db->query("DELETE FROM " . TABLE_EMAIL . " WHERE uid=" . (int)$user['uid']);

      /* then add all the emails we have from the CGI post input */

      $emails = explode("\n", $user['email']);
      foreach ($emails as $email) {
         $email = rtrim($email);
         $query = $this->db->query("INSERT INTO " . TABLE_EMAIL . " (uid, email) VALUES(" . (int)$user['uid'] . ", '" . $this->db->escape($email) . "')");

         /* remove from memcached */

         if(MEMCACHED_ENABLED) {
            $memcache = Registry::get('memcache');
            $memcache->delete("_c:" . $this->db->escape($email));
         }

      }

      return 1;
   }


   public function deleteUser($uid = 0) {
      if($uid < 1){ return 0; }

      $query = $this->db->query("DELETE FROM " . TABLE_EMAIL . " WHERE uid=" . (int)$uid);
      $query = $this->db->query("DELETE FROM " . TABLE_USER . " WHERE uid=" . (int)$uid);
      $query = $this->db->query("DELETE FROM " . TABLE_WHITELIST . " WHERE uid=" . (int)$uid);
      $query = $this->db->query("DELETE FROM " . TABLE_BLACKLIST . " WHERE uid=" . (int)$uid);
      $query = $this->db->query("DELETE FROM " . TABLE_MISC . " WHERE uid=" . (int)$uid);

      return 1;
   }



}

?>
