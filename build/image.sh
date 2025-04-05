#!/bin/bash

set -o nounset
set -o errexit
set -o pipefail

SCRIPT_PATH="$(readlink -f "$0")"
SCRIPT_DIR="${SCRIPT_PATH%/*}"
REPO_ROOT="$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel)"

pushd "$REPO_ROOT"

pushd "go-app/"
export GOOS=linux GOARCH=amd64 CGO_ENABLED=0
go build -o ../kospam-smtpd cmd/smtpd/main.go
go build -o ../kospam-send cmd/send/main.go
popd

docker buildx build --load --target prod -f docker/Dockerfile.alpine -t sutoj/kospam:test .
