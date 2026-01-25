#include <Arduino.h>
#include <USBComposite.h>

// --- KONFIGURASI 61 KEYS (DARI KODE ANDA) ---
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

// --- KONFIGURASI ANALOG PS ---
#define ANALOG_X_PIN PA4
#define ANALOG_Y_PIN PA5

// --- KONFIGURASI PANEL KONTROL (Intro, Var, Start/Stop) ---
const int panelPins[] = {PA0, PA1, PA2, PA3, PA6, PA7, PB12, PB13, PB14, PB15};
const int numPanel = 10;

// --- KONFIGURASI ROTARY ENCODER ---
#define ENC_A PB0
#define ENC_B PB1
int lastEncoded = 0;

// USB Objects
USBHID HID;
HIDJoystick Joystick(HID);
USBMIDI midi;

void setup() {
  // Setup Pins 61 Keys
  pinMode(CLOCK_ENABLE_PIN, OUTPUT);
  pinMode(PARALLEL_LOAD_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(SERIAL_DATA_PIN, INPUT);
  digitalWrite(PARALLEL_LOAD_PIN, HIGH);
  digitalWrite(CLOCK_ENABLE_PIN, HIGH);

  // Setup Panel & Analog
  for(int i=0; i<numPanel; i++) pinMode(panelPins[i], INPUT_PULLUP);
  pinMode(ANALOG_X_PIN, INPUT_ANALOG);
  pinMode(ANALOG_Y_PIN, INPUT_ANALOG);

  // Setup Encoder
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);

  // Start USB
  USBComposite.setProductId(0x0031);
  midi.begin();
  Joystick.begin();
  USBComposite.begin();
}

void loop() {
  // --- 1. LOGIKA 61 KEYS (Kirim MIDI Note) ---
  digitalWrite(PARALLEL_LOAD_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(PARALLEL_LOAD_PIN, HIGH);
  
  for (int r = 0; r < REGISTRIES_COUNT; r++) {
    digitalWrite(CLOCK_PIN, HIGH);
    digitalWrite(CLOCK_ENABLE_PIN, LOW);
    byte incoming = shiftIn(SERIAL_DATA_PIN, CLOCK_PIN, MSBFIRST);
    digitalWrite(CLOCK_ENABLE_PIN, HIGH);

    int registryOffset = r * 8;
    int lastRegistryNote = (r == (REGISTRIES_COUNT - 1)) ? LAST_REGISTRY_NOTE : 8;

    for (int i = 0; i < lastRegistryNote; i++) {
      byte* keyState = &keyStates[registryOffset + i];
      bool isCurrentlyPressed = incoming & (1 << i);
      if (!(*keyState) && isCurrentlyPressed) {
        midi.sendNoteOn(0, FIRST_NOTE + registryOffset + i, 127);
        *keyState = DEBOUNCING_CYCLES;
      }
      if ((*keyState) && !isCurrentlyPressed) {
        if (*keyState > 0) *keyState = *keyState - 1;
        if (*keyState == 0) midi.sendNoteOff(0, FIRST_NOTE + registryOffset + i, 127);
      }
    }
  }

  // --- 2. LOGIKA ANALOG PS (Kirim Joystick Axis) ---
  // Membaca analog dan merubah rentang ke standar Joystick (-32767 sampai 32767)
  int valX = analogRead(ANALOG_X_PIN);
  int valY = analogRead(ANALOG_Y_PIN);
  Joystick.X(map(valX, 0, 4095, -32767, 32767));
  Joystick.Y(map(valY, 0, 4095, -32767, 32767));

  // --- 3. LOGIKA PANEL KONTROL (Kirim Joystick Buttons) ---
  for (int j = 0; j < numPanel; j++) {
    Joystick.button(j + 1, !digitalRead(panelPins[j]));
  }

  // --- 4. LOGIKA ROTARY ENCODER (Kirim MIDI CC Tempo) ---
  int MSB = digitalRead(ENC_A);
  int LSB = digitalRead(ENC_B);
  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;
  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) midi.sendControlChange(0, 20, 1);
  if(sum == 0b1110 || sum == 0b1000 || sum == 0b0001 || sum == 0b0111) midi.sendControlChange(0, 20, 127);
  lastEncoded = encoded;

  delayMicroseconds(200);
}
