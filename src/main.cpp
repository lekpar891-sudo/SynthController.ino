#include <Arduino.h>
#include <USBComposite.h>

// Inisialisasi MIDI
USBMIDI midi;

// Konfigurasi Pin Matrix Anda
const uint8_t ROW_PINS[8] = {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7};
const uint8_t COL_PINS[8] = {PB0, PB1, PB2, PB10, PB11, PB12, PB13, PB14};
uint8_t matrixState[8][8] = {0};

void setup() {
    // Setup USB agar dideteksi sebagai MIDI Device oleh Android
    USBComposite.setProductId(0x0030);
    midi.begin();
    USBComposite.begin();

    for (int i = 0; i < 8; i++) {
        pinMode(ROW_PINS[i], INPUT_PULLUP);
        pinMode(COL_PINS[i], INPUT);
    }
}

void loop() {
    // Wajib dipanggil agar koneksi USB tetap stabil
    midi.poll();

    // Scan Tombol
    for (int c = 0; c < 8; c++) {
        pinMode(COL_PINS[c], OUTPUT);
        digitalWrite(COL_PINS[c], LOW);
        for (int r = 0; r < 8; r++) {
            bool pressed = (digitalRead(ROW_PINS[r]) == LOW);
            if (pressed != matrixState[r][c]) {
                matrixState[r][c] = pressed;
                uint8_t note = 48 + (r * 8 + c); 
                if (pressed) {
                    midi.sendNoteOn(0, note, 100);
                } else {
                    midi.sendNoteOff(0, note, 0);
                }
            }
        }
        pinMode(COL_PINS[c], INPUT);
    }
}
