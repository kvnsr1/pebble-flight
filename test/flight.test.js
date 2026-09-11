'use strict';
var assert = require('assert');
var flight = require('../src/pkjs/flight');

function sample(delayMinutes) {
  return {
    ident_iata: 'AA100',
    scheduled_out: '2026-09-10T10:00:00-07:00',
    estimated_out: new Date(Date.parse('2026-09-10T10:00:00-07:00') + delayMinutes * 60000).toISOString(),
    scheduled_in: '2026-09-10T18:00:00-04:00',
    estimated_in: '2026-09-10T18:10:00-04:00',
    origin: {code_iata: 'LAX'}, destination: {code_iata: 'JFK'},
    terminal_origin: '4', gate_origin: '41', terminal_destination: '8', gate_destination: '12'
  };
}

assert.strictEqual(flight.statusFor(sample(0)).label, 'ON TIME');
assert.strictEqual(flight.statusFor(sample(29)).level, 1);
assert.strictEqual(flight.statusFor(sample(30)).level, 2);
assert.strictEqual(flight.statusFor(sample(60)).level, 2);
assert.strictEqual(flight.statusFor(sample(61)).level, 3);
assert.strictEqual(flight.toMessage(sample(30)).ORIGIN, 'LAX');
assert.strictEqual(flight.chooseFlight([sample(0)], '2026-09-10').ident_iata, 'AA100');
console.log('Flight transformation tests passed');

