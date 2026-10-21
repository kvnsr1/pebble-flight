#!/usr/bin/env python3
"""Small native review of the separately built simple/missing-logo variants."""
import argparse,importlib.util,json
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('variant',choices=['simple','missing','reentry']);args=p.parse_args()
s=importlib.util.spec_from_file_location('capture',ROOT/'scripts/review-motion.py');c=importlib.util.module_from_spec(s);s.loader.exec_module(c)
c.OUT=ROOT/'docs/stretch-motion'/args.variant;c.OUT.mkdir(parents=True,exist_ok=True)
try:
    c.time.sleep(4) # Install may return before the intro owns the buttons.
    c.click('up',settle=.6) # Skip an intro, or stop at the first flight.
    initial=c.shot('list')
    c.same(Image.open(ROOT/'docs/stretch-motion/12h/list.png'),initial,'Same first-card resting layout')
    if args.variant=='missing':
        for _ in range(4):c.click('down',settle=.6)
        initial=c.shot('missing-logo-list')
    c.record('open','select');c.shot('overview')
    if args.variant!='missing':
        c.same(Image.open(ROOT/'docs/stretch-motion/12h/first-overview.png'),c.grab(),'Same Overview resting layout')
    c.record('back','back');c.same(initial,c.grab(),'Fallback/review returns to the exact selected card')
    c.click('select',settle=.06);c.click('back',settle=.06);c.click('back',settle=.5)
    c.shot('exited');c.click('select',settle=.2);c.click('up',settle=.6)
    c.same(Image.open(ROOT/'docs/stretch-motion/12h/list.png'),c.shot('reentered'),'Exit during motion and re-entry restores valid first card')
    (c.OUT/'validation.json').write_text(json.dumps({'variant':args.variant,'platform':'emery','checks':c.checks,'provider_requests':0},indent=2)+'\n')
    print(args.variant,':',len(c.checks),'checks passed')
finally:c.transport.ws.close();c.monitor.close()
