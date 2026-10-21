'use strict';
var assert = require('assert');
var vm = require('vm');
var fs = require('fs');
var view = require('../src/pkjs/native-view');
var raw = require('./fixtures/provider-flight.json');
function clone(value) { return JSON.parse(JSON.stringify(value)); }
var configured = {flightNumber:'BA284', flightDate:'2026-10-16', bookingCode:'ABC123', seatNumber:'8C', seatPosition:'window', seatPositionOverride:true};
var record = {flight:raw, updatedAt:Date.parse('2026-10-17T03:00:00Z')};
function frame(flight) { return view.toMessage(configured,{flight:flight,updatedAt:record.updatedAt},null,0,1,1); }
var message = frame(raw);
var duplicate=view.toMessage(configured,record,null,1,2,1);
assert.notStrictEqual(message.UI_KEY,duplicate.UI_KEY,'duplicate saved entries retain their own booking selection');
assert.strictEqual(message.DEPARTURE_TIME,'16:47');
assert.strictEqual(message.ARRIVAL_TIME,'10:58');
assert.strictEqual(message.UI_DEP_ZONE,'PDT');
assert.strictEqual(message.UI_ARR_ZONE,'BST');
assert.strictEqual(message.UI_DEP_OFFSET,-420);
assert.strictEqual(message.UI_ARR_OFFSET,60);
assert.strictEqual(message.UI_DEP_DATE,'16 OCT');
assert.strictEqual(message.UI_ARR_DATE,'17 OCT');
assert.strictEqual(message.UI_DEP_KIND,1);
assert.strictEqual(message.UI_ARR_KIND,0);
assert.strictEqual(message.UI_DEP_DELAY,12);
assert.strictEqual(message.UI_ARR_DELAY,-12);
assert.strictEqual(message.UI_STATUS,2);
assert.strictEqual(message.UI_PROGRESS,62);
assert.strictEqual(message.UI_BAGGAGE_STATE,1);
assert.strictEqual(message.SEAT_POSITION,'WINDOW');
assert.strictEqual(message.UI_TIME_FORMAT,24);
var twelve=view.toMessage(configured,record,null,0,1,1,{},'12H');
assert.strictEqual(twelve.UI_TIME_FORMAT,12);
assert.strictEqual(twelve.UI_DEP_SCHEDULED,'4:35 PM');
assert.strictEqual(twelve.DEPARTURE_TIME,'4:47 PM');
assert.strictEqual(twelve.ARRIVAL_TIME,'10:58 AM');
assert.strictEqual(twelve.UI_UPDATED,'2026-10-17 3:00 AM UTC');
['UI_DEP_DATE','UI_ARR_DATE','UI_DEP_ZONE','UI_ARR_ZONE','UI_DEP_OFFSET','UI_ARR_OFFSET','UI_DEP_DELAY','UI_ARR_DELAY','UI_STATUS','UI_PROGRESS'].forEach(function(key) {
  assert.strictEqual(twelve[key],message[key], 'formatting must not change '+key);
});
var midnight=clone(raw);midnight.scheduled_out=midnight.actual_out='2026-10-17T07:05:00Z';
assert.strictEqual(view.toMessage(configured,{flight:midnight},null,0,1,1,{},'12H').DEPARTURE_TIME,'12:05 AM');
midnight.scheduled_out=midnight.actual_out='2026-10-17T19:05:00Z';
assert.strictEqual(view.toMessage(configured,{flight:midnight},null,0,1,1,{},'12H').DEPARTURE_TIME,'12:05 PM');
assert.strictEqual(view.toMessage(configured,record,null,0,1,1,{},'invalid').DEPARTURE_TIME,'16:47');
var dst=clone(raw); dst.origin.timezone='America/New_York';
dst.scheduled_out='2026-11-01T05:30:00Z'; dst.actual_out='2026-11-01T06:30:00Z';
assert.strictEqual(frame(dst).UI_DEP_SCHEDULED_ZONE,'EDT');
assert.strictEqual(frame(dst).UI_DEP_ZONE,'EST');
assert.strictEqual(frame(dst).UI_DEP_DELAY,60);
var old=view.toMessage(configured,null,{ORIGIN:'SFO',DESTINATION:'LHR'},0,1,1,{});
assert.strictEqual(old.ORIGIN,'SFO'); assert.strictEqual(old.UI_STALE,1);
assert.strictEqual(old.UI_DEP_KIND,2); assert.strictEqual(old.UI_DEP_SCHEDULED,'--:--');
var altered=clone(raw); delete altered.baggage_claim;
assert.strictEqual(frame(altered).UI_BAGGAGE_STATE,2);
altered.progress_percent=null;
assert.strictEqual(frame(altered).UI_PROGRESS,-1);
altered.cancelled=true; delete altered.actual_out; delete altered.actual_off;
assert.strictEqual(frame(altered).UI_STATUS,5);
assert.strictEqual(frame(altered).UI_DEP_KIND,3);
altered=clone(raw); altered.destination.timezone='Asia/Kathmandu';
assert.strictEqual(frame(altered).UI_ARR_OFFSET,345);
altered.destination.timezone='Invalid/Timezone';
assert.strictEqual(frame(altered).ARRIVAL_TIME,'');
assert.strictEqual(frame(altered).UI_ARR_OFFSET,2000);
altered=clone(raw); delete altered.actual_off;
assert.strictEqual(frame(altered).UI_STATUS,7);
altered.actual_on='2026-10-17T09:40:00Z';
assert.strictEqual(frame(altered).UI_STATUS,3);
// Landing does not invent an actual gate-arrival time.
assert.strictEqual(frame(altered).UI_ARR_KIND,0);
altered.diverted=true;
assert.strictEqual(frame(altered).UI_STATUS,6);
altered={origin:raw.origin,destination:raw.destination,scheduled_off:raw.actual_off,actual_off:raw.actual_off};
assert.strictEqual(frame(altered).UI_DEP_DELAY_KNOWN,0);
assert.strictEqual(frame(altered).UI_DEP_SCHEDULED,'--:--');
var airport = require('../src/pkjs/airport-time');
assert.strictEqual(airport.local('2026-11-01T05:30:00Z','America/New_York').format('HH:mm z'),'01:30 EDT');
assert.strictEqual(airport.local('2026-11-01T06:30:00Z','America/New_York').format('HH:mm z'),'01:30 EST');
// The sender coalesces whole snapshots while preserving one in-flight frame.
var delivered=[], ack=[];
var sender=view.sender({sendAppMessage:function(payload,success){delivered.push(payload);ack.push(success);}});
sender([{generation:1,index:0},{generation:1,index:1}]);
sender([{generation:2,index:0}]); sender([{generation:3,index:0}]);
ack.shift()(); ack.shift()(); ack.shift()();
assert.deepStrictEqual(delivered.map(function(f){return f.generation;}),[1,1,3]);
// Exercise the real companion with synthetic HTTP responses and old storage keys.
var storage={'pebbleFlight.flights':JSON.stringify([configured,{flightNumber:'AA100',flightDate:'2026-10-16'}])};
var sent=[], handlers={}, requests=[], url='';
var context={console:console,Date:Date,JSON:JSON,Number:Number,encodeURIComponent:encodeURIComponent,decodeURIComponent:decodeURIComponent,module:{exports:{}},
  localStorage:{getItem:function(k){return storage[k]||null;},setItem:function(k,v){storage[k]=String(v);}},
  Pebble:{addEventListener:function(k,fn){handlers[k]=fn;},sendAppMessage:function(payload,success){sent.push(payload);success();},openURL:function(value){url=value;}},
  XMLHttpRequest:function(){requests.push(this);this.open=function(){};this.setRequestHeader=function(){};this.send=function(){};},
  require:function(path){return require('../src/pkjs/'+path.replace('./',''));}};
