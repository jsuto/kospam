<p>

<?php if(isset($policy)) { ?>
<form action="index.php?route=policy/add" method="post">
   <input type="hidden" name="policy_group" value="<?php print $policy['policy_group']; ?>">

   <table>
      <tr><td><?php print $text_policy_name; ?></td><td><b><input type="text" name="name" value="<?php print $policy['name']; ?>" size="30"></b></td><td>&nbsp;</td></tr>
      <tr><td>deliver_infected_email</td><td><input name="deliver_infected_email" value="<?php print $policy['deliver_infected_email']; ?>" size="3"></td><td>0|1</td></tr>
      <tr><td>silently_discard_infected_email</td><td><input name="silently_discard_infected_email" value="<?php print $policy['silently_discard_infected_email']; ?>" size="3"></td><td>0|1</td></tr>
      <tr><td>use_antispam</td><td><input name="use_antispam" value="<?php print $policy['use_antispam']; ?>" size="3"></td><td>0|1</td></tr>

      <tr><td>spam_subject_prefix</td><td><input name="spam_subject_prefix" value="<?php print $policy['spam_subject_prefix']; ?>" size="30"></td><td>[string]</td></tr>
      <tr><td>enable_auto_white_list</td><td><input name="enable_auto_white_list" value="<?php print $policy['enable_auto_white_list']; ?>" size="1"></td><td>0|1</td></tr>
      <tr><td>max_message_size_to_filter</td><td><input name="max_message_size_to_filter" value="<?php print $policy['max_message_size_to_filter']; ?>" size="6"></td><td>integer</td></tr>
      <tr><td>rbl_domain</td><td><input name="rbl_domain" value="<?php print $policy['rbl_domain']; ?>" size="30"></td><td>string</td></tr>
      <tr><td>surbl_domain</td><td><input name="surbl_domain" value="<?php print $policy['surbl_domain']; ?>" size="30"></td><td>string</td></tr>
      <tr><td>spam_overall_limit</td><td><input name="spam_overall_limit" value="<?php print $policy['spam_overall_limit']; ?>" size="3"></td><td>float</td></tr>

      <tr><td>spaminess_oblivion_limit</td><td><input name="spaminess_oblivion_limit" value="<?php print $policy['spaminess_oblivion_limit']; ?>" size="3"></td><td>float</td></tr>
      <tr><td>replace_junk_characters</td><td><input name="replace_junk_characters" value="<?php print $policy['replace_junk_characters']; ?>" size="3"><td>0|1</td></td></tr>
      <tr><td>invalid_junk_limit</td><td><input name="invalid_junk_limit" value="<?php print $policy['invalid_junk_limit']; ?>" size="3"></td><td>integer</td></tr>
      <tr><td>invalid_junk_line</td><td><input name="invalid_junk_line" value="<?php print $policy['invalid_junk_line']; ?>" size="3"></td><td>integer</td></tr>
      <tr><td>penalize_images</td><td><input name="penalize_images" value="<?php print $policy['penalize_images']; ?>" size="3"></td><td>0|1</td></tr>
      <tr><td>penalize_embed_images</td><td><input name="penalize_embed_images" value="<?php print $policy['penalize_embed_images']; ?>" size="3"></td><td>0|1</td></tr>

      <tr><td>penalize_octet_stream</td><td><input name="penalize_octet_stream" value="<?php print $policy['penalize_octet_stream']; ?>" size="3"></td><td>0|1</td></tr>
      <tr><td>training_mode</td><td><input name="training_mode" value="<?php print $policy['training_mode']; ?>" size="3"></td><td>0|1</td></tr>
      <tr><td>initial_1000_learning</td><td><input name="initial_1000_learning" value="<?php print $policy['initial_1000_learning']; ?>" size="3"></td><td>0|1</td></tr>
      <tr><td>store_metadata</td><td><input name="store_metadata" value="<?php print $policy['store_metadata']; ?>" size="3"></td><td>0|1</td></tr>
      <tr><td><input type="submit" value="<?php print $text_add; ?>"> <input type="reset" value="<?php print $text_cancel; ?>"></td><td></td></tr>
   </table>

</form>
<?php } else { print $x; ?>. 
<?php } ?>

<a href="index.php?route=policy/policy"><?php print $text_back; ?></a>

</p>



