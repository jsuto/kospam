<?php


class ModelUserImport extends Model {


   public function getLdapParameters() {
      $my_domain = $this->model_user_user->getDomains();
      $query = $this->db->query("SELECT remotehost, basedn, binddn FROM " . TABLE_REMOTE . " WHERE remotedomain='" . $this->db->escape($my_domain[0]) . "'");

      return $query->row;
   }


   public function queryRemoteUsers($host) {
      $data = array();

      $attrs = array("cn", "mail", "mailAlternateAddress");
      $mailAttr = 'mail';
      $mailAttrs = array("mail", "mailalternateaddress");

      $ldap = new LDAP($host['ldap_host'], $host['ldap_binddn'], $host['ldap_bindpw']);
      if($ldap->is_bind_ok() == 0) { return 0; }

      if($host['type'] == "AD") {
         $attrs = array("cn", "proxyAddresses");

         $mailAttr = "proxyAddresses";
         $mailAttrs = array("proxyAddresses");
      }


      $query = $ldap->query($host['ldap_basedn'], "$mailAttr=*", $attrs );

      foreach ($query->rows as $result) {
         $emails = "";

         if(!isset($result['cn']) || !isset($result['dn']) ) { continue; }

         foreach($mailAttrs as $__mail_attr) {

            if(isset($result[$__mail_attr]) ) {

               if(is_array($result[$__mail_attr]) ) {
                  for($i = 0; $i < $result[$__mail_attr]['count']; $i++) {
                     $emails .= preg_replace("/smtp\:/i", "", $result[$__mail_attr][$i]) . "\n";
                  }
               }
               else {
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

      }

      return 1;
   }



   public function processUsers($users = array(), $globals = array(), $verbose = 0) {
      $late_add = array();
      $uids = array();
      $exclude = array();
      $n = 0;

      /* build a list of DNs to exclude from the import */

      while (list($k, $v) = each($globals)) {
         if(preg_match("/^reject_/", $k)) {
            $exclude[$v] = $v;
         }
      }


      foreach ($users as $_user) {

         if(in_array($_user['dn'], $exclude) ) { continue; }

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
                  if($verbose) { print "new email: $email => $rc\n"; }

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
                  if($verbose) { print "remove email: $email => $rc\n"; }

                  $changed++;
               }
            }

            if($verbose) { print $_user['dn'] . ": exists, changed=$changed\n"; }

            if($changed > 0) { $n++; }
         }
         else {

            /* or add the new user */

            $user = $this->createNewUserArray($_user['dn'], $_user['username'], $_user['emails'], $globals);
            array_push($uids, $user['uid']);

            $rc = $this->model_user_user->addUser($user);
            if($rc == 1) { $n++; }

            if($verbose) { print $_user['dn'] . ": new, rc=$rc\n"; }
         }
      }


      /* add the rest to the email table */

      foreach ($late_add as $new) {
         $rc = $this->model_user_user->addEmail($new['uid'], $new['email']);
         if($rc == 1) { $n++; }

         if($verbose) { print $new['uid'] . ", " . $new['email'] . "=> $rc\n"; }
      }


      /* delete accounts not present in the LDAP directory */

      if(count($uids) > 0) {
         $uidlist = implode("','", $uids);
         $query = $this->db->query("SELECT uid, username FROM " . TABLE_USER . " WHERE domain='" . $this->db->escape($globals['domain']) . "' AND dn != '*' AND dn is NOT NULL AND uid NOT IN ('$uidlist')");

         foreach ($query->rows as $deleted) {
            $this->model_user_user->deleteUser($deleted['uid']);
            if($verbose) { print "deleted " . $deleted['username'] . ", uid: " . $deleted['uid'] . "\n"; }
         }
      }

      return $n;
   }


   private function createNewUserArray($dn = '', $username = '', $emails = '', $globals = array()) {
      $user = array();

      $user['uid'] = $this->model_user_user->getNextUid();
      $user['gid'] = $globals['gid'];

      $user['email'] = $emails;

      $user['username'] = $username . $user['uid'];
      $user['password'] = "aaaaaaa" . $user['username'];

      $user['realname'] = $username;

      $user['domain'] = $globals['domain'];
      $user['dn'] = $dn;
      $user['policy_group'] = $globals['policy_group'];
      $user['isadmin'] = 0;
      $user['whitelist'] = '';
      $user['blacklist'] = '';

      return $user;
   }


}

?>
