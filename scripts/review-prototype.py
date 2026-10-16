#!/usr/bin/env python3
"""Exercise native physical buttons and capture the actual Emery framebuffer.

Requires pebble on PATH, Pillow, SDK 4.33.1, and a built mock-mode PBW.
Run from the project root: python3 scripts/review-prototype.py
This installs only Flight Preview in the emulator; it never publishes anything.
"""
import json
from pathlib import Path
import subprocess
import time
from PIL import Image, ImageChops, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs/screenshots"
TEMP = ROOT / "build/review"
OUT.mkdir(parents=True, exist_ok=True)
TEMP.mkdir(parents=True, exist_ok=True)
checks = []


def pebble(*args):
    result = subprocess.run(["pebble", *args, "--emulator", "emery"],
                            cwd=ROOT, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)


def button(name, duration=100):
    pebble("emu-button", "click", name, "--duration", str(duration))
    time.sleep(0.12)


def shot(name, temporary=False):
    path = (TEMP if temporary else OUT) / (name + ".png")
    pebble("screenshot", str(path), "--no-open")
    image = Image.open(path).convert("RGB")
    assert image.size == (200, 228), image.size
    return path


def same(first, second, description):
    a = Image.open(first).convert("RGB")
    b = Image.open(second).convert("RGB")
    assert not ImageChops.difference(a, b).getbbox(), description
    checks.append(description)


pebble("install")
time.sleep(0.5)
scenarios = ["aa100", "ua206", "ba284", "dl418", "af83", "ek202"]
for index, scenario in enumerate(scenarios):
    before = shot(scenario + "-list")
    if index == 0:
        button("up")
        same(before, shot("list-start", True), "List stops at first flight")
    button("select")
    overview = shot(scenario + "-overview")
    button("up")
    same(overview, shot("overview-start", True), scenario + ": Up stops at Overview")
    if index == 0:
        button("select")
        shot("data-note")
        button("select")
        same(overview, shot("note-closed", True), "Select opens and closes data note")
        button("select", 850)
        same(overview, shot("detail-long-select", True), "Detail long Select is inert")
    if index == 1:
        button("select")
        button("back")
        same(before, shot("back-from-note", True), "Back from note returns directly to preserved list")
        button("select")
    button("down")
    departure = shot(scenario + "-departure")
    button("down")
    arrival = shot(scenario + "-arrival")
    button("down")
    same(arrival, shot("arrival-end", True), scenario + ": Down stops at Arrival")
    button("up")
    same(departure, shot("departure-reverse", True), scenario + ": Up reverses to Departure")
    button("up")
    same(overview, shot("overview-reverse", True), scenario + ": Up reverses to Overview")
    button("back")
    same(before, shot("list-restored", True), scenario + ": Back preserves selection and scroll pixels")
    print("Verified " + scenario + " list + three detail pages", flush=True)
    if index < len(scenarios) - 1:
        button("down")

last_list = shot("list-scrolled", True)
button("down")
same(last_list, shot("list-end", True), "List stops at last flight")
button("select", 850)
empty = shot("empty-list")
for key in ["up", "down", "select"]:
    button(key)
    same(empty, shot("empty-" + key, True), "Empty list safely ignores " + key)
button("select", 850)
same(last_list, shot("restored-after-empty", True), "Empty toggle preserves selection and scroll pixels")

# A contact sheet for visual QA; the individual PNGs are unmodified screenshots.
sheet = Image.new("RGB", (848, 6 * 258 + 280), "#111111")
draw = ImageDraw.Draw(sheet)
for row, scenario in enumerate(scenarios):
    for col, page in enumerate(["list", "overview", "departure", "arrival"]):
        x, y = col * 212 + 6, row * 258
        draw.text((x, y + 3), scenario.upper() + " / " + page, fill="white")
        sheet.paste(Image.open(OUT / (scenario + "-" + page + ".png")).convert("RGB"), (x, y + 22))
for col, name in enumerate(["empty-list", "data-note"]):
    x, y = col * 212 + 6, 6 * 258
    draw.text((x, y + 3), name, fill="white")
    sheet.paste(Image.open(OUT / (name + ".png")).convert("RGB"), (x, y + 22))
sheet.save(OUT / "contact-sheet.png")
(OUT / "navigation-results.json").write_text(json.dumps({
    "platform": "emery", "resolution": [200, 228],
    "scenarios": scenarios, "passed_pixel_comparisons": checks
}, indent=2) + "\n")
print(str(len(checks)) + " framebuffer comparisons passed. Screenshots: " + str(OUT), flush=True)
