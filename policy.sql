create table if not exists t_policy (
	policy_group int not null,
	name char(128) not null,
	deliver_infected_email int default 0,
	silently_discard_infected_email int default 1,
	use_antispam int default 1,
	spam_subject_prefix char(128) default null,
	enable_auto_white_list int default 1,
	max_message_size_to_filter int default 128000,
	rbl_domain char(128) default null,
	surbl_domain char(128) default null,
	spam_overall_limit float default 0.92,
	spaminess_oblivion_limit float default 1.01,

	replace_junk_characters int default 1,
	invalid_junk_limit int default 5,
	invalid_junk_line int default 1,

	penalize_images int default 0,
	penalize_embed_images int default 0,
	penalize_octet_stream int default 0,

	training_mode int default 0,
	initial_1000_learning int default 0,

	unique(policy_group)
);

create index t_policy_idx on t_policy(policy_group);

