#!/bin/sh
##
## db_init_cdb.sh, 2007.08.10, SJ
## create the CDB file from scratch
##

if [ $# -ne 2 ]; then echo "usage: $0 <ham> <spam>"; exit 1; fi

export PATH=$PATH:/usr/local/bin:.:`pwd`/perl

HAM=$1
SPAM=$2
NHAM=0
NSPAM=0

TS1=`date +%s`

rm -f ham.tmp spam.tmp tokens*.cdb num_of_ham.tmp num_of_spam.tmp


create_cdb_file(){
   createcdb.pl ham.tmp spam.tmp $NHAM $NSPAM tokens
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

   create_cdb_file;
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

   create_cdb_file;
}


if [ -d $HAM ]; then process_maildir; exit 1; fi
if [ -f $HAM ]; then process_mbox; exit 1; fi

echo "ham and spam should be either (Maildir) directories or mbox format files"
exit 1;

