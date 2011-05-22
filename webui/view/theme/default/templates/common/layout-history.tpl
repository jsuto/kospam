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
   <link rel="stylesheet" type="text/css" href="view/theme/default/stylesheet/dropdown.css" />
   <link rel="stylesheet" type="text/css" href="view/theme/default/stylesheet/jquery.ui.all.css" />

   <script type="text/javascript" src="view/javascript/quarantine.js"></script>
   <script type="text/javascript" src="view/javascript/jquery-1.4.2.min.js"></script>
   <script type="text/javascript" src="view/javascript/jquery-ui-1.8.13.custom.min.js"></script>
   <script type="text/javascript" src="view/javascript/jquery-ui-i18n.js"></script>
   <script type="text/javascript" src="view/javascript/jquery.dropdownPlain.js"></script>

</head>

<body onload="loadHistory('<?php print HISTORY_WORKER_URL; ?><?php if(isset($_GET['page']) && $_GET['page'] > 0) { ?>&page=<?php print $_GET['page']; } ?>'); <?php if(!isset($_GET['page']) || $_GET['page'] == 0) { ?>setInterval('checkHistory()', <?php print HISTORY_REFRESH; ?> * 1000)<?php } ?>" style="cursor: wait">

   <script type="text/javascript" src="view/javascript/wz_tooltip.js"></script>
   <script type="text/javascript" src="view/javascript/tip_balloon.js"></script>

<script type="text/javascript">

function readCookie(name) {
        var nameEQ = name + "=";
        var ca = document.cookie.split(';');
        for(var i=0;i < ca.length;i++) {
                var c = ca[i];
                while (c.charAt(0)==' ') c = c.substring(1,c.length);
                if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length,c.length);
        }
        return null;
}


function getXMLHttp() {
   var XMLHttp = null;

   if(window.XMLHttpRequest) {
      try {
         XMLHttp = new XMLHttpRequest();
      } catch(e) { }
   }
   else if(window.ActiveXObject) {
      try {
         XMLHttp = new ActiveXObject("Msxml2.XMLHTTP");
      } catch (e) {
         try {
            XMLHttp = new ActiveXObject("Microsoft.XMLHTTP");
         } catch(e) { }
      }
   }

   return XMLHttp;
}


var xmlhttp = getXMLHttp();

function loadHistory(url) {

   if(xmlhttp != null) {
      xmlhttp.onreadystatechange = state_Change;
      xmlhttp.open("GET", url, true);
      xmlhttp.send(null);

      document.body.style.cursor = 'default';
   }

}


function state_Change() {
   if(xmlhttp.readyState == 4) {
      if(xmlhttp.status == 200) {
         document.getElementById('A1').innerHTML = xmlhttp.responseText;
      }
      else {
         alert("Problem retrieving XML data:" + xmlhttp.statusText);
      }
   }
}


function checkHistory() {

   if(xmlhttp != null) {
      xmlhttp.onreadystatechange = f_helper;
      xmlhttp.open("GET", '<?php print HISTORY_HELPER_URL; ?>', true);
      xmlhttp.send(null);
   }

}


function f_helper() {
   if(xmlhttp.readyState == 4){
      if(xmlhttp.status == 200) {
         var a = readCookie('lastupdate');
         if(xmlhttp.responseText > a) {
            loadHistory('<?php print HISTORY_WORKER_URL; ?>');
         }
      }
   }
}


$(document).ready(function() {
   $.datepicker.setDefaults($.datepicker.regional['<?php if(LANG == 'en') { ?>en-GB<?php } else { print LANG; } ?>']);
   $("#date1").datepicker( { dateFormat: 'yy-mm-dd' } );
   $("#date2").datepicker( { dateFormat: 'yy-mm-dd' } );
  });

</script>


<div id="wrap">

  <div id="menu">
    <ul id="menulist">

    <?php print $menu; ?>

    </ul>

  </div> <!-- menu -->

  <div id="content">

      <h3><?php print $title; ?></h3>

      <?php if(isset($_SESSION['username']) && !strstr($_SERVER['QUERY_STRING'], "login/logout") ){ ?><p><?php print $text_you_are; ?>: <?php print $_SESSION['username']; ?></p><?php } ?>

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
