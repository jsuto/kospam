<?php

class ModelQuarantineUser extends Model {

   public function ShowUsers($dir = '') {
      $users = array();

      if($dir == ""){ return $users; }

      if(!chdir($dir)){ return $users; }


      if($dh = opendir($dir)){
         while(($file = readdir($dh)) != false){

            if($file != "." && $file != ".." && is_dir("$dir/$file")){

               if($dh2 = opendir("$dir/$file")){
                  while(($file2 = readdir($dh2)) !== false){
                     if($file2 != "." && $file2 != ".."){
                        $users[] = array('name' => $file2);
                     }
                  }
               }
               closedir($dh2);
            } 
         }
         closedir($dh);
      }

      return $users;
   }


}

?>
