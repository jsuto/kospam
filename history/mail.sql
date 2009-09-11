drop table if exists smtpd;
create table if not exists smtpd (
	ts int default 0,
	queue_id char(16) not null,
	client_ip char(64) not null
);

create index smtpd_idx on smtpd(queue_id);

drop table if exists cleanup;
create table if not exists cleanup (
	ts int default 0,
	queue_id char(16) not null,
	message_id char(64) not null
);

create index cleanup_idx on cleanup(queue_id);

drop table if exists qmgr;
create table if not exists qmgr (
	ts int default 0,
	queue_id char(16) not null,
	`from` char(64) not null,
	size int default 0
);

create index qmgr_idx on qmgr(queue_id);


drop table if exists smtp;
create table if not exists smtp (
	ts int default 0,
	queue_id char(16) not null,
	`to` char(64) not null,
	orig_to char(64) default null,
	relay char(64) default null,
	delay float default 0.0,
	result char(64) default null
);

create index smtp_idx on smtp(queue_id);


drop table if exists clapf;
create table if not exists clapf (
	ts int default 0,
	queue_id char(32) not null,
	queue_id2 char(16) not null,
	relay char(64) default null,
	delay float default 0.0,
	result char(16) default null,
	spaminess float default 0.5,
	virus char(32) default null
);

create index clapf_idx on clapf(queue_id);

