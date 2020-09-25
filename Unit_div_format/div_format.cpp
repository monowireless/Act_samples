#include <TWELITE>

/*** the setup procedure (called on boot) */
void setup() {
}

/*** the begin procedure (called once) */
void begin() {
    the_twelite.stop_watchdog(); // stop watch dog (wait in this function longer time.)

    int tblv[] = { 0, 12345, -12345, 45, 300, -45, -400, 1234567, -1234567};
    int tblf[] = { 100, 10, 1000 };

    Serial.set_timeout(0xFF); // no timeout.

    for (auto f : tblf) {
        for (auto y : tblv) {
            div_result_i32 x;
            char_t c;

            Serial << crlf << "-- Press ANY KEY --";
            Serial >> c;

            switch(f) {
            case 10: x = div10(y); break;
            case 100: x = div100(y); break;
            case 1000: x = div1000(y); break;
            }

            Serial << crlf << format("div%d(%d)             : ", f, y) << x;
            Serial << crlf << format("div%d(%d).format( , ) : ", f, y) << x.format();
            Serial << crlf << format("div%d(%d).format(0,1) : ", f, y) << x.format(0,1);
            Serial << crlf << format("div%d(%d).format(0,2) : ", f, y) << x.format(0,2);
            Serial << crlf << format("div%d(%d).format(0,3) : ", f, y) << x.format(0,3);
            Serial << crlf << format("div%d(%d).format(0,4) : ", f, y) << x.format(0,4);
            Serial << crlf << format("div%d(%d).format(0,5) : ", f, y) << x.format(0,5);
            Serial << crlf << format("div%d(%d).format(4, ) : ", f, y) << x.format(4);
            Serial << crlf << format("div%d(%d).format(4,1) : ", f, y) << x.format(4,1);
            Serial << crlf << format("div%d(%d).format(4,2) : ", f, y) << x.format(4,2);
            Serial << crlf << format("div%d(%d).format(4,3) : ", f, y) << x.format(4,3);
            Serial << crlf << format("div%d(%d).format(4,4) : ", f, y) << x.format(4,4);
            Serial << crlf << format("div%d(%d).format(4,5) : ", f, y) << x.format(4,5);
        }
    }

    Serial << crlf << "-- FINISHED --";
}

/*** the loop procedure (called every event) */
void loop() {
}