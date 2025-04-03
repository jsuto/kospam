#!/bin/bash

set -o nounset
set -o errexit
set -o pipefail

pemfile="server.pem"

SSL_CERT_DATA="/C=US/ST=Denial/L=Springfield/O=Dis/CN=mail.kospam"

make_certificate() {
   openssl req -new -newkey rsa:4096 -days 3650 -nodes -x509 -subj "$SSL_CERT_DATA" -keyout "$pemfile" -out 1.cert -sha256 2>/dev/null
   cat 1.cert >> "$pemfile"
   rm -f 1.cert
   chmod 640 "$pemfile"
}

make_certificate
