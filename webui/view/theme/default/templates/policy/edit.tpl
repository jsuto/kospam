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

<?php if(isset($errorstring)){ ?><div class="alert alert-danger"><?php print $text_error; ?>: <?php print $errorstring; ?></div><?php } ?>

<?php if(isset($x)){ ?>

<div class="alert alert-success"><?php print $x; ?>.</div>
<p><a href="index.php?route=policy/list"><i class="icon-circle-arrow-left"></i>&nbsp;<?php print $text_back; ?></a></p>

<?php } elseif(isset($post)) { ?>

<p><a href="index.php?route=policy/list"><i class="icon-circle-arrow-left"></i>&nbsp;<?php print $text_back; ?></a> | <a href="index.php?route=policy/remove&amp;id=<?php print $post['id']; ?>&amp;name=<?php print $post['name']; ?>" class="confirm-delete" data-id="<?php print $post['id']; ?>" data-name="<?php print $post['name']; ?>"><i class="icon-remove-sign"></i>&nbsp;<?php print $text_remove; ?></a></p>


<form action="index.php?route=policy/edit" name="add" method="post" autocomplete="off" class="form-horizontal">
        <input type="hidden" name="id" value="<?php print $post['id']; ?>" />

	<div class="control-group<?php if(isset($errors['name'])){ print " error"; } ?>">
		<label class="control-label" for="name"><?php print $text_policy_name; ?>:</label>
		<div class="controls">
		  <input type="text" name="name" id="name" <?php if(isset($post['name'])){ print 'value="'.$post['name'].'" '; } ?>class="text" />
          <?php if ( isset($errors['name']) ) { ?><span class="help-inline"><?php print $errors['name']; ?></span><?php } ?>
		</div>
	</div>

        <div class="control-group<?php if(isset($errors['deliver_infected_email'])){ print " error"; } ?>">
                <label class="control-label" for="name">deliver_infected_email:</label>
                <div class="controls">
                  <input type="checkbox" name="deliver_infected_email" id="deliver_infected_email" <?php if(isset($post['deliver_infected_email']) && $post['deliver_infected_email'] != 0){ print 'checked="checked"'; } ?> class="checkbox" />
          <?php if ( isset($errors['deliver_infected_email']) ) { ?><span class="help-inline"><?php print $errors['deliver_infected_email']; ?></span><?php } ?>
                </div>
        </div>

        <div class="control-group<?php if(isset($errors['silently_discard_infected_email'])){ print " error"; } ?>">
                <label class="control-label" for="name">silently_discard_infected_email:</label>
                <div class="controls">
                  <input type="checkbox" name="silently_discard_infected_email" id="silently_discard_infected_email" <?php if(isset($post['silently_discard_infected_email']) && $post['silently_discard_infected_email'] != 0){ print 'checked="checked"'; } ?> class="checkbox" />
          <?php if ( isset($errors['silently_discard_infected_email']) ) { ?><span class="help-inline"><?php print $errors['silently_discard_infected_email']; ?></span><?php } ?>
                </div>
        </div>

        <div class="control-group<?php if(isset($errors['use_antispam'])){ print " error"; } ?>">
                <label class="control-label" for="name">use_antispam:</label>
                <div class="controls">
                  <input type="checkbox" name="use_antispam" id="use_antispam" <?php if(isset($post['use_antispam']) && $post['use_antispam'] != 0){ print 'checked="checked"'; } ?> class="checkbox" />
          <?php if ( isset($errors['use_antispam']) ) { ?><span class="help-inline"><?php print $errors['use_antispam']; ?></span><?php } ?>
                </div>
        </div>

        <div class="control-group<?php if(isset($errors['spam_subject_prefix'])){ print " error"; } ?>">
                <label class="control-label" for="spam_subject_prefix">spam_subject_prefix:</label>
                <div class="controls">
                  <input type="text" name="spam_subject_prefix" id="spam_subject_prefix" <?php if(isset($post['spam_subject_prefix'])){ print 'value="'.$post['spam_subject_prefix'].'" '; } ?>class="text" />
          <?php if ( isset($errors['spam_subject_prefix']) ) { ?><span class="help-inline"><?php print $errors['spam_subject_prefix']; ?></span><?php } ?>
                </div>
        </div>

        <div class="control-group<?php if(isset($errors['max_message_size_to_filter'])){ print " error"; } ?>">
                <label class="control-label" for="spam_subject_prefix">max_message_size_to_filter:</label>
                <div class="controls">
                  <input type="text" name="max_message_size_to_filter" id="max_message_size_to_filter" <?php if(isset($post['max_message_size_to_filter'])){ print 'value="'.$post['max_message_size_to_filter'].'" '; } ?>class="text" />
          <?php if ( isset($errors['max_message_size_to_filter']) ) { ?><span class="help-inline"><?php print $errors['max_message_size_to_filter']; ?></span><?php } ?>
                </div>
        </div>

        <div class="control-group<?php if(isset($errors['surbl_domain'])){ print " error"; } ?>">
                <label class="control-label" for="spam_subject_prefix">surbl_domain:</label>
                <div class="controls">
                  <input type="text" name="surbl_domain" id="surbl_domain" <?php if(isset($post['surbl_domain'])){ print 'value="'.$post['surbl_domain'].'" '; } ?>class="text" />
          <?php if ( isset($errors['surbl_domain']) ) { ?><span class="help-inline"><?php print $errors['surbl_domain']; ?></span><?php } ?>
                </div>
        </div>

        <div class="control-group<?php if(isset($errors['spam_overall_limit'])){ print " error"; } ?>">
                <label class="control-label" for="spam_subject_prefix">spam_overall_limit:</label>
                <div class="controls">
                  <input type="text" name="spam_overall_limit" id="spam_overall_limit" <?php if(isset($post['spam_overall_limit'])){ print 'value="'.$post['spam_overall_limit'].'" '; } ?>class="text" />
          <?php if ( isset($errors['spam_overall_limit']) ) { ?><span class="help-inline"><?php print $errors['spam_overall_limit']; ?></span><?php } ?>
                </div>
        </div>

        <div class="control-group<?php if(isset($errors['spaminess_oblivion_limit'])){ print " error"; } ?>">
                <label class="control-label" for="spam_subject_prefix">spaminess_oblivion_limit:</label>
                <div class="controls">
                  <input type="text" name="spaminess_oblivion_limit" id="spaminess_oblivion_limit" <?php if(isset($post['spaminess_oblivion_limit'])){ print 'value="'.$post['spaminess_oblivion_limit'].'" '; } ?>class="text" />
          <?php if ( isset($errors['spaminess_oblivion_limit']) ) { ?><span class="help-inline"><?php print $errors['spaminess_oblivion_limit']; ?></span><?php } ?>
                </div>
        </div>

        <div class="control-group<?php if(isset($errors['replace_junk_characters'])){ print " error"; } ?>">
                <label class="control-label" for="name">replace_junk_characters:</label>
                <div class="controls">
                  <input type="checkbox" name="replace_junk_characters" id="replace_junk_characters" <?php if(isset($post['replace_junk_characters']) && $post['replace_junk_characters'] != 0){ print 'checked="checked"'; } ?> class="checkbox" />
          <?php if ( isset($errors['replace_junk_characters']) ) { ?><span class="help-inline"><?php print $errors['replace_junk_characters']; ?></span><?php } ?>
                </div>
        </div>

        <div class="control-group<?php if(isset($errors['penalize_images'])){ print " error"; } ?>">
                <label class="control-label" for="name">penalize_images:</label>
                <div class="controls">
                  <input type="checkbox" name="penalize_images" id="penalize_images" <?php if(isset($post['penalize_images']) && $post['penalize_images'] != 0){ print 'checked="checked"'; } ?> class="checkbox" />
          <?php if ( isset($errors['penalize_images']) ) { ?><span class="help-inline"><?php print $errors['penalize_images']; ?></span><?php } ?>
                </div>
        </div>

        <div class="control-group<?php if(isset($errors['penalize_embed_images'])){ print " error"; } ?>">
                <label class="control-label" for="name">penalize_embed_images:</label>
                <div class="controls">
                  <input type="checkbox" name="penalize_embed_images" id="penalize_embed_images" <?php if(isset($post['penalize_embed_images']) && $post['penalize_embed_images'] != 0){ print 'checked="checked"'; } ?> class="checkbox" />
          <?php if ( isset($errors['penalize_embed_images']) ) { ?><span class="help-inline"><?php print $errors['penalize_embed_images']; ?></span><?php } ?>
                </div>
        </div>

        <div class="control-group<?php if(isset($errors['penalize_octet_stream'])){ print " error"; } ?>">
                <label class="control-label" for="name">penalize_octet_stream:</label>
                <div class="controls">
                  <input type="checkbox" name="penalize_octet_stream" id="penalize_octet_stream" <?php if(isset($post['penalize_octet_stream']) && $post['penalize_octet_stream'] != 0){ print 'checked="checked"'; } ?> class="checkbox" />
          <?php if ( isset($errors['penalize_octet_stream']) ) { ?><span class="help-inline"><?php print $errors['penalize_octet_stream']; ?></span><?php } ?>
                </div>
        </div>

        <div class="control-group<?php if(isset($errors['training_mode'])){ print " error"; } ?>">
                <label class="control-label" for="training_mode">training_mode:</label>
                <div class="controls">
                  <input type="text" name="training_mode" id="training_mode" <?php if(isset($post['training_mode'])){ print 'value="'.$post['training_mode'].'" '; } ?>class="text" />
          <?php if ( isset($errors['training_mode']) ) { ?><span class="help-inline"><?php print $errors['training_mode']; ?></span><?php } ?>
                </div>
        </div>

        <div class="control-group<?php if(isset($errors['store_emails'])){ print " error"; } ?>">
                <label class="control-label" for="name">store_emails:</label>
                <div class="controls">
                  <input type="checkbox" name="store_emails" id="store_emails" <?php if(isset($post['store_emails']) && $post['store_emails'] != 0){ print 'checked="checked"'; } ?> class="checkbox" />
          <?php if ( isset($errors['store_emails']) ) { ?><span class="help-inline"><?php print $errors['store_emails']; ?></span><?php } ?>
                </div>
        </div>

        <div class="control-group<?php if(isset($errors['store_only_spam'])){ print " error"; } ?>">
                <label class="control-label" for="name">store_only_spam:</label>
                <div class="controls">
                  <input type="checkbox" name="store_only_spam" id="store_only_spam" <?php if(isset($post['store_only_spam']) && $post['store_only_spam'] != 0){ print 'checked="checked"'; } ?> class="checkbox" />
          <?php if ( isset($errors['store_only_spam']) ) { ?><span class="help-inline"><?php print $errors['store_only_spam']; ?></span><?php } ?>
                </div>
        </div>

        <div class="control-group<?php if(isset($errors['message_from_a_zombie'])){ print " error"; } ?>">
                <label class="control-label" for="spam_subject_prefix">message_from_a_zombie:</label>
                <div class="controls">
                  <input type="text" name="message_from_a_zombie" id="message_from_a_zombie" <?php if(isset($post['message_from_a_zombie'])){ print 'value="'.$post['message_from_a_zombie'].'" '; } ?>class="text" />
          <?php if ( isset($errors['message_from_a_zombie']) ) { ?><span class="help-inline"><?php print $errors['message_from_a_zombie']; ?></span><?php } ?>
                </div>
        </div>

        <div class="control-group<?php if(isset($errors['smtp_addr'])){ print " error"; } ?>">
                <label class="control-label" for="spam_subject_prefix">smtp_addr:</label>
                <div class="controls">
                  <input type="text" name="smtp_addr" id="smtp_addr" <?php if(isset($post['smtp_addr'])){ print 'value="'.$post['smtp_addr'].'" '; } ?>class="text" />
          <?php if ( isset($errors['smtp_addr']) ) { ?><span class="help-inline"><?php print $errors['smtp_addr']; ?></span><?php } ?>
                </div>
        </div>

        <div class="control-group<?php if(isset($errors['smtp_port'])){ print " error"; } ?>">
                <label class="control-label" for="spam_subject_prefix">smtp_port:</label>
                <div class="controls">
                  <input type="text" name="smtp_port" id="smtp_port" <?php if(isset($post['smtp_port'])){ print 'value="'.$post['smtp_port'].'" '; } ?>class="text" />
          <?php if ( isset($errors['smtp_port']) ) { ?><span class="help-inline"><?php print $errors['smtp_port']; ?></span><?php } ?>
                </div>
        </div>


	<div class="form-actions">
        <input type="submit" value="<?php print $text_modify; ?>" class="btn btn-primary" /> <a href="index.php?route=policy/list" class="btn"><?php print $text_cancel; ?></a>
	</div>

</form>

<?php } ?>
