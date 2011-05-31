
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

<div id="pagenav">
   <?php if($page > 0){ ?><a href="index.php?route=user/list&amp;page=0&amp;search=<?php print $search; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &laquo; <?php if($page > 0){ ?></a><?php } ?>
   <?php if($page > 0){ ?><a href="index.php?route=user/list&amp;page=<?php print $prev_page; ?>&amp;search=<?php print $search; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &lsaquo; <?php if($page > 0){ ?></a><?php } ?>
   <?php print $users[0][$sort]; ?> - <?php print $users[count($users)-1][$sort]; ?>
   <?php if($total_users >= $page_len*($page+1) && $total_users > $page_len){ ?><a href="index.php?route=user/list&amp;page=<?php print $next_page; ?>&amp;search=<?php print $search; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &rsaquo; <?php if($total_users >= $page_len*($page+1) && $total_users > $page_len){ ?></a><?php } ?>
   <?php if($page < $total_pages){ ?><a href="index.php?route=user/list&amp;page=<?php print $total_pages; ?>&amp;search=<?php print $search; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &raquo; <?php if($page < $total_pages){ ?></a><?php } ?>
</div>

<table id="user-list-table" border="1">
   <tr align="center">
      <th>&nbsp;</th>
      <th><?php print $text_user_id; ?> <a href="index.php?route=user/list&amp;sort=uid&amp;order=0"><img src="view/theme/<?php print THEME; ?>/images/arrowup.gif" border="0"></a> <a href="index.php?route=user/list&amp;sort=uid&amp;order=1"><img src="view/theme/<?php print THEME; ?>/images/arrowdown.gif" border="0"></a></th>
      <th><?php print $text_username; ?> <a href="index.php?route=user/list&amp;sort=username&amp;order=0"><img src="view/theme/<?php print THEME; ?>/images/arrowup.gif" border="0"></a> <a href="index.php?route=user/list&amp;sort=username&amp;order=1"><img src="view/theme/<?php print THEME; ?>/images/arrowdown.gif" border="0"></a></th>
      <th><?php print $text_realname; ?></th>
      <th><?php print $text_email; ?></th>
      <th><?php print $text_domain; ?> <a href="index.php?route=user/list&amp;sort=domain&amp;order=0"><img src="view/theme/<?php print THEME; ?>/images/arrowup.gif" border="0"></a> <a href="index.php?route=user/list&amp;sort=domain&amp;order=1"><img src="view/theme/<?php print THEME; ?>/images/arrowdown.gif" border="0"></a></th>
      <th><?php print $text_policy_group; ?> <a href="index.php?route=user/list&amp;sort=policy&amp;order=0"><img src="view/theme/<?php print THEME; ?>/images/arrowup.gif" border="0"></a> <a href="index.php?route=user/list&amp;sort=policy&amp;order=1"><img src="view/theme/<?php print THEME; ?>/images/arrowdown.gif" border="0"></a></th>
      <th>&nbsp;</th>
      <th>&nbsp;</th>
      <th>&nbsp;</th>
   </tr>

<?php foreach($users as $user) { ?>
   <tr align="left">
      <td><input type="checkbox" name="aa_<?php print $user['uid']; ?>" /></td>
      <td><?php print $user['uid']; ?></td>
      <td><?php print $user['username']; ?></td>
      <td class="realname"><?php print $user['realname']; ?></td>
      <td><?php if($user['email'] != $user['shortemail']){ ?><span onmouseover="Tip('<?php print $user['email']; ?>', BALLOON, true, ABOVE, true)" onmouseout="UnTip()"><?php print $user['shortemail']; ?></span><?php } else { print $user['email']; } ?></td>
      <td><?php print $user['domain']; ?></td>
      <td><?php print $user['policy_group']; ?></td>
      <td><a href="index.php?route=user/edit&amp;uid=<?php print $user['uid']; ?>"><?php print $text_edit_or_view; ?></a></td>
      <td><a href="index.php?route=quarantine/quarantine&amp;user=<?php print $user['username']; ?>&amp;hamspam=SPAM"><?php print $text_quarantine; ?></a></td>
      <td><a href="index.php?route=stat/stat&amp;uid=<?php print $user['uid']; ?>"><?php print $text_statistics; ?></a></td>
   </tr>
<?php } ?>

</table>

<div id="pagenav">
   <?php if($page > 0){ ?><a href="index.php?route=user/list&amp;page=0&amp;search=<?php print $search; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &laquo; <?php if($page > 0){ ?></a><?php } ?>
   <?php if($page > 0){ ?><a href="index.php?route=user/list&amp;page=<?php print $prev_page; ?>&amp;search=<?php print $search; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &lsaquo; <?php if($page > 0){ ?></a><?php } ?>
   <?php print $users[0][$sort]; ?> - <?php print $users[count($users)-1][$sort]; ?>
   <?php if($total_users >= $page_len*($page+1) && $total_users > $page_len){ ?><a href="index.php?route=user/list&amp;page=<?php print $next_page; ?>&amp;search=<?php print $search; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &rsaquo; <?php if($total_users >= $page_len*($page+1) && $total_users > $page_len){ ?></a><?php } ?>
   <?php if($page < $total_pages){ ?><a href="index.php?route=user/list&amp;page=<?php print $total_pages; ?>&amp;search=<?php print $search; ?>&amp;sort=<?php print $sort; ?>&amp;order=<?php print $order; ?>" class="navlink"><?php } ?> &raquo; <?php if($page < $total_pages){ ?></a><?php } ?>
</div>


<input type="submit" value="<?php print $text_bulk_edit_selected_uids; ?>" /></form>

<?php } else { ?>
<?php print $text_not_found; ?>
<?php } ?>


