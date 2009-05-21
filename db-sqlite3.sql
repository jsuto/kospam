
create table if not exists t_misc (
	nham int default 0,
	nspam int default 0,
	uid int unsigned default 0
);

create index t_misc_idx on t_misc(uid);

INSERT INTO t_misc (nham, nspam, uid) VALUES(0, 0, 0);

create table if not exists t_token (
	token integer not null,
	nham int default 0,
	nspam int default 0,
	timestamp int default 0,
	unique(token)
);

create index t_token_idx on t_token(token);

create table if not exists user (
        uid int unsigned not null,
        username char(32) not null,
	password char(48) default null,
	policy_group integer(4) default 0,
	isadmin tinyint default 0,
        unique (uid, username)
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
	is_spam integer(1) default 0,
	data blob not null
);

create index t_queue_idx on t_queue(uid, id);
create index t_queue_idx2 on t_queue(ts);

create table if not exists t_stat (
	uid int unsigned not null,
	ts bigint unsigned not null,
	nham int default 0,
	nspam int default 0
);

create index t_stat_idx on t_stat(uid);

create table if not exists t_train_log (
	uid int unsigned not null,
	ts bigint unsigned not null,
	msgid char(32) not null,
	is_spam tinyint not null
);

create index t_train_log_idx on t_train_log(uid);

