
      <div id="searchpopup1" class="container">

         <div id="popupx" onclick="$('#searchpopup1').hide();"><p class="text-right"><i class="icon-remove-circle icon-large"></i></p></div>

        <form id="ss1" class="form-horizontal" action="#" onsubmit="Clapf.complex(this); return false;">
         <div class="control-group">
             <label class="control-label" for="from"><?php print $text_from; ?>:</label>
             <div class="controls">
                <input type="text" name="xfrom" id="xfrom" value="" />
             </div>
         </div>

         <div class="control-group">
            <label class="control-label" for="to"><?php print $text_to; ?>:</label>
            <div class="controls">
               <input type="text" name="xto" id="xto" value="" />
            </div>
         </div>

         <div class="control-group">
            <label class="control-label" for="subject"><?php print $text_subject; ?>:</label>
            <div class="controls">
               <input type="text" name="xsubject" id="xsubject" value="" />
            </div>
         </div>

         <div class="control-group">
            <label class="control-label" for="date"><?php print $text_date; ?>:</label>
            <div class="controls">
               <input type="text" name="date" id="date" value="" placeholder="YYYY-MM-DD" />
            </div>
         </div>

         <div class="control-group">
            <label class="control-label" for="hamspam">HAM/SPAM:</label>
            <div class="controls">
               <select id="xhamspam" name="xhamspam">
            <?php if(SPAM_ONLY_QUARANTINE == 0) { ?>
                  <option value=""<?php if($hamspam == ""){ ?> selected="selected"<?php } ?>>All</option>
                  <option value="HAM"<?php if($hamspam == "HAM"){ ?> selected="selected"<?php } ?>>HAM</option>
            <?php } ?>
                  <option value="SPAM"<?php if($hamspam == "SPAM"){ ?> selected="selected"<?php } ?>>SPAM</option>
               </select>
            </div>
         </div>

         <div class="form-actions">
            <input type="button" id="button_search" class="btn btn-danger" value="<?php print $text_search; ?>" onclick="Clapf.complex(this); return false;" />
            <input type="button" class="btn" onclick="Clapf.cancel();" value="<?php print $text_cancel; ?>" />
         </div>

        </form>

      </div> <!-- searchpopup1 -->


