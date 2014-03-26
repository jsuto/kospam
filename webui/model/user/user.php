<?php

class ModelUserUser extends Model {


   public function checkUID($uid) {
      if($uid == "") { return 0; }

      if(!is_numeric($uid)) { return 0; }

      if($uid < 1) { return 0; }

      return 1;
   }


   public function getWhitelist($username = '') {

      $uid = $this->getUidByName($username);

      $query = $this->db->query("SELECT whitelist FROM " . TABLE_WHITELIST . " WHERE uid=?", array($uid));

      if(isset($query->row['whitelist'])){
         return $query->row['whitelist'];
      }

      return "";
   }


   public function setWhitelist($username = '', $whitelist = '') {

      $uid = $this->getUidByName($username);

      $query = $this->db->query("UPDATE " . TABLE_WHITELIST . " SET whitelist=? WHERE uid=?", array($whitelist, $uid));

      if(MEMCACHED_ENABLED) {
         $memcache = Registry::get('memcache');
         $memcache->delete("_c:wbl" . (int)$uid);
      }


      $rc = $this->db->countAffected();

      LOGGER("set whitelist for $username");

      return $rc;
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

      $query = $this->db->query("UPDATE " . TABLE_BLACKLIST . " SET blacklist=? WHERE uid=?", array($blacklist, $uid));

      if(MEMCACHED_ENABLED) {
         $memcache = Registry::get('memcache');
         $memcache->delete("_c:wbl" . (int)$uid);
      }

      $rc = $this->db->countAffected();

      LOGGER("set blacklist for $username");

      return $rc;
   }


   public function getUidByName($username = '') {
      if($username == ""){ return -1; }

      $query = $this->db->query("SELECT uid FROM " . TABLE_USER . " WHERE username=?", array($username));

      if(isset($query->row['uid'])){
         return $query->row['uid'];
      }

      return -1;
   }


   public function getUsernameByUid($uid = 0) {

      $query = $this->db->query("SELECT username FROM " . TABLE_USER . " WHERE uid=?", array($uid));

      if(isset($query->row['username'])){
         return $query->row['username'];
      }

      return "";
   }


   public function get_uid_by_email($email = '') {
      $query = $this->db->query("SELECT uid FROM " . TABLE_EMAIL . " WHERE email=?", array($email));

      if(isset($query->row['uid'])){ return $query->row['uid']; }

      return -1;
   }


   public function get_username_by_email($email = '') {
      $query = $this->db->query("SELECT username FROM " . TABLE_USER . ", " . TABLE_EMAIL . " WHERE " . TABLE_USER . ".uid=" . TABLE_EMAIL . ".uid AND email=?", array($email));

      if(isset($query->row['username'])){ return $query->row['username']; }

      return "";
   }


   public function get_additional_uids($uid = 0) {
      $data = array();

      if($uid > 0) {
         $query = $this->db->query("SELECT gid FROM " . TABLE_QUARANTINE_GROUP . " WHERE uid=?", array($uid));

         if(isset($query->rows)) {
            foreach ($query->rows as $q) {
               array_push($data, $q['gid']);
            }
         }
      }

      return $data;
   }


   public function getEmailAddress($username = '') {

      $query = $this->db->query("SELECT " . TABLE_EMAIL . ".email AS email FROM " . TABLE_EMAIL . "," . TABLE_USER . " WHERE " . TABLE_EMAIL . ".uid=" . TABLE_USER . ".uid AND " . TABLE_USER . ".username=? LIMIT 1", array($username));

      if(isset($query->row['email'])){
         return $query->row['email'];
      }

      return "";
   }


   public function getEmails($username = '') {
      $emails = "";

      $query = $this->db->query("SELECT " . TABLE_EMAIL . ".email AS email FROM " . TABLE_EMAIL . "," . TABLE_USER . " WHERE " . TABLE_EMAIL . ".uid=" . TABLE_USER . ".uid AND " . TABLE_USER . ".username=?", array($username));

      foreach ($query->rows as $q) {
         $emails .= $q['email'] . "\n";
      }

      return preg_replace("/\n$/", "", $emails);
   }


   public function getEmailsByUid($uid = 0) {
      $emails = "";

      $query = $this->db->query("SELECT email FROM " . TABLE_EMAIL . " WHERE uid=?", array($uid));
      foreach ($query->rows as $q) {
         $emails .= $q['email'] . "\n";
      }

      return preg_replace("/\n$/", "", $emails);
   }


