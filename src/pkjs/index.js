'use strict';

var flightTools = require('./flight');
var API_ROOT = 'https://aeroapi.flightaware.com/aeroapi';
var CONFIG_URL = 'https://kvnsr1.github.io/pebble-flight/config/';
var FLIGHTS_KEY = 'pebbleFlight.flights';
var ACTIVE_KEY = 'pebbleFlight.activeIndex';
var CACHE_KEY = 'pebbleFlight.flightCache';
var LEGACY_CACHE_KEY = 'pebbleFlight.lastFlight';

function localToday() {
  var now = new Date();
  return new Date(now.getTime() - now.getTimezoneOffset() * 60000)
    .toISOString().slice(0, 10);
}

function cleanFlights(flights) {
  if (!Array.isArray(flights)) { return []; }
  return flights.slice(0, 3).map(function(flight) {
    return {
      flightNumber: String(flight.flightNumber || '').trim().toUpperCase(),
      flightDate: String(flight.flightDate || '').trim()
    };
  }).filter(function(flight) {
    return flight.flightNumber && flight.flightDate;
  });
}

function savedFlights() {
  var stored = localStorage.getItem(FLIGHTS_KEY);
  if (stored) {
    try { return cleanFlights(JSON.parse(stored)); } catch (ignore) {}
  }
  var legacyNumber = (localStorage.getItem('flightNumber') || '').toUpperCase();
  if (!legacyNumber) { return []; }
  return [{
    flightNumber: legacyNumber,
    flightDate: localStorage.getItem('flightDate') || localToday()
  }];
}

function activeIndex(flights) {
  var index = parseInt(localStorage.getItem(ACTIVE_KEY), 10);
  if (isNaN(index) || index < 0 || index >= flights.length) { index = 0; }
  return index;
}

function settings() {
  var flights = savedFlights();
  return {
    apiKey: localStorage.getItem('aeroApiKey') || '',
    flights: flights,
    activeIndex: activeIndex(flights)
  };
}

function currentFlight(config) {
  return config.flights[config.activeIndex] || null;
}

function flightKey(flight) {
  return flight.flightNumber + '|' + flight.flightDate;
}

function readCache() {
  try { return JSON.parse(localStorage.getItem(CACHE_KEY) || '{}'); }
  catch (ignore) { return {}; }
}

function cachedMessage(flight) {
  var cache = readCache();
  var message = cache[flightKey(flight)];
  if (!message && localStorage.getItem(LEGACY_CACHE_KEY)) {
    try {
      message = JSON.parse(localStorage.getItem(LEGACY_CACHE_KEY));
      if (message.FLIGHT_NUMBER === flight.flightNumber && message.FLIGHT_DATE === flight.flightDate) {
        cache[flightKey(flight)] = message;
        localStorage.setItem(CACHE_KEY, JSON.stringify(cache));
      } else {
        message = null;
      }
    } catch (ignore) { message = null; }
  }
  return message;
}

function cacheMessage(flight, message) {
  var cache = readCache();
  cache[flightKey(flight)] = message;
  localStorage.setItem(CACHE_KEY, JSON.stringify(cache));
}

function send(payload) {
  Pebble.sendAppMessage(payload, function() {}, function(error) {
    console.log('AppMessage failed: ' + JSON.stringify(error));
  });
}

function sendError(message) {
  send({IS_LOADING: 0, ERROR_MESSAGE: message.slice(0, 80)});
}

function sendPlaceholder(flight) {
  send({
    FLIGHT_NUMBER: flight.flightNumber,
    FLIGHT_DATE: flight.flightDate,
    ORIGIN: '---',
    DESTINATION: '---',
    DEPARTURE_TIME: '--',
    ARRIVAL_TIME: '--',
    STATUS_LABEL: 'REFRESHING',
    STATUS_LEVEL: 0,
    DEPARTURE_GATE: '--',
    DEPARTURE_TERMINAL: '--',
    ARRIVAL_GATE: '--',
    ARRIVAL_TERMINAL: '--',
    UPDATED_AT: '--',
    ERROR_MESSAGE: '',
    IS_LOADING: 1
  });
}

