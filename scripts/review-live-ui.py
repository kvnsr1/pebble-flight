#!/usr/bin/env python3
"""Verify the production message/render path with synthetic provider data.
Run AFTER the live-mode app is open in an Emery emulator with no saved flights.
Does not configure credentials or make a provider request. Sends only test frames
through AppMessage; does not write phone settings. The app must already be open.
"""
import json
from pathlib import Path
import subprocess
import time
from PIL import Image, ImageChops, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'docs/live-ui'
OUT.mkdir(parents=True,exist_ok=True)
keys=json.loads((ROOT/'package.json').read_text())['pebble']['messageKeys']
checks=[]
def pebble(*args):
    p=subprocess.run(['pebble',*args,'--emulator','emery'],cwd=ROOT,capture_output=True,text=True)
    if p.returncode: raise RuntimeError(p.stdout+p.stderr)
def button(name,duration=100):
    pebble('emu-button','click',name,'--duration',str(duration));time.sleep(.15)
def send(payload):
    ints=[str(keys[k])+'='+str(v) for k,v in payload.items() if isinstance(v,int)]
    strings=[str(keys[k])+'='+v for k,v in payload.items() if isinstance(v,str)]
    args=['send-app-message']
    if ints: args+=['--int',*ints]
    if strings: args+=['--string',*strings]
    pebble(*args)
def shot(name):
    p=OUT/(name+'.png');pebble('screenshot',str(p),'--no-open');return p
def same(a,b,note):
    assert not ImageChops.difference(Image.open(a).convert('RGB'),Image.open(b).convert('RGB')).getbbox(),note
    checks.append(note)
# Generate the exact production wire payload using the real JS adapter.
js="""var view=require('./src/pkjs/native-view');var raw=require('./test/fixtures/provider-flight.json');
var settings=[{flightNumber:'BA284',flightDate:'2026-10-16',bookingCode:'ABC123',seatNumber:'8C',seatPosition:'window'},
{flightNumber:'AA100',flightDate:'2026-10-16'},{flightNumber:'UA206',flightDate:'2026-10-16'}];
var old={AIRCRAFT_MODEL:'Boeing 787-9',AIRCRAFT_NUMBER:'G-DEMO',AIRCRAFT_FIRST_FLIGHT:'Unavailable',AIRCRAFT_LEG_1:'LHR > SFO',AIRCRAFT_LEG_1_STATUS:'LANDED'};
console.log(JSON.stringify(settings.map(function(s,i){var f=JSON.parse(JSON.stringify(raw));f.ident_iata=s.flightNumber;return view.toMessage(s,{flight:f,updatedAt:Date.now()},old,i,3,100,{});})));"""
frames=json.loads(subprocess.check_output(['node','-e',js],cwd=ROOT,text=True))
shot('empty-startup')
for frame in frames: send(frame)
initial=shot('all-flights')
button('select')
overview=shot('overview');button('up');same(overview,shot('overview-limit'),'Up stops at Overview')
button('down');departure=shot('departure')
button('down');arrival=shot('arrival')
button('down');booking=shot('booking')
button('down');same(booking,shot('booking-limit'),'Down stops at Booking')
button('select');shot('sync-note');button('select');same(booking,shot('booking-note-closed'),'Booking note closes')
button('up');same(arrival,shot('arrival-reverse'),'Up returns to Arrival')
button('back');same(initial,shot('list-restored'),'Back restores live selection')
# All three slots are selectable; no division-by-zero scrollbar for a 3-row list.
button('down');button('down');last=shot('third-flight');button('down');same(last,shot('last-limit'),'List stops at third saved flight')
button('select');button('down');button('down');button('down');shot('third-flight-booking')
button('back');same(last,shot('third-restored'),'Back retains third-flight selection')
button('up');button('up');button('select')
button('select',850);shot('actions');button('down');button('select');shot('aircraft');button('back')
# Missing progress/timezone and stale/error handling use real production flags.
for frame in frames:
    frame['UI_GENERATION']=101;frame['UI_STALE']=1
    if frame['UI_INDEX']==0:
        frame['UI_PROGRESS']=-1;frame['ERROR_MESSAGE']='Flight service unavailable; showing saved data'
    send(frame)
shot('saved-overview');button('select');shot('saved-note');button('back')
# A list update clears a removed selected flight without misattributing its data.
send({'UI_COUNT':0,'UI_INDEX':0,'UI_GENERATION':102,'ERROR_MESSAGE':'Open phone settings to add a flight'})
shot('empty-after-clear')
(OUT/'results.json').write_text(json.dumps({'mode':'live native app with synthetic AppMessage inputs',
    'platform':'emery','passed_comparisons':checks,'provider_requests':0},indent=2)+'\n')
images=['all-flights','overview','departure','arrival','booking','sync-note','actions','aircraft','saved-overview','saved-note','empty-after-clear']
sheet=Image.new('RGB',(5*212,3*256),'#111111');draw=ImageDraw.Draw(sheet)
for i,name in enumerate(images):
    x,y=(i%5)*212+6,(i//5)*256
    draw.text((x,y+2),name,fill='white');sheet.paste(Image.open(OUT/(name+'.png')).convert('RGB'),(x,y+22))
sheet.save(OUT/'contact-sheet.png')
print(str(len(checks))+' production navigation comparisons passed; screenshots '+str(OUT),flush=True)
