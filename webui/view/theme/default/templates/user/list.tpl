
<p/>

<p><a href="index.php?route=user/add"><?php print $text_add_new_user_alias; ?></a></p>

<!--h4><?php print $text_existing_users; ?></h4-->

<form method="post" name="search1" action="index.php?route=user/list" class="form-search">
   <span><?php print $text_existing_users; ?>:</span>
   <input type="text" name="search" value="<?php print $search; ?>" />
   <input type="submit" class="btn btn-primary" value="<?php print $text_search; ?>" />
</form>


<p>&nbsp;</p>

<?php if(isset($users)){ ?>

<form method="post" class="form-inline" name="massedit" action="index.php?route=user/massedit">

<div id="pagenav" style="margin-bottom: 20px;">
   <?php if($page > 0){ ?><a href="index.php?route=user/list&amp;page=0&amp;search=<?php print $search; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &laquo; <?php if($page > 0){ ?></a><?php } ?>
   <?php if($page > 0){ ?><a href="index.php?route=user/list&amp;page=<?php print $prev_page; ?>&amp;search=<?php print $search; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &lsaquo; <?php if($page > 0){ ?></a><?php } ?>
   <?php print $users[0][$sort]; ?> - <?php print $users[count($users)-1][$sort]; ?>
   <?php if($total_users >= $page_len*($page+1) && $total_users > $page_len){ ?><a href="index.php?route=user/list&amp;page=<?php print $next_page; ?>&amp;search=<?php print $search; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &rsaquo; <?php if($total_users >= $page_len*($page+1) && $total_users > $page_len){ ?></a><?php } ?>
   <?php if($page < $total_pages){ ?><a href="index.php?route=user/list&amp;page=<?php print $total_pages; ?>&amp;search=<?php print $search; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &raquo; <?php if($page < $total_pages){ ?></a><?php } ?>
</div>


   <table class="table table-bordered table-striped table-hover">
    <thead>
     <tr>
      <th>&nbsp;</th>
      <th><?php print $text_realname; ?> <a href="index.php?route=user/list&amp;sort=realname&amp;order=0"><i class="icon-chevron-up"></i></a> <a href="index.php?route=user/list&amp;sort=realname&amp;order=1"><i class="icon-chevron-down"></i></a></th>
      <th><?php print $text_email; ?> <a href="index.php?route=user/list&amp;sort=email&amp;order=0"><i class="icon-chevron-up"></i></a> <a href="index.php?route=user/list&amp;sort=email&amp;order=1"><i class="icon-chevron-down"></i></a></th>
      <th><?php print $text_role; ?> <a href="index.php?route=user/list&amp;sort=domain&amp;order=0"><i class="icon-chevron-up"></i></a> <a href="index.php?route=user/list&amp;sort=domain&amp;order=1"><i class="icon-chevron-down"></i></a></th>
      <th>&nbsp;</th>
      <th>&nbsp;</th>
      <th>&nbsp;</th>
     </tr>
    </thead>

    <tbody>

<?php foreach($users as $user) { ?>
    <tr>
      <td><input type="checkbox" name="aa_<?php print $user['uid']; ?>" /></td>
      <td><?php print $user['realname']; ?></td>
      <td><?php if($user['email'] != $user['shortemail']){ ?><span title="<?php print $user['email']; ?>"><?php print $user['shortemail']; ?></span><?php } else { print $user['email']; } ?></td>
      <td><?php
         if($user['isadmin'] == 0){ print $text_user_regular; }
         if($user['isadmin'] == 1){ print $text_user_masteradmin; }
         if($user['isadmin'] == 2){ print $text_user_domainadmin; }
         if($user['isadmin'] == 3){ print $text_user_read_only_admin; }
      ?></td>
      <td><a href="index.php?route=user/edit&amp;uid=<?php print $user['uid']; ?>"><?php print $text_edit_or_view; ?></a></td>
      <td><a href="index.php?route=quarantine/quarantine&amp;user=<?php print $user['username']; ?>&amp;hamspam=SPAM"><?php print $text_quarantine; ?></a></td>
      <td><a href="index.php?route=stat/stat&amp;uid=<?php print $user['uid']; ?>"><?php print $text_statistics; ?></a></td>
    </tr>
<?php } ?>
    </tbody>
   </table>

<div id="pagenav">
   <?php if($page > 0){ ?><a href="index.php?route=user/list&amp;page=0&amp;search=<?php print $search; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &laquo; <?php if($page > 0){ ?></a><?php } ?>
   <?php if($page > 0){ ?><a href="index.php?route=user/list&amp;page=<?php print $prev_page; ?>&amp;search=<?php print $search; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &lsaquo; <?php if($page > 0){ ?></a><?php } ?>
   <?php print $users[0][$sort]; ?> - <?php print $users[count($users)-1][$sort]; ?>
   <?php if($total_users >= $page_len*($page+1) && $total_users > $page_len){ ?><a href="index.php?route=user/list&amp;page=<?php print $next_page; ?>&amp;search=<?php print $search; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &rsaquo; <?php if($total_users >= $page_len*($page+1) && $total_users > $page_len){ ?></a><?php } ?>
   <?php if($page < $total_pages){ ?><a href="index.php?route=user/list&amp;page=<?php print $total_pages; ?>&amp;search=<?php print $search; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &raquo; <?php if($page < $total_pages){ ?></a><?php } ?>
</div>

<p>&nbsp;</p>

<input type="submit" class="btn btn-ok" value="<?php print $text_bulk_edit_selected_uids; ?>" />

</form>

<?php } else { ?>
<?php print $text_not_found; ?>
<?php } ?>


