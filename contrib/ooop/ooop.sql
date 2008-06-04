create table if not exists pop3_gw_users (
   login char(64) not null primary key,
   pwd char(32) not null,

   email char(128) not null unique,

   registration_date int default 0,
   activation_date int default 0,
   secret char(32) default null,

   max_message_size_to_filter int default 128000,
   spam_subject_prefix char(16) default null,

   rbl_condemns_a_message int default 1,
   surbl_condemns_a_message int default 1,

   has_personal_db int default 0,

   whitelist blob default null,

   total_ham int default 0,
   total_spam int default 0,
   fp int default 0,
   fn int default 0
);

create index pop3_gw_users_idx on pop3_gw_users (email, login);
