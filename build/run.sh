#!/bin/bash

set -o errexit
set -o pipefail
set -o nounset

REPO_ROOT="/repo"
DEBUG="${DEBUG:-false}"
DEBUG_OPTION=""
INSTALL_DIR="/tmp/PACKAGE"
PACKAGE_OUTDIR="/data"


set_mysql_flavour() {
   # shellcheck disable=SC2034
   MYSQL_FLAVOUR="mariadb"
}

get_pkg_name() {
   if [[ "$DEBUG" == true ]]; then
      DEBUG_SUFFIX="-debug"
   else
      DEBUG_SUFFIX=""
   fi

   PACKAGE="${PROJECT_ID}_${VERSION}-${DISTRO}-${COMMIT_ID}${DEBUG_SUFFIX}_${ARCH}.deb"

   rm -f "${PACKAGE_OUTDIR}/${PACKAGE}"
}

make_package() {
   local project="$1"
   local description="$2"

   make_deb_package "$project" "$description"

   # Upload_artifact_to_object_store

   if [[ -v S3_URL && -v S3_ACCESS_KEY && -v S3_SECRET_KEY ]]; then
      echo "Uploading ${PACKAGE} to ${S3_URL}"

      mc alias set s3 "$S3_URL" "$S3_ACCESS_KEY" "$S3_SECRET_KEY"

      mc cp "${PACKAGE_OUTDIR}/${PACKAGE}" "s3/${PROJECT_ID}"
   fi

   # Get rid of the PACKAGE dir
   rm -rf "$INSTALL_DIR"
}

make_deb_package() {
   local project="$1"
   local description="$2"

   pushd "$PACKAGE_OUTDIR"

   fpm \
      --deb-use-file-permissions \
      -s dir \
      -C "$INSTALL_DIR" \
      -t deb \
      -n "$PROJECT_ID" \
      -v "${VERSION}-${DISTRO}" \
      --iteration "${COMMIT_ID}${DEBUG_SUFFIX}" \
      --description "$description" \
      --before-install "${REPO_ROOT}/build/fpm-${project}-before-install.sh" \
      --after-install "${REPO_ROOT}/build/fpm-${project}-after-install.sh"

   popd
}

make_certificate() {
   local pemfile="kospam.pem"
   local config_dir="/etc/kospam"

   log "Making an ssl certificate"
   openssl req -new -newkey rsa:4096 -days 3650 -nodes -x509 -subj "$SSL_CERT_DATA" -keyout "${config_dir}/${pemfile}" -out "${config_dir}/1.cert" -sha256 2>/dev/null
   cat "${config_dir}/1.cert" >> "${config_dir}/${pemfile}"
   rm -f "${config_dir}/1.cert"
   chown root:kospam "${config_dir}/${pemfile}"
   chmod 640 "${config_dir}/${pemfile}"
}

git config --global --add safe.directory /repo

pushd "$REPO_ROOT"

if [[ "$DEBUG" == "true" ]]; then
   DEBUG_OPTION="--enable-debug"
fi

## FIXME

echo "Creating kospam user and group"

groupadd kospam
useradd -g kospam -d /var/kospam -s /bin/bash kospam

set_mysql_flavour

get_pkg_name

###check_if_package_exists_in_object_store

./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var --with-database="$MYSQL_FLAVOUR" "$DEBUG_OPTION"
make clean
make -j2

if [[ -x ./unit_tests/run.sh && "$DISTRO" =~ ^(jammy|noble)$ ]]; then
   make_certificate
   ./unit_tests/run.sh
else
   echo Not running unit tests
fi

make install DESTDIR="$INSTALL_DIR"

# TODO: init.d scripts

make_package "kospam" "K.O. spam filter"