vm.runInNewContext(fs.readFileSync('src/pkjs/live.js','utf8'),context);
context.module.exports.configure({nativeUI:true});
assert.ok(url.indexOf('#state=')>0);
assert.strictEqual(requests.length,0,'opening phone settings must not query the flight service');
context.module.exports.start({nativeUI:true});
assert.ok(sent.every(function(m){return typeof m.UI_COUNT==='number' || typeof m.GLANCE_PLAN==='string';}));
assert.strictEqual(sent.slice(-2)[0].ERROR_MESSAGE.indexOf('AeroAPI')>=0,true);
storage.aeroApiKey='SYNTHETIC_TEST_VALUE';
handlers.appmessage({payload:{REQUEST_SELECT_FLIGHT:0,REQUEST_REFRESH:1}});
assert.strictEqual(requests.length,1);
// Select another flight before the first response, then complete the first request.
handlers.appmessage({payload:{REQUEST_SELECT_FLIGHT:1,REQUEST_REFRESH:1}});
assert.strictEqual(requests.length,2);
requests[0].status=200; requests[0].responseText=JSON.stringify({flights:[raw]}); requests[0].onload();
var frames=sent.slice(-2);
assert.strictEqual(frames[0].FLIGHT_NUMBER,'BA284');
assert.strictEqual(frames[0].UI_PROGRESS,62);
assert.strictEqual(frames[0].BOOKING_CODE,'ABC123');
assert.strictEqual(frames[1].FLIGHT_NUMBER,'AA100');
assert.strictEqual(frames[1].UI_STATUS,8,'late responses must not populate another flight');
requests[1].onerror();
assert.ok(sent[sent.length-1].ERROR_MESSAGE.indexOf('FlightAware')>=0);
// The new All Flights list retains logged landed flights until removed in settings.
var states=JSON.parse(storage['pebbleFlight.refreshState']);
states['BA284|2026-10-16'].actualOn=Date.now()-7200000;
storage['pebbleFlight.refreshState']=JSON.stringify(states);
context.module.exports.start({nativeUI:true});
assert.strictEqual(JSON.parse(storage['pebbleFlight.flights']).length,2);
states['BA284|2026-10-16'].lastRequestAt=Date.now()-31000;
states['BA284|2026-10-16'].nativeRequestedAt=Date.now()-31000;
storage['pebbleFlight.refreshState']=JSON.stringify(states);
handlers.appmessage({payload:{REQUEST_SELECT_FLIGHT:0,REQUEST_REFRESH:1}});
assert.strictEqual(requests.length,3,'explicit refresh may retrieve baggage after landing');
requests[2].onerror();
var priorRequests=requests.length;
context.module.exports.sync();
assert.strictEqual(requests.length,priorRequests,'reconnect resync sends cache without starting another lookup');
// Clearing all saved flights is allowed and produces the native empty list.
handlers.webviewclosed({response:encodeURIComponent(JSON.stringify({apiKey:'',flights:[]}))});
assert.strictEqual(JSON.parse(storage['pebbleFlight.flights']).length,0);
assert.strictEqual(sent[sent.length-1].UI_COUNT,0);
assert.ok(!JSON.stringify(sent).includes('SYNTHETIC_TEST_VALUE'),'keys never enter the native payload');
console.log('Native mapping, timezone/DST, snapshot queue, settings, HTTP races, errors and empty list passed');
