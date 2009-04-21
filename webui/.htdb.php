<?php

// mysql stuff

$dsn = "mysql://clapf:changeme@localhost/clapf";

$user_table = "user";
$misc_table = "t_misc";
$whitelist_table = "t_white_list";
$blacklist_table = "t_black_list";
$stat_table = "t_stat";
$policy_group_table = "t_policy";

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
