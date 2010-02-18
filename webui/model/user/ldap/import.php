<?php


class ModelUserImport extends Model {

   public function queryRemoteUsers($host) {
      $data = array();

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
         }

      }

      return $data;
   }


   public function importUsers($data) {
      $added = 0;
      $a = array();

      $a[0] = "top";
      $a[1] = "person";
      $a[2] = "qmailUser";
      $a[3] = "qmailGroup";


      $ldap = new LDAP($data['ldap_host'], $data['ldap_binddn'], $data['ldap_bindpw']);
      if($ldap->is_bind_ok() == 0) { return $added; }

      $query = $ldap->query($data['ldap_basedn'], "proxyAddresses=*", array("cn", "proxyAddresses") );

      foreach ($query->rows as $result) {

         if(isset($result['proxyaddresses']) && isset($result['dn']) ) {

            $email = $result['proxyaddresses'][0];
            $email = preg_replace("/smtp\:/i", "", $email);

            $emails = array();

            if(is_array($result['proxyaddresses']) ) {
               for($i = 1; $i < $result['proxyaddresses']['count']; $i++) {
                  array_push($emails, preg_replace("/smtp\:/i", "", $result['proxyaddresses'][$i]));
               }
            }

            $entry = array();

            $entry["objectClass"] = $a;

            $entry["cn"] = $result['cn'] . '@' . $data['domain'];
            $entry["sn"] = "x";
            $entry["uid"] = $this->getNextUid();

            $entry["mail"] = $email;
            $entry["policygroupid"] = (int)$data['policy_group'];
            $entry["isadmin"] = 0;
            $entry["userPassword"] = "{MD5}" . base64_encode(md5(microtime(true), TRUE));

            $entry["mailalternateaddress"] = $emails;

            $entry["filtersender"] = '';
            $entry["blacklist"] = '';

            $entry["proxydn"] = $result['dn'];
            $entry["domain"] = $data['domain'];

            $entry["mailMessageStore"] = "";

            if($this->db->ldap_add("cn=" . $entry['cn'] . "," .  LDAP_USER_BASEDN, $entry) == TRUE) {
               $added++;
            }

         }
      }

      return $added;
   }


   private function getNextUid() {
      $x = 0;

      $query = $this->db->ldap_query(LDAP_USER_BASEDN, "uid=*", array("uid") );

      foreach ($query->rows as $result) {
         if($result['uid'] > $x){
            $x = $result['uid'];
         }
      }

      return $x + 1;
   }

}

?>
