<?php

include_once("config.php");

session_start();
$username = get_authenticated_username();

include_once("header.php");

if($username == "") show_auth_popup();

?>


  <h3><?php print $HOME; ?></h3>

  <div id="body">

   <table border="0" cellspacing="0" cellpadding="0">
    <tr valign="top">
      <td>

<p>

<?php print "$YOU_ARE: $username"; ?>

</p>


      </td>
      <td>
      </td>
    </tr>
   </table>


  </div> <!-- body -->




<?php include_once("footer.php"); ?>
