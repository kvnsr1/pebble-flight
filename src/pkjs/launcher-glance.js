'use strict';
var times = require('./airport-time');
var nativeView = require('./native-view');
var DAY = 86400000;

function epoch(value) { var n = Date.parse(value); return isFinite(n) && n > 0 ? n : 0; }
function code(value) { return /^[A-Z0-9]{3,4}$/.test(value || '') ? value : '---'; }
function number(value) { return String(value || '').toUpperCase().replace(/[^A-Z0-9 ]/g, '').slice(0, 12); }
function timeLabel(raw, departure) {
  var airport = raw[departure ? 'origin' : 'destination'] || {};
  var suffix = departure ? 'out' : 'in';
  var value = raw['actual_' + suffix] || raw['estimated_' + suffix] || raw['scheduled_' + suffix];
  var local = times.local(value, airport.timezone);
  if (!local) { return 'Time unknown'; }
  var kind = raw['actual_' + suffix] ? 'ACT' : raw['estimated_' + suffix] ? 'EST' : 'SCHED';
  return (departure ? 'DEP ' : 'ARR ') + kind + ' ' + local.format('HH:mm') + ' ' + local.zoneAbbr() +
    ' ' + local.format('DD MMM').toUpperCase();
}

// No network requests. Native AppGlance slices persist after the app closes.
// Expiring a schedule never asserts that the flight actually departed/landed.
function plan(flights, rawCache, legacyCache, states, now) {
  var candidates = [];
  flights.forEach(function(configured, index) {
    var key = configured.flightNumber + '|' + configured.flightDate;
    var record = rawCache[key], raw = record && record.flight || {}, state = states[key] || {};
    if (raw.cancelled || raw.actual_on || raw.actual_in || state.cancelled || state.actualOn) { return; }
    var departure = epoch(raw.estimated_out) || epoch(raw.scheduled_out) || epoch(raw.scheduled_off);
    var arrival = epoch(raw.estimated_in) || epoch(raw.scheduled_in) || epoch(raw.estimated_on) || epoch(raw.scheduled_on);
    var day = epoch(configured.flightDate + 'T00:00:00Z');
    // With no timezone/time, retain the date until it has ended in UTC-12.
    // This is an expiry bound only: never display it as a departure time.
    var end = arrival || Math.max(departure, day) + 36 * 3600000;
    if (!day || end <= now) { return; }
    var legacy = legacyCache[key] || {};
    var origin = code((raw.origin || {}).code_iata || legacy.ORIGIN);
    var destination = code((raw.destination || {}).code_iata || legacy.DESTINATION);
    var prefix = number(configured.flightNumber) + ' · ' + origin + '>' + destination + ' · ';
    var airborne = Boolean(raw.actual_off), departed = Boolean(raw.actual_out);
    var status = ['Upcoming', 'Boarding', 'In flight', 'Landed', 'Delayed', 'Canceled', 'Diverted', 'Departed', 'Not updated'][
      record ? nativeView.toMessage(configured, record, legacy, index, flights.length, 0, {}).UI_STATUS : 8];
    var detail = record ? timeLabel(raw, airborne || departed ? false : true) :
      times.dateLabel(configured.flightDate) + ' · Details not updated';
    var text = prefix + (record ? status + ' · ' : '') + detail;
    // Cached operational text must not persist indefinitely after its expected
    // departure. Ask for a fresh status instead of predicting an airborne state.
    var boundary = !airborne && !departed && departure > now ? departure : end;
    if (!airborne && !departed && departure && departure <= now) {
      text = prefix + 'Open for flight status';
    }
    candidates.push({order: departure || day + DAY / 2, index: index, end: end,
      boundary: boundary, text: text, fallback: prefix + 'Open for flight status'});
  });
  candidates.sort(function(a, b) { return a.order - b.order || a.index - b.index; });
  var slices = [], cursor = now;
  candidates.forEach(function(flight) {
    if (flight.end <= cursor) { return; }
    if (flight.boundary > cursor) { slices.push({expires: flight.boundary, text: flight.text}); }
    if (flight.end > flight.boundary) { slices.push({expires: flight.end, text: flight.fallback}); }
    cursor = flight.end;
  });
  slices.push({expires: 0, text: flights.length ? 'No upcoming flights · Open to view all' : 'Add flights in phone settings'});
  // At most 2 slices per saved flight (3) + the final empty-state slice (1).
  return slices.map(function(slice) { return Math.floor(slice.expires / 1000) + '\t' + slice.text + '\n'; }).join('');
}
module.exports = {plan: plan};
