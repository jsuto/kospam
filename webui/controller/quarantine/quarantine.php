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

      $this->data['user'] = $this->data['from'] = $this->data['to'] = $this->data['subj'] = $this->data['date'] = "";
      $this->data['hamspam'] = "SPAM";

      if(isset($this->request->get['user'])) { $this->data['user'] = $this->request->get['user']; }
      if(isset($this->request->get['from'])) { $this->data['from'] = $this->request->get['from']; }
      if(isset($this->request->get['to'])) { $this->data['to'] = $this->request->get['to']; }
      if(isset($this->request->get['subj'])) { $this->data['subj'] = $this->request->get['subj']; }
      if(isset($this->request->get['hamspam'])) { $this->data['hamspam'] = $this->request->get['hamspam']; }
      if(isset($this->request->get['date'])) { $this->data['date'] = $this->request->get['date']; }



      $this->data['uid'] = 0;
      $this->data['uids'] = array();

      $this->data['username'] = Registry::get('username');


      if(isset($this->request->get['page']) && is_numeric($this->request->get['page']) && $this->request->get['page'] > 0) {
         $this->data['page'] = $this->request->get['page'];
      }


      if($this->data['to']) {
         $this->data['user'] = $this->model_user_user->get_username_by_email($this->data['to']);

         if($this->data['user'] == "") {
            $this->template = "common/error.tpl";
            $this->data['errorstring'] = $this->data['text_non_existing_user'] . ": " . $this->data['to'];

            $this->render();
            exit;
         }
      }


      /* determine uids to query from quarantine database */

      if(Registry::get('admin_user') == 1) {
         if($this->data['user']) {
            array_push($this->data['uids'], $this->model_user_user->getUidByName($this->data['user']));
            $this->data['username'] = $this->data['user'];
         }
      }

      else if(Registry::get('domain_admin') == 1) {

         if($this->data['user'] && $this->model_user_user->isUserInMyDomain($this->request->get['user']) == 1) {
            array_push($this->data['uids'], $this->model_user_user->getUidByName($this->data['user']));
         }
         else {
            $this->data['uids'] = $this->model_user_user->getUidsByDomain($_SESSION['domain']);
         }
      }

      else {
         array_push($this->data['uids'], $this->model_user_user->getUidByName(Registry::get('username')));
      }



      /* sort and order */

      $this->data['sort'] = 'ts';

      $this->data['order'] = (int)@$this->request->get['order'];

      if(isset($this->request->get['sort'])) {
         if($this->request->get['sort'] == "ts") { $this->data['sort'] = "ts"; }
         if($this->request->get['sort'] == "from") { $this->data['sort'] = "from"; }
         if($this->request->get['sort'] == "to") { $this->data['sort'] = "to"; }
         if($this->request->get['sort'] == "subj") { $this->data['sort'] = "subj"; }
         if($this->request->get['sort'] == "size") { $this->data['sort'] = "size"; }
      }

      if($this->data['sort'] == "ts" && @$this->request->get['order'] == "") { $this->data['order'] = 1; }



      $Q = Registry::get('Q');


      /* search terms */

      $uid = $this->model_user_user->getUidByName(Registry::get('username'));

      if($this->data['from'] || $this->data['to'] || $this->data['subj'] || $this->data['hamspam']) {
         $this->model_quarantine_database->addSearchTerm($this->data['date'], $this->data['from'], $this->data['to'], $this->data['subj'], $this->data['hamspam'], $uid);
      }

      $this->data['searchterms'] = $this->model_quarantine_database->getSearchTerms($uid);




      /* get messages from quarantine */

      list ($this->data['n'], $this->data['total_size'], $this->data['messages']) =
                 $this->model_quarantine_database->getMessages($this->data['uids'], $this->data['page'], $this->data['page_len'], $this->data['date'], $this->data['from'], $this->data['subj'], $this->data['hamspam'], $this->data['sort'], $this->data['order']);

      if(count($this->data['messages']) == 0 && $this->data['page'] > 0) {
         $this->data['page']--;
         list ($this->data['n'], $this->data['total_size'], $this->data['messages']) = $this->model_quarantine_database->getMessages($this->data['uids'], $this->data['page'], $this->data['page_len'], $this->data['date'], $this->data['from'], $this->data['subj'], $this->data['hamspam'], $this->data['sort'], $this->data['order']);
      }


      /* paging info */

      $this->data['prev_page'] = $this->data['page'] - 1;
      $this->data['next_page'] = $this->data['page'] + 1;

      $this->data['total_pages'] = floor($this->data['n'] / $this->data['page_len']);


      $this->render();
   }


}

?>
