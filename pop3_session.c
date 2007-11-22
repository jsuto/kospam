/*
 * pop3_session.c, 2007.11.20, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "misc.h"
#include "messages.h"
#include "pop3.h"
#include "bayes.h"
#include "ssl_util.h"
#include "config.h"


#ifdef HAVE_SQLITE3
   #include <sqlite3.h>
   sqlite3 *db;
   sqlite3_stmt *pStmt;
   const char **ppzTail=NULL;
   char *zErrMsg = 0;
   int rc;
#endif
#ifdef HAVE_MYDB
   #include "mydb.h"
   int rc;
#endif


#define CHK_NULL(x, errmsg) if ((x) == NULL) __fatal(errmsg)
#define CHK_SSL(x, errmsg) if ((x) == -1) __fatal(errmsg);

SSL_CTX *ctx = NULL;
SSL *ssl = NULL;
SSL_METHOD *meth = NULL;

struct timezone tz;
struct timeval tv_spam_start, tv_spam_stop;
unsigned long ts, n_msgs, top_lines;
int sd, inj, ret, auth_done, prevlen=0;
char prevbuf[MAXBUFSIZE], last2buf[2*MAXBUFSIZE+1], username[SMALLBUFSIZE], password[SMALLBUFSIZE], msgid[SMALLBUFSIZE];
double spaminess=0.5;

void kill_child(){
   LOG_MESSAGE("child is killed by force");

   SSL_free(ssl);
   SSL_CTX_free(ctx);

   exit(0);
}


unsigned long resolve_host(char *h){
   struct hostent *host;
   struct in_addr addr;

   if((addr.s_addr = inet_addr(h)) == -1){
       if((host = gethostbyname(h)) == NULL){
          return 0;
       }
       else return *(unsigned long*)host->h_addr;
   }
   else return addr.s_addr;
}


/*
 * init POP3 session
 */

void init_child(){
   n_msgs = 0;
   prevlen = 0;
   auth_done = 0;
   memset(password, 0, SMALLBUFSIZE);
   time(&ts);
}


/*
 * handle POP3 session
 */

