<?php

class ModelPolicyPolicy extends Model {

   public function getPolicies() {
      $query = $this->db->query("SELECT policy_group, name FROM " . TABLE_POLICY);

      return $query->rows;
   }


   public function getPolicy($policy_group = 0) {
      if(!is_numeric($policy_group) || $policy_group <= 0) {
         return array();
      }

      $query = $this->db->query("SELECT * FROM " . TABLE_POLICY . " WHERE policy_group=" . (int)$policy_group);

      return $query->row;
   }


   public function removePolicy($policy_group = 0) {
      if(!is_numeric($policy_group) || $policy_group <= 0) {
         return 0;
      }

      $query = $this->db->query("DELETE FROM " . TABLE_POLICY . " WHERE policy_group=" . (int)$policy_group);

      return $this->db->countAffected();
   }


   public function updatePolicy($policy) {

      if(count($policy) < 5 || $policy['policy_group'] <= 0 || !is_numeric($policy['policy_group'])){
         return 0;
      }

      $query = $this->db->query(
         "UPDATE " . TABLE_POLICY . " SET name='" . $this->db->escape($policy['name']) . "', deliver_infected_email=" . (int)$policy['deliver_infected_email'] .
         ", silently_discard_infected_email=" . (int)$policy['silently_discard_infected_email'] . ", use_antispam=" . (int)$policy['use_antispam'] .
         ", spam_subject_prefix='" . $this->db->escape($policy['spam_subject_prefix']) . "', enable_auto_white_list=" . (int)$policy['enable_auto_white_list'] .
         ", max_message_size_to_filter=" . (int)$policy['max_message_size_to_filter'] . ", rbl_domain='" . $this->db->escape($policy['rbl_domain']) .
         "', surbl_domain='" . $this->db->escape($policy['surbl_domain']) . "', spam_overall_limit=" . (double)$policy['spam_overall_limit'] .
         ", spaminess_oblivion_limit=" . (double)$policy['spaminess_oblivion_limit'] . ", replace_junk_characters=" . (int)$policy['replace_junk_characters'] .
         ", invalid_junk_limit=" . (int)$policy['invalid_junk_limit'] . ", invalid_junk_line=" . (int)$policy['invalid_junk_line'] . ", penalize_images=" .
         (int)$policy['penalize_images'] . ", penalize_embed_images=" . (int)$policy['penalize_embed_images'] . ", penalize_octet_stream=" .
         (int)$policy['penalize_octet_stream'] . ", training_mode=" . (int)$policy['training_mode'] . ", initial_1000_learning=" .
         (int)$policy['initial_1000_learning'] . ", store_metadata=" . (int)$policy['store_metadata'] . ", store_only_spam=" . (int)$policy['store_only_spam'] .
         ", message_from_a_zombie=" . (int)$policy['message_from_a_zombie'] .
         " WHERE policy_group=" . (int)$policy['policy_group']
      );

      return $this->db->countAffected();
   }


   public function addPolicy($policy) {
      if(count($policy) < 5 || $policy['policy_group'] <= 0 || !is_numeric($policy['policy_group'])){
         return 0;
      }

      $query = $this->db->query(
         "INSERT INTO " . TABLE_POLICY . " (policy_group, name, deliver_infected_email, silently_discard_infected_email, use_antispam, " .
         "spam_subject_prefix, enable_auto_white_list, max_message_size_to_filter, rbl_domain, surbl_domain, spam_overall_limit, spaminess_oblivion_limit, " .
         "replace_junk_characters, invalid_junk_limit, invalid_junk_line, penalize_images, penalize_embed_images, penalize_octet_stream, training_mode, " .
         "initial_1000_learning, store_metadata, store_only_spam) VALUES (" .

         (int)$policy['policy_group'] . ",'" . $this->db->escape($policy['name']) . "'," . (int)$policy['deliver_infected_email'] . "," . (int)$policy['silently_discard_infected_email'] . "," .
         (int)$policy['use_antispam'] . ",'" . $this->db->escape($policy['spam_subject_prefix']) . "'," . (int)$policy['enable_auto_white_list']  . "," .
         (int)$policy['max_message_size_to_filter'] . ",'" . $this->db->escape($policy['rbl_domain']) . "','" . $this->db->escape($policy['surbl_domain']) . "'," .
         (double)$policy['spam_overall_limit'] . "," . (double)$policy['spaminess_oblivion_limit'] . "," . (int)$policy['replace_junk_characters'] . "," . (int)$policy['invalid_junk_limit'] . "," .
         (int)$policy['invalid_junk_line'] . "," . (int)$policy['penalize_images'] . "," . (int)$policy['penalize_embed_images'] . "," . (int)$policy['penalize_octet_stream'] . "," .
         (int)$policy['training_mode'] . "," . (int)$policy['initial_1000_learning'] . "," . (int)$policy['store_metadata'] . "," . (int)$policy['store_only_spam'] .
         ", message_from_a_zombie=" . (int)$policy['message_from_a_zombie'] .
         ")"
      );


      return $this->db->countAffected();
   }


   public function getNewPolicyGroupId() {

      $query = $this->db->query("SELECT MAX(policy_group) AS last_id FROM " . TABLE_POLICY);

      if(isset($query->row['last_id']) && $query->row['last_id'] > 0) {
         return (int)$query->row['last_id'] + 1;
      }

      return 1;
   }


}

?>
