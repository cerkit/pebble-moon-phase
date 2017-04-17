var KEY_LAT = 0;
var KEY_LNG = 1;


function sendCoordinates(lat, lng){
  
  // Send the message

  var dictionary = {
    KEY_LAT : lat,
    KEY_LNG : lng
  };
  
  Pebble.sendAppMessage(dictionary, 
    function(e) {
      console.log('coordinates sent to Pebble successfully');
    },
    function(e) {
      console.log('error sending coordinates to Pebble');
    }
  );
}

function locationSuccess(pos){
  var lat = pos.coords.latitude;
  var lng = pos.coords.longitude;
  
  sendCoordinates(lat, lng);
}

function locationError(err){
  console.log('Error requesting location');
  // send northern hemisphere (close to New York) as default
  sendCoordinates(43, 76);
}

function getLocation() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}



// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!');
    //getLocation();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log('Received message');
    getLocation();
  }                     
);
