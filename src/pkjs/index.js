'use strict';

var flightTools = require('./flight');
var API_ROOT = 'https://aeroapi.flightaware.com/aeroapi';
var CONFIG_URL = 'https://kvnsr1.github.io/pebble-flight/config/';
var CACHE_KEY = 'pebbleFlight.lastFlight';

function localToday() {
  var now = new Date();
  return new Date(now.getTime() - now.getTimezoneOffset() * 60000)
    .toISOString().slice(0, 10);
}

function settings() {
  return {
    apiKey: localStorage.getItem('aeroApiKey') || '',
    flightNumber: (localStorage.getItem('flightNumber') || '').toUpperCase(),
    flightDate: localStorage.getItem('flightDate') || localToday()
  };
}

function send(payload) {
  Pebble.sendAppMessage(payload, function() {}, function(error) {
    console.log('AppMessage failed: ' + JSON.stringify(error));
  });
}

function sendError(message) {
  send({IS_LOADING: 0, ERROR_MESSAGE: message.slice(0, 80)});
}

function apiError(status) {
  if (status === 401 || status === 403) { return 'AeroAPI key was rejected; check phone settings'; }
  if (status === 404) { return 'Flight service endpoint was not found'; }
  if (status === 429) { return 'AeroAPI request limit reached; try again later'; }
  if (status >= 500) { return 'FlightAware is temporarily unavailable'; }
  return 'Flight service error (' + status + ')';
}

function dateWindow(date) {
  var start = new Date(date + 'T00:00:00Z');
  var end = new Date(date + 'T23:59:59Z');
  start.setUTCDate(start.getUTCDate() - 1);
  end.setUTCDate(end.getUTCDate() + 1);
  return {start: start.toISOString(), end: end.toISOString()};
}

function refresh() {
  var config = settings();
  if (!config.apiKey || !config.flightNumber) {
    sendError('Open phone settings to add a flight and AeroAPI key');
    return;
  }

  send({IS_LOADING: 1});
  var window = dateWindow(config.flightDate);
  var url = API_ROOT + '/flights/' + encodeURIComponent(config.flightNumber) +
    '?start=' + encodeURIComponent(window.start) +
    '&end=' + encodeURIComponent(window.end) + '&max_pages=1';
  var request = new XMLHttpRequest();
  request.open('GET', url, true);
  request.setRequestHeader('Accept', 'application/json');
  request.setRequestHeader('x-apikey', config.apiKey);
  request.timeout = 15000;
  request.onload = function() {
    if (request.status < 200 || request.status >= 300) {
      sendError(apiError(request.status));
      return;
    }
    try {
      var body = JSON.parse(request.responseText);
      var flight = flightTools.chooseFlight(body.flights || [], config.flightDate);
      if (!flight) {
        sendError('No ' + config.flightNumber + ' flight found on ' + config.flightDate);
        return;
      }
      var message = flightTools.toMessage(flight);
      message.IS_LOADING = 0;
      message.ERROR_MESSAGE = '';
      localStorage.setItem(CACHE_KEY, JSON.stringify(message));
      send(message);
    } catch (error) {
      sendError('Could not read flight data');
    }
  };
  request.onerror = function() { sendError('Could not reach FlightAware; showing saved data'); };
  request.ontimeout = function() { sendError('Flight lookup timed out'); };
  request.send();
}

Pebble.addEventListener('ready', function() {
  var cached = localStorage.getItem(CACHE_KEY);
  if (cached) {
    try { send(JSON.parse(cached)); } catch (ignore) {}
  }
  refresh();
});

Pebble.addEventListener('appmessage', function(event) {
  if (event.payload.REQUEST_REFRESH) { refresh(); }
});

Pebble.addEventListener('showConfiguration', function() {
  var config = settings();
  var query = '?flightNumber=' + encodeURIComponent(config.flightNumber) +
    '&flightDate=' + encodeURIComponent(config.flightDate) +
    '&hasApiKey=' + (config.apiKey ? '1' : '0');
  Pebble.openURL(CONFIG_URL + query);
});

Pebble.addEventListener('webviewclosed', function(event) {
  if (!event.response || event.response === 'CANCELLED') { return; }
  try {
    var config = JSON.parse(decodeURIComponent(event.response));
    if (config.apiKey) { localStorage.setItem('aeroApiKey', config.apiKey); }
    localStorage.setItem('flightNumber', (config.flightNumber || '').toUpperCase());
    localStorage.setItem('flightDate', config.flightDate || '');
    refresh();
  } catch (error) {
    sendError('Settings could not be saved');
  }
});
