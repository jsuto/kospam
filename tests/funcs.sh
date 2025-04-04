# shellcheck shell=bash

set -o errexit

VERDICT=0
###RESULT_CRITICAL=2
# shellcheck disable=SC2034
PEMFILE="${SCRIPT_DIR}/server.pem"
LOGFILE="mail.log"
EML_DIR="${SCRIPT_DIR}/eml"
##SYSLOG_HOST="syslog.host"

error() {
   echo "$@"
   exit 1
}

set_verdict() {
   [[ ${VERDICT} -ge "$1" ]] || { VERDICT="$1"; echo "verdict: ${VERDICT}"; }
}

get_verdict() {
   echo "verdict: ${VERDICT}"
   exit "$VERDICT"
}

create_pem_file() {
   local pemfile="$1"
   local subject="$2"

   if [[ ! -f "$pemfile" ]]; then
      echo "Generate PEM file"
      openssl req -newkey rsa:4096 -new -nodes -x509 -subj "$subject" -days 3650 -sha256 -keyout "$pemfile" -out "1.crt" 2>/dev/null
      cat "1.crt" >> "$pemfile"
      rm -f "1.crt"
   fi
}

wait_until_emails_are_processed() {
   local container="$1"
   local num=$2
   local loops
   local i=0
   local processed=0

   echo "${FUNCNAME[0]}"

   loops=$(( num / 100 ))

   while true; do
      processed="$( docker exec "$container" grep -c -E 'piler/store.*status=' "/var/log/${LOGFILE}" )"
      i=$(( i + 1 ))
      echo "processed ${processed} messages"

      [[ $processed -lt "$num" ]] || break

      sleep 5

      [[ $i -gt $loops ]] && error "${container} did not process ${num} emails"
   done

   docker exec "$container" tail "/var/log/${LOGFILE}"

   echo "${processed} emails are processed in ${container}"
}

prepare() {
   if command -v mc; then
      MC_COMMAND="$( command -v mc )"
   else
      curl -L -o mc https://dl.min.io/client/mc/release/linux-amd64/mc
      chmod +x mc
      MC_COMMAND="${PWD}/mc"
      "$MC_COMMAND" alias set ibm "$MINIO_URL" "$MINIO_USER" "$MINIO_PASSWORD"
   fi

   if [[ ! -d "$EML_DIR" ]]; then
      "$MC_COMMAND" cp ibm/piler-ci/piler-test-files.tar.xz .
      tar Jxf piler-test-files.tar.xz
   fi

   create_pem_file "$PEMFILE" "/C=US/ST=Denial/L=Springfield/O=Dis/CN=mail.kospam"
}

start_containers() {
   local compose_file="$1"
   local container="$2"
   local i

   if [[ -v BUILD_NUMBER ]]; then setup_compose_files; fi

   docker compose -f "$compose_file" up -d
   i=0
   while [[ "$(docker inspect -f '{{.State.Health.Status}}' "$container")" != "healthy" ]]; do
      i=$((i+1))
      sleep 2
      if [[ $i -gt 45 ]]; then
         echo "sleep is still not running inside ${container}"
         docker logs "$container"
         docker exec "$container" cat "/var/log/${LOGFILE}"
         exit 1
      fi
   done

   docker compose -f "$compose_file" ps
}

print_errors() {
   echo "Getting errors from mail.log"
   docker exec syslog.host grep -ri ERROR /var/log/mail.log || true
}
