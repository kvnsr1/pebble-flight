#!/usr/bin/env python3
"""Verify the real message/render path using only synthetic emulator messages.

Install the --live-test bundle first. Its phone companion is inert. This script
never loads a credential, writes phone settings, or calls a provider endpoint.
"""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import time
from uuid import UUID
from PIL import ImageChops
from libpebble2.services.appmessage import AppMessageService, Int32, CString

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('motion_capture',ROOT/'scripts/review-motion.py')
m=importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
connection=m.connection
service=AppMessageService(connection)
keys=json.loads((ROOT/'package.json').read_text())['pebble']['messageKeys']
target=UUID('b8cf4ac3-4e1a-4e92-9c28-99631fab925c')
node=Path(os.environ.get('PEBBLE_TOOLS_DIR',Path.home()/'Developer/pebble-flight/.tools'))/'node/bin/node'
js="""var view=require('./src/pkjs/native-view');var raw=require('./test/fixtures/provider-flight.json');
var settings=[{flightNumber:'BA284',flightDate:'2026-10-16',bookingCode:'ABC123',seatNumber:'8C',seatPosition:'window'},
{flightNumber:'AA100',flightDate:'2026-10-16'},{flightNumber:'UA206',flightDate:'2026-10-16'}];
console.log(JSON.stringify(settings.map(function(s,i){var f=JSON.parse(JSON.stringify(raw));f.ident_iata=s.flightNumber;
return view.toMessage(s,{flight:f,updatedAt:Date.now()},{},i,3,900,{});})));"""
frames=json.loads(subprocess.check_output([str(node),'-e',js],cwd=ROOT,text=True))
generation=int(time.time())

def snapshot(changes=None):
    global generation
    generation+=1
    if changes:
        frames[0].update(changes)
    for frame in frames:
        frame['UI_GENERATION']=generation
        frame['UI_STALE']=0
        dictionary={keys[k]:Int32(v) if isinstance(v,int) else CString(v) for k,v in frame.items()}
        service.send_message(target,dictionary)
        time.sleep(.04)

snapshot()
time.sleep(.4)
m.click('select')
initial=m.shot('live-overview')
m.record_action('live-progress-update',lambda:snapshot({'UI_PROGRESS':80}),duration=.8)
progress=m.shot('live-progress-settled')
assert ImageChops.difference(initial.crop((56,40,140,68)),progress.crop((56,40,140,68))).getbbox(), 'New percentage not rendered'
time.sleep(.8)
m.same(progress,m.grab(),'Confirmed progress remains still after update')
m.click('down')
gate_before=m.grab()
m.record_action('live-gate-update',lambda:snapshot({'DEPARTURE_GATE':'B12','UI_DEP_GATE_STATE':0}),duration=.85)
gate=m.shot('live-gate-settled')
assert ImageChops.difference(gate_before.crop((106,207,188,228)),gate.crop((106,207,188,228))).getbbox(), 'Updated gate not rendered'
time.sleep(.8)
m.same(gate,m.grab(),'Gate emphasis expires and the page becomes still')
m.click('up')
m.record_action('live-refresh-activity',lambda:snapshot({'IS_LOADING':1}),duration=.7)
waiting=m.grab()
time.sleep(.15)
assert ImageChops.difference(waiting,m.grab()).getbbox(), 'Refresh activity did not advance'
# A failure with a stuck loading bit must stop activity and preserve existing data.
snapshot({'ERROR_MESSAGE':'Synthetic refresh failure; saved data retained','IS_LOADING':1})
time.sleep(.3)
failed=m.shot('live-refresh-error')
time.sleep(.6)
m.same(failed,m.grab(),'Refresh failure stops activity even if loading bit remains set')
m.same(progress.crop((40,24,156,140)),failed.crop((40,24,156,140)), 'Refresh activity never moves the journey ring')
snapshot({'ERROR_MESSAGE':'','IS_LOADING':0})
time.sleep(.3)
before_aircraft=m.grab()
m.click('select',850)
m.shot('live-actions')
m.click('down')
m.click('select')
m.shot('live-aircraft')
m.click('back')
m.same(before_aircraft,m.grab(),'Native action menu and Aircraft return preserve the main detail')
snapshot({'ERROR_MESSAGE':'','IS_LOADING':0,'UI_PROGRESS':-1})
time.sleep(.4)
unknown=m.shot('live-progress-unknown')
time.sleep(.6)
m.same(unknown,m.grab(),'Missing progress is static and never synthesized')
snapshot({'UI_STATUS':5,'UI_PROGRESS':0})
time.sleep(.8)
canceled=m.shot('live-canceled')
time.sleep(.6)
m.same(canceled,m.grab(),'Cancellation settles without celebratory motion')
# Remove the selected record during an entrance; stale scene pointers must be discarded.
m.click('back')
m.click('select',settle=.035)
generation+=1
service.send_message(target,{keys['UI_COUNT']:Int32(0),keys['UI_INDEX']:Int32(0),
                            keys['UI_GENERATION']:Int32(generation)})
time.sleep(.5)
empty=m.shot('live-removed-during-motion')
time.sleep(.5)
m.same(empty,m.grab(),'Removal during motion returns safely to the empty list')
(m.OUT/'live-results.json').write_text(json.dumps({'platform':'emery','provider_requests':0,
    'mode':'real native message path / inert phone companion / synthetic snapshots',
    'checks':m.checks},indent=2)+'\n')
service.shutdown()
m.transport.ws.close()
m.monitor.close()
print('%d live-path emulator checks passed' % len(m.checks),flush=True)
