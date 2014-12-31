<?php  

class ControllerCommonLayoutHistory extends Controller {

      protected function index() {


         $this->data['title'] = $this->document->title;

         $this->template = "common/layout-history.tpl";

         $this->data['search_args'] = '';

         $this->data['open_saved_search_box'] = 0;


         $this->children = array(
                      "common/menu",
                      "common/footer"
         );

         $this->render();

      }


}


?>
