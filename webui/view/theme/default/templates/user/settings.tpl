
<?php if(!isset($x)){ ?>

<form action="index.php?route=user/settings" method="post" name="setpagelen" class="formbottom">

   <div id="search">

          <h4><?php print $text_display_settings; ?></h4>

      <div id="search1">

         <div class="row">
            <div class="logincell"><?php print $text_page_length; ?>:</div>
            <div class="tcell">
               <select name="pagelen">
                  <option value="10"<?php if($page_len == 10) { ?> selected="selected"<?php } ?>>10
                  <option value="20"<?php if($page_len == 20) { ?> selected="selected"<?php } ?>>20
                  <option value="30"<?php if($page_len == 30) { ?> selected="selected"<?php } ?>>30
                  <option value="50"<?php if($page_len == 50) { ?> selected="selected"<?php } ?>>50
               </select>
            </div>
         </div>

         <div class="row">
            <div class="logincell"><?php print $text_language; ?>:</div>
            <div class="tcell">
               <select name="lang">
                  <!--option value=""><?php print $text_use_browser_settings; ?></option-->
            <?php foreach(Registry::get('langs') as $t) { ?>
                  <option value="<?php print $t; ?>"<?php if(isset($_SESSION['lang']) && $_SESSION['lang'] == $t) { ?> selected="selected"<?php } ?>><?php print $t; ?></option>
            <?php } ?>
               </select>
            </div>
         </div>



<?php if(Registry::get('admin_user') == 1) { ?>
         <div class="row">
            <div class="logincell"><?php print $text_admin_user_trains_global_database; ?>:</div>
            <div class="tcell">
               <select name="global_train">
                  <option value="0"<?php if(isset($_SESSION['train_global']) && $_SESSION['train_global'] == 0) { ?>selected="selected""<?php } ?>><?php print $text_off; ?></option>
                  <option value="1"<?php if(isset($_SESSION['train_global']) && $_SESSION['train_global'] == 1) { ?>selected="selected""<?php } ?>><?php print $text_on; ?></option>
               </select>
            </div>
         </div>
<?php } ?>


         <div class="row">
            <div class="logincell">&nbsp;</div>
            <div class="tcell"><input type="submit" value="<?php print $text_set; ?>" class="btn btn-primary" /> <input type="reset" class="btn" value="<?php print $text_cancel; ?>" /></div>
         </div>


      </div>

   </div>

</form>


<p>&nbsp;</p>


<?php if(PASSWORD_CHANGE_ENABLED == 1) { ?>
      <form method="post" name="pwdchange" action="index.php?route=user/settings" class="formbottom">
         <div class="row">
            <div class="logincell"><?php print $text_password; ?>:</div>
            <div class="tcell"><input type="password" class="text" name="password" /></div>
         </div>
         <div class="row">
            <div class="logincell"><?php print $text_password_again; ?>:</div>
            <div class="tcell"><input type="password" class="text" name="password2" /></div>
         </div>
         <div class="row">
            <div class="logincell">&nbsp;</div>
            <div class="tcell"><input type="submit" class="btn btn-primary" value="<?php print $text_submit; ?>" /> <input type="reset" class="btn" value="<?php print $text_cancel; ?>" /></div>
         </div>
   </form>
<?php } ?>



<?php } else { ?>
<?php print $x; ?>. <a href="index.php?route=user/settings"><?php print $text_back; ?></a>
<?php } ?>


