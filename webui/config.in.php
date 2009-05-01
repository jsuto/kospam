<?php

error_reporting(7);

$clapf_conf = "/usr/local/etc/clapf.conf";

$lang = "en";

/*
 * if webui is in the DocumentRoot, then let
 * the $directory variable have an empty value, ie.
 * $directory = "";
 */
$directory = "/webui";

/* possible values are: mysql, sqlite3, ldap */
$userdb = "mysql";

include_once($_SERVER['DOCUMENT_ROOT'] . "$directory/.htdb.php");
include_once($_SERVER['DOCUMENT_ROOT'] . "$directory/funcs.php");
include_once($_SERVER['DOCUMENT_ROOT'] . "$directory/webui.php");
include_once($_SERVER['DOCUMENT_ROOT'] . "$directory/lang/$lang/messages.php");

include_once($_SERVER['DOCUMENT_ROOT'] . "$directory/$userdb.php");

$queue_directory = "/var/lib/clapf/queue";

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


$admin_user = 0;

$min_password_len = 6;

?>
