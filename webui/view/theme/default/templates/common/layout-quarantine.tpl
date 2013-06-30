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

<body class="mybody" onload="Clapf.add_shortcuts();">

<div id="messagebox1"></div>

<div id="clapf1" class="container">

<div id="menu">
   <?php print $menu; ?>
</div>


<div id="expertsearch">

   <input type="hidden" name="user" value="<?php print $username; ?>" />
   <input type="text" name="search" id="search" class="input-medium search-query span8" placeholder="Enter search terms" />

   <a id="advsearch_caret" href="#" onclick="$('#searchpopup1').show();"><b class="caret"></b></a>

   <button id="button_search" class="btn btn-danger" onclick="Clapf.expert(this); return false;"><?php print $text_submit; ?></button>
   <button class="btn" onclick="Clapf.cancel(); return false;"><?php print $text_cancel; ?></button>

   <!--button id="button_expert" class="btn " onclick="$('#searchpopup1').show(); return false;">Advanced search &nbsp;<span class="caret"></span></button-->
   <button class="btn " onclick="Clapf.saved_search_terms('<?php print $text_saved; ?>'); return false;"><?php print $text_save; ?></button>
   <button class="btn btn-inverse" onclick="Clapf.load_saved_search_terms(); return false;"><?php print $text_load; ?></button>

   <?php print $popup; ?>

   <div id="sspinner">
      <div class="progress progress-striped active">
         <div class="bar"></div>
      </div>
   </div>


</div>



<div id="mainscreen">
  <div id="mailleftcontainer">



  </div>

  <div id="mailrightcontainernofolder">

    <div id="mailrightcontent">

      <div id="mailcontframe">

        <div id="messagelistcontainer" class="boxlistcontent"> 

<?php print $content; ?>

        </div>
      </div>


<script type="text/javascript">
  var mailviewsplit = new rcube_splitter({id:'splitter2', p1: 'mailcontframe', p2: 'mailpreviewframe', orientation: 'h', relative: true, start: 205});
  split.add_onload('mailviewsplit.init()');
</script>

      <div id="mailpreviewframe">
      </div>


  </div>



</div>


</div> <!-- clapf1 -->


<script type="text/javascript">
$(document).ready(function(){
   split.init();
   $("#button_search").click();
});
</script>



</body>
</html>
