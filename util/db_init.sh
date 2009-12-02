#!/bin/sh
##
## db_init.sh, 2009.12.02, SJ
## create the database tables
##

if [ $# -ne 2 ]; then echo "usage: $0 <mysql|sqlite3> <config file>"; exit 1; fi

export PATH=$PATH:/usr/local/bin:/usr/local/sbin:/usr/local/mysql/bin

DBTYPE=$1
CONFIG=$2

SQLSCRIPT=/usr/local/share/clapf/db.sql
SQLSCRIPT=~/devel/clapf/db-sqlite3.sql

SQLITEDB=`grep ^sqlite3= $CONFIG | cut -f2 -d '='`

MYSQLDB=`grep ^mysqldb= $CONFIG | cut -f2 -d '='`
MYSQLUSER=`grep ^mysqluser= $CONFIG | cut -f2 -d '='`
MYSQLHOST=`grep ^mysqlhost= $CONFIG | cut -f2 -d '='`
MYSQLPORT=`grep ^mysqlport= $CONFIG | cut -f2 -d '='`
MYSQLSOCKET=`grep ^mysqlsocket= $CONFIG | cut -f2 -d '='`


### create sqlite3 database if we have to

create_sqlite3(){

   if [ "x$SQLITEDB" != "x" ]; then
      if [ ! -f $SQLITEDB ]; then
         echo -n "creating sqlite3 database . . . "
         sqlite3 $SQLITEDB < $SQLSCRIPT
         echo "Done."
      else
         echo "$SQLITEDB exists. Quit."
      fi

   else
      echo "Please edit your clapf.conf to have sqlite3=..."
   fi

   exit 1
}


### create mysql tables

create_mysql(){

   if [ "x$MYSQLDB" != "x" ]; then
      echo -n "creating mysql tables for clapf. Enter the mysql password please for the command: ";
      echo "mysql -u $MYSQLUSER -p $MYSQLDB < $SQLSCRIPT"
      mysql -u $MYSQLUSER -p $MYSQLDB < $SQLSCRIPT

      echo "Done."
   else
      echo "Please edit your clapf.conf to have mysqldb=..."
   fi

   exit 1
}


if [ "$DBTYPE" = "sqlite3" ]; then
   create_sqlite3;
fi

if [ "$DBTYPE" = "mysql" ]; then
   create_mysql;
fi

echo "Invalid database type. It should be either sqlite3 or mysql"
exit 0

