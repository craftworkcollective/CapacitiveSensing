#include <Adafruit_CircuitPlayground.h>

// ===== Change these to test =====
const int NUM_PADS   = 3;     // 1 to 7: uses pads A1, A2, A3... in order
const int NOISE      = 35;    // ignore changes smaller than this (no sound)
const int FULL_TOUCH = 150;   // change at a full touch: check the Serial Plotter
const int SLOWEST    = 600;   // ms between beeps when hand is far
const int FASTEST    = 60;    // ms between beeps at full touch
const int SAMPLES    = 4;     // readings averaged each time
const float SMOOTHING = 0.6;  // 0.1 = very smooth/slow, 1.0 = no smoothing


// ================================

const uint8_t  allPads[7]  = {A1, A2, A3, A4, A5, A6, A7};
const char*    padNames[7] = {"A1", "A2", "A3", "A4", "A5", "A6", "A7"};
const uint16_t notes[7]    = {262, 294, 330, 349, 392, 440, 494};  // C D E F G A B

uint16_t baseline[7];
float    smoothed[7];
unsigned long lastBeep[7];

// Average several quick readings from one pad
uint16_t readPad(int i) {
  uint32_t total = 0;
  for (int s = 0; s < SAMPLES; s++) {
    total += CircuitPlayground.readCap(allPads[i]);
  }
  return total / SAMPLES;
}

// Measure each pad's resting value (don't touch anything!)
void calibrate() {
  CircuitPlayground.setPixelColor(0, 0, 0, 255);  // blue = measuring

  for (int w = 0; w < 5; w++) {                   // warm-up
    for (int i = 0; i < NUM_PADS; i++) readPad(i);
    delay(20);
  }

  for (int i = 0; i < NUM_PADS; i++) {            // average 20 readings
    uint32_t total = 0;
    for (int s = 0; s < 20; s++) {
      total += readPad(i);
      delay(5);
    }
    baseline[i] = total / 20;
    smoothed[i] = baseline[i];
    lastBeep[i] = 0;
  }

  CircuitPlayground.clearPixels();                // light off = ready
}

void setup() {
  Serial.begin(9600);
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

    // Print for Serial Monitor / Plotter
    Serial.print(padNames[i]);
    Serial.print("_change:");
    Serial.print(change);
    Serial.print(",");

    if (change > NOISE) {
      // Closer = shorter gap between beeps
      int gap = map(closeness, NOISE, FULL_TOUCH, SLOWEST, FASTEST);
      // Closer = higher pitch (up to one octave above the pad's note)
      int pitch = map(closeness, NOISE, FULL_TOUCH, notes[i], notes[i] * 2);

      if (millis() - lastBeep[i] > (unsigned long)gap) {
        CircuitPlayground.playTone(pitch, 40, false);  // short beep, don't wait
        lastBeep[i] = millis();
      }

      // Closer = brighter light (on the pixel next to the pad)
      int brightness = map(closeness, NOISE, FULL_TOUCH, 10, 255);
      CircuitPlayground.setPixelColor(i, brightness, brightness * 0.7, 0);  // mustard
    } else {
      CircuitPlayground.setPixelColor(i, 0, 0, 0);
    }
  }
  Serial.println();
}