# shellcheck shell=bash

export CONTAINER="kospam"

start_containers "${TESTCASE}.yaml" "$CONTAINER"

./smtptest 127.0.0.1:10025 "$EML_DIR"
