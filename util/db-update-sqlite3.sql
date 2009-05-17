create table t_email (uid int unsigned not null, email char(128) not null);
create index t_email_idx on t_email(email);

insert into t_email (uid, email) select uid, email from user;             



create table if not exists bbbb (
        uid int unsigned not null,
        username char(32) not null,
        password char(48) default null,
        policy_group int(4) default 0,
        isadmin tinyint default 0,
        unique (uid, username)
);


insert into bbbb (uid, username, password, policy_group, isadmin) select distinct uid, username, password, policy_group, isadmin from user;

alter table user rename to user_old;

alter table bbbb rename to user;

drop index user_idx;
create index user_idx on user (uid, username);
