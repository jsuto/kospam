#!/bin/bash

set -o errexit
set -o pipefail
set -o nounset

config_dir=/etc/kospam
pemfile="kospam.pem"

SSL_CERT_DATA="/C=US/ST=Denial/L=Springfield/O=Dis/CN=antispam.example.com"


error() {
   echo "ERROR:" "$*" 1>&2
   exit 1
}


log() {
   echo "DEBUG:" "$*"
}


make_certificate() {
   log "Making an ssl certificate"
   openssl req -new -newkey rsa:4096 -days 3650 -nodes -x509 -subj "$SSL_CERT_DATA" -keyout "${config_dir}/${pemfile}" -out "${config_dir}/1.cert" -sha256 2>/dev/null
   cat "${config_dir}/1.cert" >> "${config_dir}/${pemfile}"
   rm -f "${config_dir}/1.cert"
   chown root:kospam "${config_dir}/${pemfile}"
   chmod 640 "${config_dir}/${pemfile}"
}


[[ -f "${config_dir}/${pemfile}" ]] || make_certificate

chown kospam:kospam /var/kospam/error /var/kospam/tmp /var/kospam/run

## TODO: systemd services

log "postinstall has finished"
