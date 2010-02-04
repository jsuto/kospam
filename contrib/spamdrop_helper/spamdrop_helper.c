/** spamdrop_helper.c, 2009.12.22, VG
** Based on spamdrop.c :)
**
*******
**
** 2010.02.01, VG
**   new option -U username
**              -u uid
**   modify usage ;-) see -h or -?
**   syslog prefix in program name.
**   no contans (USER_QUEUE_DIR) in path, use config variable (cfg.queuedir)
**   In this the source in a file translating the comments from Hungarian onto English.
**
********
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pwd.h>
#include <unistd.h>
#include <sysexits.h>
#include <clapf.h>
#include <locale.h>
#include <ctype.h>
#include <time.h>

sqlite3 *db;
struct stat st;
int rc = 0, old_umask ;

#define PA1 "%s/%s/%c"
#define PA2 PA1"/%s"
#define PA3 PA2"/%s"

static int go_purge( char *dir, time_t sec, int verbose )
{
  char *name = ( char * ) NULL;
  char buf[1] = { '!' };
  time_t t = 0;
  int fd = 0, ret = 0, go = 1;
  char *fn = "/last-purge";
  int l = strlen( dir ) + strlen( fn ) + 1;

  if( ( name = malloc( l ) ) != ( char * ) NULL ) {
    strcat( strcpy( name, dir ), fn );
    time( &t );
    st.st_mtime = 0;
    ret = stat( name, &st );
    if( ( ret == 0 ) && ( ( time_t ) ( st.st_mtime + sec ) > t ) ) {
      go = 0;
    } else {
      old_umask = umask( 0 );
      fd = open( name, O_WRONLY | O_CREAT | O_TRUNC, 0644 );
      if( fd != -1 ) {
        write( fd, buf, sizeof( buf ) );
        close( fd );
      }
      umask( old_umask );
    }
    free( name );
  }
  if( verbose >= _LOG_INFO )
    syslog( LOG_PRIORITY, "PURGE time_file:%lu time_now:%lu, sec:%lu, go:%d.", st.st_mtime, t, sec, go );
  return go;
}

static int end_is_semicolon( const char *str, unsigned int len )
{
/*
** Leaving end of space
*/
  while( ( len ) && ( isspace( str[len - 1] ) ) ) {
    len--;
  }
/*
** Last char is semicolon?
*/
  return ( ( len ) && ( str[len - 1] == ';' ) );
}

void file_to_db( char *infile, sqlite3 * db, int verbose )
{
  char buf[MAXBUFSIZE];
  char *buf_p = ( char * ) NULL;
  char *sql_p = ( char * ) NULL;
  unsigned int sql_len = 0;
  FILE *inf = ( FILE * ) NULL;

  if( infile == ( char * ) NULL ) {
    return;
  }
  memset( buf, 0, sizeof( buf ) );
  if( ( inf = fopen( infile, "r" ) ) != ( FILE * ) NULL ) {
    if( verbose >= _LOG_INFO )
      syslog( LOG_PRIORITY, "SQL command from '%s' start", infile );
    while( ( buf_p = fgets( buf, sizeof( buf ) - 1, inf ) ) != ( char * ) NULL ) {
      char *p = strrchr( buf_p, '\n' );
      if( p != ( char * ) NULL ) {
        *p = '\0';
      }
      if( sql_p == ( char * ) NULL ) {
        unsigned int i = 0;
        for( ; buf_p[i] && isspace( buf_p[i] ); ) {
          i++;
        }
        if( buf_p[i] ) {
          sql_len = strlen( buf_p );
          sql_p = malloc( sql_len + 1 );        /* +1 for '\0' (strcpy...) */
          strcpy( sql_p, buf_p );
        }
      } else {
        unsigned int new_len = strlen( buf_p ) + sql_len + 1;   /* +1 for '\n' */
        sql_p = realloc( sql_p, new_len + 1 );  /* +1 for '\0' (strcpy...) */
        strcpy( &sql_p[sql_len++], "\n" );
        strcpy( &sql_p[sql_len], buf_p );
        sql_len = new_len;
      }
      if( ( sql_p != ( char * ) NULL ) && end_is_semicolon( sql_p, sql_len ) && sqlite3_complete( sql_p ) ) {
        rc = sqlite3_exec( db, sql_p, NULL, NULL, NULL );
        free( sql_p );
        sql_p = ( char * ) NULL;
        sql_len = 0;
        if( rc ) {
          fclose( inf );
          return;
        }
      }
    }
    if( sql_p != ( char * ) NULL ) {
      free( sql_p );
    }
    fclose( inf );
    if( verbose >= _LOG_INFO )
      syslog( LOG_PRIORITY, "SQL command from '%s' stop", infile );
  }
}

