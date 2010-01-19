<?php

class ControllerHistoryView extends Controller {

   public function index(){

      $this->id = "content";
      $this->template = "history/view.tpl";
      $this->layout = "common/layout";

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


      $db = Registry::get('db');
      $db_history = Registry::get('db_history');


      $this->data['entries'] = array();

      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {

         $username = "";

         if(Registry::get('DB_DRIVER') == 'mysql'){ $db->select_db(Registry::get('DB_DATABASE')); }

         $user = $db->query("select user.username from user, t_email where user.uid=t_email.uid and t_email.email='" . $db->escape(@$this->request->get['to']) . "'");
         if($user->num_rows == 1) { $username = $user->row['username']; }

         if(Registry::get('HISTORY_DRIVER') == 'mysql'){ $db_history->select_db(Registry::get('HISTORY_DATABASE')); }


         $query = $db_history->query("select queue_id, result, spaminess, relay, delay, queue_id2, virus from clapf where queue_id='" . $db_history->escape(@$this->request->get['id']) . "'");

         foreach ($query->rows as $__clapf) {

            // smtp/local/virtual records after content filter
            $__smtp = $db_history->query("select * from smtp where queue_id='" . $db->escape($__clapf['queue_id2']) . "' order by ts desc");

            // what we had before the content filter
            $__smtp2 = $db_history->query("select * from smtp where clapf_id='" . $db_history->escape($__clapf['queue_id']) . "'");
            $__qmgr = $db_history->query("select * from qmgr where queue_id='" . $db_history->escape($__smtp2->row['queue_id']) . "'");
            $__cleanup = $db_history->query("select message_id from cleanup where queue_id='" . $db_history->escape($__smtp2->row['queue_id']) . "'");
            $__smtpd = $db_history->query("select client_ip from smtpd where queue_id='" . $db_history->escape($__smtp2->row['queue_id']) . "'");


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

               if($smtp['to'] != @$this->request->get['to'] && $smtp['orig_to'] != @$this->request->get['to']) { continue; }

               $x = explode(" ", $smtp['result']);
               $status = array_shift($x);

               $status_the_rest = join(" ", $x);

               if(preg_match("/\[/", $smtp['relay'])) {
                  $status_the_rest = $smtp['relay'] . "<br/>" . $status_the_rest . "<br/>" . date("Y.m.d. H:i:s", $smtp['ts']);
               }
               else {
                  $status_the_rest = $smtp['relay'] . ", " . $status_the_rest . "<br/>" . date("Y.m.d. H:i:s", $smtp['ts']);
               }

               if(isset($smtp['orig_to']) && strlen($smtp['orig_to']) > 3) { $smtp['to'] = $smtp['orig_to']; }


               $this->data['entry'] = array(
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
                                               'status'         => $smtp['relay'] . " " . $smtp['result'],
                                               'username'       => $username
                                        );

            }

         }

         $this->render();

      }
      else {
         die("go away");
      }


   }

}


?>
