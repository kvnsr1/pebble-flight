#!/usr/bin/env python3
"""Compare captured resting pixels and enlarge actual QEMU GIFs for review.

Run after review-stretch-motion.py and review-stretch-fallbacks.py reentry,
with the SDK Python (Pillow). No invented or interpolated motion frames.
"""
from pathlib import Path
from PIL import Image,ImageSequence,ImageChops
import subprocess,io,json
ROOT=Path(__file__).resolve().parents[1];out=ROOT/'docs/stretch-motion'
matches=[]
for path in sorted((out/'12h').glob('*.png')):
    if path.name.endswith('-frames.png') or path.name in ['preview.png','exited-mid-motion.png']:continue
    result=subprocess.run(['git','show','36c1be6:docs/settings-cards/12h/'+path.name],cwd=ROOT,capture_output=True)
    if result.returncode:continue
    original=Image.open(io.BytesIO(result.stdout)).convert('RGB')
    assert not ImageChops.difference(original,Image.open(path).convert('RGB')).getbbox(),path.name
    matches.append(path.name)
(out/'resting-layouts.json').write_text(json.dumps({'baseline':'36c1be6','exact_matches':matches},indent=2)+'\n')
for name,sources in [('open-flight.gif',['reentry/open.gif','reentry/back.gif']),('focus-change.gif',['12h/selection-down.gif','12h/selection-up.gif'])]:
    frames=[];durations=[]
    for source in sources:
        for frame in ImageSequence.Iterator(Image.open(out/source)):
            frames.append(frame.convert('RGB').resize((400,456),Image.Resampling.NEAREST))
            durations.append(frame.info.get('duration',40))
    durations[0]=600;durations[-1]=900
    frames[0].save(out/name,save_all=True,append_images=frames[1:],duration=durations,loop=0,optimize=False)
print(len(matches),'approved resting layouts match; actual native GIFs packaged')
