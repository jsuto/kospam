create table if not exists pop_gw_users (
   email char(64) not null primary key,
   login char(64) not null,
   max_message_size_to_filter int default 128000,
   spam_subject_prefix char(16) default null,
   do_rbl_check int default 1,
   do_surbl_check int default 0,
   do_bayes_check int default 1,
   do_spamsum_check int default 0,
   autolearn int default 0,
   do_virus_check int default 1,
   whitelist blob default null
);

create index pop_gw_users_idx on pop_gw_users (email, login);
