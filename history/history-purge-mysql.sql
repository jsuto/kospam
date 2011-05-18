BEGIN;
delete from smtpd where ts < UNIX_TIMESTAMP()-DAYS_TO_RETAIN*86400;
delete from smtp where ts < UNIX_TIMESTAMP()-DAYS_TO_RETAIN*86400;
delete from cleanup where ts < UNIX_TIMESTAMP()-DAYS_TO_RETAIN*86400;
delete from clapf where ts < UNIX_TIMESTAMP()-DAYS_TO_RETAIN*86400;
delete from qmgr where ts < UNIX_TIMESTAMP()-DAYS_TO_RETAIN*86400;
COMMIT;

