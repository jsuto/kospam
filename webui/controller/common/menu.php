<?php  


class ControllerCommonMenu extends Controller {

   protected function index() {

      $this->id = "menu";
      $this->template = "common/menu.tpl";


      $db = Registry::get('db');

      $this->data['admin_user'] = Registry::get('admin_user');

      $this->render();


   }


}



?>
