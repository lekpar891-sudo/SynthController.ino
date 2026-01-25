#include <Arduino.h>
#include <USBComposite.h>

// --- KONFIGURASI 61 KEYS (SESUAI KODE ANDA - JANGAN DIUBAH) ---
#define CLOCK_ENABLE_PIN PB4
#define PARALLEL_LOAD_PIN PB7
#define CLOCK_PIN PB6
#define SERIAL_DATA_PIN PB8
#define FIRST_NOTE (60 - 12*2)
#define NOTES_COUNT (5 * 12 + 1)
#define REGISTRIES_COUNT (NOTES_COUNT/8 + 1)
#define LAST_REGISTRY_NOTE (NOTES_COUNT - (REGISTRIES_COUNT-1)*8)
#define DEBOUNCING_CYCLES 128
byte keyStates[NOTES_COUNT] = {0};

// --- KONFIGURASI PANEL KONTROL (INTRO, VAR, START/STOP, KEY SET) ---
// Pin PA0 s/d PA9 (Total 10 tombol). Hubungkan ke tombol lalu ke GND.
const int panelPins[] = {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9};
const int numPanel = 10;
// Mapping: 1-3 Intro, 4-7 Var, 8 Start/Stop, 9-10 Key Set

// --- KONFIGURASI ROTARY ENCODER (TEMPO) ---
#define ENC_A PB0
#define ENC_B PB1
int lastEncoded = 0;

// --- INISIALISASI USB COMPOSITE (MIDI + JOYSTICK) ---
USBHID HID;
HIDJoystick Joystick(HID);
USBMIDI midi;

void setup() {
  // Setup Pin 61 Keys (Persis Kode Anda)
  pinMode(CLOCK_ENABLE_PIN, OUTPUT);
  pinMode(PARALLEL_LOAD_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(SERIAL_DATA_PIN, INPUT);
  digitalWrite(PARALLEL_LOAD_PIN, HIGH);
  digitalWrite(CLOCK_ENABLE_PIN, HIGH);

  // Setup Pin Panel Kontrol (Joystick Buttons)
  for(int i=0; i<numPanel; i++) {
    pinMode(panelPins[i], INPUT_PULLUP);
  }

  // Setup Pin Rotary Encoder
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);

  // Memulai USB Composite
  USBComposite.setProductId(0x0031); // ID Perangkat
  midi.begin();
  Joystick.begin();
  USBComposite.begin();
}

void loop() {
  // --- BAGIAN 1: LOGIKA 61 KEYS (PERSIS KODE ASLI ANDA) ---
  digitalWrite(PARALLEL_LOAD_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(PARALLEL_LOAD_PIN, HIGH);
  delayMicroseconds(5);

  for (int r = 0; r < REGISTRIES_COUNT; r++) {
    digitalWrite(CLOCK_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(CLOCK_ENABLE_PIN, LOW);
    delayMicroseconds(5);
    
    byte incoming = shiftIn(SERIAL_DATA_PIN, CLOCK_PIN, MSBFIRST);
    
    digitalWrite(CLOCK_ENABLE_PIN, HIGH);
    delayMicroseconds(5);

    int registryOffset = r * 8;
    int lastRegistryNote = (r == (REGISTRIES_COUNT - 1)) ? LAST_REGISTRY_NOTE : 8;

    for (int i = 0; i < lastRegistryNote; i++) {
      byte* keyState = &keyStates[registryOffset + i];
      bool isCurrentlyPressed = incoming & (1 << i);
      bool wasPressedBefore = *keyState;

      if (!wasPressedBefore && isCurrentlyPressed) {
        midi.sendNoteOn(0, FIRST_NOTE + registryOffset + i, 127);
        *keyState = DEBOUNCING_CYCLES;
      }

      if (wasPressedBefore && !isCurrentlyPressed) {
        if (*keyState > 0) *keyState = *keyState - 1;
        if (*keyState == 0) {
          midi.sendNoteOff(0, FIRST_NOTE + registryOffset + i, 127);
        }
      }
    }
  }

  // --- BAGIAN 2: LOGIKA PANEL KONTROL (DIUBAH KE JOYSTICK BUTTONS) ---
  // Ini menggantikan logika Joystick Diego Viejo agar sesuai dengan tombol fisik
  for (int j = 0; j < numPanel; j++) {
    if (digitalRead(panelPins[j]) == LOW) {
      Joystick.button(j + 1, 1); // Tekan tombol joystick
    } else {
      Joystick.button(j + 1, 0); // Lepas tombol joystick
    }
  }

  // --- BAGIAN 3: LOGIKA ROTARY ENCODER (TEMPO) ---
  int MSB = digitalRead(ENC_A);
  int LSB = digitalRead(ENC_B);
  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;

  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
    midi.sendControlChange(0, 20, 1); // CC 20 untuk Tempo Up
  }
  if(sum == 0b1110 || sum == 0b1000 || sum == 0b0001 || sum == 0b0111) {
    midi.sendControlChange(0, 20, 127); // CC 20 untuk Tempo Down
  }
  lastEncoded = encoded;

  delayMicroseconds(100); 
}
