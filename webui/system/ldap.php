<?php


class DB {
   private $driver;


   public function __construct($driver, $ldaphost, $binddn, $bindpw) {
      if (!@require_once(DIR_DATABASE . $driver . '.php')) {
         exit('Error: Could not load database file ' . $driver . '!');
      }

      $this->driver = new $driver($ldaphost, $binddn, $bindpw);
   }


   /***** ldap related stuff ***********/

   public function ldap_query($basedn, $filter, $justthese) {
      return $this->driver->query($basedn, $filter, $justthese);
   }


   public function ldap_add($dn, $entry) {
      return $this->driver->add($dn, $entry);
   }


   public function ldap_modify($dn, $entry) {
      return $this->driver->modify($dn, $entry);
   }


   public function ldap_replace($dn, $entry) {
      return $this->driver->replace($dn, $entry);
   }


   public function ldap_delete($dn) {
      return $this->driver->delete($dn);
   }


}


$db = new DB(DB_DRIVER, LDAP_HOST, LDAP_BINDDN, LDAP_BINDPW);
Registry::set('db', $db);


?>