function showCurrent(config) {
  var flight = currentFlight(config);
  if (!flight) {
    sendError('Open phone settings to add a flight');
    return;
  }
  var cached = cachedMessage(flight);
  if (cached) { send(cached); } else { sendPlaceholder(flight); }
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
  var selected = currentFlight(config);
  if (!config.apiKey || !selected) {
    sendError('Open phone settings to add flights and an AeroAPI key');
    return;
  }

  send({IS_LOADING: 1, ERROR_MESSAGE: ''});
  var selectedKey = flightKey(selected);
  var window = dateWindow(selected.flightDate);
  var url = API_ROOT + '/flights/' + encodeURIComponent(selected.flightNumber) +
    '?start=' + encodeURIComponent(window.start) +
    '&end=' + encodeURIComponent(window.end) + '&max_pages=1';
  var request = new XMLHttpRequest();
  request.open('GET', url, true);
  request.setRequestHeader('Accept', 'application/json');
  request.setRequestHeader('x-apikey', config.apiKey);
  request.timeout = 15000;
  function sendIfStillSelected(message) {
    var active = currentFlight(settings());
    if (active && flightKey(active) === selectedKey) { sendError(message); }
  }
  request.onload = function() {
    if (request.status < 200 || request.status >= 300) {
      sendIfStillSelected(apiError(request.status));
      return;
    }
    try {
      var body = JSON.parse(request.responseText);
      var flight = flightTools.chooseFlight(body.flights || [], selected.flightDate);
      if (!flight) {
        sendIfStillSelected('No ' + selected.flightNumber + ' flight found on ' + selected.flightDate);
        return;
      }
      var message = flightTools.toMessage(flight);
      message.IS_LOADING = 0;
      message.ERROR_MESSAGE = '';
      cacheMessage(selected, message);
      var latest = currentFlight(settings());
      if (latest && flightKey(latest) === selectedKey) { send(message); }
    } catch (error) {
      sendIfStillSelected('Could not read flight data');
    }
  };
  request.onerror = function() { sendIfStillSelected('Could not reach FlightAware; showing saved data'); };
  request.ontimeout = function() { sendIfStillSelected('Flight lookup timed out'); };
  request.send();
}

function nextFlight() {
  var config = settings();
  if (config.flights.length < 2) {
    sendError('Add another flight in phone settings');
    return;
  }
  config.activeIndex = (config.activeIndex + 1) % config.flights.length;
  localStorage.setItem(ACTIVE_KEY, String(config.activeIndex));
  showCurrent(config);
  refresh();
}

Pebble.addEventListener('ready', function() {
  var config = settings();
  showCurrent(config);
  refresh();
});

Pebble.addEventListener('appmessage', function(event) {
  if (event.payload.REQUEST_NEXT_FLIGHT) { nextFlight(); }
  else if (event.payload.REQUEST_REFRESH) { refresh(); }
});

Pebble.addEventListener('showConfiguration', function() {
  var config = settings();
  var query = [];
  for (var i = 0; i < 3; i += 1) {
    var flight = config.flights[i] || {};
    query.push('f' + (i + 1) + '=' + encodeURIComponent(flight.flightNumber || ''));
    query.push('d' + (i + 1) + '=' + encodeURIComponent(flight.flightDate || ''));
  }
  query.push('hasApiKey=' + (config.apiKey ? '1' : '0'));
  Pebble.openURL(CONFIG_URL + '?' + query.join('&'));
});

Pebble.addEventListener('webviewclosed', function(event) {
  if (!event.response || event.response === 'CANCELLED') { return; }
  try {
    var config = JSON.parse(decodeURIComponent(event.response));
    var flights = cleanFlights(config.flights);
    if (!flights.length) {
      sendError('Add at least one flight in phone settings');
      return;
    }
    if (config.apiKey) { localStorage.setItem('aeroApiKey', config.apiKey); }
    localStorage.setItem(FLIGHTS_KEY, JSON.stringify(flights));
    localStorage.setItem(ACTIVE_KEY, '0');
    showCurrent(settings());
    refresh();
  } catch (error) {
    sendError('Settings could not be saved');
  }
});
