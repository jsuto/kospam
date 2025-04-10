<?php if(Registry::get('username')) { ?>

    <div class="navbar navbar-fixed-top">
      <div class="navbar-inner">
        <div class="container-fluid"<?php if($settings['background_colour']) { ?> style="background: <?php print $settings['background_colour']; ?>;"<?php } ?>>
        
          <button type="button" class="btn btn-navbar" data-toggle="collapse" data-target=".nav-collapse">
            <span class="icon-bar"></span>
            <span class="icon-bar"></span>
            <span class="icon-bar"></span>
          </button>
        
          <a class="brand" target="_blank" href="<?php print $settings['branding_url']; ?>" title="<?php print $settings['branding_text']; ?>"><?php if($settings['branding_logo']) { ?><img src="/images/<?php print $settings['branding_logo']; ?>" alt="<?php print $settings['branding_text']; ?>" /><?php } ?></a>
          
            <div class="nav-collapse collapse">
                <ul class="nav">

    <?php if($admin_user == 1) { ?>

                    <li class="dropdown">
                        <a href="#" class="dropdown-toggle" data-toggle="dropdown" <?php if($settings['text_colour']) { ?> style="color: <?php print $settings['text_colour']; ?>;"<?php } ?>><i class="icon-desktop"></i>&nbsp;<?php print $text_monitor; ?>&nbsp;<b class="caret"></b></a>
                        <ul class="dropdown-menu">
                            <li><a href="index.php?route=stat/stat&timespan=daily"><i class="icon-bar-chart"></i>&nbsp;<?php print $text_statistics; ?></a></li>
                            <li><a href="index.php?route=health/health"><i class="icon-medkit"></i>&nbsp;<?php print $text_health; ?></a></li>
                            <li><a href="index.php?route=history/history"><i class="icon-calendar"></i>&nbsp;<?php print $text_history; ?></a></li>
        <?php if(ENABLE_AUDIT == 1) { ?>
                            <li><a href="index.php?route=audit/audit"><i class="icon-book"></i>&nbsp;<?php print $text_audit; ?></a></li>
        <?php } ?>
                        </ul>
                    </li>
                    <li class="dropdown"<?php if(strstr($_SERVER['QUERY_STRING'], "domain/") || ($_SERVER['QUERY_STRING'] != "route=user/settings" && strstr($_SERVER['QUERY_STRING'], "user/")) || strstr($_SERVER['QUERY_STRING'], "policy/") || strstr($_SERVER['QUERY_STRING'], "import/")) { ?> id="active"<?php } ?>>
                        <a href="#" class="dropdown-toggle" data-toggle="dropdown" <?php if($settings['text_colour']) { ?> style="color: <?php print $settings['text_colour']; ?>;"<?php } ?>><i class="icon-cogs"></i>&nbsp;<?php print $text_administration; ?>&nbsp;<b class="caret"></b></a>
                        <ul class="dropdown-menu">
                            <li><a href="index.php?route=user/list"><i class="icon-user"></i>&nbsp;<?php print $text_users; ?></a></li>
                            <li><a href="index.php?route=group/list"><i class="icon-group"></i>&nbsp;<?php print $text_groups; ?></a></li>
                            <li><a href="index.php?route=domain/domain"><i class="icon-globe"></i>&nbsp;<?php print $text_domain; ?></a></li>
                            <li><a href="index.php?route=policy/list"><i class="icon-hand-right"></i>&nbsp;<?php print $text_policy; ?></a></li>
                        </ul>
                    </li>

        <?php //if(LDAP_ADMIN_MEMBER_DN) { ?>
                    <li><a href="search.php" <?php if($settings['text_colour']) { ?> style="color: <?php print $settings['text_colour']; ?>;"<?php } ?>><i class="icon-search"></i>&nbsp;<?php print $text_search; ?></a></li>
        <?php //} ?>

    <?php } else { ?>

                    <li><a href="search.php" <?php if($settings['text_colour']) { ?> style="color: <?php print $settings['text_colour']; ?>;"<?php } ?>><i class="icon-search"></i>&nbsp;<?php print $text_search; ?></a></li>
                    <li><a href="whitelist.php" <?php if($settings['text_colour']) { ?> style="color: <?php print $settings['text_colour']; ?>;"<?php } ?>>&nbsp;<?php print $text_whitelist; ?></a></li>
                    <li><a href="blacklist.php" <?php if($settings['text_colour']) { ?> style="color: <?php print $settings['text_colour']; ?>;"<?php } ?>>&nbsp;<?php print $text_blacklist; ?></a></li>

        <?php if(ENABLE_AUDIT == 1 && $auditor_user == 1) { ?>
                    <li><a href="index.php?route=audit/audit" <?php if($settings['text_colour']) { ?> style="color: <?php print $settings['text_colour']; ?>;"<?php } ?>><i class="icon-book"></i>&nbsp;<?php print $text_audit; ?></a></li>
        <?php } ?>

        <?php if($settings['support_link']) { ?>
                    <li><a href="<?php print $settings['support_link']; ?>" <?php if($settings['text_colour']) { ?> style="color: <?php print $settings['text_colour']; ?>;"<?php } ?>><?php print $text_contact_support; ?></a></li>
        <?php } ?>

        <?php if(ENABLE_FOLDER_RESTRICTIONS == 1) { ?>
                    <li><a href="/folders.php" <?php if($settings['text_colour']) { ?> style="color: <?php print $settings['text_colour']; ?>;"<?php } ?>><i class="icon-folder-close"></i>&nbsp;<?php print $text_folders; ?></a></li>
        <?php } ?>

    <?php } ?>
                </ul>
            
                <ul class="nav pull-right">
	<?php if($settings['branding_url']) { ?>
                    <li><a href="<?php print $settings['branding_url']; ?>" <?php if($settings['text_colour']) { ?> style="color: <?php print $settings['text_colour']; ?>;"<?php } ?> target="_blank"><i class="icon-phone"></i>&nbsp;<?php print $settings['branding_text']; ?></a></li>
    <?php } ?>

                    <li class="dropdown">
                        <a href="#" class="dropdown-toggle" data-toggle="dropdown"><i class="icon-user"></i>&nbsp;<?php if($realname) { print $realname; ?>&nbsp;<?php } ?><b class="caret"></b></a>
                        <ul class="dropdown-menu">
    <?php if($settings['support_link']) { ?>
                            <li><a href="<?php print $settings['support_link']; ?>" target="_blank"><i class="icon-question-sign"></i>&nbsp;<?php print $text_contact_support; ?></a></li>
                            <li class="divider"></li>
    <?php } ?>
                            <li><a href="settings.php"><i class="icon-cog"></i>&nbsp;<?php print $text_settings; ?></a></li>
                            <li class="divider"></li>
                            <li><a href="logout.php"><i class="icon-off"></i>&nbsp;<?php print $text_logout; ?></a></li>
                        </ul>
                    </li>
                </ul>  
          </div><!--/.nav-collapse -->
        </div>
      </div>
    </div>
<?php } ?>
