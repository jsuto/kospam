<?php

$title = "";

$w = explode("/", $_SERVER['PHP_SELF']);

$title = $TITLE['/' . array_pop($w) ];

if($ADMIN[$username] == 1) $admin_user = 1;

?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="hu" lang="hu">

<head>
    <title><?php print $title; ?></title>
    <meta http-equiv="content-type" content="text/html; charset=iso-8859-2" />
    <meta name="keywords" content="spam, anti-spam, email, e-mail, mail, unsolicited commercial bulk email, blacklist, software, filtering, service, Bayes, bayesian, bayes-i, minden a spamről" />
    <meta  name="description" content="spam, anti-spam, antispam, pop3, gateway, proxy, gw" />
    <meta name="rating" content="general" />
    <meta name="robots" content="all" />
    <link rel="stylesheet" type="text/css" href="spamtelenul.css" media="all" />
</head>

<body>


<div id="wrap">

  <div id="menu">
    <ul id="menulist">

          <li><a href="<?php print $base_url; ?>/index.php"<?php if(strstr($_SERVER['PHP_SELF'], "/index.php")) print " id=\"active\""; ?>><?php print $HOME; ?></a></li>
          <li><a href="<?php print $base_url; ?>/q.php"<?php if(strstr($_SERVER['PHP_SELF'], "/q.php")) print ", id=\"active\""; ?>><?php print $QUARANTINE; ?></a></li>
          <li><a href="<?php print $base_url; ?>/whitelist.php"<?php if(strstr($_SERVER['PHP_SELF'], "/whitelist.php")) print " id=\"active\""; ?>><?php print $WHITELIST; ?></a></li>
          <li><a href="<?php print $base_url; ?>/blacklist.php"<?php if(strstr($_SERVER['PHP_SELF'], "/blacklist.php")) print " id=\"active\""; ?>><?php print $BLACKLIST; ?></a></li>
          <li><a href="<?php print $base_url; ?>/stat.php"<?php if(strstr($_SERVER['PHP_SELF'], "/stat.php")) print " id=\"active\""; ?>><?php print $STATS; ?></a></li>
<?php
   if($admin_user == 1){

      print "<li><a href=\"$base_url/users.php\""; if(strstr($_SERVER['PHP_SELF'], "/users.php")) print " id=\"active\""; print ">$USER_MANAGEMENT</a></li>\n";
      print "<li><a href=\"$base_url/policy.php\""; if(strstr($_SERVER['PHP_SELF'], "/policy.php")) print " id=\"active\""; print ">$POLICY</a></li>\n";
   }
?>
          <li><a href="<?php print $base_url; ?>/logout.php"<?php if(strstr($_SERVER['PHP_SELF'], "/logout.php")) print " id=\"active\""; ?>><?php print $LOGOUT; ?></a></li>

    </ul>

  </div>

  <div id="content">


