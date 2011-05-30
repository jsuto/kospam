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
      $this->data['uid'] = 0;
      $this->data['uids'] = array();


      if(isset($this->request->get['page']) && is_numeric($this->request->get['page']) && $this->request->get['page'] > 0) {
         $this->data['page'] = $this->request->get['page'];
      }

      $this->data['username'] = Registry::get('username');
      $uid = $this->model_user_user->getUidByName($this->data['username']);



      if(Registry::get('admin_user') == 1 || Registry::get('domain_admin') == 1) {

         if(isset($this->request->get['user']) && strlen($this->request->get['user']) > 1) {

            /* fix username if given */

            if(Registry::get('admin_user') == 1 || $this->model_user_user->isUserInMyDomain($this->request->get['user']) == 1) {
               $this->data['username'] = $this->request->get['user'];
               $this->data['uid'] = $this->model_user_user->getUidByName($this->data['username']);
               array_push($this->data['uids'], $this->data['uid']);
            }

            /* or restrict user list for domain admin */

            else if(Registry::get('domain_admin') == 1) {
               $this->data['uids'] = $this->model_user_user->getUidsByDomain($_SESSION['domain']);
            }
         }

      }
      else {
         array_push($this->data['uids'], $this->model_user_user->getUidByName($this->data['username']));
      }



      $this->data['sort'] = 'ts';

      $this->data['order'] = (int)@$this->request->get['order'];

      if(@$this->request->get['sort'] == "ts") { $this->data['sort'] = "ts"; }
      if(@$this->request->get['sort'] == "from") { $this->data['sort'] = "from"; }
      if(@$this->request->get['sort'] == "subj") { $this->data['sort'] = "subj"; }
      if(@$this->request->get['sort'] == "size") { $this->data['sort'] = "size"; }

      if($this->data['sort'] == "ts" && @$this->request->get['order'] == "") { $this->data['order'] = 1; }


      /* check if he's a valid user */

      if($this->data['uid'] == -1) {
         $this->template = "common/error.tpl";
         $this->data['errorstring'] = $this->data['text_non_existing_user'] . ": " . $this->data['username'];

         $this->render();
         exit;
      }

      $Q = Registry::get('Q');

      /* add search term if there's any */

      if($this->data['from'] || $this->data['subj'] || $this->data['hamspam']) {
         $this->model_quarantine_database->addSearchTerm($this->data['from'], $this->data['subj'], $this->data['hamspam'], $uid);
      }


      /* read search terms */
      $this->data['searchterms'] = $this->model_quarantine_database->getSearchTerms($uid);


      /* get messages from quarantine */

      list ($this->data['n'], $this->data['total_size'], $this->data['messages']) =
                 $this->model_quarantine_database->getMessages($this->data['uids'], $this->data['page'], $this->data['page_len'], $this->data['from'], $this->data['subj'], $this->data['hamspam'], $this->data['sort'], $this->data['order']);


      /* print paging info */

      $this->data['prev_page'] = $this->data['page'] - 1;
      $this->data['next_page'] = $this->data['page'] + 1;

      $this->data['total_pages'] = floor($this->data['n'] / $this->data['page_len']);


      $this->render();
   }


}

?>
