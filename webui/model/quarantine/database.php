<?php

class ModelQuarantineDatabase extends Model {

   public function CreateDatabase() {
      $type = "";

      $Q = Registry::get('Q');

      if(QUARANTINE_DRIVER == "mysql") { $type = " Engine=InnoDB character set utf8 "; }

      $query = $Q->query("
         create table if not exists " . TABLE_QUARANTINE . " (
            id char(32) not null,
            uid int unsigned not null,
            is_spam char(1) default null,
            `from` char(64) default null,
            subj char(64) default null,
            size int default 0,
            ts int default 0,
            hidden integer(1)
         ) $type");

      $query = $Q->query("create index " . TABLE_QUARANTINE . "_idx on " . TABLE_QUARANTINE . " (uid, is_spam, ts, hidden) ");

      $query = $Q->query("
         create table if not exists " . TABLE_SEARCH . " (
            uid int default 0,
            term char(128) not null,
            ts int default 0
         ) $type");

      $query = $Q->query("create index " . TABLE_SEARCH . "_idx on " . TABLE_SEARCH . " (uid) ");

      return 0;
   }


   public function get_new_files($files = array(), $dir, $maxts = 0, $expr = '', $uid = 0){
      $new_files = array();
      $Q = Registry::get('Q');

      while(list($k, $v) = each($files)){
         if(!preg_match($expr, $v)) { continue; }

         $st = stat($dir . "/" . $v);

         if($st['ctime'] >= $maxts){

            $a = explode(".", $v);
            $ok = 1;

            if($st['ctime'] == $maxts){
               $query = $Q->query("select count(*) as count from " . TABLE_QUARANTINE . " where uid=" . (int)$uid . " and id='" . $Q->escape($a[1]) . "'");
               if(isset($query->row['count']) && $query->row['count'] == 1){ $ok = 0; }
            }

            if($ok == 1) {

               $new_files[] = array(
                                    'id'      => $a[1],
                                    'is_spam' => $a[0],
                                    'ts'      => $st['ctime'],
                                    'size'    => $st['size']
                                   );
            }

         }
         else {
            break;
         }
      }

      return $new_files;
   }


   public function PopulateDatabase($dir = '', $uid = 0, $group_q_dirs = array() ) {
      $count = 0;

      if($dir == "" || !file_exists($dir)) { return 0; }

      $Q = Registry::get('Q');

      /* check if the additional queue directories exist */

      while(list($k, $v) = each($group_q_dirs) ) {
         if(!file_exists($v)) {
            $dir1 = dirname($v);
            $dir2 = dirname($dir1);
            $dir3 = dirname($dir2);

            if(!file_exists($dir3)) { mkdir($dir3, 0770); }
            if(!file_exists($dir2)) { mkdir($dir2, 0770); }
            if(!file_exists($dir1)) { mkdir($dir1, 0770); }
            mkdir($v, 0770);
         }
      }


      /* remove entries not present in the filesystem */

      $oldest_ts = 0;

      $files = scandir($dir, 0);

      $oldest_ts = $this->getOldestMessageTimestamp($dir, $files, '/^(h\.[0-9a-f]+)$/');
      $ts = $this->getOldestMessageTimestamp($dir, $files, '/^(s\.[0-9a-f]+)$/');

      if($ts > 0 && $ts < $oldest_ts) $oldest_ts = $ts;

      $ts = $this->getOldestMessageTimestamp($dir, $files, '/^(v\.[0-9a-f]+)$/');
      if($ts > 0 && $ts < $oldest_ts) $oldest_ts = $ts;

      if($oldest_ts > 0) {
         $query = $Q->query("delete from " . TABLE_QUARANTINE . " where uid=" . (int)$uid . " and ts < $oldest_ts");
      }


      /* determine what files are to be populated */

      $query = $Q->query("select max(ts) as maxts from " . TABLE_QUARANTINE . " where uid=" . (int)$uid);
      $maxts = (int)@$query->row['maxts'];

      $files = scandir($dir, 1);

      $new_hams = $this->get_new_files($files, $dir, $maxts, '/^(h\.[0-9a-f]+)$/', $uid);
      reset($files);
      $new_spams = $this->get_new_files($files, $dir, $maxts, '/^(s\.[0-9a-f]+)$/', $uid);
      reset($files);
      $new_viruses = $this->get_new_files($files, $dir, $maxts, '/^(v\.[0-9a-f]+)$/', $uid);

      $new_files = array_merge($new_hams, $new_spams, $new_viruses);

      $query = $Q->query("BEGIN");

      foreach ($new_files as $file){
         list ($mailfrom, $subject) = $this->model_quarantine_message->scan_message($dir, $file['is_spam'] . "." . $file['id']);

         $query = $Q->query("insert into " . TABLE_QUARANTINE . " (id, uid, ts, size, is_spam, hidden, `from`, subj) values('" . $file['id'] . "', $uid, " . $file['ts'] . ", " . $file['size'] . ", '" . $file['is_spam'] . "', 0, '" . $Q->escape($mailfrom) . "', '" . $Q->escape($subject) . "')");

         // create hard links to the additional user directories
         reset($group_q_dirs);
         while(list($k, $v) = each($group_q_dirs) ) {
            if(!file_exists("$v/" . $file['is_spam'] . "." . $file['id'])) { link($dir . "/" . $file['is_spam'] . "." . $file['id'], "$v/" . $file['is_spam'] . "." . $file['id']); }
         }

         $count++;
      }

      $query = $Q->query("COMMIT");

      return $count;
   }


   private function getOldestMessageTimestamp($dir = '', $files = array(), $expr = '') {
      $ts = 0;

      while(list($k, $v) = each($files)){
         if(preg_match($expr, $v)) {
            $st = stat($dir . "/" . $v);
            $ts = $st['ctime'];
            break;
         }
      }

      reset($files);

      return $ts;
   }


   public function getMessages($uids = array(), $page = 0, $page_len = PAGE_LEN, $date = '', $from = '', $subj = '', $hamspam = '', $sort = 'ts', $order = 0) {
      $n = $total_size = $i = 0;
      $messages = array();
      $limit = "";
      $uid_list = "";


      $Q = Registry::get('Q');


      $where_cond = "WHERE hidden=0";

      if($date) {
         if(QUARANTINE_DRIVER == 'mysql') { $datesql = "UNIX_TIMESTAMP('" . $Q->escape($date) . " 00:00:00') "; }
         if(QUARANTINE_DRIVER == 'sqlite') { $datesql = "strftime('%s', '" . $Q->escape($date) . " 00:00:00') "; }

         $where_cond .= " AND ts >= $datesql";

         if(QUARANTINE_DRIVER == 'mysql') { $datesql = "UNIX_TIMESTAMP('" . $Q->escape($date) . " 23:59:59') "; }
         if(QUARANTINE_DRIVER == 'sqlite') { $datesql = "strftime('%s', '" . $Q->escape($date) . " 23:59:59') "; }

         $where_cond .= " AND ts < $datesql";
      }

      if(count($uids) > 0) {

         while(list($k, $v) = each($uids)) {
            $uid_list .= ",$v";
         }

         $where_cond .=  " AND uid IN (" . preg_replace("/^\,/", "", $uid_list) . ")";
      }


      if($hamspam == "HAM") { $where_cond .= " AND is_spam='h'"; }
      else if($hamspam == "SPAM") {
         if(Registry::get('admin_user') == 1) { $where_cond .= " AND (is_spam='s' OR is_spam='v')"; }
         else { $where_cond .= " AND is_spam='s' "; }
      }
      else {
         if(Registry::get('admin_user') == 1) { $where_cond .= " AND (is_spam='s' OR is_spam='v' OR (is_spam='h' and size>0))"; }
         else { $where_cond .= " AND (is_spam='s' OR (is_spam='h' and size>0))"; }
      }

      if($from) { $where_cond .= " AND `from` like '%" . $Q->escape($from) . "%'"; }
      if($subj) { $where_cond .= " AND subj like '%" . $Q->escape($subj) . "%'"; }

      /* sort order */

      if($order == 0) { $order = "ASC"; }
      else { $order = "DESC"; }


      /* select total size */

      $query = $Q->query("select count(size) as total_num, sum(size) as total_size from " . TABLE_QUARANTINE . " $where_cond");


      if(isset($query->row['total_size'])) { $total_size = $query->row['total_size']; }
      if(isset($query->row['total_num'])) { $n = $query->row['total_num']; }

      if($page_len > 0) { $limit = " LIMIT " . $page_len*$page . "," . $page_len; }

      $query = $Q->query("select * from " . TABLE_QUARANTINE . " $where_cond ORDER BY `$sort` $order $limit");

      $i = $page_len*$page;

      if(!isset($query->rows)) { return array(0,0,0); }

      foreach ($query->rows as $message){
         if($message['size'] < 5) { $n--; continue; }

         $i++;

         $message['subj'] = preg_replace('/"/', "&quot;", $message['subj']);

         if(!isset($users[$message['uid']])) {
            $username = $this->model_user_user->getUsernameByUid($message['uid']);
            $users[$message['uid']] = $username;
         }
         else {
            $username = $users[$message['uid']];
         }

         $messages[] = array(
                             'i' => $i,
                             'id' => $message['is_spam'] . '.' . $message['id'],
                             'uid' => $message['uid'],
                             'username' => $username,
                             'from' => $message['from'],
                             'shortfrom' => $this->MakeShortString($message['from'], MAX_CGI_FROM_SUBJ_LEN),
                             'subject' => $message['subj'],
                             'shortsubject' => $this->MakeShortString($message['subj'], MAX_CGI_FROM_SUBJ_LEN),
                             'size' => $this->model_quarantine_message->NiceSize($message['size']),
                             'date' => date("Y.m.d.", $message['ts'])
                            );

      }

      if($total_size == 0) { $n = 0; }

      return array($n, $total_size, $messages);
   }


   public function RemoveEntry($id = '', $uid = 0) {

      $Q = Registry::get('Q');

      $a = explode(".", $id);

      $query = $Q->query("update " . TABLE_QUARANTINE . " set hidden=1 where uid=" . (int)$uid . " AND id='" . $Q->escape($a[1]) . "'");

      if($query->error == 0 && $Q->countAffected() == 1) { return 1; }

      return 0;
   }


   public function RemoveAllEntries($uid = 0) {

      $Q = Registry::get('Q');

      if((int)$uid > 0) {
         $query = $Q->query("update " . TABLE_QUARANTINE . " set hidden=1 where uid=" . (int)$uid);
      } else {
         $query = $Q->query("update " . TABLE_QUARANTINE . " set hidden=1");
      }

      return 0;
   }


   public function getSearchTerms($uid = 0) {
      $Q = Registry::get('Q');

      $query = $Q->query("select * from " . TABLE_SEARCH . " where uid=" . (int)$uid . " order by ts desc");
      if(isset($query->rows)) { return $query->rows; }

      return array();
   }


   public function addSearchTerm($date = '', $from = '', $to = '', $subj = '', $hamspam = '', $uid = 0) {
      $term = "date=$date&subj=$subj&from=$from&to=$to&hamspam=$hamspam";

      $Q = Registry::get('Q');

      $query = $Q->query("update " . TABLE_SEARCH . " set ts=" . time() . " where term='" . $Q->escape($term) . "' and uid=" . (int)$uid);
      if($Q->countAffected() == 0) {
         $query = $Q->query("insert into " . TABLE_SEARCH . " (uid, term, ts) values(" . (int)$uid . ", '" . $Q->escape($term) . "', " . time() . ")");
      }
   }


   private function MakeShortString($what, $length) {
      $l = $n = 0;
      $s = "";

      $x = preg_split("/\s/", $what);

      while(list($k, $v) = each($x)){
         if($l < $length){
            if($n > 0) $s .= " $v";
            else       $s .= $v;

            $l += strlen($v);
         }
         else break;

         $n++;
      }

      return $s;
   }


}

?>
