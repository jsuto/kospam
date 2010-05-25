BEGIN;
delete from smtpd where ts < strftime('%s','now')-604800;
delete from smtp where ts < strftime('%s','now')-604800;
delete from cleanup where ts < strftime('%s','now')-604800;
delete from clapf where ts < strftime('%s','now')-604800;
delete from qmgr where ts < strftime('%s','now')-604800;
COMMIT;

VACUUM;

