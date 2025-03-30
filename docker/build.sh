#!/bin/bash

set -o errexit
set -o pipefail
set -o nounset

if [[ $# -ne 1 ]]; then echo "ERROR: missing package name" 1>&2; exit 1; fi

a="$(cut -f2 -d '-' <<< "$1")"
TAG="${a//ee_}"
IMAGE_NAME="sutoj/kospam:${TAG}"

docker buildx build --load --build-arg PACKAGE="${1%_*}" -t "$IMAGE_NAME" .
#rm -f "$1"
