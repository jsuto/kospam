#!/bin/sh
##
## add a user to the spam quarantine, 2007.05.09, SJ
##

# edit these variables!

SPAMQUARANTINE_DIR=/opt/av/spamquarantine
HTTPD_GROUP=httpd
HTPASSWD_FILE=/var/www/cgi-bin/spamquarantine/.htpasswd
MY_CNF=/usr/local/etc/sq.cnf

if [ $# -ne 3 ]; then echo "usage: $0 <username> <password> <email address>"; exit 1; fi

USER=$1
PWD=`perl -e '@salt = ("A".."Z", "a".."z", 0..9); $so = join("", @salt[rand @salt, rand @salt]); print crypt($2, $so);'`
EMAIL=$3

# create directory

mkdir $SPAMQUARANTINE_DIR/$USER
chown $USER:$HTTPD_GROUP $SPAMQUARANTINE_DIR/$USER
chmod 770 $SPAMQUARANTINE_DIR/$USER

# add .htpasswd entry

echo "$USER:$PWD" >> $HTPASSWD_FILE

# add default mysql entry

echo "INSERT INTO user (email, action, pagelen, username) VALUES('$EMAIL', 'quarantine', 30, '$USER')" | mysql --defaults-file=$MY_CNF

# add .mailfilter entry
echo "if(/^X-Clapf-spamicity: Yes/:h)" >> ~$USER/.mailfilter
echo "{" >> ~$USER/.mailfilter
echo "    to \"|/usr/local/bin/passmail\"" >> ~$USER/.mailfilter
echo "}" >> ~$USER/.mailfilter
echo >> ~$USER/.mailfilter
echo "to \"Mailbox\"" >> ~$USER/.mailfilter

chown $USER ~$USER/.mailfilter
chmod 600 ~$USER/.mailfilter

echo "Done."
