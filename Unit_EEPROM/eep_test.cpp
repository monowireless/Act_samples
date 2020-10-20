#include <TWELITE>

/*** small class to address with 256bytes' block number */
class _n_area {
    int _n;
    static const int BLKSIZ = 256; // note: it's not EEPROM's sector size.
    static const int BLKNUM = EEPROM.size() / BLKSIZ;
public:
    _n_area(int n = 0) : _n(n) {} // construct with block number
    int operator = (int n) { return _n = n; } // assgn block number
    operator int () { return _n; } // get block number
    int get_addr(int offset) { return _n * BLKSIZ + offset; } // get address from offset value
    int get_offset(int addr) { return addr - _n * BLKSIZ; } // get offset from address value
    int begin() { return get_addr(0); } // beginning index
    int end() { return get_addr(BLKSIZ); } // ending index + 1
    void operator --() { _n--; if (_n <= 0) _n = BLKNUM - 1; }
    void operator ++() { _n++; if (_n >= BLKNUM) _n = 0; }
} n_area;

/*** the setup procedure (called on boot) */
void setup() {
    Serial << "\033[2J\033[H";
    Serial << "--- EEPROM TESTING (press 'h' to show help)";
}

/*** the loop procedure (called every event) */

void loop() {
    uint32_t tick_start, tick_end;
    if (Serial.available()) {
        int c = Serial.read();
        switch(c) {
        case 'h':
            Serial << crlf;
            Serial << crlf << "0..9: select block and show the block";
            Serial << crlf << "Enter: show the current block";
            Serial << crlf << "c: clear area (fill w/ 0xFF)";
            Serial << crlf << "z: clear area (fill w/ 0x00)";
            Serial << crlf << "r: random pattern";
            Serial << crlf << "p: write incrementally (0..255)";
            Serial << crlf << "u: using update()";
            Serial << crlf << "s: using stream helper";
        break;

        case '\r': case '<': case '>':
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            {
                if (c >= '0' && c <= '9') { n_area = c - '0'; }
                else if (c == '<') { --n_area; } 
                else if (c == '>') { ++n_area; }

                Serial << "\033[2J\033[H" << format("[%04x-%04x]", n_area.begin(), n_area.end() - 1);
                for (int i = n_area.begin(); i < n_area.end(); i++) {
                    int offset = n_area.get_offset(i);
                    if ((offset & 15) == 0) {
                        Serial.flush(); 
                        Serial << crlf << format("%04x:", i);
                    }
                    Serial << format(" %02x", EEPROM.read(i));
                }
            }
        break;

        case 'c':
            Serial << crlf << crlf << "Write current block with 0xFF..";
            tick_start = millis();
            for (int i = n_area.begin(); i < n_area.end(); i++) {
                EEPROM.write(i, 0xFF);
            }
            tick_end = millis();
            Serial << "(took " << int(tick_end - tick_start) << "ms)";
        break;
        
        case 'z':
            Serial << crlf << crlf << "Write current block with 0x00..";
            tick_start = millis();
            for (int i = n_area.begin(); i < n_area.end(); i++) {
                EEPROM.write(i, 0x00);
            }
            tick_end = millis();
            Serial << "(took " << int(tick_end - tick_start) << "ms)";
        break;

        case 'p':
            Serial << crlf << crlf << "Write current block with incremental values..";
            tick_start = millis();
            for (int i = n_area.begin(); i < n_area.end(); i++) {
                EEPROM.write(i, n_area.get_offset(i) & 0xFF);
            }
            tick_end = millis();
            Serial << "(took " << int(tick_end - tick_start) << "ms)";
        break;

        case 'r':
            Serial << crlf << crlf << "Write current block with random values..";
            tick_start = millis();
            for (int i = n_area.begin(); i < n_area.end(); i++) {
                
                int v = random(0, 255) & 0xFF;
                EEPROM.write(i, v);

                int offset = n_area.get_offset(i);
                if ((offset & 15) == 0) {
                    Serial.flush(); 
                    Serial << crlf << format("%04x:", i);
                }
                Serial << format(" %02x", v);
            }
            tick_end = millis();
            Serial << crlf << "..(took " << int(tick_end - tick_start) << "ms)";
        break;

        case 'u':
            Serial << crlf << crlf << "Update current block by randomly(10%) changing value.";
            tick_start = millis();
            for (int i = n_area.begin(); i < n_area.end(); i++) {
                int r = random(0, 9);
            
                uint8_t v = EEPROM.read(i);
                if (!r) v++; // if hit 10% chance, set a different from EEPROM value.

                EEPROM.update(i, v);

                int offset = n_area.get_offset(i);
                if ((offset & 15) == 0) {
                    Serial.flush(); 
                    Serial << crlf << format("%04x:", i);
                }
                if (!r) {
                    Serial << format(" \033[7m%02x\033[0m", v);
                } else {
                    Serial << format(" %02x", v);
                }
            }
            tick_end = millis();
            Serial << crlf << "..(took " << int(tick_end - tick_start) << "ms)";
        break;

        case 's':
            {
                // get stream helper
                auto strm = EEPROM.get_stream_helper();
                
                // write data
                strm.seek(n_area.begin());
                uint8_t msg_hello[16] = "HELLO WORLD!";

                strm << format("%08x", 12345678);
                strm << uint32_t(0x12ab34cd);
                strm << msg_hello; // NOTE: put 16bytes of msg_hello[16].

                // read data
                strm.seek(n_area.begin());
                uint8_t msg1[8];
                strm >> msg1;
                Serial << crlf << "MSG1=" << msg1;

                uint32_t var1;
                strm >> var1;
                Serial << crlf << "VAR1=" << format("%08x", var1);

                uint8_t msg2[16];
                strm >> msg2;
                Serial << crlf << "MSG2=" << msg2;
            }
        break;
        }
    }
    
}