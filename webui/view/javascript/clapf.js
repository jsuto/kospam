
var Clapf = 
{

   search: '',

   current_message_id: 0,
   pos: -1,
   health_refresh: 300,
   history_refresh: 300,

   Shared: {
        page:0,
        sort:1,
        order:'date',
        type: 'quarantine'
   },

   Searches:{},

   Messages: [],

   Search_strings: [],

   log: function()
   {
      if(window.console)
      {
         var 
            a = arguments,
            b = +new Date,
            c = window.console;

            if(!a.length)
               c.clear();
            else if(a.length > 1)
               c.log(b, a[0], [].slice.call(arguments, 1));
            else
               c.log(b, a[0]);                
      }
   },


   /*
    * Change the list order
    * 
    *    HTML: <a class="xxxx" onclick="Clapf.changeOrder(this)" xid="date" xorder="0"></a>
    */

   changeOrder: function(a)
   {
      a = $( a );// a == DOM element
      // a = Clapf.getSource( a );// a == Javascript event

      Clapf.Shared.sort = a.attr('xid');// {String} (date|from|subject|size)
      Clapf.Shared.order = a.attr('xorder');// {Number} (0|1) -> (ASC|DESC)

      Clapf.log("[changeOrder]", Clapf.Shared.sort, Clapf.Shared.order);

      Clapf.load_search_results();
   },


   /*
    * show a temporary message to the user
    */

   show_message: function(id, msg, timeout)
   {
      msg = '<p>' + msg.replace("\n", "<br />") + '</p>';

      Clapf.log("[show_message]", id, msg);

      $('#'+id).html(msg);
      $('#'+id).show();
      setTimeout(function() { $('#'+id).hide(); }, timeout*1000);
   },


   fill_current_messages_array: function() 
   {
      Clapf.log("[fill_current_messages_array]"); 
        
      var z = $('#thetable tbody').children(), y = z.length, x;
      var u = [];

      Clapf.log("[fill_current_messages_array] y", y ); 

      for(i=0; i<y; i++) {
         x = z[i];

         if(x.nodeName == "TR" && x.id.charAt(2) == '_' ) {
            Clapf.log("[fill_current_messages_array], pos/id", i, x.id);
            Clapf.Messages[i] = x.id.substring(3, 1000);
         }
      }

   },


   get_selected_messages: function()
   {
      var selected = '';

      for(i=0; i<Clapf.Messages.length; i++) {
         e = document.getElementById(Clapf.Messages[i]);
         if(e && e.checked) {
            if(selected) selected += ' ' + Clapf.Messages[i];
            else selected = Clapf.Messages[i];

            Clapf.log("[checked]", Clapf.Messages[i]);
         }
      }

      return selected;
   },


   toggle_bulk_check: function()
   {
      var e;

      Clapf.log("[toggle_bulk_check]", Clapf.Messages.length);

      if($('#bulkcheck').prop('checked')) {
         bulkcheck = true; classname = 'info';
      } else {
         bulkcheck = false; classname = '';
      }

      for(i=0; i<Clapf.Messages.length; i++) {

         Clapf.log("[toggle], setting:", Clapf.Messages[i]);

         e = document.getElementById(Clapf.Messages[i]);
         if(e) e.checked = bulkcheck;

         e = document.getElementById('tr_' + Clapf.Messages[i]);
         if(e) e.className = classname;
      }


   },


   toggle_row_highlight: function(id) {
      var elem = document.getElementById(id);

      var chbox = id.replace("tr_", ""); 
      var chboxid = document.getElementById(chbox);

      if(elem.className == "info") {
         elem.className = "";
         chboxid.checked = 0;
      }
      else {
         elem.className = "info";
         chboxid.checked = 1;
      }
   },


   fix_quarantine_search: function() {
      var z = $('div#searchpopup1');

      var newloc = encodeURI("index.php?route=quarantine/quarantine&subj=" + $('input#subject', z).val().trim() + "&from=" + $('input#from', z).val().trim() + "&to=" + $('input#to', z).val().trim() + "&date=" + $('input#date', z).val().trim() + "&search=" + $('#search').val().trim() + "&hamspam=" + $('#hamspam', z).val().trim());

      Clapf.log("[fix_quarantine_search]", newloc);

      document.location.href = newloc;
   },


   fix_search_terms: function(user) {
      var newloc = $('#searchterm').val().trim();

      if(newloc == '') newloc = "index.php?route=quarantine/quarantine&user=" + user;

      document.location.href = newloc;
   },


   load_search_results: function() {
      var url;

      Clapf.Shared.type == 'quarantine' ? url = 'index.php?route=quarantine/helper' : url = '/audit-helper.php';

      Clapf.log("[load_search_results]", url);

      Clapf.spinner('start');

      jQuery.ajax( url, {
         data: $.extend(!0, {}, Clapf.Shared, Clapf.Searches[Clapf.search]),
         type: "POST"
      })
      .done( function(a) {
         $('#mailcontframe').html(a);
         Clapf.fill_current_messages_array();
         Clapf.spinner('stop');
         $('#resultsheader').show();
      })
      .fail(function(a, b) { alert("Problem retrieving XML data:" + b) });
   },


   change_quarantine_user: function(user, hamspam) {
      Clapf.log("[change_quarantine_user]");

      var s = 'to: ' + user;

      if(hamspam) { s += ', hamspam: ' + hamspam; }

      $('#search').val(s);

      Clapf.expert(); 
   },


   spinner: function(cmd) {
      Clapf.log("[spinner]", cmd);

      if(cmd == 'start') {
         $('#sspinner').show();
         $('#messagelistcontainer').hide();
      }

      if(cmd == 'stop') {
         $('#sspinner').hide();
         $('#messagelistcontainer').show();
      }
   },


   load_saved_search_terms: function()
   {
      Clapf.log("[load_saved_search_terms]");

      jQuery.ajax('index.php?route=quarantine/load', {})
      .done( function(a) {
         $('#mailcontframe').html(a);
      })
      .fail(function(a, b) { alert("Problem retrieving XML data:" + b) });
   },


   load_search_results_for_saved_query: function(terms)
   {
      Clapf.log("[load_search_results_for_saved_query]", terms);
      var s = '';

      var pairs = terms.split('&');
      $.each(pairs, function(i, v){
         var pair = v.split('=');
         if(s) s += ',';
         s += ' ' + pair[0] + ': ' + pair[1];
      });

      Clapf.log("[load_search_results_for_saved_query]", s);

      $("input#search").val(s);

      $('#search').css('color', 'black');

      Clapf.expert();
   },


   view_message_by_pos: function(pos)
   {
      Clapf.log("[view_message_by_pos]", pos, Clapf.Messages[pos]);

      if(pos == -1) return false;

      var a = Clapf.Messages[pos];

      var pair = a.split('+');

      Clapf.pos = pos;

      Clapf.view_message(pair[0], pair[1]);
   },


   view_message: function(id, user)
   {
      Clapf.log("[view_message]", id, user);

      jQuery.ajax('index.php?route=quarantine/message&id=' + id + '&user=' + user, { cache: true })
      .done( function(a) {
         $('#mailpreviewframe').html(a);
      })
      .fail(function(a, b) { alert("Problem retrieving XML data:" + b) });
   },


   expert: function()
   {
      Clapf.log("[expert]");

      Clapf.search = 'Expert';

      Clapf.Shared.page = 0;
      Clapf.Shared.type = 'quarantine';

      var s = $.trim($('input#search').val());

      Clapf.Searches.Expert = {
                                 search : s,
                                 searchtype : 'expert'
      }

      if(s) {
         var found = 0;

         for(i=0; i<Clapf.Search_strings.length; i++) {
            if(Clapf.Search_strings[i] == s) {
               found = 1;
               break;
            }
         }

         if(found == 0) {
            Clapf.Search_strings[Clapf.Search_strings.length] = s;
            Clapf.log("[ss]", Clapf.Search_strings.length, s);
         }

      }

      Clapf.load_search_results();
   },


   complex: function()
   {
      Clapf.log("[complex]");

      Clapf.search = 'Complex';

      Clapf.Shared.page = 0;
      Clapf.Shared.type = 'quarantine';


      var z = $('div#searchpopup1');

      Clapf.Searches.Complex = {
         from : $('input#xfrom', z).val().trim(),
         to : $('input#xto', z).val().trim(),
         subject : $('input#xsubject', z).val().trim(),
         date : $('input#date', z).val().trim(),
         searchtype : 'simple'
      }

      Clapf.load_search_results();

      $('#searchpopup1').hide();
   },


   filter_history: function()
   {
      Clapf.log("[filter_history]");

      Clapf.search = 'History';

      Clapf.Shared.page = 0;
      Clapf.Shared.type = 'history';

      Clapf.Searches.History = {
         sender_domain : $.trim($('input#sender_domain').val()),
         rcpt_domain : $.trim($('input#rcpt_domain').val()),
         date1 : $.trim($('input#date1').val()),
         date2 : $.trim($('input#date2').val()),
         subject : $.trim($('input#subject').val()),
         hamspam : $.trim($('select#hamspam').val()),
         searchtype : 'history'
      }

      Clapf.load_history();
   },


   cancel: function()
   {
      Clapf.log("[cancel]");

        $('#search').val(''); 

        Clapf.Searches.Expert = {};

        $('input#xfrom').val('');
        $('input#xto').val('');
        $('input#xsubject').val('');
        $('input#date').val('');
        $('input#xhamspam').val('');

        Clapf.Searches.Complex = {};

        if(Clapf.Search_strings.length > 1) {
           var s = Clapf.Search_strings[Clapf.Search_strings.length - 2];
           Clapf.log("[ss prev]", Clapf.Search_strings[Clapf.Search_strings.length - 2]);
           $('input#search').val(s);
           Clapf.expert();
           Clapf.Search_strings.length--;
        }

   },


   navigation: function(a)
   {
      Clapf.log("[navigation]");

      Clapf.Shared.page = a;

      Clapf.load_search_results();
   },


   history_navigation: function(a)
   {
      Clapf.log("[history_navigation]");

      Clapf.Shared.page = a;

      Clapf.load_history();
   },


   remove_message: function(user, id)
   {
      jQuery.ajax('index.php?route=quarantine/remove&id=' + id + '&user=' + user, { cache: true })
      .done( function(a) {
         $('#mailpreviewframe').html(a);
         Clapf.load_search_results();
      })
      .fail(function(a, b) { alert("Problem retrieving XML data:" + b) });
   },


   remove_messages: function()
   {
      Clapf.log("[remove_messages]");

      var msg = Clapf.get_selected_messages();

      if(msg == '') { return; }

      jQuery.ajax('index.php?route=quarantine/remove', {
         data: $.extend(!0, { }, {ids: msg, topurge: '1'} ),
         type: "POST"
      })
      .done (function(a) {
         $('#mailpreviewframe').html(a);
         Clapf.load_search_results();
      })
      .fail(function(a, b) { alert("Problem retrieving XML data:" + b) });
   },


   remove_all_messages: function()
   {
      Clapf.log("[remove_all_messages]");

      jQuery.ajax('index.php?route=quarantine/remove&purgeallfromqueue=1', { cache: true })
      .done( function(a) {
         $('#mailpreviewframe').html(a);
         Clapf.load_search_results();
      })
      .fail(function(a, b) { alert("Problem retrieving XML data:" + b) });
   },


   deliver_message: function(user, id)
   {
      jQuery.ajax('index.php?route=quarantine/deliver&id=' + id + '&user=' + user, { cache: true })
      .done( function(a) {
         $('#mailpreviewframe').html(a);
         Clapf.load_search_results();
      })
      .fail(function(a, b) { alert("Problem retrieving XML data:" + b) });
   },


   deliver_messages: function()
   {
      Clapf.log("[deliver_messages]");

      var msg = Clapf.get_selected_messages();

      if(msg == '') { return; }

      jQuery.ajax('index.php?route=quarantine/deliver', {
         data: $.extend(!0, { }, {ids: msg} ),
         type: "POST"
      })
      .done (function(a) {
         $('#mailpreviewframe').html(a);
         Clapf.load_search_results();
      })
      .fail(function(a, b) { alert("Problem retrieving XML data:" + b) });
   },


   load_health: function()
   {
      Clapf.log("[load_health]");

      document.body.style.cursor = 'wait';

      jQuery.ajax('index.php?route=health/worker', { })
      .done( function(a) {
         $('#A1').html(a);
         document.body.style.cursor = 'default';
      })
      .fail(function(a, b) { alert("Problem retrieving XML data:" + b) });
   },


   load_history: function()
   {
      Clapf.log("[load_history]");

      document.body.style.cursor = 'wait';

      Clapf.spinner('start');

      jQuery.ajax('index.php?route=history/worker',
      {
         data: $.extend(!0, {}, Clapf.Shared, Clapf.Searches[Clapf.search]),
         type: "POST"
      })
      .done( function(a) {
         $('#A1').html(a);
         document.body.style.cursor = 'default';
         Clapf.spinner('stop');
      })
      .fail(function(a, b) { alert("Problem retrieving XML data:" + b) });

   },


   run_expert_query: function()
   {
      Clapf.log("[run_expert_query]");

      $("#button_search").click();
   },


   /*
    * add shortcuts on the search page
    */

   add_shortcuts: function()
   {
      Clapf.log("[add_shortcuts]");

      $(document).keypress(function(e){

         if(e.which == 13) {
            Clapf.run_expert_query();
         }

         if(e.which == 0) {
            $("#search").val('');
            $("#search").focus();
         }

      });

   },


   stop_propagation: function(event) {
      Clapf.log("[stop_propagation]");
      try {
         event.stopPropagation();
      }
      catch ( e ) {
         Clapf.log("[stop_propagation]", e );
      }
   },


   fix_form: function()
   {
      var a = '';

      Clapf.log("[fix_form]");

      for(i=0; i<document.forms[0].elements.length; i++){
         if(document.forms[0].elements[i].name) {
            a = a + "&" + document.forms[0].elements[i].name + "=" + document.forms[0].elements[i].value;
         }
      }

      a = document.forms[0].action + a;

      Clapf.log("[fix_form]", a);

      document.location.href = a;
   },


   update_search_window: function()
   {
      var s = $.trim($('input#search').val());

      if(s.length > 3) {
         Clapf.expert();
      }
   }


}


$.datepicker.setDefaults($.datepicker.regional['hu']);

$(function() {

    $("#date").datepicker( {dateFormat: 'yy-mm-dd' });
    $("#date1").datepicker( {dateFormat: 'yy-mm-dd' });
    $("#date2").datepicker( {dateFormat: 'yy-mm-dd' });

});


