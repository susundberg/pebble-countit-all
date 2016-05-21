

// var TARGET_HOST = 'https://rawgit.com/susundberg/pebble-countit-all/master/config/';
var TARGET_HOST = 'https://cdn.rawgit.com/susundberg/pebble-countit-all/master/config/';

// var TARGET_HOST = 'http://localhost:8000/';

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
  var url = TARGET_HOST + 'index.html';
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


// Convert x to an unsigned 32-bit integer
function ToUint32(x) {
    return x >>> 0;
}

function strip_out_uint32( data, index )
{
   
   var x = 0;
   for ( var loop = 0; loop < 4; loop ++ )
   {
      // Fuck you javascript without types.
      var added_shifter = ToUint32( data[index + loop] << (loop*8) );
      x = x + added_shifter;
   }
   console.log('Received uint32:' + x );
   return x;
}



Pebble.addEventListener('appmessage', function(e)
{
  // [08:22:29] javascript> Received DATA:{"type":"AppMessage","bubbles":false,"cancelable":false,"defaultPrevented":false,"target":null,"currentTarget":null,"eventPhase":2,"_aborted":false,"payload":{"1000":0,"1001":[22,0,0,240,252,227,38,87],"KEY_BUTTON_INDEX":0,"KEY_BUTTON_DATA":[22,0,0,240,252,227,38,87]}}
   
  console.log('Received DATA:' + JSON.stringify(e) );
  var index    = e["payload"]["KEY_BUTTON_INDEX"];
  var raw_data = e["payload"]["KEY_BUTTON_DATA"];
  var local_data = null;
  
  try
  {
     local_data = JSON.parse( localStorage.getItem('data-button-' + index ) );
  }
  catch (e)
  {
     console.log("Loading failed: " + e );
  }
  
  if ( !local_data )
     local_data = [];
  
  console.log("Loaded:" + JSON.stringify( local_data ) );
  if ( ( raw_data.length % 8 )  != 0 )
  {
        console.log('Data check mismatch, wrong length!');
        return;
  }
     
  for ( var loop = 0; loop < raw_data.length/8 ; loop ++ )
  {
     var duration_n_flags   = strip_out_uint32( raw_data, loop*8 );
     var header   = ToUint32( duration_n_flags & 0xF0000000 );
     ;
     if ( header != 0xF0000000 )
     {
        console.log('Data check mismatch, wrong header:' + header );
        return;
     }
     
     var duration = ToUint32( duration_n_flags & 0x0FFFFFFF )
     var since_epoc = strip_out_uint32( raw_data, loop*8 + 4 ) ;
     console.log("WRITING TO " + local_data + ":" + [since_epoc, duration, ] );
     local_data.push( [since_epoc, duration, ] );  
  }
  var key = 'data-button-' + index;
  localStorage.setItem( key , JSON.stringify( local_data ) );
  console.log('Data "' + key + '" now:' + JSON.stringify( local_data ) );
});


Pebble.addEventListener('webviewclosed', function(e) {
  if ( !e.response )
  {
     console.log('Got nothing from closed!'); 
     return;
  }
  var config = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify( config ));

  
  if ( config["type"] == "data_clear_all" )
  {
     for ( var index = 0; index < 3; index++ )
     {
        var key = 'data-button-' + index;
        localStorage.setItem(key,"");
        console.log("Clear data on " + key );
     }
     return;
  }
  
  if ( config["type"] == "button_data" )
  {
      var index = parseInt(config["id"]);
      var key = 'data-button-' + index;
      
      var data  = null; 
      
      try
      {
         data = JSON.parse( localStorage.getItem(key) );
      }
      catch (e)
      {
         console.log("Loading failed: " + e );
      }
      
      if ( !data )
         data = [];
      
      console.log("Got data data " + key + " : " + data );
      var data_enc = encodeURIComponent( data );
      // use redirect page since we want to get the think opened in a browser, since at least on android version the
      // pebble build in webview is unable to a) hande download b) copy text
      var url = TARGET_HOST + "download.html?" + data_enc ;
      console.log("Download data: "  +url );
      Pebble.openURL( url ); 
      return;
  }
  
  if ( config["type"] == "config" )
  {
      var dict = {};
      for ( var loop_button = 0; loop_button < 3; loop_button ++ )
      {
         var prefix="button" + loop_button;
         
         var key_icon = ("key_" + prefix+"_icon").toUpperCase();
         var key_type = ("key_" + prefix+"_type").toUpperCase();
         dict[key_icon] = get_icon_number_from_string( config[ prefix + "-icon" ].toUpperCase() );
         dict[key_type] = config[ prefix + "-type" ].toUpperCase();
      }
      send_wrapped( dict );
      return;
  }
  
  console.log("Unknown return!");
  
});