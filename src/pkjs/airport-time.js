'use strict';
// Bundled IANA data works in PebbleKit JS / QuickJS without Intl.
var moment = require('moment-timezone');
function local(iso, zone) {
  if (!iso || !zone || !moment.tz.zone(zone)) { return null; }
  var value = moment.parseZone(iso, moment.ISO_8601, true);
  return value.isValid() ? value.tz(zone) : null;
}
function dateLabel(date) {
  var value = moment.utc(date, 'YYYY-MM-DD', true);
  return value.isValid() ? value.format('DD MMM YY').toUpperCase() : 'Unknown';
}
function timeFormat(value) { return value === '12H' ? '12H' : '24H'; }
function clockPattern(value) { return timeFormat(value) === '12H' ? 'h:mm A' : 'HH:mm'; }
function updatedLabel(epoch, format) {
  var value = moment.utc(epoch);
  return value.isValid() ? value.format('YYYY-MM-DD ' + clockPattern(format)) + ' UTC' : 'Never';
}
module.exports = {local: local, dateLabel: dateLabel, timeFormat: timeFormat,
  clockPattern: clockPattern, updatedLabel: updatedLabel};
