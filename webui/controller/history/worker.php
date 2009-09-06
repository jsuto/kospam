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


      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {

         require_once(DIR_DATABASE . "/sqlite3.php");
         $db = new DB("sqlite3", DB_HOSTNAME, DB_USERNAME, DB_PASSWORD, HISTORY_DATA, DB_PREFIX);

         $last_update = 0;

         $i = 0;


         $query = $db->query("select queue_id, result, spaminess, relay, delay, queue_id2 from clapf order by ts desc limit " . HISTORY_ENTRIES_PER_PAGE);

         //print_r($query);

         foreach ($query->rows as $__clapf) {

            // smtp/local/virtual records after content filter
            $__smtp = $db->query("select * from smtp where queue_id='" . $db->escape($__clapf['queue_id2']) . "'");

            if($i == 0) { $last_update = $__smtp->row['ts']; }

            // what we had before the content filter
            $__smtp2 = $db->query("select * from smtp where result like '%" . $db->escape($__clapf['queue_id']) . "%'");
            $__qmgr = $db->query("select * from qmgr where queue_id='" . $db->escape($__smtp2->row['queue_id']) . "'");
            $__cleanup = $db->query("select message_id from cleanup where queue_id='" . $db->escape($__smtp2->row['queue_id']) . "'");

            $i++;

            foreach ($__smtp->rows as $smtp) {
               $x = explode(" ", $smtp['result']);
               $status = array_shift($x);

               $status_the_rest = join(" ", $x);

               if(preg_match("/\[/", $smtp['relay'])) {
                  $status_the_rest = $smtp['relay'] . "<br/>" . $status_the_rest;
               }
               else {
                  $status_the_rest = $smtp['relay'] . ", " . $status_the_rest;
               }

               $this->data['entries'][] = array(
                                               'timedate'       => date("Y.m.d. H:i:s", $__smtp->row['ts']),
                                               'queue_id1'      => $__qmgr->row['queue_id'],
                                               'message_id'     => $__cleanup->row['message_id'],
                                               'shortfrom'      => strlen($__qmgr->row['from']) > 30 ? substr($__qmgr->row['from'], 0, 30) . "..." : $__qmgr->row['from'],
                                               'from'           => $__qmgr->row['from'],
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


         $this->render();

      }
      else {
         die("go away");
      }


   }

}


?>
