'use strict';

function localDate(iso, timezone) {
  if (!iso) { return '--'; }
  if (timezone && typeof Intl !== 'undefined' && Intl.DateTimeFormat) {
    try {
      var formatted = new Intl.DateTimeFormat('en-US', {
        timeZone: timezone,
        year: 'numeric',
        month: '2-digit',
        day: '2-digit'
      }).format(new Date(iso));
      var parts = formatted.match(/(\d{2})\/(\d{2})\/(\d{4})/);
      if (parts) { return parts[3] + '-' + parts[1] + '-' + parts[2]; }
    } catch (ignore) {}
  }
  return iso.slice(0, 10);
}

function localTime(iso, timezone) {
  if (!iso || iso.length < 16) { return '--'; }
  if (timezone && typeof Intl !== 'undefined' && Intl.DateTimeFormat) {
    try {
      return new Intl.DateTimeFormat('en-US', {
        timeZone: timezone,
        hour: 'numeric',
        minute: '2-digit',
        hour12: true
      }).format(new Date(iso)).replace(/\s/g, ' ');
    } catch (ignore) {}
  }
  var hour = parseInt(iso.slice(11, 13), 10);
  var minute = iso.slice(14, 16);
  var suffix = hour >= 12 ? 'PM' : 'AM';
  hour = hour % 12 || 12;
  return hour + ':' + minute + ' ' + suffix;
}

function deviceLocalTime(iso) {
  if (!iso) { return '--'; }
  if (typeof Intl !== 'undefined' && Intl.DateTimeFormat) {
    try {
      return new Intl.DateTimeFormat('en-US', {
        hour: 'numeric',
        minute: '2-digit',
        hour12: true
      }).format(new Date(iso)).replace(/\s/g, ' ');
    } catch (ignore) {}
  }
  return localTime(iso, 'America/Los_Angeles');
}

function deviceLocalDateTime(epoch) {
  if (!epoch) { return '--'; }
  if (typeof Intl !== 'undefined' && Intl.DateTimeFormat) {
    try {
      return new Intl.DateTimeFormat('en-US', {
        month: 'short',
        day: 'numeric',
        hour: 'numeric',
        minute: '2-digit',
        hour12: true
      }).format(new Date(epoch)).replace(/,|\s/g, function(match) {
        return match === ',' ? '' : ' ';
      });
    } catch (ignore) {}
  }
  return deviceLocalTime(new Date(epoch).toISOString());
}

function epochMs(iso) {
  return iso ? Date.parse(iso) : NaN;
}

var MINUTE_MS = 60 * 1000;
var HOUR_MS = 60 * MINUTE_MS;
var DAY_MS = 24 * HOUR_MS;

function toRefreshState(flight, requestedAt) {
  return {
    lastRequestAt: requestedAt,
    scheduledOut: epochMs(flight.scheduled_out || flight.scheduled_off),
    estimatedOut: epochMs(flight.estimated_out || flight.estimated_off),
    actualOut: epochMs(flight.actual_out || flight.actual_off),
    scheduledIn: epochMs(flight.scheduled_in || flight.scheduled_on),
    estimatedIn: epochMs(flight.estimated_in || flight.estimated_on),
    actualOn: epochMs(flight.actual_on || flight.actual_in),
    cancelled: Boolean(flight.cancelled),
    registration: valueOrDash(flight.registration),
    aircraftType: valueOrDash(flight.aircraft_type),
    faFlightId: valueOrDash(flight.fa_flight_id)
  };
}

function aircraftModel(type) {
  var models = {
    A19N: 'Airbus A319neo', A20N: 'Airbus A320neo', A21N: 'Airbus A321neo',
    A319: 'Airbus A319', A320: 'Airbus A320', A321: 'Airbus A321',
    A332: 'Airbus A330-200', A333: 'Airbus A330-300', A359: 'Airbus A350-900',
    B37M: 'Boeing 737 MAX 7', B38M: 'Boeing 737 MAX 8', B39M: 'Boeing 737 MAX 9',
    B737: 'Boeing 737-700', B738: 'Boeing 737-800', B739: 'Boeing 737-900',
    B752: 'Boeing 757-200', B763: 'Boeing 767-300', B772: 'Boeing 777-200',
    B77W: 'Boeing 777-300ER', B788: 'Boeing 787-8', B789: 'Boeing 787-9',
    CRJ2: 'CRJ-200', CRJ7: 'CRJ-700', CRJ9: 'CRJ-900',
    E170: 'Embraer E170', E175: 'Embraer E175', E190: 'Embraer E190'
  };
  return models[type] || valueOrDash(type);
}

function departureMs(state, fallback) {
  if (!state) { return fallback; }
  return state.estimatedOut || state.scheduledOut || fallback;
}

function isTerminal(state) {
  return Boolean(state && (state.cancelled || state.actualOn));
}

