<?php

class ModelPolicyPolicy extends Model {

   public function getPolicies() {
      $data = array();

      $query = $this->db->ldap_query(LDAP_POLICY_BASEDN, "policyGroup=*", array("policygroup", "policyname") );

      foreach ($query->rows as $result) {
         $data[] = array(
                          'name'         => $result['policyname'],
                          'policy_group' => $result['policygroup']                          
                        );

      }

      return $data;
   }


   public function getPolicy($policy_group = 0) {
      if(!is_numeric($policy_group) || $policy_group <= 0) {
         return array();
      }

      $query = $this->db->ldap_query(LDAP_POLICY_BASEDN, "policyGroup=$policy_group", array() );

      $result = $query->row;

      $data = array(
                       'name'                            => $result['policyname'],
                       'policy_group'                    => $result['policygroup'],
                       'deliver_infected_email'          => $result['deliverinfectedemail'],
                       'silently_discard_infected_email' => $result['silentlydiscardinfectedemail'],
                       'use_antispam'                    => $result['useantispam'],
                       'spam_subject_prefix'             => $result['spamsubjectprefix'],
                       'enable_auto_white_list'          => $result['enableautowhitelist'],
                       'max_message_size_to_filter'      => $result['maxmessagesizetofilter'],
                       'rbl_domain'                      => $result['rbldomain'],
                       'surbl_domain'                    => $result['surbldomain'],
                       'spam_overall_limit'              => $result['spamoveralllimit'],
                       'spaminess_oblivion_limit'        => $result['spaminessoblivionlimit'],
                       'replace_junk_characters'         => $result['replacejunkcharacters'],
                       'invalid_junk_limit'              => $result['invalidjunklimit'],
                       'invalid_junk_line'               => $result['invalidjunkline'],
                       'penalize_images'                 => $result['penalizeimages'],
                       'penalize_embed_images'           => $result['penalizeembedimages'],
                       'penalize_octet_stream'           => $result['penalizeoctetstream'],
                       'training_mode'                   => $result['trainingmode'],
                       'initial_1000_learning'           => $result['initial1000learning'],
                       'store_metadata'                  => $result['storemetadata'],
                       'store_only_spam'                 => $result['storeonlyspam']
                     );



      return $data;
   }


   public function removePolicy($policy_group = 0) {
      if(!is_numeric($policy_group) || $policy_group <= 0) {
         return 0;
      }

      if($this->db->ldap_delete("policyGroup=$policy_group," . LDAP_POLICY_BASEDN) == TRUE) {
         return 1;
      }

      return 0;
   }


   public function updatePolicy($policy) {
      if(count($policy) < 5 || $policy['policy_group'] <= 0 || !is_numeric($policy['policy_group'])){
         return 0;
      }

      $entry = array();

      $entry["policyname"] = $policy['name'];
      $entry["deliverinfectedemail"] = (int)$policy['deliver_infected_email'];
      $entry["silentlydiscardinfectedemail"] = (int)$policy['silently_discard_infected_email'];
      $entry["useantispam"] = (int)$policy['use_antispam'];
      $entry["spamsubjectprefix"] = $policy['spam_subject_prefix'];
      $entry["enableautowhitelist"] = (int)$policy['enable_auto_white_list'];
      $entry["maxmessagesizetofilter"] = (int)$policy['max_message_size_to_filter'];
      $entry["rbldomain"] = $policy['rbl_domain'];
      $entry["surbldomain"] = $policy['surbl_domain'];
      $entry["spamoveralllimit"] = (double)$policy['spam_overall_limit'];
      $entry["spaminessoblivionlimit"] = (double)$policy['spaminess_oblivion_limit'];
      $entry["replacejunkcharacters"] = (int)$policy['replace_junk_characters'];
      $entry["invalidjunklimit"] = (int)$policy['invalid_junk_limit'];
      $entry["invalidjunkline"] = (int)$policy['invalid_junk_line'];
      $entry["penalizeimages"] = (int)$policy['penalize_images'];
      $entry["penalizeembedimages"] = (int)$policy['penalize_embed_images'];
      $entry["penalizeoctetstream"] = (int)$policy['penalize_octet_stream'];
      $entry["trainingmode"] = (int)$policy['training_mode'];
      $entry["initial1000learning"] = (int)$policy['initial_1000_learning'];
      $entry["storemetadata"] = (int)$policy['store_metadata'];
      $entry["storeonlyspam"] = (int)$policy['store_only_spam'];

      if($this->db->ldap_modify("policyGroup=" . (int)$policy['policy_group'] . "," . LDAP_POLICY_BASEDN, $entry) == TRUE) {
         return 1;
      }

      return 0;
   }


   public function addPolicy($policy) {
      if(count($policy) < 5 || $policy['policy_group'] <= 0 || !is_numeric($policy['policy_group'])){
         return 0;
      }

      $entry["objectClass"] = "clapfPolicyGroup";
      $entry["policygroup"] = (int)$policy['policy_group'];
      $entry["policyname"] = $policy['name'];
      $entry["deliverinfectedemail"] = (int)$policy['deliver_infected_email'];
      $entry["silentlydiscardinfectedemail"] = (int)$policy['silently_discard_infected_email'];
      $entry["useantispam"] = (int)$policy['use_antispam'];
      $entry["spamsubjectprefix"] = $policy['spam_subject_prefix'];
      $entry["enableautowhitelist"] = (int)$policy['enable_auto_white_list'];
      $entry["maxmessagesizetofilter"] = (int)$policy['max_message_size_to_filter'];
      $entry["rbldomain"] = $policy['rbl_domain'];
      $entry["surbldomain"] = $policy['surbl_domain'];
      $entry["spamoveralllimit"] = (double)$policy['spam_overall_limit'];
      $entry["spaminessoblivionlimit"] = (double)$policy['spaminess_oblivion_limit'];
      $entry["replacejunkcharacters"] = (int)$policy['replace_junk_characters'];
      $entry["invalidjunklimit"] = (int)$policy['invalid_junk_limit'];
      $entry["invalidjunkline"] = (int)$policy['invalid_junk_line'];
      $entry["penalizeimages"] = (int)$policy['penalize_images'];
      $entry["penalizeembedimages"] = (int)$policy['penalize_embed_images'];
      $entry["penalizeoctetstream"] = (int)$policy['penalize_octet_stream'];
      $entry["trainingmode"] = (int)$policy['training_mode'];
      $entry["initial1000learning"] = (int)$policy['initial_1000_learning'];
      $entry["storemetadata"] = (int)$policy['store_metadata'];
      $entry["storeonlyspam"] = (int)$policy['store_only_spam'];

      if($this->db->ldap_add("policyGroup=" . (int)$policy['policy_group'] . "," . LDAP_POLICY_BASEDN, $entry) == TRUE) {
         return 1;
      }

      return 0;
   }


   public function getNewPolicyGroupId() {
      $x = 0;

      $query = $this->db->ldap_query(LDAP_POLICY_BASEDN, "policyGroup=*", array("policygroup") );

      foreach ($query->rows as $result) {
         if($result['policygroup'] > $x){
            $x = $result['policygroup'];
         }
      }

      return $x + 1;
   }


}

?>
