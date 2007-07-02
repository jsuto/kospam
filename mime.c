/*
 * mime.c 2006.02.09, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include "misc.h"
#include "decoder.h"
#include "parser.h"
#include "mime.h"
#include "config.h"

struct rfc822_attachment extract_from_rfc822(char *message){
   FILE *f, *f2;
   char *p, buf[MAXBUFSIZE], puf[MAXBUFSIZE], boundary[BOUNDARY_LEN], boundary2[BOUNDARY_LEN], newfile[SMALLBUFSIZE];
   int x, len, is_header, has_boundary, has_content_type, seen_boundary, search_new_boundary, process_mime_part, base64;
   struct rfc822_attachment Q;

   memset(Q.tmpdir, 0, SMALLBUFSIZE);
   Q.cnt = 0;
   Q.result = 0;

   if(message == NULL)
      return Q;

   f= fopen(message, "r");
   if(!f){
      syslog(LOG_PRIORITY, "cannot open: %s", message);
      return Q;
   }

   /* init */

   is_header = 1;
   has_boundary = 0;
   has_content_type = 0;
   seen_boundary = 0;
   search_new_boundary = 0;
   process_mime_part = 0;
   base64 = 0;
   Q.cnt = 0;
   f2 = NULL;

   memset(boundary, 0, BOUNDARY_LEN);
   memset(boundary2, 0, BOUNDARY_LEN);

   memset(Q.tmpdir, 0, SMALLBUFSIZE);
   strncpy(Q.tmpdir, "aaXXXXXX", SMALLBUFSIZE-1);

   if(mkdtemp(Q.tmpdir) == NULL){
      syslog(LOG_PRIORITY, "failed to create temp dir: %s", Q.tmpdir);
      return Q;
   }

   while(fgets(buf, MAXBUFSIZE-1, f)){
      if(is_header == 1 && (buf[0] == '\r' || buf[0] == '\n')){
         is_header = 0;
         continue;
      }

      if(strncasecmp(buf, "Content-Transfer-Encoding: base64", strlen("Content-Transfer-Encoding: base64")) == 0)
         base64 = 1;

      if(strncasecmp(buf, "Content-Type: ", strlen("Content-Type: ")) == 0)
         has_content_type = 1;

      if(strncmp(buf, "Content-Type: multipart/alternative", strlen("Content-Type: multipart/alternative")) == 0)
         search_new_boundary = 1;

      /* search for bondary in the header */

      if(is_header == 1){
         p = str_case_str(buf, "boundary");
         if(p){
            x = extract_boundary(p, boundary, BOUNDARY_LEN-1);
            if(x == 1) has_boundary = 1;
         }
      }

      /* extract stuff from the mail body */

      else if(has_boundary == 1){

         /* search new boundary in the body */

         if(search_new_boundary == 1 && (p = str_case_str(buf, "boundary")) ){
            x = extract_boundary(p, boundary2, BOUNDARY_LEN-1);
            if(x == 1) search_new_boundary = 0;
         }

         /* new boundary, reset variables */

         if(strstr(buf, boundary) || (boundary2[0] && strstr(buf, boundary2)) ){
            seen_boundary = 1;
            base64 = 0;
            process_mime_part = 0;
            has_content_type = 0;
         }

         if(seen_boundary == 1 && has_content_type == 1 && (buf[0] == '\r' || buf[0] == '\n')){
            process_mime_part = 1;

            /* create new file name and open it for writing */

               Q.cnt++;
               memset(newfile, 0, SMALLBUFSIZE);
               snprintf(newfile, SMALLBUFSIZE-1, "%s/%ld", Q.tmpdir, Q.cnt);
               seen_boundary = 0;

               if(f2)
                 fclose(f2);

               f2 = fopen(newfile, "w+");
               if(!f2){
                  syslog(LOG_PRIORITY, "error while extracting creating: %s", newfile);
                  return Q;
               }

            continue;
         }

         if(process_mime_part == 1){

            /* process base64 encoded stuff */

            if(base64 == 1){
               if(buf[0] != '\r' && buf[0] != '\n'){

                  sanitiseBase64(buf);
                  len = base64_decode(buf, puf);
                  if(len > 0)
                     fwrite(puf, len, 1, f2);                  
               }
            }

            /* by default it is assumed to be textual */

            else
               fprintf(f2, "%s", buf);


         }


      }
      memset(buf, 0, MAXBUFSIZE-1);

   }
   fclose(f);

   if(f2)
     fclose(f2);

   Q.result = 1;

   return Q;
}
