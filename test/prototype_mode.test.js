'use strict';
var assert = require('assert');
var vm = require('vm');
var fs = require('fs');
var handlers = {};
var loads = 0;
var starts = 0;
var messages = [];
vm.runInNewContext(fs.readFileSync('src/pkjs/index.js', 'utf8'), {
  Pebble: {
    addEventListener: function(name, fn) { handlers[name] = fn; },
    sendAppMessage: function(payload) { messages.push(payload); }
  },
  // Deliberately omit localStorage, XMLHttpRequest, and credentials.
  require: function(path) {
    assert.strictEqual(path, './live');
    loads++;
    return {start: function() { starts++; }};
  }
});
handlers.ready();
handlers.appmessage({payload: {}});
handlers.appmessage({payload: {LIVE_MODE: 0}});
assert.strictEqual(loads, 0, 'mock startup must not load any production service');
assert.strictEqual(messages.length, 1);
assert.strictEqual(messages[0].REQUEST_MODE, 1);
handlers.appmessage({payload: {LIVE_MODE: 1}});
handlers.appmessage({payload: {LIVE_MODE: 1}});
assert.strictEqual(loads, 1);
assert.strictEqual(starts, 1, 'live startup must run once after handshake');
console.log('Prototype isolation and live handshake passed');
