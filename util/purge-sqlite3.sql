BEGIN;
-- 1296000 -> 15 days
DELETE FROM t_token WHERE nham+nspam = 1 AND timestamp < strftime('%s','now')-1296000;
-- 5184000 -> 60 days
DELETE FROM t_token WHERE (2*nham)+nspam < 5 AND timestamp < strftime('%s','now')-5184000;
-- 7776000 -> 90 days
DELETE FROM t_token WHERE timestamp < strftime('%s','now')-7776000;
-- purge aged entries from the t_minefield table
-- 86400 -> 1 day
DELETE FROM t_minefield WHERE ts < strftime('%s','now')-86400;
COMMIT;
VACUUM;
