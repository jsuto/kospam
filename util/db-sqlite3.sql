
create table if not exists t_misc (
	nham int default 0,
	nspam int default 0,
	uid int unsigned default 0,
	unique(uid)
);

create index t_misc_idx on t_misc(uid);

INSERT INTO t_misc (nham, nspam, uid) VALUES(0, 0, 0);

create table if not exists t_token (
	token integer not null,
	uid int unsigned default 0, 
	nham int default 0,
	nspam int default 0,
	timestamp int default 0,
	unique(token, uid)
);

create index t_token_idx on t_token(token);

create table if not exists user (
        uid int unsigned not null,
	gid int unsigned not null,
        username char(64) not null,
	realname char(64) default null,
	password char(48) default null,
	domain char(64) default null,
	dn char(255) default '*',
	policy_group integer(4) default 0,
	isadmin tinyint default 0,
        unique (uid), unique (username)
);

create index user_idx on user (uid, domain);
insert into user (uid, gid, username, password, policy_group, isadmin, domain) values (0, 0, 'admin', '$1$kkBnp0$L/MILe67UGcvHeFlTAQjR1', 0, 1, 'local');

create table if not exists t_email (
	uid int unsigned not null,
	email char(128) not null primary key
);

create index t_email_idx on t_email(email);
insert into t_email (uid, email) values(0, 'admin@local');

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
        store_only_spam tinyint default 0,
        message_from_a_zombie tinyint default 0,
        unique(policy_group)
);

create index t_policy_idx on t_policy(policy_group);

create table if not exists t_minefield (
	ip char(15) not null unique,
	ts int default 0
);

create index t_minefield_idx on t_minefield(ip);

create table if not exists t_domain (
	domain char(64) not null,
	mapped char(64) not null,
	unique(domain)
);

insert into t_domain (domain, mapped) values('local', 'local');
create index t_domain_idx on t_domain (domain);


create table if not exists t_quarantine_group (
	uid int unsigned not null,
	gid int unsigned not null,
	unique(uid, gid)
);

create index t_quarantine_group_idx on t_quarantine_group (uid, gid);


create table if not exists t_remote (
	remotedomain char(64) not null unique,
	remotehost char(64) not null,
	basedn char(64) not null,
	binddn char(64) not null,
	sitedescription char(64) default null
);

create index t_remote_idx on t_remote(remotedomain);

create table if not exists t_transport (
        domain char(64) not null primary key,
        destination char(64) not null
);

create table if not exists t_counters (
        rcvd bigint unsigned default 0,
	mynetwork bigint unsigned default 0,
        ham bigint unsigned default 0,
        spam bigint unsigned default 0,
        possible_spam bigint unsigned default 0,
        unsure bigint unsigned default 0,
        minefield bigint unsigned default 0,
        zombie bigint unsigned default 0,
        virus bigint unsigned default 0,
        fp bigint unsigned default 0,
        fn bigint unsigned default 0
);

insert into t_counters values(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