function refreshIntervalMs(state, fallbackDeparture, isImmediate, now) {
  if (isTerminal(state)) { return null; }
  if (state && state.actualOut) { return 10 * MINUTE_MS; }

  var remaining = departureMs(state, fallbackDeparture) - now;
  if (remaining > 7 * DAY_MS) { return null; }
  if (remaining > DAY_MS) { return isImmediate ? 6 * HOUR_MS : null; }
  if (remaining > 6 * HOUR_MS) { return HOUR_MS; }
  return 15 * MINUTE_MS;
}

function refreshIsDue(state, fallbackDeparture, isImmediate, now) {
  var interval = refreshIntervalMs(state, fallbackDeparture, isImmediate, now);
  if (interval === null) { return false; }
  return !state || !state.lastRequestAt || now - state.lastRequestAt >= interval;
}

function delayMinutes(flight) {
  var hasDeparted = Boolean(flight.actual_out || flight.actual_off);
  var scheduled = epochMs(hasDeparted ? flight.scheduled_in : flight.scheduled_out);
  var current = epochMs(hasDeparted ?
    (flight.actual_in || flight.estimated_in || flight.scheduled_in) :
    (flight.actual_out || flight.estimated_out || flight.scheduled_out));
  if (isNaN(scheduled) || isNaN(current)) { return 0; }
  return Math.max(0, Math.round((current - scheduled) / 60000));
}

function statusFor(flight) {
  if (flight.cancelled) { return {label: 'CANCELED', level: 4}; }
  if (flight.diverted) { return {label: 'DIVERTED', level: 4}; }
  var minutes = delayMinutes(flight);
  if (minutes === 0) { return {label: 'ON TIME', level: 0}; }
  if (minutes < 30) { return {label: 'SLIGHT DELAY +' + minutes + 'm', level: 1}; }
  if (minutes <= 60) { return {label: 'DELAY +' + minutes + 'm', level: 2}; }
  return {label: 'MAJOR DELAY +' + minutes + 'm', level: 3};
}

function airportCode(airport) {
  if (!airport) { return '---'; }
  return airport.code_iata || airport.code || '---';
}

function valueOrDash(value) {
  return value === null || value === undefined || value === '' ? '--' : String(value);
}

function seatPositionFor(seatNumber) {
  var match = String(seatNumber || '').trim().toUpperCase().match(/^[1-9]\d?([A-F])$/);
  if (!match) { return ''; }
  if (match[1] === 'A' || match[1] === 'F') { return 'window'; }
  if (match[1] === 'B' || match[1] === 'E') { return 'middle'; }
  return 'aisle';
}

function matchesDate(flight, date) {
  var timezone = flight.origin && flight.origin.timezone;
  return localDate(flight.scheduled_out, timezone) === date ||
    localDate(flight.scheduled_off, timezone) === date;
}

function chooseFlight(flights, date) {
  var matches = flights.filter(function(flight) { return matchesDate(flight, date); });
  if (!matches.length) { return null; }
  matches.sort(function(a, b) {
    return Math.abs(epochMs(a.scheduled_out) - Date.now()) -
      Math.abs(epochMs(b.scheduled_out) - Date.now());
  });
  return matches[0];
}

function toMessage(flight) {
  var status = statusFor(flight);
  var originTimezone = flight.origin && flight.origin.timezone;
  var destinationTimezone = flight.destination && flight.destination.timezone;
  return {
    FLIGHT_NUMBER: valueOrDash(flight.ident_iata || flight.ident),
    FLIGHT_DATE: localDate(flight.scheduled_out, originTimezone),
    ORIGIN: airportCode(flight.origin),
    DESTINATION: airportCode(flight.destination),
    DEPARTURE_TIME: localTime(
      flight.actual_out || flight.estimated_out || flight.scheduled_out, originTimezone),
    ARRIVAL_TIME: localTime(
      flight.actual_in || flight.estimated_in || flight.scheduled_in, destinationTimezone),
    STATUS_LABEL: status.label,
    STATUS_LEVEL: status.level,
    DEPARTURE_GATE: valueOrDash(flight.gate_origin),
    DEPARTURE_TERMINAL: valueOrDash(flight.terminal_origin),
    ARRIVAL_GATE: valueOrDash(flight.gate_destination),
    ARRIVAL_TERMINAL: valueOrDash(flight.terminal_destination),
    AIRCRAFT_MODEL: aircraftModel(flight.aircraft_type),
    AIRCRAFT_NUMBER: valueOrDash(flight.registration),
    AIRCRAFT_FIRST_FLIGHT: '--',
    UPDATED_AT: deviceLocalTime(new Date().toISOString())
  };
}

module.exports = {
  aircraftModel: aircraftModel,
  chooseFlight: chooseFlight,
  departureMs: departureMs,
  delayMinutes: delayMinutes,
  deviceLocalDateTime: deviceLocalDateTime,
  deviceLocalTime: deviceLocalTime,
  isTerminal: isTerminal,
  localTime: localTime,
  refreshIntervalMs: refreshIntervalMs,
  refreshIsDue: refreshIsDue,
  seatPositionFor: seatPositionFor,
  statusFor: statusFor,
  toMessage: toMessage,
  toRefreshState: toRefreshState
};
