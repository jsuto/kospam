<?php

class ModelStatCounter extends Model {

   public function getCounters(){
      $counter = array();

      if(MEMCACHED_ENABLED) {
         $memcache = Registry::get('memcache');

         $counter = $memcache->get(Registry::get('counters'));

         if(isset($counter['_c:counters_last_update'])) { return $counter; }
      }

      $query = $this->db->query("SELECT * FROM " . TABLE_COUNTERS);

      if($query->num_rows == 1) {
         $counter = $query->row;
      }

      return $counter;
   }


   public function resetCounters(){

      if(MEMCACHED_ENABLED) {
         $memcache = Registry::get('memcache');

         foreach (Registry::get('counters') as $counter) {
            $memcache->set($counter, 0);
         }
      }

      $query = $this->db->query("UPDATE " . TABLE_COUNTERS . " set rcvd=0, mynetwork=0, ham=0, spam=0, possible_spam=0, unsure=0, minefield=0, virus=0, zombie=0, fp=0, fn=0");

      return 0;
   }

}

?>
