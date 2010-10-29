<?php


class ModelUserImport extends Model {


   public function getLdapParameters() {
      $my_domain = $this->model_user_user->getDomains();
      $query = $this->db->query("SELECT remotehost, basedn, binddn FROM " . TABLE_REMOTE . " WHERE remotedomain='" . $this->db->escape($my_domain[0]) . "'");

      return $query->row;
   }


   public function queryRemoteUsers($host) {
      $data = array();

      LOGGER("running queryRemoteUsers() ...");

      $attrs = array("cn", "mail", "mailAlternateAddress");
      $mailAttr = 'mail';
      $mailAttrs = array("mail", "mailalternateaddress");

      $ldap = new LDAP($host['ldap_host'], $host['ldap_binddn'], $host['ldap_bindpw']);
      if($ldap->is_bind_ok() == 0) {
         LOGGER($host['ldap_binddn'] . ": failed bind to " . $host['ldap_host']);
         return 0;
      }

      LOGGER($host['ldap_binddn'] . ": successful bind to " . $host['ldap_host']);
      LOGGER("LDAP type: " . $host['type']);

      if($host['type'] == "AD") {
         $attrs = array("cn", "proxyAddresses");

         $mailAttr = "proxyAddresses";
         $mailAttrs = array("proxyAddresses");
      }


      $query = $ldap->query($host['ldap_basedn'], "$mailAttr=*", $attrs );
      LOGGER("LDAP query: $mailAttr=* for basedn:" . $host['ldap_basedn']);

      foreach ($query->rows as $result) {
         $emails = "";

         if(!isset($result['cn']) || !isset($result['dn']) ) { continue; }

         foreach($mailAttrs as $__mail_attr) {

            if(isset($result[$__mail_attr]) ) {

               if(is_array($result[$__mail_attr]) ) {
                  for($i = 0; $i < $result[$__mail_attr]['count']; $i++) {
                     LOGGER("found email entry: " . $result['dn'] . " => $__mail_attr:" . $result[$__mail_attr][$i]);
                     $emails .= preg_replace("/smtp\:/i", "", $result[$__mail_attr][$i]) . "\n";
                  }
               }
               else {
                  LOGGER("found email entry: " . $result['dn'] . " => $__mail_attr:" . $result[$__mail_attr]);
                  $emails .= preg_replace("/smtp\:/i", "", $result[$__mail_attr]) . "\n";
               }

            }

         }

         $data[] = array(
                         'username'     => $result['cn'],
                         'dn'           => $result['dn'],
                         'emails'       => preg_replace("/\n{1,}$/", "", $emails)
                        );

      }

      LOGGER("found " . count($data) . " users");

      return $data;
   }



   public function fillRemoteTable($host = array(), $domain = '') {
      if($domain == '') { return 0; }

      /*
       * if the 't_remote' table has no entry for your domain and we read some users
       * let's put the connection info to the 't_remote' table needed for proxying
       * the authentication requests
       */

      $query = $this->db->query("SELECT COUNT(*) AS num FROM " . TABLE_REMOTE . " WHERE remotedomain='" . $this->db->escape($domain) . "'");

      if(isset($query->row['num'])) {

         if($query->row['num'] == 0) {
            $query = $this->db->query("INSERT INTO " . TABLE_REMOTE . " (remotedomain, remotehost, basedn, binddn) VALUES('" . $this->db->escape($domain) . "', '" . $this->db->escape($host['ldap_host']) . "', '" . $this->db->escape($host['ldap_basedn']) . "', '" . $this->db->escape($host['ldap_binddn']) . "')");
         }
         else {
            $query = $this->db->query("UPDATE " . TABLE_REMOTE . " SET remotehost='" . $this->db->escape($host['ldap_host']) . "', basedn='" . $this->db->escape($host['ldap_basedn']) . "', binddn='" . $this->db->escape($host['ldap_binddn']) . "' WHERE remotedomain='" . $this->db->escape($domain) . "'");

         }

         LOGGER("SQL exec:" . $query->query);

      }

      return 1;
   }



