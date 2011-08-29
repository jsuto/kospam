#!/bin/sh
##
## db_init.sh, SJ
## create the database tables
##

if [ $# -ne 2 ]; then echo "usage: $0 <mysql|sqlite3|psql> <config file>"; exit 1; fi

LANG=C
LC_CTYPE=C
LC_COLLATE=C
LC_TIME=C
LC_ALL=C

export PATH=$PATH:/usr/local/bin:/usr/local/sbin:/usr/local/mysql/bin

DBTYPE=$1
CONFIG=$2

SQLSCRIPT=/usr/local/share/clapf/db.sql

SQLITEDB=`grep ^sqlite3= $CONFIG | cut -f2 -d '='`

MYSQLDB=`grep ^mysqldb= $CONFIG | cut -f2 -d '='`
MYSQLUSER=`grep ^mysqluser= $CONFIG | cut -f2 -d '='`
MYSQLPASSWORD=`grep ^mysqlpwd= $CONFIG | cut -f2 -d '='`
MYSQLHOST=`grep ^mysqlhost= $CONFIG | cut -f2 -d '='`
MYSQLPORT=`grep ^mysqlport= $CONFIG | cut -f2 -d '='`
MYSQLSOCKET=`grep ^mysqlsocket= $CONFIG | cut -f2 -d '='`

PSQLDB=`grep ^psqldb= $CONFIG | cut -f2 -d '='`
PSQLUSER=`grep ^psqluser= $CONFIG | cut -f2 -d '='`
PSQLPWD=`grep ^psqlpwd= $CONFIG | cut -f2 -d '='`
PSQLHOST=`grep ^psqlhost= $CONFIG | cut -f2 -d '='`
PSQLPORT=`grep ^psqlport= $CONFIG | cut -f2 -d '='`

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
      if [ -n "$MYSQLSOCKET" ]; then
         mysql --user=$MYSQLUSER --password=$MYSQLPASSWORD --socket=$MYSQLSOCKET $MYSQLDB < $SQLSCRIPT
      else
         mysql --user=$MYSQLUSER --password=$MYSQLPASSWORD --host=$MYSQLHOST --port=MYSQLPORT $MYSQLDB < $SQLSCRIPT
      fi

      echo "Done."
   else
      echo "Please edit your clapf.conf to have mysqldb=..."
   fi

   exit 1
}


### create pssl tables

create_psql(){

   if [ "x$PSQLDB" != "x" ]; then

      DBRES=`su postgres -c "psql template1 -c \"CREATE USER $PSQLUSER WITH PASSWORD '$PSQLPWD';\"" 2>&1`
      DB2RES="`echo $DBRES | awk '{print $1}'`"
      if [ "x$DB2RES" = "xCREATE" ]; then
         echo "** Database user created."
      else
         if [ -n "`echo $DBRES | grep -v 'already exists'`" ]; then
            echo "!! Cannot create user $PSQLUSER!"
            echo "!! Result: $DBRES"
            exit 1;
         else
            echo "** Database user already exists."
         fi
      fi
      DBRES=`su postgres -c "psql template1 -c \"CREATE DATABASE $PSQLDB ENCODING 'UTF8' OWNER $PSQLUSER;\"" 2>&1`
      DB2RES="`echo $DBRES | awk '{print $1}'`"
      if [ "x$DB2RES" = "xCREATE" ]; then
         echo "** Database created."
      else
         if [ -n "`echo $DBRES | grep -v 'already exists'`" ]; then
            echo "!! Cannot create database $PSQLDB!"
            echo "!! Result: $DBRES"
            exit 1;
         else
            echo "** Database already exists."
            echo "** Use dropdb command."
            exit 1;
         fi
      fi
      echo -n "creating psql tables for clapf";
      DBRES=`PGPASSWORD=$PSQLPWD psql -U $PSQLUSER -h $PSQLHOST -p $PSQLPORT -d $PSQLDB < $SQLSCRIPT 2>&1 | egrep '^ERROR:'`
      if [ "x$DBRES" = "x" ]; then
         echo " done."
      else
         echo ' error.'
         echo "** See output this command: psql -U $PSQLUSER -h $PSQLHOST -p $PSQLPORT -d $PSQLDB < $SQLSCRIPT"
      fi
   else
      echo "Please edit your clapf.conf to have psqldb=..."
   fi

   exit 1
}


if [ "$DBTYPE" = "psql" ]; then
   create_psql;
fi

if [ "$DBTYPE" = "sqlite3" ]; then
   create_sqlite3;
fi

if [ "$DBTYPE" = "mysql" ]; then
   create_mysql;
fi

echo "Invalid database type. It should be either sqlite3 or mysql or psql"
exit 0

