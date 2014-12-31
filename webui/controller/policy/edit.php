<?php


class ControllerPolicyEdit extends Controller {
   private $error = array();

   public function index(){
      $this->data['policy_group'] = 0;

      $this->id = "content";
      $this->template = "policy/edit.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('policy/policy');

      $this->document->title = $this->data['text_policy'];

      $this->data['username'] = Registry::get('username');

      $this->data['id'] = -1;

      if(isset($this->request->get['id']) && is_numeric($this->request->get['id']) && $this->request->get['id'] > 0) {
         $this->data['id'] = $this->request->get['id'];
      }


      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {


         if($this->request->server['REQUEST_METHOD'] == 'POST') {

            if($this->validate() == true){
               if($this->model_policy_policy->update($this->request->post) == 1) {
                  $this->data['x'] = $this->data['text_successfully_updated'];
               }
               else {
                  $this->template = "common/error.tpl";
                  $this->data['errorstring'] = $this->data['text_failed_to_update'];
               }
            }
         }
         else {
            $this->data['post'] = $this->model_policy_policy->get_policy($this->data['id']);
         }
      }
      else {
         $this->template = "common/error.tpl";
         $this->data['errorstring'] = $this->data['text_you_are_not_admin'];
      }


      $this->render();
   }


   private function validate() {

      $this->model_policy_policy->fix_post_data();

      if(!isset($this->request->post['id']) || !is_numeric($this->request->post['id']) || $this->request->post['id'] <= 0){
         $this->error['aaa'] = $this->data['text_invalid_policy_setting'];
      }

      if(strlen(@$this->request->post['name']) < 2 || !preg_match("/^([\a-zA-ZµÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝßàáâãäåæçèéêëìíîïðñòóôõöøùúûüýÿ0-9\-\_\ ]+)$/i", iconv("utf-8", "iso-8859-2", $this->request->post['name']) )) {
         $this->error['aaa'] = $this->data['text_invalid_policy_name'];
      }

      if(isBinary(@$this->request->post['deliver_infected_email']) == 0) {
         $this->error['aaa'] = $this->data['text_invalid_policy_setting'];
      }

      if(isBinary(@$this->request->post['silently_discard_infected_email']) == 0) {
         $this->error['aaa'] = $this->data['text_invalid_policy_setting'];
      }

      if(isBinary(@$this->request->post['use_antispam']) == 0) {
         $this->error['aaa'] = $this->data['text_invalid_policy_setting'];
      }

      if(isBinary(@$this->request->post['enable_auto_white_list']) == 0) {
         $this->error['aaa'] = $this->data['text_invalid_policy_setting'];
      }

      if(isBinary(@$this->request->post['replace_junk_characters']) == 0) {
         $this->error['aaa'] = $this->data['text_invalid_policy_setting'];
      }

      if(isBinary(@$this->request->post['penalize_images']) == 0) {
         $this->error['aaa'] = $this->data['text_invalid_policy_setting'];
      }

      if(isBinary(@$this->request->post['penalize_embed_images']) == 0) {
         $this->error['aaa'] = $this->data['text_invalid_policy_setting'];
      }

      if(isBinary(@$this->request->post['penalize_octet_stream']) == 0) {
         $this->error['aaa'] = $this->data['text_invalid_policy_setting'];
      }

      if(isBinary(@$this->request->post['training_mode']) == 0) {
         $this->error['aaa'] = $this->data['text_invalid_policy_setting'];
      }

      if(isBinary(@$this->request->post['initial_1000_learning']) == 0) {
         $this->error['aaa'] = $this->data['text_invalid_policy_setting'];
      }

      if(isBinary(@$this->request->post['store_metadata']) == 0) {
         $this->error['aaa'] = $this->data['text_invalid_policy_setting'];
      }

      if(isBinary(@$this->request->post['store_only_spam']) == 0) {
         $this->error['aaa'] = $this->data['text_invalid_policy_setting'];
      }

      if(isBinary(@$this->request->post['message_from_a_zombie']) == 0) {
         $this->error['aaa'] = $this->data['text_invalid_policy_setting'];
      }

      if (!$this->error) {
         return true;
      } else {
         return false;
      }

   }


}

?>
