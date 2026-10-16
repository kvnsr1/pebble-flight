'use strict';
var assert = require('assert');
var vm = require('vm');
var fs = require('fs');
var handlers = {};
var loads = 0;
var starts = 0;
var messages = [];
var configured = 0;
vm.runInNewContext(fs.readFileSync('src/pkjs/index.js', 'utf8'), {
  Pebble: {
    addEventListener: function(name, fn) { handlers[name] = fn; },
    sendAppMessage: function(payload) { messages.push(payload); }
  },
  // Deliberately omit localStorage, XMLHttpRequest, and credentials.
  require: function(path) {
    assert.strictEqual(path, './live');
    loads++;
    return {start: function() { starts++; }, configure: function(options) { configured++; assert.strictEqual(options.nativeUI, false); }};
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
handlers.showConfiguration();
assert.strictEqual(configured, 1);
assert.strictEqual(loads, 1);
console.log('Prototype isolation and live handshake passed');

var recoveryHandlers={}, recoveryStarts=0, recoverySyncs=0;
vm.runInNewContext(fs.readFileSync('src/pkjs/index.js', 'utf8'), {
  Pebble:{addEventListener:function(k,fn){recoveryHandlers[k]=fn;},sendAppMessage:function(){}},
  require:function(){return {start:function(options){assert.strictEqual(options.nativeUI,true);recoveryStarts++;},sync:function(){recoverySyncs++;}};}
});
recoveryHandlers.appmessage({payload:{LIVE_MODE:2}});
recoveryHandlers.appmessage({payload:{LIVE_MODE:2}});
assert.strictEqual(recoveryStarts,1);assert.strictEqual(recoverySyncs,1);
