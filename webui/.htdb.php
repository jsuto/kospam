<?

// mysql stuff

$host = "localhost";
$u = "your_db_user";
$p = "your_db_password";
$db = "your_database";

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
