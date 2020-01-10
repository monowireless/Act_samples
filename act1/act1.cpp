#include <TWELITE>

const uint8_t PIN_LED = 18;
const uint8_t PIN_BUTTON = 12;

void setup() {
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_BUTTON, INPUT_PULLUP); 

    Buttons.setup(5);
    Buttons.begin(1UL << PIN_BUTTON, 5, 10);

    Serial << "--- First Guide1 ---" << mwx::crlf;
}

/*** loop procedure (called every event) */
void loop() {
    if (Buttons.available()) {
        uint32_t bm, cm;
        Buttons.read(bm, cm);

        if (bm & (1UL << 12)) {
            digitalWrite(PIN_LED, HIGH);
            Serial << "Button Released!" << mwx::crlf;
        } else {
            digitalWrite(PIN_LED, LOW);
            Serial << "Button Pressed!" << mwx::crlf;
        }
    }
}