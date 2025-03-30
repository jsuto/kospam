#!/bin/bash

set -o errexit
set -o pipefail
set -o nounset

NAME="kospam"

[[ $(id -g "$NAME" 2>/dev/null) ]] || groupadd "$NAME"
[[ $(id -u "$NAME" 2>/dev/null) ]] || useradd -g "$NAME" -d "/var/${NAME}" -s /bin/bash "$NAME"
