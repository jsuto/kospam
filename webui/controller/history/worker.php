<?php

class ControllerHistoryWorker extends Controller {

   public function index(){

      $this->id = "content";
      $this->template = "history/worker.tpl";
      $this->layout = "common/layout-empty";

      $request = Registry::get('request');
      $language = Registry::get('language');

      $this->document->title = $language->get('text_history');

      date_default_timezone_set(TIMEZONE);

      $this->data['page'] = 0;
      $this->data['page_len'] = getPageLength();

      $this->data['total'] = 0;

      /* get search term if there's any */

      if($this->request->server['REQUEST_METHOD'] == 'POST'){
         $this->data['search'] = @$this->request->post['search'];
      }
      else {
         $this->data['search'] = @$this->request->get['search'];
      }


      $db = Registry::get('db_history');


      /* assemble filter restrictions */

      $CLAPF_FILTER = "";
      $QMGR_TABLE = "";
      $SMTP_TABLE = "";

      $this->data['hamspam'] = "";
      $this->data['sender_domain'] = "";
      $this->data['rcpt_domain'] = "";


      /* ham or spam */

      $cookie = @$this->request->cookie['hamspam'];

      if($cookie == "HAM" || $cookie == "SPAM"){
         $this->data['hamspam'] = $cookie;
         $CLAPF_FILTER = "clapf.result='$cookie'";
      }


      /* rcpt domain */

      $cookie = @$this->request->cookie['rcpt_domain'];

      if($cookie && preg_match('/^[a-z0-9-]+(\.[a-z0-9-]+)*(\.[a-z]{2,5})$/', $cookie)) {
         $this->data['rcpt_domain'] = $cookie;
         $CLAPF_FILTER ? $CLAPF_FILTER .= " and clapf.queue_id2=smtp.queue_id and (smtp.to_domain='" . $db->escape($cookie) . "' or smtp.orig_to_domain='" . $db->escape($cookie) . "')" : $CLAPF_FILTER .= " clapf.queue_id2=smtp.queue_id and (smtp.to_domain='" . $db->escape($cookie) . "' or smtp.orig_to_domain='" . $db->escape($cookie) . "')";

         $SMTP_TABLE = ", smtp";
      }


      /* sender domain */

      $cookie = @$this->request->cookie['sender_domain'];

      if($cookie && preg_match('/^[a-z0-9-]+(\.[a-z0-9-]+)*(\.[a-z]{2,5})$/', $cookie)) {
         $this->data['sender_domain'] = $cookie;
         $CLAPF_FILTER ? $CLAPF_FILTER .= " and clapf.queue_id2=qmgr.queue_id and qmgr.from_domain='" . $db->escape($cookie) . "'" : $CLAPF_FILTER .= "clapf.queue_id2=qmgr.queue_id and qmgr.from_domain='" . $db->escape($cookie) . "'";

         $QMGR_TABLE = ", qmgr ";
      }


      /* assemble clapf filter */

      if($CLAPF_FILTER) { $CLAPF_FILTER = "where " . $CLAPF_FILTER; }



      /* get page */

      if(isset($this->request->get['page']) && is_numeric($this->request->get['page']) && $this->request->get['page'] > 0) {
         $this->data['page'] = $this->request->get['page'];
      }


      $this->data['entries'] = array();

      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {

         $last_update = 0;

         $i = 0;

         $query = $db->query("select count(*) as total from clapf $QMGR_TABLE $SMTP_TABLE $CLAPF_FILTER");
         $this->data['total'] = $query->row['total'];

         $query = $db->query("select clapf.queue_id, clapf.result, clapf.spaminess, clapf.relay, clapf.delay, clapf.queue_id2, clapf.virus from clapf $QMGR_TABLE $SMTP_TABLE $CLAPF_FILTER order by clapf.ts desc limit " . (int)$this->data['page'] * (int)$this->data['page_len'] . ", " . $this->data['page_len']);

         foreach ($query->rows as $__clapf) {

            // smtp/local/virtual records after content filter
            $__smtp = $db->query("select * from smtp where queue_id='" . $db->escape($__clapf['queue_id2']) . "' order by ts desc");

            if($i == 0) { $last_update = $__smtp->row['ts']; }

            // what we had before the content filter
            $__smtp2 = $db->query("select * from smtp where clapf_id='" . $db->escape($__clapf['queue_id']) . "'");
            $__qmgr = $db->query("select * from qmgr where queue_id='" . $db->escape($__smtp2->row['queue_id']) . "'");
            $__cleanup = $db->query("select message_id from cleanup where queue_id='" . $db->escape($__smtp2->row['queue_id']) . "'");
            $__smtpd = $db->query("select client_ip from smtpd where queue_id='" . $db->escape($__smtp2->row['queue_id']) . "'");

            $i++;

            /* fix null sender (<>) */
            if(strlen($__qmgr->row['from']) == 0) { $__qmgr->row['from'] = "&lt;&gt;"; }


            /* if there's no smtp record after clapf (ie. a dropped VIRUS), then fake an smtp entry */

            if(!isset($__smtp->row['to'])) {
               $__smtp->rows[] = array(
                                      'to'     => $__smtp2->row['to'],
                                      'result' => 'dropped',
                                      'relay'  => 'Virus: ' . $__clapf['virus'],
                                      'ts'     => $__smtp2->row['ts'],
                                      
               );
            }

            foreach ($__smtp->rows as $smtp) {
               $x = explode(" ", $smtp['result']);
               $status = array_shift($x);

               $status_the_rest = join(" ", $x);

               if(preg_match("/\[/", $smtp['relay'])) {
                  $status_the_rest = $smtp['relay'] . "<br/>" . $status_the_rest . "<br/>" . date("Y.m.d. H:i:s", $smtp['ts']);
               }
               else {
                  $status_the_rest = $smtp['relay'] . ", " . $status_the_rest . "<br/>" . date("Y.m.d. H:i:s", $smtp['ts']);
               }

               $status_the_rest = preg_replace('/"/', '&#96;', $status_the_rest);
               $status_the_rest = preg_replace("/'/", '&#101;', $status_the_rest);

               if(isset($smtp['orig_to']) && strlen($smtp['orig_to']) > 3) { $smtp['to'] = $smtp['orig_to']; }

               if($this->data['rcpt_domain'] && !preg_match("/@" . $this->data['rcpt_domain'] . "$/", $smtp['to']) ) { continue; }

               $this->data['entries'][] = array(
                                               'timedate'       => date("Y.m.d. H:i:s", $__smtp2->row['ts']),
                                               'client'         => @$__smtpd->row['client_ip'],
                                               'queue_id1'      => $__qmgr->row['queue_id'],
                                               'message_id'     => $__cleanup->row['message_id'],
                                               'shortfrom'      => strlen($__qmgr->row['from']) > FROM_LENGTH_TO_SHOW ? substr($__qmgr->row['from'], 0, FROM_LENGTH_TO_SHOW) . "..." : $__qmgr->row['from'],
                                               'from'           => $__qmgr->row['from'],
                                               'shortto'        => strlen($smtp['to']) > FROM_LENGTH_TO_SHOW ? substr($smtp['to'], 0, FROM_LENGTH_TO_SHOW) . "..." : $smtp['to'],
                                               'to'             => $smtp['to'],
                                               'size'           => $__qmgr->row['size'],
                                               'content_filter' => $__smtp2->row['relay'],
                                               'relay'          => $__clapf['relay'],
                                               'clapf_id'       => $__clapf['queue_id'],
                                               'spaminess'      => $__clapf['spaminess'],
                                               'delay'          => $__clapf['delay'],
                                               'result'         => $__clapf['result'],
                                               'status'         => $status,
                                               'status_balloon' => $status_the_rest
                                        );

            }

         }

         setcookie("lastupdate", $last_update, time()+3600);

         $this->data['total_pages'] = floor($this->data['total'] / $this->data['page_len']);

         $this->data['prev_page'] = $this->data['page'] - 1;
         $this->data['next_page'] = $this->data['page'] + 1;


         $this->render();

      }
      else {
         die("go away");
      }


   }

}


?>
