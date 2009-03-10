<?php

include_once("config.php");

session_start();
$username = get_authenticated_username();

include_once("header.php");

if($username == "") show_auth_popup();

if(isset($_GET['pagelen'])){
   Header("Set-Cookie: pagelen=" . $_GET['pagelen'] . "; path=/");
}

?>


  <h3><?php print $HOME; ?></h3>

  <div id="body">

   <table border="0" cellspacing="0" cellpadding="0">
    <tr valign="top">
      <td>

<p>

<?php print "$YOU_ARE: $username"; ?>

</p>


<p>
<form action="index.php" method="get">
<?php print $PAGE_LENGTH; ?>: <select name="pagelen">
   <option value="10">10
   <option value="20">20
   <option value="30">30
   <option value="50">50
</select>
<input type="submit" value="<?php print $SET; ?>" />
</form>

</p>

      </td>
      <td>
      </td>
    </tr>
   </table>


  </div> <!-- body -->




<?php include_once("footer.php"); ?>
