
<h4><?php print $text_add_new_domain; ?></h4>

<form method="post" name="add1" action="index.php?route=domain/domain">
   <table border="0">
      <tr>
         <td><?php print $text_domain; ?>: </td><td><input type="text" name="domain" /></td>
      </tr>
      <tr>
         <td><?php print $text_mapped_domain; ?>: </td><td><input type="text" name="mapped" /></td>
      </tr>
      <tr>
         <td>&nbsp;</td><td><input type="submit" value="<?php print $text_add; ?>" /> <input type="reset" value="<?php print $text_cancel; ?>" /></td>
      </tr>
   </table>
</form>


<h4><?php print $text_existing_domains; ?></h4>

<!--form method="post" name="search1" action="index.php?route=domain/domain">
   <input type="text" name="search" value="<?php print $search; ?>" />
   <input type="submit" value="<?php print $text_search; ?>" />
</form>

<p>&nbsp;</p-->

<?php if(isset($domains)){ ?>

<table border="1">
   <tr align="center">
      <th><?php print $text_domain; ?></th>
      <th><?php print $text_mapped_domain; ?></th>
      <th>&nbsp;</th>
   </tr>

<?php foreach($domains as $domain) { ?>
   <tr align="left">
      <td><?php print $domain['domain']; ?></td>
      <td><?php print $domain['mapped']; ?></td>
      <td><a href="index.php?route=domain/remove&amp;domain=<?php print urlencode($domain['domain']); ?>"><?php print $text_remove; ?></a></td>
   </tr>
<?php } ?>

</table>

<?php if($total > $page_len){ ?>
<p>
<?php if($page > 0){ ?>
   <a href="index.php?route=domain/domain&amp;page=0&amp;search=<?php print $search; ?>"><?php print $text_first; ?></a>
   <a href="index.php?route=domain/domain&amp;page=<?php print $prev_page; ?>&amp;search=<?php print $search; ?>"><?php print $text_previous; ?></a>
<?php } ?>

<?php if($total_users >= $page_len*($page+1) && $total_users > $page_len){ ?>
   <a href="index.php?route=domain/domain&amp;page=<?php print $next_page; ?>&amp;search=<?php print $search; ?>"><?php print $text_next; ?></a>
<?php } ?>

<?php if($page < $total_pages){ ?>
   <a href="index.php?route=domain/domain&amp;page=<?php print $total_pages; ?>&amp;search=<?php print $search; ?>"><?php print $text_last; ?></a>
<?php } ?>
</p>
<?php } ?>


<?php } else { ?>
<?php print $text_not_found; ?>
<?php } ?>


