<?php

class ModelDomainDomain extends Model {

   public function getDomains() {
      $query = $this->db->query("SELECT domain, mapped FROM " . TABLE_DOMAIN . " ORDER BY domain ASC");

      return $query->rows;
   }


   public function deleteDomain($domain = '') {
      if($domain == "") { return 0; }

      $query = $this->db->query("DELETE FROM " . TABLE_DOMAIN . " WHERE domain='" . $this->db->escape($domain) . "'");

      return $this->db->countAffected();
   }


   public function addDomain($domain = '', $mapped = '') {
      if($domain == "" || $mapped == "") { return 0; }

      $query = $this->db->query("INSERT INTO " . TABLE_DOMAIN . " (domain, mapped) VALUES ('" . $this->db->escape($domain) . "', '" . $this->db->escape($mapped) . "')");

      return $this->db->countAffected();
   }


}

?>