   public function getDomains() {
      $data = array();

      if(Registry::get('domain_admin') == 1) {
         $query = $this->db->query("SELECT domain FROM " . TABLE_USER . " WHERE username=?", array($_SESSION['username']));
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

      $query = $this->db->query("SELECT domain FROM " . TABLE_USER . " WHERE uid=?", array($uid));

      foreach ($query->rows as $q) {
         array_push($data, $q['domain']);
      }

      return $data;
   }


   public function getUidsByDomain($domain = '') {
      $data = array();

      $query = $this->db->query("SELECT uid FROM " . TABLE_USER . " WHERE domain=?", array($domain));

      foreach ($query->rows as $q) {
         array_push($data, $q['uid']);

         $u = $this->get_additional_uids($q['uid']);

         $data = array_merge($data, $u);
      }

      return $data;
   }


   public function get_quarantine_directories($uid = 0) {
      $data = array();

      $query = $this->db->query("SELECT " . TABLE_USER . ".username, " . TABLE_USER . ".domain, " . TABLE_USER . ".uid FROM " . TABLE_USER . "," . TABLE_QUARANTINE_GROUP . " WHERE " . TABLE_QUARANTINE_GROUP . ".gid=? AND " . TABLE_USER . ".uid=" . TABLE_QUARANTINE_GROUP . ".uid", array($uid));


      if(isset($query->rows)) {
         foreach($query->rows as $q) {
            array_push($data, get_per_user_queue_dir($q['domain'], $q['username'], $q['uid']));
         }
      }

      return $data;
   }


   public function getEmailDomains() {
      $data = array();

      if(Registry::get('domain_admin') == 1) {
         $my_domain = $this->getDomains();
         $query = $this->db->query("SELECT domain FROM " . TABLE_DOMAIN . " WHERE mapped=?", array($my_domain[0]));
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

      $query = $this->db->query("SELECT domain FROM " . TABLE_USER . " WHERE username=?", array($_SESSION['username']));
      if(!isset($query->row['domain'])) { return 0; }

      /* query user's domain */

      $query2 = $this->db->query("SELECT domain FROM " . TABLE_USER . " WHERE username=?", array($username));
      if(!isset($query2->row['domain'])) { return 0; }

      if($query->row['domain'] == $query2->row['domain']) { return 1; }

      return 0;
   }


   public function isUidInMyDomain($uid = 0) {
      if($uid == 0) { return 0; }

      /* query my domain */

      $query = $this->db->query("SELECT domain FROM " . TABLE_USER . " WHERE username=?", array($_SESSION['username']));
      if(!isset($query->row['domain'])) { return 0; }

      /* query user's domain */

      $query2 = $this->db->query("SELECT domain FROM " . TABLE_USER . " WHERE uid=?", array($uid));
      if(!isset($query2->row['domain'])) { return 0; }

      if($query->row['domain'] == $query2->row['domain']) { return 1; }

      return 0;
   }


   public function is_email_in_my_domain($email = '') {
      if($email == "") { return 0; }

      /* determine mapped domain of the email address */

      list($u, $d) = preg_split("/\@/", $email);
      if($d == "") { return 0; }

      $query = $this->db->query("SELECT mapped FROM " . TABLE_DOMAIN . " WHERE domain=?", array($d));
      if(!isset($query->row['mapped'])) { return 0; }


      /* query my domain */

      $query2 = $this->db->query("SELECT domain FROM " . TABLE_USER . " WHERE username=?", array($_SESSION['username']));
      if(!isset($query2->row['domain'])) { return 0; }

      if($query2->row['domain'] == $query->row['mapped']) { return 1; }

      return 0;
   }


   public function getUserByDN($dn = '') {
      if($dn == '') { return array(); }

      $query = $this->db->query("SELECT * FROM " . TABLE_USER . " WHERE dn=?", array($dn));

      if($query->num_rows == 1) {
         return $query->row;
      }

      return array();
   }


   public function getUserByUid($uid = 0) {
      if(!is_numeric($uid) || (int)$uid < 0){
         return array();
      }

      $query = $this->db->query("SELECT * FROM " . TABLE_USER . " WHERE uid=" . (int)$uid);

      return $query->row;
   }


   public function getUserByEmail($email = '') {
      if($email == '') {
         return array();
      }

      $query = $this->db->query("SELECT * FROM " . TABLE_USER . "," . TABLE_EMAIL . " WHERE " . TABLE_USER . ".uid=" . TABLE_EMAIL . ".uid AND email='" . $this->db->escape($email) . "'");

      return $query->row;
   }


   public function getUsernameByEmail($email = '') {
      $username = "";

      if($email == '') { return $username; }

      $query = $this->db->query("SELECT username FROM " . TABLE_USER . " WHERE uid IN (SELECT uid FROM " . TABLE_EMAIL . " WHERE email='" . $this->db->escape($email) . "')");

      if(isset($query->row['username'])) { $username = $query->row['username']; }

      return $username;
   }


   public function getUsers($search = '', $page = 0, $page_len = 0, $sort = 'username', $order = 0) {
      $where_cond = " WHERE " . TABLE_USER . ".uid=" . TABLE_EMAIL . ".uid ";
      $_order = "";
      $users = array();
      $my_domain = array();
      $limit = "";

      $from = (int)$page * (int)$page_len;

      if(Registry::get('domain_admin') == 1) {
         $my_domain = $this->getDomains();
         $where_cond .= " AND domain='" . $this->db->escape($my_domain[0]) . "'";
      }


      $search = preg_replace("/\s{1,}/", "", $search);

      if($search){
         $where_cond .= " AND email like '%" . $this->db->escape($search) . "%' ";
      }

      /* sort order */

      if($order == 0) { $order = "ASC"; }
      else { $order = "DESC"; }

      $_order = "ORDER BY $sort $order";

      if($page_len > 0) { $limit = " LIMIT " . (int)$from . ", " . (int)$page_len; }

      $query = $this->db->query("SELECT " . TABLE_USER . ".uid, gid, isadmin, username, realname, domain, dn, policy_group, email FROM " . TABLE_USER . "," . TABLE_EMAIL . " $where_cond group by " . TABLE_USER . ".uid $_order $limit");

      foreach ($query->rows as $q) {

         if(Registry::get('admin_user') == 1 || (isset($q['domain']) && $q['domain'] == $my_domain[0]) ) {
            $users[] = array(
                          'uid'          => $q['uid'],
                          'gid'          => $q['gid'],
                          'username'     => $q['username'],
                          'realname'     => $q['realname'],
                          'domain'       => isset($q['domain']) ? $q['domain'] : "",
                          'policy_group' => $q['policy_group'],
                          'email'        => $q['email'],
                          'dn'           => $q['dn'],
                          'isadmin'      => $q['isadmin']
                         );
         }

      }

      return $users;
   }


   public function howManyUsers($search = '') {
      $where_cond = "";

      if($search){
         $where_cond .= " WHERE email like '%" . $this->db->escape($search) . "%' ";
      }

      $query = $this->db->query("SELECT COUNT(*) AS num, uid FROM " . TABLE_EMAIL . " $where_cond group by uid");

      return $query->num_rows;
   }


   public function getNextUid() {

      $query = $this->db->query("SELECT MAX(uid) AS last_id FROM " . TABLE_USER);

      if(isset($query->row['last_id']) && $query->row['last_id'] > 0) {
         return (int)$query->row['last_id'] + 1;
      }

      return 1;
   }


   public function addUser($user) {
      LOGGER("add user: " . $user['username'] . ", uid=" . (int)$user['uid']);

      if(!isset($user['domain']) || $user['domain'] == "") { return -1; }
      if(!isset($user['username']) || $user['username'] == "" || $this->getUidByName($user['username']) > 0) { return -1; }

      $emails = explode("\n", $user['email']);
      foreach ($emails as $email) {
         $email = rtrim($email);

         $query = $this->db->query("SELECT COUNT(*) AS count FROM " . TABLE_EMAIL . " WHERE email='" . $this->db->escape($email) . "'");

         /* remove from memcached */

         if(MEMCACHED_ENABLED) {
            $memcache = Registry::get('memcache');
            $memcache->delete("_c:" . $this->db->escape($email));
         }

         if($query->row['count'] > 0) {
            return $email;
         }
      }


      $query = $this->db->query("SELECT COUNT(*) AS count FROM " . TABLE_USER . " WHERE username='" . $this->db->escape($user['username']) . "'");
      if($query->row['count'] > 0) {
         return $user['username'];
      }

      $encrypted_password = crypt($user['password']);

      $query = $this->db->query("INSERT INTO " . TABLE_USER . " (uid, gid, username, realname, password, domain, dn, policy_group, isadmin) VALUES(" . (int)$user['uid'] . ", " . (int)$user['gid'] . ", '" . $this->db->escape($user['username']) . "', '" . $this->db->escape($user['realname']) . "', '" . $this->db->escape($encrypted_password) . "', '" . $this->db->escape($user['domain']) . "', '" . $this->db->escape(@$user['dn']) . "', " . (int)$user['policy_group'] . ", " . (int)$user['isadmin'] . ")");

      if($query->error == 1 || $this->db->countAffected() == 0){ return $user['username']; }

      foreach ($emails as $email) {
         $email = rtrim($email);

         $ret = $this->addEmail((int)$user['uid'], $email);
         if($ret == 0) { return -2; }
      }


      $query = $this->db->query("INSERT INTO " . TABLE_WHITELIST . " (uid, whitelist) VALUES(" . (int)$user['uid'] . ", '" . $this->db->escape($user['whitelist']) . "')");

      $query = $this->db->query("INSERT INTO " . TABLE_BLACKLIST . " (uid, blacklist) VALUES(" . (int)$user['uid'] . ", '" . $this->db->escape($user['blacklist']) . "')");

      $query = $this->db->query("INSERT INTO " . TABLE_MISC . " (uid, nham, nspam) VALUES(" . (int)$user['gid'] . ", 0, 0)");

      return 1;
   }


   public function addEmail($uid = 0, $email = '') {
      if($uid < 1 || $email == ""){ return 0; }

      $query = $this->db->query("INSERT INTO " . TABLE_EMAIL . " (uid, email) VALUES(" . (int)$uid . ", '" . $this->db->escape($email) . "')");

      $rc = $this->db->countAffected();

      LOGGER("add email: $email, uid=$uid (rc=$rc)");

      return $rc;
   }


   public function removeEmail($uid = 0, $email = '') {
      if((int)$uid < 1 || $email == ""){ return 0; }

      $query = $this->db->query("DELETE FROM " . TABLE_EMAIL . " WHERE uid=" . (int)$uid . " AND email='" . $this->db->escape($email) . "'");

      $rc = $this->db->countAffected();

      LOGGER("remove email: $email, uid=$uid (rc=$rc)");

      return $rc;
   }


   public function updateUser($user) {
      LOGGER("update user: " . $user['username'] . ", uid=" . (int)$user['uid']);

      $emails = explode("\n", $user['email']);
      foreach ($emails as $email) {
         $email = rtrim($email);

         $query = $this->db->query("SELECT COUNT(*) AS count FROM " . TABLE_EMAIL . " WHERE uid!=" . (int)$user['uid'] . " AND email='" . $this->db->escape($email) . "'");

         if($query->row['count'] > 0) {
            return $email;
         }
      }


      /* update password field if we have to */
 
      if(strlen($user['password']) >= MIN_PASSWORD_LENGTH) {
         $query = $this->db->query("UPDATE " . TABLE_USER . " SET password='" . $this->db->escape(crypt($user['password'])) . "' WHERE uid=" . (int)$user['uid']);
         if($this->db->countAffected() != 1) { return 0; }
      }

      $query = $this->db->query("UPDATE " . TABLE_USER . " SET username='" . $this->db->escape($user['username']) ."', realname='" . $this->db->escape($user['realname']) ."', domain='" . $this->db->escape($user['domain']) . "', dn='" . @$this->db->escape($user['dn']) . "', policy_group=" . (int)$user['policy_group'] . ", isadmin=" . $user['isadmin'] . " WHERE uid=" . (int)$user['uid']);


      /* first, remove all his email addresses */

      $query = $this->db->query("DELETE FROM " . TABLE_EMAIL . " WHERE uid=" . (int)$user['uid']);

      /* then add all the emails we have from the CGI post input */

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


   public function deleteUser($uid) {
      if(!$this->checkUID($uid)){ return 0; }

      $query = $this->db->query("DELETE FROM " . TABLE_EMAIL . " WHERE uid=?", array($uid));
      $query = $this->db->query("DELETE FROM " . TABLE_USER . " WHERE uid=?", array($uid));
      $query = $this->db->query("DELETE FROM " . TABLE_WHITELIST . " WHERE uid=?", array($uid));
      $query = $this->db->query("DELETE FROM " . TABLE_BLACKLIST . " WHERE uid=?", array($uid));

      LOGGER("remove user: uid=$uid");

      return 1;
   }



}

?>
