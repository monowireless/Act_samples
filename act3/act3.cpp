#include <TWELITE>

const uint8_t PIN_DO1 = 18;
const uint8_t PIN_DO2 = 19;
int iLedCounter1 = 0;
int iLedCounter2 = 0;

/*** the setup procedure (called on boot) */
void setup() {
    pinMode(PIN_DO1, OUTPUT);
    digitalWrite(PIN_DO1, LOW); // TURN DO1 ON

    pinMode(PIN_DO2, OUTPUT);
    digitalWrite(PIN_DO2, LOW); // TURN DO1 ON

    Timer0.begin(10); // 10Hz Timer 
    Timer1.begin(5); // 5Hz Timer 

    Serial << "--- act3 (using 2 timers) ---" << crlf;
}

/*** loop procedure (called every event) */
void loop() {
    if (Timer0.available()) {
        if (iLedCounter1 == 0) {
            digitalWrite(PIN_DO1, HIGH);
            iLedCounter1 = 1;
        } else {
            digitalWrite(PIN_DO1, LOW);
            iLedCounter1 = 0;
        }
    }

    if (Timer1.available()) {
        if (iLedCounter2 == 0) {
            digitalWrite(PIN_DO2, HIGH);
            iLedCounter2 = 1;
        } else {
            digitalWrite(PIN_DO2, LOW);
            iLedCounter2 = 0;
        }
    }
}