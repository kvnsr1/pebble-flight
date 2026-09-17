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
    cancelled: Boolean(flight.cancelled)
  };
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
    UPDATED_AT: deviceLocalTime(new Date().toISOString())
  };
}

module.exports = {
  chooseFlight: chooseFlight,
  departureMs: departureMs,
  delayMinutes: delayMinutes,
  deviceLocalDateTime: deviceLocalDateTime,
  deviceLocalTime: deviceLocalTime,
  isTerminal: isTerminal,
  localTime: localTime,
  refreshIntervalMs: refreshIntervalMs,
  refreshIsDue: refreshIsDue,
  statusFor: statusFor,
  toMessage: toMessage,
  toRefreshState: toRefreshState
};
