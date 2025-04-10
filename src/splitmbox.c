/*
 * splitmbox.c, SJ
 */

#include <kospam.h>


int main(int argc, char **argv){
   int tot_msgs = 0;
   char buf[MAXBUFSIZE], fname[MAXBUFSIZE];
   FILE *F, *f=NULL;

   if(argc < 3)
      __fatal("usage: <mbox file> <messagefile basename>");


   F = fopen(argv[1], "r");
   if(!F)
      __fatal("open");

   while(fgets(buf, sizeof(buf)-1, F)){
      trim_buffer(buf);

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
