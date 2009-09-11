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


      /* get page */

      if(isset($this->request->get['page']) && is_numeric($this->request->get['page']) && $this->request->get['page'] > 0) {
         $this->data['page'] = $this->request->get['page'];
      }


      $this->data['entries'] = array();

      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {

         require_once(DIR_DATABASE . "/sqlite3.php");
         $db = new DB("sqlite3", DB_HOSTNAME, DB_USERNAME, DB_PASSWORD, HISTORY_DATA, DB_PREFIX);

         $last_update = 0;

         $i = 0;

         $query = $db->query("select count(*) as total from clapf");
         $this->data['total'] = $query->row['total'];

         $query = $db->query("select queue_id, result, spaminess, relay, delay, queue_id2, virus from clapf order by ts desc limit " . (int)$this->data['page'] * (int)$this->data['page_len'] . ", " . $this->data['page_len']);


         foreach ($query->rows as $__clapf) {

            // smtp/local/virtual records after content filter
            $__smtp = $db->query("select * from smtp where queue_id='" . $db->escape($__clapf['queue_id2']) . "' order by ts desc");

            if($i == 0) { $last_update = $__smtp->row['ts']; }

            // what we had before the content filter
            $__smtp2 = $db->query("select * from smtp where result like '%" . $db->escape($__clapf['queue_id']) . "%'");
            $__qmgr = $db->query("select * from qmgr where queue_id='" . $db->escape($__smtp2->row['queue_id']) . "'");
            $__cleanup = $db->query("select message_id from cleanup where queue_id='" . $db->escape($__smtp2->row['queue_id']) . "'");
            $__smtpd = $db->query("select client_ip from smtpd where queue_id='" . $db->escape($__smtp2->row['queue_id']) . "'");

            $i++;


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
