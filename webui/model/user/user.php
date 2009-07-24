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


   public function getUserByUid($uid = 0) {
      if(!is_numeric($uid) || (int)$uid < 0){
         return array();
      }

      $query = $this->db->query("SELECT * FROM " . TABLE_USER . " WHERE uid=" . (int)$uid);

      return $query->row;
   }


   public function getUsers($search = '', $page = 0, $page_len = 0) {
      $where_cond = " WHERE " . TABLE_USER . ".uid=" . TABLE_EMAIL . ".uid ";
      $from = (int)$page * (int)$page_len;

      if($search){
         $where_cond .= " AND (" . TABLE_USER . ".username LIKE '%" . $this->db->escape($search) . "%' OR " . TABLE_EMAIL . ".email LIKE '%" . $this->db->escape($search) . "%')";
      }

      $query = $this->db->query("SELECT " . TABLE_USER . ".uid, " . TABLE_USER . ".username, " . TABLE_USER . ".policy_group, " . TABLE_EMAIL . ".email FROM " . TABLE_USER . "," . TABLE_EMAIL . $where_cond . " ORDER BY " . TABLE_USER . ".uid LIMIT " . (int)$from . ", " . (int)$page_len);

      return $query->rows;
   }


   public function howManyUsers($search = '') {
      $where_cond = " WHERE " . TABLE_USER . ".uid=" . TABLE_EMAIL . ".uid ";

      if($search){
         $where_cond .= " AND (" . TABLE_USER . ".username LIKE '%" . $this->db->escape($search) . "%' OR " . TABLE_EMAIL . ".email LIKE '%" . $this->db->escape($search) . "%')";
      }

      $query = $this->db->query("SELECT COUNT(*) AS num_users FROM " . TABLE_USER . "," . TABLE_EMAIL . $where_cond);

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

      $query = $this->db->query("INSERT INTO " . TABLE_USER . " (uid, username, password, policy_group, isadmin) VALUES(" . (int)$user['uid'] . ", '" . $this->db->escape($user['username']) . "', '" . $this->db->escape($encrypted_password) . "', " . (int)$user['policy_group'] . ", " . (int)$user['isadmin'] . ")");

      if($this->db->countAffected() == 0){ return -1; }


      $query = $this->db->query("INSERT INTO " . TABLE_EMAIL . " (uid, email) VALUES(" . (int)$user['uid'] . ", '" . $this->db->escape($user['email']) . "')");

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

      $query = $this->db->query("UPDATE " . TABLE_USER . " SET username='" . $this->db->escape($user['username']) ."', policy_group=" . (int)$user['policy_group'] . ", isadmin=" . $user['isadmin'] . " WHERE uid=" . (int)$user['uid']);

      $query = $this->db->query("UPDATE " . TABLE_EMAIL . " SET email='" . $this->db->escape($user['email']) . "' WHERE uid=" . (int)$user['uid'] . " AND email='" . $this->db->escape($user['email_orig']) . "'");
      return 1;
   }


   public function deleteUser($uid = 0, $email = '') {
      if($uid < 1 || $email == ""){ return 0; }

      /* determine if this is the last user entry */

      $query = $this->db->query("SELECT COUNT(*) AS count FROM " . TABLE_EMAIL . " WHERE uid=" . (int)$uid);
      $n = $query->row['count'];

      $query = $this->db->query("DELETE FROM " . TABLE_EMAIL . " WHERE uid=" . (int)$uid . " AND email='" .  $this->db->escape($email) . "'");

      if($this->db->countAffected() != 1) { return 0; }

      if($n == 1 && (int)$uid > 0){
         $query = $this->db->query("DELETE FROM " . TABLE_USER . " WHERE uid=" . (int)$uid);
         $query = $this->db->query("DELETE FROM " . TABLE_WHITELIST . " WHERE uid=" . (int)$uid);
         $query = $this->db->query("DELETE FROM " . TABLE_BLACKLIST . " WHERE uid=" . (int)$uid);
         $query = $this->db->query("DELETE FROM " . TABLE_MISC . " WHERE uid=" . (int)$uid);
      }

      return 1;
   }



}

?>
