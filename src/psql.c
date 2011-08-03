/*
 * psql.c, Varadi
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <sys/mman.h>
#include <clapf.h>

struct te getHamSpamCounters( struct session_data *sdata, char *stmt ) {
   PGresult *res;
   int rows = 0;
   int i = 0;
   struct te TE;

   TE.nham = TE.nspam = 0;
   if( PQstatus( sdata->psql ) == CONNECTION_BAD ) {
      sdata->psql = PQconnectdb( sdata->conninfo );
   }
   if( PQstatus( sdata->psql ) != CONNECTION_BAD ) {
      res = PQexec( sdata->psql, stmt );
      if( res && PQresultStatus( res ) == PGRES_TUPLES_OK ) {
        rows = PQntuples( res );
        for( i = 0; i < rows; i++) {
           TE.nham += atof( ( const char * )PQgetvalue( res, i, 0 ) );
           TE.nspam += atof( ( const char * )PQgetvalue( res, i, 1 ) );
        }
        PQclear( res );
     }
   }
   return TE;
}

int update_hash( struct session_data *sdata, char *qry, struct node *xhash[] ) {
   PGresult *res;
   int rows = 0;
   int i = 0;
   float nham, nspam;
   unsigned long long token;

   if( PQstatus( sdata->psql ) == CONNECTION_BAD ) {
      sdata->psql = PQconnectdb( sdata->conninfo );
   }
   if( PQstatus( sdata->psql ) != CONNECTION_BAD ) {
      res = PQexec( sdata->psql, qry );
      if( res && PQresultStatus( res ) == PGRES_TUPLES_OK ) {
         rows = PQntuples( res );
         for( i = 0; i < rows; i++) {
            token = strtoull( ( const char * )PQgetvalue( res, i, 0 ), NULL, 10);
            nham = atof( ( const char * )PQgetvalue( res, i, 1 ) );
            nspam = atof( ( const char * )PQgetvalue( res, i, 2 ) );
            updatenode( xhash, token, nham, nspam, DEFAULT_SPAMICITY, 0 );
         }
         PQclear( res );
      }
   }
   return 1;
}

/*
 * introduce tokens to database with zero (0) counter values
 */

int introduceTokens( struct session_data *sdata, struct node *xhash[] ) {
   PGresult *res;
   int i, n=0;
   time_t cclock;
   unsigned long now;
   char s[SMALLBUFSIZE];
   struct node *q;
   buffer *query;

   if( counthash( xhash ) <= 0 ) return 0;

   query = buffer_create( NULL );
   if( !query ) return 0;

   snprintf( s, SMALLBUFSIZE-1, "SELECT token,nham,nspam FROM %s WHERE token IN (", SQL_TOKEN_TABLE );
   buffer_cat( query, s );
   for( i = 0; i < MAXHASH; i++ ) {
      q = xhash[i];
      while( q != NULL ) {
         if( n ) snprintf( s, SMALLBUFSIZE-1, ",%llu", q->key );
         else    snprintf( s, SMALLBUFSIZE-1, "%llu", q->key );
         buffer_cat( query, s );
         n++;
         q = q->r;
      }
   }
   snprintf( s, SMALLBUFSIZE-1, ") AND uid=%ld", sdata->uid );
   buffer_cat( query, s );
   update_hash( sdata, query->data, xhash );
   buffer_destroy( query );

   query = buffer_create( NULL );
   if( !query ) return 0;

   time( &cclock );
   now = cclock;
   n = 0;
   for( i = 0; i < MAXHASH; i++ ) {
      q = xhash[i];
      while( q != NULL ) {
         if( q->nham + q->nspam == 0 ){
            snprintf( s, SMALLBUFSIZE-1, "INSERT INTO %s (token, nham, nspam, uid, timestamp) VALUES (%llu,0,0,%ld,%ld);", SQL_TOKEN_TABLE, q->key, sdata->uid, now );
            buffer_cat( query, s );
            n++;
         }
         q = q->r;
      }
   }
   if( n ) {
      if( PQstatus( sdata->psql ) == CONNECTION_BAD ) {
         sdata->psql = PQconnectdb( sdata->conninfo );
      }
      if( PQstatus( sdata->psql ) != CONNECTION_BAD ) {
         res = PQexec( sdata->psql, query->data );
         PQclear( res );
      }
   }
   buffer_destroy( query );
   return 1;
}

