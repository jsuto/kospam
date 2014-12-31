<?php  


class ControllerCommonMenu extends Controller {

   protected function index() {

      $this->id = "menu";
      $this->template = "common/menu.tpl";

      $db = Registry::get('db');
      $session = Registry::get('session');

      $this->data['admin_user'] = Registry::get('admin_user');
      $this->data['auditor_user'] = Registry::get('auditor_user');
      $this->data['readonly_admin'] = Registry::get('readonly_admin');

      $this->data['settings'] = array(
                                      'branding_text' => BRANDING_TEXT,
                                      'branding_url' => BRANDING_URL,
                                      'branding_logo' => BRANDING_LOGO,
                                      'support_link' => SUPPORT_LINK,
                                      'background_colour' => BRANDING_BACKGROUND_COLOUR,
                                      'text_colour' => BRANDING_TEXT_COLOUR
                                );

      $this->data['realname'] = $session->get('realname');

      $this->render();
   }


}



?>
