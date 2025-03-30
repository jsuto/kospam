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

rsyslogd

wait_until_mysql_server_is_ready

sleep infinity