void open_db2( struct __config *cfg, char *in_file, int mode )
{
  rc = sqlite3_open( cfg->sqlite3, &db );
  sqlite3_close( db );
  if( rc ) {
    return;
  }
/*
** Open database and close database. if open error -> exit.
** After close, size of database file?
*/
  if( stat( cfg->sqlite3, &st ) == 0 ) {
    if( ( !st.st_size && !mode ) || ( st.st_size && mode ) ) {
/*
** size == 0 -> no database.
** ----
** size == 0 and mode = 0 -> create database (determinated in_file)
** size != 0 and mode = 1 -> purge database (determinated in_file)
** ----
** Open database.
*/
      rc = sqlite3_open( cfg->sqlite3, &db );
      if( rc ) {
        sqlite3_close( db );
        return;
      }
      rc = sqlite3_exec( db, cfg->sqlite3_pragma, NULL, NULL, NULL );
      file_to_db( in_file, db, cfg->verbosity );
      sqlite3_close( db );
    }
  }
}


int main( int argc, char **argv )
{
  int i, nocopy = 0;
  time_t sec = 86400;           /* 1 day */
  uid_t uid = (uid_t) -1;
  char *configfile = CONFIG_FILE;
  char *create_sqlfile = ( char * ) NULL;
  char *purge_sqlfile = ( char * ) NULL;
  char name[SMALLBUFSIZE];
  char path[MAXBUFSIZE];
  char *prg, *p;
  struct passwd *pwd;
  struct __config cfg;

  while( ( i = getopt( argc, argv, "c:u:U:p:s:t:xh?" ) ) > 0 ) {
    switch ( i ) {

      case 'c':
        configfile = optarg;
        break;

      case 'U' :
        if( ( pwd = getpwuid( atoi( optarg ) ) ) != ( struct passwd * ) NULL ) {
          uid = pwd->pw_uid;
        }
        break;

      case 'u' :
        if( ( pwd = getpwnam( optarg ) ) != ( struct passwd * ) NULL ) {
          uid = pwd->pw_uid;
        }
        break;

      case 'p':
        purge_sqlfile = optarg;
        break;

      case 's':
        create_sqlfile = optarg;
        break;

      case 't':
        sec = atoi( optarg );
        break;

      case 'x':
        nocopy = 1;
        break;

      case 'h':
      case '?':
        fprintf( stderr, "%s %s\n", argv[0], "[-c config] [-u username | -U uid] [-p sql_table_purge_file] [-s sql_table_create_file] [-t sec] [-x]" );
        return 1;
        break;

      default:
        break;
    }
  }

  prg = basename( argv[0] );
  if( ( p = strstr( prg, ".bin" ) ) != ( char * ) NULL ) {
    *p = '\0';
  }
  ( void ) openlog( prg, LOG_PID, LOG_MAIL );
  /* read config file */
  cfg = read_config( configfile );

  setlocale( LC_MESSAGES, cfg.locale );
  setlocale( LC_CTYPE, cfg.locale );

  memset( name, 0, sizeof( name ) );
  memset( path, 0, sizeof( path ) );
  if( uid == (uid_t) -1 )
    uid = getuid( );
  pwd = getpwuid( uid );
  snprintf( name, sizeof( name ) - 1, "%s", pwd->pw_name );

  if( cfg.verbosity >= _LOG_INFO )
    syslog( LOG_PRIORITY, "UserName is '%s'", name );

  if( strlen( cfg.sqlite3 ) < 4 )
    snprintf( cfg.sqlite3, sizeof( cfg.sqlite3 ) - 1, PA3, cfg.chrootdir, USER_DATA_DIR, name[0], name, PER_USER_SQLITE3_DB_FILE );

  old_umask = umask( 0 );
  snprintf( path, sizeof( path ) - 1, PA2, cfg.chrootdir, cfg.queuedir, name[0], name );
  if( stat( path, &st ) != 0 ) {
    snprintf( path, sizeof( path ) - 1, PA1, cfg.chrootdir, cfg.queuedir, name[0] );
    mkdir( path, 0750 );
    snprintf( path, sizeof( path ) - 1, PA2, cfg.chrootdir, cfg.queuedir, name[0], name );
    mkdir( path, 0750 );
  }
  snprintf( path, sizeof( path ) - 1, PA2, cfg.chrootdir, USER_DATA_DIR, name[0], name );
  if( stat( path, &st ) != 0 ) {
    snprintf( path, sizeof( path ) - 1, PA1, cfg.chrootdir, USER_DATA_DIR, name[0] );
    mkdir( path, 0750 );
    snprintf( path, sizeof( path ) - 1, PA2, cfg.chrootdir, USER_DATA_DIR, name[0], name );
    mkdir( path, 0750 );
  }
  umask( old_umask );

  if( cfg.verbosity >= _LOG_INFO )
    syslog( LOG_PRIORITY, "DataBaseFile is '%s'", cfg.sqlite3 );

  open_db2( &cfg, create_sqlfile, 0 );
  if( go_purge( path, sec, cfg.verbosity ) ) {
    open_db2( &cfg, purge_sqlfile, 1 );
  }

  if( nocopy == 0 ) {
/*
** copy from stdin to stdout ...
*/
    while( ( i = read( 0, path, sizeof( path ) - 1 ) ) > 0 ) {
      write( 1, path, i );
    }
  }

  return 0;
}
