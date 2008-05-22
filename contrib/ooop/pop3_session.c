/*
 * pop3_session.c, 2008.05.22, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include "misc.h"
#include "messages.h"
#include "pop3.h"
#include "ooop-util.h"
#include "cfg.h"
#include "config.h"
#include <clapf.h>

#ifdef HAVE_LIBWRAP
   #include <tcpd.h>

   int deny_severity = LOG_WARNING;
   int allow_severity = LOG_DEBUG;
   struct request_info req;
#endif
#ifdef HAVE_MYDB
   struct mydb_node *mhash[MAX_MYDB_HASH];
#endif


unsigned long n_msgs, top_lines;
int sd, inj, ret, prevlen=0, state, dbh;
char prevbuf[MAXBUFSIZE], last2buf[2*MAXBUFSIZE+1], username[SMALLBUFSIZE], password[SMALLBUFSIZE];
SSL_CTX *ctx2 = NULL;
SSL *ssl2 = NULL;


int remote_auth(char *username, char *password, int *sd, SSL **ssl, int use_ssl, char *errmsg);
float process_message(char *messagefile, struct __config cfg);


/*
 * init POP3 session
 */

void init_child(){
   n_msgs = 0;
   prevlen = 0;
   memset(password, 0, SMALLBUFSIZE);
}


/*
 * create a socket and SSL handler for accessing the real POP3 server
 */

