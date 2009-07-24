
<p/>

<?php if($username && $uid > 0) { ?>

<h4><?php print $text_add_new_email_address; ?></h4>

<p>
<form action="index.php?route=user/email" name="addemail" method="post">
   <input type="hidden" name="uid" value="<?php print $uid; ?>">
   <table border="0">
      <tr><td><?php print $text_username; ?>:</td><td><?php print $username; ?></td></tr>
      <tr><td><?php print $text_email; ?>:</td><td><input type="text" name="email" value=""></td></tr>
      <tr colspan="2"><td><input type="submit" value="<?php print $text_add; ?>"></td></tr>
   </table>
</form>
</p>


<p>
<?php } else if(isset($x)){ print $x; ?>. <a href="index.php?route=user/list"><?php print $text_back; ?></a>
<?php } ?>

</p>
