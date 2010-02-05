<?php

class ModelUserPrefs extends Model {

   public function getUserPrefs($db_session, $username = '') {
      if($username == "") { return 0; }

      $query = $db_session->query("select * from prefs where username='" . $db_session->escape($username) . "'");

      if(isset($query->row['pagelen'])) { $_SESSION['pagelen'] = $query->row['pagelen']; }
      if(isset($query->row['lang'])) { $_SESSION['lang'] = $query->row['lang']; }

      return 1;
   }


   public function setUserPrefs($db_session, $username = '', $prefs = array() ) {

      if(!isset($prefs['pagelen']) || !is_numeric($prefs['pagelen']) || $prefs['pagelen'] < 10 || $prefs['pagelen'] > 100
         || !isset($prefs['lang']) || strlen($prefs['lang']) != 2 || !file_exists(DIR_LANGUAGE . $prefs['lang']) ) { return 1; }


      $query = $db_session->query("select count(*) as num from prefs where username='" . $db_session->escape($username) . "'");

      if((int)@$query->row['num'] == 1) {
         $query = $db_session->query("update prefs set lang='" . $db_session->escape($prefs['lang']) . "', pagelen=" . (int)@$prefs['pagelen'] . " where username='" . $db_session->escape($username) . "'");
      }
      else {
         $query = $db_session->query("insert into prefs (username, pagelen, lang) values('" . $db_session->escape($username) . "', " . (int)@$prefs['pagelen'] . ", '" . $db_session->escape($prefs['lang']) . "')");
      }


      $_SESSION['pagelen'] = $prefs['pagelen'];
      $_SESSION['lang'] = $prefs['lang'];


      return 1;
   }

}

?>

