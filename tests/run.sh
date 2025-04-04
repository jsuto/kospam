#!/bin/bash

set -o nounset
set -o errexit
set -o pipefail

SCRIPT_PATH="$(readlink -f "$0")"
SCRIPT_DIR="${SCRIPT_PATH%/*}"
S3_HOST="${S3_HOST:-minio}"

if [[ -v GITHUB_RUN_ID ]]; then
   export BUILD_NUMBER="$GITHUB_RUN_ID"
fi

export S3="${S3:-false}"
export TESTCASE="${TESTCASE:-case3}"

echo "Running testcase: ${TESTCASE} on tag ${VERSION:-test}"

pushd "$SCRIPT_DIR"

# shellcheck disable=SC1091
source "funcs.sh"

prepare

case "$TESTCASE" in
   case1|case2|case3|case4|case5|case6)
         # shellcheck disable=SC1090
         source "${TESTCASE}.sh"
   ;;

   *)
         echo "Invalid test case: ${TESTCASE}"
         set_verdict "$RESULT_CRITICAL"
   ;;
esac

popd

get_verdict
