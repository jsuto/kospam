<?php

class ModelDomainDomain extends Model {

   public function count_mydomains() {
      $query = $this->db->query("SELECT count(*) AS num FROM " . TABLE_DOMAIN);
      return $query->row['num'];
   }


   public function getDomains($s = '') {
      $data = array();

      if($s) {
         $query = $this->db->query("SELECT domain, mapped FROM " . TABLE_DOMAIN . " WHERE domain LIKE ? ORDER BY domain ASC", array('%' . $s . '%'));
      } else {
         $query = $this->db->query("SELECT domain, mapped FROM " . TABLE_DOMAIN . " ORDER BY domain ASC");
      }

      if(isset($query->rows)) {
         foreach($query->rows as $q) {

            $data[] = array('domain' => $q['domain'], 'mapped' => $q['mapped']);

         }
      }

      return $data;
   }


   public function get_mapped_domains() {
      $data = array();

      $query = $this->db->query("SELECT DISTINCT mapped FROM " . TABLE_DOMAIN . " ORDER BY mapped ASC");

      if(isset($query->rows)) {
         foreach($query->rows as $q) {
            array_push($data, $q['mapped']);
         }
      }

      return $data;
   }


   public function get_domains_by_string($s = '', $page = 0, $page_len = PAGE_LEN) {
      $from = (int)$page * (int)$page_len;

      if(strlen($s) < 1) { return array(); }

      $query = $this->db->query("SELECT domain FROM `" . TABLE_DOMAIN . "` WHERE domain LIKE ? ORDER BY domain ASC  LIMIT " . (int)$from . ", " . (int)$page_len, array($s . "%") );

      if(isset($query->rows)) { return $query->rows; }

      return array();
   }


   public function get_your_all_domains_by_email($email = '') {
      $data = array();

      if($email == '') { return $data; }

      $a = explode("@", $email);

      $query = $this->db->query("SELECT domain FROM " . TABLE_DOMAIN . " WHERE mapped IN (SELECT mapped FROM " . TABLE_DOMAIN . " WHERE domain=?)", array($a[1]));

      if(isset($query->rows)) {
         foreach ($query->rows as $q) {
            array_push($data, $q['domain']);
         }
      }

      return $data;
   }


   public function deleteDomain($domain = '') {
      if($domain == "") { return 0; }

      $query = $this->db->query("DELETE FROM " . TABLE_DOMAIN . " WHERE domain=?", array($domain));

      $rc = $this->db->countAffected();

      LOGGER("remove domain: $domain (rc=$rc)");

      return $rc;
   }


   public function addDomain($domain = '', $mapped = '', $ldap_id = 0) {
      if($domain == "" || $mapped == "") { return 0; }

      $mapped = strtolower($mapped);

      $query = $this->db->query("INSERT INTO " . TABLE_DOMAIN . " (domain, mapped) VALUES (?,?)", array($mapped, $mapped));
      $rc = $this->db->countAffected();
      if($rc == 1) {
         LOGGER("add domain: $domain (rc=$rc)");
      }

      $domains = explode("\n", $domain);

      foreach ($domains as $domain) {
         $domain = strtolower(rtrim($domain));
         if($domain != $mapped) {
            $query = $this->db->query("INSERT INTO " . TABLE_DOMAIN . " (domain, mapped) VALUES (?,?)", array($domain, $mapped));
            $rc = $this->db->countAffected();

            LOGGER("add domain: $domain (rc=$rc)");
         }
      }

      return $rc;
   }


}

?>
