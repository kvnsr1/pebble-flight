#!/usr/bin/env python3
"""Exercise native physical buttons and capture the actual Emery framebuffer.

Requires pebble on PATH, Pillow, SDK 4.33.1, and a built mock-mode PBW.
Run from the project root: python3 scripts/review-prototype.py
This installs only Flight Preview in the emulator; it never publishes anything.
"""
import json
from io import BytesIO
from pathlib import Path
import subprocess
import time
from PIL import Image, ImageChops, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs/screenshots"
TEMP = ROOT / "build/review"
if "#define PEBBLE_MOCK_DATA 1" not in (ROOT / "src/c/prototype_config.h").read_text():
    raise SystemExit("Enable PEBBLE_MOCK_DATA=1 and rebuild before running the mock review.")
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


def unchanged(path, navigation_strip=False):
    """Compare existing screens with their committed pre-change framebuffers."""
    result = subprocess.run(["git", "show", "HEAD:docs/screenshots/" + path.name],
                            cwd=ROOT, capture_output=True, check=True)
    before = Image.open(BytesIO(result.stdout)).convert("RGB")
    after = Image.open(path).convert("RGB")
    if navigation_strip:
        # Only the rightmost 10px may change for the fourth dot/next-page cue.
        before = before.crop((0, 0, 190, 228))
        after = after.crop((0, 0, 190, 228))
    assert not ImageChops.difference(before, after).getbbox(), path.name + " changed existing content"
    checks.append(path.stem + ": existing content unchanged" +
                  (" outside navigation indicator" if navigation_strip else ""))


pebble("install")
time.sleep(0.5)
scenarios = ["aa100", "ua206", "ba284", "dl418", "af83", "ek202"]
for index, scenario in enumerate(scenarios):
    before = shot(scenario + "-list")
    unchanged(before)
    if index == 0:
        button("up")
        same(before, shot("list-start", True), "List stops at first flight")
    button("select")
    overview = shot(scenario + "-overview")
    unchanged(overview, True)
    button("up")
    same(overview, shot("overview-start", True), scenario + ": Up stops at Overview")
    if index == 0:
        button("select")
        shot("data-note")
        unchanged(OUT / "data-note.png", True)
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
    unchanged(departure, True)
    button("down")
    arrival = shot(scenario + "-arrival")
    unchanged(arrival, True)
    button("down")
    booking = shot(scenario + "-booking")
    button("down")
    same(booking, shot("booking-end", True), scenario + ": Down stops at Booking")
    button("select")
    shot("booking-note", True)
    button("select")
    same(booking, shot("booking-note-closed", True), scenario + ": Booking note opens and closes")
    if index == 0:
        button("select", 850)
        same(booking, shot("booking-long-select", True), "Booking long Select is inert")
        button("select")
    button("back")
    same(before, shot("list-from-booking", True), scenario + ": Back from Booking preserves selection and scroll")
    button("select")
    for _ in range(3):
        button("down")
    same(booking, shot("booking-reopened", True), scenario + ": Booking data belongs to selected flight")
    button("up")
    same(arrival, shot("arrival-reverse", True), scenario + ": Up reverses to Arrival")
    button("up")
    same(departure, shot("departure-reverse", True), scenario + ": Up reverses to Departure")
    button("up")
    same(overview, shot("overview-reverse", True), scenario + ": Up reverses to Overview")
    button("back")
    same(before, shot("list-restored", True), scenario + ": Back preserves selection and scroll pixels")
    print("Verified " + scenario + " list + four detail pages", flush=True)
    if index < len(scenarios) - 1:
        button("down")

last_list = shot("list-scrolled", True)
button("down")
same(last_list, shot("list-end", True), "List stops at last flight")
button("select", 850)
empty = shot("empty-list")
unchanged(empty)
for key in ["up", "down", "select"]:
    button(key)
    same(empty, shot("empty-" + key, True), "Empty list safely ignores " + key)
button("select", 850)
same(last_list, shot("restored-after-empty", True), "Empty toggle preserves selection and scroll pixels")

# A contact sheet for visual QA; the individual PNGs are unmodified screenshots.
sheet = Image.new("RGB", (1060, 6 * 258 + 280), "#111111")
draw = ImageDraw.Draw(sheet)
for row, scenario in enumerate(scenarios):
    for col, page in enumerate(["list", "overview", "departure", "arrival", "booking"]):
        x, y = col * 212 + 6, row * 258
        draw.text((x, y + 3), scenario.upper() + " / " + page, fill="white")
        sheet.paste(Image.open(OUT / (scenario + "-" + page + ".png")).convert("RGB"), (x, y + 22))
for col, name in enumerate(["empty-list", "data-note"]):
    x, y = col * 212 + 6, 6 * 258
    draw.text((x, y + 3), name, fill="white")
    sheet.paste(Image.open(OUT / (name + ".png")).convert("RGB"), (x, y + 22))
sheet.save(OUT / "contact-sheet.png")
preview = Image.new("RGB", (1048, 254), "#111111")
draw = ImageDraw.Draw(preview)
for col, page in enumerate(["list", "overview", "departure", "arrival", "booking"]):
    x = col * 212
    draw.text((x, 3), page.upper(), fill="white")
    preview.paste(Image.open(OUT / ("ba284-" + page + ".png")).convert("RGB"), (x, 22))
preview.save(OUT / "preview.png")
booking_sheet = Image.new("RGB", (636, 516), "#111111")
draw = ImageDraw.Draw(booking_sheet)
for index, scenario in enumerate(scenarios):
    x, y = (index % 3) * 212 + 6, (index // 3) * 258
    draw.text((x, y + 3), scenario.upper() + " / booking", fill="white")
    booking_sheet.paste(Image.open(OUT / (scenario + "-booking.png")).convert("RGB"), (x, y + 22))
booking_sheet.save(OUT / "booking-scenarios.png")
(OUT / "navigation-results.json").write_text(json.dumps({
    "platform": "emery", "resolution": [200, 228],
    "scenarios": scenarios, "passed_pixel_comparisons": checks
}, indent=2) + "\n")
print(str(len(checks)) + " framebuffer comparisons passed. Screenshots: " + str(OUT), flush=True)
