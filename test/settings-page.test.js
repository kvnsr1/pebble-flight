'use strict';
var assert=require('assert'),fs=require('fs'),vm=require('vm');
var html=fs.readFileSync('config/index.html','utf8');
var script=html.match(/<script>([\s\S]*?)<\/script>/)[1];
function page(state) {
  var elements={},children=[];
  function element(id) {
    return elements[id] || (elements[id]={id:id,value:id.indexOf('seatPosition')===0?'auto':'',
      options:[{}],style:{setProperty:function(){}},classList:{toggle:function(){}},handlers:{},
      addEventListener:function(name,fn){this.handlers[name]=fn;},
      setCustomValidity:function(message){this.error=message;},reportValidity:function(){},focus:function(){},
      appendChild:function(child){children.push(child);elements[child.id]=child;}});
  }
  var location={hash:state?'#state='+encodeURIComponent(JSON.stringify(state)):'',search:'',href:''};
  var document={getElementById:element,createElement:function(){return element('new'+children.length);}};
  vm.runInNewContext(script,{document:document,location:location,URLSearchParams:URLSearchParams,
    Date:Date,requestAnimationFrame:function(fn){fn();}});
  return {elements:elements,cards:children,submit:function(){
    elements.settings.handlers.submit({preventDefault:function(){}});
    return location.href?JSON.parse(decodeURIComponent(location.href.split('#')[1])):null;
  }};
}
var five=Array.from({length:5},function(_,i){return {flightNumber:'AA'+(i+1),flightDate:'2026-10-22',seatNumber:'12A'};});
var p=page({flights:five,timeFormat:'12H',hasApiKey:true});
assert.strictEqual(p.cards.length,5);assert.strictEqual(p.elements.timeFormat.value,'12H');
assert.strictEqual(p.elements.flightNumber5.value,'AA5');
var saved=p.submit();assert.strictEqual(saved.flights.length,5);assert.strictEqual(saved.timeFormat,'12H');
assert.strictEqual(saved.apiKey,'');assert.strictEqual(saved.flights[4].seatPosition,'window');
p.elements.clearFlight5.handlers.click();saved=p.submit();
assert.strictEqual(saved.flights.length,4);assert.strictEqual(saved.timeFormat,'12H');
p.elements.flightNumber5.value='AS55';p.elements.flightDate5.value='2026-10-23';
p.elements.seatNumber5.value='7C';p.elements.timeFormat.value='24H';
saved=p.submit();assert.strictEqual(saved.flights.length,5);assert.strictEqual(saved.timeFormat,'24H');
assert.strictEqual(saved.flights[4].seatPosition,'aisle');
var old=page({flights:five.slice(0,3),hasApiKey:true});
assert.strictEqual(old.cards.length,3);assert.strictEqual(old.elements.timeFormat.disabled,true);
assert.ok(old.elements.timeFormatHelp.textContent.includes('0.7.8'));
assert.strictEqual(old.submit().timeFormat,undefined);
p=page({flights:five,timeFormat:'12H'});p.elements.flightDate5.value='';
assert.strictEqual(p.submit(),null);assert.ok(p.elements.flightDate5.error.includes('departure date'));
console.log('Settings page: five-card save, clear, format roundtrip, validation and old-app compatibility passed');
