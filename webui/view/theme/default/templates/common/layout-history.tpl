<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="hu" lang="hu">

<head>
   <title>clapf web UI | <?php print $title; ?></title>
   <meta http-equiv="content-type" content="text/html; charset=utf-8" />
   <meta http-equiv="Content-Language" content="en" />
   <meta name="keywords" content="clapf, webui, web ui, gui, spam, anti-spam, filtering, service" />
   <meta name="description" content="clapf, webui, web ui, gui, spam, anti-spam, filtering, service" />
   <meta name="rating" content="general" />
   <meta name="robots" content="all" />

   <meta name="viewport" content="width=device-width, initial-scale=1.0">

   <link href="bootstrap/css/bootstrap.min.css" rel="stylesheet" media="screen">
   <link href="bootstrap/css/bootstrap-responsive.min.css" rel="stylesheet" media="screen">

   <link rel="stylesheet" type="text/css" href="css/jquery-ui-custom.min.css" />
   <link rel="stylesheet" type="text/css" href="view/theme/<?php print THEME; ?>/stylesheet/style-<?php print THEME; ?>.css" />

   <script type="text/javascript" src="view/javascript/jquery.min.js"></script>
   <script type="text/javascript" src="view/javascript/jquery-ui-custom.min.js"></script>
   <script type="text/javascript" src="view/javascript/bootstrap.min.js"></script>
   <script type="text/javascript" src="view/javascript/rc-splitter.js"></script>
   <script type="text/javascript" src="view/javascript/clapf.js"></script>

</head>

<body class="mybody"<?php if(isset($this->request->get['route']) && $this->request->get['route'] == 'history/history') { ?> onload="Clapf.load_history(); setInterval('Clapf.load_history()', Clapf.history_refresh * 1000);"<?php } ?>>

<div id="clapf1" class="container">

   <div id="menu">
      <?php print $menu; ?>
   </div>

   <div id="mainscreen" class="up">

      <?php if($title) { ?><h3 class="title"><?php print $title; ?></h3><?php } ?>

      <?php print $content; ?>

   </div> <!-- main -->

   <div id="footer"><?php print $footer; ?></div>


</div>

</body>
</html>
