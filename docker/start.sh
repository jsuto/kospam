#!/bin/bash

set -o nounset
set -o errexit
set -o pipefail

error() {
   echo "ERROR:" "$*" 1>&2
   exit 1
}


log() {
   echo "DEBUG:" "$*"
}


wait_until_mysql_server_is_ready() {
   while true; do if mysql -uroot -h"$MYSQL_HOSTNAME" -p"$MYSQL_ROOT_PASSWORD" <<< "show databases"; then break; fi; log "${MYSQL_HOSTNAME} is not ready"; sleep 3; done

   log "${MYSQL_HOSTNAME} is ready"
}

check_database() {
   local has_kospam_db=0
   local has_token_table=0
   local s

   log "checking databases"

   while read -r s; do
      if [[ "$s" == "$MYSQL_DATABASE" ]]; then has_kospam_db=1; fi
   done < <(mysql -uroot -h"$MYSQL_HOSTNAME" -p"$MYSQL_ROOT_PASSWORD" <<< 'show databases')

   if [[ $has_kospam_db -eq 0 ]]; then
      log "no ${MYSQL_DATABASE} database, creating it"
      mysql -u root -h"$MYSQL_HOSTNAME" -p"$MYSQL_ROOT_PASSWORD" <<< "create database ${MYSQL_DATABASE} character set 'utf8'; grant all privileges on ${MYSQL_DATABASE}.* to ${MYSQL_USER} identified by '${MYSQL_PASSWORD}'; flush privileges;"
   fi

   while read -r s; do
      if [[ "$s" == token ]]; then has_token_table=1; fi
   done < <(mysql -uroot -h"$MYSQL_HOSTNAME" -p"$MYSQL_ROOT_PASSWORD" "$MYSQL_DATABASE" <<< 'show tables')

   if [[ $has_token_table -eq 0 ]]; then
      log "no token table, creating tables"
      mysql -uroot -h"$MYSQL_HOSTNAME" -p"$MYSQL_ROOT_PASSWORD" "$MYSQL_DATABASE" < /usr/share/kospam/db-mysql.sql
   else
      log "token table exists"
   fi
}

rsyslogd

wait_until_mysql_server_is_ready
check_database

kospam -d

sleep infinity
