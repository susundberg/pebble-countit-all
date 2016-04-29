Pebble.addEventListener('ready', function() 
{
  console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('showConfiguration', function() {
  var url = 'https://rawgit.com/susundberg/pebble-countit-all/master/config/index.html';
  console.log('Showing configuration page: ' + url);
  Pebble.openURL(url);
});


function get_icon_number_from_string( str )
{
   //    "ICON_007.png"
   console.log("GOT: " + str ) 
   str = str.split("_")[1];
   console.log("GOT: " + str ) 
   str = str.split(".")[0];
   console.log("GOT: " + str ) 
   return parseInt(str);
}

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
  
  // Send to watchapp
  Pebble.sendAppMessage( dict, function() {
    console.log('Send successful: ' + JSON.stringify(dict));
  }, function() {
    console.log('Send failed: ' + JSON.stringify(dict) );
  });
});