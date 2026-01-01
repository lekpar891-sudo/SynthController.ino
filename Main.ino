#include <Arduino.h>

/* MIDI Controller STM32 - Versi USB MIDI Official */
#define MIDI_HW Serial1 // MIDI Out Fisik (PA9)

const uint8_t ROW_PINS[8] = {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7};
const uint8_t COL_PINS[8] = {PB0, PB1, PB2, PB10, PB11, PB12, PB13, PB14};
uint8_t matrixState[8][8] = {0};

void setup() {
  // Serial otomatis jadi USB MIDI karena opsi usb=midi di file YAML
  Serial.begin(115200);
  MIDI_HW.begin(31250);

  for (int i = 0; i < 8; i++) {
    pinMode(ROW_PINS[i], INPUT_PULLUP);
    pinMode(COL_PINS[i], INPUT);
  }
}

void loop() {
  for (int c = 0; c < 8; c++) {
    pinMode(COL_PINS[c], OUTPUT);
    digitalWrite(COL_PINS[c], LOW);
    for (int r = 0; r < 8; r++) {
      bool pressed = (digitalRead(ROW_PINS[r]) == LOW);
      if (pressed != matrixState[r][c]) {
        matrixState[r][c] = pressed;
        uint8_t note = 48 + (r * 8 + c); 
        if (pressed) {
          Serial.write(0x90); Serial.write(note); Serial.write(100);
          MIDI_HW.write(0x90); MIDI_HW.write(note); MIDI_HW.write(100);
        } else {
          Serial.write(0x80); Serial.write(note); Serial.write(0);
          MIDI_HW.write(0x80); MIDI_HW.write(note); MIDI_HW.write(0);
        }
      }
    }
    pinMode(COL_PINS[c], INPUT);
  }
  delay(5);
}
