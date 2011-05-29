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

   <script type="text/javascript" src="view/javascript/quarantine.js"></script>
   <script type="text/javascript" src="view/javascript/jquery-1.4.2.min.js"></script>
   <script type="text/javascript" src="view/javascript/jquery.dropdownPlain.js"></script>

</head>

<body onload="loadHealth(); setInterval('loadHealth()', <?php print HEALTH_REFRESH; ?> * 1000)" style="cursor: wait">

   <script type="text/javascript" src="view/javascript/wz_tooltip.js"></script>
   <script type="text/javascript" src="view/javascript/tip_balloon.js"></script>

<script type="text/javascript">

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

function loadHealth() {

   if(xmlhttp != null) {
      xmlhttp.onreadystatechange = state_Change;
      xmlhttp.open("GET", "<?php print HEALTH_WORKER_URL; ?>", true);
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


</script>


<div id="wrap">

  <div id="menu">
    <ul id="menulist">

    <?php print $menu; ?>

    </ul>

  </div> <!-- menu -->

  <div id="content">

      <h3><?php print $title; ?></h3>

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
