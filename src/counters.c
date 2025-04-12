/*
 * counters.c, SJ
 */

#include <kospam.h>


void update_counters(MYSQL *conn, struct counters *counters){
   char buf[MAXBUFSIZE];
   snprintf(buf, sizeof(buf)-1, "UPDATE %s SET rcvd=rcvd+%llu, size=size+%llu, mynetwork=mynetwork+%llu, ham=ham+%llu, "
                                "spam=spam+%llu, possible_spam=possible_spam+%llu, unsure=unsure+%llu, minefield=minefield+%llu, "
                                "virus=virus+%llu, zombie=zombie+%llu, fp=fp+%llu, fn=fn+%llu",
                                SQL_COUNTER_TABLE, counters->c_rcvd, counters->c_size, counters->c_mynetwork, counters->c_ham,
                                counters->c_spam, counters->c_possible_spam, counters->c_unsure, counters->c_minefield,
                                counters->c_virus, counters->c_zombie, counters->c_fp, counters->c_fn);

   p_query(conn, buf);
}
