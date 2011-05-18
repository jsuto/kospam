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

      $this->load->model('quarantine/message');

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

      if($cookie && (preg_match('/^[a-z0-9-]+(\.[a-z0-9-]+)*(\.[a-z]{2,5})$/', $cookie) || validemail($cookie) == 1) ) {
         $this->data['rcpt_domain'] = $cookie;

         $to = "rcptdomain";

         if(strchr($this->data['rcpt_domain'], '@')) { $to = "rcpt"; }

         $CLAPF_FILTER ? $CLAPF_FILTER .= " and clapf.`$to`='" . $db->escape($cookie) . "'" : $CLAPF_FILTER .= " clapf.`$to`='" . $db->escape($cookie) . "'";
      }


      /* sender domain */

      $cookie = @$this->request->cookie['sender_domain'];

      if($cookie && (preg_match('/^[a-z0-9-]+(\.[a-z0-9-]+)*(\.[a-z]{2,5})$/', $cookie) || validemail($cookie) == 1) ) {
         $this->data['sender_domain'] = $cookie;

         $from = "fromdomain";

         if(strchr($this->data['sender_domain'], '@')) { $from = "from"; }

         $CLAPF_FILTER ? $CLAPF_FILTER .= " and clapf.`$from`='" . $db->escape($cookie) . "'" : $CLAPF_FILTER .= "clapf.`$from`='" . $db->escape($cookie) . "'";
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

         $query = $db->query("select count(*) as total from clapf $CLAPF_FILTER");

         $this->data['total'] = $query->row['total'];

         $query = $db->query("select * from clapf $CLAPF_FILTER order by ts desc limit " . (int)$this->data['page'] * (int)$this->data['page_len'] . ", " . $this->data['page_len']);

         foreach ($query->rows as $__clapf) {

            // smtp/local/virtual records after content filter
            $__smtp = $db->query("select * from smtp where queue_id='" . $db->escape($__clapf['queue_id2']) . "' order by ts desc");
            $__cleanup = $db->query("select message_id from cleanup where queue_id='" . $db->escape($__clapf['queue_id2']) . "'");

            if($i == 0 && isset($__smtp->row['ts'])) { $last_update = $__smtp->row['ts']; }

            // what we had before the content filter
            $__smtp2 = $db->query("select * from smtp where clapf_id='" . $db->escape($__clapf['clapf_id']) . "'");
            if(isset($__smtp2->row['queue_id'])) {
               $__smtpd = $db->query("select client_ip from smtpd where queue_id='" . $db->escape($__smtp2->row['queue_id']) . "'");
            }

            $i++;

            /* fix null sender (<>) */
            if(strlen($__clapf['from']) == 0) { $__clapf['from'] = "&lt;&gt;"; }


            /* if there's no smtp record after clapf (ie. a dropped VIRUS), then fake an smtp entry */

            if(!isset($__smtp->row['to'])) {
               $__smtp->rows[] = array(
                                      'to'     => $__clapf['rcpt'],
                                      'result' => 'dropped',
                                      'relay'  => $__clapf['virus'] ? 'Virus: ' . $__clapf['virus'] : '',
                                      'ts'     => $__clapf['ts'],
                                      
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

               if($this->data['rcpt_domain'] && !preg_match("/@" . $this->data['rcpt_domain'] . "$/", $smtp['to']) && $this->data['rcpt_domain'] != $smtp['to']) { continue; }

               $this->data['entries'][] = array(
                                               'timedate'       => date("Y.m.d. H:i:s", $__clapf['ts']),
                                               'client'         => @$__smtpd->row['client_ip'],
                                               'queue_id1'      => isset($__smtp2->row['queue_id']) ? $__smtp2->row['queue_id'] : 'NOQUEUE',
                                               'message_id'     => isset($__cleanup->row['message_id']) ? $__cleanup->row['message_id'] : 'N/A',
                                               'shortfrom'      => strlen($__clapf['from']) > FROM_LENGTH_TO_SHOW ? substr($__clapf['from'], 0, FROM_LENGTH_TO_SHOW) . "..." : $__clapf['from'],
                                               'from'           => $__clapf['from'],
                                               'shortto'        => strlen($smtp['to']) > FROM_LENGTH_TO_SHOW ? substr($smtp['to'], 0, FROM_LENGTH_TO_SHOW) . "..." : $smtp['to'],
                                               'to'             => $smtp['to'],
                                               'size'           => $this->model_quarantine_message->NiceSize($__clapf['size']),
                                               'content_filter' => isset($__smtp2->row['relay']) ? $__smtp2->row['relay'] : 'none',
                                               'relay'          => $__clapf['relay'] ? $__clapf['relay'] : 'N/A',
                                               'clapf_id'       => $__clapf['clapf_id'],
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
