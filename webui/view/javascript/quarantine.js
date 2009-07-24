
function mark_all(x){
   var i; 
   var len = document.aaa1.elements.length;

   for(i=0; i<len; i++){
      document.aaa1.elements[i].checked = x;
   }

}


function fix_search(){
   location = "index.php?route=quarantine/quarantine&subj=" + document.forms.aaa0.subj.value + "&from=" + document.forms.aaa0.from.value + "&user=" + document.forms.aaa0.user.value;
}


function fix_form(){
   location = document.forms[0].action + "&" + document.forms[0].elements[0].name + "=" + document.forms[0].elements[0].value;
}

