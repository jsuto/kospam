<?php


class ModelUserImport extends Model {


   public function queryRemoteUsers($host) {
      $data = array();
      $n_users = 0;

      $ldap = new LDAP($host['ldap_host'], $host['ldap_binddn'], $host['ldap_bindpw']);
      if($ldap->is_bind_ok() == 0) { return 0; }

      $query = $ldap->query($host['ldap_basedn'], "proxyAddresses=*", array("cn", "proxyAddresses") );

      foreach ($query->rows as $result) {

         if(isset($result['proxyaddresses']) && isset($result['dn']) ) {

            $emails = "";

            if(is_array($result['proxyaddresses']) ) {
               for($i = 0; $i < $result['proxyaddresses']['count']; $i++) {
                  $emails .= preg_replace("/smtp\:/i", "", $result['proxyaddresses'][$i]) . "\n";
               }

               $emails = rtrim($emails);
            }


            $data[] = array(
                          'username'     => $result['cn'],
                          'dn'           => $result['dn'],
                          'emails'       => $emails
                          );

            $n_users++;

         }

      }


      /* if the 't_remote' table has no entry for your domain and we read some users
         let's put the connection info to the 't_remote' table needed for proxying
         the authentication requests */

      if($n_users > 0) {
         $my_domain = $this->model_user_user->getDomains();
         $query = $this->db->query("SELECT COUNT(*) AS num FROM " . TABLE_REMOTE . " WHERE remotedomain='" . $this->db->escape($my_domain[0]) . "'");

         if(isset($query->row['num']) && $query->row['num'] == 0) {
            $query = $this->db->query("INSERT INTO " . TABLE_REMOTE . " (remotedomain, remotehost, basedn) VALUES('" . $this->db->escape($my_domain[0]) . "', '" . $this->db->escape($host['ldap_host']) . "', '" . $this->db->escape($host['ldap_binddn']) . "')");
         }

      }

      return $data;
   }


   public function importUsers($data) {
      $added = 0;

      $this->load->model('user/user');

      /* build a list of DNs to exclude from the import */

      while (list($k, $v) = each($data)) {
         if(preg_match("/^reject_/", $k)) {
            $exclude[$v] = 1;
         }
      }

      /* query the domain of the domain admin user from 'user' table */

      $my_domain = $this->model_user_user->getDomains();

      $old_list = $this->getDNsByDomain($my_domain[0]);

      $ldap = new LDAP($data['ldap_host'], $data['ldap_binddn'], $data['ldap_bindpw']);
      if($ldap->is_bind_ok() == 0) { return $added; }

      $query = $ldap->query($data['ldap_basedn'], "proxyAddresses=*", array("cn", "proxyAddresses") );

      $new_list = array();

      foreach ($query->rows as $result) {
          if(isset($result['proxyaddresses']) && isset($result['dn']) && !isset($exclude[$result['dn']]) ) {

              $new_list[$result['dn']]['new'] = 1;
              $new_list[$result['dn']]['uid'] = -1;
              $new_list[$result['dn']]['cn'] = $result['cn'];
              $new_list[$result['dn']]['dn'] = $result['dn'];
              $new_list[$result['dn']]['emails'] = $result['proxyaddresses'];
          }

      }


      /* remove accounts not existing on the AD server */

      $saved_uids = array();

      foreach ($old_list as $user) {
         $dn = $user['dn'];
         if(!isset($new_list[$dn]['new'])) {
            if($user['dn'] != '*' && $user['dn'] != '' && $this->model_user_user->deleteUser($user['uid']) == 1) {
               array_push($saved_uids, $user['uid']);
            }
         }
         else {
            $new_list[$dn]['new'] = 0;
            $new_list[$dn]['uid'] = $user['uid'];
         }
      }



      $domains = $this->model_user_user->getEmailDomains();

      foreach ($new_list as $result) {

         $emails = "";

         if(is_array($result['emails']) ) {
            for($i = 0; $i < $result['emails']['count']; $i++) {
               $__email = preg_replace("/smtp\:/i", "", $result['emails'][$i]);

               if(Registry::get('admin_user') == 1 || checkemail($__email, $domains) == 1) {
                  $emails .= $__email . "\n";
               }
            }
         }

         $emails = rtrim($emails);



         /* add new user ... */

         if($result['new'] == 1) {

            $entry = array();

            /* grab an unused uid */

            $entry['uid'] = array_pop($saved_uids);
            if($entry['uid'] == "") { $entry['uid'] = $this->model_user_user->getNextUid(); }

            $entry['username'] = $result['cn'] . '@' . $data['domain'];
            $entry['password'] = md5(microtime(true), TRUE);
            $entry['domain'] = $data['domain'];
            $entry['dn'] = $result['dn'];
            $entry['policy_group'] = (int)$data['policy_group'];
            $entry['isadmin'] = 0;
            $entry['email'] = $emails;
            $entry['whitelist'] = '';
            $entry['blacklist'] = '';

            if($this->model_user_user->addUser($entry) == 1) {
               $added++;
            }
         }

         /* ... or update user's email addresses */

         else {

            $query = $this->db->query("DELETE FROM " . TABLE_EMAIL . " WHERE uid=" . (int)$result['uid']);

            $__emails = explode("\n", $emails);
            foreach ($__emails as $email) {
               $email = rtrim($email);

               $query = $this->db->query("SELECT COUNT(*) AS count FROM " . TABLE_EMAIL . " WHERE email='" . $this->db->escape($email) . "'");

               if($query->row['count'] == 0) {
                  $query = $this->db->query("INSERT INTO " . TABLE_EMAIL . " (uid, email) VALUES(" . (int)$result['uid'] . ", '" . $this->db->escape($email) . "')");
               }

            }
            
         }

      }

      return $added;
   }


   private function getDNsByDomain($domain = '') {
      if($domain == ""){ return array(); }

      $query = $this->db->query("SELECT dn, uid FROM " . TABLE_USER . " WHERE domain='" . $this->db->escape($domain) . "' AND dn != ''");

      return $query->rows;
   }


}

?>
