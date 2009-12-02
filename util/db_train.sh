#!/bin/sh
##
## db_train.sh, 2009.12.02, SJ
## load ham/spam hashed tokens from scratch
##

if [ $# -ne 4 ]; then echo "usage: $0 <ham> <spam> <mysql|sqlite3> <config file>"; exit 1; fi

export PATH=$PATH:/usr/local/bin:/usr/local/sbin:/usr/local/mysql/bin:YOUR_BINDIR:.:`pwd`/perl:YOUR_LIBEXECDIR
export LC_ALL=C

HAM=$1
SPAM=$2
DBTYPE=$3
CONFIG=$4
NHAM=0
NSPAM=0

TEMP=`pwd`/temp.$$
HASHTEMP=$TEMP.$$


import_tokens(){

   TS1=`date +%s`


   ### sqlite3

   if [ "$DBTYPE" = "sqlite3" ]; then
      echo "Num of ham messages: $NHAM"
      echo "Num of spam messages: $NSPAM"
      echo -n "Importing data . . . "

      prepare-sql ham.tmp spam.tmp | sqlite3 $SQLITEDB
      echo "UPDATE t_misc SET nham=$NHAM, nspam=$NSPAM WHERE uid=0;" | sqlite3 $SQLITEDB
   fi


   ### mysql

   if [ "$DBTYPE" = "mysql" ]; then
      prepare.pl ham.tmp spam.tmp $TEMP
      aphash < $TEMP > $HASHTEMP
      TEMP=$HASHTEMP

      echo "Num of ham messages: $NHAM"
      echo "Num of spam messages: $NSPAM"
      echo "Raw token file: $TEMP"

      echo -n "Loaded tokens in .... "

      echo "LOAD DATA LOCAL INFILE '$TEMP' INTO TABLE t_token FIELDS TERMINATED BY ' '" | mysql -u $MYSQLUSER -p$MYSQLPASSWORD $MYSQLDB

      echo "UPDATE t_misc SET nham=$NHAM, nspam=$NSPAM WHERE uid=0" | mysql -u $MYSQLUSER -p$MYSQLPASSWORD $MYSQLDB

   fi


   ### mydb

   if [ "$DBTYPE" = "mydb" ]; then
      echo "Num of ham messages: $NHAM"
      echo "Num of spam messages: $NSPAM"
      echo -n "Importing data . . . "

      prepare-sql ham.tmp spam.tmp $NHAM $NSPAM $MYDB
   fi


   ###

   TS2=`date +%s`
   echo `expr $TS2 - $TS1` " [sec]"
}


process_maildir(){
   echo -n "parsing HAM directory. . . "
   for i in `\ls $HAM`; do (echo "*** NEW_MSG_STARTS_HERE $NHAM ***"; parsembox $HAM/$i ) | shrink.pl; NHAM=`expr $NHAM + 1`; done > ham.tmp 2>/dev/null

   TS2=`date +%s`
   echo `expr $TS2 - $TS1` " [sec]"

   TS1=$TS2

   echo -n "parsing SPAM directory. . . "
   for i in `\ls $SPAM`; do (echo "*** NEW_MSG_STARTS_HERE $NSPAM ***"; parsembox $SPAM/$i ) | shrink.pl; NSPAM=`expr $NSPAM + 1`; done > spam.tmp 2>/dev/null

   TS2=`date +%s`
   echo `expr $TS2 - $TS1` " [sec]"

   import_tokens;

}

process_mbox(){

   echo -n "parsing HAM . . . "
   parsembox $HAM | shrink.pl > ham.tmp 2>num_of_ham.tmp

   TS2=`date +%s`
   echo `expr $TS2 - $TS1` " [sec]"

   TS1=$TS2

   echo -n "parsing SPAM . . . "
   parsembox $SPAM | shrink.pl > spam.tmp 2>num_of_spam.tmp

   TS2=`date +%s`
   echo `expr $TS2 - $TS1` " [sec]"

   NHAM=`cat num_of_ham.tmp`
   NSPAM=`cat num_of_spam.tmp`

   import_tokens;

}



SQLITEDB=`grep ^sqlite3= $CONFIG | cut -f2 -d '='`

MYDB=`grep ^mydbfile= $CONFIG | cut -f2 -d '='`

MYSQLDB=`grep ^mysqldb= $CONFIG | cut -f2 -d '='`
MYSQLUSER=`grep ^mysqluser= $CONFIG | cut -f2 -d '='`
MYSQLPASSWORD=`grep ^mysqlpwd= $CONFIG | cut -f2 -d '='`
MYSQLHOST=`grep ^mysqlhost= $CONFIG | cut -f2 -d '='`
MYSQLPORT=`grep ^mysqlport= $CONFIG | cut -f2 -d '='`
MYSQLSOCKET=`grep ^mysqlsocket= $CONFIG | cut -f2 -d '='`


rm -f ham.tmp spam.tmp temp.* num_of_ham.tmp num_of_spam.tmp
TS1=`date +%s`


if [ -d $HAM ]; then process_maildir; exit 1; fi
if [ -f $HAM ]; then process_mbox; exit 1; fi

echo "ham and spam should be either (Maildir) directories or mbox format files"
exit 1;

