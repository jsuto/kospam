#!/bin/sh

set -o nounset
set -o errexit


PEMFILE="/etc/kospam/kospam.pem"
CERT_SUBJECT="/C=US/ST=Illionis/L=Springfield/O=MyCompany/CN=antispam.kospam"

error() {
   echo "ERROR:" "$*" 1>&2
   exit 1
}


log() {
   echo "DEBUG:" "$*"
}


create_pem_file() {
   pemfile="$1"
   subject="$2"

   if [ ! -f "$pemfile" ]; then
      echo "Generate PEM file ${pemfile}"
      openssl req -newkey rsa:4096 -new -nodes -x509 -subj "$subject" -days 3650 -sha256 -keyout "$pemfile" -out "1.crt" 2>/dev/null
      cat "1.crt" >> "$pemfile"
      chown root:kospam "$pemfile"
      chmod 640 "$pemfile"
      rm -f "1.crt"
   fi
}

setup_cnf_file() {
   my_cnf="/root/.my.cnf"

   printf "[client]\nhost = %s\nuser = %s\npassword = %s\n\n[mysqldump]\nhost = %s\nuser = %s\npassword = %s\n" \
          "$MYSQL_HOSTNAME" "$MYSQL_USER" "$MYSQL_PASSWORD" "$MYSQL_HOSTNAME" "$MYSQL_USER" "$MYSQL_PASSWORD" > "$my_cnf"

   chmod 600 "$my_cnf"
}

wait_until_mariadb_server_is_ready() {
   while true; do if echo "show databases" | mariadb -uroot -h"$MYSQL_HOSTNAME" -p"$MYSQL_ROOT_PASSWORD" ; then break; fi; log "${MYSQL_HOSTNAME} is not ready"; sleep 3; done
   log "${MYSQL_HOSTNAME} is ready"
}

check_database() {
   has_kospam_db=0
   has_token_table=0

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
      mariadb -uroot -h"$MYSQL_HOSTNAME" -p"$MYSQL_ROOT_PASSWORD" "$MYSQL_DATABASE" < /usr/share/kospam/db-mysql.sql
   else
      log "token table exists"
   fi
}

if [ ! -f /dev/log ]; then
   log "No /dev/log in the container, starting rsyslogd"
   rsyslogd
fi

if [ ! -f "$PEMFILE" ]; then
   create_pem_file "$PEMFILE" "$CERT_SUBJECT"
fi

wait_until_mariadb_server_is_ready
check_database

setup_cnf_file

if [ -n "${MYSQL_DUMP+x}" ]; then
   gzip -dc "$MYSQL_DUMP" | mariadb "$MYSQL_DATABASE"
fi

/usr/sbin/kospam -d
/usr/libexec/kospam/kospam-smtpd -daemon
/usr/libexec/kospam/kospam-send -daemon

sleep infinity
