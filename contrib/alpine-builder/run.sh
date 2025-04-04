#!/bin/sh

set -o nounset
set -o errexit

cd /repo
git config --global --add safe.directory /repo

./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var
make clean all
cd src
make test
cd ..
make install
