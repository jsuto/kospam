#!/bin/sh
##
## purge-sqlite3.sh, 2009.09.11, SJ
##

if [ $# -ne 1 ]; then echo "usage: $0 <SQLite3 database>"; exit 1; fi

SQLITE3=sqlite3
DB=$1
NOW=`date +%s`

MINEFIELD_TTL=86400

_7_DAYS=`expr $NOW - 604800`
_15_DAYS=`expr $NOW - 1296000`
_60_DAYS=`expr $NOW - 5184000`
_90_DAYS=`expr $NOW - 7776000`

# remove unused tokens
echo "DELETE FROM t_token WHERE nham+nspam = 1 AND timestamp < $_15_DAYS;" | $SQLITE3 $DB
echo "DELETE FROM t_token WHERE (2*nham)+nspam < 5 AND timestamp < $_60_DAYS;" | $SQLITE3 $DB
echo "DELETE FROM t_token WHERE timestamp < $_90_DAYS;" | $SQLITE3 $DB

# purge aged entries from the t_minefield table
echo "DELETE FROM t_minefield WHERE ts < (SELECT strftime('%s','now')-$MINEFIELD_TTL);" | $SQLITE3 $DB

echo "VACUUM;" | $SQLITE3 $DB
