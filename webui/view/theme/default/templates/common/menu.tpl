<?php if(Registry::get('username')) { ?>

      <div class="navbar">

         <div class="navbar-inner">

       <?php if(BRANDING_TEXT) { ?>
          <a class="brand" href="<?php if(BRANDING_URL) { print BRANDING_URL; } else { ?>#<?php } ?>"><?php print BRANDING_TEXT; ?></a>
       <?php } ?>

            <ul class="nav pull-left">

               <li class="dropdown<?php if(strstr($_SERVER['QUERY_STRING'], "history")){ ?> active<?php } ?>">
                  <a href="#" class="dropdown-toggle" data-toggle="dropdown"><?php print $text_monitor; ?> <b class="caret"></b></a>
                  <ul class="dropdown-menu">
                     <li><a href="index.php?route=stat/stat&timespan=daily"><?php print $text_statistics; ?></a></li>
                     <li><a href="index.php?route=health/health"><?php print $text_health; ?></a></li>
                  <?php if(ENABLE_HISTORY == 1) { ?>
                     <li><a href="index.php?route=history/history"><?php print $text_history; ?></a></li>
                  <?php } ?>
                  </ul>
               </li> 
               <li class="divider-vertical"></li>


            <?php if($admin_user == 1 || $domain_admin == 1) { ?>
               <li class="dropdown<?php if(strstr($_SERVER['QUERY_STRING'], "domain/") || strstr($_SERVER['QUERY_STRING'], "policy/") || strstr($_SERVER['QUERY_STRING'], "user/list") || strstr($_SERVER['QUERY_STRING'], "import/")) { ?> active<?php } ?>">
                  <a href="#" class="dropdown-toggle" data-toggle="dropdown"><?php print $text_administration; ?> <b class="caret"></b></a> 
                  <ul class="dropdown-menu">
                     <li><a href="index.php?route=user/list"><?php print $text_user_management; ?></a></li>
                  <?php if($admin_user == 1) { ?>
                     <li><a href="index.php?route=domain/domain"><?php print $text_domain; ?></a></li>
                     <li><a href="index.php?route=policy/policy"><?php print $text_policy; ?></a></li>
                  <?php } ?>
                  </ul>
               </li>
               <li class="divider-vertical"></li>
            <?php } ?>


               <li<?php if(strstr($_SERVER['QUERY_STRING'], "quarantine")){ ?> class="active"<?php } ?>><a href="index.php?route=quarantine/quarantine"><?php print $text_quarantine; ?></a></li>
               <li class="divider-vertical"></li>

               <li<?php if(strstr($_SERVER['QUERY_STRING'], "user/whitelist")){ ?> class="active"<?php } ?>><a href="index.php?route=user/whitelist"><?php print $text_whitelist; ?></a></li>
               <li class="divider-vertical"></li>

            <?php if(ENABLE_BLACKLIST == 1) { ?>
               <li<?php if(strstr($_SERVER['QUERY_STRING'], "user/blacklist")){ ?> class="active"<?php } ?>><a href="index.php?route=user/blacklist"><?php print $text_blacklist; ?></a></li>
               <li class="divider-vertical"></li>
            <?php } ?>

            <?php if(HELPURL) { ?>
               <li><a href="<?php print HELPURL; ?>"><?php print $text_help; ?></a></li>
               <li class="divider-vertical"></li>
            <?php } ?>

            </ul>

            <ul class="nav pull-right">
               <li class="divider-vertical"></li>
               <li class="dropdown">
                  <a href="#" class="dropdown-toggle" data-toggle="dropdown"><?php print $_SESSION['realname']; ?> <i class="icon-user"></i> <b class="caret"></b></a>
                  <ul class="dropdown-menu">
                     <li><a href="index.php?route=user/settings"><?php print $text_settings; ?></a></li>
                     <li class="divider"></li>
                     <li><a href="index.php?route=login/logout"><?php print $text_logout; ?></a></li>
                  </ul>
               </li>
            </ul>

         </div>


      </div>


<?php } ?>

