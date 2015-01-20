<?php

class ModelSearchSearch extends Model {

   public function search_messages($data = array(), $page = 0) {
      $one_page_of_ids = array();
      $total_hits = 0;
      $total_found = 0;
      $sort = "ts";
      $order = "DESC";
      $sortorder = "ORDER BY ts DESC";
      $cache_key = "";
      $q = "";
      $s = "";
      $all_ids_csv = "";

      $session = Registry::get('session');

      while(list($k,$v) = each($data)) {
         if($v) { if(is_array($v)) { $v = implode(" ", $v); } $s .= '&' . $k . '=' . $v; }
      }

      if($s) { $s = substr($s, 1, strlen($s)); }

      AUDIT(ACTION_SEARCH, '', '', 0, $s);


      if($data['sort'] == "ts") { $sort = "ts"; }
      if($data['sort'] == "size") { $sort = "size"; }
      if($data['sort'] == "from") { $sort = "from"; }
      if($data['sort'] == "subj") { $sort = "subject"; }

      if($data['order'] == 1) { $order = "ASC"; }

      $sortorder = "ORDER BY `$sort` $order";

      $m = array();

      if(MEMCACHED_ENABLED) {
         $cache_key = $this->make_cache_file_name($data, $sortorder);
         $memcache = Registry::get('memcache');
         $m = $memcache->get($cache_key);
      }


      if(isset($m['ids'])) {
         $all_ids = $m['ids'];
         $total_found = $m['total_found'];
      } else {
         list ($total_found, $all_ids) = $this->query_all_possible_IDs($data, $sort, $order, $sortorder, $cache_key);
      }


      $total_hits = count($all_ids);


      if($total_hits > 0) {
         $session->set('last_search', serialize($all_ids));
      } else {
         $session->set('last_search', '');
      }

      $data['page_len'] = get_page_length();

      if($total_hits > 0) {
         $i = 0;

         foreach($all_ids as $id) {

            if($i >= $data['page_len'] * $page && $i < $data['page_len'] * ($page+1) ) {
               array_push($one_page_of_ids, $id);
               $all_ids_csv .= ",$id";

               if($q) { $q .= ",?"; } else { $q = "?"; }
            }

            $i++;
         }

      }

      if(isset($data['history'])) {
         $one_page_of_ids = $all_ids;
         $q = str_repeat("?,", count($all_ids)) . "-1";
      }

      $all_ids_csv = substr($all_ids_csv, 1, strlen($all_ids_csv));

      return array(
                   'total_hits'  => $total_hits,
                   'total_found' => $total_found,
                   'all_ids'     => $all_ids_csv,
                   'messages'    => $this->get_meta_data($one_page_of_ids, $q, $sortorder)
                  );
   }


   private function assemble_email_address_filter() {
      $session = Registry::get('session');

      if(Registry::get('admin_user') == 1) {
         return "";
      }

      $all_your_addresses = $this->get_all_your_address();
      return " (@sender $all_your_addresses | @rcpt $all_your_addresses) ";
   }


