'use strict';

var flightTools = require('./flight');
var API_ROOT = 'https://aeroapi.flightaware.com/aeroapi';
var CONFIG_URL = 'https://kvnsr1.github.io/pebble-flight/config/';
var FLIGHTS_KEY = 'pebbleFlight.flights';
var ACTIVE_KEY = 'pebbleFlight.activeIndex';
var CACHE_KEY = 'pebbleFlight.flightCache';
var STATE_KEY = 'pebbleFlight.refreshState';
var DETAILS_KEY = 'pebbleFlight.aircraftDetails';
var LEGACY_CACHE_KEY = 'pebbleFlight.lastFlight';

function localToday() {
  var now = new Date();
  return new Date(now.getTime() - now.getTimezoneOffset() * 60000)
    .toISOString().slice(0, 10);
}

function cleanFlights(flights) {
  if (!Array.isArray(flights)) { return []; }
  return flights.slice(0, 3).map(function(flight) {
    var seatNumber = String(flight.seatNumber || '').trim().toUpperCase().slice(0, 3);
    var inferredPosition = flightTools.seatPositionFor(seatNumber);
    var savedPosition = /^(window|middle|aisle)$/.test(flight.seatPosition) ?
      flight.seatPosition : '';
    var isLegacyOverride = typeof flight.seatPositionOverride === 'undefined' &&
      savedPosition && savedPosition !== inferredPosition;
    var hasOverride = flight.seatPositionOverride === true || isLegacyOverride;
    return {
      flightNumber: String(flight.flightNumber || '').trim().toUpperCase(),
      flightDate: String(flight.flightDate || '').trim(),
      bookingCode: String(flight.bookingCode || '').trim().toUpperCase().slice(0, 12),
      seatNumber: seatNumber,
      seatPosition: hasOverride ? savedPosition : (inferredPosition || savedPosition),
      seatPositionOverride: Boolean(hasOverride)
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

function readJson(key) {
  try { return JSON.parse(localStorage.getItem(key) || '{}'); }
  catch (ignore) { return {}; }
}

function writeJson(key, value) {
  localStorage.setItem(key, JSON.stringify(value));
}

function pruneLandedFlights(flights) {
  var now = Date.now();
  var states = readJson(STATE_KEY);
  var cache = readJson(CACHE_KEY);
  var details = readJson(DETAILS_KEY);
  var kept = flights.filter(function(flight) {
    var state = states[flightKey(flight)];
    return !state || !state.actualOn || now < state.actualOn + 60 * 60 * 1000;
  });
  if (kept.length === flights.length) { return flights; }

  var retained = {};
  kept.forEach(function(flight) { retained[flightKey(flight)] = true; });
  Object.keys(states).forEach(function(key) { if (!retained[key]) { delete states[key]; } });
  Object.keys(cache).forEach(function(key) { if (!retained[key]) { delete cache[key]; } });
  Object.keys(details).forEach(function(key) { if (!retained[key]) { delete details[key]; } });
  writeJson(STATE_KEY, states);
  writeJson(CACHE_KEY, cache);
  writeJson(DETAILS_KEY, details);
  localStorage.setItem(FLIGHTS_KEY, JSON.stringify(kept));
  var index = parseInt(localStorage.getItem(ACTIVE_KEY), 10) || 0;
  localStorage.setItem(ACTIVE_KEY, String(Math.min(index, Math.max(0, kept.length - 1))));
  return kept;
}

function activeIndex(flights) {
  var index = parseInt(localStorage.getItem(ACTIVE_KEY), 10);
  if (isNaN(index) || index < 0 || index >= flights.length) { index = 0; }
  return index;
}

function settings() {
  var flights = pruneLandedFlights(savedFlights());
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
  return readJson(CACHE_KEY);
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
  writeJson(CACHE_KEY, cache);
}

function configuredDepartureMs(flight) {
  return Date.parse(flight.flightDate + 'T12:00:00');
}

function immediateFlightKey(config, states) {
  var candidates = config.flights.filter(function(flight) {
    return !flightTools.isTerminal(states[flightKey(flight)]);
  });
  candidates.sort(function(a, b) {
    return flightTools.departureMs(states[flightKey(a)], configuredDepartureMs(a)) -
      flightTools.departureMs(states[flightKey(b)], configuredDepartureMs(b));
  });
  return candidates.length ? flightKey(candidates[0]) : '';
}

function nextRefreshLabel(config, flight, states, now) {
  var key = flightKey(flight);
  var state = states[key];
  if (flightTools.isTerminal(state)) { return 'OFF'; }
  var fallback = configuredDepartureMs(flight);
  var isImmediate = key === immediateFlightKey(config, states);
  var interval = flightTools.refreshIntervalMs(state, fallback, isImmediate, now);
  if (interval === null) {
    var departure = flightTools.departureMs(state, fallback);
    if (departure - now > 7 * 24 * 60 * 60 * 1000) {
      return flightTools.deviceLocalDateTime(departure - 7 * 24 * 60 * 60 * 1000);
    }
    return 'WHEN NEXT';
  }
  if (!state || !state.lastRequestAt) { return 'DUE NOW'; }
  return flightTools.deviceLocalTime(new Date(state.lastRequestAt + interval).toISOString());
}

function addRefreshLabel(message, config, flight) {
  message.NEXT_REFRESH_AT = nextRefreshLabel(
    config, flight, readJson(STATE_KEY), Date.now());
  return message;
}

function decorateMessage(message, config, flight) {
  addRefreshLabel(message, config, flight);
  message.BOOKING_CODE = flight.bookingCode || '--';
  message.SEAT_NUMBER = flight.seatNumber || '--';
  message.SEAT_POSITION = (flight.seatPosition || '--').toUpperCase();
  var details = readJson(DETAILS_KEY)[flightKey(flight)];
  if (details) {
    Object.keys(details).forEach(function(key) {
      if (key !== 'loaded') { message[key] = details[key]; }
    });
  }
  message.AIRCRAFT_MODEL = message.AIRCRAFT_MODEL || '--';
  message.AIRCRAFT_NUMBER = message.AIRCRAFT_NUMBER || '--';
  message.AIRCRAFT_FIRST_FLIGHT = message.AIRCRAFT_FIRST_FLIGHT || 'Unavailable';
  message.AIRCRAFT_LEG_1 = message.AIRCRAFT_LEG_1 || '--';
  message.AIRCRAFT_LEG_1_STATUS = message.AIRCRAFT_LEG_1_STATUS || '--';
  message.AIRCRAFT_LEG_1_LEVEL = message.AIRCRAFT_LEG_1_LEVEL || 0;
  message.AIRCRAFT_LEG_2 = message.AIRCRAFT_LEG_2 || '--';
  message.AIRCRAFT_LEG_2_STATUS = message.AIRCRAFT_LEG_2_STATUS || '--';
  message.AIRCRAFT_LEG_2_LEVEL = message.AIRCRAFT_LEG_2_LEVEL || 0;
  return message;
}

function send(payload) {
  Pebble.sendAppMessage(payload, function() {}, function(error) {
    console.log('AppMessage failed: ' + JSON.stringify(error));
  });
}

function sendError(message) {
  send({IS_LOADING: 0, ERROR_MESSAGE: message.slice(0, 80)});
}

function sendPlaceholder(config, flight) {
  send(decorateMessage({
    FLIGHT_NUMBER: flight.flightNumber,
    FLIGHT_DATE: flight.flightDate,
    ORIGIN: '---',
    DESTINATION: '---',
    DEPARTURE_TIME: '--',
    ARRIVAL_TIME: '--',
    STATUS_LABEL: 'SCHEDULED',
    STATUS_LEVEL: 0,
    DEPARTURE_GATE: '--',
    DEPARTURE_TERMINAL: '--',
    ARRIVAL_GATE: '--',
    ARRIVAL_TERMINAL: '--',
    AIRCRAFT_MODEL: '--',
    AIRCRAFT_NUMBER: '--',
    AIRCRAFT_FIRST_FLIGHT: 'Unavailable',
    UPDATED_AT: '--',
    ERROR_MESSAGE: '',
    IS_LOADING: 0
  }, config, flight));
}

function showCurrent(config) {
  var flight = currentFlight(config);
  if (!flight) {
    sendError('Open phone settings to add a flight');
    return;
  }
  var cached = cachedMessage(flight);
  if (cached) { send(decorateMessage(cached, config, flight)); }
  else { sendPlaceholder(config, flight); }
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

function refresh(force) {
  var config = settings();
  var selected = currentFlight(config);
  if (!config.apiKey || !selected) {
    sendError('Open phone settings to add flights and an AeroAPI key');
    return;
  }

  var selectedKey = flightKey(selected);
  var states = readJson(STATE_KEY);
  var state = states[selectedKey];
  if (flightTools.isTerminal(state)) {
    showCurrent(config);
    return;
  }
  var now = Date.now();
  var isImmediate = selectedKey === immediateFlightKey(config, states);
  if (!force && !flightTools.refreshIsDue(
    state, configuredDepartureMs(selected), isImmediate, now)) {
    showCurrent(config);
    return;
  }

  send({IS_LOADING: 1, ERROR_MESSAGE: ''});
  state = state || {};
  state.lastRequestAt = now;
  states[selectedKey] = state;
  writeJson(STATE_KEY, states);
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
      var latestStates = readJson(STATE_KEY);
      latestStates[selectedKey] = flightTools.toRefreshState(flight, now);
      writeJson(STATE_KEY, latestStates);
      message.IS_LOADING = 0;
      message.ERROR_MESSAGE = '';
      decorateMessage(message, config, selected);
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

function legDetails(flight) {
  var status = flightTools.statusFor(flight);
  var origin = flight.origin && (flight.origin.code_iata || flight.origin.code) || '---';
  var destination = flight.destination &&
    (flight.destination.code_iata || flight.destination.code) || '---';
  return {
    route: origin + ' > ' + destination,
    status: status.label,
    level: status.level
  };
}

function loadAircraftDetails() {
  var config = settings();
  var selected = currentFlight(config);
  if (!selected) { return; }
  var key = flightKey(selected);
  var detailsCache = readJson(DETAILS_KEY);
  if (detailsCache[key] && (detailsCache[key].loaded ||
      Date.now() - detailsCache[key].requestedAt < 24 * 60 * 60 * 1000)) {
    showCurrent(config);
    return;
  }
  var state = readJson(STATE_KEY)[key];
  if (!state || !state.registration || state.registration === '--') {
    showCurrent(config);
    return;
  }

  detailsCache[key] = {loaded: false, requestedAt: Date.now()};
  writeJson(DETAILS_KEY, detailsCache);

  var endMs = state.scheduledOut || Date.now();
  var startMs = endMs - 4 * 24 * 60 * 60 * 1000;
  var url = API_ROOT + '/flights/' + encodeURIComponent(state.registration) +
    '?start=' + encodeURIComponent(new Date(startMs).toISOString()) +
    '&end=' + encodeURIComponent(new Date(endMs).toISOString()) + '&max_pages=1';
  var request = new XMLHttpRequest();
  request.open('GET', url, true);
  request.setRequestHeader('Accept', 'application/json');
  request.setRequestHeader('x-apikey', config.apiKey);
  request.timeout = 15000;
  request.onload = function() {
    if (request.status < 200 || request.status >= 300) { return; }
    try {
      var flights = (JSON.parse(request.responseText).flights || []).filter(function(candidate) {
        var departure = Date.parse(candidate.scheduled_out || candidate.scheduled_off);
        return candidate.fa_flight_id !== state.faFlightId && departure < endMs;
      }).sort(function(a, b) {
        return Date.parse(b.scheduled_out || b.scheduled_off) -
          Date.parse(a.scheduled_out || a.scheduled_off);
      }).slice(0, 2);
      var first = flights[0] ? legDetails(flights[0]) : null;
      var second = flights[1] ? legDetails(flights[1]) : null;
      detailsCache[key] = {
        loaded: true,
        requestedAt: Date.now(),
        AIRCRAFT_MODEL: flightTools.aircraftModel(state.aircraftType),
        AIRCRAFT_NUMBER: state.registration,
        AIRCRAFT_FIRST_FLIGHT: 'Unavailable',
        AIRCRAFT_LEG_1: first ? first.route : '--',
        AIRCRAFT_LEG_1_STATUS: first ? first.status : '--',
        AIRCRAFT_LEG_1_LEVEL: first ? first.level : 0,
        AIRCRAFT_LEG_2: second ? second.route : '--',
        AIRCRAFT_LEG_2_STATUS: second ? second.status : '--',
        AIRCRAFT_LEG_2_LEVEL: second ? second.level : 0
      };
      writeJson(DETAILS_KEY, detailsCache);
      var active = currentFlight(settings());
      if (active && flightKey(active) === key) { showCurrent(settings()); }
    } catch (ignore) {}
  };
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
  refresh(false);
}

Pebble.addEventListener('ready', function() {
  var config = settings();
  showCurrent(config);
  refresh(false);
});

Pebble.addEventListener('appmessage', function(event) {
  if (event.payload.REQUEST_NEXT_FLIGHT) { nextFlight(); }
  else if (event.payload.REQUEST_AIRCRAFT_DETAILS) { loadAircraftDetails(); }
  else if (event.payload.REQUEST_REFRESH) { refresh(event.payload.REQUEST_REFRESH === 1); }
});

Pebble.addEventListener('showConfiguration', function() {
  var config = settings();
  var state = {flights: config.flights, hasApiKey: Boolean(config.apiKey)};
  Pebble.openURL(CONFIG_URL + '#state=' + encodeURIComponent(JSON.stringify(state)));
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
    refresh(false);
  } catch (error) {
    sendError('Settings could not be saved');
  }
});
