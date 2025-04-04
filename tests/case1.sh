# shellcheck shell=bash

export CONTAINER="kospam"

start_containers "${TESTCASE}.yaml" "$CONTAINER"

"$SMTPTEST" -smtpaddr 127.0.0.1:10025 -dir "$EML_DIR"

docker exec "$SYSLOG_HOST" tail -50 /var/log/mail.log

wait_until_emails_are_processed "$MAIL_HOST" 3021

docker exec kospam find /var/kospam

print_errors
