
<div id="messagelistcontainer" class="boxlistcontent">

   <div id="results">

      <div class="resultrow">
<?php if(count($terms) > 0){ ?>

<?php } else if(count($terms) == 0) { ?>
    <div class="alert">
       <strong><?php print $text_empty_search_result; ?></strong>
    </div>
<?php } ?>
      </div>

<?php foreach($terms as $term) {
         parse_str($term['term'], $s);
?>
         <div class="resultrow center">
            <a href="#" onclick="Clapf.load_search_results_for_saved_query('<?php print urldecode($term['term']); ?>');"><?php print implode(", ", $s); ?></a></br />
         </div>
<?php } ?>

   </div>

</div>


