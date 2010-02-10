<?php


class ControllerUserList extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "user/list.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');
      $language = Registry::get('language');

      $this->load->model('user/user');
      $this->load->model('policy/policy');

      $this->document->title = $language->get('text_user_management');


      $this->data['page'] = 0;
      $this->data['page_len'] = getPageLength();

      $this->data['total_users'] = 0;

      $users = array();


      /* get search term if there's any */

      if($this->request->server['REQUEST_METHOD'] == 'POST'){
         $this->data['search'] = @$this->request->post['search'];
      }
      else {
         $this->data['search'] = @$this->request->get['search'];
      }

      /* get page */

      if(isset($this->request->get['page']) && is_numeric($this->request->get['page']) && $this->request->get['page'] > 0) {
         $this->data['page'] = $this->request->get['page'];
      }


      /* get list order */
      $this->data['order'] = "";

      if((int)@$this->request->get['uid'] == 1) { $this->data['order'] = "&amp;uid=1"; }
      if((int)@$this->request->get['user'] == 1) { $this->data['order'] = "&amp;user=1"; }
      if((int)@$this->request->get['email'] == 1) { $this->data['order'] = "&amp;email=1"; }
      if((int)@$this->request->get['domain'] == 1) { $this->data['order'] = "&amp;domain=1"; }


      /* check if we are admin */

      if(Registry::get('admin_user') == 1 || Registry::get('domain_admin') == 1) {

         $users = $this->model_user_user->getUsers($this->data['search'], $this->data['page'], $this->data['page_len'], 
                    @$this->request->get['uid'], @$this->request->get['user'], @$this->request->get['email'], @$this->request->get['domain']);

         $this->data['total_users'] = $this->model_user_user->howManyUsers($this->data['search']);

         foreach ($users as $user) {
            $policy = $this->model_policy_policy->getPolicy($user['policy_group']);

            if(isset($policy['name'])){
               $policy_group = $policy['name'];
            }
            else {
               $policy_group = DEFAULT_POLICY;
            }

            $this->data['users'][] = array(
                                          'uid'          => $user['uid'],
                                          'username'     => $user['username'],
                                          'email'        => $user['email'],
                                          'shortemail'   => short_email($user['email']),
                                          'domain'       => $user['domain'],
                                          'policy_group' => $policy_group
                                        );
         }

      }
      else {
         $this->template = "common/error.tpl";
         $this->data['errorstring'] = $this->data['text_you_are_not_admin'];
      }


      $this->data['prev_page'] = $this->data['page'] - 1;
      $this->data['next_page'] = $this->data['page'] + 1;

      $this->data['total_pages'] = floor($this->data['total_users'] / $this->data['page_len']);


      $this->render();
   }


}

?>
