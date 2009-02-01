<?php

// mysql stuff

$dsn = "mysql://clapf:changeme@localhost/clapf";


// ldap stuff

$ldaphost = "ldap://127.0.0.1/";
$basedn = "dc=aaaa,dc=fu";
$binddn = "";
$bindpw = "";

$user_base_dn = "ou=clapfusers,$basedn";
$policy_base_dn = "ou=clapfpolicies,$basedn";

// sqlite3 stuff

$sqlite3_db = "/path/to/tokens.sdb";

?>