   private function query_all_possible_IDs($data = array(), $sort = 'ts', $order = 'DESC', $sortorder = '', $cache_key = '') {
      $ids = array();
      $match = '';
      $spam = $attachment = $size = '';
      $tag_id_list = '';
      $a = "";
      $id = "";
      $fields = array("@(subject,body)", "@sender", "@rcpt", "@subject", "@body", "@attachment_types", "@status");
      $sph_index = SPHINX_MAIN_INDEX;

      $emailfilter = $this->assemble_email_address_filter();

      $session = Registry::get('session');


      $i = 0;
      while(list($k, $v) = each($data['match'])) {
         if($v == "@attachment_types") {
            list($k, $v) = each($data['match']);
            $i++;
            if($v == "any") {
               $data['match'][$i-1] = "";
               $data['match'][$i] = "";
            }

            if($a == '') {
               $a = "attachments > 0 AND ";
            }
         }

         if(substr($v, 0, 7) == "http://") { $v = preg_replace("/\./", "X", $v); $data['match'][$i] = preg_replace("/http\:\/\//", "__URL__", $v); }

         if(!in_array($v, $fields) && $i > 0 && strchr($v, "@")) {

            //$data['match'][$i] = $this->fix_email_address_for_sphinx($v);
            if(strchr($v, '@')) { $data['match'][$i] = '"' . $v . '"'; }
         }
         $i++; 
      }

      $match = implode(" ", $data['match']);

      if($emailfilter) {
         if(strlen($match) > 2) { $match = "( $match ) & $emailfilter"; }
         else { $match = $emailfilter; }
      }


      if($match == " ") { $match = ""; }

      if($data['sort'] == 'from' || $data['sort'] == 'subj') { $sortorder = ''; }

      $date = fixup_date_condition('sent', $data['date1'], $data['date2']);

      if($date) { $date .= " AND "; }

      if(isset($data['size']) && $data['size']) {
         $data['size'] = preg_replace("/\s/", "", $data['size']);
         if(preg_match("/^(\>|\<)\={0,}\d{1,}$/", $data['size'])) { $size = "size " . $data['size'] . " AND "; }
      }

      if(isset($data['attachment_type']) && strstr($data['attachment_type'], 'any')) { $a = "attachments > 0 AND "; }
      else if(isset($data['has_attachment']) && $data['has_attachment'] == 1) { $attachment = "attachments > 0 AND "; }


      if(isset($data['id']) && $data['id']) {
         $data['id'] = preg_replace("/ /", "," , substr($data['id'], 1, strlen($data['id'])));
         $id = " id IN (" . $data['id'] . ") AND ";
      }

      if(isset($data['history'])) {
         $sph_index = SPHINX_HISTORY_INDEX;
         if(isset($data['spam']) && $data['spam'] != '') { $spam = " spam= " . $data['spam'] . " AND "; }
      } else if(SPAM_ONLY_QUARANTINE == 1) {
         $spam = " spam=1 AND ";
      }


      $query = $this->sphx->query("SELECT id FROM " . $sph_index . " WHERE $a $id $date $attachment $spam $size MATCH('$match') $sortorder LIMIT 0," . MAX_SEARCH_HITS . " OPTION max_matches=" . MAX_SEARCH_HITS);

      $total_found = $query->total_found;

      if(ENABLE_SYSLOG == 1) { syslog(LOG_INFO, sprintf("sphinx query: '%s' in %.2f s, %d hits, %d total found", $query->query, $query->exec_time, $query->num_rows, $total_found)); }


      /*
       * build an id list
       */

      $q = "";

      if(isset($query->rows)) {
         foreach($query->rows as $a) {
            array_push($ids, $a['id']);

            if($q) { $q .= ",?"; }
            else { $q = "?"; }
         }
      }


      /*
       * if the query was requested to be sorted by sender or subject, then sphinx cannot do
       * that, so we assemble the list of all sphinx IDs matching the query
       */

      if($data['sort'] == 'from' || $data['sort'] == 'subj') {

         $fs_query = $this->db->query("SELECT id FROM " . TABLE_HISTORY . " WHERE id IN ($q) ORDER BY `$sort` $order", $ids);

         $ids = array();

         foreach($fs_query->rows as $q) {
            array_push($ids, $q['id']);
         }

      }


      if(MEMCACHED_ENABLED && $cache_key) {
         $memcache = Registry::get('memcache');
         $memcache->add($cache_key, array('ts' => time(), 'total_hits' => count($ids), 'ids' => $ids, 'total_found' => $total_found), 0, MEMCACHED_TTL);
      }

      return array($total_found, $ids);
   }


   private function query_all_possible_IDs_by_reference($reference = '', $cache_key = '') {
   }


