#include <TWELITE>

const uint8_t PIN_DO1 = 18;
const uint8_t PIN_DI1 = 12;

/*** the setup procedure (called on boot) */
void setup() {
    pinMode(PIN_DO1, OUTPUT);
    pinMode(PIN_DI1, INPUT_PULLUP); 

    Buttons.setup(5);
    Buttons.begin(1UL << PIN_DI1, 5, 10);

    Serial << "--- act4 (button&LED) ---" << crlf;
}

/*** the loop procedure (called every event) */
void loop() {
    if (Buttons.available()) {
        uint32_t bm, cm;
        Buttons.read(bm, cm);

        if (bm & (1UL << 12)) {
            digitalWrite(PIN_DO1, HIGH);
            Serial << "Button Released!" << crlf;
        } else {
            digitalWrite(PIN_DO1, LOW);
            Serial << "Button Pressed!" << crlf;
        }
    }
}