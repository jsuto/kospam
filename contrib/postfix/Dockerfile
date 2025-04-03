FROM alpine:3.21.3

# hadolint ignore=DL3018
RUN apk update && \
    apk add --no-cache postfix rsyslog && \
    rm -rf /var/cache/apk/* && \
    chown 1000:1000 /var/mail && \
    echo "aaa.fu OK" > /etc/postfix/virtual_domains && \
    echo "example.com OK" >> /etc/postfix/virtual_domains && \
    echo "aaa@aaa.fu aaa.fu/aaa/" > /etc/postfix/virtual_users && \
    echo "bbb@example.com example.com/bbb/" >> /etc/postfix/virtual_users && \
    echo "@aaa.fu aaa@aaa.fu" > /etc/postfix/virtual_aliases && \
    echo "@example.com bbb@example.com" > /etc/postfix/virtual_aliases && \
    postmap lmdb:/etc/postfix/virtual_users lmdb:/etc/postfix/virtual_domains lmdb:/etc/postfix/virtual_aliases

COPY run.sh /
COPY main.cf /etc/postfix/main.cf

EXPOSE 25

CMD ["/run.sh"]
