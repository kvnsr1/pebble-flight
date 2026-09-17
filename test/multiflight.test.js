'use strict';

var assert = require('assert');
var fs = require('fs');
var vm = require('vm');

var handlers = {};
var sent = [];
var openedUrl = '';
var storage = {
  'pebbleFlight.flights': JSON.stringify([
    {flightNumber: 'aa100', flightDate: '2026-10-01', bookingCode: 'ABC123', seatNumber: '12A', seatPosition: 'window'},
    {flightNumber: 'UA200', flightDate: '2026-10-02'},
    {flightNumber: 'DL300', flightDate: '2026-10-03'},
    {flightNumber: 'WN400', flightDate: '2026-10-04'}
  ])
};

var context = {
  console: console,
  Date: Date,
  JSON: JSON,
  encodeURIComponent: encodeURIComponent,
  decodeURIComponent: decodeURIComponent,
  localStorage: {
    getItem: function(key) { return storage[key] || null; },
    setItem: function(key, value) { storage[key] = String(value); }
  },
  Pebble: {
    addEventListener: function(name, handler) { handlers[name] = handler; },
    sendAppMessage: function(message, success) { sent.push(message); if (success) { success(); } },
    openURL: function(url) { openedUrl = url; }
  },
  XMLHttpRequest: function() {
    this.open = function() {};
    this.setRequestHeader = function() {};
    this.send = function() {};
  },
  require: function(path) {
    assert.strictEqual(path, './flight');
    return require('../src/pkjs/flight');
  }
};

vm.runInNewContext(fs.readFileSync('src/pkjs/index.js', 'utf8'), context);

handlers.showConfiguration();
var configState = JSON.parse(decodeURIComponent(openedUrl.split('#state=')[1]));
assert.strictEqual(configState.flights[0].flightNumber, 'AA100');
assert.strictEqual(configState.flights[0].bookingCode, 'ABC123');
assert.strictEqual(configState.flights[0].seatPosition, 'window');
assert.strictEqual(configState.flights[2].flightNumber, 'DL300');
assert.strictEqual(openedUrl.indexOf('WN400'), -1);

handlers.appmessage({payload: {REQUEST_NEXT_FLIGHT: 1}});
assert.strictEqual(storage['pebbleFlight.activeIndex'], '1');
assert.strictEqual(sent[0].FLIGHT_NUMBER, 'UA200');

handlers.webviewclosed({
  response: encodeURIComponent(JSON.stringify({
    apiKey: '',
    flights: [
      {flightNumber: 'AS1', flightDate: '2026-11-01', bookingCode: 'NEW456', seatNumber: '8C', seatPosition: 'aisle'},
      {flightNumber: 'AS2', flightDate: '2026-11-02'},
      {flightNumber: 'AS3', flightDate: '2026-11-03'},
      {flightNumber: 'AS4', flightDate: '2026-11-04'}
    ]
  }))
});
assert.strictEqual(JSON.parse(storage['pebbleFlight.flights']).length, 3);
assert.strictEqual(JSON.parse(storage['pebbleFlight.flights'])[0].seatPosition, 'aisle');
assert.strictEqual(storage['pebbleFlight.activeIndex'], '0');

console.log('Multi-flight settings tests passed');
