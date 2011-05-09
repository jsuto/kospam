<?php


class ControllerHealthWorker extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "health/worker.tpl";
      $this->layout = "common/layout-empty";

      $db_history = Registry::get('db_history');

      $this->load->model('health/health');

      $request = Registry::get('request');

      $lang = Registry::get('language');


      $this->data['health'] = array();

      if(Registry::get('admin_user') != 1) {
         die("go away");
      }

      foreach (Registry::get('health_smtp_servers') as $smtp) {
         $this->data['health'][] = $this->model_health_health->checksmtp($smtp[0], $smtp[1], $lang->data['text_error']);
      }

      $this->data['queues'][] = format_qshape($lang->data['text_active_incoming_queue'], QSHAPE_ACTIVE_INCOMING);
      $this->data['queues'][] = format_qshape($lang->data['text_active_incoming_queue_sender'], QSHAPE_ACTIVE_INCOMING_SENDER);
      $this->data['queues'][] = format_qshape($lang->data['text_deferred_queue'], QSHAPE_DEFERRED);
      $this->data['queues'][] = format_qshape($lang->data['text_deferred_queue_sender'], QSHAPE_DEFERRED_SENDER);

      $this->data['emails'] = $this->model_health_health->get_last_maillog_entries();

      $this->data['processed_emails'] = $this->model_health_health->count_processed_emails();

      $this->data['uptime'] = $this->model_health_health->uptime();

      $this->data['meminfo'] = $this->model_health_health->meminfo();
      $this->data['diskinfo'] = $this->model_health_health->diskinfo();


      $this->render();
   }


}

?>
