<?php

class ModelQuarantineDatabase extends Model {

   public function CreateDatabase() {

      $Q = Registry::get('Q');

      $query = $Q->query("
         create table if not exists quarantine (
            id char(32) not null,
            is_spam char(1) default null,
            `from` char(64) default null,
            subj char(64) default null,
            size int default 0,
            ts int default 0,
            hidden integer(1)
         )");

      $query = $Q->query("
         create table if not exists search (
            term char(128) not null,
            ts int default 0
         )");

      return 0;
   }


   public function get_new_files($files = array(), $dir, $maxts = 0, $expr = ''){
      $new_files = array();
      $Q = Registry::get('Q');

      while(list($k, $v) = each($files)){
         if(!preg_match($expr, $v)) { continue; }

         $st = stat($dir . "/" . $v);

         if($st['ctime'] >= $maxts){

            $a = explode(".", $v);
            $ok = 1;

            if($st['ctime'] == $maxts){
               $query = $Q->query("select count(*) as count from quarantine where id='" . $Q->escape($a[1]) . "'");
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


   public function PopulateDatabase($dir = '') {
      if($dir == "" || !file_exists($dir)) { return 0; }

      $expr = '/^([sh]\.[0-9a-f]+)$/';

      $Q = Registry::get('Q');


      /* remove entries not present in the filesystem */

      $oldest_ts = 0;

      $files = scandir($dir, 0);
      while(list($k, $v) = each($files)){
         if(preg_match($expr, $v)) {
            $st = stat($dir . "/" . $v);
            $oldest_ts = $st['ctime'];
            break;
         }
      }

      if($oldest_ts > 0) {
         $query = $Q->query("delete from quarantine where ts < $oldest_ts");
      }


      /* determine what files are to be populated */

      $query = $Q->query("select max(ts) as maxts from quarantine");
      $maxts = (int)@$query->row['maxts'];


      $files = scandir($dir, 1);

      $new_hams = $this->get_new_files($files, $dir, $maxts, '/^(h\.[0-9a-f]+)$/');
      reset($files);
      $new_spams = $this->get_new_files($files, $dir, $maxts, '/^(s\.[0-9a-f]+)$/');

      $new_files = array_merge($new_hams, $new_spams);

      $query = $Q->query("BEGIN");

      foreach ($new_files as $file){
         list ($mailfrom, $subject) = $this->model_quarantine_message->scan_message($dir, $file['is_spam'] . "." . $file['id']);

         $query = $Q->query("insert into quarantine (id, ts, size, is_spam, hidden, `from`, subj) values('" . $file['id'] . "', " . $file['ts'] . ", " . $file['size'] . ", '" . $file['is_spam'] . "', 0, '" . $Q->escape($mailfrom) . "', '" . $Q->escape($subject) . "')");
      }

      $query = $Q->query("COMMIT");

      return 0;
   }


   public function getMessages($dir = '', $username = '', $page = 0, $page_len = PAGE_LEN, $from = '', $subj = '', $hamspam = '') {
      $n = $total_size = $i = 0;
      $messages = array();

      $where_cond = "WHERE hidden=0";

      if($dir == "" || !file_exists($dir)) { return array($n, $total_size, $messages); }

      $Q = Registry::get('Q');

      if($hamspam == "HAM") { $where_cond .= " AND is_spam='h'"; }
      if($hamspam == "SPAM") { $where_cond .= " AND is_spam='s'"; }

      if($from) { $where_cond .= " AND `from` like '%" . $Q->escape($from) . "%'"; }
      if($subj) { $where_cond .= " AND subj like '%" . $Q->escape($subj) . "%'"; }

      /* select total size */

      $query = $Q->query("select count(size) as total_num, sum(size) as total_size from quarantine $where_cond");

      if(isset($query->row['total_size'])) { $total_size = $query->row['total_size']; }
      if(isset($query->row['total_num'])) { $n = $query->row['total_num']; }

      $query = $Q->query("select * from quarantine $where_cond ORDER BY ts DESC LIMIT " . $page_len*$page . "," . $page_len);

      $i = $page_len*$page;

      foreach ($query->rows as $message){
         if($message['size'] < 5) { $n--; }

         $i++;

         $message['subj'] = preg_replace('/"/', "&quot;", $message['subj']);

         $messages[] = array(
                             'i' => $i,
                             'id' => $message['is_spam'] . '.' . $message['id'],
                             'from' => $message['from'],
                             //'shortfrom' => strlen($message['from']) > 6+MAX_CGI_FROM_SUBJ_LEN ? substr($message['from'], 0, MAX_CGI_FROM_SUBJ_LEN) . "..." : $message['from'],
                             'shortfrom' => $this->MakeShortString($message['from'], MAX_CGI_FROM_SUBJ_LEN),
                             'subject' => $message['subj'],
                             //'shortsubject' => strlen($message['subj']) > 6+MAX_CGI_FROM_SUBJ_LEN ? substr($message['subj'], 0, MAX_CGI_FROM_SUBJ_LEN) . "..." : $message['subj'],
                             'shortsubject' => $this->MakeShortString($message['subj'], MAX_CGI_FROM_SUBJ_LEN),
                             'size' => $this->model_quarantine_message->NiceSize($message['size']),
                             'date' => date("Y.m.d.", $message['ts']),
                            );

      }

      return array($n, $total_size, $messages);
   }


   public function RemoveEntry($id) {

      $Q = Registry::get('Q');

      $a = explode(".", $id);

      $query = $Q->query("update quarantine set hidden=1 where id='" . $Q->escape($a[1]) . "'");

      return 0;
   }


   public function RemoveAllEntries() {

      $Q = Registry::get('Q');

      $query = $Q->query("update quarantine set hidden=1");

      return 0;
   }


   public function getSearchTerms() {
      $Q = Registry::get('Q');

      $query = $Q->query("select * from search order by ts desc");

      return $query->rows;
   }


   public function AddSearchTerm($from = '', $subj = '', $hamspam = '') {
      $term = "subj=$subj&from=$from&hamspam=$hamspam";

      $Q = Registry::get('Q');

      $query = $Q->query("update search set ts=" . time() . " where term='" . $Q->escape($term) . "'");
      if($Q->countAffected() == 0) {
         $query = $Q->query("insert into search (term, ts) values('" . $Q->escape($term) . "', " . time() . ")");
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
