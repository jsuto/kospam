<?php


class ControllerHealthWorker extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "health/worker.tpl";
      $this->layout = "common/layout-empty";

      $db_history = Registry::get('db_history');

      $this->load->model('health/health');
      $this->load->model('quarantine/message');
      $this->load->model('stat/counter');

      $request = Registry::get('request');

      $lang = Registry::get('language');


      $this->data['health'] = array();

      if(Registry::get('admin_user') != 1) {
         die("go away");
      }

      foreach (Registry::get('health_smtp_servers') as $smtp) {
         $this->data['health'][] = $this->model_health_health->checksmtp($smtp, $lang->data['text_error']);
      }

      foreach (Registry::get('postgrey_servers') as $policy) {
         $this->data['health'][] = $this->model_health_health->check_postgrey($policy, $lang->data['text_error']);

      }

      $this->data['queues'][] = format_qshape($lang->data['text_active_incoming_queue'], QSHAPE_ACTIVE_INCOMING);
      $this->data['queues'][] = format_qshape($lang->data['text_active_incoming_queue_sender'], QSHAPE_ACTIVE_INCOMING_SENDER);
      $this->data['queues'][] = format_qshape($lang->data['text_deferred_queue'], QSHAPE_DEFERRED);
      $this->data['queues'][] = format_qshape($lang->data['text_deferred_queue_sender'], QSHAPE_DEFERRED_SENDER);

      $this->data['processed_emails'] = $this->model_health_health->count_processed_emails();

      list ($this->data['uptime'], $this->data['cpuload']) = $this->model_health_health->uptime();

      $this->data['cpuinfo'] = 100 - (int)file_get_contents(CPUSTAT);
      list($this->data['totalmem'], $this->data['meminfo'], $this->data['totalswap'], $this->data['swapinfo']) = $this->model_health_health->meminfo();
      $this->data['shortdiskinfo'] = $this->model_health_health->diskinfo();

      $this->data['number_of_quarantined_messages'] = file_get_contents(NUMBER_OF_QUARANTINED_MESSAGES);

      if(ENABLE_LDAP_IMPORT_FEATURE == 1) {
         $this->data['adsyncinfo'] = @file_get_contents(AD_SYNC_STAT);

         $a = preg_split("/ /", $this->data['adsyncinfo']);
         list ($this->data['totalusers'], $this->data['totalnewusers'], $this->data['totaldeletedusers'], $this->data['total_emails_in_database']) = preg_split("/\//", $a[count($a)-1]);
      }


      /* counter related stuff */

      $db = Registry::get('db');
      $db->select_db($db->database);

      if($this->request->server['REQUEST_METHOD'] == 'POST' && @$this->request->post['resetcounters'] == 1) {
         $this->model_stat_counter->resetCounters();
         header("Location: index.php?route=health/health");
         exit;
      }


      $this->data['counters'] = $this->model_stat_counter->getCounters();
      $this->data['prefix'] = '';
      if(isset($this->data['counters']['_c:rcvd'])) { $this->data['prefix'] = '_c:'; }


      $this->render();
   }


}

?>
