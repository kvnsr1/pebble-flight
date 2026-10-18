'use strict';
var assert = require('assert');
var fs = require('fs');
var vm = require('vm');
var raw = require('./fixtures/provider-flight.json');
var configured = {flightNumber:'BA284', flightDate:'2026-10-16'};
var key = 'BA284|2026-10-16';
var now = Date.parse('2026-10-17T02:00:00Z');
function initial(state, record) {
  var storage = {
    aeroApiKey:'SYNTHETIC_OLD_KEY',
    'pebbleFlight.flights':JSON.stringify([configured]),
    'pebbleFlight.refreshState':JSON.stringify((function() { var s={}; s[key]=state; return s; })()),
    'pebbleFlight.flightCache':JSON.stringify((function() {
      var c={}; c[key]={FLIGHT_NUMBER:'BA284',FLIGHT_DATE:'2026-10-16',ORIGIN:'SFO',DESTINATION:'LHR'}; return c;
    })())
  };
  if (record) { var cache={}; cache[key]=record; storage['pebbleFlight.rawCache']=JSON.stringify(cache); }
  return storage;
}
function harness(storage, entry) {
  var sent=[], requests=[], handlers={}, loaded={}, clock={now:now}, failedWrite='';
  class Clock extends Date {
    constructor() { super(arguments.length ? arguments[0] : clock.now); }
    static now() { return clock.now; }
  }
  var context=vm.createContext({console:console,Date:Clock,setTimeout:function() {
    throw new Error('Unexpected AppMessage retry in successful transport');
  },localStorage:{getItem:function(k) { return storage[k] || null; },setItem:function(k,v) {
    if (failedWrite===k) { failedWrite=''; throw new Error('Synthetic storage failure'); }
    storage[k]=String(v);
  }},
    Pebble:{addEventListener:function(k,fn) { (handlers[k] || (handlers[k]=[])).push(fn); },
      sendAppMessage:function(m,success) { sent.push(m); if (success) { success(); } },openURL:function() {}},
    XMLHttpRequest:function() {
      requests.push(this); this.headers={};
      this.open=function(method,url) { this.method=method; this.url=url; };
      this.setRequestHeader=function(k,v) { this.headers[k]=v; }; this.send=function() {};
    }});
  function load(name) {
    name=name.replace('./','');
    if (loaded[name]) { return loaded[name].exports; }
    var module=loaded[name]={exports:{}};
    vm.runInContext('(function(require,module,exports) {\n'+fs.readFileSync('src/pkjs/'+name+'.js','utf8')+'\n})',context)(
      function(path) { return path.indexOf('./')===0 ? load(path) : require(path); },module,module.exports);
    return module.exports;
  }
  var service=load(entry ? 'index' : 'live');
  return {sent:sent,requests:requests,clock:clock,service:service,
    failNextWrite:function(key) { failedWrite=key; },
    fire:function(event,value) { (handlers[event] || []).slice().forEach(function(fn) { fn(value || {}); }); },
    save:function(token) { this.fire('webviewclosed',{response:encodeURIComponent(JSON.stringify({apiKey:token,flights:[configured]}))}); },
    complete:function(index,status) {
      var request=requests[index]; request.status=status || 200;
      request.responseText=JSON.stringify({flights:[raw]}); request.onload();
    },
    latest:function() { return sent.filter(function(m) { return m.UI_COUNT===1; }).slice(-1)[0]; }
  };
}

// Existing installs have legacy fields and refresh state, but no new raw cache.
[
  {lastRequestAt:now,scheduledOut:now+7200000},
  {lastRequestAt:now,actualOn:now-3600000},
  {lastRequestAt:now,cancelled:true}
].forEach(function(state) {
  var h=harness(initial(state)); h.service.start({nativeUI:true});
  assert.strictEqual(h.requests.length,1,'upgrade must bootstrap native data despite old refresh/terminal state');
  assert.strictEqual(h.requests[0].headers['x-apikey'],'SYNTHETIC_OLD_KEY');
  h.complete(0);
  assert.strictEqual(h.latest().UI_STATUS,2); assert.strictEqual(h.latest().ORIGIN,'SFO');
  assert.strictEqual(h.latest().DEPARTURE_TIME,'16:47'); assert.strictEqual(h.latest().UI_DEP_ZONE,'PDT');
  assert.strictEqual(h.latest().ERROR_MESSAGE,'');
  assert.ok(h.latest().UI_UPDATED.indexOf('2026-10-17')===0);
  h.service.start({nativeUI:true});
  assert.strictEqual(h.requests.length,1,'successful bootstrap returns to the normal schedule');
});

