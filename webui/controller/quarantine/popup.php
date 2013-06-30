<?php


class ControllerQuarantinePopup extends Controller {

   public function index(){

      $this->id = "popup";
      $this->template = "quarantine/popup.tpl";

      $this->render();
   }


}

?>
