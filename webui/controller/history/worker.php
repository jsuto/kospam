<?php

class ControllerHistoryWorker extends Controller {
   private $db;

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

      $this->data['search'] = "";

      if($this->request->server['REQUEST_METHOD'] == 'POST' && isset($this->request->post['search']) ){
         $this->data['search'] = $this->request->post['search'];
      }
      else if($this->request->server['REQUEST_METHOD'] == 'GET' && isset($this->request->get['search']) ){
         $this->data['search'] = $this->request->get['search'];
      }


      $this->db = Registry::get('db_history');


      /* assemble filter restrictions */

      $FILTER = "( (client != 'localhost[" . LOCALHOST . "]' and relay != '" . LOCALHOST . "[" . LOCALHOST . "]:" . CLAPF_PORT . "') or client='localhost[" . LOCALHOST . "]' ) ";

      $datefilter = $fromfilter = $tofilter = $subjfilter = $hamspamfilter = "";

      $this->data['hamspam'] = "";
      $this->data['sender_domain'] = "";
      $this->data['rcpt_domain'] = "";

      $this->data['tot_time'] = 0;

      $view = "hist_in";


      /* dates */

      if(isset($this->request->cookie['date1']) && $this->request->cookie['date1']) {
         if(HISTORY_DRIVER == 'mysql') { $datesql = "UNIX_TIMESTAMP('" . $this->db->escape($this->request->cookie['date1']) . " 00:00:00') "; }
         else { $datesql = "strftime('%s', '" . $this->db->escape($this->request->cookie['date1']) . " 00:00:00') "; }

         $datefilter ? $datefilter .= " and ts >= $datesql" : $datefilter .= "ts >= $datesql";
      }

      if(isset($this->request->cookie['date2']) && $this->request->cookie['date2']) {
         if(HISTORY_DRIVER == 'mysql') { $datesql = "UNIX_TIMESTAMP('" . $this->db->escape($this->request->cookie['date2']) . " 23:59:59') "; }
         else { $datesql = "strftime('%s', '" . $db->escape($cookie) . " 23:59:59') "; }

         $datefilter ? $datefilter .= " and ts <= $datesql" : $datefilter .= "ts <= $datesql";
      }


      /* subject */

      if(isset($this->request->cookie['subject']) && $this->request->cookie['subject']) {
         $this->data['subject'] = $this->request->cookie['subject'];
         $subjfilter = " `subject` LIKE '%" . $this->db->escape(urldecode($this->request->cookie['subject']))  . "%'";

         $FILTER = "";

         $view = "hist_out";
      }


      /* ham/spam */

      if(isset($this->request->cookie['hamspam']) && $this->request->cookie['hamspam']) {
         $this->data['hamspam'] = $this->request->cookie['hamspam'];
         $hamspamfilter = " result='" . $this->db->escape($this->request->cookie['hamspam']) . "'";

         $FILTER = "";

         $view = "hist_out";
      }


      /* rcpt domain */

      if(isset($this->request->cookie['rcpt_domain']) && preg_match('/^([a-z0-9-\.\+\_\@]+)/', $this->request->cookie['rcpt_domain'])) {
         $this->data['rcpt_domain'] = $this->request->cookie['rcpt_domain'];
         $tofilter = " $view.`to` LIKE '" . $this->db->escape($this->request->cookie['rcpt_domain']) . "%' ";
      }


      /* sender domain */

      if(isset($this->request->cookie['sender_domain'])  && preg_match('/^([a-z0-9-\.\+\_\@]+)/', $this->request->cookie['sender_domain'])) {
         $this->data['sender_domain'] = $this->request->cookie['sender_domain'];
         $fromfilter = "$view.`from` LIKE '" . $this->db->escape($this->request->cookie['sender_domain']) . "%'";
      }


      if($fromfilter) { $FILTER ? $FILTER .= " AND $fromfilter" : $FILTER .= " $fromfilter"; }
      if($tofilter) { $FILTER ? $FILTER .= " AND $tofilter" : $FILTER .= " $tofilter"; }
      if($subjfilter) { $FILTER ? $FILTER .= " AND $subjfilter" : $FILTER .= " $subjfilter"; }
      if($hamspamfilter) { $FILTER ? $FILTER .= " AND $hamspamfilter" : $FILTER .= " $hamspamfilter"; }


      if($datefilter == '' && $fromfilter == '' && $tofilter == '' && $subjfilter == '') {
         $datefilter = sprintf("ts > %d", time() - HISTORY_LATEST_TIME_RANGE);
      }

