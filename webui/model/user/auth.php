<?php

class ModelUserAuth extends Model {

   public function checkLogin($username = '', $password = '') {
      $session = Registry::get('session');
      $ok = 0;

      if($username == '' || $password == '') { return 0; }

      if(ENABLE_LDAP_AUTH == 1) {
         $ok = $this->checkLoginAgainstLDAP($username, $password);
         if($ok == 1) { return $ok; }
      }

      if(ENABLE_IMAP_AUTH == 1) {
         require 'Zend/Mail/Protocol/Imap.php';
         $ok = $this->checkLoginAgainstIMAP($username, $password);

         if($ok == 1) {
            if(CUSTOM_EMAIL_QUERY_FUNCTION && function_exists(CUSTOM_EMAIL_QUERY_FUNCTION)) {
               call_user_func(CUSTOM_EMAIL_QUERY_FUNCTION, $username);
            }

            return $ok;
         }
      }

      if(ENABLE_POP3_AUTH == 1) {
         require 'Zend/Mail/Protocol/Pop3.php';
         $ok = $this->checkLoginAgainstPOP3($username, $password);

         if($ok == 1) {
            if(CUSTOM_EMAIL_QUERY_FUNCTION && function_exists(CUSTOM_EMAIL_QUERY_FUNCTION)) {
               call_user_func(CUSTOM_EMAIL_QUERY_FUNCTION, $username);
            }

            return $ok;
         }
      }

      // fallback local auth

      $query = $this->db->query("SELECT u.username, u.uid, u.realname, u.dn, u.password, u.isadmin, u.domain FROM " . TABLE_USER . " u, " . TABLE_EMAIL . " e WHERE e.email=? AND e.uid=u.uid", array($username));

      if(!isset($query->row['password'])) { return 0; }

      $pass = crypt($password, $query->row['password']);

      if($pass == $query->row['password']){
         $ok = 1;

         AUDIT(ACTION_LOGIN, $username, '', '', 'successful auth against user table');
      }
      else {
         AUDIT(ACTION_LOGIN_FAILED, $username, '', '', 'failed auth against user table');
      }

      if($ok == 0 && strlen($query->row['dn']) > 3) {
         $ok = $this->checkLoginAgainstFallbackLDAP($query->row, $password);
      }


      if($ok == 1) {
         $session->set("username", $username);
         $session->set("uid", $query->row['uid']);
         $session->set("admin_user", $query->row['isadmin']);
         $session->set("email", $username);
         $session->set("domain", $query->row['domain']);
         $session->set("realname", $query->row['realname']);

         $session->set("emails", $this->model_user_user->get_users_all_email_addresses($query->row['uid']));

         $this->is_ga_code_needed();

         return 1;
      }

      return 0;
   }


   private function checkLoginAgainstLDAP($username = '', $password = '') {
      $a = array();
      $ret = 0;

      if(ENABLE_SAAS == 1) {
         $params = $this->model_saas_ldap->get_ldap_params_by_email($username);
         foreach($params as $param) {
            $ret = $this->checkLoginAgainstLDAP_real($username, $password, $param);

            syslog(LOG_INFO, "ldap auth result against " . $param['ldap_host'] . " / " . $param['ldap_type'] . ": $ret");

            if($ret == 1) { return $ret; }
         }
      }
      else {
         $ret = $this->checkLoginAgainstLDAP_real($username, $password);
      }

      return $ret;
   }


