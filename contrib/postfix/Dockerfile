FROM alpine:3.21.3

# hadolint ignore=DL3018
RUN apk update && \
    apk add --no-cache postfix rsyslog && \
    rm -rf /var/cache/apk/* && \
    chown 1000:1000 /var/mail && \
    echo "aaa.fu OK" > /etc/postfix/virtual_domains && \
    echo "aaa@aaa.fu aaa.fu/aaa/" > /etc/postfix/virtual_users && \
    echo "bbb@aaa.fu aaa.fu/bbb/" >> /etc/postfix/virtual_users && \
    postmap lmdb:/etc/postfix/virtual_users lmdb:/etc/postfix/virtual_domains

COPY run.sh /
COPY main.cf /etc/postfix/main.cf

EXPOSE 25

CMD ["/run.sh"]
