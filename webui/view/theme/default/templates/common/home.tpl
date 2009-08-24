<p>
<?php if(isset($_SESSION['username'])){
   print $text_you_are; ?>: <?php print $_SESSION['username'];
} ?>
</p>

<?php if(!isset($x)){ ?>

<form action="index.php?route=common/home" method="get" name="setpagelen" onSubmit="fix_form(); return false; ">
<?php print $text_page_length; ?>:
   <select name="pagelen">
      <option value="10">10
      <option value="20">20
      <option value="30">30
      <option value="50">50
   </select>
   <input type="submit" value="<?php print $text_set; ?>" />
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


