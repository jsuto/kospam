
<?php if(isset($policy)) { ?>
<form class="form-inline" action="index.php?route=policy/add" method="post">
   <input type="hidden" name="policy_group" value="<?php print $policy['policy_group']; ?>" />

   <div class="row">
      <div class="healthcell"><?php print $text_policy_name; ?></div><div class="healthcell"><b><input type="text" name="name" value="<?php print $policy['name']; ?>" class="span2" /></b></div><div class="healthcell">&nbsp;</div>
   </div>
   <div class="row">
      <div class="healthcell">deliver_infected_email</div><div class="healthcell"><b><input type="text" name="deliver_infected_email" value="<?php print $policy['deliver_infected_email']; ?>" class="span2" /></b></div><div class="healthcell">0|1</div>
   </div>
   <div class="row">
      <div class="healthcell">silently_discard_infected_email</div><div class="healthcell"><b><input type="text" name="silently_discard_infected_email" value="<?php print $policy['silently_discard_infected_email']; ?>" class="span2" /></b></div><div class="healthcell">0|1</div>
   </div>
   <div class="row">
      <div class="healthcell">use_antispam</div><div class="healthcell"><b><input type="text" name="use_antispam" value="<?php print $policy['use_antispam']; ?>" class="span2" /></b></div><div class="healthcell">0|1</div>
   </div>
   <div class="row">
      <div class="healthcell">spam_subject_prefix</div><div class="healthcell"><b><input type="text" name="spam_subject_prefix" value="<?php print $policy['spam_subject_prefix']; ?>" class="span2" /></b></div><div class="healthcell">[string]</div>
   </div>
   <div class="row">
      <div class="healthcell">enable_auto_white_list</div><div class="healthcell"><b><input type="text" name="enable_auto_white_list" value="<?php print $policy['enable_auto_white_list']; ?>" class="span2" /></b></div><div class="healthcell">0|1</div>
   </div>
   <div class="row">
      <div class="healthcell">max_message_size_to_filter</div><div class="healthcell"><b><input type="text" name="max_message_size_to_filter" value="<?php print $policy['max_message_size_to_filter']; ?>" class="span2" /></b></div><div class="healthcell">integer</div>
   </div>
   <div class="row">
      <div class="healthcell">rbl_domain</div><div class="healthcell"><b><input type="text" name="rbl_domain" value="<?php print $policy['rbl_domain']; ?>" class="span2" /></b></div><div class="healthcell">string</div>
   </div>
   <div class="row">
      <div class="healthcell">surbl_domain</div><div class="healthcell"><b><input type="text" name="surbl_domain" value="<?php print $policy['surbl_domain']; ?>" class="span2" /></b></div><div class="healthcell">string</div>
   </div>
   <div class="row">
      <div class="healthcell">spam_overall_limit</div><div class="healthcell"><b><input type="text" name="spam_overall_limit" value="<?php print $policy['spam_overall_limit']; ?>" class="span2" /></b></div><div class="healthcell">float</div>
   </div>
   <div class="row">
      <div class="healthcell">spaminess_oblivion_limit</div><div class="healthcell"><b><input type="text" name="spaminess_oblivion_limit" value="<?php print $policy['spaminess_oblivion_limit']; ?>" class="span2" /></b></div><div class="healthcell">float</div>
   </div>
   <div class="row">
      <div class="healthcell">replace_junk_characters</div><div class="healthcell"><b><input type="text" name="replace_junk_characters" value="<?php print $policy['replace_junk_characters']; ?>" class="span2" /></b></div><div class="healthcell">0|1</div>
   </div>
   <div class="row">
      <div class="healthcell">invalid_junk_limit</div><div class="healthcell"><b><input type="text" name="invalid_junk_limit" value="<?php print $policy['invalid_junk_limit']; ?>" class="span2" /></b></div><div class="healthcell">integer</div>
   </div>
   <div class="row">
      <div class="healthcell">invalid_junk_line</div><div class="healthcell"><b><input type="text" name="invalid_junk_line" value="<?php print $policy['invalid_junk_line']; ?>" class="span2" /></b></div><div class="healthcell">integer</div>
   </div>
   <div class="row">
      <div class="healthcell">penalize_images</div><div class="healthcell"><b><input type="text" name="penalize_images" value="<?php print $policy['penalize_images']; ?>" class="span2" /></b></div><div class="healthcell">0|1</div>
   </div>
   <div class="row">
      <div class="healthcell">penalize_embed_images</div><div class="healthcell"><b><input type="text" name="penalize_embed_images" value="<?php print $policy['penalize_embed_images']; ?>" class="span2" /></b></div><div class="healthcell">0|1</div>
   </div>
   <div class="row">
      <div class="healthcell">penalize_octet_stream</div><div class="healthcell"><b><input type="text" name="penalize_octet_stream" value="<?php print $policy['penalize_octet_stream']; ?>" class="span2" /></b></div><div class="healthcell">0|1</div>
   </div>
   <div class="row">
      <div class="healthcell">training_mode</div><div class="healthcell"><b><input type="text" name="training_mode" value="<?php print $policy['training_mode']; ?>" class="span2" /></b></div><div class="healthcell">0|1</div>
   </div>
   <div class="row">
      <div class="healthcell">initial_1000_learning</div><div class="healthcell"><b><input type="text" name="initial_1000_learning" value="<?php print $policy['initial_1000_learning']; ?>" class="span2" /></b></div><div class="healthcell">0|1</div>
   </div>
   <div class="row">
      <div class="healthcell">store_metadata</div><div class="healthcell"><b><input type="text" name="store_metadata" value="<?php print $policy['store_metadata']; ?>" class="span2" /></b></div><div class="healthcell">0|1</div>
   </div>
   <div class="row">
      <div class="healthcell">store_only_spam</div><div class="healthcell"><b><input type="text" name="store_only_spam" value="<?php print $policy['store_only_spam']; ?>" class="span2" /></b></div><div class="healthcell">0|1</div>
   </div>
   <div class="row">
      <div class="healthcell">message_from_a_zombie</div><div class="healthcell"><b><input type="text" name="message_from_a_zombie" value="<?php print $policy['message_from_a_zombie']; ?>" class="span2" /></b></div><div class="healthcell">0|1</div>
   </div>

   <div class="row">
      <div class="healthcell"><input type="submit" class="btn btn-primary" value="<?php print $text_add; ?>" /> <input type="reset" class="btn btn-ok" value="<?php print $text_cancel; ?>" /></div>
   </div>


</form>

<?php } else { ?>
   <span class="text-ok bold"><?php print $x; ?>.</span> 
<?php } ?>

<a href="index.php?route=policy/policy"><?php print $text_back; ?></a>


