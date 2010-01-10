BEGIN;
DELETE FROM t_token WHERE nham+nspam = 1 AND timestamp < UNIX_TIMESTAMP()-1296000;
DELETE FROM t_token WHERE (2*nham)+nspam < 5 AND timestamp < UNIX_TIMESTAMP()-5184000;
DELETE FROM t_token WHERE timestamp < UNIX_TIMESTAMP()-7776000;
COMMIT;

DELETE FROM t_minefield WHERE ts < (SELECT unix_timestamp() - 86400);
