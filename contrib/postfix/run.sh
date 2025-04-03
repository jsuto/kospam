#!/bin/sh

cp /etc/resolv.conf /var/spool/postfix/etc/
rsyslogd
postfix start
sleep infinity