   private function checkLoginAgainstLDAP_real($username = '', $password = '', $a = array()) {

      $ldap_type = '';
      $ldap_host = LDAP_HOST;
      $ldap_base_dn = LDAP_BASE_DN;
      $ldap_helper_dn = LDAP_HELPER_DN;
      $ldap_helper_password = LDAP_HELPER_PASSWORD;
      $ldap_auditor_member_dn = LDAP_AUDITOR_MEMBER_DN;
      $ldap_admin_member_dn = LDAP_ADMIN_MEMBER_DN;

      $role = 0;
      $username_prefix = '';

      if(count($a) >= 6) {
         $ldap_type = $a['ldap_type'];
         $ldap_host = $a['ldap_host'];
         $ldap_base_dn = $a['ldap_base_dn'];
         $ldap_helper_dn = $a['ldap_bind_dn'];
         $ldap_helper_password = $a['ldap_bind_pw'];
         $ldap_auditor_member_dn = $a['ldap_auditor_member_dn'];

         $ldap_mail_attr = $a['ldap_mail_attr'];
         $ldap_account_objectclass = $a['ldap_account_objectclass'];
         $ldap_distributionlist_attr = $a['ldap_distributionlist_attr'];
         $ldap_distributionlist_objectclass = $a['ldap_distributionlist_objectclass'];
      }

      if($ldap_type != LDAP_TYPE_GENERIC) {
         list($ldap_mail_attr, $ldap_account_objectclass, $ldap_distributionlist_attr, $ldap_distributionlist_objectclass) = get_ldap_attribute_names($ldap_type);
      }

      if($ldap_mail_attr == 'proxyAddresses') { $username_prefix = 'smtp:'; }

      if($ldap_host == '' || $ldap_helper_password == '') { return 0; }

      $ldap = new LDAP($ldap_host, $ldap_helper_dn, $ldap_helper_password);

      if($ldap->is_bind_ok()) {

         $query = $ldap->query($ldap_base_dn, "(&(objectClass=$ldap_account_objectclass)($ldap_mail_attr=$username_prefix$username))", array());

         if(isset($query->row['dn']) && $query->row['dn']) {
            $a = $query->row;

            $ldap_auth = new LDAP($ldap_host, $a['dn'], $password);

            if(ENABLE_SYSLOG == 1) { syslog(LOG_INFO, "ldap auth against '" . $ldap_host . "', dn: '" . $a['dn'] . "', result: " . $ldap_auth->is_bind_ok()); }

            if($ldap_auth->is_bind_ok()) {

               $a['dn'] = stripslashes($a['dn']);
               $a['dn'] = preg_replace("/\(/", '\(', $a['dn']);
               $a['dn'] = preg_replace("/\)/", '\)', $a['dn']);

               $query = $ldap->query($ldap_base_dn, "(|(&(objectClass=$ldap_account_objectclass)($ldap_mail_attr=$username_prefix$username))(&(objectClass=$ldap_distributionlist_objectclass)($ldap_distributionlist_attr=$username_prefix$username)" . ")(&(objectClass=$ldap_distributionlist_objectclass)($ldap_distributionlist_attr=" . $a['dn'] . ")))", array());

               if($this->check_ldap_membership($ldap_auditor_member_dn, $query->rows) == 1) { $role = 2; }
               if($this->check_ldap_membership($ldap_admin_member_dn, $query->rows) == 1) { $role = 1; }

               $emails = $this->get_email_array_from_ldap_attr($query->rows);

               $extra_emails = $this->model_user_user->get_email_addresses_from_groups($emails);
               $emails = array_merge($emails, $extra_emails);

               $this->add_session_vars($a['cn'], $username, $emails, $role);

               AUDIT(ACTION_LOGIN, $username, '', '', 'successful auth against LDAP');

               return 1;
            }
            else {
               AUDIT(ACTION_LOGIN_FAILED, $username, '', '', 'failed auth against LDAP');
            }
         }
      }
      else if(ENABLE_SYSLOG == 1) {
         syslog(LOG_INFO, "cannot bind to '" . $ldap_host . "' as '" . $ldap_helper_dn . "'");
      }

      return 0;
   }


   private function check_ldap_membership($ldap_auditor_member_dn = '', $e = array()) {
      if($ldap_auditor_member_dn == '') { return 0; }

      foreach($e as $a) {
         foreach (array("memberof", "dn") as $memberattr) {
            if(isset($a[$memberattr])) {

               if(isset($a[$memberattr]['count']) && $a[$memberattr]['count'] > 0) {

                  for($i = 0; $i < $a[$memberattr]['count']; $i++) {
                     if($a[$memberattr][$i] == $ldap_auditor_member_dn) {
                        return 1;
                     }
                  }
               }
               else {
                  if($a[$memberattr] == $ldap_auditor_member_dn) {
                     return 1;
                  }
               }
            }
         }
      }

      return 0;
   }


