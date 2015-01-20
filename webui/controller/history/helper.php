<?php


class ControllerHistoryHelper extends Controller {
   private $error = array();
   private $a = array(
                    'date1'           => '',
                    'date2'           => '',
                    'direction'       => '',
                    'size'            => '',
                    'attachment_type' => '',
                    'tag'             => '',
                    'note'            => '',
                    'id'              => '',
                    'spam'            => '',
                    'history'         => 1,
                    'match'           => array()
                     );


   public function index(){

      $this->id = "content";
      $this->template = "history/helper.tpl";
      $this->layout = "common/layout-empty";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $sphx = Registry::get('sphx');

      $this->load->model('search/search');
      $this->load->model('search/message');
      $this->load->model('user/user');

      $this->data['page'] = 0;
      if(isset($this->request->post['page'])) { $this->data['page'] = $this->request->post['page']; }

      $this->data['page_len'] = get_page_length();

      $this->data['total_hits'] = -1;

      if($this->request->post['searchtype'] == 'expert'){
         if(isset($this->request->post['search']) && preg_match("/(from|to|subject|body|direction|d|size|date1|date2|attachment|a|tag|note|id|spam|status)\:/", $this->request->post['search'])) {
            $this->a = $this->model_search_search->preprocess_post_expert_request($this->request->post);
         }

         $this->fixup_post_request();
      }

      $this->data['result'] = $this->model_search_search->get_search_session_data($this->a);

      if(!isset($this->data['result']['total_found'])) {
         $this->data['result'] = $this->model_search_search->search_messages($this->a, $this->data['page']);
         $this->model_search_search->store_search_result_to_blob($this->a, $this->data['result']);
      }

      /* paging info */

      $this->data['prev_page'] = $this->data['page'] - 1;
      $this->data['next_page'] = $this->data['page'] + 1;

      $this->data['total_pages'] = ceil($this->data['result']['total_hits'] / $this->data['page_len'])-1;

      $this->data['hits_from'] = $this->data['page'] * $this->data['page_len'] + 1;
      $this->data['hits_to'] = ($this->data['page']+1) * $this->data['page_len'];

      if($this->data['hits_to'] > $this->data['result']['total_hits']) { $this->data['hits_to'] = $this->data['result']['total_hits']; }

      $this->data['sort'] = $this->request->post['sort'];
      $this->data['order'] = $this->request->post['order'];

      $this->render();
   }


   private function fixup_post_request() {
      $this->a['sort'] = $this->request->post['sort'];
      $this->a['order'] = $this->request->post['order'];
      $this->a['history'] = 1;
   }


}

?>
