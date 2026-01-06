#include <Arduino.h>
#include <USBComposite.h>

USBMIDI midi;

void setup() {
    // 1. SET INDIKATOR LED
    pinMode(PC13, OUTPUT);

    // 2. TES KEDIP 5 KALI (Tanda Chip Hidup)
    for(int i=0; i<5; i++) {
        digitalWrite(PC13, LOW); // Nyala (BluePill aktif LOW)
        delay(100);
        digitalWrite(PC13, HIGH); // Mati
        delay(100);
    }

    // 3. Pancingan USB
    pinMode(PA12, OUTPUT);
    digitalWrite(PA12, LOW);
    delay(500);
    pinMode(PA12, INPUT);

    USBComposite.setProductId(0x0030);
    midi.begin();
    USBComposite.begin();
}

void loop() {
    midi.poll();

    // LED berkedip pelan saat standby
    static uint32_t t = 0;
    if (millis() - t > 500) {
        digitalWrite(PC13, !digitalRead(PC13));
        t = millis();
    }
}
