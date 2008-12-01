<?

include_once("config.php");
include_once("header.inc");

$username = $_SERVER['REMOTE_USER'];
if($username == "") nice_error($err_not_authenticated);

?>


  <h3><? print $HOME; ?></h3>

  <div id="body">

   <table border="0" cellspacing="0" cellpadding="0">
    <tr valign="top">
      <td>

<p>

<? print "$YOU_ARE: $username"; ?>

</p>


      </td>
      <td>
      </td>
    </tr>
   </table>


  </div> <!-- body -->




<? include_once("footer.inc"); ?>