   public function preprocess_post_expert_request($data = array()) {
      $token = 'match';
      $ndate = 0;
      $match = array();

      $a = array(
                    'date1'           => '',
                    'date2'           => '',
                    'size'            => '',
                    'attachment_type' => '',
                    'tag'             => '',
                    'note'            => '',
                    'id'              => '',
                    'match'           => array()
      );

      if(!isset($data['search'])) { return $a; }

      $s = preg_replace("/https{0,1}:/", "httpX", $data['search']);
      $s = preg_replace("/:/", ": ", $s);
      $s = preg_replace("/,/", " ", $s);
      $s = preg_replace("/\(/", "( ", $s);
      $s = preg_replace("/\)/", ") ", $s);
      $s = preg_replace("/OR/", "|", $s);
      $s = preg_replace("/AND/", "", $s);
      $s = preg_replace("/\s{1,}/", " ", $s);
      $s = preg_replace("/httpX/", "http:", $s);
      $b = explode(" ", $s);

      while(list($k, $v) = each($b)) {
         if($v == '') { continue; }

         if($v == 'from:') { $token = 'match'; $a['match'][] = '@sender'; continue; }
         else if($v == 'to:') { $token = 'match'; $a['match'][] = '@rcpt'; continue; }
         else if($v == 'subject:') { $token = 'match'; $a['match'][] = '@subject'; continue; }
         else if($v == 'body:') { $token = 'match'; $a['match'][] = '@body'; continue; }
         else if($v == 'size:') { $token = 'size'; continue; }
         else if($v == 'date1:') { $token = 'date1'; continue; }
         else if($v == 'date2:') { $token = 'date2'; continue; }
         else if($v == 'attachment:' || $v == 'a:') { $token = 'match'; $a['match'][] = '@attachment_types'; continue; }
         else if($v == 'size') { $token = 'size'; continue; }
         else if($v == 'tag:') { $token = 'tag'; continue; }
         else if($v == 'note:') { $token = 'note'; continue; }
         else if($v == 'ref:') { $token = 'ref'; continue; }
         else if($v == 'spam:') { $token = 'spam'; continue; }
         else if($v == 'status:') { $token = 'match'; $a['match'][] = '@status'; continue; }
         else if($v == 'id:') { $token = 'id'; continue; }
         else {
            if(preg_match("/\d{4}\-\d{1,2}\-\d{1,2}/", $v) || preg_match("/\d{1,2}\/\d{1,2}\/\d{4}/", $v)) {
               $ndate++;
               $a["date$ndate"] = $v;
            }
         }

         if($token == 'match') { $a['match'][] = $v; }
         else if($token == 'date1') { $a['date1'] = ' ' . $v; }
         else if($token == 'date2') { $a['date2'] = ' ' . $v; }
         else if($token == 'tag') { $a['tag'] .= ' ' . $v; }
         else if($token == 'note') { $a['note'] .= ' ' . $v; }
         else if($token == 'ref') { $a['ref'] = ' ' . $v; }
         else if($token == 'spam') { $a['spam'] = ' ' . $v; }
         else if($token == 'id') { $a['id'] .= ' ' . $v; }

         else if($token == 'size') {
            $o = substr($v, 0, 1);
            if($o == '<' || $o == '>') {
               $v = substr($v, 1, strlen($v));
               $o1 = substr($v, 0, 1);
               if($o1 == '=') {
                  $v = substr($v, 1, strlen($v));
                  $o .= $o1;
               }
            }
            else { $o = ''; }

            $s = explode("k", $v);
            if($s[0] != $v) { $v = $s[0] * 1000; }

            $s = explode("M", $v);
            if($s[0] != $v) { $v = $s[0] * 1000000; }

            $a['size'] .= ' ' . $o . $v;
         }

      }

      $a['sort'] = $data['sort'];
      $a['order'] = $data['order'];

      return $a;
   }


