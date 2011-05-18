BEGIN;
delete from smtpd where ts < strftime('%s','now')-DAYS_TO_RETAIN*86400;
delete from smtp where ts < strftime('%s','now')-DAYS_TO_RETAIN*86400;
delete from cleanup where ts < strftime('%s','now')-DAYS_TO_RETAIN*86400;
delete from clapf where ts < strftime('%s','now')-DAYS_TO_RETAIN*86400;
delete from qmgr where ts < strftime('%s','now')-DAYS_TO_RETAIN*86400;
COMMIT;

VACUUM;

