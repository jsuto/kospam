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
      $SMTP_FILTER = "";

      $FILTER = "client != '" . $db->escape("localhost[127.0.0.1]") . "'";

      $this->data['hamspam'] = "";
      $this->data['sender_domain'] = "";
      $this->data['rcpt_domain'] = "";

      /* ham or spam */

      $cookie = @$this->request->cookie['hamspam'];

      if($cookie == "HAM" || $cookie == "SPAM"){
         $this->data['hamspam'] = $cookie;
         $CLAPF_FILTER = " and result='$cookie'";
      }


      /* dates */

      $cookie = @$this->request->cookie['date1'];
      if($cookie) {
         if(HISTORY_DRIVER == 'mysql') { $datesql = "UNIX_TIMESTAMP('" . $db->escape($cookie) . " 00:00:00') "; }
         else { $datesql = "strftime('%s', '" . $db->escape($cookie) . " 00:00:00') "; }

         $FILTER ? $FILTER .= " and ts >= $datesql" : $FILTER .= "ts >= $datesql";
      }

      $cookie = @$this->request->cookie['date2'];
      if($cookie) {
         if(HISTORY_DRIVER == 'mysql') { $datesql = "UNIX_TIMESTAMP('" . $db->escape($cookie) . " 23:59:59') "; }
         else { $datesql = "strftime('%s', '" . $db->escape($cookie) . " 23:59:59') "; }

         $FILTER ? $FILTER .= " and ts <= $datesql" : $FILTER .= "ts <= $datesql";
      }

      /* rcpt domain */

      $cookie = @$this->request->cookie['rcpt_domain'];

      if($cookie && (preg_match('/^[a-z0-9-]+(\.[a-z0-9-]+)*(\.[a-z]{2,5})$/', $cookie) || validemail($cookie) == 1) ) {
         $this->data['rcpt_domain'] = $cookie;

         $to = "to_domain";

         if(strchr($this->data['rcpt_domain'], '@')) { $to = "to"; }

         $SMTP_FILTER ? $SMTP_FILTER .= " and smtp.`$to`='" . $db->escape($cookie) . "'" : $SMTP_FILTER .= " smtp.`$to`='" . $db->escape($cookie) . "'";
      }


      /* sender domain */

      $cookie = @$this->request->cookie['sender_domain'];

      if($cookie && (preg_match('/^[a-z0-9-]+(\.[a-z0-9-]+)*(\.[a-z]{2,5})$/', $cookie) || validemail($cookie) == 1) ) {
         $this->data['sender_domain'] = $cookie;

         $from = "from_domain";

         if(strchr($this->data['sender_domain'], '@')) { $from = "from"; }

         $FILTER ? $FILTER .= " and connection.`$from`='" . $db->escape($cookie) . "'" : $FILTER .= "connection.`$from`='" . $db->escape($cookie) . "'";
      }


      /* assemble postfix filter */

      if($FILTER) { $FILTER = "where " . $FILTER; }



      /* get page */

      if(isset($this->request->get['page']) && is_numeric($this->request->get['page']) && $this->request->get['page'] > 0) {
         $this->data['page'] = $this->request->get['page'];
      }


      $this->data['entries'] = array();

      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {

         $last_update = 0;

         $i = 0;

         $query = $db->query("select count(*) as total from connection $FILTER");

         $this->data['total'] = $query->row['total'];

         $query = $db->query("select * from connection $FILTER order by ts desc limit " . (int)$this->data['page'] * (int)$this->data['page_len'] . ", " . $this->data['page_len']);

         $FILTER = "";

         $SMTP_FILTER ? $FILTER .= " and $SMTP_FILTER" : $FILTER .= " ";

         foreach ($query->rows as $connection) {

            
            $qsmtp = $db->query("select * from smtp where queue_id='" . $db->escape($connection['queue_id']) . "' $FILTER order by ts desc");

            foreach($qsmtp->rows as $smtp) {

               if(isset($smtp['orig_to']) && strlen($smtp['orig_to']) > 3) { $smtp['to'] = $smtp['orig_to']; }

               $result = 'N/A';
               $delivery = '';

               if($smtp['clapf_id']) {

                  $qclapf = $db->query("select * from clapf where clapf_id='" . $smtp['clapf_id'] . "' $CLAPF_FILTER");
                  if(isset($qclapf->row['queue_id2'])) {
                     $result = $qclapf->row['result'];

                     $qsmtp2 = $db->query("select * from smtp where queue_id='" . $qclapf->row['queue_id2'] . "'");
                     if(isset($qsmtp2->row['status'])) {
                        $delivery = $qsmtp2->row['status'] . ", by " . $qsmtp2->row['relay'];
                     }
                  }

               }
               else if(preg_match("/^xxxxxxxx/", $smtp['queue_id'])) {
                  $delivery = 'rejected';
               }
               else {
                  $delivery = $smtp['status'] . ", " . $smtp['relay'];
               }

               $x = explode(" ", $delivery);
               $shortdelivery = array_shift($x);

               if($CLAPF_FILTER == '' || $smtp['clapf_id'] == '' || $result == $this->data['hamspam']) { 
                  $this->data['entries'][] = array(
                                                 'timedate'       => date("Y.m.d. H:i:s", $connection['ts']),
                                                 'client'         => $connection['client'],
                                                 'message_id'     => 'N/A',
                                                 'queue_id'       => $connection['queue_id'],
                                                 'size'           => $this->model_quarantine_message->NiceSize($connection['size']),
                                                 'from'           => $connection['from'],
                                                 'shortfrom'      => strlen($connection['from']) > FROM_LENGTH_TO_SHOW ? substr($connection['from'], 0, FROM_LENGTH_TO_SHOW) . "..." : $connection['from'],
                                                 'to'             => $smtp['to'],
                                                 'shortto'        => strlen($smtp['to']) > FROM_LENGTH_TO_SHOW ? substr($smtp['to'], 0, FROM_LENGTH_TO_SHOW) . "..." : $smtp['to'],
                                                 'delay'          => $smtp['delay'],
                                                 'clapf_id'       => $smtp['clapf_id'],
                                                 'result'         => $result,
                                                 'shortdelivery'  => $shortdelivery,
                                                 'delivery'       => $delivery,
                                               );

               }
            }


            if($i == 0 && isset($connection['ts'])) { $last_update = $connection['ts']; }

            $i++;

            /* fix null sender (<>) */
            //if(strlen($__clapf['from']) == 0) { $__clapf['from'] = "&lt;&gt;"; }


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
