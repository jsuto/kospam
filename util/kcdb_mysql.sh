#!/bin/sh
##
## kcdb_mysql.sh, 2007.06.12, SJ
## create the MySQL tables and load ham/spam tokens from scratch
##

if [ $# -lt 3 ]; then echo "usage: $0 <ham> <spam> <my.cnf> [nohash]"; exit 1; fi
#if [ $# -ne 3 ]; then echo "usage: $0 <ham> <spam> <my.cnf>"; exit 1; fi

export PATH=$PATH:/usr/local/bin:.:`pwd`/perl
export LC_ALL=C

HAM=$1
SPAM=$2
MYCNF=$3
HASHED=$4
NHAM=0
NSPAM=0
TEMP=`pwd`/temp.$$
HASHTEMP=$TEMP.$$

TS1=`date +%s`

rm -f ham.tmp spam.tmp temp.* tokens*.cdb num_of_ham.tmp num_of_spam.tmp

import_tokens(){
   rm -f $TEMP

   prepare.pl ham.tmp spam.tmp $TEMP

   if test "$HASHED" = "nohash";
   then
      echo "not using hashed tokens..."
   else
      aphash < $TEMP > $HASHTEMP
      TEMP=$HASHTEMP
   fi

   echo "Num of ham messages: $NHAM"
   echo "Num of spam messages: $NSPAM"
   echo "Raw token file: $TEMP"

   echo

   if [ -f db.sql ]; then
      mysql --defaults-file=$MYCNF < db.sql
   fi

   #echo "LOAD DATA LOCAL INFILE '$TEMP' INTO TABLE t_token FIELDS TERMINATED BY ' '" | mysql --defaults-file=$MYCNF
   echo "LOAD DATA INFILE '$TEMP' INTO TABLE t_token FIELDS TERMINATED BY ' '" | mysql --defaults-file=$MYCNF

   echo "INSERT INTO t_misc (update_cdb, nham, nspam, uid) VALUES(0, $NHAM, $NSPAM, 0)" | mysql --defaults-file=$MYCNF
}

create_cdb_file(){
   echo "select token, nham, nspam from t_token where nham > 1 OR nspam > 1 " | mysql --defaults-file=$MYCNF | grep [0-9] | mysql2cdb.pl $NHAM $NSPAM tokens
}

process_maildir(){
   echo -n "parsing HAM directory. . . "
   for i in `\ls $HAM`; do (echo "*** NEW_MSG_STARTS_HERE $NHAM ***"; parsembox $HAM/$i ) | shrink.pl; NHAM=`expr $NHAM + 1`; done > ham.tmp

   TS2=`date +%s`
   echo `expr $TS2 - $TS1` " [sec]"

   TS1=$TS2

   echo -n "parsing SPAM directory. . . "
   for i in `\ls $SPAM`; do (echo "*** NEW_MSG_STARTS_HERE $NSPAM ***"; parsembox $SPAM/$i ) | shrink.pl; NSPAM=`expr $NSPAM + 1`; done > spam.tmp

   TS2=`date +%s`
   echo `expr $TS2 - $TS1` " [sec]"

   import_tokens;

   if [ -x `which cdb 2>/dev/null` ]; then create_cdb_file; fi

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

   if [ -x `which cdb 2>/dev/null` ]; then create_cdb_file; fi

}


if [ -d $HAM ]; then process_maildir; exit 1; fi
if [ -f $HAM ]; then process_mbox; exit 1; fi

echo "ham and spam should be either (Maildir) directories or mbox format files"
exit 1;

