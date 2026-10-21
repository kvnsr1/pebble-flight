#!/usr/bin/env python3
"""Build a local offline startup review app, or compile-check the live variant.

Never uploads or publishes. Only tracked app files plus the new startup module
are copied, so unrelated Finder duplicate files cannot enter the native build.
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
parser.add_argument('--production-check', action='store_true')
args = parser.parse_args()
name = 'startup-production-check' if args.production_check else 'startup-preview'
project = ROOT / 'build' / name
project.mkdir(parents=True, exist_ok=True)
# Generated message keys can otherwise survive a switch to newer app metadata.
if (project / 'build').exists():
    shutil.rmtree(project / 'build')
for lock in project.glob('.lock-waf_*'):
    lock.unlink()
for folder in ('src', 'resources'):
    if (project / folder).exists():
        shutil.rmtree(project / folder)
files = subprocess.check_output(
    ['git', 'ls-files', '-z', 'src', 'resources', 'package.json', 'package-lock.json', 'wscript'],
    cwd=ROOT).decode().split('\0')
files += ['src/c/prototype/startup.c', 'src/c/prototype/startup.h']
for relative in sorted(set(filter(None, files))):
    target = project / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(ROOT / relative, target)

if not args.production_check:
    path = project / 'package.json'
    package = json.loads(path.read_text())
    package['pebble']['displayName'] = 'Jetteble Intro'
    package['pebble']['uuid'] = 'b7c506bc-0cf8-4e02-83e6-a09a3a0ff8aa'
    package['pebble']['capabilities'] = []
    path.write_text(json.dumps(package, indent=2) + '\n')
    config = project / 'src/c/prototype_config.h'
    config.write_text(config.read_text()
        .replace('#define PEBBLE_MOCK_DATA 0', '#define PEBBLE_MOCK_DATA 1')
        .replace('#define PEBBLE_STARTUP_REVIEW 0', '#define PEBBLE_STARTUP_REVIEW 1'))
    (project / 'src/pkjs/index.js').write_text('// Offline startup review. No network or settings.\n')
    (project / 'README.md').write_text('''# Jetteble startup review

Local native C / Emery (200x228) preview with its own app identity.
The plane crosses left to right, leaves clouds, and the clouds part to reveal
Jetteble in Pebble Gothic 28 Bold. A rolling cloud lift dissolves into the mock
flight list. The complete sequence lasts 4 seconds.
Any button skips the intro. Hold Select on All Flights to replay.
The companion is inert; no account, credentials, or live requests are used.
''')

tools = Path(os.environ.get('PEBBLE_TOOLS_DIR', Path.home() / 'Developer/pebble-flight/.tools'))
env = dict(os.environ)
env['PATH'] = str(tools / 'node/bin') + ':' + str(tools / 'bin') + ':' + env.get('PATH', '')
subprocess.run([str(tools / 'bin/pebble'), 'build'], cwd=project, env=env, check=True)
if args.production_check:
    version = json.loads((project / 'package.json').read_text())['version']
    compiled = project / 'build' / (name + '.pbw')
    bundle = ROOT / 'build' / ('jetebble-' + version + '.pbw')
    shutil.copyfile(compiled, bundle)
    shutil.copyfile(compiled, ROOT / 'build/pebble-flight.pbw')
    print('Production bundle:', bundle)
else:
    bundle = ROOT / 'build/jetteble-startup-preview.pbw'
    shutil.copyfile(project / 'build' / (name + '.pbw'), bundle)
    source = ROOT / 'build/jetteble-startup-preview-source.zip'
    with zipfile.ZipFile(source, 'w', zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(project.rglob('*')):
            relative = path.relative_to(project)
            if path.is_file() and not any(p in ('build', 'node_modules') for p in relative.parts) and not relative.name.startswith('.lock'):
                archive.write(path, relative)
    print('Offline preview:', bundle)
    print('Review source:', source)