   private function get_meta_data($ids = array(), $q = '', $sortorder = '') {
      $messages = array();
      $rcpt = $srcpt = array();
      $tag = array();
      $note = array();

      if(count($ids) == 0) return $messages;

      if(MEMCACHED_ENABLED) {
         $cache_key = $this->make_cache_file_name($ids, 'meta');
         $memcache = Registry::get('memcache');
         $m = $memcache->get($cache_key);
         if(isset($m['meta'])) { return unserialize($m['meta']); }
      }

      $session = Registry::get('session');

      $query = $this->db->query("SELECT `id`, `sender`, `rcpt`, `subject`, `clapf_id`, `size`, `spam`, `ts`, `attachments`, `hidden`, `relay`, `status` FROM `" . TABLE_HISTORY . "` WHERE `id` IN ($q) $sortorder", $ids);

      if(isset($query->rows)) {

         array_unshift($ids, (int)$session->get("uid"));

         $lang = Registry::get('language');


         foreach($query->rows as $m) {
            $m['shortfrom'] = make_short_string($m['sender'], MAX_CGI_FROM_SUBJ_LEN);
            $m['from'] = escape_gt_lt_quote_symbols($m['sender']);

            $m['shortto'] = make_short_string($m['rcpt'], MAX_CGI_FROM_SUBJ_LEN);
            $m['to'] = escape_gt_lt_quote_symbols($m['rcpt']);


            if($m['subject'] == "") { $m['subject'] = "&lt;" . $lang->data['text_no_subject'] . "&gt;"; }
            $m['shortsubject'] = make_short_string($m['subject'], MAX_CGI_FROM_SUBJ_LEN);

            $m['subject'] = escape_gt_lt_quote_symbols($m['subject']);
            $m['shortsubject'] = make_short_string($m['subject'], MAX_CGI_FROM_SUBJ_LEN);

            $m['date'] = date(DATE_TEMPLATE, $m['ts']);
            $m['size'] = nice_size($m['size']);

            in_array($m['from'], $session->get("emails")) ? $m['yousent'] = 1 : $m['yousent'] = 0;

            /*
             * verifying 20 messages takes some time, still it's useful
             */

            if(isset($tag[$m['id']])) { $m['tag'] = $tag[$m['id']]; } else { $m['tag'] = ''; }
            if(isset($note[$m['id']])) { $m['note'] = $note[$m['id']]; } else { $m['note'] = ''; }

            $m['note'] = preg_replace("/\"/", "*", strip_tags($m['note']));
            $m['tag'] = preg_replace("/\"/", "*", strip_tags($m['tag']));

            array_push($messages, $m);
         }

      }

      if(MEMCACHED_ENABLED) {
         $memcache->add($cache_key, array('meta' => serialize($messages)), 0, MEMCACHED_TTL);
      }

      return $messages;
   }


   public function store_search_result_to_blob($query = array(), $data = array()) {
      $email = $this->session->get("email");

      if($email == '' || count($data) == 0) { return 0; }

      $cksum = $this->create_search_session_key($query);

      $expiry = NOW + 300;
      $s = zlib_encode(serialize($data), ZLIB_ENCODING_GZIP);

      $query = $this->db->query("INSERT INTO " . TABLE_SEARCH_CACHE . " (cksum, expiry, result) VALUES(?,?,?)", array($cksum, $expiry, $s));
   }


   public function get_search_session_data($data = array()) {
      $this->cksum = $this->create_search_session_key($data);

      $this->db->query("DELETE FROM " . TABLE_SEARCH_CACHE . " WHERE expiry < ?", array(NOW));

      $query = $this->db->query("SELECT expiry, result FROM " . TABLE_SEARCH_CACHE . " WHERE cksum=?", array($this->cksum));

      if(isset($query->row['result'])) {
         syslog(LOG_INFO, "result found in search cache (exp:" . $query->row['expiry'] . ")");
         return unserialize(zlib_decode($query->row['result']));
      }

      return array();
   }


   private function create_search_session_key($data = array()) {
      if($this->session->get("email") == '') { return 'null'; }

      $data['page'] = 0;

      return sha1($this->session->get("email") . serialize($data));
   }