// Failed bootstrap requests retain a short cooldown, rather than retrying at
// every selection or mode hint. A successful new key explicitly bypasses it.
var h=harness(initial({lastRequestAt:now})); h.service.start({nativeUI:true});
h.complete(0,401);
assert.ok(h.latest().ERROR_MESSAGE.indexOf('key was rejected')>=0);
h.service.start({nativeUI:true}); h.service.sync();
assert.strictEqual(h.requests.length,1);
h.save('SYNTHETIC_NEW_KEY');
assert.strictEqual(h.requests.length,2,'saving a new key immediately retries the lookup');
assert.strictEqual(h.requests[1].headers['x-apikey'],'SYNTHETIC_NEW_KEY');
h.complete(1); assert.strictEqual(h.latest().UI_STATUS,2);
h.save(''); assert.strictEqual(h.requests.length,2,'blank key preserves saved key and cooldown');
assert.ok(!JSON.stringify(h.sent).includes('SYNTHETIC_'),'credentials must never enter watch messages');

h=harness(initial({lastRequestAt:now})); h.service.start({nativeUI:true});
h.failNextWrite('pebbleFlight.rawCache'); h.complete(0);
assert.strictEqual(h.latest().ERROR_MESSAGE,'Could not read flight data','cache errors must remain visible in the flight note');

// Replacing credentials while an old request is pending cannot let that old
// callback overwrite the new result or cancel the new request.
h=harness(initial({lastRequestAt:now})); h.service.start({nativeUI:true});
h.save('SYNTHETIC_NEW_KEY'); assert.strictEqual(h.requests.length,2);
h.complete(0,401); assert.strictEqual(h.latest().IS_LOADING,1);
h.complete(1); var good=h.latest(); h.requests[0].onerror();
assert.strictEqual(h.latest(),good); assert.strictEqual(good.ERROR_MESSAGE,'');

// A replacement key also bypasses the old interval when raw data already exists.
var record={flight:raw,updatedAt:now};
h=harness(initial({lastRequestAt:now,nativeRequestedAt:now,actualOut:now},record));
h.service.start({nativeUI:true}); assert.strictEqual(h.requests.length,0);
h.save('SYNTHETIC_NEW_KEY'); assert.strictEqual(h.requests.length,1);

// Persisted generations allow a restarted phone companion to update an already
// open watch app without waiting for its counter to catch up.
var storage=initial({lastRequestAt:now,actualOut:now},record);
h=harness(storage); h.service.start({nativeUI:true}); h.service.sync();
var generation=h.latest().UI_GENERATION;
var restarted=harness(storage); restarted.service.start({nativeUI:true});
assert.ok(restarted.latest().UI_GENERATION>generation);

// Test the real entry point with multiple event listeners, including opening
// settings before ready. Only the entry point starts the production service.
h=harness(initial({lastRequestAt:now}),true);
h.fire('showConfiguration'); assert.strictEqual(h.requests.length,0);
h.fire('ready'); assert.strictEqual(h.requests.length,0,'wait for native mode confirmation');
h.fire('appmessage',{payload:{LIVE_MODE:2}}); assert.strictEqual(h.requests.length,1);
h.complete(0); assert.strictEqual(h.latest().UI_STATUS,2);
assert.ok(h.sent.filter(function(m) { return !m.REQUEST_MODE; }).every(function(m) { return m.UI_COUNT===1; }),
  'settings/ready must not silently switch back to the legacy wire format');
h.fire('appmessage',{payload:{LIVE_MODE:2}}); assert.strictEqual(h.requests.length,1,'duplicate mode hints only resync');

console.log('Native upgrade bootstrap, credential replacement/races, restart ordering and real entry handshake passed');
