create table if not exists pop_gw_users (
   login char(64) not null primary key,
   pwd char(32) not null,

   email char(128) not null unique,
   registration_date int default 0,
   activation_date int default 0,

   max_message_size_to_filter int default 128000,
   spam_subject_prefix char(16) default null,

   do_virus_check int default 1,
   do_rbl_check int default 1,
   do_surbl_check int default 0,
   do_bayes_check int default 1,
   do_spamsum_check int default 0,

   rbl_condemns_a_message int default 1,
   surbl_condemns_a_message int default 1,

   autolearn int default 0,

   whitelist blob default null,

   total_ham int default 0,
   total_spam int default 0,
   fp int default 0,
   fn int default 0
);

create index pop_gw_users_idx on pop_gw_users (email, login);
