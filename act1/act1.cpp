#include <TWELITE>

/*** the setup procedure (called on boot) */
void setup() {
    pinMode(18, OUTPUT);
    digitalWrite(18, LOW); // TURN DO1 ON
    Serial << "--- act1 (blink LED) ---" << crlf;
}

/*** loop procedure (called every event) */
void loop() {
    delay(500); // 500ms delay
    digitalWrite(18, HIGH); // TURN DO1 OFF
    delay(500); // 500ms delay
    digitalWrite(18, LOW); // TURN DO1 ON
}