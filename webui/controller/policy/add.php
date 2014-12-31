<?php


class ControllerPolicyAdd extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "policy/add.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('policy/policy');

      $this->document->title = $this->data['text_add_policy'];


      $this->data['username'] = Registry::get('username');

      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {

         if($this->request->server['REQUEST_METHOD'] == 'POST') {
            $ret = 0;

            if($this->validate() == true){
               $ret = $this->model_policy_policy->add($this->request->post);

               if($ret == 1){
                  $this->data['x'] = $this->data['text_successfully_added'];
               } else {
                  $this->data['errorstring'] = $this->data['text_failed_to_add'] . ": " . $ret;
               }
            }
            else {
               $this->data['errorstring'] = $this->data['text_error_message'];
               $this->data['errors'] = $this->error;
            }

            if($ret == 0) {
               $this->data['post'] = $this->request->post;
            }

         }
         else {
            $this->data['post'] = array(
                                     'silently_discard_infected_email' => 'a',
                                     'use_antispam'                    => 'a',
                                     'max_message_size_to_filter'      => 128000,
                                     'spam_overall_limit'              => 0.92,
                                     'spaminess_oblivion_limit'        => 1.01,
                                     'store_emails'                    => 'a',
                                     'store_only_spam'                 => 'a',
                                     'training_mode'                   => 0,
                                     'message_from_a_zombie'           => 0,
                                     'smtp_addr'                       => SMARTHOST,
                                     'smtp_port'                       => SMARTHOST_PORT
                                  );

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

      if(isBinary(@$this->request->post['store_emails']) == 0) {
         $this->error['aaa'] = $this->data['text_invalid_policy_setting'];
      }

      if(isBinary(@$this->request->post['store_only_spam']) == 0) {
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
