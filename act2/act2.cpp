#include <TWELITE>

const uint8_t PIN_DO1 = 18;
int iLedCounter = 0;

/*** the setup procedure (called on boot) */
void setup() {
    pinMode(PIN_DO1, OUTPUT);
    digitalWrite(PIN_DO1, LOW); // TURN DO1 ON

    Timer0.begin(10); // 10Hz Timer 

    Serial << "--- act2 (using a timer) ---" << crlf;
}

/*** loop procedure (called every event) */
void loop() {
    if (Timer0.available()) {
        if (iLedCounter == 0) {
            digitalWrite(PIN_DO1, HIGH);
            iLedCounter = 1;
        } else {
            digitalWrite(PIN_DO1, LOW);
            iLedCounter = 0;
        }
    }
}