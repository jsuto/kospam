<?php


class Controller {
   protected $id;
   protected $data = array();
   protected $children = array();
   protected $layout;
   protected $output;
   protected $template;


   public function __construct() {
      $language = Registry::get('language');
      $this->data = array_merge($this->data, $language->data);
   }


   protected function __get($key) {
      return Registry::get($key);
   }


   protected function __set($key, $value) {
      Registry::set($key, $value);
   }


   public function args($args = array()){
      while(list($key, $value) = each($args)) $this->data[$key] = $value;
   }


   protected function render(){

      foreach ($this->children as $child) {
         $file  = DIR_APPLICATION . $child . ".php";
         $class = 'Controller' . preg_replace('/[^a-zA-Z0-9]/', '', $child);

         if(file_exists($file)){
            require_once($file);

            $controller = new $class();

            $controller->index();

            $this->data[$controller->id] = $controller->output;
         }
         else {
            exit('Error: Could not load controller ' . $child . '!');
         }

      }




      $this->output = $this->fetch($this->template);


      if($this->layout){

          $file  = DIR_APPLICATION . $this->layout . '.php';
          $class = 'Controller' . preg_replace('/[^a-zA-Z0-9]/', '', $this->layout);

          if(file_exists($file)){
              require_once($file);

              $controller = new $class();

              $controller->data[$this->id] = $this->output;

              $controller->index();

              $this->output = $controller->output;

           }
           else {
              exit('Error: Could not load controller ' . $this->layout . '!');
           }

           print $this->output;
      }

   }


   protected function fetch(){
      $file = DIR_TEMPLATE . $this->template;
  
      if(file_exists($file)){

         extract($this->data);

         ob_start();
      
         include($file);
      
         $content = ob_get_contents();

         ob_end_clean();

         return $content;
      }
      else {
         exit('Error: Could not load template ' . $file . '!');
      }
   }


}

?>
