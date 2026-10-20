#!/usr/bin/env python3
"""Capture real QEMU frames and exercise the offline motion prototype.

Run with the SDK's Python (Pillow and pebble-tool installed) after installing
build/motion-prototype/pebble-flight in the Emery emulator. No physical watch.
"""
import json
from pathlib import Path
import socket
import tempfile
import time
from PIL import Image, ImageChops, ImageDraw, ImageSequence
from libpebble2.communication import PebbleConnection
from libpebble2.communication.transports.websocket import WebsocketTransport
from libpebble2.communication.transports.qemu.protocol import QemuButton
from pebble_tool.commands.emucontrol import send_data_to_qemu
from pebble_tool.commands.screenshot import ScreenshotCommand

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'docs/motion'
OUT.mkdir(parents=True, exist_ok=True)
TEMP = ROOT / 'build/motion-frames'
TEMP.mkdir(parents=True, exist_ok=True)
info = json.loads((Path(tempfile.gettempdir()) / 'pb-emulator.json').read_text())['emery']['4.33.1']
monitor = socket.create_connection(('127.0.0.1', info['qemu']['monitor']), timeout=2)
transport = WebsocketTransport('ws://127.0.0.1:%d/' % info['pypkjs']['port'])
connection = PebbleConnection(transport)
connection.connect()
connection.run_async()
BUTTONS = {'up':QemuButton.Button.Up,'down':QemuButton.Button.Down,
           'select':QemuButton.Button.Select,'back':QemuButton.Button.Back}

def prompt():
    data = b''
    while b'(qemu)' not in data:
        block = monitor.recv(65536)
        if not block:
            raise RuntimeError('Emulator monitor disconnected')
        data += block
    return data

prompt()

def command(value):
    monitor.sendall((value + '\n').encode())
    return prompt()

counter = 0

def grab():
    global counter
    counter += 1
    path = TEMP / ('frame-%05d.ppm' % counter)
    command('screendump ' + str(path))
    image = Image.open(path).convert('RGB')
    path.unlink()
    # QEMU dims the panel when its backlight sleeps. Normalize that uniform
    # brightness and apply the SDK's standard 64-colour screenshot correction.
    white = max(channel[1] for channel in image.getextrema())
    image = image.point([max(0,min(255,round(v * 3 / white) * 85)) for v in range(256)] * 3)
    raw = image.tobytes()
    rows = [bytearray(raw[y*600:(y+1)*600]) for y in range(228)]
    corrected = ScreenshotCommand._correct_colours(None, rows)
    return Image.frombytes('RGB', (200,228), bytes(v for row in corrected for v in row))

def click(button, duration=40, settle=.38):
    send_data_to_qemu(transport,QemuButton(state=BUTTONS[button]))
    time.sleep(duration/1000)
    send_data_to_qemu(transport,QemuButton(state=0))
    time.sleep(settle)

def shot(name):
    image = grab()
    image.save(OUT / (name + '.png'))
    return image

checks = []

def same(a,b,description):
    assert not ImageChops.difference(a,b).getbbox(), description
    checks.append(description)

def record_action(name, action, duration=.65):
    frames = [grab()]
    timestamps = [time.monotonic()]
    action()
    start = time.monotonic()
    while time.monotonic()-start < duration:
        tick = time.monotonic()
        frames.append(grab())
        timestamps.append(time.monotonic())
        time.sleep(max(0,.04-(time.monotonic()-tick)))
    durations = [max(20,round((b-a)*100)*10) for a,b in zip(timestamps,timestamps[1:])]+[400]
    frames[0].save(OUT/(name+'.gif'),save_all=True,append_images=frames[1:],
                   duration=durations,loop=0,optimize=False)
    unique = len({frame.tobytes() for frame in frames})
    assert unique >= 3, name + ': missing intermediate frames'
    checks.append(name + ': real intermediate frames captured (%d)' % unique)
    # Keep the actual intermediate frames available for visual inspection.
    different = [frames[0]]
    for frame in frames[1:]:
        if ImageChops.difference(frame,different[-1]).getbbox():
            different.append(frame)
    strip = Image.new('RGB',(5*200,228))
    for i in range(5):
        strip.paste(different[round(i*(len(different)-1)/4)],(i*200,0))
    strip.save(OUT/(name+'-frames.png'))
    return frames[-1]

