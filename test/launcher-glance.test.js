'use strict';
var assert = require('assert');
var glance = require('../src/pkjs/launcher-glance');
var now = Date.parse('2026-10-19T18:00:00Z');
function configured(number, date) { return {flightNumber: number, flightDate: date || '2026-10-19'}; }
function raw(departure, arrival) { return {
  origin: {code_iata: 'BUR', timezone: 'America/Los_Angeles'},
  destination: {code_iata: 'JFK', timezone: 'America/New_York'},
  scheduled_out: departure, estimated_out: departure, scheduled_in: arrival
}; }
function records(flights, raws) { var cache = {}; flights.forEach(function(f, i) {
  cache[f.flightNumber + '|' + f.flightDate] = {flight: raws[i], updatedAt: now};
}); return cache; }
function slices(flights, cache, time, legacy, states, format) {
  return glance.plan(flights, cache || {}, legacy || {}, states || {}, time || now, format).trim().split('\n').map(function(line) {
    var tab = line.indexOf('\t'); return {expires: Number(line.slice(0, tab)), text: line.slice(tab + 1)};
  });
}
var flights = [configured('AA200'), configured('WN1145'), configured('UA100', '2026-10-20')];
var raws = [raw('2026-10-20T00:00:00Z', '2026-10-20T06:00:00Z'),
  raw('2026-10-19T22:00:00Z', '2026-10-20T03:00:00Z'),
  raw('2026-10-21T00:00:00Z', '2026-10-21T06:00:00Z')];
var cache = records(flights, raws), result = slices(flights, cache);
assert.ok(result[0].text.indexOf('WN1145 · BUR>JFK · Upcoming · DEP EST 15:00 PDT 19 OCT') === 0,
  'next flight is chronological, not selected/saved order, and uses airport-local time');
assert.strictEqual(result[0].expires, Date.parse(raws[1].scheduled_out) / 1000);
assert.strictEqual(result[1].text, 'WN1145 · BUR>JFK · Open for flight status');
assert.ok(result[2].text.indexOf('AA200 · BUR>JFK · Open for flight status') === 0,
  'an overlapping later flight must not retain its old pre-departure state');
assert.strictEqual(result[result.length - 1].expires, 0);
result.slice(0, -1).forEach(function(s, i) {
  assert.ok(s.expires > now / 1000 && (!i || s.expires > result[i-1].expires));
});
assert.ok(result.length <= 7);
result.forEach(function(s) { assert.ok(Buffer.byteLength(s.text, 'utf8') <= 150); });

raws[1].actual_off = '2026-10-19T17:50:00Z';
raws[1].scheduled_out = raws[1].estimated_out = '2026-10-19T17:00:00Z';
result = slices(flights, cache);
assert.ok(result[0].text.includes('In flight · ARR SCHED 23:00 EDT 19 OCT'),
  'airborne flight takes priority and arrival uses destination timezone');
raws[1].actual_on = '2026-10-19T17:59:00Z';
assert.ok(slices(flights, cache)[0].text.startsWith('AA200'), 'landed flights are excluded');
delete raws[1].actual_on; raws[1].cancelled = true;
assert.ok(slices(flights, cache)[0].text.startsWith('AA200'), 'canceled flights are excluded');
delete raws[1].cancelled;
assert.ok(slices(flights, cache, now, {}, {'WN1145|2026-10-19':{actualOn:now-1}})[0].text.startsWith('AA200'),
  'legacy terminal state also excludes a flight');

var unknown = configured('DL500');
result = slices([unknown]);
assert.ok(result[0].text.includes('DL500 · --->--- · 19 OCT 26 · Details not updated'));
assert.ok(!result[0].text.includes('12:00'), 'sorting fallback cannot become an invented flight time');
result = slices([unknown], {}, now, {'DL500|2026-10-19': {ORIGIN:'SFO',DESTINATION:'LHR'}});
assert.ok(result[0].text.includes('SFO>LHR'), 'upgrade retains known old route');
result = slices([unknown], records([unknown], [{scheduled_out:'2026-10-19T22:00:00Z'}]));
assert.ok(result[0].text.includes('Time unknown'), 'an unknown timezone cannot become UTC-as-local');
assert.strictEqual(slices([], {})[0].text, 'Add flights in phone settings');
assert.strictEqual(slices([configured('DL500','2026-10-10')])[0].text, 'No upcoming flights · Open to view all');
assert.ok(!glance.plan([configured('AA1{bad}\t\n')], {}, {}, {}, now).includes('{bad}'),
  'user text cannot inject templates or wire delimiters');

// Three disjoint flights consume exactly 7 slices, within the native capacity.
raws = [raw('2026-10-19T19:00:00Z','2026-10-19T20:00:00Z'),
  raw('2026-10-19T21:00:00Z','2026-10-19T22:00:00Z'),
  raw('2026-10-20T19:00:00Z','2026-10-20T20:00:00Z')];
result = slices(flights, records(flights, raws)); assert.strictEqual(result.length, 7);
flights=[];raws=[];
for (var i=0;i<5;i++) {
  var date='2026-10-'+(19+i);
  flights.push(configured('AA'+(i+1),date));
  raws.push(raw(date+'T20:00:00Z',date+'T22:00:00Z'));
}
cache=records(flights,raws);result=slices(flights,cache);
assert.strictEqual(result.length,11,'five flights need ten expiring slices and one permanent fallback');
var twelve=slices(flights,cache,now,{}, {},'12H');
assert.ok(twelve[0].text.includes('DEP EST 1:00 PM PDT'));
assert.deepStrictEqual(twelve.map(function(s){return s.expires;}),result.map(function(s){return s.expires;}));
twelve.forEach(function(s){assert.ok(Buffer.byteLength(s.text,'utf8')<=150);});
console.log('Launcher chronology, expiry, terminal/empty states, local time, missing data and wire bounds passed');
