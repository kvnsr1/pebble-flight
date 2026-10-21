'use strict';
// Synthetic review data only: these are not actual airline schedules/bookings.
var view=require('../../src/pkjs/native-view');
var base=require('./provider-flight.json');
module.exports=function(format) {
  var numbers=['WN1145','AA2220','UA206','DL418','AS55'];
  return numbers.map(function(number,i){
    var raw=JSON.parse(JSON.stringify(base));
    raw.ident_iata=number;raw.scheduled_out='2026-10-22T23:35:00Z';
    raw.actual_out='2026-10-22T23:47:00Z';raw.actual_off='2026-10-23T00:00:00Z';
    raw.scheduled_in='2026-10-23T10:10:00Z';raw.estimated_in='2026-10-23T09:58:00Z';
    if(i===0){
      raw.origin={code_iata:'JFK',name:'John F. Kennedy International Airport',timezone:'America/New_York'};
      raw.destination={code_iata:'LHR',name:'London Heathrow Airport',timezone:'Europe/London'};
      raw.scheduled_out='2026-10-22T04:05:00Z';raw.scheduled_in='2026-10-22T11:59:00Z';
      delete raw.actual_out;delete raw.actual_off;delete raw.estimated_in;raw.progress_percent=null;
      raw.gate_origin=null;delete raw.baggage_claim;
    }
    if(i===2){delete raw.actual_out;delete raw.actual_off;raw.status='Boarding';raw.estimated_out=raw.scheduled_out;}
    if(i===3){raw.actual_on='2026-10-23T09:40:00Z';raw.actual_in='2026-10-23T09:58:00Z';}
    if(i===4){
      delete raw.actual_out;delete raw.actual_off;
      raw.scheduled_out='2026-10-22T19:05:00Z';raw.estimated_out='2026-10-22T20:20:00Z';raw.gate_origin=null;
    }
    var configured={flightNumber:number,flightDate:'2026-10-22',bookingCode:'DEMO123',seatNumber:'12A',seatPosition:'window'};
    return view.toMessage(configured,{flight:raw,updatedAt:Date.parse('2026-10-21T12:00:00Z')},null,i,5,1,{},format);
  });
};
