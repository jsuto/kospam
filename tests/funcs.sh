# shellcheck shell=bash

set -o errexit

VERDICT=0
# shellcheck disable=SC2034
RESULT_CRITICAL=2
LOGFILE="mail.log"
PEMFILE="${SCRIPT_DIR}/server.pem"
EML_DIR="${SCRIPT_DIR}/eml"
# shellcheck disable=SC2034
TRAINING_DIR="${SCRIPT_DIR}/training"
# shellcheck disable=SC2034
MAIL_HOST="mail.kospam"
SYSLOG_HOST="syslog.kospam"
SMTPTEST="${SCRIPT_DIR}/smtptest"

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
      processed="$( docker exec "$container" find /var/mail/example.com/bbb/new/ -type f | wc -l )"

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

   [[ -f kospam.sql.gz ]] || "$MC_COMMAND" cp ibm/piler-ci/kospam.sql.gz .

   create_pem_file "$PEMFILE" "/C=US/ST=Denial/L=Springfield/O=Dis/CN=mail.kospam"

   chmod 644 "$PEMFILE"

   ls -la

   chmod +x "$SMTPTEST"
}

start_containers() {
   local compose_file="$1"
   local container="$2"
   local i

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

count_malwares() {
   echo "Counting malwares"

   docker exec "$SYSLOG_HOST" grep -c result=MALWARE "/var/log/${LOGFILE}"
}

print_errors() {
   echo "Getting errors from mail.log"
   docker exec "$SYSLOG_HOST" grep -ri ERROR /var/log/mail.log || true
}

postfix_errors() {
   docker exec "$MAIL_HOST" grep -v -E '(connect from|client=|message-id=|from=|removed|relay=virtual|relay=kospam|statistics)' /var/log/mail.log
}
