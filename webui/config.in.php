<?

error_reporting(7);

$clapf_conf = "/usr/local/etc/clapf.conf";

$lang = "en";

/*
 * if webui is in the DocumentRoot, then let
 * the $directory variable have an empty value, ie.
 * $directory = "";
 */
$directory = "/webui";

include_once($_SERVER['DOCUMENT_ROOT'] . "$directory/.htdb.php");
include_once($_SERVER['DOCUMENT_ROOT'] . "$directory/funcs.php");
include_once($_SERVER['DOCUMENT_ROOT'] . "$directory/lang/$lang/messages.inc");

include_once($_SERVER['DOCUMENT_ROOT'] . "$directory/mysql.php");
//include_once($_SERVER['DOCUMENT_ROOT'] . "$directory/ldap.php");

$queue_directory = "/var/lib/clapf/queue";

$user_table = "user";
$misc_table = "t_misc";
$whitelist_table = "t_white_list";
$stat_table = "t_stat";
$policy_group_table = "t_policy";

$page_len = 25;
$max_cgi_from_subj_len = 45;

$clapf_header_field = "X-Clapf-spamicity: ";

// smtp stuff

$smtphost = "127.0.0.1";
$clapfport = 10025;
$smtpport = 10026;
$yourdomain = "xxxxx.com";
$ham_train_address = "ham@$yourdomain";
$fromaddr = "noreply@$yourdomain";


$base_url = "http://" . $_SERVER['SERVER_NAME'] . $directory;

// list of admin users

$ADMIN['admin'] = 1;

$admin_user = 0;

?>