def record(name, button, duration=.65):
    return record_action(name,lambda:click(button,settle=0),duration)

def main():
    time.sleep(.4)
    initial = shot('list')
    click('up')
    same(initial,grab(),'List stops at first flight')
    record('selection','down')
    click('down')
    selected = shot('selected-list')
    overview = record('open-flight','select')
    overview.save(OUT/'overview.png')
    time.sleep(.8)
    same(overview,grab(),'Overview is still at rest; progress does not creep')
    click('up')
    same(overview,grab(),'Up stops at Overview')
    departure = record('next-page','down')
    departure.save(OUT/'departure.png')
    arrival = record('next-arrival','down')
    arrival.save(OUT/'arrival.png')
    booking = record('next-booking','down')
    booking.save(OUT/'booking.png')
    click('down')
    same(booking,grab(),'Down stops at Booking')
    note = record('data-note','select')
    note.save(OUT/'note.png')
    record('close-note','select')
    same(booking,grab(),'Select closes note to exact previous page')
    record('back-to-list','back')
    same(selected,grab(),'Back restores exact selected list and scroll')
    click('select')
    record('open-overview-back','back')
    same(selected,grab(),'Overview reverse hero restores selected list')
    click('select')
    # A second press while the first page is moving must win immediately.
    click('down',duration=35,settle=.065)
    click('down',duration=35,settle=.45)
    same(arrival,grab(),'Rapid Down presses reach Arrival without queued motion')
    click('select')
    click('back')
    same(selected,grab(),'Back from note returns directly to list')
    click('select',850)
    empty=shot('empty')
    time.sleep(.8)
    same(empty,grab(),'Empty screen settles and is still')
    click('select',850)
    same(selected,grab(),'Empty demonstration preserves selected flight')

    # Reset the bounded list selection, then cover every operational state.
    for _ in range(6):
        click('up',settle=.13)
    for index,name in enumerate(['aa100','ua206','ba284','dl418','af83','ek202']):
        before=grab()
        click('select')
        shot(name+'-overview')
        for page in ['departure','arrival','booking']:
            click('down')
            shot(name+'-'+page)
        click('back')
        same(before,grab(),name+': exact selection and scroll after four detail pages')
        if index<5:
            click('down')

    for _ in range(3):
        click('up')
    click('select')
    before_demo=grab()
    record_action('demo-refresh',lambda:click('select',850,settle=0),duration=1.6)
    after_demo=shot('demo-refreshed')
    assert ImageChops.difference(before_demo.crop((56,40,140,68)),after_demo.crop((56,40,140,68))).getbbox(), 'Synthetic progress update not shown'
    time.sleep(.8)
    same(after_demo,grab(),'Offline refresh settles at new synthetic progress and becomes still')

    sheet=Image.new('RGB',(5*212,254),'#111111')
    draw=ImageDraw.Draw(sheet)
    for i,name in enumerate(['selected-list','overview','departure','arrival','booking']):
        draw.text((i*212+6,3),name,fill='white')
        sheet.paste(Image.open(OUT/(name+'.png')),(i*212+6,22))
    sheet.save(OUT/'preview.png')
    preview_frames=[]
    preview_durations=[]
    for name,title in [('open-flight','Open flight'),('next-page','Next detail page'),
                       ('data-note','Data note'),('demo-refresh','Offline refresh demonstration')]:
        animation=Image.open(OUT/(name+'.gif'))
        for frame in ImageSequence.Iterator(animation):
            canvas=Image.new('RGB',(200,250),'#111111')
            ImageDraw.Draw(canvas).text((6,3),title,fill='white')
            canvas.paste(frame.convert('RGB'),(0,22))
            preview_frames.append(canvas.resize((400,500),Image.Resampling.NEAREST))
            preview_durations.append(frame.info.get('duration',40))
    preview_frames[0].save(OUT/'motion-preview.gif',save_all=True,append_images=preview_frames[1:],
                           duration=preview_durations,loop=0,optimize=False)
    (OUT/'results.json').write_text(json.dumps({'platform':'emery','base':'fba3b9b',
        'provider_requests':0,'checks':checks,'capture':'native QEMU frames, SDK colour correction'},indent=2)+'\n')
    print('%d emulator checks passed; evidence: %s' % (len(checks),OUT),flush=True)


if __name__ == "__main__":
    try:
        main()
    finally:
        transport.ws.close()
        monitor.close()
