
          <li><a href="index.php?route=common/home"<?php if(strstr($_SERVER['QUERY_STRING'], "common/home")){ print ' id="active"'; } ?>><?php print $text_home; ?></a></li>
          <li><a href="index.php?route=quarantine/quarantine"<?php if(strstr($_SERVER['QUERY_STRING'], "quarantine/")){ print ' id="active"'; } ?>><?php print $text_quarantine; ?></a></li>
          <li><a href="index.php?route=user/whitelist"<?php if(strstr($_SERVER['QUERY_STRING'], "user/whitelist")){ print ' id="active"'; } ?>><?php print $text_whitelist; ?></a></li>
          <li><a href="index.php?route=user/blacklist"<?php if(strstr($_SERVER['QUERY_STRING'], "user/blacklist")){ print ' id="active"'; } ?>><?php print $text_blacklist; ?></a></li>
   <?php if(DB_DRIVER == "mysql"){ ?>
          <li><a href="index.php?route=stat/stat"<?php if(strstr($_SERVER['QUERY_STRING'], "stat/stat")){ print ' id="active"'; } ?>><?php print $text_statistics; ?></a></li>
   <?php } ?>
   <?php if($admin_user == 1 || $domain_admin == 1){ ?>
          <li><a href="index.php?route=user/list"<?php if(strstr($_SERVER['QUERY_STRING'], "user/") && !strstr($_SERVER['QUERY_STRING'], "user/whitelist") && !strstr($_SERVER['QUERY_STRING'], "user/blacklist")){ print ' id="active"'; } ?>><?php print $text_user_management; ?></a></li>
      <?php if(ENTERPRISE_VERSION == 1 && $domain_admin == 1) { ?>
          <li><a href="index.php?route=import/query"<?php if(strstr($_SERVER['QUERY_STRING'], "import/")){ print ' id="active"'; } ?>><?php print $text_import; ?></a></li>
      <?php } ?>
   <?php } ?>
   <?php if($admin_user == 1) { ?>
          <li><a href="index.php?route=domain/domain"<?php if(strstr($_SERVER['QUERY_STRING'], "domain/")){ print ' id="active"'; } ?>><?php print $text_domain; ?></a></li>
          <li><a href="index.php?route=policy/policy"<?php if(strstr($_SERVER['QUERY_STRING'], "policy/")){ print ' id="active"'; } ?>><?php print $text_policy; ?></a></li>
          <li><a href="index.php?route=history/history"<?php if(strstr($_SERVER['QUERY_STRING'], "history/")){ print ' id="active"'; } ?>><?php print $text_history; ?></a></li>
   <?php } ?>
          <li><a href="index.php?route=login/logout"<?php if(strstr($_SERVER['QUERY_STRING'], "login/logout")){ print ' id="active"'; } ?>><?php print $text_logout; ?></a></li>


