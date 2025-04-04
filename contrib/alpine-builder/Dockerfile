FROM alpine:3.21.3

# hadolint ignore=DL3018
RUN apk update && \
    apk add --no-cache unzip openssl curl musl-dev make gcc git openssl-dev zlib-dev libzip-dev mariadb-dev && \
    addgroup -g 1000 kospam && \
    adduser -h /var/kospam -G kospam -u 1000 -D kospam

ADD tre-0.9.0-alpine-bin.tar.gz /
COPY run.sh /run.sh

CMD ["/run.sh"]
