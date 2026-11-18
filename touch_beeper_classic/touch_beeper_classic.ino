#include <Adafruit_CircuitPlayground.h>

// Circuit Playground CLASSIC version.
// The board doesn't make the sound. It tells the computer when to beep,
// and the Python script (touch_beeper_sounds.py) plays it through the computer's speakers.

// ===== Change these to test =====
const int   NUM_PADS     = 3;     // 1 to 8: uses pads #3, #2, #0, #1, #12, #6, #9, #10 in order
const int   NOISE        = 25;    // ignore changes smaller than this: set just above the wobble at rest
const int   FULL_TOUCH   = 165;   // change at a full touch: check the Serial Plotter
const int   SLOWEST      = 900;   // ms between beeps at a far hover (slow)
const int   FASTEST      = 60;    // ms between beeps at full touch (fast)
const int   BEEP_MS      = 40;    // how long each beep lasts on the computer
const int   SAMPLES      = 30;    // more = feels a hover from further away, but reacts a bit slower
const float SMOOTHING    = 0.3;   // 0.1 = very smooth/slow, 1.0 = no smoothing
const bool  PLOTTER_MODE = false; // true = print numbers for the Serial Plotter
                                  // false = send beeps to the Python script
// ================================

// Touch pads on the Classic (going around the board)
const uint8_t  allPads[8]  = {3, 2, 0, 1, 12, 6, 9, 10};
const char*    padNames[8] = {"P3", "P2", "P0", "P1", "P12", "P6", "P9", "P10"};
const uint16_t notes[8]    = {262, 294, 330, 349, 392, 440, 494, 523};  // C D E F G A B C
const uint8_t  padPixel[8] = {0, 1, 3, 4, 5, 6, 8, 9};  // light roughly next to each pad

uint16_t baseline[8];
float    smoothed[8];
unsigned long lastBeep[8];

// Read one pad (averaged over SAMPLES readings)
uint16_t readPad(int i) {
  return CircuitPlayground.readCap(allPads[i], SAMPLES);
}

// Measure each pad's resting value (don't touch anything!)
void calibrate() {
  if (!PLOTTER_MODE) Serial.println("C");          // tell the web page we're measuring
  CircuitPlayground.clearPixels();
  CircuitPlayground.setPixelColor(2, 0, 0, 255);   // blue = measuring

  for (int w = 0; w < 5; w++) {                    // warm-up
    for (int i = 0; i < NUM_PADS; i++) readPad(i);
    delay(20);
  }

  for (int i = 0; i < NUM_PADS; i++) {             // average 20 readings
    uint32_t total = 0;
    for (int s = 0; s < 20; s++) {
      total += readPad(i);
      delay(5);
    }
    baseline[i] = total / 20;
    smoothed[i] = baseline[i];
    lastBeep[i] = 0;
  }

  CircuitPlayground.clearPixels();                 // light off = ready
  if (!PLOTTER_MODE) Serial.println("R");          // ready
}

void setup() {
  Serial.begin(115200);
  CircuitPlayground.begin();
  delay(1000);
  calibrate();
}

void loop() {
  // Press button A (left) to re-measure the base
  if (CircuitPlayground.leftButton()) {
    delay(300);
    calibrate();
  }

  for (int i = 0; i < NUM_PADS; i++) {
    // Read and smooth
    uint16_t reading = readPad(i);
    smoothed[i] = smoothed[i] * (1.0 - SMOOTHING) + reading * SMOOTHING;

    // How far above base? This is "how close is the hand"
    int change = (int)smoothed[i] - baseline[i];
    int closeness = constrain(change, 0, FULL_TOUCH);

    if (PLOTTER_MODE) {
      Serial.print(padNames[i]);
      Serial.print("_change:");
      Serial.print(change);
      Serial.print(",");
    }

    if (change > NOISE) {
      // Closer = shorter gap between beeps
      int gap = map(closeness, NOISE, FULL_TOUCH, SLOWEST, FASTEST);
      // Closer = higher pitch (up to one octave above the pad's note)
      int pitch = map(closeness, NOISE, FULL_TOUCH, notes[i], notes[i] * 2);
      // Closer = louder on the computer (0 to 100)
      int level = map(closeness, NOISE, FULL_TOUCH, 10, 100);

      if (millis() - lastBeep[i] > (unsigned long)gap) {
        if (!PLOTTER_MODE) {
          // Message to the computer:  N,pad,pitch,length,level
          Serial.print("N,");
          Serial.print(i);
          Serial.print(",");
          Serial.print(pitch);
          Serial.print(",");
          Serial.print(BEEP_MS);
          Serial.print(",");
          Serial.println(level);
        }
        lastBeep[i] = millis();
      }

      // Closer = brighter light (on the pixel next to the pad)
      int brightness = map(closeness, NOISE, FULL_TOUCH, 10, 255);
      CircuitPlayground.setPixelColor(padPixel[i], brightness, brightness * 0.7, 0);  // mustard
    } else {
      CircuitPlayground.setPixelColor(padPixel[i], 0, 0, 0);
    }
  }

  if (PLOTTER_MODE) Serial.println();
}
