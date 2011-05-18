<?php

class ControllerHistoryView extends Controller {

   public function index(){

      $this->id = "content";
      $this->template = "history/view.tpl";
      $this->layout = "common/layout";

      $request = Registry::get('request');
      $language = Registry::get('language');

      $this->load->model('user/user');
      $this->load->model('quarantine/message');


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



      $this->data['entries'] = array();

      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {

         //if(Registry::get('DB_DRIVER') == 'mysql'){ $db->select_db(Registry::get('DB_DATABASE')); }

         $db = Registry::get('db');
         $username = $this->model_user_user->getUserByEmail(@$this->request->get['to']);

         $db_history = Registry::get('db_history');
         $db_history->select_db($db_history->database);


         $query = $db_history->query("select * from clapf where clapf_id='" . $db_history->escape(@$this->request->get['id']) . "'");

         foreach ($query->rows as $__clapf) {

            // smtp/local/virtual records after content filter
            $__smtp = $db_history->query("select * from smtp where queue_id='" . $db_history->escape($__clapf['queue_id2']) . "' order by ts desc");
            $__cleanup = $db_history->query("select message_id from cleanup where queue_id='" . $db->escape($__clapf['queue_id2']) . "'");

            // what we had before the content filter
            $__smtp2 = $db_history->query("select * from smtp where clapf_id='" . $db_history->escape($__clapf['clapf_id']) . "'");
            if(isset($__smtp2->row['queue_id'])) {
               $__smtpd = $db_history->query("select client_ip from smtpd where queue_id='" . $db->escape($__smtp2->row['queue_id']) . "'");
            }

            /* fix null sender (<>) */
            if(strlen($__clapf['from']) == 0) { $__clapf['from'] = "&lt;&gt;"; }


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
