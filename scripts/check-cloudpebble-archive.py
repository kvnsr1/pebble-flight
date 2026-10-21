#!/usr/bin/env python3
"""Check the same source archive used by CloudPebble's GitHub importer.

Uses git archive, not the working directory: untracked local files and installed
dependencies must not enter a repository import. --worktree-attributes allows
reviewing a changed .gitattributes before committing it.
"""
import argparse
from io import BytesIO
import json
from pathlib import Path
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--output', type=Path, help='Also save the verified source ZIP')
args = parser.parse_args()
archive = subprocess.check_output(
    ['git', 'archive', '--worktree-attributes', '--format=zip', 'HEAD'], cwd=ROOT)
tracked = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', 'HEAD'],
                                  cwd=ROOT, text=True).splitlines()
with zipfile.ZipFile(BytesIO(archive)) as z:
    files = {entry.filename for entry in z.infolist() if not entry.is_dir()}
    # CloudPebble skips the root manifests in its 400-file project count.
    counted = files - {'package.json', 'appinfo.json'}
    assert len(z.infolist()) < 2000, 'CloudPebble raw archive entry limit exceeded'
    assert len(counted) <= 400, 'CloudPebble project count exceeded: %d' % len(counted)
    required = {p for p in tracked if p.startswith(('src/', 'resources/', 'docs/licenses/'))}
    required |= {'package.json', 'package-lock.json', 'wscript', 'config/index.html',
                 'README.md', 'docs/PEBBLE_MOTION_GUIDE.md'}
    assert required <= files, 'Required files omitted: %s' % sorted(required - files)
    for path in required:
        expected = subprocess.check_output(['git', 'show', 'HEAD:' + path], cwd=ROOT)
        assert z.read(path) == expected, 'Archive changed file content: ' + path
    package = json.loads(z.read('package.json'))
    for resource in package['pebble']['resources']['media']:
        assert 'resources/' + resource['file'] in files, resource['file']
    review = {p for p in tracked if p.startswith('docs/') and p.endswith(('.png', '.gif'))}
    assert review and not review & files, 'Review captures still exported'
    print('CloudPebble count: %d / 400 (%d archive files)' % (len(counted), len(files)))
    print('Verified source, resources, manifest media, guides and licenses byte-for-byte')
    print('%d review images remain tracked in Git, omitted only from archives' % len(review))
if args.output:
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(archive)
    print('Verified source ZIP:', args.output.resolve())
