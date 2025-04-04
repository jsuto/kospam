#!/bin/bash

set -o nounset
set -o errexit
set -o pipefail

make_certificate() {
   pemfile="$1"
   local subject="$2"

   openssl req -new -newkey rsa:4096 -days 3650 -nodes -x509 -subj "$subject" -keyout "$pemfile" -out 1.cert -sha256 2>/dev/null
   cat 1.cert >> "$pemfile"
   rm -f 1.cert
   chmod 640 "$pemfile"
}

make_certificate "$1" "$2"
