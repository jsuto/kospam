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


      /* get page */

      if(isset($this->request->get['page']) && is_numeric($this->request->get['page']) && $this->request->get['page'] > 0) {
         $this->data['page'] = $this->request->get['page'];
      }


      $this->data['entries'] = array();

      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {

         $last_update = 0;

         $i = 0;

         $this->data['entry'] = array();

         $__clapf = $db_history->query("select clapf.queue_id, clapf.result, clapf.spaminess, clapf.relay, clapf.delay, clapf.queue_id2, clapf.virus from clapf, smtp where clapf.queue_id2=smtp.queue_id and clapf.queue_id='" . $db_history->escape(@$this->request->get['id']) . "' and (smtp.`to`='" . $db_history->escape(@$this->request->get['to']) . "' or smtp.orig_to='" . $db_history->escape(@$this->request->get['to']) . "')");

         // smtp/local/virtual records after content filter

         $__smtp = $db_history->query("select * from smtp where queue_id='" . $db_history->escape($__clapf->row['queue_id2']) . "' and (`to`='" . $db_history->escape(@$this->request->get['to']) . "' or orig_to='" . $db_history->escape(@$this->request->get['to']) . "') order by ts desc");

         // what we had before the content filter

         $__smtp2 = $db_history->query("select * from smtp where clapf_id='" . $db_history->escape($__clapf->row['queue_id']) . "'");
         $__qmgr = $db_history->query("select * from qmgr where queue_id='" . $db_history->escape($__smtp2->row['queue_id']) . "'");
         $__cleanup = $db_history->query("select message_id from cleanup where queue_id='" . $db_history->escape($__smtp2->row['queue_id']) . "'");
         $__smtpd = $db_history->query("select client_ip from smtpd where queue_id='" . $db_history->escape($__smtp2->row['queue_id']) . "'");


         $x = explode(" ", $__smtp->row['result']);
         $status = array_shift($x);

         $status_the_rest = join(" ", $x);

         if(preg_match("/\[/", $__smtp->row['relay'])) {
            $status_the_rest = $__smtp->row['relay'] . " " . $status_the_rest . " " . date("Y.m.d. H:i:s", $__smtp->row['ts']);
         }
         else {
            $status_the_rest = $__smtp->row['relay'] . ", " . $status_the_rest . " " . date("Y.m.d. H:i:s", $__smtp->row['ts']);
         }

         if(isset($__smtp->row['orig_to']) && strlen($__smtp->row['orig_to']) > 3) { $__smtp->row['to'] = $__smtp->row['orig_to']; }


         /* query the username */

         $username = "";

         $user = $db->query("select user.username from user, t_email where user.uid=t_email.uid and t_email.email='" . $db->escape($__smtp->row['to']) . "'");
         if($user->num_rows == 1) { $username = $user->row['username']; }

         $this->data['entry'] = array(
                                               'timedate'       => date("Y.m.d. H:i:s", $__smtp2->row['ts']),
                                               'client'         => @$__smtpd->row['client_ip'],
                                               'queue_id1'      => $__qmgr->row['queue_id'],
                                               'message_id'     => $__cleanup->row['message_id'],
                                               'shortfrom'      => strlen($__qmgr->row['from']) > FROM_LENGTH_TO_SHOW ? substr($__qmgr->row['from'], 0, FROM_LENGTH_TO_SHOW) . "..." : $__qmgr->row['from'],
                                               'from'           => $__qmgr->row['from'],
                                               'shortto'        => strlen($__smtp->row['to']) > FROM_LENGTH_TO_SHOW ? substr($__smtp->row['to'], 0, FROM_LENGTH_TO_SHOW) . "..." : $__smtp->row['to'],
                                               'to'             => $__smtp->row['to'],
                                               'size'           => $__qmgr->row['size'],
                                               'content_filter' => $__smtp2->row['relay'],
                                               'relay'          => $__clapf->row['relay'],
                                               'clapf_id'       => $__clapf->row['queue_id'],
                                               'spaminess'      => $__clapf->row['spaminess'],
                                               'delay'          => $__clapf->row['delay'],
                                               'result'         => $__clapf->row['result'],
                                               'status'         => $__smtp->row['relay'] . " " . $__smtp->row['result'],
                                               'username'       => $username
                                  );



         $this->render();

      }
      else {
         die("go away");
      }


   }

}


?>