   public function processUsers($users = array(), $globals = array()) {
      $late_add = array();
      $uids = array();
      $exclude = array();
      $n = 0;

      LOGGER("running processUsers() ...");

      /* build a list of DNs to exclude from the import */

      while (list($k, $v) = each($globals)) {
         if(preg_match("/^reject_/", $k)) {
            $exclude[$v] = $v;
         }
      }


      foreach ($users as $_user) {
         if(strlen($_user['dn']) > DN_MAX_LEN) { LOGGER("ERR: too long entry: " . $_user['dn']); }

         if(in_array($_user['dn'], $exclude) ) {
            LOGGER("excluding from import:" . $_user['dn']);
            continue;
         }

         /* Does this DN exist in the user table ? */

         $__user = $this->model_user_user->getUserByDN($_user['dn']);

         if(isset($__user['uid'])) {

            array_push($uids, $__user['uid']);


            /* if so, then verify the email addresses */

            $changed = 0;
            $emails = $this->model_user_user->getEmailsByUid($__user['uid']);

            /* first let's add the new email addresses */

            $ldap_emails = explode("\n", $_user['emails']);
            $sql_emails = explode("\n", $emails);

            foreach ($ldap_emails as $email) {
               if(!in_array($email, $sql_emails)) {
                  $rc = $this->model_user_user->addEmail($__user['uid'], $email);
                  $changed++;

                  /* in case of an error add it to the $late_add array() */

                  if($rc == 0) {
                     $late_add[] = array(
                                          'uid'   => $__user['uid'],
                                          'email' => $email
                                        );
                  }
               }
            }


            /* delete emails not present in the user's LDAP entry */

            foreach ($sql_emails as $email) {
               if(!in_array($email, $ldap_emails)) {
                  $rc = $this->model_user_user->removeEmail($__user['uid'], $email);
                  $changed++;
               }
            }

            LOGGER($_user['dn'] . ": exists, changed=$changed");

            if($changed > 0) { $n++; }
         }
         else {

            /* or add the new user */

            $user = $this->createNewUserArray($_user['dn'], $_user['username'], $_user['emails'], $globals);
            array_push($uids, $user['uid']);

            $rc = $this->model_user_user->addUser($user);
            if($rc == 1) { $n++; }
         }
      }


      /* add the rest to the email table */

      foreach ($late_add as $new) {
         $rc = $this->model_user_user->addEmail($new['uid'], $new['email']);
         if($rc == 1) { $n++; }
      }


      /* delete accounts not present in the LDAP directory */

      if(count($uids) > 0) {
         $uidlist = implode("','", $uids);
         $query = $this->db->query("SELECT uid, username FROM " . TABLE_USER . " WHERE domain='" . $this->db->escape($globals['domain']) . "' AND dn != '*' AND dn is NOT NULL AND uid NOT IN ('$uidlist')");

         foreach ($query->rows as $deleted) {
            $this->model_user_user->deleteUser($deleted['uid']);
         }
      }

      return $n;
   }


   private function createNewUserArray($dn = '', $username = '', $emails = '', $globals = array()) {
      $user = array();

      $user['uid'] = $this->model_user_user->getNextUid();
      $user['gid'] = $globals['gid'];

      $user['email'] = $emails;

      if(USE_EMAIL_AS_USERNAME == 1) {
         $email = explode("\n", $emails);
         $user['username'] = $email[0];
      }
      else {
         $user['username'] = $username . $user['uid'];
      }


      $user['password'] = '*';

      $user['realname'] = $username;

      $user['domain'] = $globals['domain'];
      $user['dn'] = $dn;
      $user['policy_group'] = $globals['policy_group'];
      $user['isadmin'] = 0;
      $user['whitelist'] = '';
      $user['blacklist'] = '';

      return $user;
   }


   public function trashPassword($users = array()) {
      foreach ($users as $user) {
         $query = $this->db->query("UPDATE " . TABLE_USER . " SET password='*' WHERE dn='" . $user['dn'] . "'");
         $rc = $this->db->countAffected();
         LOGGER("setting default password for " . $user['dn'] . " (rc=$rc)");
      }
   }

}

?>
