#!/usr/bin/env python3
"""Build a separate, offline native preview from tracked files on this branch.

Does not edit the checkout, call a flight API, install on a watch or push Git.
Uses the existing SDK and scripts/build.sh. PEBBLE_TOOLS_DIR selects the toolchain.
"""
import json
import os
from pathlib import Path
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'build' / 'overview-mockup'
PROJECT = OUTPUT / 'pebble-flight'
PROJECT.mkdir(parents=True, exist_ok=True)
tracked = subprocess.check_output(['git', 'ls-files', '-z'], cwd=ROOT).decode().split('\0')
for name in tracked:
    if not name or not (name.startswith(('src/', 'resources/')) or
                        name in ('package.json', 'package-lock.json', 'wscript')):
        continue
    target = PROJECT / name
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(ROOT / name, target)

package_path = PROJECT / 'package.json'
package = json.loads(package_path.read_text())
package['pebble']['displayName'] = 'Jetebble Mockup'
package['pebble']['uuid'] = '8a2dccb9-2ff6-4439-9aeb-0876a23c833d'
package_path.write_text(json.dumps(package, indent=2) + '\n')
config = PROJECT / 'src/c/prototype_config.h'
config.write_text(config.read_text().replace('#define PEBBLE_MOCK_DATA 0', '#define PEBBLE_MOCK_DATA 1'))
shutil.copyfile(ROOT / 'test/fixtures/overview_mock_flights.c', PROJECT / 'src/c/prototype/mock_flights.c')
readme = '''# Jetebble Overview Mockup

Separate native preview: Pebble C SDK, SDK metadata 3, Emery / Pebble Time 2.
Import this ZIP into a NEW CloudPebble project, then compile and run on Emery.
Mock mode is enabled. No credentials, phone settings, network or live services.
This UUID differs from the installed Jetebble app so they can coexist.

All data is synthetic. All Flights has 17 airline-logo examples plus one neutral
fallback with unknown progress. Every example uses the same BUR/OAK demonstration
route for controlled visual comparison; these are not claimed airline schedules.

Up/Down: list selection or Overview/Departure/Arrival/Booking. Select: open flight
or toggle the detail note. Back: return to the same list row. Hold Select: toggle
the empty-list demonstration. Statuses include upcoming, boarding, airborne,
landed, delayed and canceled. This preview does not replace the production build.
'''
(PROJECT / 'README.md').write_text(readme)
source_zip = ROOT / 'build/jetebble-overview-mockup-source.zip'
with zipfile.ZipFile(source_zip, 'w', zipfile.ZIP_DEFLATED) as archive:
    for path in sorted(PROJECT.rglob('*')):
        if path.is_file() and not any(part in ('build', 'node_modules', '.tools') for part in path.relative_to(PROJECT).parts):
            archive.write(path, path.relative_to(PROJECT))
tools_dir = Path(os.environ.get('PEBBLE_TOOLS_DIR', ROOT / '.tools'))
env = dict(os.environ)
env['PATH'] = str(tools_dir / 'node/bin') + ':' + str(tools_dir / 'bin') + ':' + env.get('PATH', '')
subprocess.run([str(tools_dir / 'bin/pebble'), 'build'], cwd=PROJECT, env=env, check=True)
pbw = ROOT / 'build/jetebble-overview-mockup.pbw'
shutil.copyfile(PROJECT / 'build/pebble-flight.pbw', pbw)
print('Offline native preview:', pbw)
print('CloudPebble source ZIP:', source_zip)
