# Plays the Circuit Playground Classic's beeps through the computer's speakers.
# Works with touch_beeper_classic.ino (PLOTTER_MODE = false).
#
# One-time setup (in a terminal):   pip install pyserial pygame
# Run it:                           python touch_beeper_sounds.py
# Close the Arduino Serial Monitor/Plotter first. Press Ctrl+C to stop.

import array
import math
import sys

import pygame
import serial
import serial.tools.list_ports

# ===== Change these =====
PORT = None        # None = find the board automatically, or e.g. "COM5" / "/dev/tty.usbmodem14101"
VOLUME = 0.7       # 0.0 to 1.0
LOUDER_WHEN_CLOSER = True
HOVER_VOLUME = 0.15  # how loud the faintest hover is (0.0 = silent, 1.0 = as loud as a touch)
WAVE = "triangle"  # "sine", "triangle", "square"
MIN_BEEP_MS = 120  # beeps are at least this long, so short ones are easy to hear
SHOW_BEEPS = True  # print each beep the board sends (handy for testing)
SHOW_EVERYTHING = False # print every message from the board, exactly as it arrives

# Use your own sound files for some pads (.wav or .ogg works best).
# Pad numbers: 0 = first pad (#3), 1 = second (#2), 2 = third (#0) ...
# Pads not listed here play a beep.
SOUND_FILES = {
    # 0: "drum.wav",
    # 1: "C:/Users/me/Music/cat.wav",
}
# ========================

RATE = 44100
pygame.mixer.pre_init(RATE, -16, 1, 256)   # small buffer = less delay
pygame.mixer.init()
pygame.mixer.set_num_channels(32)


def make_tone(freq, ms):
    """Build a short beep with a tiny fade in/out so it doesn't click."""
    n = int(RATE * ms / 1000)
    fade = min(int(RATE * 0.005), n // 2)
    samples = array.array("h")
    for t in range(n):
        phase = (freq * t / RATE) % 1.0
        if WAVE == "sine":
            v = math.sin(2 * math.pi * phase)
        elif WAVE == "square":
            v = 1.0 if phase < 0.5 else -1.0
        else:  # triangle
            v = 4 * abs(phase - 0.5) - 1
        env = min(1.0, t / fade, (n - t) / fade) if fade else 1.0
        samples.append(int(v * env * 12000))
    return pygame.mixer.Sound(buffer=samples.tobytes())


tone_cache = {}
def get_tone(freq, ms):
    key = (freq, ms)
    if key not in tone_cache:
        tone_cache[key] = make_tone(freq, ms)
    return tone_cache[key]


file_sounds = {}
for pad, path in SOUND_FILES.items():
    try:
        file_sounds[pad] = pygame.mixer.Sound(path)
        print(f"Pad {pad}: using {path}")
    except Exception as e:
        print(f"Pad {pad}: couldn't load {path} ({e}), using a beep instead")


def find_port():
    ports = list(serial.tools.list_ports.comports())
    for p in ports:                      # Adafruit boards
        if p.vid == 0x239A:
            return p.device
    for p in ports:                      # anything that looks like a USB board
        if "usbmodem" in p.device or "ACM" in p.device or "Circuit" in (p.description or ""):
            return p.device
    print("Couldn't find the board. Ports I can see:")
    for p in ports:
        print("  ", p.device, "-", p.description)
    sys.exit("Plug in the board, or set PORT at the top of this file.")


def play(pad, pitch, ms, level):
    # The board sends loudness 10 (just noticed) to 100 (full touch).
    # Turn that into quiet at a far hover, up to full volume at a full touch.
    closeness = max(0.0, min(1.0, (level - 10) / 90))
    loudness = HOVER_VOLUME + (1 - HOVER_VOLUME) * closeness
    vol = VOLUME * (loudness if LOUDER_WHEN_CLOSER else 1)
    if vol <= 0:
        return
    sound = file_sounds.get(pad) or get_tone(pitch, ms)
    channel = sound.play()
    if channel:
        channel.set_volume(vol)


port = PORT or find_port()
try:
    board = serial.Serial(port, 115200, timeout=1)
except serial.SerialException as e:
    sys.exit(f"Couldn't open {port}: {e}\nIs the Arduino Serial Monitor still open?")

print(f"Touch Beeper sounds. Connected to {port}.")
print("Playing a test beep... if you don't hear it, check the laptop's volume and speaker.")
play(0, 440, 300, 100)
print("Bring your hand close to a pad. Press button A on the board to check it's talking. (Ctrl+C to stop)")

try:
    while True:
        line = board.readline().decode(errors="ignore").strip()
        if not line:
            continue
        if SHOW_EVERYTHING:
            print("  from board:", repr(line))
        if line == "C":
            print("Measuring... don't touch!")
        elif line == "R":
            print("Ready.")
        elif line.startswith("N,"):
            try:
                pad, pitch, ms, level = (int(x) for x in line.split(",")[1:])
                if SHOW_BEEPS:
                    print(f"Beep: pad {pad}, {pitch} Hz, loudness {level}%")
                play(pad, pitch, max(ms, MIN_BEEP_MS), level)
            except ValueError:
                pass   # half a message, skip it
        else:
            print("Board says:", line)   # e.g. Plotter numbers = PLOTTER_MODE is still true
except KeyboardInterrupt:
    print("\nBye!")
finally:
    board.close()