#include <Arduino.h>
#include <USBComposite.h>

USBMIDI midi;

/* ---------- KONFIGURASI PIN ---------- */
// Matrix 8 Baris (Row) x 10 Kolom (Col)
const uint8_t ROW_PINS[8] = {PA2, PA3, PA5, PA6, PA7, PB3, PB4, PB5};
const uint8_t COL_PINS[10] = {PB0, PB1, PB2, PB10, PB11, PB12, PB13, PB14, PA8, PA10};

const uint8_t PIN_PITCH_X = A0; 
const uint8_t PIN_PITCH_Y = A1; 
const uint8_t PIN_POT_MASTER = A4;
const uint8_t PIN_ENCODER_A = PB6;
const uint8_t PIN_ENCODER_B = PB7;
const uint8_t PIN_ENCODER_SW = PB8;

/* ---------- VARIABEL KONTROL ---------- */
uint8_t matrixState[8][10] = {0};
int currentOctave = 0;
int transpose = 0;
volatile int encoderPos = 0;
volatile bool encoderMoved = false;
bool isXTremoloMode = false;

int lastX = -1, lastY_Up = -1, lastY_Down = -1, lastVol = -1;
unsigned long lastBtnPress = 0;

void encoderISR() {
    if (digitalRead(PIN_ENCODER_A) == digitalRead(PIN_ENCODER_B)) encoderPos++;
    else encoderPos--;
    encoderMoved = true;
}

// Memaksa koneksi USB agar terdeteksi HP
void forceUSBReset() {
    pinMode(PA12, OUTPUT);
    digitalWrite(PA12, LOW);
    delay(200);
    pinMode(PA12, INPUT);
    delay(200);
}

void setup() {
    forceUSBReset(); 

    USBComposite.setProductId(0x0030);
    midi.begin();
    USBComposite.begin();
    
    for (int i = 0; i < 8; i++) pinMode(ROW_PINS[i], INPUT_PULLUP);
    for (int i = 0; i < 10; i++) pinMode(COL_PINS[i], INPUT);
    
    pinMode(PIN_ENCODER_A, INPUT_PULLUP);
    pinMode(PIN_ENCODER_B, INPUT_PULLUP);
    pinMode(PIN_ENCODER_SW, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), encoderISR, CHANGE);
}

void loop() {
    midi.poll();

    // 1. Klik Encoder untuk ganti mode X
    if (digitalRead(PIN_ENCODER_SW) == LOW && (millis() - lastBtnPress > 300)) {
        isXTremoloMode = !isXTremoloMode;
        lastBtnPress = millis();
    }

    // 2. Scan Matrix 8x10
    for (int c = 0; c < 10; c++) {
        pinMode(COL_PINS[c], OUTPUT);
        digitalWrite(COL_PINS[c], LOW);
        for (int r = 0; r < 8; r++) {
            bool pr = (digitalRead(ROW_PINS[r]) == LOW);
            if (pr != matrixState[r][c]) {
                matrixState[r][c] = pr;
                int bID = (r * 10) + c; 
                if (bID < 61) {
                    uint8_t note = 36 + bID + (currentOctave * 12) + transpose;
                    if (pr) midi.sendNoteOn(0, note, 100);
                    else midi.sendNoteOff(0, note, 0);
                } else if (bID >= 61 && bID < 77) {
                    uint8_t ccList[] = {80,81,82,83,84,85,86,87,88,89,90,91,92,102,103,104};
                    midi.sendControlChange(0, ccList[bID - 61], pr ? 127 : 0);
                }
            }
        }
        pinMode(COL_PINS[c], INPUT);
    }

    // 3. Joystick X
    int rawX = analogRead(PIN_PITCH_X);
    if (isXTremoloMode) {
        int vx = map(rawX, 0, 1023, 0, 127);
        if (abs(vx - lastX) > 1) { midi.sendControlChange(0, 1, vx); lastX = vx; }
    } else {
        int vx = map(rawX, 0, 1023, 0, 16383);
        if (abs(vx - lastX) > 100) { midi.sendPitchChange(0, vx); lastX = vx; }
    }

    // 4. Joystick Y (Up = Vibrato, Down = Filter)
    int rawY = analogRead(PIN_PITCH_Y);
    if (rawY > 532) {
        int v = map(rawY, 532, 1023, 0, 127);
        if (v != lastY_Up) { midi.sendControlChange(0, 1, v); lastY_Up = v; }
    } else if (lastY_Up != 0) { midi.sendControlChange(0, 1, 0); lastY_Up = 0; }
    
    if (rawY < 492) {
        int v = map(rawY, 492, 0, 0, 127);
        if (v != lastY_Down) { midi.sendControlChange(0, 74, v); lastY_Down = v; }
    } else if (lastY_Down != 0) { midi.sendControlChange(0, 74, 0); lastY_Down = 0; }

    // 5. Volume Master
    int vv = map(analogRead(PIN_POT_MASTER), 0, 1023, 0, 127);
    if (abs(vv - lastVol) > 1) { midi.sendControlChange(0, 7, vv); lastVol = vv; }

    // 6. Encoder Transpose
    if (encoderMoved) {
        noInterrupts();
        int delta = encoderPos; 
        encoderPos = 0; 
        encoderMoved = false;
        interrupts();
        if (delta > 0) transpose++; 
        else if (delta < 0) transpose--;
    }
} // <--- Pastikan kurung tutup ini ada dan tidak terhapus
