
function mark_all(x){
   var i; 
   var len = document.aaa1.elements.length;

   for(i=0; i<len; i++){
      document.aaa1.elements[i].checked = x;
   }

}


function fix_search(){
   location = "index.php?route=quarantine/quarantine&subj=" + document.forms.aaa0.subj.value + 
         "&from=" + document.forms.aaa0.from.value + "&user=" + document.forms.aaa0.user.value +
           "&hamspam=" + document.forms.aaa0.hamspam.value;
}


function fix_form(){
   var a = "";

   for(i=0; i<document.forms[0].elements.length; i+=2){
      a = a + "&" + document.forms[0].elements[i].name + "=" + document.forms[0].elements[i].value;
   }

   location = document.forms[0].action + a;
}


function fix_mass_action(username) {

   if(document.forms.aaa1.massaction.value == "purge") document.forms.aaa1.action="index.php?route=quarantine/remove&topurge=1&user=" + username;
   if(document.forms.aaa1.massaction.value == "purgeeverything") document.forms.aaa1.action="index.php?route=quarantine/remove&purgeallfromqueue=1&user=" + username;
   if(document.forms.aaa1.massaction.value == "deliver") document.forms.aaa1.action="index.php?route=quarantine/massdeliver&user=" + username;
   if(document.forms.aaa1.massaction.value == "train") document.forms.aaa1.action="index.php?route=quarantine/masstrain&nodeliver=1&user=" + username;
   if(document.forms.aaa1.massaction.value == "train+deliver") document.forms.aaa1.action="index.php?route=quarantine/masstrain&user=" + username;

   if(document.forms.aaa1.massaction.value != "-") document.forms.aaa1.submit();
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
}


function ham_or_spam(){
   var x = document.forms[0].hamspam.value;
   setCookie("hamspam", x, 1);
}


