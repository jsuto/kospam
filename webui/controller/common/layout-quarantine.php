<?php  

class ControllerCommonLayoutQuarantine extends Controller {

      protected function index() {


         $this->data['title'] = $this->document->title;

         $this->template = "common/layout-quarantine.tpl";


         $this->children = array(
                      "common/menu",
                      "quarantine/popup",
                      "common/footer"
         );

         $this->render();

      }


}


?>
