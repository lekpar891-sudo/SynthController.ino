#include <Arduino.h>
#include <USBComposite.h>

/* ---------- KONFIGURASI USB MIDI ---------- */
USBMIDI usbMIDI;

/* ---------- KONFIGURASI PIN ---------- */
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
const uint8_t BASE_NOTE = 48;

uint8_t padNoteMap[64];
uint8_t matrixState[8][8] = {0};
unsigned long lastDebounce[8][8] = {0};
const unsigned long DEBOUNCE_MS = 8;

volatile int encoderPos = 0;
volatile bool encoderMoved = false;
int currentOctaveOffset = 0;
int transpose = 0;
unsigned long lastScanTime = 0;

/* ---------- FUNGSI MIDI ---------- */
void midiSendNoteOn(uint8_t ch, uint8_t note, uint8_t vel) {
  uint8_t status = 0x90 | ((ch - 1) & 0x0F);
  MIDI_HARDWARE_SERIAL.write(status); MIDI_HARDWARE_SERIAL.write(note); MIDI_HARDWARE_SERIAL.write(vel);
  usbMIDI.sendNoteOn(ch - 1, note, vel);
}

void midiSendNoteOff(uint8_t ch, uint8_t note, uint8_t vel) {
  uint8_t status = 0x80 | ((ch - 1) & 0x0F);
  MIDI_HARDWARE_SERIAL.write(status); MIDI_HARDWARE_SERIAL.write(note); MIDI_HARDWARE_SERIAL.write(vel);
  usbMIDI.sendNoteOff(ch - 1, note, vel);
}

void midiSendCC(uint8_t ch, uint8_t cc, uint8_t val) {
  uint8_t status = 0xB0 | ((ch - 1) & 0x0F);
  MIDI_HARDWARE_SERIAL.write(status); MIDI_HARDWARE_SERIAL.write(cc); MIDI_HARDWARE_SERIAL.write(val);
  usbMIDI.sendControlChange(ch - 1, cc, val);
}

void midiSendPitchbend(uint8_t ch, int value14) {
  uint8_t status = 0xE0 | ((ch - 1) & 0x0F);
  MIDI_HARDWARE_SERIAL.write(status); MIDI_HARDWARE_SERIAL.write(value14 & 0x7F); MIDI_HARDWARE_SERIAL.write((value14 >> 7) & 0x7F);
  usbMIDI.sendPitchWheel(value14);
}

/* ---------- LOGIKA SCAN ---------- */
void scanMatrix() {
  for (int c = 0; c < 8; c++) {
    pinMode(COL_PINS[c], OUTPUT);
    digitalWrite(COL_PINS[c], LOW);
    delayMicroseconds(10);
    for (int r = 0; r < 8; r++) {
      pinMode(ROW_PINS[r], INPUT_PULLUP);
      bool pressed = (digitalRead(ROW_PINS[r]) == LOW);
      if (pressed != matrixState[r][c] && (millis() - lastDebounce[r][c] > DEBOUNCE_MS)) {
        lastDebounce[r][c] = millis();
        matrixState[r][c] = pressed;
        uint8_t note = BASE_NOTE + (r * 8 + c) + (currentOctaveOffset * 12) + transpose;
        if (pressed) midiSendNoteOn(MIDI_CHANNEL, note, 100);
        else midiSendNoteOff(MIDI_CHANNEL, note, 0);
      }
    }
    pinMode(COL_PINS[c], INPUT);
  }
}

void readAnalogs() {
  int pbx = map(analogRead(PIN_PITCH_WHEEL), 0, 1023, 0, 16383);
  midiSendPitchbend(MIDI_CHANNEL, pbx);
  midiSendCC(MIDI_CHANNEL, 10, map(analogRead(PIN_POT_BALANCE), 0, 1023, 0, 127));
  midiSendCC(MIDI_CHANNEL, 7, map(analogRead(PIN_POT_MASTER), 0, 1023, 0, 127));
  for (int i = 0; i < 3; i++) {
    midiSendCC(MIDI_CHANNEL, 70 + i, map(analogRead(PIN_EQ[i]), 0, 1023, 0, 127));
  }
}

void encoderISR() {
  uint8_t s = (digitalRead(PIN_ENCODER_A) << 1) | digitalRead(PIN_ENCODER_B);
  static uint8_t last = 0;
  if (s != last) {
    if ((last == 0b00 && s == 0b01) || (last == 0b11 && s == 0b10)) encoderPos++;
    else encoderPos--;
    encoderMoved = true;
    last = s;
  }
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
  pinMode(PIN_ENCODER_A, INPUT_PULLUP);
  pinMode(PIN_ENCODER_B, INPUT_PULLUP);
  pinMode(PIN_ENCODER_SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), encoderISR, CHANGE);
}

void loop() {
  usbMIDI.poll();
  if (millis() - lastScanTime > 10) {
    lastScanTime = millis();
    scanMatrix();
    readAnalogs();
  }
  if (encoderMoved) {
    noInterrupts();
    int delta = encoderPos; encoderPos = 0; encoderMoved = false;
    interrupts();
    if (digitalRead(PIN_ENCODER_SW) == LOW) currentOctaveOffset += (delta > 0 ? 1 : -1);
    else transpose += (delta > 0 ? 1 : -1);
  }
}
