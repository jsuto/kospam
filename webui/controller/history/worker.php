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

      $CLAPF_FILTER = "";

      $FILTER = " ( (client != 'localhost[127.0.0.1]' and relay != '127.0.0.1[127.0.0.1]:10025') or client='localhost[127.0.0.1]' ) ";

      $this->data['hamspam'] = "";
      $this->data['sender_domain'] = "";
      $this->data['rcpt_domain'] = "";

      $this->data['tot_time'] = 0;

      $view = "hist_in";


      /* subject */

      if(isset($this->request->cookie['subject']) && $this->request->cookie['subject']) {
         $this->data['subject'] = $this->request->cookie['subject'];
         $FILTER = " `subject` LIKE '%" . $this->db->escape($this->request->cookie['subject']) . "%'";

         $view = "hist_out";
      }


      /* ham or spam */

      /*if(isset($this->request->cookie['hamspam']) && ($this->request->cookie['hamspam'] == "HAM" || $this->request->cookie['hamspam']  == "SPAM") ) {
         $this->data['hamspam'] = $this->request->cookie['hamspam'];
         $FILTER = " and result='" . $this->db->escape($this->request->cookie['hamspam']) . "'";
      }*/



      /* dates */

      if(isset($this->request->cookie['date1']) && $this->request->cookie['date1']) {
         if(HISTORY_DRIVER == 'mysql') { $datesql = "UNIX_TIMESTAMP('" . $this->db->escape($this->request->cookie['date1']) . " 00:00:00') "; }
         else { $datesql = "strftime('%s', '" . $this->db->escape($this->request->cookie['date1']) . " 00:00:00') "; }

         $FILTER ? $FILTER .= " and ts >= $datesql" : $FILTER .= "ts >= $datesql";
      }

      if(isset($this->request->cookie['date2']) && $this->request->cookie['date2']) {
         if(HISTORY_DRIVER == 'mysql') { $datesql = "UNIX_TIMESTAMP('" . $this->db->escape($this->request->cookie['date2']) . " 23:59:59') "; }
         else { $datesql = "strftime('%s', '" . $db->escape($cookie) . " 23:59:59') "; }

         $FILTER ? $FILTER .= " and ts <= $datesql" : $FILTER .= "ts <= $datesql";
      }


      /* rcpt domain */

      //if($cookie && (preg_match('/^[a-z0-9-]+(\.[a-z0-9-]+)*(\.[a-z]{2,5})$/', $cookie) || validemail($cookie) == 1) ) {
      if(isset($this->request->cookie['rcpt_domain']) && preg_match('/^([a-z0-9-\.\+\_\@]+)/', $this->request->cookie['rcpt_domain'])) {
         $this->data['rcpt_domain'] = $this->request->cookie['rcpt_domain'];
         $FILTER ? $FILTER .= " and ($view.`to` LIKE '%" . $this->db->escape($this->request->cookie['rcpt_domain']) . "%' or $view.`orig_to` LIKE '%" . $this->db->escape($this->request->cookie['rcpt_domain']) . "%')" : $FILTER .= " ($view.`to` LIKE '%" . $this->db->escape($this->request->cookie['rcpt_domain']) . "%' or $view.`orig_to` LIKE '%" . $this->db->escape($this->request->cookie['rcpt_domain']) . "%')";
      }


      /* sender domain */

      if(isset($this->request->cookie['sender_domain'])  && preg_match('/^([a-z0-9-\.\+\_\@]+)/', $this->request->cookie['sender_domain'])) {
         $this->data['sender_domain'] = $this->request->cookie['sender_domain'];
         $FILTER ? $FILTER .= " and $view.`from` LIKE '%" . $this->db->escape($this->request->cookie['sender_domain']) . "%'" : $FILTER .= "$view.`from` LIKE '%" . $this->db->escape($this->request->cookie['sender_domain']) . "%'";
      }


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


      foreach ($query->rows as $connection) {

         if(isset($connection['orig_to']) && $connection['orig_to']) { $connection['to'] = $connection['orig_to']; }

         $delivery = $connection['status'] . ", " . $connection['relay'];

         if(preg_match("/^xxxxxxxx/", $connection['queue_id'])) {
            $delivery = $connection['status'];
         }

         if($view == "hist_in") {
            $query_clapf = $this->db->query("select result, subject from clapf where queue_id2='" . $this->db->escape($connection['queue_id']) . "'");
            $this->data['tot_time'] += $query_clapf->exec_time;

            if(isset($query_clapf->row['result'])) {
               $connection['result'] = $query_clapf->row['result'];
               $connection['subject'] = $query_clapf->row['subject'];
            }
         }

         $x = explode(" ", $delivery);
         $shortdelivery = array_shift($x);

         isset($connection['subject']) ? $subject = $connection['subject'] : $subject = '-';
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
