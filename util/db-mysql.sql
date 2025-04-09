
create table if not exists misc (
   nham int default 0,
   nspam int default 0
) Engine=InnoDB;

insert into misc (nham, nspam) values(0, 0);


create table if not exists token (
   token bigint unsigned not null,
   nham int default 0,
   nspam int default 0,
   updated date default CURRENT_DATE()
) Engine=InnoDB;

create index token_idx ON token(token);


create table if not exists user (
   `uid` int unsigned not null primary key,
   `gid` int unsigned not null,
   `username` varchar(64) not null unique,
   `realname` varchar(64) default null,
   `password` varchar(128) default null,
   `domain` varchar(128) default null,
   `dn` char(255) default '*',
   `policy_group` int(4) default 0,
   `isadmin` tinyint default 0
) Engine=InnoDB;

insert into user (uid, gid, username, password, policy_group, isadmin, domain) values (0, 0, 'admin', '$5$QkCGu9$7udzPLoK21Rt/QAnYc5tlZT4Vfm7Rsh9o7BY7lvtAt.', 0, 1, 'local');


create table if not exists email (
   `uid` int unsigned not null,
   `email` varchar(128) not null primary key
) Engine=InnoDB;

insert into email (uid, email) values(0, 'admin@local');


create table if not exists t_white_list (
   `uid` int unsigned not null primary key,
   `whitelist` blob default null
) Engine=InnoDB;

insert into t_white_list (uid) values(0);


create table if not exists t_black_list (
   `uid` int unsigned not null primary key,
   `blacklist` blob default null
) Engine=InnoDB;

insert into t_black_list (uid) values(0);



create table if not exists domain (
   `domain` varchar(128) not null primary key,
   `mapped` varchar(128) not null
) Engine=InnoDB;;

insert into domain (domain, mapped) values('local', 'local');



create table if not exists history (
   id bigint unsigned not null auto_increment,
   clapf_id char(36) not null,
   ts int unsigned not null,
   spam tinyint default 0,
   sender varchar(128) default null,
   subject tinyblob default null,
   size int default 0,
   hidden tinyint default 0,
   key (id)
) ENGINE=InnoDB PARTITION BY RANGE (ts) ( PARTITION p0 VALUES LESS THAN (1) );

create index history_idx1 on history(clapf_id);
create index history_idx2 on history(hidden);
create index history_idx3 on history(ts);
create index history_idx4 on history(spam);


create table if not exists policy (
   `id` int unsigned not null auto_increment,
   `name` varchar(128) not null,
   `deliver_infected_email` tinyint default 0,
   `silently_discard_infected_email` tinyint default 1,
   `use_antispam` tinyint default 1,
   `spam_subject_prefix` varchar(128) default null,
   `max_message_size_to_filter` int default 128000,
   `surbl_domain` varchar(128) default null,
   `spam_overall_limit` float default 0.92,
   `spaminess_oblivion_limit` float default 1.01,

   `replace_junk_characters` tinyint default 1,

   `penalize_images` tinyint default 0,
   `penalize_embed_images` tinyint default 0,
   `penalize_octet_stream` tinyint default 0,

   `training_mode` tinyint default 0,
   `store_emails` tinyint default 0,
   `store_only_spam` tinyint default 0,
   `message_from_a_zombie` tinyint default 0,

   `smtp_addr` varchar(128) default null,
   `smtp_port` int default 0,
   primary key(`id`)
) ENGINE=InnoDB;


create table if not exists minefield (
   `ip` char(15) not null primary key,
   `ts` int default 0
) ENGINE=InnoDB;


create table if not exists counter (
   `rcvd` bigint unsigned default 0,
   `mynetwork` bigint unsigned default 0,
   `ham` bigint unsigned default 0,
   `spam` bigint unsigned default 0,
   `possible_spam` bigint unsigned default 0,
   `unsure` bigint unsigned default 0,
   `minefield` bigint unsigned default 0,
   `zombie` bigint unsigned default 0,
   `virus` bigint unsigned default 0,
   `fp` bigint unsigned default 0,
   `fn` bigint unsigned default 0,
   `size` bigint unsigned default 0
) Engine=InnoDB;

insert into counter values(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);


create table if not exists `user_settings` (
   `username` char(64) not null unique,
   `pagelen` int default 20,
   `theme` char(8) default 'default',
   `lang` char(2) default null,
   `ga_enabled` int default 0,
   `ga_secret` varchar(255) default null
) Engine=InnoDB;

create index `user_settings_idx` on `user_settings`(`username`);


create table if not exists `online` (
   `username` varchar(255) not null,
   `ts` int default 0,
   `last_activity` int default 0,
   `ipaddr` varchar(255) default null,
   unique(`username`,`ipaddr`)
) Engine=InnoDB;


create table if not exists `audit` (
   `id` bigint unsigned not null auto_increment,
   `ts` int not null,
   `email` varchar(128) not null,
   `domain` varchar(128) not null,
   `action` int not null,
   `ipaddr` char(15) not null,
   `meta_id` bigint unsigned not null,
   `description` varchar(255) default null,
   `vcode` char(64) default null,
   primary key (`id`)
) ENGINE=InnoDB;

create index `audit_idx` on `audit`(`email`);
create index `audit_idx2` on `audit`(`action`);
create index `audit_idx3` on `audit`(`ipaddr`);
create index `audit_idx4` on `audit`(`ts`);
create index `audit_idx5` on `audit`(`domain`);


create table if not exists `group` (
   `id` bigint unsigned not null auto_increment primary key,
   `groupname` char(255) not null unique
) ENGINE=InnoDB;


create table if not exists `group_user` (
   `id` bigint unsigned not null,
   `email` char(128) not null,
   key `group_user_idx` (`id`),
   key `group_user_idx2` (`email`)
) ENGINE=InnoDB;


create table if not exists `group_email` (
   `id` bigint unsigned not null,
   `email` char(128) not null,
   key `group_email_idx` (`id`)
) ENGINE=InnoDB;


create table if not exists `search` (
   `email` char(128) not null,
   `ts` int default 0,
   `term` text(512) not null
) Engine=InnoDB;

create index `search_idx` on `search`(`email`);


create table if not exists `search_cache` (
   `cksum` char(128) not null,
   `expiry` int default 0,
   `result` longblob not null
) Engine=MyISAM;

create index `search_cache_idx` on `search_cache`(`cksum`);
create index `search_cache_idx2` on `search_cache`(`expiry`);


create table if not exists `status` (
   pid int default 0, messages int default 0, status char(1) default '?', ts int unsigned default 0
) ENGINE=MEMORY;


create table if not exists `attach_digest` (
   `digest` char(64) unique not null,
   `counter` int default 1
) Engine=InnoDB;