int updateTokenCounters( struct session_data *sdata, int ham_or_spam, struct node *xhash[], int train_mode ) {
   PGresult *res;
   int i, n=0;
   char s[SMALLBUFSIZE];
   struct node *q;
   buffer *query;

   if( counthash( xhash ) <= 0 ) return 0;

   query = buffer_create( NULL );
   if( !query ) return n;

   if( ham_or_spam == 1 ) {
      if( train_mode == T_TUM ) snprintf( s, SMALLBUFSIZE-1, "UPDATE %s SET nham=nham-1 WHERE token IN (", SQL_TOKEN_TABLE );
      else                      snprintf( s, SMALLBUFSIZE-1, "UPDATE %s SET nspam=nspam+1 WHERE token IN (", SQL_TOKEN_TABLE );
   } else {
      if( train_mode == T_TUM ) snprintf( s, SMALLBUFSIZE-1, "UPDATE %s SET nspam=nspam-1 WHERE token IN (", SQL_TOKEN_TABLE );
      else                      snprintf( s, SMALLBUFSIZE-1, "UPDATE %s SET nham=nham+1 WHERE token IN (", SQL_TOKEN_TABLE );
   }
   buffer_cat(query, s);
   for( i = 0; i < MAXHASH; i++ ) {
      q = xhash[i];
      while( q != NULL ) {
         if( n ) snprintf( s, SMALLBUFSIZE-1, ",%llu", q->key );
         else    snprintf( s, SMALLBUFSIZE-1, "%llu", q->key );
         buffer_cat( query, s );
         q = q->r;
         n++;
      }
   }
   buffer_cat(query, ")");
   if( train_mode == T_TUM ) {
      if( ham_or_spam == 1 ) buffer_cat( query, " AND nham > 0" );
      else                   buffer_cat( query, " AND nspam > 0" );
   }
   snprintf( s, SMALLBUFSIZE-1, " AND uid=%ld", sdata->uid );
   buffer_cat( query, s );
   if( PQstatus( sdata->psql ) == CONNECTION_BAD ) {
      sdata->psql = PQconnectdb( sdata->conninfo );
   }
   if( PQstatus( sdata->psql ) != CONNECTION_BAD ) {
      res = PQexec( sdata->psql, query->data );
      PQclear( res );
   }
   buffer_destroy( query );
   return 1;
}

int updateMiscTable( struct session_data *sdata, int ham_or_spam, int train_mode ) {
   PGresult *res;
   char s[SMALLBUFSIZE];

   if( ham_or_spam == 1 ) {
      if( train_mode == T_TUM ) snprintf( s, SMALLBUFSIZE-1, "UPDATE %s SET nham=nham-1 WHERE uid=%ld AND nham > 0", SQL_MISC_TABLE, sdata->uid );
      else                      snprintf( s, SMALLBUFSIZE-1, "UPDATE %s SET nspam=nspam+1 WHERE uid=%ld", SQL_MISC_TABLE, sdata->uid );
   } else {
      if( train_mode == T_TUM ) snprintf( s, SMALLBUFSIZE-1, "UPDATE %s SET nspam=nspam-1 WHERE uid=%ld AND nspam > 0", SQL_MISC_TABLE, sdata->uid );
      else                      snprintf( s, SMALLBUFSIZE-1, "UPDATE %s SET nham=nham+1 WHERE uid=%ld", SQL_MISC_TABLE, sdata->uid );
   }
   if( PQstatus( sdata->psql ) == CONNECTION_BAD ) {
      sdata->psql = PQconnectdb( sdata->conninfo );
   }
   if( PQstatus( sdata->psql ) != CONNECTION_BAD ) {
      res = PQexec( sdata->psql, s );
      PQclear( res );
   }
   return 1;
}

int updateTokenTimestamps( struct session_data *sdata, struct node *xhash[] ) {
   PGresult *res;
   int i, n=0;
   unsigned long now;
   time_t cclock;
   char s[SMALLBUFSIZE];
   struct node *q;
   buffer *query;

   if( counthash( xhash ) <= 0 ) return 0;

   query = buffer_create( NULL );
   if( !query ) return n;

   time( &cclock );
   now = cclock;
   snprintf( s, SMALLBUFSIZE-1, "UPDATE %s SET timestamp=%ld WHERE token IN (", SQL_TOKEN_TABLE, now );
   buffer_cat( query, s );
   for( i = 0; i < MAXHASH; i++ ) {
      q = xhash[i];
      while( q != NULL ) {
         if( q->spaminess != DEFAULT_SPAMICITY ) {
            if( n ) snprintf( s, SMALLBUFSIZE-1, ",%llu", q->key );
            else    snprintf( s, SMALLBUFSIZE-1, "%llu", q->key );
            buffer_cat( query, s );
            n++;
         }
         q = q->r;
      }
   }
   if( sdata->uid > 0 )
      snprintf( s, SMALLBUFSIZE-1, ") AND (uid=0 OR uid=%ld)", sdata->uid );
   else
      snprintf( s, SMALLBUFSIZE-1, ") AND uid=0" );
   buffer_cat( query, s );
   if( PQstatus( sdata->psql ) == CONNECTION_BAD ) {
      sdata->psql = PQconnectdb( sdata->conninfo );
   }
   if( PQstatus( sdata->psql ) != CONNECTION_BAD ) {
      res = PQexec( sdata->psql, query->data );
      if( !res || PQresultStatus( res ) != PGRES_COMMAND_OK ) {
         n = -1;
      }
      PQclear( res );
   } else {
      n = -1;
   }
   buffer_destroy( query );
   return n;
}

