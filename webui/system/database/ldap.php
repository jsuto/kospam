<?php


class LDAP {

   private $link;

   public function __construct($ldaphost, $binddn, $bindpw) {

      $this->link = ldap_connect($ldaphost) or exit('Error: ldap_connect()');
      ldap_set_option($this->link, LDAP_OPT_PROTOCOL_VERSION, 3);

      ldap_bind($this->link, $binddn, $bindpw) or exit('Error: ldap_bind()');

      return $this->link;
   }


   public function query($basedn, $filter, $justthese) {
      $i = 0;
      $data = array();

      $sr = ldap_search($this->link, $basedn, $filter, $justthese);

      $results = ldap_get_entries($this->link, $sr);

      for($i=0; $i < $results['count']; $i++) {
         for($k=0; $k < $results[$i]['count']; $k++) {
            $attr = $results[$i][$k];

            if($results[$i][$attr]['count'] == 1) {
               $data[$i][$attr] = isset($results[$i][$attr][0]) ? $results[$i][$attr][0] : "";
            }
            else {
               $data[$i][$attr] = isset($results[$i][$attr]) ? $results[$i][$attr] : "";
            }

         }

      }


      $query = new stdClass();

      $query->row      = isset($data[0]) ? $data[0] : array();
      $query->dn       = isset($results[0]['dn']) ? $results[0]['dn'] : "";
      $query->rows     = $data;
      $query->num_rows = $results['count'];

      unset($data);

      return $query;
   }


   public function add($dn, $entry) {
      return ldap_add($this->link, $dn, $entry);
   }


   public function modify($dn, $entry) {
      return ldap_modify($this->link, $dn, $entry);
   }


   public function replace($dn, $entry) {
      return ldap_mod_replace($this->link, $dn, $entry);
   }


   public function delete($dn) {
      return ldap_delete($this->link, $dn);
   }


   public function __destruct() {
      ldap_unbind($this->link);
   }


}


?>
