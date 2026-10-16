'use strict';

// No storage or flight service is touched until a native LIVE_MODE reply.
// The default prototype does not open AppMessage and runs entirely on-watch.
var liveStarted = false;
Pebble.addEventListener('ready', function() {
  Pebble.sendAppMessage({REQUEST_MODE: 1}, function() {}, function() {});
});
Pebble.addEventListener('appmessage', function(event) {
  if (!liveStarted && event.payload.LIVE_MODE === 1) {
    liveStarted = true;
    require('./live').start();
  }
});
