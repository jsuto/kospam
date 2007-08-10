#!/bin/sh
##
## db_init_sqlite3.sh, 2007.08.10, SJ
## create the SQLite3 tables and load ham/spam tokens from scratch
##

if [ $# -ne 3 ]; then echo "usage: $0 <ham> <spam> <database>"; exit 1; fi

export PATH=$PATH:/usr/local/bin:.:`pwd`/perl
export LC_ALL=C

HAM=$1
SPAM=$2
DB=$3
NHAM=0
NSPAM=0
TEMP=`pwd`/temp.$$

TS1=`date +%s`

rm -f ham.tmp spam.tmp temp.* $DB num_of_ham.tmp num_of_spam.tmp

import_tokens(){
   rm -f $TEMP

   sqlite3.pl ham.tmp spam.tmp $TEMP

   echo "Num of ham messages: $NHAM"
   echo "Num of spam messages: $NSPAM"
   echo "Raw token file: $TEMP"

   echo

   if [ -f db-sqlite3.sql ]; then
      sqlite3 $DB < db-sqlite3.sql
   fi

   echo "INSERT INTO t_misc (nham, nspam, uid) VALUES($NHAM, $NSPAM, 0);" | sqlite3 $DB

   cat $TEMP | sqlite3 $DB

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


if [ -d $HAM ]; then process_maildir; exit 1; fi
if [ -f $HAM ]; then process_mbox; exit 1; fi

echo "ham and spam should be either (Maildir) directories or mbox format files"
exit 1;

