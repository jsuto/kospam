#!/bin/sh
##
## mysql2cdb.sh, 2007.01.08, SJ
## create CDB file from MySQL database
##

if [ $# -lt 2 ]; then echo "usage: $0 <token> <my.cnf> [x]"; exit 1; fi

export PATH=$PATH:/usr/local/bin:.:`pwd`/perl

TOKEN=$1
MYCNF=$2


UPDATECDB=`echo "select update_cdb from t_misc" | mysql --defaults-file=$MYCNF | tail -1`

if test $UPDATECDB -eq 1 || test "$3" = "x" ; then

	NHAM=`echo "select nham from t_misc" | mysql --defaults-file=$MYCNF | tail -1`
	NSPAM=`echo "select nspam from t_misc" | mysql --defaults-file=$MYCNF | tail -1`

	rm -f $TOKEN*cdb

	echo "select token, nham, nspam from t_token where nspam > 1 OR nham > 1 " | mysql --defaults-file=$MYCNF | grep [0-9] | mysql2cdb.pl $NHAM $NSPAM $TOKEN
	echo "update t_misc set update_cdb=0" | mysql --defaults-file=$MYCNF

fi

