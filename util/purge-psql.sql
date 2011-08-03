BEGIN;
DELETE FROM t_token WHERE nham+nspam = 1 AND timestamp < round(date_part('epoch',now()))-1296000;
DELETE FROM t_token WHERE (2*nham)+nspam < 5 AND timestamp < round(date_part('epoch',now()))-5184000;
DELETE FROM t_token WHERE timestamp < round(date_part('epoch',now()))-7776000;
DELETE FROM t_minefield WHERE ts < round(date_part('epoch',now()))-86400;
COMMIT;
