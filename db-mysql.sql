
create table if not exists t_misc (
	nham int default 0,
	nspam int default 0,
	uid int unsigned default 0,
	unique(uid)
);

create index t_misc_idx on t_misc(uid);

INSERT INTO t_misc (nham, nspam, uid) VALUES(0, 0, 0);

create table if not exists t_token (
	token bigint unsigned not null,
	uid smallint unsigned default 0,
	nham int default 0,
	nspam int default 0,
	timestamp int unsigned default 0,
	unique(token, uid)
) Engine=InnoDB;

create index t_token_idx on t_token(token, uid);

create table if not exists user (
	uid int unsigned not null,
        username char(32) not null,
	password char(48) default null,
	policy_group int(4) default 0,
	isadmin tinyint default 0,
	unique (uid)
);

create index user_idx on user (uid);
insert into user (uid, username, password, policy_group, isadmin) values (0, 'admin', '$1$kkBnp0$L/MILe67UGcvHeFlTAQjR1', 0, 1);

create table if not exists t_email (
	uid int unsigned not null,
	email char(128) not null primary key
);

create index t_email_idx on t_email(email);
insert into t_email (uid, email) values(0, 'admin@yourdomain.com');

create table if not exists t_white_list (
	uid int unsigned not null primary key,
	whitelist blob default null
);

create index t_white_list_idx on t_white_list (uid);
insert into t_white_list (uid) values(0);

create table if not exists t_black_list (
        uid int unsigned not null primary key,
        blacklist blob default null
);

create index t_black_list_idx on t_black_list (uid);
insert into t_black_list (uid) values(0);

create table if not exists t_queue (
	id char(32) not null,
	uid int unsigned not null,
	ts bigint unsigned not null,
	is_spam tinyint default 0,
	data blob not null
) Engine=InnoDB;

create index t_queue_idx on t_queue(uid, id);
create index t_queue_idx2 on t_queue(ts);

create table if not exists t_stat (
	uid int unsigned not null,
	ts bigint unsigned not null,
	nham int default 0,
	nspam int default 0
);

create index t_stat_idx on t_stat(uid);


create table if not exists t_policy (
	policy_group int(4) not null,
	name char(128) not null,
	deliver_infected_email tinyint default 0,
	silently_discard_infected_email tinyint default 1,
	use_antispam tinyint default 1,
	spam_subject_prefix char(128) default null,
	enable_auto_white_list tinyint default 1,
	max_message_size_to_filter int default 128000,
	rbl_domain char(128) default null,
	surbl_domain char(128) default null,
	spam_overall_limit float default 0.92,
	spaminess_oblivion_limit float default 1.01,

	replace_junk_characters tinyint default 1,
	invalid_junk_limit tinyint default 5,
	invalid_junk_line tinyint default 1,

	penalize_images tinyint default 0,
	penalize_embed_images tinyint default 0,
	penalize_octet_stream tinyint default 0,

	training_mode tinyint default 0,
	initial_1000_learning tinyint default 0,
        store_metadata tinyint default 0,

	unique(policy_group)
);

create index t_policy_idx on t_policy(policy_group);

