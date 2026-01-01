#include <Arduino.h>
#include <USBComposite.h>

/* ---------- KONFIGURASI USB MIDI ---------- */
USBMIDI usbMIDI;

/* ---------- KONFIGURASI PIN (Sama seperti sebelumnya) ---------- */
const uint8_t ROW_PINS[8] = {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7};
const uint8_t COL_PINS[8] = {PB0, PB1, PB2, PB10, PB11, PB12, PB13, PB14};
const uint8_t PIN_PITCH_WHEEL = A2;
const uint8_t PIN_POT_BALANCE = A3;
const uint8_t PIN_POT_MASTER = A4;
const uint8_t PIN_EQ[3] = {A5, A6, A7}; 
const uint8_t PIN_ENCODER_A = PB6;
const uint8_t PIN_ENCODER_B = PB7;
const uint8_t PIN_ENCODER_SW = PB8;

#define MIDI_HARDWARE_SERIAL Serial1
const uint16_t MIDI_CHANNEL = 1;
uint8_t matrixState[8][8] = {0};
unsigned long lastScanTime = 0;

void midiSendNoteOn(uint8_t ch, uint8_t note, uint8_t vel) {
  MIDI_HARDWARE_SERIAL.write(0x90 | (ch-1)); MIDI_HARDWARE_SERIAL.write(note); MIDI_HARDWARE_SERIAL.write(vel);
  usbMIDI.sendNoteOn(ch-1, note, vel);
}
void midiSendNoteOff(uint8_t ch, uint8_t note, uint8_t vel) {
  MIDI_HARDWARE_SERIAL.write(0x80 | (ch-1)); MIDI_HARDWARE_SERIAL.write(note); MIDI_HARDWARE_SERIAL.write(vel);
  usbMIDI.sendNoteOff(ch-1, note, vel);
}
void midiSendCC(uint8_t ch, uint8_t cc, uint8_t val) {
  MIDI_HARDWARE_SERIAL.write(0xB0 | (ch-1)); MIDI_HARDWARE_SERIAL.write(cc); MIDI_HARDWARE_SERIAL.write(val);
  usbMIDI.sendControlChange(ch-1, cc, val);
}

void setup() {
  USBComposite.setProductId(0x0030);
  usbMIDI.begin();
  USBComposite.begin();
  
  MIDI_HARDWARE_SERIAL.begin(31250);
  for (int i = 0; i < 8; i++) {
    pinMode(ROW_PINS[i], INPUT_PULLUP);
    pinMode(COL_PINS[i], INPUT);
  }
}

void loop() {
  usbMIDI.poll();
  // Scan Matrix & Analog logic di sini (lanjutkan seperti script awal Anda)
  // ... (Gunakan scanMatrix() dan readAnalogs() dari script asli Anda)
}