   public function search_result_to_csv($cksum = '') {
      $email = $this->session->get("email");

      print "Date" . DELIMITER . "ID" . DELIMITER . "Sender" . DELIMITER . "Recipient" . DELIMITER . "Subject" . DELIMITER . "Size" . DELIMITER . "Spam" . DELIMITER . "Relay" . DELIMITER . "Status\n";

      $query = $this->db->query("SELECT result FROM " . TABLE_SEARCH_CACHE . " WHERE cksum=?", array($cksum));

      if(isset($query->row['result'])) {
         $data = unserialize(zlib_decode($query->row['result']));

         if(count($data['messages']) > 0) {
            foreach($data['messages'] as $d) {
               print $d['date'] . DELIMITER . $d['clapf_id'] . DELIMITER . $d['sender'] . DELIMITER . $d['rcpt'] . DELIMITER . $d['subject'] . DELIMITER . $d['size'] . DELIMITER . $d['spam'] . DELIMITER . $d['relay'] . DELIMITER . $d['status'] . "\n";
            } 
         }
      }
   }


   public function get_message_recipients($id = '') {
      $rcpt = array();
      $domains = array();

      if(Registry::get('admin_user') == 0) { return $rcpt; }

      $query = $this->db->query("SELECT `domain` FROM " . TABLE_DOMAIN);
      foreach($query->rows as $q) {
         array_push($domains, $q['domain']);
      }

      $query = $this->db->query("SELECT `to` FROM " . VIEW_MESSAGES . " WHERE id=?", array($id));

      foreach($query->rows as $q) {
         $mydomain = 0;

         foreach ($domains as $domain) {
            if(preg_match("/\@$domain$/", $q['to'])) { $mydomain = 1; break; }
         }

         if($mydomain == 1) {
            array_push($rcpt, $q['to']);
         }
      }

      return $rcpt;
   }


   public function get_message_addresses_in_my_domain($id = '') {
      $addr = array();
      $domains = array();

      if(Registry::get('admin_user') == 0) { return $addr; }

      $query = $this->db->query("SELECT `domain` FROM " . TABLE_DOMAIN);
      foreach($query->rows as $q) {
         array_push($domains, $q['domain']);
      }

      $query = $this->db->query("SELECT `from`, `to` FROM " . VIEW_MESSAGES . " WHERE id=?", array($id));

      if(isset($query->row['from'])) {
         foreach ($domains as $domain) {
            if(preg_match("/\@$domain$/", $query->row['from'])) { array_push($addr, $query->row['from']); }
         }
      }

      foreach($query->rows as $q) {
         $mydomain = 0;

         foreach ($domains as $domain) {
            if(preg_match("/\@$domain$/", $q['to'])) { $mydomain = 1; break; }
         }

         if($mydomain == 1) {
            if(!in_array($q['to'], $addr)) { array_push($addr, $q['to']); }
         }
      }

      return $addr;

   }


   private function get_all_your_address() {
      $s = '';

      $session = Registry::get('session');

      $emails = $session->get("emails");

      while(list($k, $v) = each($emails)) {
         if($s) { $s .= '| ' .  $this->fix_email_address_for_sphinx($v); }
         else { $s = $this->fix_email_address_for_sphinx($v); }
      }

      return $s;
   }


   public function check_your_permission_by_id($id = '') {
      $q = '';
      $arr = $a = array();

      if($id == '') { return 0; }

      if(Registry::get('admin_user') == 1) { return 1; }

      $session = Registry::get('session');

      array_push($arr, $id);

      $emails = $session->get("emails");

      while(list($k, $v) = each($emails)) {
         if(validemail($v) == 1) {
            $q .= ",?";
            array_push($a, $v);
         }
      }

      $q = preg_replace("/^\,/", "", $q);

      $arr = array_merge($arr, $a);

      $query = $this->db->query("SELECT id FROM " . TABLE_HISTORY . " WHERE id=? AND `rcpt` IN ($q)", $arr);

      if(isset($query->row['id'])) { return 1; }

      return 0;
   }


