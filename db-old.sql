
create table if not exists t_misc (
	nham int default 0,
	nspam int default 0,
	uid int unsigned default 0
);

create index t_misc_idx on t_misc(uid);

create table if not exists t_token (
	token char(38) not null,
	uid smallint unsigned default 0,
	nham int default 0,
	nspam int default 0,
	unique(token, uid)
);

create index t_token_idx on t_token(token, uid);

create table if not exists user (
	uid int unsigned not null,
        email char(128) not null,
	action enum ('drop', 'junk', 'quarantine') default 'junk',
	pagelen int default 25,
        username char(32) primary key not null
);

create index user_idx on user (username, email);

create table if not exists t_queue (
        id char(32) not null,
        uid int unsigned not null,
        ts bigint unsigned not null,
        data blob not null,
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

