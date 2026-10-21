'use strict';
var times = require('./airport-time');
// Must match flight_model.h: field/time/status enums are part of the wire format.
var UNKNOWN = 1, UNAVAILABLE = 2;
function present(value) { return value !== undefined && value !== null && value !== '' && value !== '--'; }
function field(object, key) {
  var value = object[key];
  return {value: present(value) ? String(value) : '',
    state: present(value) ? 0 : Object.prototype.hasOwnProperty.call(object, key) ? UNKNOWN : UNAVAILABLE};
}
function endpoint(raw, departure, timeFormat) {
  var airport = raw[departure ? 'origin' : 'destination'] || {};
  // Gate schedule and current times use the same basis. Never mix runway and
  // gate times to manufacture a delay or an unreported gate estimate.
  var scheduled = raw[departure ? 'scheduled_out' : 'scheduled_in'];
  var actual = raw[departure ? 'actual_out' : 'actual_in'];
  var estimated = raw[departure ? 'estimated_out' : 'estimated_in'];
  var current = actual || (!raw.cancelled && estimated);
  var a = times.local(scheduled, airport.timezone);
  var b = times.local(current, airport.timezone);
  var reference = b || a;
  return {
    code: airport.code_iata || airport.code || '---', name: airport.name || 'Airport name unknown',
    scheduled: a ? a.format(times.clockPattern(timeFormat)) : '--:--',
    current: b ? b.format(times.clockPattern(timeFormat)) : '',
    date: reference ? reference.format('DD MMM').toUpperCase() : 'Unknown',
    scheduledDate: a ? a.format('DD MMM').toUpperCase() : 'Unknown',
    scheduledZone: a && b && a.utcOffset() !== b.utcOffset() ? (a.zoneAbbr() !== b.zoneAbbr() ? a.zoneAbbr() : 'UTC' + a.format('Z')) : '',
    zone: reference ? reference.zoneAbbr() : 'Unknown', offset: reference ? reference.utcOffset() : 2000,
    kind: b ? (actual ? 1 : 0) : raw.cancelled ? 3 : 2,
    delayKnown: Boolean(a && b), delay: a && b ? Math.round((b.valueOf() - a.valueOf()) / 60000) : 0,
    terminal: field(raw, departure ? 'terminal_origin' : 'terminal_destination'),
    facility: field(raw, departure ? 'gate_origin' : 'baggage_claim')
  };
}
function operational(raw, departure) {
  if (raw.cancelled) { return 5; }
  if (raw.diverted) { return 6; }
  if (raw.actual_on || raw.actual_in) { return 3; }
  if (raw.actual_off) { return 2; }
  if (raw.actual_out) { return 7; }
  if (/boarding/i.test(raw.status || '')) { return 1; }
  if (departure.delayKnown && departure.delay > 0) { return 4; }
  return raw.scheduled_out || raw.scheduled_off ? 0 : 8;
}
function toMessage(configured, record, legacy, index, count, generation, state, timeFormat) {
  state = state || {};
  var raw = record && record.flight || {};
  var departure = endpoint(raw, true, timeFormat), arrival = endpoint(raw, false, timeFormat);
  var number = configured.flightNumber;
  var hasRaw = Boolean(record && record.flight);
  var booking = {value: configured.bookingCode || '', state: configured.bookingCode ? 0 : UNKNOWN};
  var seat = {value: configured.seatNumber || '', state: configured.seatNumber ? 0 : UNKNOWN};
  var result = {
    UI_COUNT: count, UI_INDEX: index, UI_GENERATION: generation,
    UI_TIME_FORMAT: times.timeFormat(timeFormat) === '12H' ? 12 : 24,
    UI_KEY: number + '|' + configured.flightDate + '|' + index,
    FLIGHT_NUMBER: number, FLIGHT_DATE: times.dateLabel(configured.flightDate),
    ORIGIN: departure.code !== '---' ? departure.code : legacy && legacy.ORIGIN || '---',
    DESTINATION: arrival.code !== '---' ? arrival.code : legacy && legacy.DESTINATION || '---',
    UI_DEP_NAME: departure.name, UI_ARR_NAME: arrival.name,
    UI_DEP_SCHEDULED: departure.scheduled, UI_ARR_SCHEDULED: arrival.scheduled,
    DEPARTURE_TIME: departure.current, ARRIVAL_TIME: arrival.current,
    UI_DEP_DATE: departure.date, UI_ARR_DATE: arrival.date,
    UI_DEP_SCHEDULED_DATE: departure.scheduledDate, UI_ARR_SCHEDULED_DATE: arrival.scheduledDate,
    UI_DEP_SCHEDULED_ZONE: departure.scheduledZone, UI_ARR_SCHEDULED_ZONE: arrival.scheduledZone,
    UI_DEP_ZONE: departure.zone, UI_ARR_ZONE: arrival.zone,
    UI_DEP_OFFSET: departure.offset, UI_ARR_OFFSET: arrival.offset,
    UI_DEP_KIND: departure.kind, UI_ARR_KIND: arrival.kind,
    UI_DEP_DELAY: departure.delay, UI_ARR_DELAY: arrival.delay,
    UI_DEP_DELAY_KNOWN: departure.delayKnown ? 1 : 0,
    UI_ARR_DELAY_KNOWN: arrival.delayKnown ? 1 : 0,
    UI_STATUS: hasRaw ? operational(raw, departure) : 8,
    UI_PROGRESS: typeof raw.progress_percent === 'number' && isFinite(raw.progress_percent) ? Math.round(raw.progress_percent) : -1,
    DEPARTURE_TERMINAL: departure.terminal.value, UI_DEP_TERMINAL_STATE: departure.terminal.state,
    DEPARTURE_GATE: departure.facility.value, UI_DEP_GATE_STATE: departure.facility.state,
    ARRIVAL_TERMINAL: arrival.terminal.value, UI_ARR_TERMINAL_STATE: arrival.terminal.state,
    UI_BAGGAGE: arrival.facility.value, UI_BAGGAGE_STATE: arrival.facility.state,
    BOOKING_CODE: booking.value, UI_BOOKING_STATE: booking.state,
    SEAT_NUMBER: seat.value, UI_SEAT_STATE: seat.state,
    SEAT_POSITION: (configured.seatPosition || '').toUpperCase(),
    UI_UPDATED: record && isFinite(record.updatedAt) ? times.updatedLabel(record.updatedAt, timeFormat) : 'Never',
    UI_STALE: !hasRaw || state.stale || state.error ? 1 : 0,
    IS_LOADING: state.loading ? 1 : 0, ERROR_MESSAGE: state.error || '',
    NEXT_REFRESH_AT: legacy && legacy.NEXT_REFRESH_AT || '--'
  };
  ['AIRCRAFT_MODEL', 'AIRCRAFT_NUMBER', 'AIRCRAFT_FIRST_FLIGHT', 'AIRCRAFT_LEG_1',
    'AIRCRAFT_LEG_1_STATUS', 'AIRCRAFT_LEG_2', 'AIRCRAFT_LEG_2_STATUS'].forEach(function(key) {
    result[key] = legacy && legacy[key] || 'Unavailable';
  });
  return result;
}
// One message in flight. New snapshots replace pending ones, avoiding stale
// backlogs when HTTP callbacks or quick button presses overlap.
function sender(pebble) {
  var pending = null, running = false;
  function flush() {
    if (running || !pending) { return; }
    var frames = pending; pending = null; running = true;
    var index = 0;
    function next() {
      if (index === frames.length) { running = false; flush(); return; }
      var frame = frames[index++], attempts = 0;
      function attempt() {
        pebble.sendAppMessage(frame, next, function() {
          if (++attempts < 3) { setTimeout(attempt, 250); }
          else { running = false; flush(); }
        });
      }
      attempt();
    }
    next();
  }
  return function(frames) { pending = frames; flush(); };
}
module.exports = {toMessage: toMessage, sender: sender};
