   <div id="search">

      <div id="health1">

         <div class="row">
            <div class="cellhealthleft"><?php print $text_refresh_period; ?>:</div>
            <div class="cellhealthright"><?php print HEALTH_REFRESH; ?> sec</div>
         </div>

         <div class="row">
            <div class="cellhealthleft"><?php print $text_server_name; ?>:</div>
            <div class="cellhealthright"><?php print $sysinfo[0]; ?></div>
         </div>

         <div class="row">
            <div class="cellhealthleft"><?php print $text_server_operating_system; ?>:</div>
            <div class="cellhealthright"><?php print $sysinfo[1]; ?></div>
         </div>

         <div class="row">
            <div class="cellhealthleft"><?php print $text_uptime; ?>:</div>
            <div class="cellhealthright"><?php print $uptime; ?></div>
         </div>

         <div class="row">
            <div class="cellhealthleft"><?php print $text_processed_emails_in; ?>:</div>
            <div class="cellhealthright"><?php print $processed_emails[0]; ?> / <?php print $processed_emails[1]; ?> / <?php print $processed_emails[2]; ?></div>
         </div>

         <div class="row">
            <div class="cellhealthleft"><?php print $text_quarantined_emails; ?>:</div>
            <div class="cellhealthright"><?php print $number_of_quarantined_messages; ?></div>
         </div>

         <div class="row">
            <div class="cellhealthleft"><?php print $text_smtp_status; ?>:</div>
            <div class="cellhealthright">
               <?php foreach($health as $h) {
                        if(preg_match("/^220/", $h[1])) {
                           $status = 'OK'; $class = 'text-success';
                        } else {
                           $status = 'ERROR'; $class = 'text-error';
                        }
               ?>
                        <div class="bold <?php print $class; ?>"><span><?php print $h[3]; ?>: <?php print $status; ?></span></div>
               <?php } ?>
            </div>
         </div>

      <?php if(MAILLOG_PID_FILE) { ?>
         <div class="row">
            <div class="cellhealthleft"><?php print $text_maillog_status; ?>:</div>
            <div class="cellhealthright"><span class=" bold <?php if($maillog_status == $text_running) { ?>text-success<?php } else { ?>text-error<?php } ?>"><?php print $maillog_status; ?></span></div>
         </div>
      <?php } ?>

      <?php if(ENABLE_LDAP_IMPORT_FEATURE == 1) { ?>
         <div class="row">
            <div class="cellhealthleft"><?php print $text_ad_sync_status; ?>:</div>
            <div class="cellhealthright"><span class="<?php if($totalusers >= LDAP_IMPORT_MINIMUM_NUMBER_OF_USERS_TO_HEALTH_OK && $total_emails_in_database >= LDAP_IMPORT_MINIMUM_NUMBER_OF_USERS_TO_HEALTH_OK) { ?>text-success<?php } else { ?>text-error<?php } ?>"><?php print $adsyncinfo; ?> <?php print strtolower($text_email); ?></span></div>
         </div>
      <?php } ?>


      <?php if(DAILY_QUARANTINE_REPORT_STAT) { ?>
         <div class="row">
            <div class="cellhealthleft"><?php print $text_daily_quarantine_report_status; ?>:</div>
            <div class="cellhealthright"><span class="bold <?php if(preg_match("/\/0$/", $quarantinereportinfo)) { ?>text-success<?php } else { ?>text-error<?php } ?>"><?php print $quarantinereportinfo; ?></span></div>
         </div>
      <?php } ?>

         <div class="row">
            <div class="cellhealthleft"><?php print $text_cpu_usage; ?>:</div>
            <div class="cellhealthright"><?php print $cpuload; ?></div>
         </div>

         <div class="row">
            <div class="cellhealthleft"><?php print $text_memory_usage; ?>:</div>
            <div class="cellhealthright"><span class="bold <?php if($meminfo < HEALTH_RATIO) { ?>text-success<?php } else { ?>text-error<?php } ?>"><?php print $meminfo; ?>%</span> / <?php print $totalmem; ?> MB</div>
         </div>

         <div class="row">
            <div class="cellhealthleft"><?php print $text_swap_usage; ?>:</div>
            <div class="cellhealthright"><span class="bold <?php if($swapinfo < HEALTH_RATIO) { ?>text-success<?php } else { ?>text-error<?php } ?>"><?php print $swapinfo; ?>%</span> / <?php print $totalswap; ?> MB</div>
         </div>

         <div class="row">
            <div class="cellhealthleft"><?php print $text_disk_usage; ?>:</div>
            <div class="cellhealthright"><?php foreach($shortdiskinfo as $partition) { ?><span class="bold <?php if($partition['utilization'] < HEALTH_RATIO) { ?>text-success<?php } else { ?>text-error<?php } ?>"><?php print $partition['partition']; ?> <?php print $partition['utilization']; ?>%</span> <?php } ?></div>
         </div>

         <div class="row">
            <div class="cellhealthleft"><?php print $text_counters; ?>:</div>
            <div class="cellhealthright">
            <?php while(list($k, $v) = each($counters)) { ?>
               <div class="row">
                  <div class="domaincell"><?php $a = preg_replace("/^_c\:/", "", $k); if(isset($$a)) { print $$a; } else { print $a; } ?></div>
                  <div class="domaincell"><?php print $v; ?></div>
               </div>
            <?php } ?>

               <?php if($counters[$prefix . 'rcvd'] > 0) { ?><div class="row"><div class="domaincell">spam / <?php print $text_total_ratio; ?></div><div class="domaincell"><?php print sprintf("%.2f", 100*$counters[$prefix . 'spam'] / $counters[$prefix . 'rcvd']); ?> %</div></div><?php } ?>
               <?php if($counters[$prefix . 'rcvd'] > 0) { ?><div class="row"><div class="domaincell">virus / <?php print $text_total_ratio; ?></div><div class="domaincell"><?php print sprintf("%.2f", 100*$counters[$prefix . 'virus'] / $counters[$prefix . 'rcvd']); ?> %</div></div><?php } ?>

            <?php if(Registry::get('admin_user') == 1) { ?>
               <form action="index.php?route=health/worker" method="post">
                  <input type="hidden" name="resetcounters" value="1" />
                  <input type="submit" name="submit" value="<?php print $text_reset_counters; ?>" />
               </form>
            <?php } ?>

            </div>
         </div>




      </div>


      <div id="health2">
         <h4><?php print $text_queue_status; ?>: </h4>

         <?php foreach ($queues as $queue) { ?>

         <div class="row">
            <div class="cellhealthleft"><?php print $queue['desc']; ?></div>
            <div class="cellhealthright">
               <?php

                  $i = 0;
                  while(list($k, $v) = each($queue['lines'])) {
                     $i++;

                     $v = preg_replace("/^\s {1,}/", "", $v);
                     $v = preg_replace("/\ {1,}/", "</div><div class=\"healthcell\">", $v);
               ?>
                  <div class="row">
                     <?php if($i == 1) { ?><div class="healthcell">&nbsp;</div><?php } ?><div class="healthcell"><?php print $v; ?></div>
                  </div>

               <?php
                     if($i == count($queue['lines'])-1) { break; }

                  }
                  
               ?>

            </div>
         </div>

         <?php } ?>


      </div>

   </div>

