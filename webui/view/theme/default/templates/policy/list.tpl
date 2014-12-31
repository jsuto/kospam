<div id="deleteconfirm-modal" class="modal hide fade">
  <div class="modal-header">
    <button type="button" class="close" data-dismiss="modal" role="dialog" aria-hidden="true"><i class="icon-remove"></i></button>
    <h3><?php print $text_confirm; ?> <?php print $text_delete; ?></h3>
  </div>
  <div class="modal-body">
    <p><?php print $text_delete_confirm_message; ?> <span id="name">ERROR</span>?</p>
  </div>
  <div class="modal-footer">
    <a href="#" class="btn" data-dismiss="modal" aria-hidden="true"><?php print $text_close; ?></a>
    <a href="index.php?route=policy/remove&amp;id=-1&amp;name=Error&amp;confirmed=0" class="btn btn-primary" id="id"><?php print $text_delete; ?></a>
  </div>
</div>

<form method="post" name="search1" action="index.php?route=policy/list" class="form-inline pull-right">
    <div class="input-append">
        <input type="text" name="search" class="input-medium" value="<?php print $search; ?>" />
        <input type="submit" class="btn" value="<?php print $text_search; ?>" />
    </div>
</form>

<p><a href="index.php?route=policy/add"><i class="icon-plus"></i>&nbsp;<?php print $text_add_policy; ?></a></p>

<h4><?php print $text_existing_policies; ?></h4>

<?php if(isset($policies) && is_array($policies)){ ?>

   <table class="table table-striped table-condensed" id="ss1">
      <tr class="domainrow">
         <th><?php print $text_policy; ?></th>
         <th>&nbsp;</th>
         <th>&nbsp;</th>
      </tr>

<?php foreach($policies as $policy) { ?>
      <tr class="domainrow">
         <td><?php print $policy['name']; ?></td>
         <td><a href="index.php?route=policy/edit&amp;id=<?php print $policy['id']; ?>"><i class="icon-edit"></i>&nbsp;<?php print $text_edit_or_view; ?></a></td>
         <td><a href="index.php?route=policy/remove&amp;id=<?php print $policy['id']; ?>&amp;name=<?php print $policy['name']; ?>" class="confirm-delete" data-id="<?php print $policy['id']; ?>" data-name="<?php print $policy['name']; ?>"><i class="icon-remove-sign"></i>&nbsp;<?php print $text_remove; ?></a></td>
      </tr>
<?php } ?>

   </table>


<?php } else { ?>
<div class="alert alert-error lead">
<?php print $text_not_found; ?>
</div>
<?php } ?>


