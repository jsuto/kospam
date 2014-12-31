<?php


class ModelPolicyPolicy extends Model {

   public function get_policies($s = '') {
      if($s) {
         $query = $this->db->query("SELECT * FROM " . TABLE_POLICY . " WHERE `from` LIKE ? OR `to` LIKE ? OR subject LIKE ? ORDER BY domain, id", array('%' . $s . '%', '%' . $s . '%', '%' . $s . '%'));
      } else {
         $query = $this->db->query("SELECT * FROM " . TABLE_POLICY . " ORDER BY id");
      }

      if(isset($query->rows)) { return $query->rows; }

      return array();
   }


   public function get_policy($id = 0) {
      $query = $this->db->query("SELECT * FROM " . TABLE_POLICY . " WHERE id=?", array($id));

      if(isset($query->row)) { return $query->row; }

      return array();
   }


   public function add($data = array()) {
      $query = $this->db->query("INSERT INTO " . TABLE_POLICY . " (`name`,`deliver_infected_email`,`silently_discard_infected_email`,`use_antispam`,`spam_subject_prefix`,`max_message_size_to_filter`,`surbl_domain`,`spam_overall_limit`,`spaminess_oblivion_limit`,`replace_junk_characters`, `penalize_images`, `penalize_embed_images`, `penalize_octet_stream`, `training_mode`, `store_emails`, `store_only_spam`, `message_from_a_zombie`, `smtp_addr`, `smtp_port`) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)", array($data['name'], $data['deliver_infected_email'], $data['silently_discard_infected_email'], $data['use_antispam'], $data['spam_subject_prefix'], $data['max_message_size_to_filter'], $data['surbl_domain'], $data['spam_overall_limit'], $data['spaminess_oblivion_limit'], $data['replace_junk_characters'], $data['penalize_images'], $data['penalize_embed_images'], $data['penalize_octet_stream'], $data['training_mode'], $data['store_emails'], $data['store_only_spam'], $data['message_from_a_zombie'], $data['smtp_addr'], $data['smtp_port']));

      return $this->db->countAffected();
   }


   public function update($data = array()) {

      if(count($data) < 5 || $data['id'] <= 0 || !is_numeric($data['id'])){
         return 0;
      }

      $query = $this->db->query(
         "UPDATE " . TABLE_POLICY . " SET `name`=?,`deliver_infected_email`=?,`silently_discard_infected_email`=?,`use_antispam`=?,`spam_subject_prefix`=?,`max_message_size_to_filter`=?,`surbl_domain`=?,`spam_overall_limit`=?,`spaminess_oblivion_limit`=?,`replace_junk_characters`=?, `penalize_images`=?, `penalize_embed_images`=?, `penalize_octet_stream`=?, `training_mode`=?, `store_emails`=?, `store_only_spam`=?, `message_from_a_zombie`=?, `smtp_addr`=?, `smtp_port`=? WHERE id=?", array($data['name'], $data['deliver_infected_email'], $data['silently_discard_infected_email'], $data['use_antispam'], $data['spam_subject_prefix'], $data['max_message_size_to_filter'], $data['surbl_domain'], $data['spam_overall_limit'], $data['spaminess_oblivion_limit'], $data['replace_junk_characters'], $data['penalize_images'], $data['penalize_embed_images'], $data['penalize_octet_stream'], $data['training_mode'], $data['store_emails'], $data['store_only_spam'], $data['message_from_a_zombie'], $data['smtp_addr'], $data['smtp_port'], $data['id']));

      $rc = $this->db->countAffected();

      LOGGER("update policy: " . $data['name'] . " (id=" . (int)$data['id'] . ") (rc=$rc)");

      return $rc;
   }

   public function remove($id = 0) {
      $query = $this->db->query("DELETE FROM " .  TABLE_POLICY . " WHERE id=?", array($id));
      return $this->db->countAffected();
   }


   public function fix_post_data() {

      foreach(array(
                     'deliver_infected_email',
                     'silently_discard_infected_email',
                     'use_antispam',
                     'replace_junk_characters',
                     'penalize_images',
                     'penalize_embed_images',
                     'penalize_octet_stream',
                     'store_emails',
                     'store_only_spam'
               )
               as $a) {

         if(!isset($this->request->post[$a])) { $this->request->post[$a] = 0; }
         else { $this->request->post[$a] = 1; }
      }

   }


}

?>
