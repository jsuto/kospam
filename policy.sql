create table if not exists t_policy (
	policy_group int(4) not null,
	name char(128) not null,
	deliver_infected_email tinyint default 0,
	silently_discard_infected_email tinyint default 1,
	use_antispam tinyint default 1,
	spam_subject_prefix char(128) default null,
	enable_auto_white_list tinyint default 1,
	max_message_size_to_filter int default 128000,
	rbl_domain char(128) default null,
	surbl_domain char(128) default null,
	spam_overall_limit float default 0.92,
	spaminess_oblivion_limit float default 1.01,

	replace_junk_characters tinyint default 1,
	invalid_junk_limit tinyint default 5,
	invalid_junk_line tinyint default 1,

	penalize_images tinyint default 0,
	penalize_embed_images tinyint default 0,
	penalize_octet_stream tinyint default 0,

	training_mode tinyint default 0,
	initial_1000_learning tinyint default 0,

	unique(policy_group)
);

create index t_policy_idx on t_policy(policy_group);

