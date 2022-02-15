#include <TWELITE>
#include <EASTL/fixed_string.h>
#include <EASTL/unique_ptr.h>

extern void test_fixed_string();
extern void test_fixed_list();

void help() {
    Serial << crlf << " s : fixed_string";
    Serial << crlf << " l : fixed_list";
    Serial << crlf;
}

/*** the setup procedure (called on boot) */
void setup() {
    Serial << crlf << "!!!Unit_EASTL";
    Serial << crlf << "!sample codes using EASTL.";
    help();
}

/*** the loop procedure (called every event) */
void loop() {
    if (Serial.available()) {
        int c = Serial.read();

        switch(c) {
            case 's': test_fixed_string(); break;
            case 'l': test_fixed_list(); break;
            case 0xd: help(); break;
            case '!': the_twelite.reset_system(); break;
            default: break;
        }
    }
}
