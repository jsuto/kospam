--
-- 2011.04.29 Varadi
--

CREATE TABLE t_misc (
  nham int4 DEFAULT 0,
  nspam int4 DEFAULT 0,
  uid bigint DEFAULT 0,
  UNIQUE (uid)
);

INSERT INTO t_misc (nham,nspam,uid) VALUES (0,0,0);

-- bigint unsigned -> numeric(20)

CREATE TABLE t_token (
  token numeric(20) NOT NULL,
  uid int4 NOT NULL DEFAULT 0,
  nham int4 DEFAULT 0,
  nspam int4 DEFAULT 0,
  "timestamp" bigint DEFAULT 0,
  PRIMARY KEY (token,uid)
);


CREATE TABLE "user" (
  uid bigint NOT NULL PRIMARY KEY,
  username varchar(32) NOT NULL UNIQUE,
  realname varchar(32) DEFAULT NULL,
  password varchar(128) DEFAULT NULL,
  domain varchar(64) DEFAULT NULL,
  dn varchar(64) DEFAULT '*',
  policy_group int4 DEFAULT 0,
  isadmin smallint DEFAULT 0
);

INSERT INTO "user" (uid,username,password,policy_group,isadmin,domain) VALUES (0,'admin','$1$kkBnp0$L/MILe67UGcvHeFlTAQjR1',0,1,'local');

CREATE TABLE t_email (
  uid bigint NOT NULL,
  email varchar(128) NOT NULL PRIMARY KEY
);

INSERT INTO t_email (uid,email) VALUES (0,'admin@local');

CREATE TABLE t_domain (
  domain varchar(64) NOT NULL PRIMARY KEY,
  mapped varchar(64) NOT NULL
);

INSERT INTO t_domain (domain,mapped) VALUES ('local','local');
INSERT INTO t_domain (domain,mapped) VALUES ('yourdomain.com','yourdomain.com');

CREATE TABLE t_white_list (
  uid bigint NOT NULL PRIMARY KEY,
  whitelist BYTEA DEFAULT NULL
);

INSERT INTO t_white_list (uid) VALUES (0);

CREATE TABLE t_black_list (
  uid bigint NOT NULL PRIMARY KEY,
  blacklist BYTEA DEFAULT NULL
);

INSERT INTO t_black_list (uid) VALUES (0);

CREATE TABLE t_queue (
       id varchar(32) NOT NULL,
       uid bigint NOT NULL,
       ts numeric(20) NOT NULL,
       is_spam smallint DEFAULT 0,
       data BYTEA NOT NULL
);

create index t_queue_idx on t_queue(uid, id);
create index t_queue_idx2 on t_queue(ts);

CREATE TABLE t_stat (
  uid bigint NOT NULL,
  ts numeric(20) NOT NULL,
  nham int4 DEFAULT 0,
  nspam int4 DEFAULT 0
);

create index t_stat_idx on t_stat(uid);


CREATE TABLE t_policy (
  policy_group int4 NOT NULL PRIMARY KEY,
  name varchar(128) NOT NULL,
  deliver_infected_email smallint DEFAULT 0,
  silently_discard_infected_email smallint DEFAULT 1,
  use_antispam smallint DEFAULT 1,
  spam_subject_prefix varchar(128) DEFAULT NULL,
  enable_auto_white_list smallint DEFAULT 1,
  max_message_size_to_filter int4 DEFAULT 128000,
  rbl_domain varchar(128) DEFAULT NULL,
  surbl_domain varchar(128) DEFAULT NULL,
  spam_overall_limit REAL DEFAULT 0.92,
  spaminess_oblivion_limit REAL DEFAULT 1.01,

  replace_junk_characters smallint DEFAULT 1,
  invalid_junk_limit smallint DEFAULT 5,
  invalid_junk_line smallint DEFAULT 1,

  penalize_images smallint DEFAULT 0,
  penalize_embed_images smallint DEFAULT 0,
  penalize_octet_stream smallint DEFAULT 0,

  training_mode smallint DEFAULT 0,
  initial_1000_learning smallint DEFAULT 0,
  store_metadata smallint DEFAULT 0,
  store_only_spam smallint DEFAULT 0,
  message_from_a_zombie smallint DEFAULT 0
);


CREATE TABLE t_minefield (
  ip varchar(15) NOT NULL PRIMARY KEY,
  ts int4 DEFAULT 0
);

CREATE TABLE t_remote (
  remotedomain varchar(64) NOT NULL PRIMARY KEY,
  remotehost varchar(64) NOT NULL,
  basedn varchar(255) NOT NULL,
  binddn varchar(255) NOT NULL,
  sitedescription varchar(64) DEFAULT NULL
);


CREATE TABLE t_transport (
  domain varchar(64) NOT NULL PRIMARY KEY,
  destination varchar(64) NOT NULL
);

CREATE TABLE t_counters (
  rcvd numeric(20) DEFAULT 0,
  ham numeric(20) DEFAULT 0,
  spam numeric(20) DEFAULT 0,
  possible_spam numeric(20) DEFAULT 0,
  unsure numeric(20) DEFAULT 0,
  minefield numeric(20) DEFAULT 0,
  virus numeric(20) DEFAULT 0,
  fp numeric(20) DEFAULT 0,
  fn numeric(20) DEFAULT 0
);

INSERT INTO t_counters VALUES (0,0,0,0,0,0,0,0,0);

-- End.


