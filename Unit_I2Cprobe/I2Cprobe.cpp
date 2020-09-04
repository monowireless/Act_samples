#include <TWELITE>

/*** the setup procedure (called on boot) */
void setup() {
    Wire.begin();
}

/*** the second setup procedure called once after setup() */
void begin() {
    int stat;
    for (int i = 1; i < 127; i++) {
        Wire.beginTransmission(i);
        stat = Wire.endTransmission();
        if (stat == 0)
            Serial << format("\033[7m[!%02x]\033[0m", i, stat);
        else
            Serial << format("[%02x]", i, stat);
    }
    Serial << crlf;
}

/*** the loop procedure (called every event) */
void loop() {
}