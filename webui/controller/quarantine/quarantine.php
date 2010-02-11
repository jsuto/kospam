<?php


class ControllerQuarantineQuarantine extends Controller {
   private $error = array();
   public $Q;

   public function index(){

      $this->id = "content";
      $this->template = "quarantine/list.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('quarantine/message');
      $this->load->model('quarantine/database');
      $this->load->model('user/user');

      $this->document->title = $this->data['text_quarantine'];


      $this->data['page'] = 0;

      $this->data['page_len'] = getPageLength();
      $this->data['from'] = @$this->request->get['from'];
      $this->data['subj'] = @$this->request->get['subj'];
      $this->data['hamspam'] = @$this->request->get['hamspam'];

      if(isset($this->request->get['page']) && is_numeric($this->request->get['page']) && $this->request->get['page'] > 0) {
         $this->data['page'] = $this->request->get['page'];
      }

      $this->data['username'] = Registry::get('username');

      /* fix username if we are admin */

      if(isset($this->request->get['user']) && strlen($this->request->get['user']) > 1 && (Registry::get('admin_user') == 1 || $this->model_user_user->isUserInMyDomain($this->request->get['user']) == 1) ) {
         $this->data['username'] = $this->request->get['user'];
      }


      /* 
         if you are an admin user I show you a form to select a user's quarantine
       */


      if(Registry::get('admin_user') == 1 && !isset($this->request->get['user'])) {
          $this->document->title = $this->data['text_users_quarantine'];

          if(!file_exists(QUEUE_DIRECTORY)){
             $this->template = "common/error.tpl";
             $this->document->title = $this->data['text_error'];
             $this->data['errorstring'] = $this->data['text_non_existent_queue_directory'] . ": " . QUEUE_DIRECTORY;

          }
          else {
             $this->template = "quarantine/user.tpl";
          }

          $this->render();
          exit;
      }

      /* check if he's a valid user */

      $uid = $this->model_user_user->getUidByName($this->data['username']);

      if($uid == -1) {
         $this->template = "common/error.tpl";
         $this->data['errorstring'] = $this->data['text_non_existing_user'] . ": " . $this->data['username'];

         $this->render();
         exit;
      }


      $domain = $this->model_user_user->getDomainsByUid($uid);
      $my_q_dir = get_per_user_queue_dir($domain[0], $this->data['username'], $uid);


      if(file_exists($my_q_dir)) {

         $Q = new DB("sqlite", "", "", "", $my_q_dir . "/" . QUARANTINE_DATA, "");
         Registry::set('Q', $Q);

         /* create schema if it's a 0 byte length file */

         $st = stat($my_q_dir . "/" . QUARANTINE_DATA);
         if(isset($st['size']) && $st['size'] < 1024) {
            $this->model_quarantine_database->CreateDatabase();
         }


         /* populate quarantine files to database if we are on the 1st page */

         if($this->data['page'] == 0) {
            $this->model_quarantine_database->PopulateDatabase($my_q_dir);
         }


         /* add search term if there's any */

         if($this->data['from'] || $this->data['subj'] || $this->data['hamspam']) {
            $this->model_quarantine_database->addSearchTerm($this->data['from'], $this->data['subj'], $this->data['hamspam']);
         }


         /* read search terms */
         $this->data['searchterms'] = $this->model_quarantine_database->getSearchTerms();


         /* get messages from quarantine */

         list ($this->data['n'], $this->data['total_size'], $this->data['messages']) =
                 $this->model_quarantine_database->getMessages($my_q_dir, $this->data['username'], $this->data['page'], $this->data['page_len'], $this->data['from'], $this->data['subj'], $this->data['hamspam']);
      }
      else {
         $this->data['n'] = $this->data['total_size'] = 0;
         $this->data['messages'] = $this->data['searchterms'] = array();
      }


      /* print paging info */

      $this->data['prev_page'] = $this->data['page'] - 1;
      $this->data['next_page'] = $this->data['page'] + 1;

      $this->data['total_pages'] = floor($this->data['n'] / $this->data['page_len']);


      $this->render();
   }


}

?>
