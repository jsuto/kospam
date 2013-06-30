
<?php if(!isset($_SESSION['username'])){ ?>

<form class="form-signin" action="index.php?route=login/login" method="post">
   <input type="hidden" name="relocation" value="<?php if(isset($_GET['route']) && !preg_match("/^login/", $_GET['route']) ) { print $_SERVER['QUERY_STRING']; } ?>" />
   <input type="text" name="username" class="input-block-level" placeholder="<?php print $text_email; ?>">
   <input type="password" name="password" class="input-block-level" placeholder="<?php print $text_password; ?>">
   <button class="btn btn-large btn-primary" type="submit"><?php print $text_submit; ?></button>
</form>


<?php if(isset($x)){ ?><p class="text-error text-center"><?php print $x; ?></p><?php } ?>

<?php } ?>

