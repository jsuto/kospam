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


create table if not exists whitelist (
   email varchar(128) not null primary key
) Engine=InnoDB;


create table if not exists blacklist (
   email varchar(128) not null primary key
) Engine=InnoDB;


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


create table if not exists minefield (
   ip char(15) not null primary key,
   ts int default 0
) ENGINE=InnoDB;


create table if not exists counter (
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
   fn bigint unsigned default 0,
   size bigint unsigned default 0
) Engine=InnoDB;

insert into counter values(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);


create table if not exists attach_digest (
   digest char(64) unique not null,
   counter bigint default 1
) Engine=InnoDB;
