BEGIN;
delete from smtpd where ts < UNIX_TIMESTAMP()-604800;
delete from smtp where ts < UNIX_TIMESTAMP()-604800;
delete from cleanup where ts < UNIX_TIMESTAMP()-604800;
delete from clapf where ts < UNIX_TIMESTAMP()-604800;
delete from qmgr where ts < UNIX_TIMESTAMP()-604800;
COMMIT;

