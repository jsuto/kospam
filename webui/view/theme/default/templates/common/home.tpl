
<?php if(!isset($x)){ ?>

<form action="index.php?route=common/home" method="get" name="setpagelen" onSubmit="fix_form(); return false; ">

<table>
   <tr>
      <td><?php print $text_page_length; ?>:</td>
      <td>
         <select name="pagelen">
            <option value="10"<?php if($page_len == 10) { ?> selected="selected"<?php } ?>>10
            <option value="20"<?php if($page_len == 20) { ?> selected="selected"<?php } ?>>20
            <option value="30"<?php if($page_len == 30) { ?> selected="selected"<?php } ?>>30
            <option value="50"<?php if($page_len == 50) { ?> selected="selected"<?php } ?>>50
         </select>
      </td>
   </tr>
   <tr>
      <td><?php print $text_language; ?>:</td>
      <td>
         <select name="lang">
            <option value="en"<?php if(isset($_SESSION['lang']) && $_SESSION['lang'] == "en") { ?> selected="selected"<?php } ?>>en</option>
            <option value="hu"<?php if(isset($_SESSION['lang']) && $_SESSION['lang'] == "hu") { ?> selected="selected"<?php } ?>>hu</option>
         </select>
      </td>
   </tr>
   <tr>
      <td>&nbsp;</td>
      <td><input type="submit" value="<?php print $text_set; ?>" /></td>
   </tr>
</table>

</form>

<p>&nbsp;</p>


<form name="pwdchange" action="index.php?route=common/home" method="post">
   <table border="0" cellpadding="0" cellspacing="0">
      <tr><td><?php print $text_password; ?>: </td><td><input type="password" name="password" /></td></tr>
      <tr><td><?php print $text_password_again; ?>: </td><td><input type="password" name="password2" /></td></tr>
     <tr><td>&nbsp;</td><td><input type="submit" value="<?php print $text_submit; ?>" /> <input type="reset" value="<?php print $text_cancel; ?>" /></td></tr>
   </table>
</form>


<?php } else { ?>
<?php print $x; ?>. <a href="index.php?route=common/home"><?php print $text_back; ?></a>
<?php } ?>


