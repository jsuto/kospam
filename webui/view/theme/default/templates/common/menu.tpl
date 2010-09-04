
      <ul class="dropdown">

         <li><a href="index.php?route=common/home"<?php if(strstr($_SERVER['QUERY_STRING'], "common/home")){ print ' id="active"'; } ?>><?php print $text_home; ?></a></li>
         <li><a href="index.php?route=quarantine/quarantine"<?php if(strstr($_SERVER['QUERY_STRING'], "quarantine")){ print ' id="active"'; } ?>><?php print $text_quarantine; ?></a></li>
         <li><a href="index.php?route=user/whitelist"<?php if(strstr($_SERVER['QUERY_STRING'], "user/whitelist")){ print ' id="active"'; } ?>><?php print $text_whitelist; ?></a></li>
   <?php if(ENABLE_BLACKLIST == 1) { ?>
          <li><a href="index.php?route=user/blacklist"<?php if(strstr($_SERVER['QUERY_STRING'], "user/blacklist")){ print ' id="active"'; } ?>><?php print $text_blacklist; ?></a></li>
   <?php } ?>

         <li><a class="hide" href="#"<?php if(strstr($_SERVER['QUERY_STRING'], "stat/") || strstr($_SERVER['QUERY_STRING'], "history/")) { print ' id="active"'; } ?>><?php print $text_monitor; ?></a>
            <ul class="sub_menu">
               <li><a href="index.php?route=stat/stat"><?php print $text_statistics; ?></a></li>
            <?php if($admin_user == 1) { ?>
               <li><a href="index.php?route=stat/counter"><?php print $text_counters; ?></a></li>
            <?php } ?>
            <?php if($admin_user == 1) { ?>
               <li><a href="index.php?route=history/history"><?php print $text_history; ?></a></li>
            <?php } ?>
            </ul>
         </li>


   <?php if($admin_user == 1 || $domain_admin == 1) { ?>
         <li><a class="hide" href="#"<?php if(strstr($_SERVER['QUERY_STRING'], "domain/") || strstr($_SERVER['QUERY_STRING'], "policy/") || strstr($_SERVER['QUERY_STRING'], "user/list") || strstr($_SERVER['QUERY_STRING'], "import/")) { print ' id="active"'; } ?>><?php print $text_administration; ?></a>
            <ul class="sub_menu">
               <li><a href="index.php?route=user/list"><?php print $text_user_management; ?></a></li>
      <?php if($admin_user == 1) { ?>
               <li><a href="index.php?route=domain/domain"><?php print $text_domain; ?></a></li>
               <li><a href="index.php?route=policy/policy"><?php print $text_policy; ?></a></li>
      <?php } ?>
      <?php if(ENTERPRISE_VERSION == 1 && $domain_admin == 1) { ?>
               <li><a href="index.php?route=import/query"><?php print $text_import; ?></a></li>
      <?php } ?>
            </ul>
         </li>
   <?php } ?>
          <li><a href="http://clapf.acts.hu/wiki/doku.php/current:index"><?php print $text_help; ?></a></li>
          <li><a href="index.php?route=login/logout"<?php if(strstr($_SERVER['QUERY_STRING'], "login/logout")){ print ' id="active"'; } ?>><?php print $text_logout; ?></a></li>

      </ul>

