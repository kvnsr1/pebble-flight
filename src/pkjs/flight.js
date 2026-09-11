'use strict';

function localDate(iso) {
  if (!iso) { return '--'; }
  return iso.slice(0, 10);
}

function localTime(iso) {
  if (!iso || iso.length < 16) { return '--'; }
  var hour = parseInt(iso.slice(11, 13), 10);
  var minute = iso.slice(14, 16);
  var suffix = hour >= 12 ? 'PM' : 'AM';
  hour = hour % 12 || 12;
  return hour + ':' + minute + ' ' + suffix;
}

function epochMs(iso) {
  return iso ? Date.parse(iso) : NaN;
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
  return localDate(flight.scheduled_out) === date || localDate(flight.scheduled_off) === date;
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
  return {
    FLIGHT_NUMBER: valueOrDash(flight.ident_iata || flight.ident),
    FLIGHT_DATE: localDate(flight.scheduled_out),
    ORIGIN: airportCode(flight.origin),
    DESTINATION: airportCode(flight.destination),
    DEPARTURE_TIME: localTime(flight.actual_out || flight.estimated_out || flight.scheduled_out),
    ARRIVAL_TIME: localTime(flight.actual_in || flight.estimated_in || flight.scheduled_in),
    STATUS_LABEL: status.label,
    STATUS_LEVEL: status.level,
    DEPARTURE_GATE: valueOrDash(flight.gate_origin),
    DEPARTURE_TERMINAL: valueOrDash(flight.terminal_origin),
    ARRIVAL_GATE: valueOrDash(flight.gate_destination),
    ARRIVAL_TERMINAL: valueOrDash(flight.terminal_destination),
    UPDATED_AT: localTime(new Date().toISOString())
  };
}

module.exports = {
  chooseFlight: chooseFlight,
  delayMinutes: delayMinutes,
  localTime: localTime,
  statusFor: statusFor,
  toMessage: toMessage
};
