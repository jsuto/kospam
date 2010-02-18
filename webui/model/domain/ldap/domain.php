<?php

class ModelDomainDomain extends Model {

   public function getDomains() {
      $data = array();

      $query = $this->db->ldap_query(LDAP_DOMAIN_BASEDN, "maildomain=*", array("maildomain", "mapped") );

      foreach ($query->rows as $result) {

         $data[] = array(
                         'domain'  => $result['maildomain'],
                         'mapped'  => $result['mapped'],
                        );
      }


      return $data;
   }


   public function deleteDomain($domain = '') {
      if($domain == "") { return 0; }

      if($this->db->ldap_delete("maildomain=$domain," . LDAP_DOMAIN_BASEDN) == TRUE) {
         return 1;
      }

      return 0;
   }


   public function addDomain($domain = '', $mapped = '') {
      if($domain == "" || $mapped == "") { return 0; }

      $a = array();

      $a[0] = "top";
      $a[1] = "clapfdomain";

      $domains = explode("\n", $domain);

      foreach ($domains as $domain) {
         $domain = rtrim($domain);

         $entry = array();

         $entry["objectClass"] = $a;

         $entry["maildomain"] = $domain;
         $entry["mapped"] = $mapped;

         if($this->db->ldap_add("maildomain=" . $domain . "," .  LDAP_DOMAIN_BASEDN, $entry) != TRUE) {
            return 0;
         }

      }

      return 1;
   }


}

?>
