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


   public function PopulateDatabase($dir = '') {
      if($dir == "" || !file_exists($dir)) { return 0; }

      $expr = '/^([sh]\.[0-9a-f]+)$/';

      $Q = Registry::get('Q');


      /* remove entries not present in the filesystem */

      $files = $this->model_quarantine_message->myscandir($dir, $expr, 0, 0);
      if(isset($files[0])) {
         $st = stat($dir . "/" . $files[0]);
         if(isset($st['ctime'])) {
            $query = $Q->query("delete from quarantine where ts < " . (int)$st['ctime']);
         }
      }


      /* determine what files are to be populated */

      $query = $Q->query("select max(ts) as maxts from quarantine");
      $maxts = (int)@$query->row['maxts'];

      $files = $this->model_quarantine_message->myscandir($dir, $expr, 1, $maxts);

      $query = $Q->query("BEGIN");

      while(list($k, $v) = each($files)){

         $f = $dir . "/" . $v;

         list ($mailfrom, $subject) = $this->model_quarantine_message->scan_message($dir, $v);

         $st = stat($f);

         $a = explode(".", $v);

         $query = $Q->query("insert into quarantine (id, ts, size, is_spam, hidden, `from`, subj) values('" . $a[1] . "', " . $st['ctime'] . ", " . $st['size'] . ", '" . $a[0] . "', 0, '" . $Q->escape($mailfrom) . "', '" . $Q->escape($subject) . "')");

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

      $query = $Q->query("select * from quarantine $where_cond ORDER BY ts DESC LIMIT " . $page_len*$page . "," . $page_len*($page+1));

      $i = $page_len*$page;

      foreach ($query->rows as $message){
         $i++;

         if(strlen($message['from']) > 6+MAX_CGI_FROM_SUBJ_LEN){ $message['from'] = substr($message['from'], 0, MAX_CGI_FROM_SUBJ_LEN) . "..."; }
         if(strlen($message['subj']) > 6+MAX_CGI_FROM_SUBJ_LEN){ $message['subj'] = substr($message['subj'], 0, MAX_CGI_FROM_SUBJ_LEN) . "..."; }

         $messages[] = array(
                             'i' => $i,
                             'id' => $message['is_spam'] . '.' . $message['id'],
                             'from' => $message['from'],
                             'subject' => $message['subj'],
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

}

?>