      if($datefilter) { $FILTER = " $datefilter AND $FILTER "; }


      /* assemble filter */

      if($FILTER) { $FILTER = "where " . $FILTER; }



      /* get page */

      if(isset($this->request->get['page']) && is_numeric($this->request->get['page']) && $this->request->get['page'] > 0) {
         $this->data['page'] = $this->request->get['page'];
      }


      $this->data['entries'] = array();

      /* check if we are admin */

      if(Registry::get('admin_user') == 1 || Registry::get('readonly_admin') == 1) {

         $this->get_history($view, $FILTER);


         $this->data['total_pages'] = floor($this->data['total'] / $this->data['page_len']);

         $this->data['prev_page'] = $this->data['page'] - 1;
         $this->data['next_page'] = $this->data['page'] + 1;


         $this->render();

      }
      else {
         die("go away");
      }


   }

   private function get_history($view = '', $filter = '') {
      $i=0;
      $last_update = 0;


      $query = $this->db->query("select count(*) as total from $view $filter");

      $this->data['total'] = $query->row['total'];
      $this->data['tot_time'] += $query->exec_time;

      $query = $this->db->query("select * from $view $filter order by ts desc limit " . (int)$this->data['page'] * (int)$this->data['page_len'] . ", " . $this->data['page_len']);
      $this->data['tot_time'] += $query->exec_time;

      if($view == "hist_in") {
         $queue_ids = "";
         foreach ($query->rows as $connection) {
            if($queue_ids) { $queue_ids .= ","; }
            $queue_ids .= "'" . $this->db->escape($connection['queue_id']) . "'";
         }

         $clapf = array();

         $query_clapf = $this->db->query("select result, subject, queue_id2 from clapf where queue_id2 IN ($queue_ids)");
         $this->data['tot_time'] += $query_clapf->exec_time;

         foreach ($query_clapf->rows as $qclapf) {
            $clapf[$qclapf['queue_id2']] = array('result' => $qclapf['result'], 'subject' => $qclapf['subject']);
         }
      }


      foreach ($query->rows as $connection) {

         if(isset($connection['orig_to']) && $connection['orig_to']) { $connection['to'] = $connection['orig_to']; }

         $delivery = $connection['status'] . ", " . $connection['relay'];

         if(preg_match("/^xxxxxxxx/", $connection['queue_id'])) {
            $delivery = $connection['status'];
         }

         if($view == "hist_in" && isset($clapf[$connection['queue_id']])) {
            $connection['result'] = $clapf[$connection['queue_id']]['result'];
            $connection['subject'] = $clapf[$connection['queue_id']]['subject'];
         }

         $x = explode(" ", $delivery);
         $shortdelivery = array_shift($x);

         isset($connection['subject']) ? $subject = htmlspecialchars($connection['subject']) : $subject = '-';

         if(strlen($subject) > 2*FROM_LENGTH_TO_SHOW) { $shortsubject = substr($subject, 0, 2*FROM_LENGTH_TO_SHOW) . "..."; }
         else { $shortsubject = $subject; }

         $this->data['entries'][] = array(
                                                 'timedate'       => date("Y.m.d. H:i:s", $connection['ts']),
                                                 'client'         => isset($connection['client']) ? $connection['client'] : '',
                                                 'queue_id'       => $connection['queue_id'],
                                                 'size'           => $this->model_quarantine_message->NiceSize($connection['size']),
                                                 'from'           => $connection['from'],
                                                 'shortfrom'      => strlen($connection['from']) > FROM_LENGTH_TO_SHOW ? substr($connection['from'], 0, FROM_LENGTH_TO_SHOW) . "..." : $connection['from'],
                                                 'to'             => $connection['to'],
                                                 'shortto'        => strlen($connection['to']) > FROM_LENGTH_TO_SHOW ? substr($connection['to'], 0, FROM_LENGTH_TO_SHOW) . "..." : $connection['to'],
                                                 'result'         => isset($connection['result']) ? $connection['result'] : 'N/A',
                                                 'subject'        => $subject,
                                                 'shortsubject'   => $shortsubject,
                                                 'delivery'       => $delivery,
                                                 'shortdelivery'  => $shortdelivery,
                                          );


         if($i == 0 && isset($connection['ts'])) { $last_update = $connection['ts']; }

         $i++;

      }

      setcookie("lastupdate", $last_update, time()+3600);

   }


}


?>
