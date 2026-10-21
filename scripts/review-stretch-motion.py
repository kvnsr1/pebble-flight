#!/usr/bin/env python3
"""Verify the local stretch-motion preview with actual Emery button events.

Run with the Pebble SDK Python after installing jetebble-stretch-12h.pbw.
Uses the existing QEMU capture helper, never a flight service or physical watch.
"""
import argparse, importlib.util, json, re
from pathlib import Path
from PIL import Image, ImageDraw

ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--format',choices=['24H','12H'],default='24H');args=parser.parse_args()
spec=importlib.util.spec_from_file_location('capture',ROOT/'scripts/review-motion.py')
c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)
from libpebble2.protocol.logs import AppLogMessage,AppLogShippingControl
heap_samples=[]
def log(packet):
    match=re.search(r'Motion settled.* / heap (\d+)',packet.message)
    if match:heap_samples.append(int(match.group(1)))
c.connection.register_endpoint(AppLogMessage,log)
c.connection.send_packet(AppLogShippingControl(enable=True))
original_click=c.click
def settled_click(button,duration=40,settle=.55):return original_click(button,duration,settle)
c.click=settled_click
c.OUT=ROOT/'docs/stretch-motion'/args.format.lower();c.OUT.mkdir(parents=True,exist_ok=True)

def main():
    c.time.sleep(4) # Wait for the 3.3s startup after SDK installation.
    c.click('up',settle=.6) # Skip the startup overlay if it is still showing.
    initial=c.shot('list')
    c.click('up',settle=.65);c.same(initial,c.grab(),'List stops at first flight')
    c.record('open-first','select');c.shot('first-overview')
    c.click('up');c.same(Image.open(c.OUT/'first-overview.png'),c.grab(),'Up stops at Overview')
    for page in ['departure','arrival','booking']:
        c.click('down');c.shot('first-'+page)
    last=c.grab();c.click('down');c.same(last,c.grab(),'Down stops at Booking')
    c.click('select');c.shot('note');c.click('select');c.same(last,c.grab(),'Select toggles the data note')
    c.click('back');c.same(initial,c.grab(),'Back restores first card exactly')
    c.record('selection-down','down');second=c.shot('second-list')
    c.record('selection-up','up');c.same(initial,c.grab(),'Reverse focus motion returns to first card')
    c.click('down')
    c.record('open-second','select');overview=c.shot('second-overview')
    c.time.sleep(.8);c.same(overview,c.grab(),'Overview is still at rest')
    c.record('back-second','back');c.same(second,c.grab(),'Reverse shared transition restores second card')
    c.click('select');c.click('down',duration=35,settle=.065);c.click('down',duration=35,settle=.45)
    arrival=c.shot('rapid-arrival');c.click('up');c.shot('second-departure');c.click('down')
    c.same(arrival,c.grab(),'Rapid input interrupts motion and reaches Arrival')
    c.click('back');c.same(second,c.grab(),'Back from Arrival preserves selection and scroll')
    for _ in range(3):c.click('down')
    fifth=c.shot('fifth-list');c.click('down');c.same(fifth,c.grab(),'List stops at fifth flight')
    c.click('select');c.shot('fifth-overview')
    for page in ['departure','arrival','booking']:
        c.click('down');c.shot('fifth-'+page)
    c.click('back');c.same(fifth,c.grab(),'Fifth flight restores selection and scrolled position')
    c.click('select',850);empty=c.shot('empty');c.time.sleep(.6);c.same(empty,c.grab(),'Empty screen settles')
    c.click('select',850);c.same(fifth,c.grab(),'Empty demonstration restores fifth flight')
    # Repeated animations must not drift, leave ghosts or run out of resources.
    sample_start=len(heap_samples)
    for _ in range(25):
        c.click('select',settle=.5);c.click('back',settle=.5)
    c.same(fifth,c.grab(),'25 open/back cycles retain exact pixels and navigation')
    cycles=heap_samples[sample_start:]
    assert len(cycles)>=40, 'Missing native heap telemetry'
    assert min(cycles)==max(cycles), 'Heap drifts across repeated open/back cycles'
    c.checks.append('Heap remains constant across 25 open/back cycles: %d bytes free'%min(cycles))
    # Leave the preview at the first card for interactive review.
    for _ in range(4):c.click('up')
    # Interrupt the shared header with Back and immediately retarget selection.
    c.click('select',settle=.06);c.click('back',settle=.06);c.click('down',settle=.5)
    c.same(second,c.grab(),'Back and Down during shared motion immediately select second flight')
    c.click('up')
    c.click('down',duration=1200,settle=.5);c.same(fifth,c.grab(),'Held Down reaches fifth flight without a navigation queue')
    c.click('up',duration=1200,settle=.5);c.same(initial,c.grab(),'Held Up reaches first flight without wraparound')
    c.click('select',settle=.06);c.click('back',settle=.06);c.click('back',settle=.5)
    c.shot('exited-mid-motion')
    c.checks.append('App exits during shared motion without a crash')
    sheet=Image.new('RGB',(1060,254),'#111111');draw=ImageDraw.Draw(sheet)
    for i,name in enumerate(['list','second-overview','second-departure','first-arrival','fifth-list']):
        draw.text((i*212+6,3),name,fill='white');sheet.paste(Image.open(c.OUT/(name+'.png')),(i*212+6,22))
    sheet.save(c.OUT/'preview.png')
    (c.OUT/'validation.json').write_text(json.dumps({'format':args.format,'platform':'emery','provider_requests':0,'heap_samples':heap_samples,'checks':c.checks,'capture':'Actual QEMU frames with SDK colour correction; synthetic data'},indent=2)+'\n')
    print('%d checks passed; %s'%(len(c.checks),c.OUT))

if __name__=='__main__':
    try:main()
    finally:c.transport.ws.close();c.monitor.close()
