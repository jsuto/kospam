# shellcheck shell=bash

export CONTAINER="kospam"

# shellcheck disable=SC2016
mysql_cmd=( docker exec -i "$CONTAINER" sh -c 'mariadb -h "$MYSQL_HOSTNAME" -u "$MYSQL_USER" -p"$MYSQL_PASSWORD" "$MYSQL_DATABASE"' )

send_emails() {
   echo "Sending emails"

   echo "insert into attach_digest (digest) values('be2d9c8e1fee1123c60f367b286cd497585a0ddf57ea8d86a4df807defdefdf0'),('8964293eb4cd0aa049b9763566fbb8497eeac35decd6cfd2baa9fc5b9dfbf0a6'),('84e641b91ba52636b5013df6a1cc0937a0c4cec70bc7e8d9f2b93233d82afce9')" | "${mysql_cmd[@]}"

   "$SMTPTEST" -smtpaddr 127.0.0.1:22225 -dir "$EML_DIR"

   docker exec "$SYSLOG_HOST" tail -20 /var/log/mail.log

   wait_until_emails_are_processed "$MAIL_HOST" 3021

   docker exec kospam find /var/kospam

   echo "select * from history limit 10" | "${mysql_cmd[@]}"
   echo "select * from token order by updated desc limit 10" | "${mysql_cmd[@]}"
}

training() {
   echo "insert into history (kospam_id, ts, spam) values ('40000000600790f3210ece8400cb72d324e3', 1744277737, 0)" | "${mysql_cmd[@]}"

   tokens_before_training="$( echo "select count(*) from token" | "${mysql_cmd[@]}" | tail -1 )"
   echo "Tokens before training: ${tokens_before_training}"
   "$SMTPTEST" -smtpaddr 127.0.0.1:10025 -dir "$TRAINING_DIR" -recipient spam@example.com
   docker cp "${TRAINING_DIR}/1.eml" "${CONTAINER}:/tmp"

   sleep 3

   docker exec -w /tmp "$CONTAINER" spamdrop -D -m 1.eml
   tokens_after_training="$( echo "select count(*) from token" | "${mysql_cmd[@]}" | tail -1 )"
   echo "Tokens after training ${tokens_after_training}"
   [[ $tokens_after_training -gt $(( tokens_before_training + 10 )) ]] || set_verdict "$RESULT_CRITICAL"
}

start_containers "docker-compose.yaml" "$CONTAINER"

send_emails
training

count_malwares

print_errors

postfix_errors

get_verdict
