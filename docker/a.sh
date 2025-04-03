#!/bin/sh

set -o nounset
set -o errexit


error() {
   echo "ERROR:" "$*" 1>&2
   exit 1
}


log() {
   echo "DEBUG:" "$*"
}


wait_until_mariadb_server_is_ready() {
   has_kospam_db=0
   has_token_table=0

   while true; do if echo "show databases" | mariadb -uroot -h"$MYSQL_HOSTNAME" -p"$MYSQL_ROOT_PASSWORD" ; then break; fi; log "${MYSQL_HOSTNAME} is not ready"; sleep 3; done

   log "${MYSQL_HOSTNAME} is ready"

   for i in $(echo "show databases" | mariadb -uroot -h"$MYSQL_HOSTNAME" -p"$MYSQL_ROOT_PASSWORD" "$MYSQL_DATABASE" ); do
      if [ "$i" = "$MYSQL_DATABASE" ]; then has_kospam_db=1; fi
   done

   if [ $has_kospam_db -eq 0 ]; then
      log "no ${MYSQL_DATABASE} database, creating it"
      echo "create database ${MYSQL_DATABASE} character set 'utf8'; grant all privileges on ${MYSQL_DATABASE}.* to ${MYSQL_USER} identified by '${MYSQL_PASSWORD}'; flush privileges;" | mariadb -u root -h"$MYSQL_HOSTNAME" -p"$MYSQL_ROOT_PASSWORD"
   fi

   for i in $( echo 'show tables' | mariadb -uroot -h"$MYSQL_HOSTNAME" -p"$MYSQL_ROOT_PASSWORD" "$MYSQL_DATABASE" ); do
      if [ "$i" = token ]; then has_token_table=1; fi
   done

   if [ $has_token_table -eq 0 ]; then
      log "no token table, creating tables"
      mariadb -uroot -h"$MYSQL_HOSTNAME" -p"$MYSQL_ROOT_PASSWORD" "$MYSQL_DATABASE" < /usr/share/kospam/db-mariadb.sql
   else
      log "token table exists"
   fi
}

rsyslogd

wait_until_mariadb_server_is_ready

if [ -f /kospam.sql.gz ]; then
   gzip -dc /kospam.sql.gz | mariadb -u root -h"$MYSQL_HOSTNAME" -p"$MYSQL_ROOT_PASSWORD" "$MYSQL_DATABASE"
fi

/usr/sbin/kospam -d
/usr/libexec/kospam/kospam-smtpd -daemon
/usr/libexec/kospam/kospam-send -daemon

sleep infinity
