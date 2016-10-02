
<form action="<?php $formurl; ?>" name="adduser" method="post" autocomplete="off" class="form-horizontal">
   <input type="hidden" name="type" value="<?php print $type; ?>" />

        <div class="control-group<?php if(isset($errors['xxx'])){ print " error"; } ?>">
            <label class="control-label" for="gid"><?php print $text_settings; ?>:</label>
            <div class="controls">
              <textarea style="height:280px; width:300px;" id="list" name="list"><?php print $list; ?></textarea>
              <?php if ( isset($errors['gid']) ) { ?><span class="help-inline"><?php print $errors['xxx']; ?></span><?php } ?>
            </div>
        </div>

        <div class="form-actions">
        <input type="submit" value="<?php print $text_modify; ?>" class="btn btn-primary" /> <a href="index.php?route=user/wbl" class="btn"><?php print $text_cancel; ?></a>
        </div>

</form>