   public function get_email_array_from_ldap_attr($e = array()) {
      $data = array();

      foreach($e as $a) {
         //syslog(LOG_INFO, "checking ldap entry dn: " . $a['dn'] . ", cn: " . $a['cn']);

         foreach (array("mail", "mailalternateaddress", "proxyaddresses", "zimbraMailForwardingAddress", "member", "memberOfGroup") as $mailattr) {
            if(isset($a[$mailattr])) {

               if(is_array($a[$mailattr])) {
                  for($i = 0; $i < $a[$mailattr]['count']; $i++) {

                     //syslog(LOG_INFO, "checking entry: " . $a[$mailattr][$i]);

                     $a[$mailattr][$i] = strtolower($a[$mailattr][$i]);

                     if(strchr($a[$mailattr][$i], '@')) {

                        if(preg_match("/^([\w]+)\:/i", $a[$mailattr][$i], $p)) {
                           if(isset($p[0]) && $p[0] != "smtp:") { continue; }
                        }

                        $email = preg_replace("/^([\w]+)\:/i", "", $a[$mailattr][$i]);
                        if(validemail($email) && !in_array($email, $data)) { array_push($data, $email); }
                     }
                  }
               }
               else {
                  //syslog(LOG_INFO, "checking entry #2: " . $a[$mailattr]);

                  $email = strtolower(preg_replace("/^([\w]+)\:/i", "", $a[$mailattr]));
                  if(validemail($email) && !in_array($email, $data)) { array_push($data, $email); }
               }
            }
         }
      }

      return $data;
   }


   private function add_session_vars($name = '', $email = '', $emails = array(), $role = 0) {
      $session = Registry::get('session');

      $a = explode("@", $email);

      $uid = $this->model_user_user->get_uid_by_email($email);
      if($uid < 1) {
         $uid = $this->model_user_user->get_next_uid(TABLE_EMAIL);
         $query = $this->db->query("INSERT INTO " . TABLE_EMAIL . " (uid, email) VALUES(?,?)", array($uid, $email));
      }


      $session->set("username", $email);
      $session->set("uid", $uid);

      if($role > 0) {
         $session->set("admin_user", $role);
      } else {
         $session->set("admin_user", 0);
      }

      $session->set("email", $email);
      $session->set("domain", $a[1]);
      $session->set("realname", $name);

      $session->set("emails", $emails);

      $this->is_ga_code_needed();
   }


   private function checkLoginAgainstFallbackLDAP($user = array(), $password = '') {
      if($password == '' || !isset($user['username']) || !isset($user['domain']) || !isset($user['dn']) || strlen($user['domain']) < 2){ return 0; }

      $query = $this->db->query("SELECT remotehost, basedn FROM " . TABLE_REMOTE . " WHERE remotedomain=?", array($user['domain']));

      if($query->num_rows != 1) { return 0; }

      $ldap = new LDAP($query->row['remotehost'], $user['dn'], $password);

      if($ldap->is_bind_ok()) {
         $this->change_password($user['username'], $password);

         AUDIT(ACTION_LOGIN, $user['username'], '', '', 'changed password in local table');

         return 1;
      }
      else {
         AUDIT(ACTION_LOGIN_FAILED, $user['username'], '', '', 'failed bind to ' . $query->row['remotehost'], $user['dn']);
      }

      return 0; 
   }


   private function checkLoginAgainstIMAP($username = '', $password = '') {
      $session = Registry::get('session');
      $emails = array($username);

      if(!strchr($username, '@')) { return 0; }

      $imap = new Zend_Mail_Protocol_Imap(IMAP_HOST, IMAP_PORT, IMAP_SSL);
      if($imap->login($username, $password)) {
         $imap->logout();

         $extra_emails = $this->model_user_user->get_email_addresses_from_groups($emails);
         $emails = array_merge($emails, $extra_emails);

         $this->add_session_vars($username, $username, $emails, 0);

         $session->set("password", $password);

         return 1;
      }

      return 0;
   }