   public function check_your_permission_by_id_list($id = array()) {
      $q = $q2 = '';
      $arr = $a = $result = array();

      if(count($id) < 1) { return $result; }

      $session = Registry::get('session');

      $arr = $id;

      for($i=0; $i<count($id); $i++) {
         $q2 .= ",?";
      }


      $q2 = preg_replace("/^\,/", "", $q2);

         if(Registry::get('admin_user') == 0) {
            $emails = $session->get("emails");

            while(list($k, $v) = each($emails)) {
               if(validemail($v) == 1) {
                  $q .= ",?";
                  array_push($a, $v);
               }
            }
         }

      $q = preg_replace("/^\,/", "", $q);


      if(Registry::get('admin_user') == 1) {
         $query = $this->db->query("SELECT id FROM `" . TABLE_HISTORY . "` WHERE `id` IN ($q2)", $arr);
      }
      else {
         $arr = array_merge($arr, $a, $a);
         $query = $this->db->query("SELECT id FROM `" . TABLE_HISTORY . "` WHERE `id` IN ($q2) AND ( `sender` IN ($q) OR `rcpt` IN ($q) )", $arr);
      }

      if($query->num_rows > 0) {
         foreach ($query->rows as $q) {
            array_push($result, $q['id']);
         }
      }

      return $result;
   }


   private function fix_email_address_for_sphinx($email = '') {
      //$email = preg_replace("/\|@/", "|", $email);
      //return preg_replace("/[\@\.\+\-\_]/", "X", $email);
      return '"' . $email . '"';
   }


   public function hide_message($id = 0) {
      $query = $this->db->query("UPDATE " . TABLE_HISTORY . " SET hidden=1 WHERE id=?", array($id));
   }


   public function get_search_terms() {
      $session = Registry::get('session');

      $query = $this->db->query("SELECT term, ts FROM " . TABLE_SEARCH . " WHERE email=? ORDER BY ts DESC", array($session->get("email")));
      if(isset($query->rows)) { return $query->rows; }

      return array();
   }


   public function add_search_term($term = '') {
      if($term == '') { return 0; }

      $session = Registry::get('session');

      parse_str($term, $s);
      if(!isset($s['search']) || $s['search'] == '') { return 0; }

      if($this->update_search_term($term) == 0) {
         AUDIT(ACTION_SAVE_SEARCH, '', '', '', $term);
         $query = $this->db->query("INSERT INTO " . TABLE_SEARCH . " (email, ts, term) VALUES(?,?,?)", array($session->get("email"), time(), $term));
      }

      return 1;
   }


   public function update_search_term($term = '') {
      if($term == '') { return 0; }

      AUDIT(ACTION_SEARCH, '', '', '', $term);

      $session = Registry::get('session');

      $query = $this->db->query("UPDATE " . TABLE_SEARCH . " SET ts=? WHERE term=? AND email=?", array(time(), $term, $session->get("email")));

      return $this->db->countAffected();
   }


   public function remove_search_term($ts = 0) {
      $session = Registry::get('session');

      $query = $this->db->query("DELETE FROM " . TABLE_SEARCH . " WHERE email=? AND ts=?", array($session->get("email"), $ts));
   }


   private function fixup_meta_characters($s = '') {
      if($s == '') { return $s; }

      $s = preg_replace("/\'/", ' ', $s);
      $s = preg_replace("/\./", ' ', $s);

      return $s;
   }


   private function fixup_sphinx_operators($s = '') {
      if($s == '') { return $s; }

      $s = preg_replace("/ OR /", "|", $s);
      $s = preg_replace("/(\-)/", " ", $s);
      $s = preg_replace("/\'/", '"', $s);
      $a = explode(" ", $s);
      $s = '';

      while(list($k, $v) = each($a)) {

         if(substr($v, 0, 4) == 'http') {
            $v = preg_replace("/http(s){0,1}\:\/\//", "__URL__", $v);
            $b = explode("/", $v);
            $s .= ' ' . $this->fix_email_address_for_sphinx($b[0]);
         }
         else {
            $s .= ' ' . $v;
         }
      }

      return $s;
   }


   private function make_cache_file_name($data = array(), $sortorder = '') {
      $s = '';
      $session = Registry::get('session');

      while(list($k, $v) = each($data)) {
         if($v) {
            if(is_array($v)) { $v = join("*", $v); }
            $s .= "*$k=$v";
         }
      }

      return sha1($session->get("email") . "/" . $s . "-" . (NOW - NOW % 3600) . "-" . $sortorder);
   }

}


?>
