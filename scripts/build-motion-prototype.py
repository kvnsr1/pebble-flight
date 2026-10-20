#!/usr/bin/env python3
"""Package an isolated native prototype; never install, publish, or modify main.

Default: offline mock data. --live-test: real message path with an inert phone
companion, for synthetic AppMessage verification without provider requests.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--live-test', action='store_true')
args = parser.parse_args()
name = 'motion-live-test' if args.live_test else 'motion-prototype'
project = ROOT / 'build' / name / 'pebble-flight'
project.mkdir(parents=True, exist_ok=True)
for directory in ('src', 'resources'):
    target = project / directory
    if target.exists():
        shutil.rmtree(target)
    shutil.copytree(ROOT / directory, target)
for name_ in ('package.json', 'package-lock.json', 'wscript'):
    shutil.copyfile(ROOT / name_, project / name_)
package_path = project / 'package.json'
package = json.loads(package_path.read_text())
package['pebble']['displayName'] = 'Jetebble Motion'
package['pebble']['uuid'] = 'b8cf4ac3-4e1a-4e92-9c28-99631fab925c'
package['pebble']['capabilities'] = []
package_path.write_text(json.dumps(package, indent=2) + '\n')
if not args.live_test:
    config = project / 'src/c/prototype_config.h'
    config.write_text(config.read_text().replace('#define PEBBLE_MOCK_DATA 0', '#define PEBBLE_MOCK_DATA 1')
                      .replace('#define PEBBLE_MOTION_DEMO 0', '#define PEBBLE_MOTION_DEMO 1'))
# Both review variants have no phone service, settings, credentials, or network.
(project / 'src/pkjs/index.js').write_text('// Jetebble Motion: inert, offline review companion.\n')
(project / 'README.md').write_text('''# Jetebble Motion Prototype

Native Pebble C SDK / SDK metadata 3 / Emery / Pebble Time 2 (200x228).
Import this source ZIP into a NEW CloudPebble project, then compile for Emery.
The prototype has its own UUID and coexists with production Jetebble.

All displayed flights are synthetic, frozen examples. No credentials, phone
settings, provider requests, or live services are used by the offline prototype.

Up/Down: select a flight or move Overview -> Departure -> Arrival -> Booking.
Select: open a flight or toggle its data note. Back: return to the same list row.
Hold Select on All Flights: toggle the empty demonstration. Hold Select on a
detail: simulate a refresh (WAIT, then a synthetic progress/gate update).
Buttons interrupt
motion immediately; idle screens are still. Progress is never clock-derived.
''')
stem = ROOT / 'build' / ('jetebble-' + name)
with zipfile.ZipFile(str(stem) + '-source.zip', 'w', zipfile.ZIP_DEFLATED) as archive:
    for path in sorted(project.rglob('*')):
        relative = path.relative_to(project)
        if path.is_file() and not any(p in ('build', 'node_modules', '.tools') for p in relative.parts):
            archive.write(path, relative)
tools = Path(os.environ.get('PEBBLE_TOOLS_DIR', Path.home() / 'Developer/pebble-flight/.tools'))
env = dict(os.environ)
env['PATH'] = str(tools / 'node/bin') + ':' + str(tools / 'bin') + ':' + env.get('PATH', '')
subprocess.run([str(tools / 'bin/pebble'), 'build'], cwd=project, env=env, check=True)
shutil.copyfile(project / 'build/pebble-flight.pbw', str(stem) + '.pbw')
print('Native bundle:', str(stem) + '.pbw')
print('Source ZIP:', str(stem) + '-source.zip')
