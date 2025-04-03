FROM sutoj/alpine:builder AS build

COPY . /repo
RUN /run.sh

FROM alpine:3.21.3 AS prod

LABEL description="kospam alpine image" \
      maintainer="Janos SUTO, sj@acts.hu"

COPY kospam-send /usr/libexec/kospam/kospam-send
COPY kospam-smtpd /usr/libexec/kospam/kospam-smtpd

# hadolint ignore=DL3008,DL3015,DL3018
RUN apk update && \
    apk add --no-cache openssl zlib libzip && \
    addgroup -g 1000 kospam && \
    adduser -h /var/kospam -G kospam -u 1000 -D kospam && \
    chmod +x /usr/libexec/kospam/kospam-send /usr/libexec/kospam/kospam-smtpd && \
    mkdir -p /var/kospam/tmp /var/kospam/send /var/kospam/send-error && \
    chown kospam:kospam /var/kospam/tmp /var/kospam/send /var/kospam/send-error

COPY --chown=root:root --from=build /usr/local/lib/libtre.so.5 /usr/local/lib/libtre.so.5
COPY --chown=root:root --from=build /usr/lib/libmariadb.so.3 /usr/lib/libmariadb.so.3
COPY --chown=root:root --from=build /usr/lib/libkospam.so.0.1.1 /usr/lib/libkospam.so
COPY --chown=root:root --from=build /usr/sbin/kospam /usr/sbin
COPY --chown=root:root --from=build /usr/sbin/kospamconf /usr/sbin
COPY --chown=root:root --from=build /usr/bin/spamdrop /usr/bin
COPY --chown=root:root --from=build /etc/kospam/kospam.conf.dist /etc/kospam/
COPY --chown=root:root --from=build /usr/share/kospam/zombienets.regex.dist /usr/share/kospam/zombienets.regex.dist
