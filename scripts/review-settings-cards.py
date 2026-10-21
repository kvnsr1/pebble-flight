#!/usr/bin/env python3
"""Exercise the isolated five-flight preview with actual Emery button events.

Run with the Pebble SDK Python after installing jetebble-review-24h/12h.pbw.
Uses the existing QEMU capture helper, never a flight service or physical watch.
"""
import argparse, importlib.util, json
from pathlib import Path
from PIL import Image, ImageDraw

ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--format',choices=['24H','12H'],default='24H');args=parser.parse_args()
spec=importlib.util.spec_from_file_location('capture',ROOT/'scripts/review-motion.py')
c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)
c.OUT=ROOT/'docs/settings-cards'/args.format.lower();c.OUT.mkdir(parents=True,exist_ok=True)

def main():
    # The startup intro has already settled before this script is invoked.
    initial=c.shot('list')
    c.click('up');c.same(initial,c.grab(),'List stops at first flight')
    c.record('open-first','select');c.shot('first-overview')
    c.click('up');c.same(Image.open(c.OUT/'first-overview.png'),c.grab(),'Up stops at Overview')
    for page in ['departure','arrival','booking']:
        c.click('down');c.shot('first-'+page)
    last=c.grab();c.click('down');c.same(last,c.grab(),'Down stops at Booking')
    c.click('select');c.shot('note');c.click('select');c.same(last,c.grab(),'Select toggles the data note')
    c.click('back');c.same(initial,c.grab(),'Back restores first card exactly')
    c.record('selection','down');second=c.shot('second-list')
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
    # Leave the preview at the first card for interactive review.
    for _ in range(4):c.click('up')
    sheet=Image.new('RGB',(1060,254),'#111111');draw=ImageDraw.Draw(sheet)
    for i,name in enumerate(['list','second-overview','second-departure','first-arrival','fifth-list']):
        draw.text((i*212+6,3),name,fill='white');sheet.paste(Image.open(c.OUT/(name+'.png')),(i*212+6,22))
    sheet.save(c.OUT/'preview.png')
    (c.OUT/'validation.json').write_text(json.dumps({'format':args.format,'platform':'emery','provider_requests':0,'checks':c.checks,'capture':'Actual QEMU frames with SDK colour correction; synthetic data'},indent=2)+'\n')
    print('%d checks passed; %s'%(len(c.checks),c.OUT))

if __name__=='__main__':
    try:main()
    finally:c.transport.ws.close();c.monitor.close()
