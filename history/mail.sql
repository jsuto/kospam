
drop table if exists `connection`;
create table if not exists `connection` (
	ts int default 0,
	queue_id char(16) not null,
	client char(64) default null,
	`from` char(64) default null,
	`from_domain` char(64) default null,
	`size` int default 0
) ENGINE=InnoDB PARTITION BY RANGE (ts) ( PARTITION p0 VALUES LESS THAN (1) );
create index connection_idx on connection(queue_id, ts, `from`, `from_domain`);


drop table if exists smtp;
create table if not exists smtp (
	ts int default 0,
	queue_id char(16) not null,
	`to` char(64) not null,
	to_domain char(48) not null,
	orig_to char(64) default null,
	orig_to_domain char(48) default null,
	relay char(64) default null,
	delay float default 0.0,
	`status` char(255) default null,
	clapf_id char(32) default null
) ENGINE=InnoDB PARTITION BY RANGE (ts) ( PARTITION p0 VALUES LESS THAN (1) );
create index smtp_idx on smtp(queue_id, to_domain, orig_to_domain, clapf_id);


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
create index clapf_idx on clapf(clapf_id, result, ts, `from`, `fromdomain`, rcpt, rcptdomain);


drop view if exists summary;
create view summary as select smtp.ts, smtp.queue_id, connection.`from`, connection.`from_domain`, connection.client, connection.size, smtp.clapf_id, clapf.result, clapf.relay, clapf.queue_id2 from connection, smtp, clapf where smtp.queue_id=connection.queue_id and smtp.clapf_id=clapf.clapf_id;


drop view if exists hist_in;
create view hist_in as select connection.`from`, connection.client, connection.`size`, smtp.ts, smtp.queue_id, smtp.`to`, smtp.orig_to, smtp.delay, smtp.clapf_id, smtp.relay, smtp.`status` from connection, smtp where smtp.queue_id = connection.queue_id;


drop view if exists hist_out;
create view hist_out as select smtp.ts, smtp.queue_id, smtp.`to`, clapf.`from`, clapf.`size`, clapf.subject, clapf.result, smtp.relay, smtp.`status` from smtp, clapf where smtp.queue_id = clapf.queue_id2;

