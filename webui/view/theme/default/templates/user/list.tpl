
<p/>

<p><a href="index.php?route=user/add"><?php print $text_add_new_user_alias; ?></a></p>

<h4><?php print $text_existing_users; ?></h4>

<form method="post" name="search1" action="index.php?route=user/list">
   <input type="text" name="search" value="<?php print $search; ?>" />
   <input type="submit" value="<?php print $text_search; ?>" />
</form>


<p>&nbsp;</p>

<?php if(isset($users)){ ?>

<form method="post" name="massedit" action="index.php?route=user/massedit">

<table border="1">
   <tr align="center">
      <th>&nbsp;</th>
      <th><?php print $text_user_id; ?></th>
      <th><?php print $text_username; ?></th>
      <th><?php print $text_email; ?></th>
      <th><?php print $text_domain; ?></th>
      <th><?php print $text_policy_group; ?></th>
      <th>&nbsp;</th>
      <th>&nbsp;</th>
   </tr>

<?php foreach($users as $user) { ?>
   <tr align="left">
      <td><input type="checkbox" name="aa_<?php print $user['uid']; ?>" /></td>
      <td><?php print $user['uid']; ?></td>
      <td><?php print $user['username']; ?></td>
      <td><?php print $user['email']; ?></td>
      <td><?php print $user['domain']; ?></td>
      <td><?php print $user['policy_group']; ?></td>
      <td><a href="index.php?route=user/edit&amp;uid=<?php print $user['uid']; ?>"><?php print $text_edit_or_view; ?></a></td>
      <td><a href="index.php?route=quarantine/quarantine&amp;user=<?php print $user['username']; ?>"><?php print $text_view_user_quarantine; ?></a></td>
   </tr>
<?php } ?>

</table>

<?php if($total_users > $page_len){ ?>
<p>
<?php if($page > 0){ ?>
   <a href="index.php?route=user/list&amp;page=0&amp;search=<?php print $search; ?>"><?php print $text_first; ?></a>
   <a href="index.php?route=user/list&amp;page=<?php print $prev_page; ?>&amp;search=<?php print $search; ?>"><?php print $text_previous; ?></a>
<?php } ?>

<?php if($total_users >= $page_len*($page+1) && $total_users > $page_len){ ?>
   <a href="index.php?route=user/list&amp;page=<?php print $next_page; ?>&amp;search=<?php print $search; ?>"><?php print $text_next; ?></a>
<?php } ?>

<?php if($page < $total_pages){ ?>
   <a href="index.php?route=user/list&amp;page=<?php print $total_pages; ?>&amp;search=<?php print $search; ?>"><?php print $text_last; ?></a>
<?php } ?>
</p>
<?php } ?>


<input type="submit" value="<?php print $text_bulk_edit_selected_uids; ?>" /></form>

<?php } else { ?>
<?php print $text_not_found; ?>
<?php } ?>