   private function checkLoginAgainstPOP3($username = '', $password = '') {
      $rc = 0;
      $emails = array($username);

      try {
         $conn = new Zend_Mail_Protocol_Pop3(POP3_HOST, POP3_PORT, POP3_SSL);

         if($conn) {
            $s = $conn->connect(POP3_HOST);

            if($s) {

               try {
                  $conn->login($username, $password);

                  $extra_emails = $this->model_user_user->get_email_addresses_from_groups($emails);
                  $emails = array_merge($emails, $extra_emails);

                  $this->add_session_vars($username, $username, $emails, 0);
                  $rc = 1;
               }
               catch (Zend_Mail_Protocol_Exception $e) {}
            }
         }
      }
      catch (Zend_Mail_Protocol_Exception $e) {}

      return $rc;
   }


   public function check_ntlm_auth() {
      $ldap_auditor_member_dn = LDAP_AUDITOR_MEMBER_DN;
      $ldap_admin_member_dn = LDAP_ADMIN_MEMBER_DN;

      $role = 0;

      if(!isset($_SERVER['REMOTE_USER']) || $_SERVER['REMOTE_USER'] == '') { return 0; }

      $u = explode("\\", $_SERVER['REMOTE_USER']);

      if(isset($u[1])) { $username = $u[1]; }
      else { $username = $_SERVER['REMOTE_USER']; }

      if(ENABLE_SYSLOG == 1) { syslog(LOG_INFO, "sso login: $username"); }

      $ldap = new LDAP(LDAP_HOST, LDAP_HELPER_DN, LDAP_HELPER_PASSWORD);

      if($ldap->is_bind_ok()) {

         $query = $ldap->query(LDAP_BASE_DN, "(&(objectClass=user)(samaccountname=" . $username . "))", array());

         if(isset($query->row['dn'])) {
            $a = $query->row;

            if(isset($a['mail']['count'])) { $username = $a['mail'][0]; } else { $username = $a['mail']; }
            $username = strtolower(preg_replace("/^smtp\:/i", "", $username));

            if($username == '') {
               syslog(LOG_INFO, "no email address found for " . $a['dn']);
               return 0;
            }

            $ldap_mail_attr = LDAP_MAIL_ATTR;
            if(LDAP_MAIL_ATTR == 'proxyAddresses') { $ldap_mail_attr = 'proxyAddresses=smtp:'; }

            $query = $ldap->query(LDAP_BASE_DN, "(|(&(objectClass=user)(" . $ldap_mail_attr . "=$username))(&(objectClass=group)(member=$username))(&(objectClass=group)(member=" . stripslashes($a['dn']) . ")))", array());


            $emails = $this->get_email_array_from_ldap_attr($query->rows);

            $extra_emails = $this->model_user_user->get_email_addresses_from_groups($emails);
            $emails = array_merge($emails, $extra_emails);

            if(!in_array($username, $emails)) { array_push($emails, $username); }

            if($this->check_ldap_membership($ldap_auditor_member_dn, $query->rows) == 1) { $role = 2; }
            if($this->check_ldap_membership($ldap_admin_member_dn, $query->rows) == 1) { $role = 1; }

            $this->add_session_vars($a['cn'], $username, $emails, $role);

            $this->model_user_prefs->get_user_preferences($username);

            AUDIT(ACTION_LOGIN, $username, '', '', 'successful auth against LDAP');

            return 1;
         }

      }

      return 0; 
   }


   public function get_failed_login_count() {
      $session = Registry::get('session');

      $n = $session->get('failed_logins');
      if($n == '') { $n = 0; }

      return $n;
   }


   public function increment_failed_login_count($n = 0) {
      $session = Registry::get('session');

      $n = $session->get('failed_logins') + 1;
      $session->set('failed_logins', $n);
   }


   private function is_ga_code_needed() {
      $session = Registry::get('session');

      $query = $this->db->query("SELECT ga_enabled FROM " . TABLE_USER_SETTINGS . " WHERE username=?", array($session->get("username")));

      if(isset($query->row['ga_enabled']) && $query->row['ga_enabled'] == 1) {
         $session->set("ga_block", 1);
      }
   }


   public function change_password($username = '', $password = '') {
      if($username == "" || $password == ""){ return 0; }

      $query = $this->db->query("UPDATE " . TABLE_USER . " SET password=? WHERE username=?", array(crypt($password), $username));

      $rc = $this->db->countAffected();

      return $rc;
   }

}

?>
