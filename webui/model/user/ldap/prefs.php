<?php

class ModelUserPrefs extends Model {

   public function getUserPrefs($db_session, $username = '') {
      if($username == "") { return 0; }

      $query = $db_session->query("select * from prefs where username='" . $db_session->escape($username) . "'");

      if(isset($query->row['pagelen'])) { $_SESSION['pagelen'] = $query->row['pagelen']; }
      if(isset($query->row['lang'])) { $_SESSION['lang'] = $query->row['lang']; }
      if(isset($query->row['train_global'])) { $_SESSION['train_global'] = $query->row['train_global']; }

      return 1;
   }


   public function setUserPrefs($db_session, $username = '', $prefs = array() ) {

      if(!isset($prefs['pagelen']) || !is_numeric($prefs['pagelen']) || $prefs['pagelen'] < 10 || $prefs['pagelen'] > 100
         || !isset($prefs['lang']) || strlen($prefs['lang']) != 2 || !file_exists(DIR_LANGUAGE . $prefs['lang']) ) { return 1; }

      if(Registry::get('admin_user') == 0) { $prefs['global_train'] = 0; }

      $query = $db_session->query("select count(*) as num from prefs where username='" . $db_session->escape($username) . "'");

      if((int)@$query->row['num'] == 1) {
         $query = $db_session->query("update prefs set lang='" . $db_session->escape($prefs['lang']) . "', pagelen=" . (int)@$prefs['pagelen'] . ", train_global=" . (int)@$prefs['global_train'] . " where username='" . $db_session->escape($username) . "'");
      }
      else {
         $query = $db_session->query("insert into prefs (username, pagelen, lang, train_global) values('" . $db_session->escape($username) . "', " . (int)@$prefs['pagelen'] . ", '" . $db_session->escape($prefs['lang']) . "', " . (int)@$prefs['global_train'] . ")");
      }


      $_SESSION['pagelen'] = $prefs['pagelen'];
      $_SESSION['lang'] = $prefs['lang'];
      $_SESSION['train_global'] = (int)@$prefs['global_train'];

      return 1;
   }

}

?>
