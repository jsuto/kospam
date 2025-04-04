# shellcheck shell=bash

export CONTAINER="kospam"

start_containers "${TESTCASE}.yaml" "$CONTAINER"
