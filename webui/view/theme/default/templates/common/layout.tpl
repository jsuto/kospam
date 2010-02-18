<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="hu" lang="hu">

<head>
   <title>clapf web UI | <?php print $title; ?></title>
   <meta http-equiv="content-type" content="text/html; charset=utf-8" />
   <meta http-equiv="Content-Language" content="en" />
   <meta name="keywords" content="clapf, webui, web ui, spam, anti-spam, email, e-mail, mail, unsolicited commercial bulk email, blacklist, software, filtering, service, Bayes, Bayesian" />
   <meta name="description" content="clapf, webui, web ui, spam, anti-spam, antispam" />
   <meta name="rating" content="general" />
   <meta name="robots" content="all" />

   <link rel="stylesheet" type="text/css" href="view/theme/default/stylesheet/style.css" />

   <script type="text/javascript" src="view/javascript/quarantine.js"></script>

</head>

<body>

   <script type="text/javascript" src="view/javascript/wz_tooltip.js"></script>
   <script type="text/javascript" src="view/javascript/tip_balloon.js"></script>

<div id="wrap">

  <div id="menu">
    <ul id="menulist">

    <?php print $menu; ?>

    </ul>

  </div> <!-- menu -->

  <div id="content">

      <h3><?php print $title; ?></h3>

      <?php if(isset($_SESSION['username']) && !strstr($_SERVER['QUERY_STRING'], "login/logout") ){ ?>
<p><?php print $text_you_are; ?>: <?php print $_SESSION['username']; ?>
<?php if(strstr($_SERVER['QUERY_STRING'], "quarantine/quarantine") && isset($_GET['user']) && $_GET['user'] != $_SESSION['username']) { ?>&nbsp; (<?php print $text_quarantine; ?>: <?php print $_GET['user']; ?>)<?php } ?></p>
      <?php } ?>

      <div id="body">

<?php print $content; ?>

      </div> <!-- body -->

   </div> <!-- content -->


   <div id="footer">
<?php print $footer; ?>
   </div>


</div> <!-- wrap -->

</body>
</html>