void pop3_session(int new_sd, struct __config cfg){
   int n, err=0, retr_cmd, fd, cfd, state, is_header, plen, port=110, totlen=0, is_spam, ham_msg=0, spam_msg=0;
   float spamicity;
   char *p, cmdbuf[MAXBUFSIZE], buf[2*MAXBUFSIZE], puf[3*MAXBUFSIZE], filename[SMALLBUFSIZE];
   struct timezone tz;
   struct timeval tv;
   FILE *f=NULL;
   struct sockaddr_in pop3_addr;
   struct session_data sdata;
   struct _state sstate;

   init_child();
   state = POP3_STATE_INIT;

   alarm(SESSION_TIMEOUT);
   signal(SIGALRM, kill_child);

   if(cfg.use_ssl == 1){
      SSL_load_error_strings();
      SSLeay_add_ssl_algorithms();
      meth = TLSv1_client_method();

      ctx = SSL_CTX_new(meth);
      CHK_NULL(ctx, "SSL_CTX_new() failed");

      ssl = SSL_new(ctx);
      CHK_NULL(ssl, "internal ssl error");

      port = LISTEN_SSL_PORT;
   }


   if((cfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
      LOG_MESSAGE("cannot create socket");
      goto QUITTING;
   }

   if(cfg.use_ssl == 1){
      err = SSL_set_fd(ssl, cfd);
      CHK_SSL(err, "internal ssl error");
   }

   pop3_addr.sin_family = AF_INET;
   pop3_addr.sin_port = htons(port);
   bzero(&(pop3_addr.sin_zero), 8);


   /* send our local banner */

   write1(new_sd, POP3_RESP_BANNER, 0, ssl);

   while((n = read1(new_sd, cmdbuf, 0, ssl)) > 0){

      /* read command */

      if(prevlen == 0) memset(buf, 0, 2*MAXBUFSIZE);

      memcpy(&buf[0]+prevlen, cmdbuf, n);
      prevlen += n;

      if(prevlen < 3) continue;

      if(buf[prevlen-2] != '\r' || buf[prevlen-1] != '\n'){
         if(prevlen > MAXBUFSIZE){
            write1(new_sd, POP3_RESP_INVALID_CMD, 0, ssl);
            prevlen = 0;
         }

         continue;
      }

      prevlen = 0;

      alarm(SESSION_TIMEOUT);


      /* QUIT command */

      if(strncasecmp(buf, POP3_CMD_QUIT, strlen(POP3_CMD_QUIT)) == 0){

         state = POP3_STATE_FINISHED;

         write1(cfd, buf, cfg.use_ssl, ssl);
         read1(cfd, buf, cfg.use_ssl, ssl);
         write1(new_sd, buf, 0, ssl);

         goto QUITTING;
      }


      /* USER command */

      if(strncasecmp(buf, POP3_CMD_USER, strlen(POP3_CMD_USER)) == 0){
         state = POP3_STATE_USER;

         /* determine connection data */
         /* username:hostname */

         p = strchr(buf, ' ');
         if(p){
            *p = '\0';
            p++;
            p[strlen(p)-2] = '\0';
            memset(username, 0, SMALLBUFSIZE);
            strncpy(username, p, SMALLBUFSIZE-1);

            p = strchr(username, ':');
            if(p){
               *p = '\0';
               p++;

               pop3_addr.sin_addr.s_addr = resolve_host(p);

               if(connect(cfd, (struct sockaddr *)&pop3_addr, sizeof(struct sockaddr)) == -1){
                  write1(new_sd, "-ERR cannot connect to remote server\r\n", 0, ssl);
                  goto QUITTING;
               }

               if(cfg.use_ssl == 1){
                  err = SSL_connect(ssl);
                  CHK_SSL(err, "internal ssl error");
               }

               /* read server banner */

               read1(cfd, buf, cfg.use_ssl, ssl);


               /* send username, then read reply */

               snprintf(buf, MAXBUFSIZE-1, "USER %s\r\n", username);
               write1(cfd, buf, cfg.use_ssl, ssl);
               read1(cfd, buf, cfg.use_ssl, ssl);

               write1(new_sd, buf, 0, ssl);

               continue;
            }
         }

         write1(new_sd, "-ERR invalid usage\r\n", 0, ssl);
         continue;
      }


      /* PASS command */

      if(strncasecmp(buf, POP3_CMD_PASS, strlen(POP3_CMD_PASS)) == 0){
         if(state < POP3_STATE_USER)
            write1(new_sd, POP3_RESP_INVALID_CMD, 0, ssl);
         else {
            write1(cfd, buf, cfg.use_ssl, ssl);
            read1(cfd, buf, cfg.use_ssl, ssl);

            if(strncmp(buf, "+OK", 3) == 0){
               state = POP3_STATE_PASS;
            }

            write1(new_sd, buf, 0, ssl);

         }

         continue;
      }


      /* HELP command */

      if(strncasecmp(buf, POP3_CMD_HELP, strlen(POP3_CMD_HELP)) == 0){
         write1(new_sd, POP3_RESP_OK, 0, ssl);
         continue;
      }



      /* NOOP command */

      if(strncasecmp(buf, POP3_CMD_NOOP, strlen(POP3_CMD_NOOP)) == 0){
         write1(cfd, buf, cfg.use_ssl, ssl);
         read1(cfd, buf, cfg.use_ssl, ssl);
         write1(new_sd, buf, 0, ssl);

         continue;
      }


      /* STAT command */

      if(strncasecmp(buf, POP3_CMD_STAT, strlen(POP3_CMD_STAT)) == 0){
         if(state < POP3_STATE_PASS) goto INVALID_CMD;
         write1(cfd, buf, cfg.use_ssl, ssl);
         read1(cfd, buf, cfg.use_ssl, ssl);
         write1(new_sd, buf, 0, ssl);

         continue;
      }


      /* LIST or UIDL command */

      if(strncasecmp(buf, POP3_CMD_LIST, strlen(POP3_CMD_LIST)) == 0 || strncasecmp(buf, POP3_CMD_UIDL, strlen(POP3_CMD_UIDL)) == 0){
         if(state < POP3_STATE_PASS) goto INVALID_CMD;

         write1(cfd, buf, cfg.use_ssl, ssl);

         p = strchr(buf, ' ');
         if(p){
            p++;
            if(atoi(p) > 0){
               read1(cfd, buf, cfg.use_ssl, ssl);
               write1(new_sd, buf, 0, ssl);
            }
            else 
               write1(new_sd, "-ERR syntax error\r\n", 0, ssl);
         }
         else {
            while((n = read1(cfd, buf, cfg.use_ssl, ssl)) > 0){
               write1(new_sd, buf, 0, ssl);
               if(strstr(buf, "\r\n.\r\n")) break;
            }
         }
         continue;
      }


      /* RETR or TOP command */

      if(strncasecmp(buf, POP3_CMD_RETR, strlen(POP3_CMD_RETR)) == 0 || strncasecmp(buf, POP3_CMD_TOP, strlen(POP3_CMD_TOP)) == 0){
         if(state < POP3_STATE_PASS) goto INVALID_CMD;

         retr_cmd = 0;
         fd = -1;
         totlen = 0;

         p = strchr(buf, ' ');
         if(p){
            p++;
            write1(cfd, buf, cfg.use_ssl, ssl);
            memset(cmdbuf, 0, MAXBUFSIZE);

            if(strncasecmp(buf, POP3_CMD_RETR, strlen(POP3_CMD_RETR)) == 0){
               retr_cmd = 1;
               gettimeofday(&tv, &tz);
               snprintf(filename, SMALLBUFSIZE-1, "%ld%ld", tv.tv_sec, tv.tv_usec);
               fd = open(filename, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
            }

            plen = 0;
            memset(cmdbuf, 0, MAXBUFSIZE);

            LOG_MESSAGE("downloading message");

            while((n = read1(cfd, buf, cfg.use_ssl, ssl)) > 0){
               totlen += n;

               if(plen == 0 && strncmp(buf, "-ERR", 4) == 0){
                  unlink(filename);
                  goto NO_SUCH_MESSAGE;
               }

               if(retr_cmd == 1 && fd != -1) write(fd, buf, n);
               else write1(new_sd, buf, 0, ssl);

               memset(puf, 0, 3*MAXBUFSIZE);
               memcpy(puf, cmdbuf, plen);
               memcpy(puf+plen, buf, n);

               // save buffer
               memcpy(cmdbuf, buf, n);

               if(puf[plen+n-5] == '\r' && puf[plen+n-4] == '\n' && puf[plen+n-3] == '.' && puf[plen+n-2] == '\r' && puf[plen+n-1] == '\n') break;

               plen = n;

            }

            if(retr_cmd == 1 && fd != -1){
               close(fd);

               spamicity = DEFAULT_SPAMICITY;

               if(totlen > cfg.max_message_size_to_filter)
                  LOG_MESSAGE("message size is above max_message_size_to_filter parameter, skip spam checking");
               else {

               #ifdef HAVE_SQLITE3
                  rc = sqlite3_open(cfg.sqlite3, &db);
                  if(rc)
                     LOG_MESSAGE(ERR_SQLITE3_OPEN);
               #endif
               #ifdef HAVE_MYDB
                  rc = init_mydb(cfg.mydbfile, mhash);
                  if(rc == 0)
                     LOG_MESSAGE("cannot open mydb file");
               #endif
                  else {

                  #ifdef HAVE_SQLITE3
                     /* issue a pragma command */
                     rc = sqlite3_exec(db, SQLITE3_PRAGMA, NULL, NULL, &zErrMsg);
                     if(rc != SQLITE_OK)
                        LOG_MESSAGE("cannot set pragma: " SQLITE3_PRAGMA);
                  #endif

                     sdata.num_of_rcpt_to = 1;
                     sdata.uid = getuid();
                     sdata.skip_id_check = 1;

                     sstate = parse_message(filename, cfg);

                     gettimeofday(&tv_spam_start, &tz);
                  #ifdef HAVE_SQLITE3
                     spamicity = bayes_file(db, filename, sstate, sdata, cfg);
                  #endif
                  #ifdef HAVE_MYDB

                  #endif
                     gettimeofday(&tv_spam_stop, &tz);

                     fprintf(stderr, "spam check in %ld\n", tvdiff(tv_spam_stop, tv_spam_start)/1000);

                     if(spamicity >= cfg.spam_overall_limit)
                        is_spam = 1;
                     else
                        is_spam = 0;

                     snprintf(sdata.ttmpfile, RND_STR_LEN, "%s", filename);

                     ham_msg = spam_msg = 0;

                  #ifdef HAVE_SQLITE3
                     snprintf(buf, MAXBUFSIZE-1, "SELECT nham, nspam FROM %s WHERE uid=0", SQL_MISC_TABLE);

                     if(sqlite3_prepare_v2(db, buf, -1, &pStmt, ppzTail) == SQLITE_OK){
                        while(sqlite3_step(pStmt) == SQLITE_ROW){
                           ham_msg += sqlite3_column_int(pStmt, 0);
                           spam_msg += sqlite3_column_int(pStmt, 1);
                        }
                     }
                     sqlite3_finalize(pStmt);
                  #endif
                  #ifdef HAVE_MYDB
                     ham_msg = Nham;
                     spam_msg = Nspam;
                  #endif

                     fprintf(stderr, "nham: %d, nspam: %d\n", ham_msg, spam_msg);

                     gettimeofday(&tv_spam_start, &tz);

                     if(ham_msg + spam_msg < NUMBER_OF_INITIAL_1000_MESSAGES_TO_BE_LEARNED)
                     #ifdef HAVE_SQLITE3
                        retraining(db, sdata, "", is_spam, cfg);
                     #endif
                     #ifdef HAVE_MYDB
                        retraining(sdata, "", is_spam, cfg);
                     #endif
                     else
                     #ifdef HAVE_SQLITE3
                        tum_train(filename, spaminess, cfg);
                     #endif
                     #ifdef HAVE_MYDB
                        tum_train(filename, spaminess, cfg);
                     #endif

                     gettimeofday(&tv_spam_stop, &tz);

                     fprintf(stderr, "training in %ld\n", tvdiff(tv_spam_stop, tv_spam_start)/1000);

                  #ifdef HAVE_SQLITE3
                     sqlite3_close(db);
                  #endif
                  #ifdef HAVE_MYDB
                     close_mydb(mhash);
                  #endif
                  }
               }

               /* then send it back to the client */

               f = fopen(filename, "r");
               if(f){
                  is_header = 1;

                  while(fgets(buf, MAXBUFSIZE-1, f)){
                     if(is_header == 1 && (buf[0] == '\r' || buf[0] == '\n') ) is_header = 0;

                     if(is_header == 0){
                        snprintf(cmdbuf, MAXBUFSIZE-1, "%s%s\r\n", cfg.clapf_header_field, msgid);
                        write1(new_sd, cmdbuf, 0, ssl);

                        snprintf(cmdbuf, MAXBUFSIZE-1, "%s%.4f\r\n", cfg.clapf_header_field, spaminess);
                        write1(new_sd, cmdbuf, 0, ssl);

                        if(spamicity >= cfg.spam_overall_limit){
                           snprintf(cmdbuf, MAXBUFSIZE-1, "%sYes\r\n", cfg.clapf_header_field);
                           write1(new_sd, cmdbuf, 0, ssl);
                        }

                        is_header = 5;
                     }

                     write1(new_sd, buf, 0, ssl);
                  }
                  fclose(f);
               }
               else write1(new_sd, "-ERR internal error, try again", 0, ssl);

               unlink(filename);
            }


         }
         else
            NO_SUCH_MESSAGE:
            write1(new_sd, POP3_RESP_NO_SUCH_MESSAGE, 0, ssl);

         continue;
      }


      /* DELE command */

      if(strncasecmp(buf, POP3_CMD_DELE, strlen(POP3_CMD_DELE)) == 0){
         if(state < POP3_STATE_PASS) goto INVALID_CMD;

         write1(cfd, buf, cfg.use_ssl, ssl);
         read1(cfd, buf, cfg.use_ssl, ssl);
         write1(new_sd, buf, 0, ssl);

         continue;
      }


      /* RSET command */

      if(strncasecmp(buf, POP3_CMD_RSET, strlen(POP3_CMD_RSET)) == 0){
         if(state < POP3_STATE_PASS) goto INVALID_CMD;

         write1(cfd, buf, cfg.use_ssl, ssl);
         read1(cfd, buf, cfg.use_ssl, ssl);
         write1(new_sd, buf, 0, ssl);

         continue;
      }



      /* AUTH command */

      if(strncasecmp(buf, POP3_CMD_AUTH, strlen(POP3_CMD_AUTH)) == 0){
         write1(new_sd, POP3_RESP_INVALID_AUTH_TYPE, 0, ssl);
         continue;
      }


INVALID_CMD:

      /* by default send invalid command message */
      write1(new_sd, POP3_RESP_INVALID_CMD, 0, ssl);
   }

QUITTING:
   if(cfg.use_ssl == 1){
      SSL_free(ssl);
      SSL_CTX_free(ctx);
   }

   close(cfd);
   _exit(0);
}
