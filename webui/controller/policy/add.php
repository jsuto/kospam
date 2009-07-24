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

      $this->document->title = $this->data['text_policy'];


      $this->data['username'] = Registry::get('username');

      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {

         if($this->request->server['REQUEST_METHOD'] == 'POST') {
            if($this->validate() == true){
               if($this->model_policy_policy->addPolicy($this->request->post) == 1) {
                  $this->data['x'] = $this->data['text_successfully_added'];
               } else {
                  $this->template = "common/error.tpl";
                  $this->data['errorstring'] = $this->data['text_failed_to_add'];
               }
            }
            else {
               $this->template = "common/error.tpl";
               $this->data['errorstring'] = array_pop($this->error);
            }
         }
         else {
            $this->data['policy'] = array(
                                     'policy_group'                    => $this->model_policy_policy->getNewPolicyGroupId(),
                                     'name'                            => "",
                                     'deliver_infected_email'          => 0,
                                     'silently_discard_infected_email' => 1,
                                     'use_antispam'                    => 1,
                                     'spam_subject_prefix'             => "",
                                     'enable_auto_white_list'          => 1,
                                     'max_message_size_to_filter'      => 128000,
                                     'rbl_domain'                      => "",
                                     'surbl_domain'                    => "",
                                     'spam_overall_limit'              => 0.92,
                                     'spaminess_oblivion_limit'        => 1.01,
                                     'replace_junk_characters'         => 1,
                                     'invalid_junk_limit'              => 5,
                                     'invalid_junk_line'               => 1,
                                     'penalize_images'                 => 0,
                                     'penalize_embed_images'           => 0,
                                     'penalize_octet_stream'           => 0,
                                     'training_mode'                   => 0,
                                     'initial_1000_learning'           => 0,
                                     'store_metadata'                  => 0
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

      if((int)@$this->request->post['policy_group'] <= 0){
         $this->error['aaa'] = $this->data['text_invalid_policy_group'];
      }

      if(strlen(@$this->request->post['name']) < 2){
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

      if (!$this->error) {
         return true;
      } else {
         return false;
      }

   }


}

?>
