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
module.exports = {local: local, dateLabel: dateLabel};