int create_socket_and_ssl_handler(int *sd, SSL **ssl, SSL_CTX **ctx, int use_ssl){
   SSL_METHOD *meth = NULL;

   if((*sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
      syslog(LOG_PRIORITY, "ERR: create socket");
      return 0;
   }

   if(use_ssl == 1){
      SSL_load_error_strings();
      SSLeay_add_ssl_algorithms();
      meth = TLSv1_client_method();
      *ctx = SSL_CTX_new(meth);
      if(*ctx == NULL) return 0;
      *ssl = SSL_new(*ctx);
      if(*ssl == NULL) return 0;
      if(SSL_set_fd(*ssl, *sd) != 1) return 0;
   }

   return 1;
}


/*
 * read answer from remote POP3 server until trailing dot(.)
 */

int read_until_period(int sd, SSL *ssl, int sd2, SSL *ssl2, int use_ssl, int fd){
   int n, i=0, prevlen=0;
   char last2buf[2*MAXBUFSIZE], buf[MAXBUFSIZE];

   memset(last2buf, 0, 2*MAXBUFSIZE);

   do {
      n = recvtimeoutssl(sd2, buf, MAXBUFSIZE, TIMEOUT, use_ssl, ssl2);
      if(fd != -1) write(fd, buf, n);
      else write1(sd, buf, 0, ssl);

      if(i == 0 && strncmp(buf, "-ERR", 4) == 0) return 0;

      if(n >= 5){
         memset(last2buf, 0, 2*MAXBUFSIZE);
         prevlen = 0;
      }

      /* copy or append */

      memcpy(last2buf+prevlen, buf, n);
      prevlen += n;

      if(search_in_buf(last2buf, 2*MAXBUFSIZE, SMTP_CMD_PERIOD, 5) == 1){
         //syslog(LOG_PRIORITY, "found the trailing dot(.)");
         return 1;
      }

      /* if remote pop3 server gives us data in very small chunks */
      if(prevlen > MAXBUFSIZE)
         memmove(last2buf, &last2buf[prevlen-5], 5);

 
      i++;
   } while(n > 0);

   return 0;
}


/*
 * send message from remote pop3 server to the client
 */

int send_message_to_client(int sd, int use_ssl, SSL *ssl, char *messagefile, int is_spam, struct __config cfg){
   int fd, n, is_header=1, num_of_reads=0, put_subject_spam_prefix=0;
   char *p, *q, bigbuf[MAX_MAIL_HEADER_SIZE], spaminessbuf[SMALLBUFSIZE];

   fd = open(messagefile, O_RDONLY);
   /* send an error message back if we cannot open the message */
   if(fd == -1){
      write1(sd, "-ERR internal error, try again later\r\n", use_ssl, ssl);
      return 0;
   }

   if(is_spam == 1){
      snprintf(spaminessbuf, SMALLBUFSIZE-1, "\r\n%s%s\r\n%s\r\n\r\n", cfg.clapf_header_field, messagefile, cfg.clapf_spam_header_field);
      if(strlen(cfg.spam_subject_prefix) > 1)
         put_subject_spam_prefix = 1;
   }
   else
      snprintf(spaminessbuf, SMALLBUFSIZE-1, "\r\n%s%s\r\n%sOK\r\n\r\n", cfg.clapf_header_field, messagefile, cfg.clapf_header_field);

   while((n = read(fd, bigbuf, MAX_MAIL_HEADER_SIZE)) > 0){
      num_of_reads++;

      /* if this is the first read and it contains all the header lines */

      if(num_of_reads == 1){

         /* first find the end of the mail header */

         p = strstr(bigbuf, "\r\n\r\n");
         if(!p){
            p = strstr(bigbuf, "\n\n");
         }

         if(p){
            is_header = 0;
            *p = '\0'; p++;

            if(put_subject_spam_prefix == 1){
               q = strstr(bigbuf, "\nSubject:");
               if(q){
                  *(q+9) = '\0';
                  write1(sd, bigbuf, use_ssl, ssl);
                  write1(sd, cfg.spam_subject_prefix, use_ssl, ssl);
                  write1(sd, q+10, use_ssl, ssl);               
               }
            }
            else
               write1(sd, bigbuf, use_ssl, ssl);


            write1(sd, spaminessbuf, use_ssl, ssl);
            write1(sd, p, use_ssl, ssl);
         }
         else {
            write1(sd, bigbuf, use_ssl, ssl);
         } 
      }
      else
         write1(sd, bigbuf, use_ssl, ssl);

   }

   close(fd);

   return 1;
}


/*
 * handle POP3 session
 */

void ooop(int new_sd, int use_ssl, SSL *ssl, struct __config cfg){
   int n, state, fd, is_spam;
   unsigned long message_size;
   char *p, cmdbuf[MAXBUFSIZE], buf[2*MAXBUFSIZE];
   char errmsg[SMALLBUFSIZE], messagefile[3*RND_STR_LEN+1];
   struct session_data sdata;
   struct _state sstate;
   struct c_res result;


   init_child();
   state = POP3_STATE_INIT;
   dbh = 0;
   prevlen = 0;

#ifdef HAVE_LIBWRAP
   request_init(&req, RQ_DAEMON, PROGNAME, RQ_FILE, new_sd, 0);
   fromhost(&req);
   if(!hosts_access(&req)){
      syslog(LOG_PRIORITY, "denied connection from %s", eval_client(&req));
      goto QUITTING;
   }
   else
      syslog(LOG_PRIORITY, "connection from %s", eval_client(&req));
#endif


   /* send POP3 banner */
   write1(new_sd, POP3_RESP_BANNER, 0, ssl);

   while((n = recvtimeoutssl(new_sd, cmdbuf, MAXBUFSIZE-1, TIMEOUT, 0, ssl)) > 0){

      //syslog(LOG_PRIORITY, "got command: %s (%d)", cmdbuf, n);

      /* buffer command until we get \r\n */

      if(n >= 2){
         memset(buf, 0, 2*MAXBUFSIZE);
         prevlen = 0;
      }

      /* copy or append */

      memcpy(buf+prevlen, cmdbuf, n);
      prevlen += n;

      if(search_in_buf(buf, 2*MAXBUFSIZE, "\r\n", 2) == 0){

         /* don't let the client bother us any longer if he will not want to send
            the trailing \r\n sequence */

         if(prevlen >= MAXBUFSIZE) goto QUITTING;

         continue;
      }





      /* QUIT command */

      if(strncasecmp(buf, POP3_CMD_QUIT, strlen(POP3_CMD_QUIT)) == 0){

         write1(sd, buf, use_ssl, ssl2);
         n = recvtimeoutssl(sd, cmdbuf, MAXBUFSIZE, TIMEOUT, use_ssl, ssl2);

         write1(new_sd, cmdbuf, 0, ssl);

         state = POP3_STATE_FINISHED;

         goto QUITTING;
      }


      /* USER command */

      if(strncasecmp(buf, POP3_CMD_USER, strlen(POP3_CMD_USER)) == 0){
         state = POP3_STATE_USER;

         p = strchr(buf, ' ');
         if(p){
            *p = '\0';
            p++;
            p[strlen(p)-2] = '\0';
            memset(username, 0, SMALLBUFSIZE);
            strncpy(username, p, SMALLBUFSIZE-1);

            /* USER username:pop3_server_name */
         }

         write1(new_sd, POP3_RESP_SEND_PASS, 0, ssl);
         continue;
      }


      /* PASS command */

      if(strncasecmp(buf, POP3_CMD_PASS, strlen(POP3_CMD_PASS)) == 0){
         if(state < POP3_STATE_USER)
            write1(new_sd, POP3_RESP_INVALID_CMD, 0, ssl);
         else {

            p = strchr(buf, ' ');
            if(p){
               *p = '\0';
               p++;
               p[strlen(p)-2] = '\0';
               memset(password, 0, SMALLBUFSIZE);
               strncpy(password, p, SMALLBUFSIZE-1);
            }

         }

         /* do autentication on the remote site */

         create_socket_and_ssl_handler(&sd, &ssl2, &ctx2, use_ssl);

         ret = remote_auth(username, password, &sd, &ssl2, use_ssl, &errmsg[0]);
         write1(new_sd, errmsg, 0, ssl);

         if(ret == 0) goto QUITTING;


         /* load user profile now... */

         cfg.training_mode = 0;
         cfg.initial_1000_learning=0;

         sdata.uid = 0;
         sdata.num_of_rcpt_to = -1;
         memset(sdata.rcptto[0], 0, MAXBUFSIZE);

         /* open database */

         dbh = init_mydb(cfg.mydbfile, mhash, &sdata);

         state = POP3_STATE_PASS;

         continue;
      }


      /* HELP command */

      if(strncasecmp(buf, POP3_CMD_HELP, strlen(POP3_CMD_HELP)) == 0){
         write1(new_sd, POP3_RESP_OK, 0, ssl);
         continue;
      }



      /* NOOP command */

      if(strncasecmp(buf, POP3_CMD_NOOP, strlen(POP3_CMD_NOOP)) == 0){
         write1(new_sd, POP3_RESP_OK, 0, ssl);
         continue;
      }


      /* STAT command */

      if(strncasecmp(buf, POP3_CMD_STAT, strlen(POP3_CMD_STAT)) == 0){
         if(state < POP3_STATE_PASS) goto INVALID_CMD;

         write1(sd, buf, use_ssl, ssl2);
         n = recvtimeoutssl(sd, cmdbuf, MAXBUFSIZE, TIMEOUT, use_ssl, ssl2);

         write1(new_sd, cmdbuf, 0, ssl);
         continue;
      }


      /* CAPA command */

      if(strncasecmp(buf, POP3_CMD_CAPA, strlen(POP3_CMD_CAPA)) == 0){
         write1(new_sd, POP3_RESP_CAPA, 0, ssl);
         continue;
      }


      /* LIST command */

      if(strncasecmp(buf, POP3_CMD_LIST, strlen(POP3_CMD_LIST)) == 0){
         if(state < POP3_STATE_PASS) goto INVALID_CMD;

         write1(sd, buf, use_ssl, ssl2);

         if(strchr(buf, ' ')){
            n = recvtimeoutssl(sd, cmdbuf, MAXBUFSIZE, TIMEOUT, use_ssl, ssl2);
            write1(new_sd, cmdbuf, 0, ssl);
         }
         else {
            read_until_period(new_sd, ssl, sd, ssl2, use_ssl, -1);
         }
         continue;
      }


      /* RETR command */

      if(strncasecmp(buf, POP3_CMD_RETR, strlen(POP3_CMD_RETR)) == 0){
         if(state < POP3_STATE_PASS) goto INVALID_CMD;

         if(strchr(buf, ' ')){

            /* before downloading the message, let's see how big it is */

            strncpy(buf, "LIST", 4);

            write1(sd, buf, use_ssl, ssl2);
            n = recvtimeoutssl(sd, cmdbuf, MAXBUFSIZE, TIMEOUT, use_ssl, ssl2);

            trim(cmdbuf);

            is_spam = 0;
            message_size = 0;
            fd = -1;

            p = strrchr(cmdbuf, ' ');
            if(p){
               message_size = atol(++p);
            }

            strncpy(buf, "RETR", 4);

            write1(sd, buf, use_ssl, ssl2);


            /* we have to filter this message */

            if(message_size > 30 && message_size < cfg.max_message_size_to_filter){
               make_rnd_string(messagefile);
               fd = open(messagefile, O_CREAT|O_EXCL|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
            }

            syslog(LOG_PRIORITY, "%s: reading message from remote pop3 server", messagefile);
            read_until_period(new_sd, ssl, sd, ssl2, use_ssl, fd);
            syslog(LOG_PRIORITY, "%s: reading %ld bytes is done", messagefile, message_size);

            if(fd != -1){
               close(fd);
               syslog(LOG_PRIORITY, "%s: processing message", messagefile);

               sstate = parse_message(messagefile, sdata, cfg);

               /*
                * TODO: whitelist check
                */




               /* rbl first, if it can condemn the message */

               if(cfg.rbl_condemns_the_message == 1){
                  reverse_ipv4_addr(sstate.ip);
                  if(rbl_list_check(cfg.rbl_domain, sstate.ip) == 1){
                     is_spam = 1;
                     syslog(LOG_PRIORITY, "%s: found on rbl: %s", messagefile, sstate.ip); 
                     goto END_OF_SPAMCHECK;
                  }
               }

               /* statistical check */

               result = bayes_file(mhash, messagefile, sstate, sdata, cfg);

               if(result.spaminess >= cfg.spam_overall_limit)
                  is_spam = 1;

            END_OF_SPAMCHECK:

               free_and_print_list(sstate.first, 0);

               /*
                  we sending everything to the client via cleartext, and stunnel
                  will encrypt it if the client really wants so
               */

               syslog(LOG_PRIORITY, "%s: sending message back to client (spam: %d)", messagefile, is_spam);
               send_message_to_client(new_sd, 0, ssl, messagefile, is_spam, cfg);

               unlink(messagefile);
            }

         }
         else write1(new_sd, POP3_RESP_NO_SUCH_MESSAGE, 0, ssl);

         continue;
      }



      /* TOP command */

      if(strncasecmp(buf, POP3_CMD_TOP, strlen(POP3_CMD_TOP)) == 0){
         if(state < POP3_STATE_PASS) goto INVALID_CMD;

         write1(sd, buf, use_ssl, ssl2);
         read_until_period(new_sd, ssl, sd, ssl2, use_ssl, -1);

         continue;
      }


      /* UIDL command */

      if(strncasecmp(buf, POP3_CMD_UIDL, strlen(POP3_CMD_UIDL)) == 0){
         if(state < POP3_STATE_PASS) goto INVALID_CMD;

         write1(sd, buf, use_ssl, ssl2);

         if(strchr(buf, ' ')){
            n = recvtimeoutssl(sd, cmdbuf, MAXBUFSIZE, TIMEOUT, use_ssl, ssl2);
            write1(new_sd, cmdbuf, 0, ssl);
         }
         else {
            read_until_period(new_sd, ssl, sd, ssl2, use_ssl, -1);
         }

         continue;
      }



      /* DELE command */

      if(strncasecmp(buf, POP3_CMD_DELE, strlen(POP3_CMD_DELE)) == 0){
         if(state < POP3_STATE_PASS) goto INVALID_CMD;

         write1(sd, buf, use_ssl, ssl2);
         n = recvtimeoutssl(sd, cmdbuf, MAXBUFSIZE, TIMEOUT, use_ssl, ssl2);

         write1(new_sd, cmdbuf, 0, ssl);

         continue;
      }



      /* RSET command */

      if(strncasecmp(buf, POP3_CMD_RSET, strlen(POP3_CMD_RSET)) == 0){
         if(state < POP3_STATE_PASS) goto INVALID_CMD;

         write1(sd, buf, use_ssl, ssl2);
         n = recvtimeoutssl(sd, cmdbuf, MAXBUFSIZE, TIMEOUT, use_ssl, ssl2);

         write1(new_sd, cmdbuf, 0, ssl);

         continue;
      }



      /* LAST command */

      if(strncasecmp(buf, POP3_CMD_LAST, strlen(POP3_CMD_LAST)) == 0){
         if(state < POP3_STATE_PASS) goto INVALID_CMD;

         write1(sd, buf, use_ssl, ssl2);
         n = recvtimeoutssl(sd, cmdbuf, MAXBUFSIZE, TIMEOUT, use_ssl, ssl2);

         write1(new_sd, cmdbuf, 0, ssl);

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

   close(sd);

#ifdef HAVE_MYDB
   close_mydb(mhash);
#endif

   if(use_ssl == 1){
      SSL_free(ssl2);
      SSL_CTX_free(ctx2);
   }

   _exit(0);
}
