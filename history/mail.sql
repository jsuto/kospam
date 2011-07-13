
drop table if exists `connection`;
create table if not exists `connection` (
	ts int default 0,
	queue_id char(16) not null,
	client char(64) default null,
	`from` char(64) default null,
	`from_domain` char(64) default null,
	`size` int default 0
) ENGINE=InnoDB PARTITION BY RANGE (ts) ( PARTITION p0 VALUES LESS THAN (1) );
create index connection_idx on connection(queue_id, client);
create index connection_idx2 on connection(`from`);
create index connection_idx3 on connection(ts);

drop table if exists smtp;
create table if not exists smtp (
	ts int default 0,
	queue_id char(16) not null,
	`to` char(64) not null,
	to_domain char(48) not null,
	relay char(64) default null,
	delay float default 0.0,
	`status` char(255) default null,
	clapf_id char(32) default null
) ENGINE=InnoDB PARTITION BY RANGE (ts) ( PARTITION p0 VALUES LESS THAN (1) );
create index smtp_idx on smtp(queue_id, clapf_id);
create index smtp_idx2 on smtp(`relay`);
create index smtp_idx3 on smtp(`to`);
create index smtp_idx4 on smtp(ts);


drop table if exists clapf;
create table if not exists clapf (
	ts int default 0,
	clapf_id char(32) not null,
	queue_id2 char(16) default null,
	relay char(64) default null,
	delay float default 0.0,
        `from` char(64) not null,
        `fromdomain` char(64) not null,
	rcpt char(64) not null,
	rcptdomain char(32) not null,
	subject char(128) not null,
        `size` int default 0,
	result char(16) default null,
	spaminess float default 0.5,
	virus char(32) default null
) ENGINE=InnoDB PARTITION BY RANGE (ts) ( PARTITION p0 VALUES LESS THAN (1) );
create index clapf_idx on clapf(queue_id2);
create index clapf_idx2 on clapf(`from`);


drop view if exists hist_in;
create view hist_in as select connection.`from`, connection.client, connection.`size`, smtp.ts, smtp.queue_id, smtp.`to`, smtp.delay, smtp.clapf_id, smtp.relay, smtp.`status` from connection, smtp where smtp.queue_id = connection.queue_id;


drop view if exists hist_out;
create view hist_out as select smtp.ts, smtp.queue_id, smtp.`to`, clapf.`from`, clapf.`size`, clapf.subject, clapf.result, smtp.relay, smtp.`status` from smtp, clapf where smtp.queue_id = clapf.queue_id2;

