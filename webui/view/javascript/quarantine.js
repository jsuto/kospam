
function mark_all(x){
   var i; 
   var len = document.aaa1.elements.length;

   for(i=0; i<len; i++){
      if(document.aaa1.elements[i].name.substr(0,2) == "s." || document.aaa1.elements[i].name.substr(0,2) == "h.") {
         document.aaa1.elements[i].checked = x;
      }
   }

   highlight_table(true);
}


function highlight_table(action) {

   var table = document.getElementById("thetable");
   var rows = table.getElementsByTagName("tr");
   for(i = 2; i < rows.length; i++){
      if(action == true)
         rows[i].className = "highlight";
      else
         rows[i].className = "";
   }
}


function toggle_row_highlight(what) {
   var elem = document.getElementById(what);

   if(elem.className == "highlight")
      elem.className = "";
   else
      elem.className = "highlight";
}


function fix_search(){
   location = "index.php?route=quarantine/quarantine&subj=" + document.forms.aaa0.subj.value + 
         "&from=" + document.forms.aaa0.from.value +
           "&hamspam=" + document.forms.aaa0.hamspam.value +
           "&user=" + document.forms.aaa0.user.value;
}


function fix_form(){
   var a = "";

   for(i=0; i<document.forms[0].elements.length; i++){
      if(document.forms[0].elements[i].name) {
         a = a + "&" + document.forms[0].elements[i].name + "=" + document.forms[0].elements[i].value;
      }
   }

   location = document.forms[0].action + a;
}


function fix_mass_action(subj, from, hamspam, username, page) {

   var query_params = "&subj=" + document.forms.aaa0.subj.value + 
                      "&from=" + document.forms.aaa0.from.value +
                      "&hamspam=" + document.forms.aaa0.hamspam.value +
                      //"&user=" + username +
                      "&page=" + page;

   if(document.forms.aaa1.massaction.value == "purge") document.forms.aaa1.action="index.php?route=quarantine/remove&topurge=1" + query_params;
   if(document.forms.aaa1.massaction.value == "purgeeverything") document.forms.aaa1.action="index.php?route=quarantine/remove&purgeallfromqueue=1" + query_params;
   if(document.forms.aaa1.massaction.value == "deliver") document.forms.aaa1.action="index.php?route=quarantine/massdeliver" + query_params;
   if(document.forms.aaa1.massaction.value == "train") document.forms.aaa1.action="index.php?route=quarantine/masstrain&nodeliver=1" + query_params;
   if(document.forms.aaa1.massaction.value == "train+deliver") document.forms.aaa1.action="index.php?route=quarantine/masstrain" + query_params;

   if(document.forms.aaa1.massaction.value != "-") document.forms.aaa1.submit();
}


function fix_search_terms(user) {
   if(document.forms.aaa0.searchterm.value != "") document.location.href = document.forms.aaa0.searchterm.value;
   else document.location.href = "index.php?route=quarantine/quarantine&user=" + user;
}


function setCookie(c_name, value, expiredays){
   var exdate = new Date();

   exdate.setDate(exdate.getDate() + expiredays);

   document.cookie = c_name + "=" + escape(value) + ((expiredays == null) ? "" : ";expires=" + exdate.toGMTString());
}


function filterhistory(){
   var x;

   x = document.forms[0].hamspam.value;
   setCookie("hamspam", x, 1);

   x = document.forms[0].sender_domain.value;
   setCookie("sender_domain", x, 1);

   x = document.forms[0].rcpt_domain.value;
   setCookie("rcpt_domain", x, 1);

   x = document.forms[0].date1.value;
   setCookie("date1", x, 1);

   x = document.forms[0].date2.value;
   setCookie("date2", x, 1);
}


function ham_or_spam(){
   var x = document.forms[0].hamspam.value;
   setCookie("hamspam", x, 1);
}

