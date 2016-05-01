
function send_wrapped( dict )
{
  Pebble.sendAppMessage( dict, function() 
  {
    console.log('Send successful: ' + JSON.stringify(dict));
  }, function() {
    console.log('Send failed: ' + JSON.stringify(dict) );
  });
}

Pebble.addEventListener('ready', function() 
{
  console.log('PebbleKit JS ready!');
  var dict = { KEY_BUTTON_JS_READY : 1 };  
  send_wrapped( dict );
  
});

Pebble.addEventListener('showConfiguration', function() {
  var url = 'https://rawgit.com/susundberg/pebble-countit-all/master/config/index.html';
  console.log('Showing configuration page: ' + url);
  Pebble.openURL(url);
});


function get_icon_number_from_string( str )
{
   //    "ICON_007.png"
   str = str.split("_")[1];
   str = str.split(".")[0];
   return parseInt(str);
}


Pebble.addEventListener('appmessage', function(e)
{
  console.log('Received DATA:' + JSON.stringify(e) );
});

Pebble.addEventListener('webviewclosed', function(e) {
  var config = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify( config ));

  var dict = {};
  for ( var loop_button = 1; loop_button < 3; loop_button ++ )
  {
     var prefix="button" + loop_button;
     
     var key_icon = ("key_" + prefix+"_icon").toUpperCase();
     var key_type = ("key_" + prefix+"_type").toUpperCase();
     dict[key_icon] = get_icon_number_from_string( config[ prefix + "-icon" ].toUpperCase() );
     dict[key_type] = config[ prefix + "-type" ].toUpperCase();
  }
  
  send_wrapped( dict );
});