/*
 * splitmbox.c, SJ
 */

#include <kospam.h>


int main(int argc, char **argv){
   int tot_msgs = 0;
   char buf[MAXBUFSIZE], fname[MAXBUFSIZE];
   FILE *F, *f=NULL;

   if(argc < 3) {
      printf("usage: <mbox file> <messagefile basename>\n");
      return 1;
   }

   F = fopen(argv[1], "r");
   if(!F) {
      printf("ERROR: open %s\n", argv[1]);
      return 1;
   }

   while(fgets(buf, sizeof(buf)-1, F)){
      chop_newlines(buf, strlen(buf));

      if(buf[0] == 'F' && buf[1] == 'r' && buf[2] == 'o' && buf[3] == 'm' && buf[4] == ' '){
         tot_msgs++;
         if(f) fclose(f);
         printf("parsing message %d ...\n", tot_msgs);
         snprintf(fname, 200, "%s-%d", argv[2], tot_msgs);
         f = fopen(fname, "w+");
         continue;
      }

      if(f) fprintf(f, "%s\r\n", buf);
   }

   if(f) fclose(f);

   fclose(F);

   return 0;
}
