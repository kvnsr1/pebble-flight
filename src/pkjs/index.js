'use strict';

// Wait for the native mode reply before starting data access. Settings can also
// be opened explicitly from the phone when the watch app is not running.
var liveStarted = false;
var mode = 0;
var service = null;
function live() { if (!service) { service = require('./live'); } return service; }
Pebble.addEventListener('ready', function() {
  Pebble.sendAppMessage({REQUEST_MODE: 1}, function() {}, function() {});
});
Pebble.addEventListener('appmessage', function(event) {
  if (!liveStarted && (event.payload.LIVE_MODE === 1 || event.payload.LIVE_MODE === 2)) {
    liveStarted = true;
    mode = event.payload.LIVE_MODE;
    live().start({nativeUI: mode === 2});
  } else if (liveStarted && mode === 2 && event.payload.LIVE_MODE === 2) {
    live().sync();
  }
});
Pebble.addEventListener('showConfiguration', function() {
  live().configure({nativeUI: mode !== 1});
});
