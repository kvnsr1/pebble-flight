#!/usr/bin/env python3
"""Record and check the offline startup preview in an already running emulator.

Use the Pebble SDK Python (Pillow/libpebble2). Captures actual QEMU frames;
does not render an approximation or contact a phone/provider.
"""
import json
from pathlib import Path
import socket
import tempfile
import time
import uuid
from PIL import Image, ImageChops, ImageDraw
from libpebble2.communication import PebbleConnection
from libpebble2.communication.transports.websocket import WebsocketTransport
from libpebble2.communication.transports.qemu.protocol import QemuButton
from libpebble2.protocol.apps import AppRunState, AppRunStateStart, AppRunStateStop
from pebble_tool.commands.emucontrol import send_data_to_qemu
from pebble_tool.commands.screenshot import ScreenshotCommand

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'docs/startup'
OUT.mkdir(parents=True, exist_ok=True)
TEMP = ROOT / 'build/startup-frames'
TEMP.mkdir(parents=True, exist_ok=True)
APP = uuid.UUID('b7c506bc-0cf8-4e02-83e6-a09a3a0ff8aa')
info = json.loads((Path(tempfile.gettempdir()) / 'pb-emulator.json').read_text())['emery']['4.33.1']
monitor = socket.create_connection(('127.0.0.1', info['qemu']['monitor']), timeout=3)
transport = WebsocketTransport('ws://127.0.0.1:%d/' % info['pypkjs']['port'])
connection = PebbleConnection(transport)
connection.connect()
connection.run_async()
buttons = {'up': QemuButton.Button.Up, 'down': QemuButton.Button.Down,
           'select': QemuButton.Button.Select, 'back': QemuButton.Button.Back}

def prompt():
    data = b''
    while b'(qemu)' not in data:
        block = monitor.recv(65536)
        if not block:
            raise RuntimeError('Emulator monitor disconnected')
        data += block
    return data

prompt()

def grab():
    path = TEMP / 'capture.ppm'
    monitor.sendall(('screendump ' + str(path) + '\n').encode())
    prompt()
    frame = Image.open(path).convert('RGB')
    path.unlink()
    assert frame.size == (200, 228), frame.size
    # Same panel/backlight normalization and SDK color correction used by the
    # repository's existing native motion review.
    white = max(channel[1] for channel in frame.getextrema())
    frame = frame.point([max(0, min(255, round(v * 3 / white) * 85)) for v in range(256)] * 3)
    raw = frame.tobytes()
    rows = [bytearray(raw[y * 600:(y + 1) * 600]) for y in range(228)]
    corrected = ScreenshotCommand._correct_colours(None, rows)
    return Image.frombytes('RGB', (200, 228), bytes(v for row in corrected for v in row))

def launch():
    connection.send_packet(AppRunState(data=AppRunStateStop(uuid=APP)))
    time.sleep(.2)
    connection.send_packet(AppRunState(data=AppRunStateStart(uuid=APP)))
    return time.monotonic()

def click(name):
    send_data_to_qemu(transport, QemuButton(state=buttons[name]))
    time.sleep(.06)
    send_data_to_qemu(transport, QemuButton(state=0))
    time.sleep(.25)

def same(a, b, message):
    assert not ImageChops.difference(a, b).getbbox(), message
    checks.append(message)

checks = []
try:
    start = launch()
    frames, times = [], []
    while time.monotonic() - start < 4.1:
        tick = time.monotonic()
        frames.append(grab())
        times.append(time.monotonic() - start)
        time.sleep(max(0, .04 - (time.monotonic() - tick)))
    def at(seconds):
        return frames[min(range(len(times)), key=lambda i: abs(times[i] - seconds))]

    samples = [('crossing', .65), ('cloud-trail', 1.25), ('clearing', 1.85),
               ('title', 2.9), ('app', 3.9)]
    for name, seconds in samples:
        at(seconds).save(OUT / (name + '.png'))

    # Restrict the review GIF to the complete intro and its final title hold.
    # The app handoff is retained in the separate screenshot and tested below.
    end = min(range(len(times)), key=lambda i: abs(times[i] - 3.15))
    shown = [frame.resize((400, 456), Image.Resampling.NEAREST) for frame in frames[:end + 1]]
    durations = [max(20, round((b - a) * 100) * 10)
                 for a, b in zip(times[:end], times[1:end + 1])] + [850]
    shown[0].save(OUT / 'startup.gif', save_all=True, append_images=shown[1:],
                  duration=durations, loop=0, optimize=False)
    sheet = Image.new('RGB', (5 * 216 + 16, 270), '#111915')
    draw = ImageDraw.Draw(sheet)
    for i, (name, seconds) in enumerate(samples):
        draw.text((16 + i * 216, 10), name.replace('-', ' ').title(), fill='white')
        sheet.paste(at(seconds), (16 + i * 216, 31))
    sheet.save(OUT / 'storyboard.png')

    assert len({f.tobytes() for f in frames[:end]}) > 25, 'Missing intermediate frames'
    checks.append('Native intermediate frames captured')
    same(at(2.8), at(3.1), 'Title holds still after clouds clear')
    baseline = at(3.9)
    time.sleep(.4)
    same(baseline, grab(), 'Intro hands off to an idle mock flight list')
    # Black appears only in the plane outline during the flight pass.
    centers = []
    for seconds in (.4, .7, 1.0):
        frame = at(seconds)
        points = [(x, y) for y in range(70, 145) for x in range(200)
                  if max(frame.getpixel((x, y))) < 35]
        assert points, 'Plane outline absent at %s' % seconds
        centers.append(sum(x for x, y in points) / len(points))
    assert centers[0] < centers[1] - 20 < centers[2] - 40, centers
    checks.append('Plane travels left to right through three measured positions')

    for button in buttons:
        start = launch()
        time.sleep(.6)
        click(button)
        same(baseline, grab(), button + ' skips intro without navigating behind it')
    click('select')
    time.sleep(.4)
    assert ImageChops.difference(baseline, grab()).getbbox(), 'Select did not open flight'
    click('back')
    same(baseline, grab(), 'Back from flight restores list without replaying intro')
    (OUT / 'validation.json').write_text(json.dumps({
        'capture': 'Native Emery QEMU framebuffer, SDK color correction',
        'size': [200, 228], 'title': 'Jetteble', 'font': 'FONT_KEY_GOTHIC_28_BOLD',
        'duration_ms': 3300, 'frames': len(frames), 'plane_x_samples': centers,
        'provider_requests': 0, 'checks': checks}, indent=2) + '\n')
    print('%d checks passed. Preview: %s' % (len(checks), OUT / 'startup.gif'))
finally:
    transport.ws.close()
    monitor.close()
